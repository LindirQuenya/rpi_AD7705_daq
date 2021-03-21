#ifndef FAST_CGI_H
#define FAST_CGI_H

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

/**
 * C++ wrapper around the fastCGI which is being used by
 * nginx, apache and lighttpd which acts as an external
 * html generator.
 **/
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

#endif
