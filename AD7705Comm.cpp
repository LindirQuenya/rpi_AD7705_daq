#include "AD7705Comm.h"


AD7705Comm::AD7705Comm(const char* spiDevice) {
	fd = open(spiDevice, O_RDWR);
	if (fd < 0)
		throw "can't open device";
	
	// set SPI mode
	int ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		throw "can't set spi mode";
	} else {
		fprintf(stderr,"SPI mode %d set (ret=%d).\n",mode,ret);
	}
	
	// enable master clock for the AD
	// divisor results in roughly 4.9MHz
	// this also inits the general purpose IO
	gz_clock_ena(GZ_CLK_5MHz,5);
	fprintf(stderr,"Main ADC clock enabled.\n");
	
	// enables sysfs entry for the GPIO pin
	gpio_export(drdy_GPIO);
	// set to input
	gpio_set_dir(drdy_GPIO,0);
	// set interrupt detection to falling edge
	gpio_set_edge(drdy_GPIO,"falling");
	// get a file descriptor for the GPIO pin
	sysfs_fd = gpio_fd_open(drdy_GPIO);	
}

void AD7705Comm::setCallback(AD7705callback* cb) {
	ad7705callback = cb;
}

int AD7705Comm::spi_transfer(int fd, uint8_t* tx, uint8_t* rx, int n) {
	struct spi_ioc_transfer tr;
	memset(&tr,0,sizeof(struct spi_ioc_transfer));
	tr.tx_buf = (unsigned long)tx;
	tr.rx_buf = (unsigned long)rx;
	tr.len = n;
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bpw;	
	int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	return ret;
}

void AD7705Comm::writeReset(int fd) {
	uint8_t tx[5] = {0xff,0xff,0xff,0xff,0xff};
	uint8_t rx[5] = {0};
	
	int ret = spi_transfer(fd, tx, rx, 5);
	
	if (ret < 1) {
		printf("\nerr=%d when trying to reset. \n",ret);
		throw "Can't send spi message";
	}
}

void AD7705Comm::writeReg(int fd, uint8_t v) {
	uint8_t tx = v;
	uint8_t rx = 0;
	
	int ret = spi_transfer(fd, &tx, &rx, 1);
	if (ret < 1)
		throw "can't send spi message";
}

uint8_t AD7705Comm::readReg(int fd) {
	uint8_t tx = 0;
	uint8_t rx = 0;
	
	int ret = spi_transfer(fd, &tx, &rx, 1);
		if (ret < 1)
			throw "can't send spi message";
		
		return rx;
}

int AD7705Comm::readData(int fd) {
	uint8_t tx[2] = {0,0};
		uint8_t rx[2] = {0,0};
		
		int ret = spi_transfer(fd, tx, rx, 2);
		if (ret < 1) {
			printf("\n can't send spi message, ret = %d\n",ret);
			throw "Can't send spi message";
		}
		
		return (rx[0]<<8)|(rx[1]);
}


void AD7705Comm::run(AD7705Comm* ad7705comm) {
	ad7705comm->running = 1;
	while (ad7705comm->running) {
		// let's wait for data for max one second
		// goes to sleep until an interrupt happens
		int ret = gpio_poll(ad7705comm->sysfs_fd,1000);
		if (ret<1) {
			fprintf(stderr,"Poll error %d\n",ret);
			throw "Interrupt timeout";
		}
		
		// tell the AD7705 to read the data register (16 bits)
		ad7705comm->writeReg(ad7705comm->fd,0x38);
		// read the data register by performing two 8 bit reads
		int value = ad7705comm->readData(ad7705comm->fd)-0x8000;
		if (ad7705comm->ad7705callback) {
			ad7705comm->ad7705callback->hasSample(value);
		}
	}
	close(ad7705comm->fd);
	gpio_fd_close(ad7705comm->sysfs_fd);
}


void AD7705Comm::start() {
	if (daqThread) {
		throw "Called while DAQ is running.";
	}
	// resets the AD7705 so that it expects a write to the communication register
	fprintf(stderr,"Sending reset.\n");
	writeReset(fd);
	
	// tell the AD7705 that the next write will be to the clock register
	writeReg(fd,0x20);
	// write 00001100 : CLOCKDIV=1,CLK=1,expects 4.9152MHz input clock, sampling rate 50Hz
	writeReg(fd,0x0C);
	
	// tell the AD7705 that the next write will be the setup register
	writeReg(fd,0x10);
	// intiates a self calibration and then after that starts converting
	writeReg(fd,0x40);
	
	fprintf(stderr,"Receiving data.\n");
	
	daqThread = new std::thread(run,this);
}


void AD7705Comm::stop() {
	running = 0;
	if (daqThread) {
		daqThread->join();
		delete daqThread;
		daqThread = NULL;
	}
}
