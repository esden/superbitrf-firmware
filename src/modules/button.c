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

#include <stdlib.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/f1/nvic.h>

#include "button.h"

// Bind button pressed callback
button_pressed_callback button_pressed_bind = NULL;

/**
 * Initialize the buttons
 */
void button_init(void) {
#ifdef USE_BTN_BIND
	BTN_INIT(BIND);
#endif
}

/**
 * The bind button interrupt
 */
#ifdef USE_BTN_BIND
void BTN_BIND_ISR(void) {
	exti_reset_request(BTN_BIND_EXTI);
	if (button_pressed_bind != NULL)
		button_pressed_bind();
}
#endif

/**
 * Register bind button pressed callback
 * @param[in] callback The function that needs to be called when the button is pressed
 */
void button_bind_register_callback(button_pressed_callback callback) {
	button_pressed_bind = callback;
}
