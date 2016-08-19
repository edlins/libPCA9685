pca9685 README

	This software is a library for fast and efficient control of a
	PCA9685 16-channel 12-bit PWM/servo driver via an I2C interface
	on a Raspberry Pi platform.
	This library was written to provide methods for both reading and
	writing all PWM channel values as quickly as possible.
	Existing implementations are often limited to reading or writing
	a single register at a time, which incurs significant overhead
	of the underlying ioctl() calls when updating all channels one-
	by-one.
	Many implementations do not even have the ability to read arbitrary
	registers due to the lack of support for combined I2C transactions
	with a reStart condition.
	This library was also written to require the fewest dependencies
	and so does not require wiringPi or other such add-on libraries.
	This library bypasses the smbus library and the read() and write()
	functions and provides functions for direct I2C control via ioctl()
	which allows for fast and efficient block reading and writing of
	all registers using I2C combined transactions.

	On my B+, the example application computes new PWM values and
	updates all 16 channels in a single refresh, and completes about
	1520 refreshes per second while consuming 10% of the CPU.
	The calculated throughput is:
        1520 refreshes / second * 64 bytes / refresh = 97,280 Bps = 95 KBps
        95 KBps * 10 bits/byte (inc start, stop, ack, etc) = 950 kbps
	which is very close to the theoretical maximum:
	Fm+ I2C (1MHz) = 1,000,000 bps = 976 kbps (almost 1mbps)

	Copyright (c) 2016 Scott Edlin
	edlins ta yahoo tod com


DEPENDENCIES

	pca9685 requires two working and loaded i2c kernel modules
	(i2c_bcm-2708 and i2c_dev) in order to access the i2c bus via the
	/dev/i2c-N device files.
	On the intended Raspbian platforms, as of 20160819, this is
	accomplished by adding the following lines to the /boot/config.txt
	file:

	dtparam=i2c_arm=on
	dtparam=i2c_arm_baudrate=1000000

	The first line enables the i2c system and the second line raises
	the baudrate from the default 100kHz to 1MHz which is the speed
	of Fast-mode plus (Fm+) devices such as the PCA9685.
	Also, this library requires combined transactions which are not
	enabled by default.  This can be enabled by creating any file
	(e.g. "i2c_repeated_start.conf") in the /etc/modprobe.d folder
	with one line:

	options i2c_bcm2708 combined=1

	After making those two file changes and rebooting, lsmod should
	include:

	i2c_bcm2708
	i2c_dev

	This device file should exist:

	/dev/i2c-1

	The messages log /var/log/messages should contain lines like the
	following (search for "i2c"):

	Aug 17 19:33:53 raspberrypi kernel: [    5.718973]
		i2c /dev entries driver
	[...]
	Aug 17 19:33:53 raspberrypi kernel: [    9.086281]
		bcm2708_i2c 20804000.i2c: BSC 1 Controller at 0x20804000
		(irq 77) (baudrate 1000000)

	And the file /sys/modele/i2c_bcm2708/parameters/combined should
	contain the one line:

	Y


CONNECTION

	Connect the PCA9685's SDA and SCL pins to the Pi's designated
	special purpose I2C pins for SDA and SCL.
	On a Raspberry Pi B+ these pins are physical pins 3 and 5.
        Connect the PCA9685's Vdd pin to 3.3V (B+ phys pin 1 or 17).
        Connect the PCA9685's Vss pin to GND (6, 9, 14, 20, 25, 30, 34, 39).
	If using multiple power supplies, make sure their GND's are
	connected to each other, and connected to one of the Pi's GND pins.

INSTALL

	You can include pca9685.h and pca9685.c directly in your project
	or compile the object file and use it as a dynamically linked
        library instead.
	To compile the library, navigate into the src folder and run:

	$ make
	$ sudo make install

	This will install libpca9685.so in your /usr/local/lib directory,
	and pca9685.h in your /usr/local/include directory.
	To include the files add the following line to your source:

	#include <pca9685.h>

	And include the pca9685 library in your linking:

	-lpca9685

	Enjoy your superfast Pi PWMing!

FUNCTIONS

	Most projects will only require the following three functions used
	in order to open an I2C bus, initialize a PCA9685 device, and
	begin setting PWM values.


	----------------------------------------------------------------
	int setupI2C(int adpt, int addr);
	----------------------------------------------------------------
	adpt:	integer adpt number ("1" in most cases)
	addr:	integer default I2C slave address (only matters if using
		read() and write() instead of the library functions)
	returns	an integer file descriptor for the I2C bus or negative
		for an error

	Opens the I2C bus device file, sets the default I2C slave address,
	and returns a file descriptor for use with the subsequent functions.


	----------------------------------------------------------------
	int initpca(int fd, int addr, int freq);
	----------------------------------------------------------------
	fd:	integer file descriptor for an I2C bus
	addr:	integer I2C slave address of the PCA9685 (default "0x40")
	freq:	integer PWM frequency for the PCA9685 (24 - 1526, in Hz)
	returns an integer, zero for success, non-zero for failure

	Performs an all-devices software reset on an I2C bus, turns off
	all PWM outputs on a PCA9685 device, sets the PWM frequency on the
	PCA9685, and sets the MODE1 to 0x20 (auto-increment).


	----------------------------------------------------------------
	int setEachPwmBatch(int fd, int addr, int len, int* vals);
	----------------------------------------------------------------
	fd:	integer file descriptor for an I2C bus
	addr:	integer I2C slave address of the PCA9685
	len:	length of the vals array, the number of channels to update
	vals:	array of integer values used to set the LEDnOFF registers
	returns an integer, zero for success, non-zero for failure

	Updates all PWM register values on a PCA9685 device based on an
	array of integers.
	Each PWM channel has a pair of ON registers and a pair of OFF
	registers.
	This function sets the ON registers to 0x00 (turn on at the
	beginning of the cycle) and sets the OFF registers to the low
	12-bits of a corresponding integer in the vals array (turn off
	at a delay offset of 0 - 4095).
	Higher vals correspond to longer pulse widths.
	val <=0 is full off and val >= 4095 is full on.



	The following function provides read functionality which most users
	will probably not require.
	There are additional functions providing lower-level access to I2C
	combined transactions.
	Please refer to the pca9685.c source to understand how those work.

	----------------------------------------------------------------
	int readI2C(int fd, unsigned char addr, unsigned char startReg,
	            int len, unsigned char* readBuf);
	----------------------------------------------------------------
	Reads len number of register values on a PCA9685 device into
	an array of unsigned chars.
	The register values are raw unsigned chars, not 12-bit PWM ON and
	OFF values because this function may be used to read any registers
	including the MODE1 register.
	To get raw PWM register values use startReg = 0x06 (the first LED ON
	register), len = 64 (four 1-byte registers per channel * 16
	channels), and a readBuf pointer to a unsigned char array of at
	least a 64-bytes.

	Each PWM channel has two 1-byte ON registers and two 1-byte OFF
	registers.
	The first register is the low 8-bits of the ON value, the second
	register is the high 4-bits of the ON value, the third register is
	the low 8-bits of the OFF value, and the fourth register is the high
	4-bits of the OFF value.  To convert to an integer value, read the
	high 4-bits, left shift that value by 8 bits, and add the low 8-bits
	to get a value 0 - 4095.


TODO

	len vs NUMCHANNELS not used consistently in setEachPwmBatch()
	rename functions more consistently
	use unsigned char for addr throughout (instead of int)
	move #define constants from src into header
	many other things..
