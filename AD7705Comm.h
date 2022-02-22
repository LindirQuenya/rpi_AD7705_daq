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
#include <string>

#ifndef NDEBUG
#define DEBUG
#endif


/**
 * Callback for new samples which needs to be implemented by the main program.
 * The function hasSample needs to be overloaded in the main program.
 **/
class AD7705callback {
public:
	/**
	 * Called after a sample has arrived.
	 **/
	virtual void hasSample(float sample) = 0;
};

struct AD7705settings {
	std::string spiDevice = "/dev/spidev0.0";
	
	/**
	 * Sampling rates
	 **/
	enum SamplingRates {
		FS50HZ  = 0,
		FS60HZ  = 1,
		FS250HZ = 2,
		FS500HZ = 3
	};

	/**
	 * Sampling rate requested
	 **/
	SamplingRates samplingRate = FS50HZ;

	/**
	 * Gains of the PGA
	 **/
	enum PGAGains {
		G1   = 0,
		G2   = 1,
		G4   = 2,
		G8   = 3,
		G16  = 4,
		G32  = 5,
		G64  = 6,
		G128 = 7
	};

	/**
	 * Requested gain
	 **/
	PGAGains pgaGain = G1;

	/**
	 * Channel indices
	 **/
	enum AIN {
		AIN1 = 0,
		AIN2 = 1
	};

	/**
	 * Requested input channel (0 or 1)
	 **/
	AIN channel = AIN1;

	/**
	 * Unipolar or bipolar mode
	 **/
	enum Modes {
		Bipolar  = 0,
		Unipolar = 1
	};

	/**
	 * Unipolar or biploar
	 **/
	Modes mode = Unipolar;
};


/**
 * This class reads data from the AD7705 in the background (separate
 * thread) and calls a callback function whenever data is available.
 **/
class AD7705Comm {

public:
	/**
	 * Constructor. Opens the SPI device and waits for to start
	 * the acquisition.
	 * \param settings All AD7705 settings.
	 **/
	AD7705Comm(AD7705settings settings);

	/**
	 * Destructor which makes sure the data acquisition
	 * has stopped.
	 **/
	~AD7705Comm();
       
	/**
	 * Registers the callback which is called whenever there is a sample.
	 * \param cb Pointer to the callback interface.
	 **/
	void registerCallback(AD7705callback* cb);

	/**
	 * Unregisters the callback to the callback interface.
	 **/
	void unRegisterCallback();

	/**
	 * Starts the data acquisition
	 **/
	void start();

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
	static constexpr float ADC_REF = 2.5;
	int fd = 0;
	std::thread* daqThread = nullptr;
	int running = 0;
	AD7705callback* ad7705callback = nullptr;
	AD7705settings ad7705settings;

	int spi_transfer(int fd, uint8_t* tx, uint8_t* rx, int n);
	void writeReset(int fd);
	void writeReg(int fd, uint8_t v);
	uint8_t readReg(int fd);
	int16_t readData(int fd);
	static void run(AD7705Comm* ad7705comm);

        inline float pgaGain() {
		return (float)(1 << ad7705settings.pgaGain);
	}

	inline uint8_t commReg() {
		return ((uint8_t)(ad7705settings.channel));
	}

	static int getSysfsIRQfd(int gpio);
	static int fdPoll(int gpio_fd, int timeout);
	static void gpio_unexport(int gpio);
};


#endif
