#ifndef __AD7705COMM_H
#define __AD7705COMM_H

/*
 * AD7705 class to read data at a given sampling rate
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 * Copyright (c) 2013-2022  Bernd Porr <mail@berndporr.me.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <thread>  




/**
 * Callback for new samples which needs to be implemented by the main program.
 * The function hasSample needs to be overloaded in the main program.
 **/
class AD7705callback {
public:
	/**
	 * Called after a sample has arrived.
	 **/
	virtual void hasSample(int sample) = 0;
};


/**
 * This class reads data from the AD7705 in the background (separate
 * thread) and calls a callback function whenever data is available.
 **/
class AD7705Comm {

public:
	/**
	 * Constructor with the spiDevice. The default device
	 * is /dev/spidev0.0.
	 * \param spiDevice The raw /dev spi device.
	 **/
	AD7705Comm(const char* spiDevice = "/dev/spidev0.0");

	/**
	 * Destructor which makes sure the data acquisition
	 * has stopped.
	 **/
	~AD7705Comm() {
		stop();
	}

	/**
	 * Sampling rates
	 **/
	enum SamplingRate {
		SAMPLING_RATE_50HZ = 0,
		SAMPLING_RATE_60HZ = 1,
		SAMPLING_RATE_250HZ = 2,
		SAMPLING_RATE_500HZ = 3
	};

	/**
	 * Sets the callback which is called whenever there is a sample.
	 * \param cb Pointer to the callback interface.
	 **/
	void setCallback(AD7705callback* cb);

	/**
	 * Starts the data acquisition in the background and the
	 * callback is called with new samples.
	 * \param samplingRate The sampling rate of the ADC.
	 **/
	void start(SamplingRate samplingRate = SAMPLING_RATE_50HZ);

	/**
	 * Stops the data acquistion
	 **/
	void stop();

private:
	const uint8_t mode = SPI_CPHA | SPI_CPOL;
	const int drdy_GPIO = 22;
	const uint32_t speed = 500000;
	const uint16_t delay = 0;
	const uint8_t bpw   = 8;
	int fd = 0;
	std::thread* daqThread = NULL;
	int running = 0;
	AD7705callback* ad7705callback = NULL;

	int spi_transfer(int fd, uint8_t* tx, uint8_t* rx, int n);
	void writeReset(int fd);
	void writeReg(int fd, uint8_t v);
	uint8_t readReg(int fd);
	int readData(int fd);
	static void run(AD7705Comm* ad7705comm);

};


#endif
