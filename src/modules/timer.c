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

/* The timer callbacks */
timer_on_event _timer_dsm_on_event = NULL;

/**
 * Initialize the DSM timer
 */
static void timer_dsm_init(void) {
	rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM2EN);

	// Enable the timer NVIC
	nvic_enable_irq(TIMER_DSM_NVIC);
	nvic_set_priority(TIMER_DSM_NVIC, 1);

	// Setup the timer
	timer_disable_counter(TIMER_DSM);
	timer_reset(TIMER_DSM);
	timer_set_mode(TIMER_DSM, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_disable_preload(TIMER_DSM);
	timer_continuous_mode(TIMER_DSM);

	// Disable interrupts on Compare 1
	timer_disable_irq(TIMER_DSM, TIM_DIER_CC1IE);

	// Clear the Output Compare of OC1
	timer_disable_oc_clear(TIMER_DSM, TIM_OC1);
	timer_disable_oc_preload(TIMER_DSM, TIM_OC1);
	timer_set_oc_slow_mode(TIMER_DSM, TIM_OC1);
	timer_set_oc_mode(TIMER_DSM, TIM_OC1, TIM_OCM_FROZEN);

	// Set timer updates each 10 microseconds
	timer_set_prescaler(TIMER_DSM, 720 - 1);
	timer_set_period(TIMER_DSM, 65535);

	// Start the timer
	timer_enable_counter(TIMER_DSM);
}

/**
 * Initialize the timers
 */
void timer_init(void) {
	// Initialize the DSM timer
	timer_dsm_init();
}

/**
 * Set the DSM timer to interrupt
 * @param[in] us The time in microseconds divided by 10
 */
void timer_dsm_set(u16 us) {
	u16 new_t = (us + timer_get_counter(TIMER_DSM)) & 65535;

	// Update the timer compare value 1
	timer_set_oc_value(TIMER_DSM, TIM_OC1, new_t);

	// Clear the interrupt flag and enable the interrupt of compare 1
	timer_clear_flag(TIMER_DSM, TIM_SR_CC1IF);
	timer_enable_irq(TIMER_DSM, TIM_DIER_CC1IE);
}

/**
 * Stop the DSM timer interrupts
 */
void timer_dsm_stop(void) {
	// Clear the interrupt flag and disable the interrupt of compare 1
	timer_clear_flag(TIMER_DSM, TIM_SR_CC1IF);
	timer_disable_irq(TIMER_DSM, TIM_DIER_CC1IE);
}

/**
 * Register DSM timer callback
 * @param[in] callback The callback function when an interrupt occurs
 */
void timer_dsm_register_callback(timer_on_event callback) {
	_timer_dsm_on_event = callback;
}

/**
 * The timer interrupt handler
 */
void TIMER_DSM_IRQ(void) {
	// Stop the timer
	timer_dsm_stop();

	// Callback
	if (_timer_dsm_on_event != NULL)
		_timer_dsm_on_event();
}
