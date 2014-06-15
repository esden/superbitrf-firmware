##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

BINARY = usbrf
LDSCRIPT = ../stm32f103cbt6.ld
TOOLCHAIN_DIR = ../libopencm3

# The modules and helpers used for the usbrf module
OBJS += modules/led.o modules/button.o modules/timer.o modules/cdcacm.o modules/cyrf6936.o modules/config.o
OBJS += helper/convert.o helper/dsm.o

# The different kind of protocols available
OBJS += protocol/dsm_receiver.o protocol/dsm_transmitter.o protocol/dsm_mitm.o

include ../Makefile.include

BOARD?=2

ifeq ($(BOARD),2)
CFLAGS += -DBOARD_V1_0
LDFLAGS += -Wl,-Ttext=0x8002000
endif


cdw:
	make flash BMP_PORT=/dev/ttyACM0


