/*
 * This file is part of the superbitrf project.
 *
 * Copyright (C) 2013-2015 Freek van Tienen <freek.v.tienen@gmail.com>
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
#include "console.h"
#include <stdio.h>
#include <string.h>
#include <libopencm3/stm32/flash.h>

/* console commands */
static void config_cmd_version(char *cmdLine);
static void config_cmd_load(char *cmdLine);
static void config_cmd_save(char *cmdLine);
static void config_cmd_set(char *cmdLine);
static void config_cmd_list(char *cmdLine);
static void config_cmd_reset(char *cmdLine);

/* We are assuming we are using the STM32F103TBU6.
 * Flash: 128 * 1kb pages
 * We want to store the config in the last flash sector.
 */
#define CONFIG_ADDR 0x0801FC00

/* Default configuration settings.
 * The version allways needs to be at the first place
 */
const struct ConfigItem init_config[CONFIG_ITEMS] = {
	{"VERSION", "%.1f", 1.0},
	{"DEBUG", "%.0f", 0}
};
struct ConfigItem usbrf_config[CONFIG_ITEMS];

/**
 * Initializes the configuration
 * When the version of the 'Default configuration' is different the one from flash gets updated.
 * Else the one from flash gets loaded.
 */
void config_init(void) {
	struct ConfigItem flash_config[CONFIG_ITEMS];
	config_load(flash_config);

	/* Check if the version stored in flash is the same as the one we have set
	   by default. Otherwise the config is very likely outdated and we will have to
	   discard it. */
	if (flash_config[0].value == init_config[0].value) {
		memcpy(usbrf_config, flash_config, sizeof(struct ConfigItem) * CONFIG_ITEMS);
	} else {
		memcpy(usbrf_config, init_config, sizeof(init_config));
		config_store();
	}

	/* Add some commands to the console */
	console_cmd_add("version", "", config_cmd_version);
	console_cmd_add("load", "", config_cmd_load);
	console_cmd_add("save", "", config_cmd_save);
	console_cmd_add("list", "", config_cmd_list);
	console_cmd_add("set", "[name] [value]", config_cmd_set);
	console_cmd_add("reset", "", config_cmd_reset);
}

/**
 * Stores the current config in flash
 */
void config_store(void) {
	uint16_t size = sizeof(struct ConfigItem) * CONFIG_ITEMS;
	uint32_t addr = CONFIG_ADDR;
	uint8_t  *byte_config = (uint8_t *)usbrf_config;
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
void config_load(struct ConfigItem config[]) {
	uint16_t size = sizeof(struct ConfigItem) * CONFIG_ITEMS;
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

/**
 * Show the current version and information
 */
static void config_cmd_version(char *cmdLine __attribute((unused))) {
	console_print("\r\nCurrent version: %.1f", usbrf_config[0].value);
	console_print("\r\nMade by Freek van Tienen and Piotr Esden-Tempski");
	console_print("\r\nLGPL V3");
}

/**
 * Load the config from flash
 */
static void config_cmd_load(char *cmdLine __attribute((unused))) {
	struct ConfigItem flash_config[CONFIG_ITEMS];
	config_load(flash_config);

	/* Check if the version is the same, else show error */
	if (flash_config[0].value == init_config[0].value) {
		memcpy(&usbrf_config, &flash_config, sizeof(struct ConfigItem) * CONFIG_ITEMS);
		console_print("\r\nSuccessfully loaded config from the memory!");
	} else {
		console_print("\r\nThere is no loadable config found.");
	}
}

/**
 * Save the config to flash
 */
static void config_cmd_save(char *cmdLine __attribute((unused))) {
	config_store();
	console_print("\r\nSuccessfully saved config to the memory!");
}


/**
 * Set a value
 */
static void config_cmd_set(char *cmdLine) {
	int i;
	char name[16], buf[128];
	float value;

	if (sscanf(cmdLine, "%16s %f", name, &value) != 2) {
		console_print("\r\nThis function needs a name and value!");
	} else {
		for(i = 0; i < CONFIG_ITEMS; i++) {
			if(usbrf_config[i].name[0] != 0 && !strncasecmp(usbrf_config[i].name, name, strlen(usbrf_config[i].name))) {
				usbrf_config[i].value = value;

				console_print("\r\n%s = ", usbrf_config[i].name);
    		sprintf(buf, usbrf_config[i].format, usbrf_config[i].value);
				console_print(buf);
				return;
			}
		}

		console_print("\r\nConfig value not found!");
	}
}

/**
 * List all values
 */
static void config_cmd_list(char *cmdLine __attribute((unused))) {
	int i = 0;
	char buf[128];

	// Loop trough all config items
	for(i = 0; i < CONFIG_ITEMS; i++) {
		// Check if it is set
		if(usbrf_config[i].name[0] != 0) {
   		console_print("\r\n%s = ", usbrf_config[i].name);
    	sprintf(buf, usbrf_config[i].format, usbrf_config[i].value);
			console_print(buf);
		}
	}

}

/**
 * Reset to initial settings
 */
static void config_cmd_reset(char *cmdLine __attribute((unused))) {
	memcpy(usbrf_config, init_config, sizeof(init_config));
	console_print("\r\nSuccessfully reset to default settings.");
}
