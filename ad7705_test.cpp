/*
 * AD7705 test/demo program for the Raspberry PI
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 * Copyright (c) 2013-2020  Bernd Porr <mail@berndporr.me.uk>
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

#include "AD7705Comm.h"

// Handler which receives the data and prints it on the
// screen.
class AD7705printSampleCallback : public AD7705callback {
	virtual void hasSample(int v) {
		printf("v = %d\n",v);
	}
};

// Creates an instance of the AD7705 class.
// Registers the callback.
// Prints data till the user presses a key.
int main(int argc, char *argv[]) {
	AD7705Comm ad7705comm;
	AD7705printSampleCallback ad7705printSampleCallback;
	ad7705comm.setCallback(&ad7705printSampleCallback);
	ad7705comm.start();
	getchar();
	ad7705comm.stop();
	return 0;
}
