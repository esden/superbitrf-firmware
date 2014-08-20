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

#include "../modules/config.h"
#include "../modules/led.h"
#include "../modules/button.h"
#include "../modules/timer.h"
#include "../modules/cyrf6936.h"

#include "dsm_mitm.h"

struct DsmMitm dsm_mitm;

void dsm_mitm_start_bind(void);
void dsm_mitm_start_transfer(void);
void dsm_mitm_timer_cb(void);
void dsm_mitm_receive_cb(bool error);
void dsm_mitm_send_cb(bool error);
void dsm_mitm_cdcacm_cb(char *data, int size);

void dsm_mitm_set_rf_channel(uint8_t chan);
void dsm_mitm_set_channel(uint8_t chan);
void dsm_mitm_set_next_channel(void);

void dsm_mitm_create_packet(uint8_t data[], uint8_t length);

void Delay2(uint32_t x);
void Delay2(uint32_t x)
{
    (void)x;
    __asm ("mov r1, #24;"
         "mul r0, r0, r1;"
         "b _delaycmp;"
         "_delayloop:"
         "subs r0, r0, #1;"
         "_delaycmp:;"
         "cmp r0, #0;"
         " bne _delayloop;");
}

/**
 * DSM MITM protocol initialization
 */
void dsm_mitm_init(void) {
	uint8_t mfg_id[6];
	DEBUG(protocol, "DSM MITM initializing");
	dsm_mitm.status = DSM_MITM_STOP;

	// Configure the CYRF
	cyrf_set_config_len(cyrf_config, dsm_config_size());

	// Read the CYRF MFG and copy from the config
	cyrf_get_mfg_id(mfg_id);
	memcpy(dsm_mitm.mfg_id, usbrf_config.dsm_bind_mfg_id, 4);

	// Stop the timer
	timer_dsm_stop();

	// Setup the buffer
	convert_init(&dsm_mitm.tx_buffer);

	// Set the callbacks
	timer_dsm_register_callback(dsm_mitm_timer_cb);
	cyrf_register_recv_callback(dsm_mitm_receive_cb);
	cyrf_register_send_callback(dsm_mitm_send_cb);
	button_bind_register_callback(dsm_mitm_start_bind);
	cdcacm_register_receive_callback(dsm_mitm_cdcacm_cb);

	DEBUG(protocol, "DSM MITM initialized 0x%02X 0x%02X 0x%02X 0x%02X", mfg_id[0], mfg_id[1], mfg_id[2], mfg_id[3]);
}

/**
 * DSM MITM protocol start
 */
void dsm_mitm_start(void) {
	DEBUG(protocol, "DSM MITM starting");

	// Check if need to start with binding procedure
	if(usbrf_config.dsm_start_bind)
		dsm_mitm_start_bind();
	else
		dsm_mitm_start_transfer();
}

/**
 * DSM MITM protocol stop
 */
void dsm_mitm_stop(void) {
	// Stop the timer
	timer_dsm_stop();
	dsm_mitm.status = DSM_MITM_STOP;
}


/**
 * DSM MITM start bind
 */
void dsm_mitm_start_bind(void) {
	uint8_t data_code[16];
	DEBUG(protocol, "DSM MITM start bind");

	dsm_mitm.status = DSM_MITM_BIND;
	dsm_mitm.missed_packets = 0;
	dsm_mitm.tx_packet_count = 0;
	dsm_mitm.rx_packet_count = 0;

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
		dsm_mitm_set_rf_channel(usbrf_config.dsm_bind_channel);
	else
		dsm_mitm_set_rf_channel(1);

	// Start receiving
	cyrf_start_recv();

	// Enable the timer
	timer_dsm_set(DSM_BIND_RECV_TIME);
}

/**
 * DSM MITM start transfer
 */
void dsm_mitm_start_transfer(void) {
	DEBUG(protocol, "DSM MITM start transfer");

	dsm_mitm.status = DSM_MITM_SYNC_A;
	dsm_mitm.rf_channel_idx = 0;
	dsm_mitm.missed_packets = 0;
	dsm_mitm.tx_packet_count = 0;
	dsm_mitm.rx_packet_count = 0;

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

	dsm_mitm.num_channels = usbrf_config.dsm_num_channels;
	dsm_mitm.protocol = usbrf_config.dsm_protocol;
	dsm_mitm.resolution = (dsm_mitm.protocol & 0x10)>>4;

	// Calculate the CRC seed, SOP column and Data column
	dsm_mitm.crc_seed = ~((dsm_mitm.mfg_id[0] << 8) + dsm_mitm.mfg_id[1]);
	dsm_mitm.sop_col = (dsm_mitm.mfg_id[0] + dsm_mitm.mfg_id[1] + dsm_mitm.mfg_id[2] + 2) & 0x07;
	dsm_mitm.data_col = 7 - dsm_mitm.sop_col;

	DEBUG(protocol, "DSM MITM bound(MFG_ID: {0x%02X, 0x%02X, 0x%02X, 0x%02X}, num_channels: 0x%02X, protocol: 0x%02X, resolution: 0x%02X, sop_col: 0x%02X, data_col 0x%02X)",
				dsm_mitm.mfg_id[0], dsm_mitm.mfg_id[1], dsm_mitm.mfg_id[2], dsm_mitm.mfg_id[3],
				dsm_mitm.num_channels, dsm_mitm.protocol, dsm_mitm.resolution, dsm_mitm.sop_col, dsm_mitm.data_col);

	// When DSMX generate channels and set channel
	if(IS_DSMX(dsm_mitm.protocol)) {
		dsm_generate_channels_dsmx(dsm_mitm.mfg_id, dsm_mitm.rf_channels);
		dsm_mitm.rf_channel_idx = 22;
		dsm_mitm_set_next_channel();
	} else
		dsm_mitm_set_channel(0);

	// Start receiving
	cyrf_start_recv();

	// Enable the timer
	if(IS_DSM2(dsm_mitm.protocol))
		timer_dsm_set(DSM_SYNC_RECV_TIME);
	else
		timer_dsm_set(DSM_SYNC_FRECV_TIME); // Because we know for sure where DSMX starts we can wait the full bind
}

/**
 * DSM MITM timer callback
 */
void dsm_mitm_timer_cb(void) {
	// Abort the receive
	cyrf_set_mode(CYRF_MODE_SYNTH_RX, true);
	cyrf_write_register(CYRF_RX_ABORT, 0x00);

	// Check the receiver status
	switch (dsm_mitm.status) {
	case DSM_MITM_BIND:
		// Set the next bind channel if bind channel not set
		if(usbrf_config.dsm_bind_channel < 0)
			dsm_mitm_set_rf_channel((dsm_mitm.rf_channel + 2) % usbrf_config.dsm_max_channel);

		// Start receiving
		cyrf_start_recv();

		// Set the new timeout
		timer_dsm_set(DSM_BIND_RECV_TIME);
		break;
	case DSM_MITM_SYNC_A:
	case DSM_MITM_SYNC_B:
		// When we are in DSM2 mode we need to scan all channels
		if(IS_DSM2(dsm_mitm.protocol)) {
			// Set the next channel
			dsm_mitm_set_channel((dsm_mitm.rf_channel + 2) % usbrf_config.dsm_max_channel);
		} else {
			// Just set the next channel we know
			dsm_mitm_set_next_channel();
		}

		cyrf_start_recv();

		// Set the new timeout
		timer_dsm_set(DSM_SYNC_RECV_TIME);
		break;
	case DSM_MITM_RECV:
		// Check if we missed too much packets
		DEBUG(protocol, "Lost a packet at channel 0x%02X", dsm_mitm.rf_channel);
		dsm_mitm.missed_packets++;

		// Set RX led off
#ifdef LED_RX
		LED_OFF(LED_RX);
#endif

		if(dsm_mitm.missed_packets < usbrf_config.dsm_max_missed_packets) {

			// We still have to go to the next channel
			dsm_mitm_set_next_channel();
			cyrf_start_recv();

			// Start the timer
			timer_dsm_set(DSM_RECV_TIME);
		} else {
			DEBUG(protocol, "Lost sync after 0x%02X missed packets", dsm_mitm.missed_packets);
			// We are out of sync and start syncing again
			dsm_mitm.status = DSM_MITM_SYNC_A;

			// Set the new timeout
			timer_dsm_set(DSM_SYNC_RECV_TIME);
		}
		break;
	default:
		break;
	}
}

/**
 * DSM MITM receive callback
 */
void dsm_mitm_receive_cb(bool error) {
	uint8_t packet_length, packet[16], rx_status;
	uint16_t bind_sum;
	int i;

	// Get the receive count, rx_status and the packet
	packet_length = cyrf_read_register(CYRF_RX_COUNT);
	rx_status = cyrf_get_rx_status();
	cyrf_recv_len(packet, packet_length);

	// Abort the receive
	cyrf_write_register(CYRF_XACT_CFG, CYRF_MODE_SYNTH_RX | CYRF_FRC_END);
	cyrf_write_register(CYRF_RX_ABORT, 0x00); //TODO: CYRF_RX_ABORT_EN

	// Check if length bigger then two
	if(packet_length < 2)
		return;

	// Send a debug message that we have received a packet
	//DEBUG(protocol, "DSM MITM receive (channel: 0x%02X, packet_length: 0x%02X)", dsm_mitm.rf_channel, packet_length);

	// Check the receiver status
	switch (dsm_mitm.status) {
	case DSM_MITM_BIND:
		// Check if there is an error and the MFG id is exactly the same twice
		if(packet[0] != packet[4] || packet[1] != packet[5]
				|| packet[2] != packet[6] || packet[3] != packet[7]) {
			// Set the new timeout
			timer_dsm_set(DSM_BIND_RECV_TIME);
			break;
		}

		// Calculate the first sum
		bind_sum = 384 - 0x10;
		for(i = 0; i < 8; i++)
			bind_sum += packet[i];

		// Check the first sum
		if(packet[8] != bind_sum >> 8 || packet[9] != (bind_sum & 0xFF))
			break;

		// Calculate second sum
		for(i = 8; i < 14; i++)
			bind_sum += packet[i];

		// Check the second sum
		if(packet[14] != bind_sum >> 8 || packet[15] != (bind_sum & 0xFF))
			break;

		// Stop the timer
		timer_dsm_stop();

		// Update the mfg id, number of channels and protocol
		dsm_mitm.mfg_id[0] = ~packet[0];
		dsm_mitm.mfg_id[1] = ~packet[1];
		dsm_mitm.mfg_id[2] = ~packet[2];
		dsm_mitm.mfg_id[3] = ~packet[3];
		memcpy(usbrf_config.dsm_bind_mfg_id, dsm_mitm.mfg_id, 4);
		usbrf_config.dsm_num_channels = packet[11];
		usbrf_config.dsm_protocol = packet[12];
		config_store();

		DEBUG(protocol, "Binded with values : mfg_id[0-3]=0x%02X; 0x%02X; 0x%02X; 0x%02X, num_channel=0x%02X; protocol=0x%02X", dsm_mitm.mfg_id[0], dsm_mitm.mfg_id[1], dsm_mitm.mfg_id[2], dsm_mitm.mfg_id[3], usbrf_config.dsm_num_channels, usbrf_config.dsm_protocol);

		// Start receiver
		dsm_mitm_start_transfer();
		break;
	case DSM_MITM_SYNC_A:
		// If other error than bad CRC or MFG id doesn't match reject the packet
		if(error && !(rx_status & CYRF_BAD_CRC))
			break;
		if(!CHECK_MFG_ID_BOTH(dsm_mitm.protocol, packet, dsm_mitm.mfg_id))
			break;

		// Invert the CRC when received bad CRC
		if (error && (rx_status & CYRF_BAD_CRC))
			dsm_mitm.crc_seed = ~dsm_mitm.crc_seed;

		DEBUG(protocol, "Synchronized channel A 0x%02X", dsm_mitm.rf_channel);

		// Stop the timer
		timer_dsm_stop();

		// Check whether it is DSM2 or DSMX
		if(IS_DSM2(dsm_mitm.protocol)) {
			dsm_mitm.rf_channels[0] = dsm_mitm.rf_channel;
			dsm_mitm.rf_channels[1] = dsm_mitm.rf_channel;

			dsm_mitm.status = DSM_MITM_SYNC_B;
		} else {
			// When it is DSMX we can stop because we know all the channels
			dsm_mitm.status = DSM_MITM_RECV;
			dsm_mitm.missed_packets = 0;
		}
		// Set the next channel and start receiving
		dsm_mitm_set_next_channel();
		cyrf_start_recv();

		// Start the timer
		timer_dsm_set(DSM_RECV_TIME);
		break;
	case DSM_MITM_SYNC_B:
		// If other error than bad CRC or MFG id doesn't match reject the packet
		if(error && !(rx_status & CYRF_BAD_CRC))
			break;
		if(!CHECK_MFG_ID_BOTH(dsm_mitm.protocol, packet, dsm_mitm.mfg_id))
			break;

		// Invert the CRC when received bad CRC
		if (error && (rx_status & CYRF_BAD_CRC))
			dsm_mitm.crc_seed = ~dsm_mitm.crc_seed;

		// Set the appropriate channel
		if(dsm_mitm.crc_seed != ((dsm_mitm.mfg_id[0] << 8) + dsm_mitm.mfg_id[1]))
			dsm_mitm.rf_channels[0] = dsm_mitm.rf_channel;
		else
			dsm_mitm.rf_channels[1] = dsm_mitm.rf_channel;

		// Check if we have both channels
		if(dsm_mitm.rf_channels[0] != dsm_mitm.rf_channels[1]) {
			DEBUG(protocol, "Synchronized channel B 0x%02X", dsm_mitm.rf_channel);

			// Stop the timer
			timer_dsm_stop();

			// Set the next channel and start receiving
			dsm_mitm.status = DSM_MITM_RECV;
			dsm_mitm.missed_packets = 0;
			dsm_mitm_set_next_channel();
			cyrf_start_recv();

			// Start the timer
			timer_dsm_set(DSM_RECV_TIME);
		}
		break;
	case DSM_MITM_RECV:
		// If other error than bad CRC or MFG id doesn't match reject the packet
		if(error && !(rx_status & CYRF_BAD_CRC))
			break;
		if(!CHECK_MFG_ID_BOTH(dsm_mitm.protocol, packet, dsm_mitm.mfg_id))
			break;

		// Invert the CRC when received bad CRC
		if (error && (rx_status & CYRF_BAD_CRC))
			dsm_mitm.crc_seed = ~dsm_mitm.crc_seed;

		// Stop the timer
		timer_dsm_stop();
		dsm_mitm.rx_packet_count++;
		dsm_mitm.missed_packets = 0;

		// Set RX led on
#ifdef LED_RX
		LED_ON(LED_RX);
#endif

		// Check if we got a data packet
		if(CHECK_MFG_ID_DATA(dsm_mitm.protocol, packet, dsm_mitm.mfg_id)) {
			DEBUG(protocol, "Receive data channel[0x%02X]: 0x%02X (timing %s: %u)", dsm_mitm.rf_channel_idx, dsm_mitm.rf_channel,
								dsm_mitm.crc_seed == ((dsm_mitm.mfg_id[0] << 8) + dsm_mitm.mfg_id[1])? "short":"long", timer_dsm_get_time());

			// Check if we need to send a packet
			if(usbrf_config.dsm_mitm_has_uplink) {
				// Only create packet without packet loss
				//if(packet[1] == ((dsm_mitm.mfg_id[3]+1+dsm_mitm.packet_loss_bit)&0xFF) || packet[1] == ((~dsm_mitm.mfg_id[3]+1+dsm_mitm.packet_loss_bit)&0xFF)) {
					dsm_mitm.packet_loss_bit = !dsm_mitm.packet_loss_bit;
					uint8_t tx_data[14];
					uint8_t tx_size = convert_extract(&dsm_mitm.tx_buffer, tx_data, 14);
					dsm_mitm_create_packet(tx_data, tx_size);
				//}

				// Send the packet with a timeout, need to fix the sleep
				Delay2(200);
				cyrf_send_len(dsm_mitm.tx_packet, dsm_mitm.tx_packet_length);
			} else {
				// Start receiving on next channel
				dsm_mitm_set_next_channel();
				cyrf_start_recv();

				// Start the timer (short or long)
				if(dsm_mitm.crc_seed == ((dsm_mitm.mfg_id[0] << 8) + dsm_mitm.mfg_id[1]))
					timer_dsm_set(DSM_RECV_TIME_SHORT);
				else
					timer_dsm_set(DSM_RECV_TIME);
			}

			// Output the data received
			cdcacm_send((char*)&packet[2], packet_length-2);
		} else {
			// Convert the channels
			static int16_t channels[14];
			convert_radio_to_channels(&packet[2], dsm_mitm.num_channels, dsm_mitm.resolution, channels);

			//DEBUG(protocol, "Receive commands channel[0x%02X]: 0x%02X (timing %s: %u)", dsm_mitm.rf_channel_idx, dsm_mitm.rf_channel,
				//	dsm_mitm.crc_seed == ((dsm_mitm.mfg_id[0] << 8) + dsm_mitm.mfg_id[1])? "short":"long", timer_dsm_get_time());

			// Go to the next channel if needed
			if(usbrf_config.dsm_mitm_both_data || dsm_mitm.crc_seed != ((dsm_mitm.mfg_id[0] << 8) + dsm_mitm.mfg_id[1])) {
				dsm_mitm_set_next_channel();

				// Start the timer (short or long)
				if(dsm_mitm.crc_seed == ((dsm_mitm.mfg_id[0] << 8) + dsm_mitm.mfg_id[1]))
					timer_dsm_set(DSM_RECV_TIME_SHORT);
				else
					timer_dsm_set(DSM_RECV_TIME);
			} else {
				// Start the data timer
				timer_dsm_set(DSM_RECV_TIME_DATA);
			}

			// Start receiving
			cyrf_start_recv();
		}
		break;
	default:
		break;
	}
}

/**
 * DSM MITM send callback
 */
void dsm_mitm_send_cb(bool error) {
	(void) error;
	dsm_mitm.tx_packet_count++;

	// Set TX led on
#ifdef LED_TX
	LED_ON(LED_TX);
#endif

	// Start receiving on next channel
	dsm_mitm_set_next_channel();
	cyrf_start_recv();

	// Start the timer (short or long)
	if(dsm_mitm.crc_seed == ((dsm_mitm.mfg_id[0] << 8) + dsm_mitm.mfg_id[1]))
		timer_dsm_set(DSM_RECV_TIME_SHORT-timer_dsm_get_time());
	else
		timer_dsm_set(DSM_RECV_TIME-timer_dsm_get_time());
}

/**
 * DSM MITM CDCACM receive callback
 */
void dsm_mitm_cdcacm_cb(char *data, int size) {
	convert_insert(&dsm_mitm.tx_buffer, (uint8_t*)data, size);
}

/**
 * Change DSM MITM RF channel
 * @param[in] chan The channel that need to be switched to
 */
void dsm_mitm_set_rf_channel(uint8_t chan) {
	dsm_mitm.rf_channel = chan;
	cyrf_set_channel(chan);
}

/**
 * Change DSM MITM RF channel and also set SOP, CRC and DATA code
 * @param[in] chan The channel that need to be switched to
 */
void dsm_mitm_set_channel(uint8_t chan) {
	dsm_mitm.crc_seed		= ~dsm_mitm.crc_seed;
	dsm_mitm.rf_channel 	= chan;
	dsm_set_channel(dsm_mitm.rf_channel, IS_DSM2(dsm_mitm.protocol),
			dsm_mitm.sop_col, dsm_mitm.data_col, dsm_mitm.crc_seed);
}

/**
 * Change DSM MITM RF channel to the next channel and also set SOP, CRC and DATA code
 */
void dsm_mitm_set_next_channel(void) {
	dsm_mitm.rf_channel_idx = IS_DSM2(dsm_mitm.protocol)? (dsm_mitm.rf_channel_idx+1) % 2 : (dsm_mitm.rf_channel_idx+1) % 23;
	dsm_mitm.crc_seed		= ~dsm_mitm.crc_seed;
	dsm_mitm.rf_channel 	= dsm_mitm.rf_channels[dsm_mitm.rf_channel_idx];
	dsm_set_channel(dsm_mitm.rf_channel, IS_DSM2(dsm_mitm.protocol),
			dsm_mitm.sop_col, dsm_mitm.data_col, dsm_mitm.crc_seed);
}

/**
 * Create DSM MITM data packet
 */
void dsm_mitm_create_packet(uint8_t data[], uint8_t length) {
	int i;
	if(IS_DSM2(dsm_mitm.protocol)) {
		dsm_mitm.tx_packet[0] = ~dsm_mitm.mfg_id[2];
		dsm_mitm.tx_packet[1] = (~dsm_mitm.mfg_id[3]+1+dsm_mitm.packet_loss_bit)&0xFF;
	} else {
		dsm_mitm.tx_packet[0] = dsm_mitm.mfg_id[2];
		dsm_mitm.tx_packet[1] = (dsm_mitm.mfg_id[3]+1+dsm_mitm.packet_loss_bit)&0xFF;
	}

	// Copy the commands
	for(i = 0; i < length; i++)
		dsm_mitm.tx_packet[i+2] = data[i];

	// Set the length
	dsm_mitm.tx_packet_length = length+2;
}
