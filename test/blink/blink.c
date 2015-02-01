/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
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
#include "modules/led.h"
#include "modules/timer.h"

void timer1_cb(void) {
 	LED_TOGGLE(1);	/* LED on/off */
	LED_TOGGLE(2);	/* LED on/off */
	LED_TOGGLE(3);	/* LED on/off */
	timer1_set(50000); /* 0.5 second timer */
}

int main(void)
{
	rcc_clock_setup_in_hse_12mhz_out_72mhz();

	/* Initialize the modules */
	led_init();
	timer_init();

	/* Start the timer and set the callback */
	timer1_register_callback(timer1_cb);
	timer1_set(50000); /* 0.5 second timer */

	/* Blinking is done in interrupt */
	while (1);

	return 0;
}
