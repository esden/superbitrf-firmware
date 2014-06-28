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

#ifndef MODULES_CONFIG_H_
#define MODULES_CONFIG_H_

#include <libopencm3/cm3/common.h>

/**
 * Include the different protocols
 */
#include "../protocol/dsm_receiver.h"
#include "../protocol/dsm_transmitter.h"
#include "../protocol/dsm_mitm.h"

/**
 * Includes for debugging
 */
#include "cdcacm.h"
#include <stdio.h>
#include <string.h>

/**
 * Sending a debug string
 */
extern char debug_msg[512];
#define DEBUG(type, fmt, ...) {											\
	if(usbrf_config.debug_enable && usbrf_config.debug_ ## type) {		\
		sprintf(debug_msg, "[" #type "]: " #fmt "\r\n", ##__VA_ARGS__);	\
		cdcacm_send(debug_msg, strlen(debug_msg));						\
	}																	\
}

/**
 * The different kind of protocols available
 */
enum Protocol {
	DSM_RECEIVER		= 0,
	DSM_TRANSMITTER,
	DSM_MITM,
	DSM_SCANNER,
	DSM_HIJACK,
	TUDELFT_DELFY
};

/**
 * The init and start, stop functions of the protocols
 */
#define PROTOCOL_INIT 0
#define PROTOCOL_START 1
#define PROTOCOL_STOP 2
extern void (*protocol_functions[][3])(void);

struct Config {
	uint32_t version;					/**< The static version number of the config */
	enum Protocol protocol;				/**< The protocol that is running */
	bool protocol_start;				/**< Start the protocol at boot */

	bool debug_enable;					/**< When debugging is enabled */
	bool debug_button;					/**< When debugging the button module is enabled */
	bool debug_cyrf6936;				/**< When debugging the CYRF6936 module is enabled */
	bool debug_dsm;						/**< When debugging the DSM helper is enabled */
	bool debug_protocol;				/**< When debugging the protocol is enabled */

	uint32_t timer_scaler;				/**< The timer scaler for debugging */

	/* DSM protocol specific */
	bool dsm_start_bind;				/**< Start with binding at boot */
	uint8_t dsm_max_channel;			/**< The maximum channel nummer */
	int8_t dsm_bind_channel;			/**< The channel used for binding */
	uint8_t dsm_bind_mfg_id[4];			/**< The Manufacturer ID used for binding */
	uint8_t dsm_protocol;				/**< The DSM protocol used */
	uint8_t dsm_num_channels;			/**< The number of command channels */
	bool dsm_force_dsm2;				/**< Force the use of DSM2 instead of DSMX */
	uint8_t dsm_max_missed_packets;		/**< The maximum amount of missed packets since last receive */
	uint16_t dsm_bind_packets;			/**< The amount of bind packets to send */
	bool dsm_mitm_both_data;			/**< Whether we receive data on both channel A->B and B->A or only B->A */
	bool dsm_mitm_has_uplink;			/**< Whether the MITM has the uplink enabled */
};
extern struct Config usbrf_config;

/**
 * External functions
 */
void config_init(void);
void config_store(void);
void config_load(struct Config *config);

#endif /* MODULES_CONFIG_H_ */
