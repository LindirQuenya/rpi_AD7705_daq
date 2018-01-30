/*
 * AD7705 test program for the Raspberry PI
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 * Copyright (c) 2013-2015  Bernd Porr <mail@berndporr.me.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "gz_clk.h"
#include "gpio-sysfs.h"


static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev0.0";
static const uint8_t mode = SPI_CPHA | SPI_CPOL;
static const int drdy_GPIO = 22;
static const uint32_t speed = 500000;
static const uint16_t delay = 0;
static const uint8_t bpw   = 8;

static int spi_transfer(int fd, uint8_t* tx, uint8_t* rx, int n) {
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

static void writeReset(int fd) {
	uint8_t tx[5] = {0xff,0xff,0xff,0xff,0xff};
	uint8_t rx[5] = {0};

	int ret = spi_transfer(fd, tx, rx, 5);

	if (ret < 1) {
		printf("\nerr=%d when trying to reset. \n",ret);
		pabort("Can't send spi message");
	}
}

static void writeReg(int fd, uint8_t v) {
	uint8_t tx = v;
	uint8_t rx = 0;

	int ret = spi_transfer(fd, &tx, &rx, 1);
	if (ret < 1)
		pabort("can't send spi message");
}

static uint8_t readReg(int fd) {
	uint8_t tx = 0;
	uint8_t rx = 0;

	int ret = spi_transfer(fd, &tx, &rx, 1);
	if (ret < 1)
		pabort("can't send spi message");
	
	return rx;
}

static int readData(int fd) {
	int ret;
	uint8_t tx[2] = {0,0};
	uint8_t rx[2] = {0,0};

	ret = spi_transfer(fd, tx, rx, 2);
	if (ret < 1) {
		printf("\n can't send spi message, ret = %d\n",ret);
		exit(1);
	}
	  
	return (rx[0]<<8)|(rx[1]);
}


int main(int argc, char *argv[]) {
	int ret = 0;
	int fd;
	int sysfs_fd;

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		pabort("can't set spi mode");
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

	// resets the AD7705 so that it expects a write to the communication register
        printf("Sending reset.\n");
	writeReset(fd);

	// tell the AD7705 that the next write will be to the clock register
	writeReg(fd,0x20);
	// write 00001100 : CLOCKDIV=1,CLK=1,expects 4.9152MHz input clock
	writeReg(fd,0x0C);

	// tell the AD7705 that the next write will be the setup register
	writeReg(fd,0x10);
	// intiates a self calibration and then after that starts converting
	writeReg(fd,0x40);

        printf("Receiving data.\n");
	// we read data in an endless loop and display it
	// this needs to run in a thread ideally
	while (1) {

	  // let's wait for data for max one second
	  // goes to sleep until an interrupt happens
	  ret = gpio_poll(sysfs_fd,1000);
	  if (ret<1) {
	    fprintf(stderr,"Poll error %d\n",ret);
	  }

	  // tell the AD7705 to read the data register (16 bits)
	  writeReg(fd,0x38);
	  // read the data register by performing two 8 bit reads
	  int value = readData(fd)-0x8000;
	  fprintf(stderr,"data = %d       \r",value);
	  
	}

	close(fd);
	gpio_fd_close(sysfs_fd);

	return ret;
}
