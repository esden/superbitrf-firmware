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

#include <stdlib.h>
#include <libopencm3/cm3/common.h>
#include "../modules/timer.h"
#include "../modules/cyrf6936.h"

#include "dsm.h"
#include "convert.h"
#include "dsm_transmitter.h"

/* The DSM transmitter struct */
struct DsmTransmitter dsm_transmitter;

static void dsm_transmitter_send(void);
static void dsm_transmitter_create_bind_packet(void);
static void dsm_transmitter_create_data_packet(void);

/**
 * Start the transmitter binding
 */
void dsm_transmitter_start_bind(void) {
	// Stop the timer
	timer_dsm_stop();

	// Set the status to bind and reset the packet count
	dsm_transmitter.packet_count = 0;
	dsm_transmitter.status = DSM_TRANSMITTER_BIND;
	dsm.protocol = DSM_PROTOCOL;

	// Set the CYRF channel (random if not set)
#ifdef DSM_BIND_CHANNEL
	dsm_set_channel_raw(DSM_BIND_CHANNEL);
#else
	dsm_set_channel_raw(rand() / (RAND_MAX / DSM_MAX_CHANNEL + 1));
#endif

	// Create the bind packet and send
	dsm_transmitter_create_bind_packet();
	cyrf_send(dsm.transmit_packet);
	dsm_transmitter.packet_count++;

	// Enable the timer
	timer_dsm_set(DSM_BIND_SEND_TIME);
}

/**
 * Start transmitting
 */
void dsm_transmitter_start(void) {
	// Stop the timer
	timer_dsm_stop();

	// Set the status to ready and reset the packet count
	dsm_transmitter.packet_count = 0;
	dsm_transmitter.status = DSM_TRANSMITTER_SENDA;

	// Start the timer
	timer_dsm_set(DSM_CHA_CHB_SEND_TIME);
}

/**
 * When the timer send an IRQ
 */
void dsm_transmitter_on_timer(void) {
	// Check the transmitter status
	switch (dsm_transmitter.status) {
	case DSM_TRANSMITTER_BIND:
		// Send the bind packet again
		dsm_transmitter_send();

		// Check for switching back
		if (dsm_transmitter.packet_count >= DSM_BIND_SEND_COUNT)
			dsm_start();
		else {
			// Start the timer
			timer_dsm_set(DSM_BIND_SEND_TIME);
		}
		break;
	case DSM_TRANSMITTER_SENDA:
		// Start the timer as first so we make sure the timing is right
		timer_dsm_set(DSM_CHA_CHB_SEND_TIME);

		// Update the channel
		dsm_set_next_channel();

		// Change the status
		dsm_transmitter.status = DSM_TRANSMITTER_SENDB;

		// TODO: create the packet
		dsm_transmitter_create_data_packet();

		// Send the packet
		dsm_transmitter_send();

		// TODO: receive telemetry
		cyrf_start_recv();
		break;
	case DSM_TRANSMITTER_SENDB:
		// Start the timer as first so we make sure the timing is right
		timer_dsm_set(DSM_SEND_TIME - DSM_CHA_CHB_SEND_TIME);

		// Update the channel
		dsm_set_next_channel();

		// Change the status
		dsm_transmitter.status = DSM_TRANSMITTER_SENDA;

		// TODO: create the packet
		dsm_transmitter_create_data_packet();

		// Send the packet
		dsm_transmitter_send();
		break;
	default:
		break;
	}
}

/**
 * When the receive is completed (IRQ)
 * @param[in] error When the receive was with an error
 */
void dsm_transmitter_on_receive(bool error) {
	// Receive the data
	cyrf_recv(dsm.receive_packet);

	// Handle packet
	convert_radio_to_cdcacm_insert(dsm.receive_packet, 16);
}

/**
 * When the send is completed (IRQ)
 * @param[in] error When the send was with an error
 */
void dsm_transmitter_on_send(bool error) {
	// Check if there is no error
	if (error)
		dsm_transmitter.error_count++;

	dsm_transmitter.sending = 0;

	// Receive telemetry
	cyrf_start_recv();
}

/**
 * Create the bind packet
 */
static void dsm_transmitter_create_bind_packet(void) {
	u8 i;
	u16 sum = 384 - 0x10;

	dsm.transmit_packet[0] = ~dsm.cyrf_mfg_id[0];
	dsm.transmit_packet[1] = ~dsm.cyrf_mfg_id[1];
	dsm.transmit_packet[2] = ~dsm.cyrf_mfg_id[2];
	dsm.transmit_packet[3] = ~dsm.cyrf_mfg_id[3];
	dsm.transmit_packet[4] = dsm.transmit_packet[0];
	dsm.transmit_packet[5] = dsm.transmit_packet[1];
	dsm.transmit_packet[6] = dsm.transmit_packet[2];
	dsm.transmit_packet[7] = dsm.transmit_packet[3];

	// Calculate the sum
	for (i = 0; i < 8; i++)
		sum += dsm.transmit_packet[i];

	dsm.transmit_packet[8] = sum >> 8;
	dsm.transmit_packet[9] = sum & 0xFF;
	dsm.transmit_packet[10] = 0x01; //???
	dsm.transmit_packet[11] = 0x00; // Number of channels, but we use own protocol
	dsm.transmit_packet[12] = dsm.protocol;
	dsm.transmit_packet[13] = 0x00; //???

	// Calculate the sum
	for (i = 8; i < 14; i++)
		sum += dsm.transmit_packet[i];

	dsm.transmit_packet[14] = sum >> 8;
	dsm.transmit_packet[15] = sum & 0xFF;
}

/**
 * Create the transmitter data packet
 */
static void dsm_transmitter_create_data_packet(void) {
	u8 packet_length, i;
	// Set the first two bytes to the cyrf_mfg_id
	dsm.transmit_packet[0] = dsm.cyrf_mfg_id[2];
	dsm.transmit_packet[1] = dsm.cyrf_mfg_id[3];

	// Fill as much bytes with data
	packet_length = convert_extract(&cdcacm_to_radio, dsm.transmit_packet+2, 14);
	// Clean the rest of the packet
	for(i = packet_length + 2; i < 16; i++)
		dsm.transmit_packet[i] = 0x00;
}

/**
 * Send the packet
 */
static void dsm_transmitter_send(void) {
	// Check if already sending
	if (dsm_transmitter.sending)
		dsm_transmitter.overflow_count++;

	cyrf_send(dsm.transmit_packet);
	dsm_transmitter.packet_count++;
	dsm_transmitter.sending = 1;
}
