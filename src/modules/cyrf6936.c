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

#include <unistd.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/stm32/exti.h>

#include "cyrf6936.h"

/*The CYRF config */
static const u8 cyrf_config[][2] = {
		{CYRF_MODE_OVERRIDE, CYRF_RST},											// Reset the device
		{CYRF_CLK_EN, CYRF_RXF},												// Enable the clock
		{CYRF_AUTO_CAL_TIME, 0x3C},												// From manual, needed for initialization
		{CYRF_AUTO_CAL_OFFSET, 0x14},											// From manual, needed for initialization
		{CYRF_TX_OFFSET_LSB, 0x55},												// From manual, typical configuration
		{CYRF_TX_OFFSET_MSB, 0x05},												// From manual, typical configuration
		{CYRF_DATA64_THOLD, 0x0A},												// From manual, typical configuration
		{CYRF_EOP_CTRL, 0x02},													// Only enable EOP symbol count of 2
		{CYRF_TX_CTRL, CYRF_TXC_IRQEN | CYRF_TXE_IRQEN},						// Transmit interrupt on complete and error
		{CYRF_RX_CTRL, CYRF_RXC_IRQEN | CYRF_RXE_IRQEN},						// Receive interrupt on complete and error
		{CYRF_RX_ABORT, 0x0F},													// RX Abort
		{CYRF_ANALOG_CTRL, 0x01},												// RX Inverse
		{CYRF_XACT_CFG, CYRF_MODE_IDLE | CYRF_FRC_END},							// Force in idle mode
};

/* The CYRF receive and send callbacks */
cyrf_on_event _cyrf_recv_callback = NULL;
cyrf_on_event _cyrf_send_callback = NULL;

/* The pin for selecting the device */
#define CYRF_CS_HI() gpio_set(CYRF_DEV_SS_PORT, CYRF_DEV_SS_PIN)
#define CYRF_CS_LO() gpio_clear(CYRF_DEV_SS_PORT, CYRF_DEV_SS_PIN)

// TODO: Fix a nice delay
void Delay(u32 x);
void Delay(u32 x)
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
 * Initialize the CYRF6936
 */
void cyrf_init(void) {
	/* Initialize the clocks */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, CYRF_DEV_SPI_CLK); //SPI
	rcc_peripheral_enable_clock(&RCC_APB2ENR, CYRF_DEV_IRQ_CLK); //IRQ
	rcc_peripheral_enable_clock(&RCC_APB2ENR, CYRF_DEV_RST_CLK); //RST

	/* Initialize the GPIO */
	gpio_set_mode(CYRF_DEV_IRQ_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
			CYRF_DEV_IRQ_PIN); 													//IRQ
	gpio_set_mode(CYRF_DEV_RST_PORT, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_PUSHPULL, CYRF_DEV_RST_PIN); 						//RST
	gpio_set_mode(CYRF_DEV_SS_PORT, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_PUSHPULL, CYRF_DEV_SS_PIN); 						//SS
	gpio_set_mode(CYRF_DEV_SCK_PORT, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, CYRF_DEV_SCK_PIN); 					//SCK
	gpio_set_mode(CYRF_DEV_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
			CYRF_DEV_MISO_PIN); 												//MISO
	gpio_set_mode(CYRF_DEV_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, CYRF_DEV_MOSI_PIN); 				//MOSI

	/* Enable the IRQ */
	exti_select_source(CYRF_DEV_IRQ_EXTI, CYRF_DEV_IRQ_PORT);
	exti_set_trigger(CYRF_DEV_IRQ_EXTI, EXTI_TRIGGER_FALLING);
	exti_enable_request(CYRF_DEV_IRQ_EXTI);

	// Enable the IRQ NVIC
	nvic_enable_irq(CYRF_DEV_IRQ_NVIC);

	/* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
	spi_reset(CYRF_DEV_SPI);

	/* Set up SPI in Master mode with:
	 * Clock baud rate: 1/64 of peripheral clock frequency
	 * Clock polarity: Idle High
	 * Clock phase: Data valid on 2nd clock pulse
	 * Data frame format: 8-bit
	 * Frame format: MSB First
	 */
	spi_init_master(CYRF_DEV_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_64,
			SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_1,
			SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

	/* Set NSS management to software. */
	spi_enable_software_slave_management(CYRF_DEV_SPI);
	spi_set_nss_high(CYRF_DEV_SPI);

	/* Enable SPI1 periph. */
	spi_enable(CYRF_DEV_SPI);

	/* Reset the CYRF chip */
	gpio_set(CYRF_DEV_RST_PORT, CYRF_DEV_RST_PIN);
	Delay(100);
	gpio_clear(CYRF_DEV_RST_PORT, CYRF_DEV_RST_PIN);
	Delay(100);

	/* Set the config */
	cyrf_set_config(cyrf_config, sizeof(cyrf_config)/2);
}

/**
 * On interrupt request
 */
void CYRF_DEV_IRQ_ISR(void) {
	u8 tx_irq_status, rx_irq_status;

	// Read the transmit IRQ
	tx_irq_status = cyrf_read_register(CYRF_TX_IRQ_STATUS);
	if (((tx_irq_status & CYRF_TXC_IRQ) || (tx_irq_status & CYRF_TXE_IRQ))
			&& _cyrf_send_callback != NULL) {
		_cyrf_send_callback((tx_irq_status & CYRF_TXE_IRQ) > 0x0);
	}

	// Read the read IRQ
	rx_irq_status = cyrf_read_register(CYRF_RX_IRQ_STATUS);
	if (((rx_irq_status & CYRF_RXC_IRQ) || (rx_irq_status & CYRF_RXE_IRQ))
			&& _cyrf_recv_callback != NULL) {
		_cyrf_recv_callback((rx_irq_status & CYRF_RXE_IRQ) > 0x0);
	}

	exti_reset_request(CYRF_DEV_IRQ_EXTI);
}

/*
 * Register the receive callback
 * @param[in] callback The callback when it receives an interrupt for receive
 */
void cyrf_register_recv_callback(cyrf_on_event callback) {
	_cyrf_recv_callback = callback;
}

/*
 * Register the send callback
 * @param[in] callback The callback when it receives an interrupt for send
 */
void cyrf_register_send_callback(cyrf_on_event callback) {
	_cyrf_send_callback = callback;
}

/**
 * Write a byte to the register
 * @param[in] address The one byte address number of the register
 * @param[in] data The one byte data that needs to be written to the address
 */
void cyrf_write_register(const u8 address, const u8 data) {
	CYRF_CS_LO();
	spi_xfer(CYRF_DEV_SPI, CYRF_DIR | address);
	spi_xfer(CYRF_DEV_SPI, data);
	CYRF_CS_HI();
}

/**
 * Write a block to the register
 * @param[in] address The one byte address number of the register
 * @param[in] data The data that needs to be written to the address
 * @param[in] length The length in bytes of the data that needs to be written
 */
void cyrf_write_block(const u8 address, const u8 data[], const int length) {
	int i;
	CYRF_CS_LO();
	spi_xfer(CYRF_DEV_SPI, CYRF_DIR | address);

	for (i = 0; i < length; i++)
		spi_xfer(CYRF_DEV_SPI, data[i]);

	CYRF_CS_HI();
}

/**
 * Read a byte from the register
 * @param[in] The one byte address of the register
 * @return The one byte data of the register
 */
u8 cyrf_read_register(const u8 address) {
	u8 data;
	CYRF_CS_LO();
	spi_xfer(CYRF_DEV_SPI, address);
	data = spi_xfer(CYRF_DEV_SPI, 0);
	CYRF_CS_HI();
	return data;
}

/**
 * Read a block from the register
 * @param[in] address The one byte address of the register
 * @param[out] data The data that was received from the register
 * @param[in] length The length in bytes what needs to be read
 */
void cyrf_read_block(const u8 address, u8 data[], const int length) {
	int i;
	CYRF_CS_LO();
	spi_xfer(CYRF_DEV_SPI, address);

	for (i = 0; i < length; i++)
		data[i] = spi_xfer(CYRF_DEV_SPI, 0);

	CYRF_CS_HI();
}

/**
 * Read the MFG id from the register
 * @param[out] The MFG id from the device
 */
void cyrf_get_mfg_id(u8 *mfg_id) {
	cyrf_read_block(CYRF_MFG_ID, mfg_id, 6);
}

/**
 * Get the RSSI (signal strength) of the last received packet
 * @return The RSSI of the last received packet
 */
u8 cyrf_get_rssi(void) {
	return cyrf_read_register(CYRF_RSSI) & 0x0F;
}

/**
 * Get the RX status
 * @return The RX status register
 */
u8 cyrf_get_rx_status(void) {
	return cyrf_read_register(CYRF_RX_STATUS);
}

/**
 * Set multiple (config) values at once
 * @param[in] config An array of len by 2, consisting the register address and the values
 * @param[in] length The length of the config array
 */
void cyrf_set_config(const u8 config[][2], const u8 length) {
	int i;
	for (i = 0; i < length; i++)
		cyrf_write_register(config[i][0], config[i][1]);
}

/**
 * Set the RF channel
 * @param[in] chan The channel needs to be set
 */
void cyrf_set_channel(const u8 chan) {
	cyrf_write_register(CYRF_CHANNEL, chan);
}

/**
 * Set the power
 * @param[in] power The power that needs to be set
 */
void cyrf_set_power(const u8 power) {
	u8 tx_cfg = cyrf_read_register(CYRF_TX_CFG) & (0xFF - CYRF_PA_4);
	cyrf_write_register(CYRF_TX_CFG, tx_cfg | power);
}

/**
 * Set the mode
 * @param[in] mode The mode that the chip needs to be set to
 * @param[in] force Force the mode switch
 */
void cyrf_set_mode(const u8 mode, const bool force) {
	if (force)
		cyrf_write_register(CYRF_XACT_CFG, mode | CYRF_FRC_END);
	else
		cyrf_write_register(CYRF_XACT_CFG, mode);
}

/**
 * Set the CRC seed
 * @param[in] crc The 16-bit CRC seed
 */
void cyrf_set_crc_seed(const u16 crc) {
	cyrf_write_register(CYRF_CRC_SEED_LSB, crc & 0xff);
	cyrf_write_register(CYRF_CRC_SEED_MSB, crc >> 8);
}

/**
 * Set the SOP code
 * @param[in] sopcode The 8 bytes SOP code
 */
void cyrf_set_sop_code(const u8 *sopcode) {
	cyrf_write_block(CYRF_SOP_CODE, sopcode, 8);
}

/**
 * Set the data code
 * @param[in] datacode The 16 bytes data code
 */
void cyrf_set_data_code(const u8 *datacode) {
	cyrf_write_block(CYRF_DATA_CODE, datacode, 16);
}

/**
 * Set the preamble
 * @param[in] preamble The 3 bytes preamble
 */
void cyrf_set_preamble(const u8 *preamble) {
	cyrf_write_block(CYRF_PREAMBLE, preamble, 3);
}

/**
 * Set the Framing config
 * @param[in] config The framing config register
 */
void cyrf_set_framing_cfg(const u8 config) {
	cyrf_write_register(CYRF_FRAMING_CFG, config);
}

/**
 * Set the RX config
 * @param[in] config The RX config register
 */
void cyrf_set_rx_cfg(const u8 config) {
	cyrf_write_register(CYRF_RX_CFG, config);
}

/**
 * Set the TX config
 * @param[in] config The TX config register
 */
void cyrf_set_tx_cfg(const u8 config) {
	cyrf_write_register(CYRF_TX_CFG, config);
}

/*
 * Set the RX override
 * @param[in] override The RX override register
 */
void cyrf_set_rx_override(const u8 override) {
	cyrf_write_register(CYRF_RX_OVERRIDE, override);
}

/*
 * Set the TX override
 * @param[in] override The TX override register
 */
void cyrf_set_tx_override(const u8 override) {
	cyrf_write_register(CYRF_TX_OVERRIDE, override);
}

/*
 * Send a data packet with length
 * @param[in] data The data of the packet
 * @param[in] length The length of the data
 */
void cyrf_send_len(const u8 *data, const u8 length) {
	cyrf_write_register(CYRF_TX_LENGTH, length);
	cyrf_write_register(CYRF_TX_CTRL, CYRF_TX_CLR);
	cyrf_write_block(CYRF_TX_BUFFER, data, length);
	cyrf_write_register(CYRF_TX_CTRL, CYRF_TX_GO | CYRF_TXC_IRQEN | CYRF_TXE_IRQEN);
}

/**
 * Send a 16 byte data packet
 * @param[in] data The 16 byte data of the packet
 */
void cyrf_send(const u8 *data) {
	cyrf_send_len(data, 16);
}

/**
 * Start receiving
 */
void cyrf_start_recv(void) {
	cyrf_set_mode(CYRF_MODE_SYNTH_RX, 0);
	cyrf_write_register(CYRF_RX_CTRL, CYRF_RX_GO | CYRF_RXC_IRQEN | CYRF_RXE_IRQEN); // Start receiving and set the IRQ
}

/**
 * Receive the packet from the RX buffer with length
 * @param[out] data The data from the RX buffer
 * @param[in] length The length of data that is received from the RX buffer
 */
void cyrf_recv_len(u8 *data, const u8 length) {
	cyrf_read_block(CYRF_RX_BUFFER, data, length);
}

/**
 * Receive a 16 byte packet from the RX buffer
 * @param[out] data The 16 byte data from the RX buffer
 */
void cyrf_recv(u8 *data) {
	cyrf_recv_len(data, 16);
}
