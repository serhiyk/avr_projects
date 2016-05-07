# Arduino clock

### Component list

 * Arduino pro mini
 * Display modules based on MAX7219 (6x2)
 * RTC based on DS3231
 * PIR sensor (optional)
 * Light sensor (optional)

### Compiling the source

I've tested the compiling process only on Ubuntu 14.04

#### Install the required software:

`sudo apt-get install gcc-avr`
`sudo apt-get install avr-libc`
`sudo apt-get install avrdude`

#### Compile the source:

`cd src/clock_6x2/`
`make all`

#### Upload FW to the device:

`sudo make upload`
