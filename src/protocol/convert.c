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

#include <string.h>
#include <libopencm3/cm3/common.h>

#include "convert.h"

/* The convert structures */
struct TransmitConvert transmit_convert;
struct ReceiveConvert receive_convert;

/**
 * Initialize the convert structures
 */
void convert_init(void) {
	// Initialize the transmit structure
	transmit_convert.in_ptr = 0;
	transmit_convert.in_max_ptr = 0;
	transmit_convert.out_ptr = 0;
	transmit_convert.out_max_ptr = 0;

	// Initialize the receive structure
	receive_convert.read_ptr = 0;
	receive_convert.read_max_ptr = 0;
	receive_convert.write_ptr =0;
	receive_convert.write_max_ptr = 0;
}

/**
 * Convert from PPRZ to DSMP
 */
void convert_pprz_dsmp(void) {
	int i, readable_size;
	u8 size, checksum_a, checksum_b;

	// Calculate the size we can read
	if (transmit_convert.in_ptr <= transmit_convert.in_max_ptr)
		readable_size = transmit_convert.in_ptr - transmit_convert.in_max_ptr;
	else
		readable_size = transmit_convert.in_ptr - transmit_convert.in_max_ptr + CONVERT_MAX_BUFFER;

	// First we need to check if we have a full PPRZ message header in the transmit_convert read buffer
	if (readable_size < 2)
		return;

	// When it isn't a PPRZ message (not starting with STX)
	if (transmit_convert.in_buffer[transmit_convert.in_ptr] != 0x99) {
		// Update the read pointer and try again
		transmit_convert.in_ptr = (transmit_convert.in_ptr + 1) % CONVERT_MAX_BUFFER;
		convert_pprz_dsmp();
		return;
	}

	// Now read the size and test if we have the full package
	size = transmit_convert.in_buffer[transmit_convert.in_ptr+1];
	if (readable_size < size)
		return;

	// We now have the full package and can calculate the checksums
	checksum_a = size;
	checksum_b = size;
	for (i = transmit_convert.in_ptr+2; i < transmit_convert.in_ptr+size-3; i++) {
		checksum_a += transmit_convert.in_buffer[i];
		checksum_b += checksum_a;
	}

	// When the checksums don't match
	if (checksum_a != transmit_convert.in_buffer[transmit_convert.in_ptr+size-2] ||
			checksum_b != transmit_convert.in_buffer[transmit_convert.in_ptr+size-1]) {
		// Update the read pointer and try again
		transmit_convert.in_ptr = (transmit_convert.in_ptr + 1) % CONVERT_MAX_BUFFER;
		convert_pprz_dsmp();
		return;
	}

	//TODO: Check if there is an overflow
	// We now have a full PPRZ packet in the buffer and can copy the packet
	transmit_convert.out_buffer[transmit_convert.out_max_ptr+1] = size-3;
	memcpy(transmit_convert.out_buffer + transmit_convert.out_max_ptr + 1,
			transmit_convert.out_buffer + transmit_convert.out_ptr + 2,
			size - 4);
	transmit_convert.out_max_ptr = (transmit_convert.out_max_ptr + size-4) % CONVERT_MAX_BUFFER;

	// Update the read pointer
	transmit_convert.out_ptr = (transmit_convert.out_ptr + size) % CONVERT_MAX_BUFFER;

	//TODO: notify there is a packet for transmit
}

/**
 * Convert from DSMP to PPRZ
 */
void convert_dsmp_pprz(void) {
	int i, readable_size;
	u8 size, checksum_a, checksum_b;

	// Calculate the size we can read
	if (receive_convert.read_ptr <= receive_convert.read_max_ptr)
		readable_size = receive_convert.read_ptr - receive_convert.read_max_ptr;
	else
		readable_size = receive_convert.read_ptr - receive_convert.read_max_ptr + CONVERT_MAX_BUFFER;

	// First we need to check if we have received the DSMP size
	if (readable_size < 1)
		return;

	// We can now read the size from the packet and check if we can fully read it
	size = receive_convert.read_buffer[receive_convert.read_ptr];
	if (readable_size < size)
		return;

	// Calculate the ckecksums
	checksum_a = size + 3;
	checksum_b = size + 3;
	for (i = receive_convert.read_ptr+1; i < receive_convert.read_ptr+size-1; i++) {
		checksum_a += receive_convert.read_buffer[i];
		checksum_b += checksum_a;
	}

	//TODO: Check if there is an overflow
	// We got a full packet so can start converting
	receive_convert.write_buffer[receive_convert.write_max_ptr] 		= 0x99; 	// Add the STX
	receive_convert.write_buffer[receive_convert.write_max_ptr+1]		= size + 3;	// Add the size
	memcpy(receive_convert.write_buffer + receive_convert.write_max_ptr + 1,
			receive_convert.read_buffer + receive_convert.read_ptr + 1,
				size - 1);
	receive_convert.write_buffer[receive_convert.write_max_ptr+size+1] 	= checksum_a; 	// Add the Checksum A
	receive_convert.write_buffer[receive_convert.write_max_ptr+size+2]	= checksum_b;	// Add the Checksum B
	receive_convert.write_max_ptr = (receive_convert.write_max_ptr + size+2) % CONVERT_MAX_BUFFER;

	// Update the read pointer
	receive_convert.read_ptr = (receive_convert.read_ptr + size) % CONVERT_MAX_BUFFER;

	//TODO: notify there is a packet received
}
