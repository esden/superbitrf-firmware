/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Freek van Tienen <freek.v.tienen@gmail.com>
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
#include "modules/counter.h"

int main(void)
{
	rcc_clock_setup_in_hse_12mhz_out_72mhz();
	led_init();
	counter_init();
	uint32_t ticks_ms = counter_get_ticks_of_ms(1);
	uint32_t ticks_us = counter_get_ticks_of_us(50);
	while(1) {
		LED_ON(2);
		/* Run 10 times with coarse counter. */
		for(int i = 0; i < 10; i++) {
			counter_wait_poll(ticks_ms);
			LED_ON(1);
			counter_wait_poll(ticks_us);
			LED_OFF(1);
		}
		LED_OFF(2);
		counter_wait_poll(counter_get_ticks_of_ms(10));
	}
}