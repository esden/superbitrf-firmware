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

#include <stdio.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

/* Load the modules */
#include "../../src/modules/led.h"
#include "../../src/modules/timer.h"
#include "../../src/modules/cdcacm.h"
#include "../../src/modules/cyrf6936.h"

/* The packet that it sended */
static const uint8_t packet[16] = {
			0x00,
			0x01,
			0x02,
			0x03,
			0x04,
			0x05,
			0x06,
			0x07,
			0x08,
			0x09,
			0x0A,
			0x0B,
			0x0C,
			0x0D,
			0x0E,
			0x0F
	};

void on_timer(void);
void on_receive(bool error);
void on_send(bool error);

/* When the DSM timer ends */
void on_timer(void) {
	// Sen the packet
	cyrf_send(packet);
	LED_TOGGLE(2);

	// Set timeout for next send
	timer_dsm_set(10000);
}

/* When the cyrf chip receives a packet */
void on_receive(bool error __attribute__((unused))) {
	int i, count;
	uint8_t packet_buf[16];
	char cdc_msg[512];
	uint8_t rx_status;

	LED_TOGGLE(1);

	// Check the rx status
	rx_status = cyrf_read_register(CYRF_RX_STATUS);
	if(rx_status & CYRF_BAD_CRC) {
		cyrf_start_recv();
		return;
	}

	// Copy packet to the packet buffer
	cyrf_recv(packet_buf);

	sprintf(cdc_msg, "Packet received: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n",
				packet_buf[0], packet_buf[1], packet_buf[2], packet_buf[3], packet_buf[4], packet_buf[5],
				packet_buf[6], packet_buf[7], packet_buf[8], packet_buf[9], packet_buf[10], packet_buf[11],
				packet_buf[12], packet_buf[13], packet_buf[14], packet_buf[15]);
	cdcacm_send(cdc_msg, strlen(cdc_msg));

	// Compare with packet
	count = 0;
	for(i =0; i<16; i++) {
		if(packet[i] == packet_buf[i])
			count++;
	}
	if(count >= 16)
		LED_TOGGLE(2);

	// Start receiving for the next packet
	cyrf_start_recv();
}

/* When the cyrf chip successfully sended the packet */
void on_send(bool error __attribute__((unused))) {
	LED_TOGGLE(3);
}

int main(void)
{
	uint8_t sop_code[] = {
		0x03, 0xE7, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8
	};
	uint8_t data_code[] = {
		0x88, 0x17, 0x13, 0x3B, 0x2D, 0xBF, 0x06, 0xD6
	};
	rcc_clock_setup_in_hse_12mhz_out_72mhz();

	// Initialize the modules
	led_init();
	timer_init();
	cdcacm_init();
	cyrf_init();

	// Register callbacks
	timer_dsm_register_callback(on_timer);
	cyrf_register_recv_callback(on_receive);
	cyrf_register_send_callback(on_send);

	// Config the cyrf RX mode, TX mode and framing
	cyrf_set_rx_cfg(CYRF_LNA | CYRF_FAST_TURN_EN); // Enable low noise amplifier and fast turn
	/*cyrf_set_tx_cfg(CYRF_DATA_CODE_LENGTH | CYRF_DATA_MODE_SDR | CYRF_PA_4); // Enable 64 chip codes, SDR mode and amplifier +4dBm
	cyrf_set_rx_override(CYRF_FRC_RXDR | CYRF_DIS_RXCRC); // Force receive data rate and disable receive CRC checker
	cyrf_set_tx_override(CYRF_DIS_TXCRC); // Disable the transmit CRC
	cyrf_set_framing_cfg(CYRF_SOP_LEN | 0xA); // Set SOP CODE to 64 chips and SOP Correlator Threshold to 0xA */
	cyrf_set_tx_cfg(CYRF_DATA_MODE_8DR | CYRF_PA_4); // Enable 32 chip codes, 8DR mode and amplifier +4dBm
	cyrf_set_rx_override(0x0); // Reset the rx override
	cyrf_set_tx_override(0x0); // Reset the tx override
	cyrf_set_framing_cfg(CYRF_SOP_EN | CYRF_SOP_LEN | CYRF_LEN_EN | 0xE); // Set SOP CODE enable, SOP CODE to 64 chips, Packet length enable, and SOP Correlator Threshold to 0xE

	// Set the channel
	cyrf_set_channel(0x61);

	// Set some other stuff
	cyrf_set_crc_seed(0x1A34);
	cyrf_set_sop_code(sop_code);
	cyrf_set_data_code(data_code);

	// Set the timer or start receive
#ifdef RECEIVER
	cyrf_start_recv();
#else
	timer_dsm_set(10);
#endif

	/* Main loop */
	while (1) {
	}

	return 0;
}
