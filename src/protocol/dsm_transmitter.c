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
#include "dsm_transmitter.h"

#include <libopencm3/stm32/gpio.h>
#include "../modules/led.h"
#include <stdio.h>
#include <string.h>

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

	LED_ON(1);
	LED_OFF(2);

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
	cyrf_send(dsm_transmitter.packet);
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

	LED_OFF(1);
	LED_ON(2);

	// Set the status to ready and reset the packet count
	dsm_transmitter.packet_count = 0;
	dsm_transmitter.status = DSM_TRANSMITTER_SENDA;

	dsm_transmitter_create_data_packet();

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

		// Send the packet
		dsm_transmitter_send();

		// TODO: receive telemetry
		break;
	case DSM_TRANSMITTER_SENDB:
		// Start the timer as first so we make sure the timing is right
		timer_dsm_set(DSM_SEND_TIME - DSM_CHA_CHB_SEND_TIME);

		// Update the channel
		dsm_set_next_channel();

		// Change the status
		dsm_transmitter.status = DSM_TRANSMITTER_SENDA;

		// TODO: create the packet

		// Send the packet
		dsm_transmitter_send();

		// TODO: receive telemetry
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
	//TODO: Get the telemetry data
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
}

/**
 * Create the bind packet
 */
static void dsm_transmitter_create_bind_packet(void) {
	u8 i;
	u16 sum = 384 - 0x10;

	dsm_transmitter.packet[0] = ~dsm.cyrf_mfg_id[0];
	dsm_transmitter.packet[1] = ~dsm.cyrf_mfg_id[1];
	dsm_transmitter.packet[2] = ~dsm.cyrf_mfg_id[2];
	dsm_transmitter.packet[3] = ~dsm.cyrf_mfg_id[3];
	dsm_transmitter.packet[4] = dsm_transmitter.packet[0];
	dsm_transmitter.packet[5] = dsm_transmitter.packet[1];
	dsm_transmitter.packet[6] = dsm_transmitter.packet[2];
	dsm_transmitter.packet[7] = dsm_transmitter.packet[3];

	// Calculate the sum
	for (i = 0; i < 8; i++)
		sum += dsm_transmitter.packet[i];

	dsm_transmitter.packet[8] = sum >> 8;
	dsm_transmitter.packet[9] = sum & 0xFF;
	dsm_transmitter.packet[10] = 0x01; //???
	dsm_transmitter.packet[11] = 0x00; // Number of channels, but we use own protocol
	dsm_transmitter.packet[12] = dsm.protocol;
	dsm_transmitter.packet[13] = 0x00; //???

	// Calculate the sum
	for (i = 8; i < 14; i++)
		sum += dsm_transmitter.packet[i];

	dsm_transmitter.packet[14] = sum >> 8;
	dsm_transmitter.packet[15] = sum & 0xFF;
}

static void dsm_transmitter_create_data_packet(void) {
	dsm_transmitter.packet[0] = dsm.cyrf_mfg_id[2];
	dsm_transmitter.packet[1] = dsm.cyrf_mfg_id[3];
}

/**
 * Send the packet
 */
static void dsm_transmitter_send(void) {
	char cdc_msg[512];

	sprintf(cdc_msg, "Packet send (channel %i): 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n",
			dsm.cur_channel,
			dsm_transmitter.packet[0], dsm_transmitter.packet[1], dsm_transmitter.packet[2], dsm_transmitter.packet[3], dsm_transmitter.packet[4], dsm_transmitter.packet[5],
			dsm_transmitter.packet[6], dsm_transmitter.packet[7], dsm_transmitter.packet[8], dsm_transmitter.packet[9], dsm_transmitter.packet[10], dsm_transmitter.packet[11],
			dsm_transmitter.packet[12], dsm_transmitter.packet[13], dsm_transmitter.packet[14], dsm_transmitter.packet[15]);
	cdcacm_send(cdc_msg, strlen(cdc_msg));

	// Check if already sending
	if (dsm_transmitter.sending)
		dsm_transmitter.overflow_count++;

	cyrf_send(dsm_transmitter.packet);
	dsm_transmitter.packet_count++;
	dsm_transmitter.sending = 1;
}
