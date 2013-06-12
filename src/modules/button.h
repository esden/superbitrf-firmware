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

#ifndef MODULES_BUTTON_H_
#define MODULES_BUTTON_H_

// Include the board specifications for the buttons
#include "../board.h"


#define _(i)  i
#define BTN_GPIO_PORT(i)	_(BTN_ ## i ## _GPIO_PORT)
#define BTN_GPIO_PIN(i) 	_(BTN_ ## i ## _GPIO_PIN)
#define BTN_GPIO_CLK(i) 	_(BTN_ ## i ## _GPIO_CLK)
#define BTN_NVIC(i) 		_(BTN_ ## i ## _NVIC)
#define BTN_EXTI(i)			_(BTN_ ## i ## _EXTI)

#define BTN_INIT(i) {                               \
	rcc_peripheral_enable_clock(&RCC_APB2ENR,       \
								BTN_GPIO_CLK(i) | RCC_APB2ENR_AFIOEN);	\
	gpio_set_mode(BTN_GPIO_PORT(i),                 \
				  GPIO_MODE_INPUT,                  \
				  GPIO_CNF_INPUT_FLOAT,             \
				  BTN_GPIO_PIN(i));                 \
	exti_select_source(BTN_EXTI(i),					\
					   BTN_GPIO_PORT(i));			\
	exti_set_trigger(BTN_EXTI(i), 					\
					 EXTI_TRIGGER_FALLING);			\
	exti_enable_request(BTN_EXTI(i));				\
	nvic_set_priority(BTN_NVIC(i), 0);              \
	nvic_enable_irq(BTN_NVIC(i));					\
}

/* External functions */
typedef void (*button_pressed_callback) (void);
void button_init(void);
void button_bind_register_callback(button_pressed_callback callback);

#endif /* MODULES_BUTTON_H_ */
