##
## This file is part of the superbitrf project.
##
## Copyright (C) 2013 Piotr Esden-Tempski <piotr@esden.net>
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

TEST_TARGETS := test/blink test/usb_cdcacm test/transfer

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
# Do not print "Entering directory ...".
MAKEFLAGS += --no-print-directory
endif

all: lib main

$(TEST_TARGETS): lib
	@printf "  BUILD   $@\n";
	$(Q)$(MAKE) --directory=$@

tests: $(TEST_TARGETS)
	$(Q)true

main: lib
	@printf "  BUILD   main\n";
	$(Q)$(MAKE) --directory=src

lib:
	$(Q)if [ ! "`ls -A libopencm3`" ] ; then \
	echo "######## ERROR ########"; \
	echo "\tlibopencm3 is not initialized."; \
	echo "\tPlease run:"; \
	echo "\t$$ git submodule init"; \
	echo "\t$$ git submodule update"; \
	echo "\tbefore running make."; \
	echo "######## ERROR ########"; \
	exit 1; \
	fi
	$(Q)$(MAKE) -C libopencm3 lib TARGETS="stm32/f1"

flash: main
	$(Q)$(MAKE) -C src flash

clean:
	$(Q)$(MAKE) -C libopencm3 clean
	@printf "  CLEAN   src\n"
	$(Q)$(MAKE) -C src clean
	$(Q)for i in $(TEST_TARGETS); do \
		if [ -d $$i ]; then \
			printf "  CLEAN   $$i\n"; \
			$(MAKE) -C $$i clean SRCLIBDIR=$(SRCLIBDIR) || exit $?; \
		fi; \
	done

.PHONY: all lib
