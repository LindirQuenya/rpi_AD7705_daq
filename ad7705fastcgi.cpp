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

#include "AD7705Comm.h"
#include "FastCGI.h"


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
	float currentTemperature;
	long t;

	// callback
	virtual void hasSample(int v) {
		// crude conversion to temperature
		currentTemperature = (float)v / 65536 * 2.5 * 0.6 * 100;
		// timestamp
		t = time(NULL);
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
		FastCGIHandler::JSONGenerator jsonGenerator;
		jsonGenerator.add("epoch",(long)time(NULL));
		jsonGenerator.add("temperature",ad7705fastcgi->currentTemperature);
		return jsonGenerator.getJSON();
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
	FastCGIHandler* fastCGIHandler = new FastCGIHandler(&fastCGIADCCallback,
							    "/tmp/adc7705socket");

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
