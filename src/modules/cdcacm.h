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

#ifndef MODULES_CDCACM_H_
#define MODULES_CDCACM_H_

// Include the board specifications for the USB define
#include "../board.h"

typedef void (*cdcacm_receive_callback) (char *data, int size);
extern bool cdcacm_did_receive;

void cdcacm_init(void);
void cdcacm_run(void);
void cdcacm_register_receive_callback(cdcacm_receive_callback callback);
bool cdcacm_send(const char *data, const int size);

#endif /* MODULES_CDCACM_H_ */
