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

#ifndef PROTOCOL_DSM_TRANSMITTER_H_
#define PROTOCOL_DSM_TRANSMITTER_H_

#include "../helper/dsm.h"

enum dsm_transmitter_status {
	DSM_TRANSMITTER_STOP		= 0x0,			/**< The transmitter is stopped */
	DSM_TRANSMITTER_BIND		= 0x1,			/**< The transmitter bind status */
	DSM_TRANSMITTER_SENDA		= 0x2,			/**< The transmitter send on channel A status */
	DSM_TRANSMITTER_SENDB		= 0x3,			/**< The transmitter send on channel B status */
};

struct DsmTransmitter {
	enum dsm_transmitter_status status;			/**< The receiver status */
	enum dsm_protocol protocol;					/**< The type of DSM protocol */
	enum dsm_resolution resolution;				/**< Is true when the transmitters uses 11 bit resolution */

	uint8_t mfg_id[4];							/**< The Manufacturer ID used for binding */
	uint8_t tx_packet[16];						/**< The transmit packet */
	uint8_t tx_packet_length;					/**< The transmit packet length */
	uint32_t tx_packet_count;					/**< The amount of packets send */

	uint8_t rf_channel;							/**< The current RF channel*/
	uint8_t rf_channel_idx;						/**< The index of the current channel */
	uint8_t rf_channels[23];					/**< The RF channels used for transmitting */

	uint8_t sop_col;							/**< The SOP column number */
	uint8_t data_col;							/**< The DATA column number */
	uint16_t crc_seed;							/**< The CRC seed */
	uint8_t num_channels;						/**< The number of channels the transmitter is sending commands over (not RF channels) */
};

/* External functions */
void dsm_transmitter_init(void);
void dsm_transmitter_start(void);
void dsm_transmitter_stop(void);

#endif /* PROTOCOL_DSM_TRANSMITTER_H_ */
