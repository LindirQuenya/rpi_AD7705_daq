/*
 * AD7705 test/demo program for the Raspberry PI
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 * Copyright (c) 2013-2021  Bernd Porr <mail@berndporr.me.uk>
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
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <iostream>
#include "fcgio.h"
#include <thread>
#include <string>

#include "AD7705Comm.h"

// the socket nginx communicates with


class FastCGIHandler {
public:
	class FastCGICallback {
	public:
		virtual std::string getDataString() = 0;
	};
	
private:
	FCGX_Request request;
	int sock_fd = 0;
	int running = 1;
	std::thread* mainThread;
	FastCGICallback* fastCGICallback = nullptr;

public:
	// constructor which inits it and starts the main thread
	FastCGIHandler(FastCGICallback* argFastCGICallback = nullptr,
		       const char socketpath[] = "/tmp/adc7705socket") {
		fastCGICallback = argFastCGICallback;
		// set it to zero
		memset(&request, 0, sizeof(FCGX_Request));
		// init the connection
		FCGX_Init();
		// open the socket
		sock_fd = FCGX_OpenSocket(socketpath, 1024);
		// making sure the nginx process can read/write to it
		chmod(socketpath, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH);
		// init requests so that we can accept requests
		FCGX_InitRequest(&request, sock_fd, 0);
		// starting main loop
		mainThread = new std::thread(FastCGIHandler::exec, this);
		fprintf(stderr,"Listening to CGI requests.\n");
	}

	static void exec(FastCGIHandler* fastCGIHandler) {
		while ((fastCGIHandler->running) && (FCGX_Accept_r(&(fastCGIHandler->request)) == 0)) {
			// create header
			std::string buffer =
				"Content-type: text/html; charset=utf-8\r\n"
				"\r\n";
			// if the callback is not null then call it
			// buffer = buffer + "Bla\r\n";
			if (fastCGIHandler->fastCGICallback != nullptr) {
				buffer = buffer + fastCGIHandler->fastCGICallback->getDataString();
				buffer = buffer + "\r\n";
			} else {
				fprintf(stderr,"BUG: No handler registered.\n");
			}
			FCGX_PutStr(buffer.c_str(), buffer.length(), fastCGIHandler->request.out);
			FCGX_Finish_r(&(fastCGIHandler->request));
		}
	}

	~FastCGIHandler() {
		running = 0;
		shutdown(sock_fd, SHUT_RDWR);
		mainThread->join();
		FCGX_Free(&request, sock_fd);
	}
};


///////////////////////////////////////////////////////////////////////////////


// Below that would be a separate thread in a class
// if part of a larger userspace program.
// Here, it's just running as a background process
// it is just a main program with a
// global variable "running" which is set to zero by
// the signal handler.

int mainRunning = 1;

void sigHandler(int sig) { 
	if((sig == SIGHUP) || (sig == SIGINT)) {
		mainRunning = 0;
	}
}


// sets a signal handler so that you can kill
// the background process gracefully with:
// kill -HUP <PID>
void setHUPHandler() {
	struct sigaction act;
	memset (&act, 0, sizeof (act));
	act.sa_handler = sigHandler;
	if (sigaction (SIGHUP, &act, NULL) < 0) {
		perror ("sigaction");
		exit (-1);
	}
	if (sigaction (SIGINT, &act, NULL) < 0) {
		perror ("sigaction");
		exit (-1);
	}
}


// Handler which receives the data, prints it on the
// screen and sends it out.
class AD7705fastcgicallback : public AD7705callback {
public:
	// TODO: should be a ringbuffer. Data is arriving here
	// continously. Just for the sake of simplicity just
	// the most recent value is stored.
	int currentSample;

	// callback
	virtual void hasSample(int v) {
		currentSample = v;
	}
};


// returns data to the nginx server
class FastCGIADCCallback : public FastCGIHandler::FastCGICallback {
private:
	AD7705fastcgicallback* ad7705fastcgi;

public:
	FastCGIADCCallback(AD7705fastcgicallback* argAD7705fastcgi) {
		ad7705fastcgi = argAD7705fastcgi;
	}
	
	virtual std::string getDataString() {
		std::string data;
		data = std::to_string(ad7705fastcgi->currentSample);
		return data;
	}
};


// Creates an instance of the AD7705 class.
// Registers the callback.
// In a real program that would be a thread!
int main(int argc, char *argv[]) {
	// getting all the ADC related acquistion going
	AD7705Comm* ad7705comm = new AD7705Comm();
	AD7705fastcgicallback ad7705fastcgicallback;
	ad7705comm->setCallback(&ad7705fastcgicallback);

	// getting the FastCGI stuff going
	// The callback which is called when fastCGI needs data
	// gets a pointer to the AD7705 callback class which
	// contains the samples
	FastCGIADCCallback fastCGIADCCallback(&ad7705fastcgicallback);
	
	// starting the fastCGI handler
	FastCGIHandler* fastCGIHandler = new FastCGIHandler(&fastCGIADCCallback);

	// starting the data acquisition
	ad7705comm->start(AD7705Comm::SAMPLING_RATE_50HZ);

	// catching Ctrl-C or kill -HUP so that we can terminate properly
	setHUPHandler();

	fprintf(stderr,"'%s' up and running.\n",argv[0]);

	// just do nothing here and sleep. It's all dealt with in threads!
	while (mainRunning) sleep(1);

	fprintf(stderr,"'%s' shutting down.\n",argv[0]);

	// stopping ADC
	ad7705comm->stop();
	delete ad7705comm;

	// stops the fast CGI handlder
	delete fastCGIHandler;
	
	return 0;
}
