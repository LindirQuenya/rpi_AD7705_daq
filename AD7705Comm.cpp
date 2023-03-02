#include "AD7705Comm.h"

#include <fcntl.h>
#include <poll.h>
#include <string.h>

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 256

AD7705Comm::AD7705Comm(AD7705settings settings) {
	fd = open(settings.spiDevice.c_str(), O_RDWR);
	if (fd < 0)
		throw "Can't open device";
	
	// set SPI mode
	int ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		throw "can't set spi mode";
	} else {
#ifdef DEBUG
		fprintf(stderr,"SPI mode %d set (ret=%d).\n",mode,ret);
#endif
	}
}

AD7705Comm::~AD7705Comm() {
	stop();
	close(fd);
}


void AD7705Comm::registerCallback(AD7705callback* cb) {
	ad7705callback = cb;
}

void AD7705Comm::unRegisterCallback() {
	ad7705callback = nullptr;
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
#ifdef DEBUG
		printf("\nerr=%d when trying to reset. \n",ret);
#endif
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

int16_t AD7705Comm::readData(int fd) {
	uint8_t tx[2] = {0,0};
		uint8_t rx[2] = {0,0};
		
		int ret = spi_transfer(fd, tx, rx, 2);
		if (ret < 1) {
			printf("\n can't send spi message, ret = %d\n",ret);
			throw "Can't send spi message";
		}
		
		return (rx[0]<<8)|(rx[1]);
}


void AD7705Comm::run() {
	// get a file descriptor for the IRQ GPIO pin
	int sysfs_fd = getSysfsIRQfd(drdy_GPIO);
	running = 1;
	while (running) {
		// let's wait for data for max one second
		// goes to sleep until an interrupt happens
		int ret = fdPoll(sysfs_fd,1000);
		if (ret<1) {
#ifdef DEBUG
			fprintf(stderr,"Poll error %d\n",ret);
#endif
			throw "Interrupt timeout";
		}
		
		// tell the AD7705 to read the data register (16 bits)
		writeReg(fd, commReg() | 0x38);
		// read the data register by performing two 8 bit reads
		const float norm = 0x8000;
		const float value =(readData(fd))/norm *
			ADC_REF / pgaGain();
		if (nullptr != ad7705callback) {
			ad7705callback->hasSample(value);
		}
	}
	close(sysfs_fd);
	gpio_unexport(drdy_GPIO);
}


void AD7705Comm::start() {
  if (running) return;

#ifdef DEBUG
	fprintf(stderr,"Sending reset.\n");
#endif
	writeReset(fd);

	// tell the AD7705 that the next write will be to the clock register
	writeReg(fd,commReg() | 0x20);
	// write 00000100 : CLOCKDIV=0,CLK=1,expects 4.9152MHz input clock, sampling rate
	writeReg(fd,0x04 | ad7705settings.samplingRate);
	
	// tell the AD7705 that the next write will be the setup register
	writeReg(fd,commReg() | 0x10);
	// intiates a self calibration and then converting starts
	writeReg(fd,0x40 | (ad7705settings.mode << 2) | ( ad7705settings.pgaGain << 3) );

#ifdef DEBUG
	fprintf(stderr,"Starting DAQ thread.\n");
#endif
	
	daqThread = std::thread(&AD7705Comm::run,this);
}


void AD7705Comm::stop() {
	if (!running) return;
	running = 0;
	daqThread.join();
#ifdef DEBUG
	fprintf(stderr,"DAQ thread stopped.\n");
#endif	
}

int AD7705Comm::getSysfsIRQfd(int gpio) {
	  int fd, len;
	  char buf[MAX_BUF];
 
	  fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	  if (fd < 0) {
#ifdef DEBUG
		  perror("gpio/export");
#endif
		  return fd;
	  }
	  
	  len = snprintf(buf, sizeof(buf), "%d", gpio);
	  int r = write(fd, buf, len);
	  close(fd);
	  if (r < 0) {
#ifdef DEBUG
		  perror("gpio/export");
#endif
		  return fd;
	  }
 
	  snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
	  
	  fd = open(buf, O_WRONLY);
	  if (fd < 0) {
#ifdef DEBUG
		  perror("gpio/direction");
#endif
		  return fd;
	  }
	  
	  r = write(fd, "in", 3);
	  close(fd);
	  if (r < 0) {
#ifdef DEBUG
		  perror("gpio/export");
#endif
		  return fd;
	  }
 
	  const char edge[] = "falling";
	  snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);
	  
	  fd = open(buf, O_WRONLY);
	  if (fd < 0) {
#ifdef DEBUG
		  perror("gpio/set-edge");
#endif
		  return fd;
	  }
	  
	  r = write(fd, edge, strlen(edge) + 1); 
	  close(fd);
	  if (r < 0) {
#ifdef DEBUG
		  perror("gpio/export");
#endif
		  return fd;
	  }
	  
	  snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
	  
	  fd = open(buf, O_RDONLY | O_NONBLOCK );
	  if (fd < 0) {
#ifdef DEBUG
		  perror("gpio/fd_open");
#endif
	  }
	  return fd;
}

void AD7705Comm::gpio_unexport(int gpio) {
	int fd, len;
	char buf[MAX_BUF];
	
	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
#ifdef DEBUG
		perror("gpio/unexport");
#endif
	}
	
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	int r = write(fd, buf, len);
	if (r < 0) {
#ifdef DEBUG
		perror("gpio/export");
#endif
	}

	close(fd);
}


int AD7705Comm::fdPoll(int gpio_fd, int timeout)
{
	struct pollfd fdset[1];
	int nfds = 1;
	int rc;
	char buf[MAX_BUF];
	
	memset((void*)fdset, 0, sizeof(fdset));
	
	fdset[0].fd = gpio_fd;
	fdset[0].events = POLLPRI;
	
	rc = poll(fdset, nfds, timeout);
	
	if (fdset[0].revents & POLLPRI) {
		// dummy read
		int r = read(fdset[0].fd, buf, MAX_BUF);
		if (r < 0) {
#ifdef DEBUG
			perror("gpio/export");
#endif
			return r;
		}
	}

	return rc;
}
