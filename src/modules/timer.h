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

#ifndef MODULES_TIMER_H_
#define MODULES_TIMER_H_

// Include the board specifications for the timers
#include "../board.h"

/* External functions */
typedef void (*timer_on_event) (void);
void timer_init(void);
void timer_dsm_set(u16 us);
uint16_t timer_dsm_get_time(void);
void timer_dsm_stop(void);
void timer_dsm_register_callback(timer_on_event callback);

#endif /* MODULES_TIMER_H_ */
