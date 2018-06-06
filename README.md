libPCA9685 README

        This is a library for fast and efficient control of a PCA9685
        16-channel 12-bit PWM/servo driver via an I2C interface on a
        Raspberry Pi platform.
        This library was written to provide methods for both reading and
        writing all PWM channel values as quickly as possible.

        Features include:
        * ISO 2011 C source code
        * CMake build system
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

        The quickstart example application computes 16 new random PWM
        values and writes them to the PCA9685.

        The PCA9685demo example application allows the user to set each
        channel manually or press 'a' to activate automatic mode.

        The olaclient example application uses the Open Lighting
        Architecture to turn the host into a DMX512 receiver taking
        input via E1.31 over TCP/IP.

        Copyright (c) 2016 - 2018 Scott Edlin
        edlins ta yahoo tod com


DEPENDENCIES

        CMake version 3.0 or later is required to build the library.
        A working C compiler is also required.  gcc 4.9.2 works well.

        The following hardware-specific dependencies apply to the
        Raspbian platform.  This may be adapted to other Debian variants
        but the scope of the differences is not currently known.

        libPCA9685 requires two working and loaded I2C kernel modules
        (i2c_bcm-2708 and i2c_dev) in order to access the I2C bus via the
        /dev/i2c-N device files.  On the intended Raspbian platforms, as
        of this writing, this is accomplished by adding the following lines
        to the /boot/config.txt file:

        dtparam=i2c_arm=on
        dtparam=i2c_arm_baudrate=1000000

        The first line enables the I2C system and the second line raises
        the baudrate from the default 100 kHz to 1 MHz which is the speed
        of Fast-mode plus (Fm+) devices such as the PCA9685.
        Note that with short wire runs and minimal RFI/EMI I have been
        able to run the PCA9685 on the I2C bus at 2 MHz without errors,
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

        crw-rw---- 1 root i2c 89, 1 Oct 16  2017 /dev/i2c-1

        `dmesg | grep i2c` should report something like:

        [   10.386982] i2c /dev entries driver
        [   14.964511] bcm2708_i2c 20804000.i2c: BSC1 Controller at
                       0x20804000 (irq 77) (baudrate 1000000)

        And the file /sys/module/i2c_bcm2708/parameters/combined should
        contain the one line:

        Y

        OLACLIENT

        To build the olaclient, additional ola dependencies must be
        installed.  For more information, see:

        examples/olaclient/README.md


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
        If the PCA9685 is wired to respond at a different address, that
        address should be shown in i2cdetect.


DOWNLOAD

        For now, the recommended download procedure is the same as for
        other GitHub projects without release packages.  To create a new
        folder in the current working directory containing the latest
        "master" release of the project:

        $ git clone https://github.com/edlins/libPCA9685

        Or, to clone the latest unreleased "develop" branch:

        $ git clone -b develop https://github.com/edlins/libPCA9685

INSTALL

        You can include PCA9685.h and PCA9685.c directly in your project
        or compile the object file and use it as a dynamically linked
        library instead.

        To compile and install the library and examples from the location
        where the "clone" was run, execute:

        $ cd libPCA9685 && mkdir build && cd build
        $ cmake ..
        $ make
        $ ctest
        $ sudo make install

        Note that "make" will not attempt to build the examples.  Run
        `make examples` to build the example applications.  Then run
        `cd examples/<app> && sudo make install` to install an example.
        See the README.md files in each example's folder.

        This will install libPCA9685.so in your /usr/local/lib directory,
        and PCA9685.h in your /usr/local/include directory.
        To include the files add the following line to your source:

        #include <PCA9685.h>

        And include libPCA9685 in your linking:

        -lPCA9685

        Example applications are included in the examples/ folder.


CONTRIBUTING

        Your contributions are very welcome!  Please develop contributions
        against the "develop" branch and submit PRs against "develop".
        PRs against "master" will not be accepted.  For all but trivial
        contributions please open an Issue for discussion first.


VARIABLES

        There are extern library variables that may be changed to effect
        changes in the operation of the library.  After #include <PCA9685.h>
        is invoked, these variables may be assigned values from applications
        without any additional declaration.


        ----------------------------------------------------------------
        extern bool _PCA9685_TEST;
        ----------------------------------------------------------------
        0 (default):     test mode is not enabled
        non-zero:        test mode is enabled, hardware calls are faked

        example: _PCA9685_TEST = 1; // enable test mode


        ----------------------------------------------------------------
        extern bool _PCA9685_DEBUG;
        ----------------------------------------------------------------
        0 (default):     debugging output is not enabled
        non-zero:        debugging output is enabled on stdout

        example: _PCA9685_DEBUG = 1; // enable debugging output


        ----------------------------------------------------------------
        extern unsigned char _PCA9685_MODE1;
        ----------------------------------------------------------------
        0x11 (default):  AUTOINC and SLEEP set (hardware default)
        non-default:     set or clear bits defined in PCA9685.h

        example: _PCA9685_MODE1 = _PCA9685_MODE1 | _PCA9685_SUB1BIT;
                 // enable respond to i2c subaddress 1


        ----------------------------------------------------------------
        extern unsigned char _PCA9685_MODE2;
        ----------------------------------------------------------------
        0x04 (default):  OUTDRV set to use totem pole (hardware default)
        non-default:     set or clear bits defined in PCA9685.h

        example: _PCA9685_MODE2 = _PCA9685_MODE2 & ~_PCA9685_OUTDRVBIT;
                 // switch from totem pole to open drain mode


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

        Updates all PWM register values on a PCA9685 device based on two
        arrays of length _PCA9685_CHANS (16).
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


        ----------------------------------------------------------------
        int PCA9685_setAllPWM(int fd, unsigned char addr,
                               unsigned int on, unsigned int off);
        ----------------------------------------------------------------
        fd:          file descriptor for an I2C bus
        addr:        I2C slave address of the PCA9685
        on:          value used to set the on ALLLEDREG register
        off:         value used to set the off ALLLEDREG register
        returns:     zero for success, non-zero for failure

        Updates ALLLEDREG register values on a PCA9685 device based on an
        array of length _PCA9685_CHANS (16).
        The ALLLEDREG PWM channel has a pair of ON registers and a pair of OFF
        registers.
        This function sets the ON and OFF values to the low
        12-bits of the corresponding values in the on and off
        values (turn on at a delay offset between 0 and 4095, and turn off
        at a delay offset between 0 and 4095).
        Larger differences between on and off correspond to longer
        pulse widths which correspond to brighter intensities.
        off-on <= 0 is full off and off-on >= 4095 is full on.


TODO

        CPack release packages
        multiple buses and devices
        ola client example
