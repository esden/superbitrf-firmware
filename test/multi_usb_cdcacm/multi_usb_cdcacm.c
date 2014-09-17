/*
 * This file is part of the superbitrf project.
 *
 * Copyright (C) 2013 Freek van Tienen <freek.v.tienen@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>

/* Load the modules */
#include "modules/led.h"
#include "modules/cdcacm.h"
#include "modules/ring.h"

void our_relay_process(void);

void our_relay_process(void)
{
	uint8_t buffer;

	/* Copy over byte by byte from the incoming buffer to the outgoing buffer
	 * until either the incoming buffer is empty or the outgoing buffer is full.
	 */
	while ((!RING_EMPTY(&cdcacm_data_rx)) && (!RING_FULL(&cdcacm_control_tx))) {
		ring_read_ch(&cdcacm_data_rx, &buffer);
		ring_write_ch(&cdcacm_control_tx, buffer);
	}
	while ((!RING_EMPTY(&cdcacm_control_rx)) && (!RING_FULL(&cdcacm_data_tx))) {
		ring_read_ch(&cdcacm_control_rx, &buffer);
		ring_write_ch(&cdcacm_data_tx, buffer);
	}
}

int main(void) {
	// Setup the clock
	rcc_clock_setup_in_hse_12mhz_out_72mhz();

	// Initialize the modules
	led_init();
	cdcacm_init();

	/* The main loop */
	while (1) {
		cdcacm_process();
		our_relay_process();
	}

	return 0;
}
