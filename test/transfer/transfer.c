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
static const u8 packet[16] = {
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
void on_receive(bool error) {
	int i, count;
	u8 packet_buf[16];
	u8 rx_status;

	LED_TOGGLE(1);

	// Check the rx status
	rx_status = cyrf_read_register(CYRF_RX_STATUS);
	if(rx_status & CYRF_BAD_CRC) {
		cyrf_start_recv();
		return;
	}

	// Copy packet to the packet buffer
	cyrf_recv(packet_buf);

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
void on_send(bool error) {
	LED_TOGGLE(POWER);
}

int main(void)
{
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
	cyrf_set_tx_cfg(CYRF_DATA_CODE_LENGTH | CYRF_DATA_MODE_SDR | CYRF_PA_4); // Enable 64 chip codes, SDR mode and amplifier +4dBm
	cyrf_set_rx_override(CYRF_FRC_RXDR | CYRF_DIS_RXCRC); // Force receive data rate and disable receive CRC checker
	cyrf_set_tx_override(CYRF_DIS_TXCRC); // Disable the transmit CRC
	cyrf_set_framing_cfg(CYRF_SOP_LEN | 0xA); // Set SOP CODE to 64 chips and SOP Correlator Threshold to 0xA

	// Set the channel
	cyrf_set_channel(0x61);

	// Set the timer or start receive
#ifdef RECEIVER
	cyrf_start_recv();
#else
	timer_dsm_set(10);
#endif

	/* Main loop */
	while (1) {
		cdcacm_run();
	}

	return 0;
}
