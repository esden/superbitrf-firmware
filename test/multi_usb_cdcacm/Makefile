##
## This file is part of the superbitrf project.
##
## Copyright (C) 2014 Piotr Esden-Tempski <piotr@esden.net>
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

BINARY = multi_usb_cdcacm

OBJS += ../../src/modules/led.o ../../src/modules/cdcacm.o

LDSCRIPT = ../../stm32f103cbt6.ld

CFLAGS += -DBOARD_V1_0 -I../../src

include ../../Makefile.include
