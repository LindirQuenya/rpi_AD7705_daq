#include "AD7705Comm.h"

#include "gpio-sysfs.h"

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
	// enables sysfs entry for the GPIO pin
	SysGPIO dataReadyGPIO(ad7705comm->drdy_GPIO);
	dataReadyGPIO.gpio_export();
	// set to input
	dataReadyGPIO.gpio_set_dir(false);
	// set interrupt detection to falling edge
	dataReadyGPIO.gpio_set_edge("falling");
	// get a file descriptor for the GPIO pin
	int sysfs_fd = dataReadyGPIO.gpio_fd_open();	
	ad7705comm->running = 1;
	while (ad7705comm->running) {
		// let's wait for data for max one second
		// goes to sleep until an interrupt happens
		int ret = dataReadyGPIO.gpio_poll(sysfs_fd,1000);
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
	close(sysfs_fd);
	dataReadyGPIO.gpio_unexport();
}


void AD7705Comm::start(SamplingRate samplingRate) {
	if (daqThread) {
		throw "Called while DAQ is already running.";
	}
	if (samplingRate & (~0x3)) {
		throw "Invalid sampling rate requested.";
	}
	// resets the AD7705 so that it expects a write to the communication register
	fprintf(stderr,"Sending reset.\n");
	writeReset(fd);
	
	// tell the AD7705 that the next write will be to the clock register
	writeReg(fd,0x20);
	// write 00000100 : CLOCKDIV=0,CLK=1,expects 4.9152MHz input clock, sampling rate
	writeReg(fd,0x04 | samplingRate);
	
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
