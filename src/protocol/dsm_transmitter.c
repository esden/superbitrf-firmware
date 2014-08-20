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
#include "../modules/config.h"
#include "../modules/led.h"
#include "../modules/button.h"
#include "../modules/timer.h"
#include "../modules/cyrf6936.h"
#include "../helper/convert.h"

#include "dsm_transmitter.h"

struct DsmTransmitter dsm_transmitter;

void dsm_transmitter_start_bind(void);
void dsm_transmitter_start_transfer(void);
void dsm_transmitter_timer_cb(void);
void dsm_transmitter_receive_cb(bool error);
void dsm_transmitter_send_cb(bool error);
void dsm_transmitter_cdcacm_cb(char *data, int size);

void dsm_transmitter_set_rf_channel(uint8_t chan);
void dsm_transmitter_set_channel(uint8_t chan);
void dsm_transmitter_set_next_channel(void);

static void dsm_transmitter_create_bind_packet(void);
void dsm_transmitter_create_command_packet(uint8_t commands[]);

/**
 * DSM Transmitter protocol initialization
 */
void dsm_transmitter_init(void) {
	uint8_t mfg_id[6];
	DEBUG(protocol, "DSM Transmitter initializing");
	dsm_transmitter.status = DSM_TRANSMITTER_STOP;

	// Configure the CYRF
	cyrf_set_config_len(cyrf_config, dsm_config_size());

	// Read the CYRF MFG
	cyrf_get_mfg_id(mfg_id);

	// Setup the buffer
	convert_init(&dsm_transmitter.tx_buffer);

	// Copy the MFG id
	if(usbrf_config.dsm_bind_mfg_id[0] == 0 && usbrf_config.dsm_bind_mfg_id[1] == 0 && usbrf_config.dsm_bind_mfg_id[2] == 0 && usbrf_config.dsm_bind_mfg_id[3] == 0)
		memcpy(dsm_transmitter.mfg_id, mfg_id, 4);
	else
		memcpy(dsm_transmitter.mfg_id, usbrf_config.dsm_bind_mfg_id, 4);

	// Copy other config
	dsm_transmitter.num_channels = usbrf_config.dsm_num_channels;
	dsm_transmitter.protocol = usbrf_config.dsm_protocol;

	// Stop the timer
	timer_dsm_stop();

	// Set the callbacks
	timer_dsm_register_callback(dsm_transmitter_timer_cb);
	cyrf_register_recv_callback(dsm_transmitter_receive_cb);
	cyrf_register_send_callback(dsm_transmitter_send_cb);
	button_bind_register_callback(dsm_transmitter_start_bind);
	cdcacm_register_receive_callback(dsm_transmitter_cdcacm_cb);

	DEBUG(protocol, "DSM Transmitter initialized 0x%02X 0x%02X 0x%02X 0x%02X", mfg_id[0], mfg_id[1], mfg_id[2], mfg_id[3]);
}

/**
 * DSM Transmitter protocol start
 */
void dsm_transmitter_start(void) {
	DEBUG(protocol, "DSM Transmitter starting");

	// Check if need to start with binding procedure
	if(usbrf_config.dsm_start_bind)
		dsm_transmitter_start_bind();
	else
		dsm_transmitter_start_transfer();
}

/**
 * DSM Transmitter protocol stop
 */
void dsm_transmitter_stop(void) {
	// Stop the timer
	timer_dsm_stop();
	dsm_transmitter.status = DSM_TRANSMITTER_STOP;
}


/**
 * DSM Transmitter start bind
 */
void dsm_transmitter_start_bind(void) {
	uint8_t data_code[16];
	DEBUG(protocol, "DSM Transmitter start bind");

	dsm_transmitter.status = DSM_TRANSMITTER_BIND;
	dsm_transmitter.tx_packet_count = 0;

	// Set the bind led on
#ifdef LED_BIND
	LED_ON(LED_BIND);
#endif

	// Set RX led off
#ifdef LED_RX
	LED_OFF(LED_RX);
#endif

	// Set TX led off
#ifdef LED_TX
	LED_OFF(LED_TX);
#endif

	// Stop the timer
	timer_dsm_stop();

	// Set the CYRF configuration
	cyrf_set_config_len(cyrf_bind_config, dsm_bind_config_size());

	// Set the CYRF data code
	memcpy(data_code, pn_codes[0][8], 8);
	memcpy(data_code + 8, pn_bind, 8);
	cyrf_set_data_code(data_code);

	// Set the initial bind channel
	if(usbrf_config.dsm_bind_channel > 0)
		dsm_transmitter_set_rf_channel(usbrf_config.dsm_bind_channel);
	else
		dsm_transmitter_set_rf_channel(rand() / (RAND_MAX / usbrf_config.dsm_max_channel + 1));

	// Create the bind packet and start transmitting mode
	dsm_transmitter_create_bind_packet();
	cyrf_start_transmit();

	// Enable the timer
	timer_dsm_set(DSM_BIND_SEND_TIME);
}

/**
 * DSM Transmitter start transfer
 */
void dsm_transmitter_start_transfer(void) {
	DEBUG(protocol, "DSM Transmitter start transfer");

	dsm_transmitter.status = DSM_TRANSMITTER_SENDA;
	dsm_transmitter.rf_channel_idx = 0;
	dsm_transmitter.tx_packet_count = 0;

	// Set the bind led off
#ifdef LED_BIND
	LED_OFF(LED_BIND);
#endif

	// Set RX led off
#ifdef LED_RX
	LED_OFF(LED_RX);
#endif

	// Set TX led off
#ifdef LED_TX
	LED_OFF(LED_TX);
#endif

	// Set the CYRF configuration
	cyrf_set_config_len(cyrf_transfer_config, dsm_transfer_config_size());

	dsm_transmitter.num_channels = usbrf_config.dsm_num_channels;
	dsm_transmitter.protocol = usbrf_config.dsm_protocol;
	dsm_transmitter.resolution = (dsm_transmitter.protocol & 0x10)>>4;

	// Calculate the CRC seed, SOP column and Data column
	dsm_transmitter.crc_seed = ~((dsm_transmitter.mfg_id[0] << 8) + dsm_transmitter.mfg_id[1]);
	dsm_transmitter.sop_col = (dsm_transmitter.mfg_id[0] + dsm_transmitter.mfg_id[1] + dsm_transmitter.mfg_id[2] + 2) & 0x07;
	dsm_transmitter.data_col = 7 - dsm_transmitter.sop_col;

	DEBUG(protocol, "DSM Transmitter bound(MFG_ID: {0x%02X, 0x%02X, 0x%02X, 0x%02X}, num_channels: 0x%02X, protocol: 0x%02X, resolution: 0x%02X, sop_col: 0x%02X, data_col 0x%02X)",
				dsm_transmitter.mfg_id[0], dsm_transmitter.mfg_id[1], dsm_transmitter.mfg_id[2], dsm_transmitter.mfg_id[3],
				dsm_transmitter.num_channels, dsm_transmitter.protocol, dsm_transmitter.resolution, dsm_transmitter.sop_col, dsm_transmitter.data_col);

	// When DSMX generate channels and set channel
	if(IS_DSMX(dsm_transmitter.protocol)) {
		dsm_generate_channels_dsmx(dsm_transmitter.mfg_id, dsm_transmitter.rf_channels);
		dsm_transmitter.rf_channel_idx = 22;
		dsm_transmitter_set_next_channel();
	} else {
		dsm_transmitter.rf_channels[0] = 0x15;
		dsm_transmitter.rf_channels[1] = 0x3C;
		dsm_transmitter_set_next_channel();
	}

	// Start transmitting mode
	cyrf_start_transmit();

	// Start the timer
	timer_dsm_set(DSM_CHA_CHB_SEND_TIME);
}

/**
 * DSM Transmitter timer callback
 */
void dsm_transmitter_timer_cb(void) {
	// Check the transmitter status
	switch (dsm_transmitter.status) {
	case DSM_TRANSMITTER_BIND:
		// Abort the send
		cyrf_write_register(CYRF_XACT_CFG, CYRF_MODE_SYNTH_TX | CYRF_FRC_END);
		cyrf_write_register(CYRF_RX_ABORT, 0x00);

		// Send the bind packet again
		cyrf_send_len(dsm_transmitter.tx_packet, dsm_transmitter.tx_packet_length);

		// Check for switching back
		if (dsm_transmitter.tx_packet_count >= usbrf_config.dsm_bind_packets)
			dsm_transmitter_start_transfer();
		else {
			// Start the timer
			timer_dsm_set(DSM_BIND_SEND_TIME);
		}
		break;
	case DSM_TRANSMITTER_SENDA:
		// Start the timer as first so we make sure the timing is right
		timer_dsm_stop();
		timer_dsm_set(DSM_CHA_CHB_SEND_TIME);

		// Start transmitting mode
		cyrf_start_transmit();

		// Update the channel
		dsm_transmitter_set_next_channel();

		// Abort the send
		cyrf_write_register(CYRF_XACT_CFG, CYRF_MODE_SYNTH_TX | CYRF_FRC_END);
		cyrf_write_register(CYRF_RX_ABORT, 0x00);

		// Change the status
		dsm_transmitter.status = DSM_TRANSMITTER_SENDB;

		// Create and send the packet
		//dsm_transmitter_create_data_packet(); TODO
		if(convert_extract_size(&dsm_transmitter.tx_buffer) > 14) {
			uint8_t tx_data[14];
			convert_extract(&dsm_transmitter.tx_buffer, tx_data, 14);
			dsm_transmitter_create_command_packet(tx_data);
		}
		cyrf_send_len(dsm_transmitter.tx_packet, dsm_transmitter.tx_packet_length);
		break;
	case DSM_TRANSMITTER_SENDB:
		// Start the timer as first so we make sure the timing is right
		timer_dsm_stop();
		timer_dsm_set(DSM_SEND_TIME - DSM_CHA_CHB_SEND_TIME);

		// Start transmitting mode
		cyrf_start_transmit();

		// Update the channel
		dsm_transmitter_set_next_channel();

		// Abort the send
		cyrf_write_register(CYRF_XACT_CFG, CYRF_MODE_SYNTH_TX | CYRF_FRC_END);
		cyrf_write_register(CYRF_RX_ABORT, 0x00);

		// Change the status
		dsm_transmitter.status = DSM_TRANSMITTER_SENDA;

		// Send same packet again on other channel
		cyrf_send_len(dsm_transmitter.tx_packet, dsm_transmitter.tx_packet_length);
		break;
	default:
		break;
	}
}

/**
 * DSM Transmitter receive callback
 */
void dsm_transmitter_receive_cb(bool error) {
	(void) error;
}

/**
 * DSM Transmitter send callback
 */
void dsm_transmitter_send_cb(bool error) {
	(void) error;
	dsm_transmitter.tx_packet_count++;

	// Set TX led on
#ifdef LED_TX
	LED_ON(LED_TX);
#endif
}

/**
 * DSM Transmitter CDCACM receive callback
 */
void dsm_transmitter_cdcacm_cb(char *data, int size) {
	convert_insert(&dsm_transmitter.tx_buffer, (uint8_t*)data, size);
}


/**
 * Change DSM Transmitter RF channel
 * @param[in] chan The channel that need to be switched to
 */
void dsm_transmitter_set_rf_channel(uint8_t chan) {
	dsm_transmitter.rf_channel = chan;
	cyrf_set_channel(chan);
}

/**
 * Change DSM Transmitter RF channel and also set SOP, CRC and DATA code
 * @param[in] chan The channel that need to be switched to
 */
void dsm_transmitter_set_channel(uint8_t chan) {
	dsm_transmitter.crc_seed	= ~dsm_transmitter.crc_seed;
	dsm_transmitter.rf_channel 	= chan;
	dsm_set_channel(dsm_transmitter.rf_channel, IS_DSM2(dsm_transmitter.protocol),
			dsm_transmitter.sop_col, dsm_transmitter.data_col, dsm_transmitter.crc_seed);
}

/**
 * Change DSM Transmitter RF channel to the next channel and also set SOP, CRC and DATA code
 */
void dsm_transmitter_set_next_channel(void) {
	dsm_transmitter.rf_channel_idx	= IS_DSM2(dsm_transmitter.protocol)? (dsm_transmitter.rf_channel_idx+1) % 2 : (dsm_transmitter.rf_channel_idx+1) % 23;
	dsm_transmitter.crc_seed		= ~dsm_transmitter.crc_seed;
	dsm_transmitter.rf_channel 		= dsm_transmitter.rf_channels[dsm_transmitter.rf_channel_idx];
	dsm_set_channel(dsm_transmitter.rf_channel, IS_DSM2(dsm_transmitter.protocol),
			dsm_transmitter.sop_col, dsm_transmitter.data_col, dsm_transmitter.crc_seed);
}

/**
 * Create DSM Transmitter Bind packet
 */
static void dsm_transmitter_create_bind_packet(void) {
	uint8_t i;
	uint16_t sum = 384 - 0x10;

	dsm_transmitter.tx_packet[0] = ~dsm_transmitter.mfg_id[0];
	dsm_transmitter.tx_packet[1] = ~dsm_transmitter.mfg_id[1];
	dsm_transmitter.tx_packet[2] = ~dsm_transmitter.mfg_id[2];
	dsm_transmitter.tx_packet[3] = ~dsm_transmitter.mfg_id[3];
	dsm_transmitter.tx_packet[4] = dsm_transmitter.tx_packet[0];
	dsm_transmitter.tx_packet[5] = dsm_transmitter.tx_packet[1];
	dsm_transmitter.tx_packet[6] = dsm_transmitter.tx_packet[2];
	dsm_transmitter.tx_packet[7] = dsm_transmitter.tx_packet[3];

	// Calculate the sum
	for (i = 0; i < 8; i++)
		sum += dsm_transmitter.tx_packet[i];

	dsm_transmitter.tx_packet[8] = sum >> 8;
	dsm_transmitter.tx_packet[9] = sum & 0xFF;
	dsm_transmitter.tx_packet[10] = 0x01; //???
	dsm_transmitter.tx_packet[11] = dsm_transmitter.num_channels;
	dsm_transmitter.tx_packet[12] = dsm_transmitter.protocol;
	dsm_transmitter.tx_packet[13] = 0x00; //???

	// Calculate the sum
	for (i = 8; i < 14; i++)
		sum += dsm_transmitter.tx_packet[i];

	dsm_transmitter.tx_packet[14] = sum >> 8;
	dsm_transmitter.tx_packet[15] = sum & 0xFF;

	// Set the length
	dsm_transmitter.tx_packet_length = 16;
}

/**
 * Create DSM Transmitter command packet
 */
void dsm_transmitter_create_command_packet(uint8_t commands[]) {
	int i;
	if(IS_DSM2(dsm_transmitter.protocol)) {
		dsm_transmitter.tx_packet[0] = ~dsm_transmitter.mfg_id[2];
		dsm_transmitter.tx_packet[1] = ~dsm_transmitter.mfg_id[3];
	} else {
		dsm_transmitter.tx_packet[0] = dsm_transmitter.mfg_id[2];
		dsm_transmitter.tx_packet[1] = dsm_transmitter.mfg_id[3];
	}

	// Copy the commands
	for(i = 0; i < 14; i++)
		dsm_transmitter.tx_packet[i+2] = commands[i];

	// Set the length
	dsm_transmitter.tx_packet_length = 16;
}
