superbitrf-firmware
===================

Firmware for the superbit open source radio system. This is part of a paparazzi SuperbitRF project http://paparazzi.enac.fr/wiki/SuperbitRF.
The SuperbitRF firmware is capable of transmitting, receiving and intercepting DSM2 and DSMX radio communication which is used by Spektrum transmitters and receivers. It is capable of adding a data communication next to the radio control communication for the Paparazzi autopilot.

Compiler:
========

We first need the libopencm3. We get it using the git submodules :

    git submodule init
    git submodule update

It's going to download the library (might take up to a few minutes). Then you are ready to compile the firmware, just run :

    make

You might get a few warnings, that's not an issue. Then you just flash the dongle by either using the bootloader or the Black Magic Probe. All new USBRF dongles have the bootloader and to flash using that execute the following command :

    make flash

If you want to flash using the Black Magic Probe then use the following command(where /dev/ttyACM0 the port of your BMP is):

    make flash BMP_PORT=/dev/ttyACM0

If during the flashing operation you get an error concerning the arm toolchain, you can specify the path to where you putted your toolchain as an argument to the make. You can do that by adding a PREFIX option to the make command. For exemple :

	make PREFIX=~/sat/bin/arm-none-eabi


Programs:
========

The main program of USBRF is in the ./src folder. This main program contains multiple protocols and settings. Each of this settings can be editted by connection to the console cdcacm device(also known as /dev/ttyACM1). This will be the second ttyACM port this dongle creates.
When opening a connection by using for example stty or putty, try to enter "help" for the available commands.

There are also several examples to test the hardware which are available in the ./test directory.

