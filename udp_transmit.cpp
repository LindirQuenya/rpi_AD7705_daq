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
#include <sys/signal.h>


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
		//printf("v = %d\n",v);
		sprintf(buffer,"%d\n",v);
		sendto(udpSocket,buffer,strlen(buffer)+1,0,
		       (const struct sockaddr *) &clientAddr,  
		       sizeof(clientAddr));
	}
	virtual ~AD7705UDP() {
		close(udpSocket);
	}
};


// Below that would be a separate thread in a class
// if part of a larger userspace program.
// Here, it's just running as a background process
// it is just a main program with a
// global variable "running" which is set to zero by
// the signal handler.

int running = 1;

void sigHandler(int sig) { 
    if(sig == SIGHUP) {
	    running = 0;
    }
}

// sets a signal handler so that you can kill
// the background process gracefully with:
// kill -HUP <PID>
void setHandler() {
	struct sigaction act;
	memset (&act, 0, sizeof (act));
	act.sa_handler = sigHandler;
	if (sigaction (SIGHUP, &act, NULL) < 0) {
		perror ("sigaction");
		exit (-1);
	}
}

// Creates an instance of the AD7705 class.
// Registers the callback.
// In a real program that would be a thread!
int main(int argc, char *argv[]) {
	AD7705Comm* ad7705comm = new AD7705Comm();
	AD7705UDP ad7705printSampleCallback;
	ad7705comm->setCallback(&ad7705printSampleCallback);
	ad7705comm->start(AD7705Comm::SAMPLING_RATE_50HZ);
	setHandler();
	while (running) sleep(1);
	ad7705comm->stop();
	delete ad7705comm;
	fprintf(stderr,"\nUDP transmit has terminated.\n");
	return 0;
}
