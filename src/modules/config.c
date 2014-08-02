/*
 * This file is part of the superbitrf project.
 *
 * Copyright (C) 2013 Freek van Tienen <freek.v.tienen@gmail.com>
 * Copyright (C) 2014 Piotr Esden-Tempski <piotr@esden.net>
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

#include "config.h"
#include "../helper/dsm.h"
#include <libopencm3/stm32/flash.h>

struct Config usbrf_config;
char debug_msg[512];

void (*protocol_functions[][3])(void) = {
	{dsm_receiver_init, dsm_receiver_start, dsm_receiver_stop},
	{dsm_transmitter_init, dsm_transmitter_start, dsm_transmitter_stop},
	{dsm_mitm_init, dsm_mitm_start, dsm_mitm_stop},
};

/* We are assuming we are using the STM32F103TBU6.
 * Flash: 128 * 1kb pages
 * We want to store the config in the last flash sector.
 */
#define CONFIG_ADDR 0x0801FC00

/* Default configuration settings. */
const struct Config init_config = {
			.version				= 0x01,
			.protocol				= DSM_MITM,
			.protocol_start 			= true,
			.debug_enable 				= false,
			.debug_button				= true,
			.debug_cyrf6936				= false,
			.debug_dsm				= false,
			.debug_protocol				= true,
			.timer_scaler				= 1,
			.dsm_start_bind				= false,
			.dsm_max_channel			= DSM_MAX_CHANNEL,
			.dsm_bind_channel			= -1,
			.dsm_bind_mfg_id			= {0xDC, 0x72, 0x96, 0x4F},
			.dsm_protocol				= 0x01,
			.dsm_num_channels			= 6,
			.dsm_force_dsm2				= false,
			.dsm_max_missed_packets 		= 3,
			.dsm_bind_packets			= DSM_BIND_PACKETS,
			.dsm_mitm_both_data			= false,
			.dsm_mitm_has_uplink			= true,
};

void config_init(void) {
	struct Config loaded_config;

	config_load(&loaded_config);

	/* Check if the version stored in flash is the same as the one we have set
	   by default. Otherwise the config is very likely outdated and we will have to
	   discard it. */
	if (loaded_config.version == init_config.version) {
		memcpy(&usbrf_config, &loaded_config, sizeof(struct Config));
	} else {
		memcpy(&usbrf_config, &init_config, sizeof(init_config));
	}

}

void config_store(void) {

	uint16_t size = sizeof(struct Config);
	uint32_t addr = CONFIG_ADDR;
	uint8_t  *byte_config = (uint8_t *)&usbrf_config;
	uint16_t write_word;
	int i;

	/* Unlock flash. */
	flash_unlock();

	/* Erase the config storage page. */
	flash_erase_page(CONFIG_ADDR);

	/* Write config struct to flash. */
	write_word = 0xFFFF;
	for (i = 0; i < size; i++) {
		write_word = (write_word << 8) | (*(byte_config++));
		if ((i % 2) == 1) {
			flash_program_half_word(addr, write_word);
			addr += 2;
		}
	}

	if ((i % 2) == 1) {
		write_word = (write_word << 8) | 0xFF;
		flash_program_half_word(addr, write_word);
	}

	/* Write config CRC to flash. */

	/* Lock flash. */
	flash_lock();

	/* Check flash content for accuracy. */

}

/**
 * Load the config from flash.
 * This is definitely not the most efficient way of reading out the data. But
 * as we do it only once it probably does not matter much. (esden)
 */
void config_load(struct Config *config) {
	uint16_t size = sizeof(struct Config);
	uint16_t *flash_data = (uint16_t *)CONFIG_ADDR;
	uint8_t *byte_config = (uint8_t *)config;
	int i;

	for (i = 0; i < size; i++) {
		if ((i % 2) == 0) {
			*byte_config = (*flash_data) >> 8;
			byte_config++;
		} else {
			*byte_config = (*flash_data) & 0xFF;
			byte_config++;
			flash_data++;
		}
	}
}
