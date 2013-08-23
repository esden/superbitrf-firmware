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
#include <string.h>
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/gpio.h>
#include "../modules/timer.h"
#include "../modules/cyrf6936.h"
#include "../modules/led.h"

#include "dsm.h"
#include "dsm_receiver.h"
#include "dsm_transmitter.h"
#include "dsm_mitm.h"

#if DEBUG && DEBUG_DSM
#include "../modules/cdcacm.h"
#include <stdio.h>
#endif

/* The DSM struct */
struct Dsm dsm;

/* The PN codes */
static const u8 pn_codes[5][9][8] = {
{ /* Row 0 */
  /* Col 0 */ {0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8},
  /* Col 1 */ {0x88, 0x17, 0x13, 0x3B, 0x2D, 0xBF, 0x06, 0xD6},
  /* Col 2 */ {0xF1, 0x94, 0x30, 0x21, 0xA1, 0x1C, 0x88, 0xA9},
  /* Col 3 */ {0xD0, 0xD2, 0x8E, 0xBC, 0x82, 0x2F, 0xE3, 0xB4},
  /* Col 4 */ {0x8C, 0xFA, 0x47, 0x9B, 0x83, 0xA5, 0x66, 0xD0},
  /* Col 5 */ {0x07, 0xBD, 0x9F, 0x26, 0xC8, 0x31, 0x0F, 0xB8},
  /* Col 6 */ {0xEF, 0x03, 0x95, 0x89, 0xB4, 0x71, 0x61, 0x9D},
  /* Col 7 */ {0x40, 0xBA, 0x97, 0xD5, 0x86, 0x4F, 0xCC, 0xD1},
  /* Col 8 */ {0xD7, 0xA1, 0x54, 0xB1, 0x5E, 0x89, 0xAE, 0x86}
},
{ /* Row 1 */
  /* Col 0 */ {0x83, 0xF7, 0xA8, 0x2D, 0x7A, 0x44, 0x64, 0xD3},
  /* Col 1 */ {0x3F, 0x2C, 0x4E, 0xAA, 0x71, 0x48, 0x7A, 0xC9},
  /* Col 2 */ {0x17, 0xFF, 0x9E, 0x21, 0x36, 0x90, 0xC7, 0x82},
  /* Col 3 */ {0xBC, 0x5D, 0x9A, 0x5B, 0xEE, 0x7F, 0x42, 0xEB},
  /* Col 4 */ {0x24, 0xF5, 0xDD, 0xF8, 0x7A, 0x77, 0x74, 0xE7},
  /* Col 5 */ {0x3D, 0x70, 0x7C, 0x94, 0xDC, 0x84, 0xAD, 0x95},
  /* Col 6 */ {0x1E, 0x6A, 0xF0, 0x37, 0x52, 0x7B, 0x11, 0xD4},
  /* Col 7 */ {0x62, 0xF5, 0x2B, 0xAA, 0xFC, 0x33, 0xBF, 0xAF},
  /* Col 8 */ {0x40, 0x56, 0x32, 0xD9, 0x0F, 0xD9, 0x5D, 0x97}
},
{ /* Row 2 */
  /* Col 0 */ {0x40, 0x56, 0x32, 0xD9, 0x0F, 0xD9, 0x5D, 0x97},
  /* Col 1 */ {0x8E, 0x4A, 0xD0, 0xA9, 0xA7, 0xFF, 0x20, 0xCA},
  /* Col 2 */ {0x4C, 0x97, 0x9D, 0xBF, 0xB8, 0x3D, 0xB5, 0xBE},
  /* Col 3 */ {0x0C, 0x5D, 0x24, 0x30, 0x9F, 0xCA, 0x6D, 0xBD},
  /* Col 4 */ {0x50, 0x14, 0x33, 0xDE, 0xF1, 0x78, 0x95, 0xAD},
  /* Col 5 */ {0x0C, 0x3C, 0xFA, 0xF9, 0xF0, 0xF2, 0x10, 0xC9},
  /* Col 6 */ {0xF4, 0xDA, 0x06, 0xDB, 0xBF, 0x4E, 0x6F, 0xB3},
  /* Col 7 */ {0x9E, 0x08, 0xD1, 0xAE, 0x59, 0x5E, 0xE8, 0xF0},
  /* Col 8 */ {0xC0, 0x90, 0x8F, 0xBB, 0x7C, 0x8E, 0x2B, 0x8E}
},
{ /* Row 3 */
  /* Col 0 */ {0xC0, 0x90, 0x8F, 0xBB, 0x7C, 0x8E, 0x2B, 0x8E},
  /* Col 1 */ {0x80, 0x69, 0x26, 0x80, 0x08, 0xF8, 0x49, 0xE7},
  /* Col 2 */ {0x7D, 0x2D, 0x49, 0x54, 0xD0, 0x80, 0x40, 0xC1},
  /* Col 3 */ {0xB6, 0xF2, 0xE6, 0x1B, 0x80, 0x5A, 0x36, 0xB4},
  /* Col 4 */ {0x42, 0xAE, 0x9C, 0x1C, 0xDA, 0x67, 0x05, 0xF6},
  /* Col 5 */ {0x9B, 0x75, 0xF7, 0xE0, 0x14, 0x8D, 0xB5, 0x80},
  /* Col 6 */ {0xBF, 0x54, 0x98, 0xB9, 0xB7, 0x30, 0x5A, 0x88},
  /* Col 7 */ {0x35, 0xD1, 0xFC, 0x97, 0x23, 0xD4, 0xC9, 0x88},
  /* Col 8 */ {0x88, 0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40}
},
{ /* Row 4 */
  /* Col 0 */ {0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40, 0x93},
  /* Col 1 */ {0xDC, 0x68, 0x08, 0x99, 0x97, 0xAE, 0xAF, 0x8C},
  /* Col 2 */ {0xC3, 0x0E, 0x01, 0x16, 0x0E, 0x32, 0x06, 0xBA},
  /* Col 3 */ {0xE0, 0x83, 0x01, 0xFA, 0xAB, 0x3E, 0x8F, 0xAC},
  /* Col 4 */ {0x5C, 0xD5, 0x9C, 0xB8, 0x46, 0x9C, 0x7D, 0x84},
  /* Col 5 */ {0xF1, 0xC6, 0xFE, 0x5C, 0x9D, 0xA5, 0x4F, 0xB7},
  /* Col 6 */ {0x58, 0xB5, 0xB3, 0xDD, 0x0E, 0x28, 0xF1, 0xB0},
  /* Col 7 */ {0x5F, 0x30, 0x3B, 0x56, 0x96, 0x45, 0xF4, 0xA1},
  /* Col 8 */ {0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8}
},
};
static const u8 pn_bind[] = { 0x98, 0x88, 0x1B, 0xE4, 0x30, 0x79, 0x03, 0x84 };

/*The CYRF initial config, binding config and transfer config */
static const u8 cyrf_config[][2] = {
		{CYRF_MODE_OVERRIDE, CYRF_RST},											// Reset the device
		{CYRF_CLK_EN, CYRF_RXF},												// Enable the clock
		{CYRF_AUTO_CAL_TIME, 0x3C},												// From manual, needed for initialization
		{CYRF_AUTO_CAL_OFFSET, 0x14},											// From manual, needed for initialization
		{CYRF_RX_CFG, CYRF_LNA | CYRF_FAST_TURN_EN},							// Enable low noise amplifier and fast turning
		{CYRF_TX_OFFSET_LSB, 0x55},												// From manual, typical configuration
		{CYRF_TX_OFFSET_MSB, 0x05},												// From manual, typical configuration
		{CYRF_XACT_CFG, CYRF_MODE_SYNTH_RX | CYRF_FRC_END},						// Force in Synth RX mode
		{CYRF_TX_CFG, CYRF_DATA_CODE_LENGTH | CYRF_DATA_MODE_SDR | CYRF_PA_4},	// Enable 64 chip codes, SDR mode and amplifier +4dBm
		{CYRF_DATA64_THOLD, 0x0E},												// From manual, typical configuration
		{CYRF_XACT_CFG, CYRF_MODE_SYNTH_RX},									// Set in Synth RX mode (again, really needed?)
};
static const u8 cyrf_bind_config[][2] = {
		{CYRF_TX_CFG, CYRF_DATA_CODE_LENGTH | CYRF_DATA_MODE_SDR | CYRF_PA_4},	// Enable 64 chip codes, SDR mode and amplifier +4dBm
		{CYRF_FRAMING_CFG, CYRF_SOP_LEN | 0xE},									// Set SOP CODE to 64 chips and SOP Correlator Threshold to 0xE
		{CYRF_RX_OVERRIDE, CYRF_FRC_RXDR | CYRF_DIS_RXCRC},						// Force receive data rate and disable receive CRC checker
		{CYRF_EOP_CTRL, 0x02},													// Only enable EOP symbol count of 2
		{CYRF_TX_OVERRIDE, CYRF_DIS_TXCRC},										// Disable transmit CRC generate
};
static const u8 cyrf_transfer_config[][2] = {
		{CYRF_TX_CFG, CYRF_DATA_CODE_LENGTH | CYRF_DATA_MODE_8DR | CYRF_PA_4},	// Enable 64 chip codes, 8DR mode and amplifier +4dBm
		{CYRF_FRAMING_CFG, CYRF_SOP_EN | CYRF_SOP_LEN | CYRF_LEN_EN | 0xE},		// Set SOP CODE enable, SOP CODE to 64 chips, Packet length enable, and SOP Correlator Threshold to 0xE
		{CYRF_TX_OVERRIDE, 0x00},												// Reset TX overrides
		{CYRF_RX_OVERRIDE, 0x00},												// Reset RX overrides
};

static void dsm_generate_channels(void);

/**
 * Initialize DSM
 */
void dsm_init(void) {
#if DEBUG && DEBUG_DSM
	sprintf(cdc_msg, "DSM START INIT\r\n");
	cdcacm_send(cdc_msg, strlen(cdc_msg));
#endif
	// Configure the CYRF
	cyrf_set_config(cyrf_config, sizeof(cyrf_config)/2);

	// Read the CYRF MFG
	cyrf_get_mfg_id(dsm.cyrf_mfg_id);

	// Register timer, receive and send callback
#ifdef DSM_TRANSMITTER
	timer_dsm_register_callback(dsm_transmitter_on_timer);
	cyrf_register_recv_callback(dsm_transmitter_on_receive);
	cyrf_register_send_callback(dsm_transmitter_on_send);
#else
#ifdef DSM_MITM
	timer_dsm_register_callback(dsm_mitm_on_timer);
	cyrf_register_recv_callback(dsm_mitm_on_receive);
	cyrf_register_send_callback(dsm_mitm_on_send);
#else
	timer_dsm_register_callback(dsm_receiver_on_timer);
	cyrf_register_recv_callback(dsm_receiver_on_receive);
	cyrf_register_send_callback(dsm_receiver_on_send);
#endif
#endif
}

/**
 * Start with binding
 */
void dsm_start_bind(void) {
#if DEBUG && DEBUG_DSM
	sprintf(cdc_msg, "DSM START BIND\r\n");
	cdcacm_send(cdc_msg, strlen(cdc_msg));
#endif
	u8 data_code[16];
	// Set the CYRF configuration
	cyrf_set_config(cyrf_bind_config, sizeof(cyrf_bind_config)/2);

	// Set the leds
	LED_ON(1);
	LED_OFF(2);

	// Set the CYRF data code
	memcpy(data_code, pn_codes[0][8], 8);
	memcpy(data_code + 8, pn_bind, 8);
	cyrf_set_data_code(data_code);

	// Update the status
	dsm.status = DSM_BIND;

	// Check if receiver or transmitter
#ifdef DSM_TRANSMITTER
	dsm_transmitter_start_bind();
#else
#ifdef DSM_MITM
	dsm_mitm_start_bind();
#else
	dsm_receiver_start_bind();
#endif
#endif
}

/**
 * Start with sending/receiving
 */
void dsm_start(void) {
#if DEBUG && DEBUG_DSM
	sprintf(cdc_msg, "DSM START\r\n");
	cdcacm_send(cdc_msg, strlen(cdc_msg));
#endif
	// Set the CYRF configuration
	cyrf_set_config(cyrf_transfer_config, sizeof(cyrf_transfer_config)/2);

	// Set the leds
	LED_OFF(1);
	LED_ON(2);

	dsm.cyrf_mfg_id[0] = 0xF2;
		dsm.cyrf_mfg_id[1] = 0x39;
		dsm.cyrf_mfg_id[2] = 0xB7;
		dsm.cyrf_mfg_id[3] = 0x77;

	// Generate the channels
	dsm_generate_channels();

	// Calculate the CRC seed, SOP column and Data column
	dsm.crc_seed = ~((dsm.cyrf_mfg_id[0] << 8) + dsm.cyrf_mfg_id[1]);
	dsm.sop_col = (dsm.cyrf_mfg_id[0] + dsm.cyrf_mfg_id[1] + dsm.cyrf_mfg_id[2] + 2) & 0x07;
	dsm.data_col = 7 - dsm.sop_col;

	// Update the status and set channel id to the last
	dsm.status = DSM_RDY;
	dsm.ch_idx = IS_DSM2(dsm.protocol)? 1 : 22;

	// Reset the packet loss detection
	dsm.packet_loss_bit = false;
	dsm.packet_loss = false;

	// Check if receiver or transmitter
#ifdef DSM_TRANSMITTER
	dsm_transmitter_start();
#else
#ifdef DSM_MITM
	dsm_mitm_start();
#else
	dsm_receiver_start();
#endif
#endif
}

/**
 * Generate the channels
 */
static void dsm_generate_channels(void) {
	// Check if is a DSM2 or DSMX protocol
	if (IS_DSM2(dsm.protocol) || DSM_FORCE_DSM2) {
		// Just generate 2 random channels
		dsm.channels[0] = 0x46;
		dsm.channels[1] = 0x1B;
	} else {
		// Calculate the DSMX channels
		int idx = 0;
		u32 id = ~((dsm.cyrf_mfg_id[0] << 24) | (dsm.cyrf_mfg_id[1] << 16) |
					(dsm.cyrf_mfg_id[2] << 8) | (dsm.cyrf_mfg_id[3] << 0));
		u32 id_tmp = id;

		// While not all channels are set
		while(idx < 23) {
			int i;
			int count_3_27 = 0, count_28_51 = 0, count_52_76 = 0;

			id_tmp = id_tmp * 0x0019660D + 0x3C6EF35F; // Randomization
			u8 next_ch = ((id_tmp >> 8) % 0x49) + 3;       // Use least-significant byte and must be larger than 3
			if (((next_ch ^ id) & 0x01 ) == 0)
				continue;

			// Go trough all already set channels
			for (i = 0; i < idx; i++) {
				// Channel is already used
				if(dsm.channels[i] == next_ch)
					break;

				// Count the channel groups
				if(dsm.channels[i] <= 27)
					count_3_27++;
				else if (dsm.channels[i] <= 51)
					count_28_51++;
				else
					count_52_76++;
			}

			// When channel is already used continue
			if (i != idx)
				continue;

			// Set the channel when channel groups aren't full
			if ((next_ch < 28 && count_3_27 < 8)						// Channels 3-27: max 8
			  || (next_ch >= 28 && next_ch < 52 && count_28_51 < 7)		// Channels 28-52: max 7
			  || (next_ch >= 52 && count_52_76 < 8)) {					// Channels 52-76: max 8
				dsm.channels[idx++] = next_ch;
			}
		}
	}
}

/**
 * Set the current channel with SOP, CRC and data code
 * @param[in] channel The channel that needs to be set
 */
void dsm_set_channel(u8 channel) {
	u8 pn_row;
	dsm.cur_channel = channel;
	pn_row = (IS_DSM2(dsm.protocol) || DSM_FORCE_DSM2)? channel % 5 : (channel-2) % 5;

#if DEBUG && DEBUG_DSM
	sprintf(cdc_msg, "DSM SET CHANNEL: 0x%02X (mfg_id[0]: 0x%02X, mfg_id[1]: 0x%02X, mfg_id[2]: 0x%02X, mfg_id[3]: 0x%02X, pn_row: 0x%02X, data_col: 0x%02X, sop_col: 0x%02X)\r\n",
			channel, dsm.cyrf_mfg_id[0], dsm.cyrf_mfg_id[1], dsm.cyrf_mfg_id[2], dsm.cyrf_mfg_id[3], pn_row, dsm.data_col, dsm.sop_col);
	cdcacm_send(cdc_msg, strlen(cdc_msg));
#endif

	// Update the CRC, SOP and Data code
	dsm.crc_seed = ~dsm.crc_seed;
	cyrf_set_crc_seed(dsm.crc_seed);
	cyrf_set_sop_code(pn_codes[pn_row][dsm.sop_col]);
	cyrf_set_data_code(pn_codes[pn_row][dsm.data_col]);

	// Change channel
	cyrf_set_channel(channel);
}

/**
 * Set the current channel without SOP, CRC and data code
 * @param[in] channel The channel that needs to be set
 */
void dsm_set_channel_raw(u8 channel) {
	dsm.cur_channel = channel;
	cyrf_set_channel(channel);
}

/**
 * Set to next channel with SOP, CRC and data code
 */
void dsm_set_next_channel(void) {
	// Update the channel
	dsm.ch_idx = (IS_DSM2(dsm.protocol) || DSM_FORCE_DSM2)? (dsm.ch_idx+1) % 2 : (dsm.ch_idx+1) % 23;
	dsm_set_channel(dsm.channels[dsm.ch_idx]);
}
