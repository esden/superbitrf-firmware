superbitrf-firmware
===================

Firmware for the superbit open source radio system. This is part of a paparazzi SuperbitRF project http://paparazzi.enac.fr/wiki/SuperbitRF.
The SuperbitRF firmware is capable of transmitting, receiving and intercepting DSM2 and DSMX radio communication which is used by Spektrum transmitters and receivers. It is capable of adding a data communication next to the radio control communication for the Paparazzi autopilot.

Compiler:
========

e.g. default paparazzi compiler:

PREFIX=/opt/paparazzi/arm-multilib/bin/arm-none-eabi

e.g. summon arm toolchain:

PREFIX=~/sat/bin/arm-none-eabi


Programs:
========

DSM2/DSMX transmitter, DSM2/DSMX with telemetry and DSM2/DSMX receiver can be compiled from the ./src/ directory.
You can change the configuration in runtime(not made yet), and trough editting the modules/config.c file.

There are also several exaples to test the hardware which are available in the ./examples directory.

