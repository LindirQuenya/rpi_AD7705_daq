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

Github: https://github.com/berndporr/rpi_AD7705_daq
