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

#include <unistd.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/f1/nvic.h>

#include "timer.h"
#include "config.h"

/* The timer callbacks */
timer_on_event timer1_on_event = NULL;
uint16_t timer1_value;

/**
 * Initialize timer1
 */
static void timer1_init(void) {
	rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM2EN);

	// Enable the timer NVIC
	nvic_enable_irq(TIMER1_NVIC);
	nvic_set_priority(TIMER1_NVIC, 1);

	// Setup the timer
	timer_disable_counter(TIMER1);
	timer_reset(TIMER1);
	timer_set_mode(TIMER1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_disable_preload(TIMER1);
	timer_continuous_mode(TIMER1);

	// Disable interrupts on Compare 1
	timer_disable_irq(TIMER1, TIM_DIER_CC1IE);

	// Clear the Output Compare of OC1
	timer_disable_oc_clear(TIMER1, TIM_OC1);
	timer_disable_oc_preload(TIMER1, TIM_OC1);
	timer_set_oc_slow_mode(TIMER1, TIM_OC1);
	timer_set_oc_mode(TIMER1, TIM_OC1, TIM_OCM_FROZEN);

	// Set timer updates each 10 microseconds
	timer_set_prescaler(TIMER1, 720 - 1);
	timer_set_period(TIMER1, 65535);

	// Start the timer
	timer_enable_counter(TIMER1);
}

/**
 * Initialize the timers
 */
void timer_init(void) {
	// Initialize the DSM timer
	timer1_init();
}

/**
 * Set the timer1 to interrupt
 * @param[in] us The time in microseconds divided by 10
 */
void timer1_set(uint16_t us) {
	timer1_value = timer_get_counter(TIMER1);
	uint16_t new_t = (us + timer_get_counter(TIMER1)) % 65535;

	// Update the timer compare value 1
	timer_set_oc_value(TIMER1, TIM_OC1, new_t);

	// Clear the interrupt flag and enable the interrupt of compare 1
	timer_clear_flag(TIMER1, TIM_SR_CC1IF);
	timer_enable_irq(TIMER1, TIM_DIER_CC1IE);
}

/**
 * Get the time since last set timer1
 */
uint16_t timer1_get_time(void) {
	if(timer_get_counter(TIMER1) > timer1_value)
		return timer_get_counter(TIMER1) - timer1_value;

	return timer_get_counter(TIMER1)+65535 - timer1_value;
}

/**
 * Stop the timer1 interrupts
 */
void timer1_stop(void) {
	// Clear the interrupt flag and disable the interrupt of compare 1
	timer_clear_flag(TIMER1, TIM_SR_CC1IF);
	timer_disable_irq(TIMER1, TIM_DIER_CC1IE);
}

/**
 * Register timer1 callback
 * @param[in] callback The callback function when an interrupt occurs
 */
void timer1_register_callback(timer_on_event callback) {
	timer1_on_event = callback;
}

/**
 * The timer interrupt handler
 */
void TIMER1_IRQ(void) {
	// Stop the timer
	timer1_stop();

	// Callback
	if (timer1_on_event != NULL)
		timer1_on_event();
}
