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
#include <libopencm3/stm32/gpio.h>

#include "led.h"

/**
 * Initialize the leds
 */
void led_init(void) {
#ifdef USE_LED_POWER
	LED_INIT(POWER);
	LED_OFF(POWER);
#endif
#ifdef USE_LED_1
	LED_INIT(1);
	LED_OFF(1);
#endif
#ifdef USE_LED_2
	LED_INIT(2);
	LED_OFF(2);
#endif
}




