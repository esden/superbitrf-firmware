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

#ifndef PROTOCOL_DSM_H_
#define PROTOCOL_DSM_H_

#include "../board.h"

#ifndef DSM_RECEIVER
#define DSM_TRANSMITTER				1
#endif

#ifndef DSM_MAX_CHANNEL
#define DSM_MAX_CHANNEL				0x62
#endif

#ifndef DSM_PROTOCOL
#define DSM_PROTOCOL				DSM_DSM2P
#endif

//#define DSM_BIND_CHANNEL			0x0D

/* All times are in microseconds divided by 10 */
#define DSM_BIND_RECV_TIME			1000		/**< Time before timeout when receiving bind packets */
#define DSM_SYNC_RECV_TIME			1200		/**< Time before timeout when trying to sync */
#define DSM_SYNC_FRECV_TIME			10000		/**< Time before timeout when trying to sync first packet of DSMX (bigger then bind sending) */
#define DSM_RECV_TIME				2400		/**< Time before timeout when trying to receive */
#define DSM_BIND_SEND_TIME			1000		/**< Time between sending bind packets */
#define DSM_SEND_TIME				1400		/**< Time between sending both Channel A and Channel B */
#define DSM_CHA_CHB_SEND_TIME		700			/**< Time between Channel A and Channel B send */

#ifdef DEBUG
#define DSM_BIND_SEND_COUNT			3			/**< The number of bind packets to send */
#else
#define DSM_BIND_SEND_COUNT			300			/**< The number of bind packets to send */
#endif

enum dsm_protocol {
	DSM_DSM2_1			= 0x01,		/**< The original DSM2 protocol with 1 packet of data */
	DSM_DSM2_2			= 0x02,		/**< The original DSM2 protocol with 2 packets of data */
	DSM_DSM2P			= 0x10,		/**< Our own DSM2 Paparazzi protocol */
	DSM_DSMXP			= 0x11,		/**< Our own DSMX Paparazzi protocol */
	DSM_DSMX_1			= 0xA2,		/**< The original DSMX protocol with 1 packet of data */
	DSM_DSMX_2			= 0xB2,		/**< The original DSMX protocol with 2 packets of data */
};
#define IS_DSM2(x)			(x == DSM_DSM2P || x == DSM_DSM2_1 || x == DSM_DSM2_2)
#define IS_DSMX(x)			(!IS_DSM2(x))

enum dsm_resolution {
	DSM_10_BIT_RESOLUTION			= 0x00,		/**< It has a 10 bit resolution */
	DSM_11_BIT_RESOLUTION			= 0x01,		/**< It has a 11 bit resolution */
};

enum dsm_status {
	DSM_BIND			= 0x0,		/**< The binding status */
	DSM_RDY				= 0x1,		/**< The ready status */
};

struct Dsm {
	enum dsm_protocol protocol;		/**< The type of DSM protocol */
	enum dsm_resolution resolution;	/**< Is true when the transmitters uses 11 bit resolution */
	enum dsm_status status;			/**< The status of DSM */
	u8 cyrf_mfg_id[6];				/**< The device or the received MFG id */
	u8 cur_channel;					/**< The current channel number */
	u8 channels[23];				/**< The channels that the protocol uses */
	u8 ch_idx;						/**< The current channel index */
	u16 crc_seed;					/**< The current CRC seed */
	u8 sop_col;						/**< The calculated SOP column */
	u8 data_col;					/**< The calculated data column */
	u8 transmit_packet[16];			/**< The packet that gets transmitted */
	u8 transmit_packet_length;		/**< THe length of the transmit packet */
	u8 receive_packet[16];			/**< The packet that gets received */
	u8 packet_loss_bit;				/**< This bit is used to detect packet loss */
	bool packet_loss;				/**< This is set when a packet loss is detected*/
};
extern struct Dsm dsm;

/* External functions */
void dsm_init(void);
void dsm_start_bind(void);
void dsm_start(void);

void dsm_set_channel(u8 channel);
void dsm_set_channel_raw(u8 channel);
void dsm_set_next_channel(void);

#endif /* PROTOCOL_DSM_H_ */
