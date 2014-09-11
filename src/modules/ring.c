/*
 * This file is part of the superbitrf project.
 *
 * Copyright (C) 2010-2013 by Piotr Esden-Tempski <piotr@esden.net>
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

#include <stdint.h>

#include "../modules/ring.h"

/**
 * Initialize a ring buffer structure. You have to pass an alocated piece of
 * memory as buf for the struct to consume and use to store the bytes into. It
 * can be either a statically allocated binary array or a dynamically allocated
 * with malloc.
 */
void ring_init(struct ring *ring, uint8_t * buf, ring_size_t size)
{
	ring->data = buf;
	ring->size = size;
	ring->begin = 0;
	ring->end = 0;
}

/**
 * Writes one character to the ring buffer. On success it returns the character
 * provided as a 32bit int value, on failure it returns a -1 value. A failure
 * means the buffer is full and no characters can be added at the moment.
 */
int32_t ring_write_ch(struct ring *ring, uint8_t ch)
{
	if (((ring->end + 1) % ring->size) != ring->begin) {
		ring->data[ring->end++] = ch;
		ring->end %= ring->size;
		return (int32_t) ch;
	}

	return -1;
}

/**
 * Writes a sequence of characters to the ring buffer. On success it returns the
 * count of characters added to the buffer. If there is not enough space in the
 * buffer to hold all the provided data the function will return the count of
 * characters that did fit into the buffer but with the negative bit set.
 *
 * This means that if you try to write 10 characters and there is only space for
 * 4 characters the function will return -4.
 */
int32_t ring_write(struct ring * ring, uint8_t * data, ring_size_t size)
{
	int32_t i;

	for (i = 0; i < size; i++) {
		if (ring_write_ch(ring, data[i]) < 0) {
			return -i;
		}
	}

	return i;
}

/**
 * This function works the same way as ring_write_ch by writing one additional
 * character into the ring buffer. If it succeeds it returns the value of the
 * added character. In case the character does not fit into the buffer a -1 is
 * returned.
 *
 * Additionally this function tries up to 100 times to add the character to the
 * ringbuffer. This is useful if the reading side of the ring buffer is handeled
 * in an interrupt and we assume that we are just waiting for the interrupt to
 * read a character. This function has a timeout for the cases where we get a
 * lock up and the reading side is not doing it's job.
 *
 * Note: Do not use this function inside an interrupt unless you made _SURE_
 * that the reading side can preempt your interrupt!!!
 */
int32_t ring_safe_write_ch(struct ring *ring, uint8_t ch)
{
	int ret;
	int retry_count = 100;

	do {
		ret = ring_write_ch(ring, ch);
	} while ((ret < 0) && ((retry_count--) > 0));

	return ret;
}

/**
 * This function works the same way as ring_write by writing additional data
 * into the ring buffer. If it succeeds it returns the value of the added
 * character. In case the character does not fit into the buffer a -1 is
 * returned.
 *
 * Additionally this function tries up to 100 times to add each character to the
 * ringbuffer. This is useful if the reading side of the ring buffer is handeled
 * in an interrupt and we assume that we are just waiting for the interrupt to
 * read the characters. This function has a timeout for the cases where we get a
 * lock up and the reading side is not doing it's job.
 *
 * Note: Do not use this function inside an interrupt unless you made _SURE_
 * that the reading side can preempt your interrupt!!!
 */
int32_t ring_safe_write(struct ring * ring, uint8_t * data, ring_size_t size)
{
	int32_t i;

	for (i = 0; i < size; i++) {
		if (0 > ring_safe_write_ch(ring, data[i])) {
			return -i;
		}
	}

	return i;
}

/**
 * Read one character out of the ring buffer. It stores the character in the
 * provided ch pointed position as well as returning the value of the character
 * as a return value. In case there is no value to be read the return is -1.
 */
int32_t ring_read_ch(struct ring * ring, uint8_t * ch)
{
	int32_t ret = -1;

	if (ring->begin != ring->end) {
		ret = ring->data[ring->begin++];
		ring->begin %= ring->size;
		if (ch)
			*ch = ret;
	}

	return ret;
}

/**
 * Read a defined amount of characters into the provided data buffer. Returns
 * the number of read characters. If there were not enough characters in the
 * ring buffer to fill the full requested size the returned count of characters
 * read will be negative. So for example if you want to read 10 characters out
 * and there are only 2 characters in the ring buffer, the function will return
 * -2.
 */
int32_t ring_read(struct ring * ring, uint8_t * data, ring_size_t size)
{
	int32_t i;

	for (i = 0; i < size; i++) {
		if (ring_read_ch(ring, data + i) < 0) {
			return i;
		}
	}

	return -i;
}
