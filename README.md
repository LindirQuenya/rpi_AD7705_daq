# Data acquisition with the AD7705 on the raspberry PI!

The AD7705 is a two channel sigma delta converter which has
differential inputs, a PGA and programmable data rates. It's
perfect for slowly changing inputs such as pressure, temperature,
heart rate etc.

This repo offers the class `AD7705Comm` which does all the low level
communications with the AD7705. The user just need to register a
callback handler which then returns the samples in realtime.

The class uses the DRDY of the AD7705 connected to Port 22 and
waits for a falling edge on this port to read
the data. This is done via interrupts / poll
so that the ADC process sleeps until new data becomes
available.

Further down I demonstrate how to use UDP to transmit the data to a web
page via PHP, JSON packets and client-side javascript!

## Hardware design

The design files (for EAGLE) are in the subdirectory "pcb".

![alt tag](circuit.png)

## Building the demo / test:

We use a submodule (another git repo within this repo) do:
```
git submodule init
git submodule update
```

Install the following packages:
```
apt-get install libcurl4-openssl-dev
apt-get install libfcgi-dev
```

To build:

    cmake .

    make

## Running

Run it with the command:

    sudo ./ad7705_test

## General usage

The online doc is here: http://berndporr.github.io/rpi_AD7705_daq/

### Callback handler

```
class AD7705printSampleCallback : public AD7705callback {
	virtual void hasSample(int v) {
		// process your sample here
		printf("v = %d\n",v);
	}
};
```

### Main program

Instantiate the AD7705 class and the callback handler:
```
	AD7705Comm ad7705comm;
	AD7705printSampleCallback ad7705printSampleCallback;
	ad7705comm.setCallback(&ad7705printSampleCallback);
```

Start the data acquisition:
```
	ad7705comm.start(AD7705Comm::SAMPLING_RATE_50HZ);
```
once `start` has been called the data will be arriving.

Stop the data acquisition:
```
	ad7705comm.stop();
```


## Website which shows realtime data using FastCGI

FastCGI is a concept where a webserver connects to an internal
fastcgi-server running on a certain port or socket which provides
the realtime data. Here, the fastCGI server is our AD7705
measurement program which continously measures the data from a
temperature sensor.

### FastCGI server

Start `ad7705fastcgi`
in the background with:
```
nohup ./ad7705fastcgi &
```
which creates a socket under `\tmp\adc7705socket` to communicate with
the fastcgi server.

### Configuring the nginx for FastCGI

 1. copy the the nginx config file `website/nginx-sites-enabled-default` to your
    nginx config directory `/etc/nginx/sites-enabled/default`.
 2. copy `website/index.html` to `/var/www/html`.
 
Then point your web-browser to your raspberry pi. You should see the current
temperatue reading on the screen and a plot with dygraph.

The demo uses JSON packages which are retrieved via the server
subdirectory `/data/`. Here, it's just a timestamp in epoch time and
the temperature. The class `json_fastcgi_web_api.h` is not just a
wrapper for the fast-CGI method but is also able to package JSON which
can then be read by JavaScipt in the browser.

See https://github.com/berndporr/json_fastcgi_web_api for the complete
documentation of the fastCGI server.

![alt tag](screenshot.png)


## Author: Bernd Porr

bernd.porr@glasgow.ac.uk
mail@berndporr.me.uk
www.berndporr.me.uk
