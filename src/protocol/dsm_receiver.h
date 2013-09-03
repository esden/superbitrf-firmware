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

#ifndef PROTOCOL_DSM_RECEIVER_H_
#define PROTOCOL_DSM_RECEIVER_H_

#include "../helper/dsm.h"

enum dsm_receiver_status {
	DSM_RECEIVER_STOP			= 0x0,			/**< The receiver is stopped */
	DSM_RECEIVER_BIND			= 0x1,			/**< The receiver is binding */
	DSM_RECEIVER_SYNC_A			= 0x2,			/**< The receiver is syncing channel A */
	DSM_RECEIVER_SYNC_B			= 0x3,			/**< The receiver is syncing channel B */
	DSM_RECEIVER_RECV			= 0x4,			/**< The receiver is receiving */
};

struct DsmReceiver {
	enum dsm_receiver_status status;			/**< The receiver status */
	enum dsm_protocol protocol;					/**< The type of DSM protocol */
	enum dsm_resolution resolution;				/**< Is true when the transmitters uses 11 bit resolution */

	uint8_t mfg_id[4];							/**< The Manufacturer ID used for binding */

	uint8_t rf_channel;							/**< The current RF channel*/
	uint8_t rf_channel_idx;						/**< The index of the current channel */
	uint8_t rf_channels[23];					/**< The RF channels used for receiving */

	uint8_t sop_col;							/**< The SOP column number */
	uint8_t data_col;							/**< The DATA column number */
	uint16_t crc_seed;							/**< The CRC seed */

	uint8_t missed_packets;					/**< Missed packets since last receive */
	uint8_t num_channels;						/**< The number of channels the transmitter is sending commands over (not RF channels) */
};

/* External functions */
void dsm_receiver_init(void);
void dsm_receiver_start(void);
void dsm_receiver_stop(void);

#endif /* PROTOCOL_DSM_RECEIVER_H_ */
