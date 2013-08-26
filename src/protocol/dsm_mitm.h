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

#ifndef PROTOCOL_DSM_MITM_H_
#define PROTOCOL_DSM_MITM_H_

#include "../helper/dsm.h"
#include "../helper/convert.h"

enum dsm_mitm_status {
	DSM_MITM_STOP			= 0x0,			/**< The receiver is stopped */
	DSM_MITM_BIND			= 0x1,			/**< The receiver is binding */
	DSM_MITM_SYNC_A			= 0x2,			/**< The receiver is syncing channel A */
	DSM_MITM_SYNC_B			= 0x3,			/**< The receiver is syncing channel B */
	DSM_MITM_RECV			= 0x4,			/**< The receiver is receiving */
};

struct DsmMitm {
	enum dsm_mitm_status status;				/**< The mitm status */
	enum dsm_protocol protocol;					/**< The type of DSM protocol */
	enum dsm_resolution resolution;				/**< Is true when the transmitters uses 11 bit resolution */

	uint8_t mfg_id[4];							/**< The Manufacturer ID used for binding */
	uint8_t tx_packet[16];						/**< The transmit packet */
	uint8_t tx_packet_length;					/**< The transmit packet length */
	uint32_t tx_packet_count;					/**< The amount of packets send */
	uint8_t rx_packet[16];						/**< The receive packet */
	uint32_t rx_packet_count;					/**< The amount of packets received */

	uint8_t rf_channel;							/**< The current RF channel*/
	uint8_t rf_channel_idx;						/**< The index of the current channel */
	uint8_t rf_channels[23];					/**< The RF channels used for receiving */

	uint8_t sop_col;							/**< The SOP column number */
	uint8_t data_col;							/**< The DATA column number */
	uint16_t crc_seed;							/**< The CRC seed */

	uint8_t missed_packets;						/**< Missed packets since last receive */
	uint8_t num_channels;						/**< The number of channels the transmitter is sending commands over (not RF channels) */

	struct Buffer tx_buffer;					/**< The transmit buffer */
};

#define CHECK_MFG_ID_DATA(protocol, packet, id) ((IS_DSM2(protocol) && packet[0] == (~id[2]&0xFF) && (packet[1] == ((~id[3]+1)&0xFF) || packet[1] == ((~id[3]+2)&0xFF))) || \
		(IS_DSMX(protocol) && packet[0] == id[2] && (packet[1] == id[3]+1 || packet[1] == id[3]+2)))
#define CHECK_MFG_ID_BOTH(protocol, packet, id) (CHECK_MFG_ID(protocol, packet, id) || CHECK_MFG_ID_DATA(protocol, packet, id))

/* External functions */
void dsm_mitm_init(void);
void dsm_mitm_start(void);
void dsm_mitm_stop(void);

#endif /* PROTOCOL_DSM_MITM_H_ */
