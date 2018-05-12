olaclient README

        This is an application that reads DMX data provided by olad and
        sets the PWM channels of a PCA9685 based on the DMX data.

        The DMX data provided by olad consists of a frame of an entire
        universe.  It is interpreted as 16x 16-bit values starting with
        the most significant byte of LED0 at DMX channel 1 followed by
        the least significant byte of LED0 at DMX channel 2.  The next
        15 LED values are similarly interpreted to reside in DMX channels
        2 - 16.

        The MSBs and LSBs are combined into 16-bit values which are then
        right-shifted by 4 bits to derive the 12-bit PWM values.

        Copyright (c) 2016 - 2018 Scott Edlin
        edlins ta yahoo tod com


DEPENDENCIES

        CMake version 3.0 or later is required to build the application.
        A working C++ compiler is also required.  g++ 4.9.2 works well.

        libPCA9685 0.6 or later is required.  olaclient is bundled in
        versions 0.7 and later.

        RASPBIAN

        OLA (Open Lighting Architecture) is required.  ola 0.10.2 works
        well.  ola 0.9.1 is in the raspbian repository:
        http://mirrordirector.raspbian.org/raspbian/ jessie/main armhf Packages
        however that version has not been tested.  To try it, execute:

        $ sudo apt-get update
        $ sudo apt-get install ola
        $ sudo apt-get install libola-dev

        UBUNTU 14.04 "TRUSTY"

        The ola wiki regarding Ubuntu does not cover Trusty and the Ubuntu
        Trusty repositories do not contain ola packages.  To install ola on
        Trusty from the ola PPA execute:

        $ sudo add-apt-repository ppa:voltvisionfrenchy/ola
        $ sudo apt-get update
        $ sudo apt-get install ola
        $ sudo apt-get install libola-dev


        Once installed and running `ps waux|grep olad` should return
        something like:

        olad 1014 0.0 1.7 68760 7856 ? Sl Apr18 19:23 /usr/bin/olad
              --syslog --log-level 3 --config-dir /var/lib/ola/conf

        The OLA web interface should also be reachable at:

        http://127.0.0.1:9090

        The web interface should indicate an active universe that
        matches both the olaclient DMX_UNIVERSE and the universe of the
        DMX controller.  The universe should also have an input port
        defined using the "E1.31 (DMX over ACN)" enabled.

        The web interface is also useful for troubleshooting the reception
        of DMX data from the controller with the DMX Monitor.

        Finally, of course, a DMX controller is required.  QLC+ is
        a software DMX controller that supports E1.31 streaming ACN
        which is also supported by OLA.  However, QLC+ does not exist
        in the raspbian repositories.  QLC+ may be compiled from source
        on a raspbian platform but it is not recommended due to the very
        long compile time.  QLC+ for raspbian is best cross-compiled in
        a more powerful build environment.

CONNECTION

        Configure the DMX controller to transmit to universe 1 via
        E1.31 to the IP address of the olaclient host.  Configure OLA
        to receive input on universe 1 via E1.31.  Using the DMX Monitor
        verify that OLA can recieve the output from the DMX controller.

        Run the olaclient binary which should output it's version number
        and the PWM channel and value of the recieved DMX data as it is
        received.  At this point, if the PCA9685 is connected and properly
        configured, it's LED outputs should be responding to values sent
        in DMX universe 1 channels 1 - 32, via E1.31, to olad on the host,
        and on to the olaclient which directly communicates with PCA9685.

DOWNLOAD

        olaclient is bundled with libPCA9685 0.7 and later as an example
        application.  For now, the recommended download procedure is the
        same as for other GitHub projects without release packages.  To create
        a new folder in the current working directory containing the latest
        "master" release of the project:

        $ git clone https://github.com/edlins/libPCA9685

        Or, to you clone the latest unreleased "develop" branch:

        $ git clone -b develop https://github.com/edlins/libPCA9685

INSTALL

        olaclient is by default excluded from `make`.  In order to build
        and install the library and build olaclient, execute:

        $ cd libPCA9685 && mkdir build && cd build
        $ cmake ..
        $ make
        $ make olaclient
        $ ctest
        $ sudo make install

        This will install libPCA9685.so in your /usr/local/lib directory,
        and PCA9685.h in your /usr/local/include directory.

        If the library is already built and installed you can just execute
        `make olaclient` from the build/ directory.

        To run olaclient after the build is complete execute:

        $ ./olaclient


CONTRIBUTING

        Your contributions are very welcome!  Please develop contributions
        against the "develop" branch and submit PRs against "develop".
        PRs against "master" will not be accepted.  For all but trivial
        contributions please open an Issue for discussion first.


CONSTANTS

        There are application constants in olaclient.cpp that may be changed
        to effect changes in the operation of the application.


        ----------------------------------------------------------------
        PWM_FREQ - PWM frequency setting
        ----------------------------------------------------------------
        200 (default):   200 Hz PWM frequency
        non-default:     frequency in Hz (24 - 1526)

        example: #define PWM_FREQ 400 // set frequency to 400 Hz


        ----------------------------------------------------------------
        DMX_UNIVERSE - DMX universe to get input from
        ----------------------------------------------------------------
        1 (default):     listen for data in universe 1
        non-default:     listen for data in another universe

        example: #define DMX_UNIVERSE 2 // listen in universe 2


        ----------------------------------------------------------------
        I2C_ADPT - i2c bus number to output to
        ----------------------------------------------------------------
        1 (default):     use /dev/i2c-1 (bus number 1)
        non-default:     use /dev/i2c-n (for bus number n)

        example: #define I2C_ADPT 2 // use bus number 2


        ----------------------------------------------------------------
        I2C_ADDR - i2c PCA9685 address
        ----------------------------------------------------------------
        0x40 (default):  PCA9685 is configured at address 0x40
        non-default:     address configured on PCA9685

        example: #define I2C_ADDR 0x50 // PCA9685 is at address 0x50

TODO

        Use command line parameters instead of preprocessor constants
