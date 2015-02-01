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
#include "modules/config.h"
#include "modules/led.h"
#include "modules/button.h"
#include "modules/timer.h"
#include "modules/cdcacm.h"
#include "modules/cyrf6936.h"
#include "modules/console.h"


int main(void) {
	// Setup the clock
	rcc_clock_setup_in_hse_12mhz_out_72mhz();

	// Initialize the modules
	config_init();
	led_init();
	timer_init();
	cdcacm_init();
	button_init();
	cyrf_init();
	console_init();

	// Initialize all protocols
	//TODO

	// Start the protocol at boot
	//TODO

	/* The main loop */
	while (1) {
		cdcacm_process();
		console_run();

		//The protocol main loop
		//TODO
	}

	return 0;
}
