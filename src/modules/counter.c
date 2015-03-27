/*
 * This file is part of the superbitrf project.
 *
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
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

/*
 * This module is using the systick timer to provide timing functions to the
 * system.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/systick.h>

#include "counter.h"

/* Configuration */
#define COUNTER_FREQ 1000000 /* Counter interval of 1ms */

/* Private variables. */

/* Status of the counter module. */
struct counter_status counter_status = {
  .init = false,
  .frequency = COUNTER_FREQ,
  .fine_frequency = 0, /* We don't know that yet. */
  .ticks = 0
};


/* Module function implementations. */

/**
 * Initialize the counter module.
 */
void counter_init(void)
{
  
  /* Initialize status.
   * The first few fields should be initialized globally already, but we are
   * just making sure it is done.
   */
  counter_status.init = false;
  counter_status.frequency = COUNTER_FREQ;
  counter_status.fine_frequency = rcc_ahb_frequency / 8;
  counter_status.ticks = 0;

  /* Initialize the hardware.
   * XXX: Move to the HAL.
   */
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

  systick_set_reload((rcc_ahb_frequency / 8) / COUNTER_FREQ);

  systick_interrupt_enable();

  systick_counter_enable();

  /* Mark that we are done initializing. */
  counter_status.init = true;
}

/**
 * Handler for the counter module periodically called from the main loop.
 */
void counter_handler(void)
{

}

/* Interrupt handlers */
void sys_tick_handler(void);
void sys_tick_handler(void)
{
  counter_status.ticks++;
}
