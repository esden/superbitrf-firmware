/*
 * This file is part of the superbitrf project.
 *
 * Copyright (C) 2010-2014 by Piotr Esden-Tempski <piotr@esden.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

 /* This ring buffer implementation is based on the libgovernor implementation
  * from the open-bldc project.
  */

#ifndef RING_H
#define RING_H

#include <stdint.h>

typedef int32_t ring_size_t;

struct ring {
	uint8_t *data;
	ring_size_t size;
	uint32_t begin;
	uint32_t end;
};

#define RING_SIZE(RING) ((RING)->size - 1)
#define RING_DATA(RING) (RING)->data
#define RING_EMPTY(RING) ((RING)->begin == (RING)->end)
#define RING_FULL(RING) ((((RING)->end + 1) % (RING)->size) == (RING)->begin)

void ring_init(struct ring *ring, uint8_t * buf, ring_size_t size);
int32_t ring_write_ch(struct ring *ring, uint8_t ch);
int32_t ring_write(struct ring *ring, uint8_t * data, ring_size_t size);
int32_t ring_safe_write_ch(struct ring *ring, uint8_t ch);
int32_t ring_safe_write(struct ring *ring, uint8_t * data, ring_size_t size);
int32_t ring_read_ch(struct ring *ring, uint8_t * ch);
int32_t ring_read(struct ring *ring, uint8_t * data, ring_size_t size);

#endif /* RING_H */
