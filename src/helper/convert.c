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

#include <stdio.h>
#include <string.h>
#include <libopencm3/cm3/common.h>

#include "convert.h"

/**
 * Initialize the buffer structure
 */
void convert_init(struct Buffer *buffer) {
	// Initialize the buffer
	buffer->insert_idx = 0;
	buffer->extract_idx = 0;
	buffer->buffer_insert_cb = NULL;
}

/**
 * Insert bytes into the buffer and return if there is enough space
 */
bool convert_insert(struct Buffer *buffer, uint8_t *data, uint16_t length) {
	uint16_t i;

	// Check if there is enough space available in the buffer
	if(convert_insert_size(buffer) < length)
		return false;

	// Insert the data into the buffer with the length in front
	for(i = 0; i < length; i++)
		buffer->buffer[(buffer->insert_idx+i) % MAX_BUFFER] = data[i];
	buffer->insert_idx = (buffer->insert_idx+length) % MAX_BUFFER;

	// Check if there is a callback
	if(buffer->buffer_insert_cb != NULL)
		buffer->buffer_insert_cb();

	return true;
}

/**
 * Extract bytes from the buffer and return the number of bytes extracted
 */
uint16_t convert_extract(struct Buffer *buffer, uint8_t *data, uint16_t length) {
	uint16_t i;
	uint16_t real_length = convert_extract_size(buffer);

	// Check the available bytes and see if we are sending less
	if(real_length > length)
		real_length = length;

	// Receive the data from the buffer
	for(i = 0; i < real_length; i++)
		data[i] = buffer->buffer[(buffer->extract_idx+i) % MAX_BUFFER];

	buffer->extract_idx = (buffer->extract_idx + real_length) % MAX_BUFFER;

	return real_length;
}

/**
 * Set the buffer callback after bytes are inserted
 */
void convert_set_insert_cb(struct Buffer *buffer, void (*buffer_insert_cb)(void)) {
	buffer->buffer_insert_cb = buffer_insert_cb;
}

/**
 * The maximum insert size from the buffer
 */
uint16_t convert_insert_size(struct Buffer *buffer) {
	return MAX_BUFFER - convert_extract_size(buffer);
}

/**
 * The maximum extract size from the buffer
 */
uint16_t convert_extract_size(struct Buffer *buffer) {
	if(buffer->extract_idx <= buffer->insert_idx)
		return buffer->insert_idx - buffer->extract_idx;

	return (buffer->insert_idx + MAX_BUFFER) - buffer->extract_idx;
}

/**
 * Convert normal radio transmitter to channel outputs
 */
void convert_radio_to_channels(uint8_t* data, uint8_t nb_channels, bool is_11bit, int16_t* channels) {
	int i;
	uint8_t bit_shift = (is_11bit)? 11:10;
	int16_t value_max = (is_11bit)? 0x07FF: 0x03FF;

	for (i=0; i<7; i++) {
		const int16_t tmp = ((data[2*i]<<8) + data[2*i+1]) & 0x7FFF;
		const uint8_t chan = (tmp >> bit_shift) & 0x0F;
		const int16_t val  = (tmp&value_max);

		if(chan < nb_channels)
			channels[chan] = val;
	}
}
