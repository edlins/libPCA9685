libPCA9685 README

        This is a library for fast and efficient control of a PCA9685
        16-channel 12-bit PWM/servo driver via an I2C interface on a
        Raspberry Pi platform.
        This library was written to provide methods for both reading and
        writing all PWM channel values as quickly as possible.

        Features include:
        * ISO 2011 C source code
        * Userspace devlib library (no root required)
        * Bulk update all PWM channels in one ioctl() transaction
        * Bulk read any/all registers in one ioctl() combined transaction
        * Minimal dependencies (does not require wiringPi)
        * Not using SMBus functions with their limitations
        * MIT License

        This library bypasses the smbus library and the read() and write()
        functions and provides functions for direct I2C control via ioctl()
        which allows for fast and efficient block reading and writing of
        all registers using I2C combined transactions.

        The quickstart example application computes 16 new random
        PWM values and writes them to the PCA9685.
        All of this happens in a single 16-channel "refresh".

        The PCA9685demo example application allows the user to set each
        channel manually or press 'a' to activate automatic mode.

        Copyright (c) 2016 Scott Edlin
        edlins ta yahoo tod com


DEPENDENCIES

        libPCA9685 requires two working and loaded I2C kernel modules
        (i2c_bcm-2708 and i2c_dev) in order to access the I2C bus via the
        /dev/i2c-N device files.
        On the intended Raspbian platforms, as of 20160819, this is
        accomplished by adding the following lines to the /boot/config.txt
        file:

        dtparam=i2c_arm=on
        dtparam=i2c_arm_baudrate=1000000

        The first line enables the I2C system and the second line raises
        the baudrate from the default 100kHz to 1MHz which is the speed
        of Fast-mode plus (Fm+) devices such as the PCA9685.
        Note that with short wire runs and minimal RFI/EMI I have been
        able to run the PCA9685 on the I2C bus at 2MHz without errors,
        which effectively doubles the refresh rate but YMMV.

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

        And the file /sys/module/i2c_bcm2708/parameters/combined should
        contain the one line:

        Y


CONNECTION

        Connect the PCA9685's SDA and SCL pins to the Pi's designated
        special purpose I2C pins for SDA and SCL.
        On a Raspberry Pi B+ these pins are physical pins 3 and 5.
        Connect the PCA9685's Vdd pin to 3.3V (B+ phys pin 1 or 17).
        Connect the PCA9685's Vss pin to GND (6, 9, 14, 20, 25, 30, 34, 39).
        If using multiple power supplies, make sure their GNDs are
        connected to each other, and connected to one of the Pi's GND pins.

        After satisfying the dependencies and making the connections
        you should be able to run:

        $ i2cdetect 1

        Which should output:

             0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
        00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
        10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
        20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
        30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
        40: 40 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
        50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
        60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
        70: -- -- -- -- -- -- -- --                         

        Which shows the PCA9685 responding at address 0x40 on I2C bus 1.


INSTALL

        You can include PCA9685.h and PCA9685.c directly in your project
        or compile the object file and use it as a dynamically linked
        library instead.
        To compile the library, navigate into the src folder and run:

        $ make
        $ sudo make install

        This will install libPCA9685.so in your /usr/local/lib directory,
        and PCA9685.h in your /usr/local/include directory.
        To include the files add the following line to your source:

        #include <PCA9685.h>

        And include libPCA9685 in your linking:

        -lPCA9685

        An example application is included in the examples/ folder.


CONTRIBUTING

	Your contributions are very welcome!  Please develop contributions
	against the "develop" branch and submit PRs against "develop".
	PRs against "master" will not be accepted.  For all but trivial
	contributions please open an Issue for discussion first.


FUNCTIONS

        Most projects will only require the first three functions below
        in order to open an I2C bus, initialize a PCA9685 device, and
        begin setting PWM values.  The fourth function may be used to
        read the PWM values after an update as a sanity check.


        ----------------------------------------------------------------
        int PCA9685_openI2C(unsigned char adpt, unsigned char addr);
        ----------------------------------------------------------------
        adpt:        adapter number ("1" in most cases)
        addr:        default I2C slave address (only matters if using
                     read() and write() instead of the library functions)
        returns:     file descriptor for the I2C bus or negative
                     for an error

        Opens the I2C bus device file, sets the default I2C slave address,
        and returns a file descriptor for use with the subsequent functions.


        ----------------------------------------------------------------
        int PCA9685_initPWM(int fd, unsigned char addr, unsigned int freq);
        ----------------------------------------------------------------
        fd:          file descriptor for an I2C bus
        addr:        I2C slave address of the PCA9685 (default "0x40")
        freq:        PWM frequency for the PCA9685 (24 - 1526, in Hz)
        returns:     zero for success, non-zero for failure

        Performs an all-devices software reset on an I2C bus, turns off
        all PWM outputs on a PCA9685 device, sets the PWM frequency on the
        PCA9685, and sets the MODE1 register to 0x20 (auto-increment).


        ----------------------------------------------------------------
        int PCA9685_setPWMVals(int fd, unsigned char addr,
                               unsigned int* onVals, unsigned int* offVals);
        ----------------------------------------------------------------
        fd:          file descriptor for an I2C bus
        addr:        I2C slave address of the PCA9685
        onVals:      array of values used to set the LEDnON registers
        offVals:     array of values used to set the LEDnOFF registers
        returns:     zero for success, non-zero for failure

        Updates all PWM register values on a PCA9685 device based on an
        array of length _PCA9685_CHANS (16).
        Each PWM channel has a pair of ON registers and a pair of OFF
        registers.
        This function sets the ON and OFF registers to the low
        12-bits of the corresponding values in the onVals and offVals
        arrays (turn on at a delay offset between 0 and 4095, and turn off
        at a delay offset between 0 and 4095).
        Larger differences between onVals and offVals correspond to longer
        pulse widths which correspond to brighter intensities.
        off-on <= 0 is full off and off-on >= 4095 is full on.


        ----------------------------------------------------------------
        int PCA9685_getPWMVals(int fd, unsigned char addr,
                               unsigned int* onVals, unsigned int* offVals);
        ----------------------------------------------------------------
        fd:          file descriptor for an I2C bus
        addr:        I2C slave address of the PCA9685
        onVals:      array to populate with the LEDnON register values
        offVals:     array to populate with the LEDnOFF register values
        returns:     zero for success, non-zero for failure

        Reads all PWM registers and populates the onVals and offVals arrays
        with the 12-bit values in a single combined transaction
        (write/read with a ReStart).
        For a validated update, call PCA9685_setPWMVals with two arrays to
        write, then call PCA9685_getPWMVals with two arrays to read, and
        then compare the elements of the two pairs of arrays and they
        should be identical.


TODO

        dev locking?
        many other things..
