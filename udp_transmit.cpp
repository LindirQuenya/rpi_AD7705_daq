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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <arpa/inet.h>



#include "AD7705Comm.h"

// Handler which receives the data, prints it on the
// screen and sends it out.
class AD7705UDP : public AD7705callback {
private:
	int udpSocket;
	struct sockaddr_in clientAddr;

public:
	AD7705UDP() {
		/*Create UDP socket*/
		udpSocket = socket(PF_INET, SOCK_DGRAM, 0);
		if (-1 == udpSocket) {
			const char tmp[] = "Could net get an UDP socket!\n";
			fprintf(stderr,tmp);
			throw tmp;
		}

		/*Configure settings in address struct*/
		clientAddr.sin_family = AF_INET;
		clientAddr.sin_port = htons(65000);
		clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	
	virtual void hasSample(int v) {
		char buffer[256];
		printf("v = %d\n",v);
		sprintf(buffer,"%d\n",v);
		sendto(udpSocket,buffer,strlen(buffer)+1,0,
		       (const struct sockaddr *) &clientAddr,  
		       sizeof(clientAddr));
	}
	virtual ~AD7705UDP() {
		close(udpSocket);
	}
};

// Creates an instance of the AD7705 class.
// Registers the callback.
// Prints data till the user presses a key.
int main(int argc, char *argv[]) {
	AD7705Comm* ad7705comm = new AD7705Comm();
	AD7705UDP ad7705printSampleCallback;
	ad7705comm->setCallback(&ad7705printSampleCallback);
	ad7705comm->start(AD7705Comm::SAMPLING_RATE_50HZ);
	getchar();
	ad7705comm->stop();
	delete ad7705comm;
	return 0;
}
