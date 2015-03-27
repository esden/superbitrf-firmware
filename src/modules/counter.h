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

#ifndef MODULES_COUNTER_H
#define MODULES_COUNTER_H

/* Headers */
#include <stdint.h>
#include <libopencm3/cm3/systick.h>

/* Type definitions. */
typedef void (*counter_callback_t)(void *);

/* Private type definitions */
struct counter_status {
  bool init;
  uint32_t frequency;
  uint32_t fine_frequency;
  volatile uint32_t ticks;
};

extern struct counter_status counter_status;

/* API declarations. */
void counter_init(void);
void counter_handler(void);

static inline uint32_t counter_get_ticks(void) {
	return counter_status.ticks;
}

static inline uint32_t counter_get_ticks_from_time(uint32_t t,
	                                               uint32_t frequency,
	                                               uint32_t t_div)
{
	/* Using 30.2 fixed point format to gain some accuracy. */
	uint32_t t_30_2 = t << 2;
	uint32_t frequency_30_2 = frequency << 2;
	uint32_t t_div_30_2 = t_div << 2;
	uint32_t ticks_30_2 = t_30_2 * ((frequency_30_2 << 2) / t_div_30_2);
	/* convert back to non fractional format with rounding. */
	uint32_t ticks = (ticks_30_2 + ((ticks_30_2 & 1 << 1) << 1)) >> 4;
	return ticks;
}

static inline uint32_t counter_get_ticks_of_ms(uint32_t ms) {
	return counter_get_ticks_from_time(ms, counter_status.frequency, 1e+3);
}

static inline uint32_t counter_get_ticks_of_us(uint32_t us) {
	return counter_get_ticks_from_time(us, counter_status.frequency, 1e+6);
}

static inline void counter_wait_poll(uint32_t ticks) {
	uint32_t start_ticks = counter_get_ticks(), now_ticks = start_ticks;
	while ((now_ticks - start_ticks) < ticks) {
		now_ticks = counter_get_ticks();
	}
}

#endif /* MODULES_COUNTER_H */
