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
#include "../modules/cdcacm.h"

/* The two buffers used */
struct Buffer cdcacm_to_radio;
struct Buffer radio_to_cdcacm;

/**
 * Initialize the buffer structures
 */
void convert_init(void) {
	// Initialize the cdcacm -> radio
	cdcacm_to_radio.insert_idx = 0;
	cdcacm_to_radio.extract_idx = 0;

	// Initialize the radio -> cdcacm
	radio_to_cdcacm.insert_idx = 0;
	radio_to_cdcacm.extract_idx = 0;

	// Set the cdcacm receive callback
	cdcacm_register_receive_callback(convert_cdcacm_receive_cb);
}

/**
 * Insert bytes into the cdcacm_to_radio buffer and return if there is enough space
 */
bool convert_cdcacm_to_radio_insert(u8 *data, u8 length) {
	u8 i;

	// Check if there is enough space available in the buffer
	if(convert_insert_size(&cdcacm_to_radio) < length+1)
		return false;

	// Insert the data into the buffer with the length in front
	cdcacm_to_radio.buffer[cdcacm_to_radio.insert_idx] = length+1;
	for(i = 0; i < length; i++)
		cdcacm_to_radio.buffer[(cdcacm_to_radio.insert_idx+i+1) % MAX_BUFFER] = data[i];
	cdcacm_to_radio.insert_idx = (cdcacm_to_radio.insert_idx+ length+1) % MAX_BUFFER;

	return true;
}

/**
 * Insert bytes into the radio_to_cdcacm buffer and return if there is enough space
 */
bool convert_radio_to_cdcacm_insert(u8 *data, u8 length) {
	u8 i, packet_length;
	// Check if there is enough space available in the buffer
	if(convert_insert_size(&radio_to_cdcacm) < length)
		return false;

	// Insert the data into the buffer
	for(i = 0; i < length; i++)
		radio_to_cdcacm.buffer[(radio_to_cdcacm.insert_idx+i) % MAX_BUFFER] = data[i];
	radio_to_cdcacm.insert_idx = (radio_to_cdcacm.insert_idx+ length) % MAX_BUFFER;

	// Update the extract_idx to filter out empty data
	while(radio_to_cdcacm.buffer[radio_to_cdcacm.extract_idx] == 0x00 && radio_to_cdcacm.extract_idx != radio_to_cdcacm.insert_idx)
		radio_to_cdcacm.extract_idx = (radio_to_cdcacm.extract_idx + 1) % MAX_BUFFER;

	// Check if the full packet is in the buffer
	packet_length = radio_to_cdcacm.buffer[radio_to_cdcacm.extract_idx];
	if(packet_length > 0 && packet_length <= convert_extract_size(&radio_to_cdcacm)) {
		// Let cdcacm know that there is packet with a certain length
		convert_cdcacm_send_cb(packet_length);
	}

	return true;
}

/**
 * Extract bytes from the buffer and return the number of bytes extracted
 */
u8 convert_extract(struct Buffer *buffer, u8 *data, u8 length) {
	u8 i;
	u8 real_length = convert_extract_size(buffer);

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
 * The maximum insert size from the buffer
 */
u8 convert_insert_size(struct Buffer *buffer) {
	return MAX_BUFFER - convert_extract_size(buffer);
}

/**
 * The maximum extract size from the buffer
 */
u8 convert_extract_size(struct Buffer *buffer) {
	if(buffer->extract_idx <= buffer->insert_idx)
		return buffer->insert_idx - buffer->extract_idx;

	return (buffer->insert_idx + MAX_BUFFER) - buffer->extract_idx;
}

/**
 * The callback when cdcacm recevies something
 */
void convert_cdcacm_receive_cb(char *data, int size) {
	convert_cdcacm_to_radio_insert((u8 *)data, size);
}

/**
 * The callback when there is something to send to cdcacm
 */
void convert_cdcacm_send_cb(int size) {
	u8 packet[size];
	convert_extract(&radio_to_cdcacm, packet, size);
	cdcacm_send((char *)packet, size);
}
