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

You might get a few warnings, that's not an issue. Then you just flash the dongle using a black magic probe by launching the script :

    ./flash.sh

If your black magic probe if on an other port than the default one (/dev/ttyACM0), you can select the right one by editing the last line of the file : src/Makefile.

If during the flashing operation you get an error concerning the arm toolchain, you can specify the path to where you putted your toolchain as an argument to the make. You can do that by adding a PREFIX option the the last line of the file : src/Makefile. For exemple :

	make flash PREFIX=~/sat/bin/arm-none-eabi BMP_PORT=/dev/ttyACM0


Programs:
========

DSM2/DSMX transmitter, DSM2/DSMX with telemetry and DSM2/DSMX receiver can be compiled from the ./src/ directory.
You can change the configuration in runtime(not made yet), and trough editting the modules/config.c file.

There are also several examples to test the hardware which are available in the ./examples directory.

