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

/* All times are in microseconds divided by 10 */
#define DSM_BIND_RECV_TIME			1000		/**< Time before timeout when receiving bind packets */
#define DSM_SYNC_RECV_TIME			2000		/**< Time before timeout when trying to sync */
#define DSM_SYNC_FRECV_TIME			10000		/**< Time before timeout when trying to sync first packet of DSMX (bigger then bind sending) */
#define DSM_RECV_TIME				    2200		/**< Time before timeout when trying to receive */
#define DSM_RECV_TIME_SHORT			800			/**< Time before timeout when trying to receive between two channels */
#define DSM_RECV_TIME_DATA			1000		/**< Time before timeout when waiting for data packet (MITM) */

#define DSM_BIND_SEND_TIME			1000		/**< Time between sending bind packets */
#define DSM_SEND_TIME				    2200		/**< Time between sending both Channel A and Channel B */
#define DSM_CHA_CHB_SEND_TIME		400			/**< Time between Channel A and Channel B send */

/* The maximum channekl number for DSM2 and DSMX */
#define DSM_MAX_CHANNEL				0x4F		/**< Maximum channel number used for DSM2 and DSMX */
#define DSM_BIND_PACKETS			300			/**< The amount of bind packets to send */

/* The different kind of protocol definitions DSM2 and DSMX with 1 and 2 packets of data */
enum dsm_protocol {
	DSM_DSM2_1			= 0x01,		/**< The original DSM2 protocol with 1 packet of data */
	DSM_DSM2_2			= 0x02,		/**< The original DSM2 protocol with 2 packets of data */
	DSM_DSMX_1			= 0xA2,		/**< The original DSMX protocol with 1 packet of data */
	DSM_DSMX_2			= 0xB2,		/**< The original DSMX protocol with 2 packets of data */
};
#define IS_DSM2(x)			(x == DSM_DSM2_1 || x == DSM_DSM2_2 || usbrf_config.dsm_force_dsm2)
#define IS_DSMX(x)			(!IS_DSM2(x))

#define CHECK_MFG_ID(protocol, packet, id) ((IS_DSM2(protocol) && packet[0] == (~id[2]&0xFF) && packet[1] == (~id[3]&0xFF)) || \
		(IS_DSMX(protocol) && packet[0] == id[2] && packet[1] == id[3]))

/* The different kind of resolutions the commands can be */
enum dsm_resolution {
	DSM_10_BIT_RESOLUTION			= 0x00,		/**< It has a 10 bit resolution */
	DSM_11_BIT_RESOLUTION			= 0x01,		/**< It has a 11 bit resolution */
};

/* External variables used in DSM2 and DSMX */
extern const uint8_t pn_codes[5][9][8];			/**< The pn_codes for the DSM2/DSMX protocol */
extern const uint8_t pn_bind[];					/**< The pn_code used during binding */
extern const uint8_t cyrf_config[][2];			/**< The CYRF DSM configuration during boot */
extern const uint8_t cyrf_bind_config[][2];		/**< The CYRF DSM binding configuration */
extern const uint8_t cyrf_transfer_config[][2];	/**< The CYRF DSM transfer configuration */

/* External functions */
uint16_t dsm_config_size(void);
uint16_t dsm_bind_config_size(void);
uint16_t dsm_transfer_config_size(void);
void dsm_generate_channels_dsmx(uint8_t mfg_id[], uint8_t *channels);
void dsm_set_channel(uint8_t channel, bool is_dsm2, uint8_t sop_col, uint8_t data_col, uint16_t crc_seed);
void dsm_radio_to_channels(uint8_t* data, uint8_t nb_channels, bool is_11bit, int16_t* channels)

#endif /* PROTOCOL_DSM_H_ */
