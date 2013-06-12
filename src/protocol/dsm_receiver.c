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

#include <libopencm3/cm3/common.h>
#include "../modules/timer.h"
#include "../modules/cyrf6936.h"

#include "dsm.h"
#include "dsm_receiver.h"

#include <libopencm3/stm32/gpio.h>
#include "../modules/led.h"
#include <stdio.h>
#include <string.h>

/* The DSM receiver struct */
struct DsmReceiver dsm_receiver;

/**
 * Start the receiver binding
 */
void dsm_receiver_start_bind(void) {
	// Stop the timer
	timer_dsm_stop();

	// Update the channel
#ifdef DMS_BIND_CHANNEL
	dsm_set_channel_raw(DMS_BIND_CHANNEL);
#else
	dsm_set_channel_raw(0);
#endif

	// Set the status to bind
	dsm_receiver.status = DSM_RECEIVER_BIND;

	// Start receiving
	cyrf_start_recv();

	// Enable the timer
	timer_dsm_set(DSM_BIND_RECV_TIME);
}

/**
 * Start receiving
 */
void dsm_receiver_start(void) {
	// Stop the timer
	timer_dsm_stop();

	// Update the channel (goes to first because of ch_id = last) and set to SYNC
	if(IS_DSM2(dsm.protocol))
		dsm_set_channel(0);
	else
		dsm_set_next_channel();
	dsm_receiver.status = DSM_RECEIVER_SYNC_A;

	// Start receiving
	cyrf_start_recv();

	// Enable the timer
	if(IS_DSM2(dsm.protocol))
		timer_dsm_set(DSM_SYNC_RECV_TIME);
	else
		timer_dsm_set(DSM_SYNC_FRECV_TIME); // Because we know for sure where DSMX starts we can wait the full bind
}

/**
 * When the timer send an IRQ
 */
void dsm_receiver_on_timer(void) {
	u8 rx_irq_status;
	// Check the receiver status
	switch (dsm_receiver.status) {
	case DSM_RECEIVER_BIND:
#ifndef DMS_BIND_CHANNEL
		// Update the channel
		dsm_set_channel_raw((dsm.cur_channel + 1) % DSM_MAX_CHANNEL);
#endif

		// Set the new timeout
		timer_dsm_set(DSM_BIND_RECV_TIME);
		break;
	case DSM_RECEIVER_SYNC_A:
	case DSM_RECEIVER_SYNC_B:
		LED_TOGGLE(POWER);
		// When we are in DSM2 mode we need to scan all channels
		if(IS_DSM2(dsm.protocol)) {
			// Set the next channel
			dsm_set_channel((dsm.cur_channel + 1) % DSM_MAX_CHANNEL);
		} else {
			// Just set the next channel we know
			dsm_set_next_channel();
		}

		cyrf_start_recv();

		// Set the new timeout
		timer_dsm_set(DSM_SYNC_RECV_TIME);
		break;
	case DSM_RECEIVER_RECV:
		// We are out of sync and start syncing again
		dsm_receiver.status = DSM_RECEIVER_SYNC_A;

		// Set the new timeout
		timer_dsm_set(DSM_SYNC_RECV_TIME);
		break;
	default:
		break;
	}
}

/**
 * When the receive is completed (IRQ)
 * @param[in] error When the receive was with an error
 */
void dsm_receiver_on_receive(bool error) {
	int i;
	u16 sum;
	char cdc_msg[512];
	u8 rx_status = cyrf_get_rx_status();

	// If there is no error get the packet
	//if (!error)
		cyrf_recv(dsm_receiver.packet);
	cyrf_start_recv();

	// Inverse the total packet
	if(dsm_receiver.status == DSM_RECEIVER_BIND) {
		for(i = 0; i < 16; i++)
			dsm_receiver.packet[i] = (~dsm_receiver.packet[i]) & 0xFF;
	}

	sprintf(cdc_msg, "Packet received: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n",
			dsm_receiver.packet[0], dsm_receiver.packet[1], dsm_receiver.packet[2], dsm_receiver.packet[3], dsm_receiver.packet[4], dsm_receiver.packet[5],
			dsm_receiver.packet[6], dsm_receiver.packet[7], dsm_receiver.packet[8], dsm_receiver.packet[9], dsm_receiver.packet[10], dsm_receiver.packet[11],
			dsm_receiver.packet[12], dsm_receiver.packet[13], dsm_receiver.packet[14], dsm_receiver.packet[15]);
	cdcacm_send(cdc_msg, strlen(cdc_msg));

	// Check the receiver status
	switch (dsm_receiver.status) {
	case DSM_RECEIVER_BIND:
		// Check if there is an error and the MFG id is exactly the same twice
		if (dsm_receiver.packet[0] != dsm_receiver.packet[4]
				|| dsm_receiver.packet[1] != dsm_receiver.packet[5]
				|| dsm_receiver.packet[2] != dsm_receiver.packet[6]
				|| dsm_receiver.packet[3] != dsm_receiver.packet[7])
			break;

		sprintf(cdc_msg, "match\r\n");
		cdcacm_send(cdc_msg, strlen(cdc_msg));

		// Calculate the first sum
		sum = 384 - 0x10;
		for(i = 0; i < 8; i++)
			sum += dsm_receiver.packet[i];

		// Check the first sum
		if (dsm_receiver.packet[8] != sum >> 8 || dsm_receiver.packet[9] != (sum & 0xFF))
			break;

		// Calculate second sum
		for(i = 8; i < 14; i++)
			sum += dsm_receiver.packet[i];

		// Check the second sum
		if (dsm_receiver.packet[14] != sum >> 8 || dsm_receiver.packet[15] != (sum & 0xFF))
			break;

		// Stop the timer
		timer_dsm_stop();

		// Update the mfg id, number of channels and protocol
		dsm.cyrf_mfg_id[0] = ~dsm_receiver.packet[0];
		dsm.cyrf_mfg_id[1] = ~dsm_receiver.packet[1];
		dsm.cyrf_mfg_id[2] = ~dsm_receiver.packet[2];
		dsm.cyrf_mfg_id[3] = ~dsm_receiver.packet[3];
		dsm_receiver.num_channels = dsm_receiver.packet[11];
		dsm.protocol = dsm_receiver.packet[12];

		LED_TOGGLE(1);

		// Start receiver
		dsm_start();
		break;
	case DSM_RECEIVER_SYNC_A:
		// If other error than bad CRC or MFG id doesn't match reject the packet
		if((error && !(rx_status & CYRF_BAD_CRC))
				|| dsm_receiver.packet[0] != dsm.cyrf_mfg_id[2] || dsm_receiver.packet[1] != dsm.cyrf_mfg_id[3])
			break;

		sprintf(cdc_msg, "SYNC1: %i\r\n", dsm.cur_channel);
				cdcacm_send(cdc_msg, strlen(cdc_msg));

		// Invert the CRC when received bad CRC
		if (error && (rx_status & CYRF_BAD_CRC))
			dsm.crc_seed = ~dsm.crc_seed;

		// Stop the timer
		timer_dsm_stop();

		if(IS_DSM2(dsm.protocol)) {
			// We now got one channel and need to find the other
			if (dsm.crc_seed == ((dsm.cyrf_mfg_id[0] << 8) + dsm.cyrf_mfg_id[1]))
				dsm.channels[0] = dsm.cur_channel;
			else
				dsm.channels[1] = dsm.cur_channel;
			dsm_receiver.status = DSM_RECEIVER_SYNC_B;

			// Set the new channel
			dsm_set_channel((dsm.cur_channel + 1) % DSM_MAX_CHANNEL);

			// Start the timer
			timer_dsm_set(DSM_SYNC_RECV_TIME);
		} else {
			// When it is DSMX we can stop because we know all the channels
			dsm_receiver.status = DSM_RECEIVER_RECV;

			// Set the next channel
			dsm_set_next_channel();

			// Start the timer
			timer_dsm_set(DSM_RECV_TIME);
		}
		break;
	case DSM_RECEIVER_SYNC_B:
		// If other error than bad CRC or MFG id doesn't match reject the packet
		if((error && !(rx_status & CYRF_BAD_CRC))
				|| dsm_receiver.packet[0] != dsm.cyrf_mfg_id[2] || dsm_receiver.packet[1] != dsm.cyrf_mfg_id[3])
			break;

		sprintf(cdc_msg, "SYNC2: %i\r\n", dsm.cur_channel);
						cdcacm_send(cdc_msg, strlen(cdc_msg));

		// Invert the CRC when received bad CRC
		if (error && (rx_status & CYRF_BAD_CRC))
			dsm.crc_seed = ~dsm.crc_seed;

		// Stop the timer
		timer_dsm_stop();

		// We now got the other channel and can start receiving
		if (dsm.crc_seed == ((dsm.cyrf_mfg_id[0] << 8) + dsm.cyrf_mfg_id[1])) {
			dsm.channels[0] = dsm.cur_channel;
			dsm.ch_idx = 0;
		}
		else {
			dsm.channels[1] = dsm.cur_channel;
			dsm.ch_idx = 1;
		}
		dsm_receiver.status = DSM_RECEIVER_RECV;

		// Set the next channel
		dsm_set_next_channel();

		// Start the timer
		timer_dsm_set(DSM_RECV_TIME);
		break;
	case DSM_RECEIVER_RECV:
		// If other error than bad CRC or MFG id doesn't match reject the packet
		if((error && !(rx_status & CYRF_BAD_CRC))
				|| dsm_receiver.packet[0] != dsm.cyrf_mfg_id[2] || dsm_receiver.packet[1] != dsm.cyrf_mfg_id[3])
			break;

		sprintf(cdc_msg, "RECV: %i\r\n", dsm.cur_channel);
						cdcacm_send(cdc_msg, strlen(cdc_msg));

		// Invert the CRC when received bad CRC TODO: not change crc seed
		if (error && (rx_status & CYRF_BAD_CRC))
			dsm.crc_seed = ~dsm.crc_seed;

		// Stop the timer
		timer_dsm_stop();

		//TODO: Handle packet

		//TODO: Send data back

		// Set the next channel
		dsm_set_next_channel();

		// Start the timer
		timer_dsm_set(DSM_RECV_TIME);
		break;
	default:
		break;
	}
}

/**
 * When the send is completed (IRQ)
 * @param[in] error When the send was with an error
 */
void dsm_receiver_on_send(bool error) {
	// We can just ignore this to make sure we keep sync
}
