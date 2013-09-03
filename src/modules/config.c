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

#include "config.h"
#include "../helper/dsm.h"
//#include <libopencm3/stm32/flash.h>

struct Config usbrf_config;
char debug_msg[512];

void (*protocol_functions[][3])(void) = {
	{dsm_receiver_init, dsm_receiver_start, dsm_receiver_stop},
	{dsm_transmitter_init, dsm_transmitter_start, dsm_transmitter_stop},
	{dsm_mitm_init, dsm_mitm_start, dsm_mitm_stop},
};

void config_init(void) {
	struct Config init_config = {
			.version					= 0x01,
			.protocol					= DSM_MITM,
			.protocol_start 			= true,
			.debug_enable 				= false,
			.debug_button				= true,
			.debug_cyrf6936				= false,
			.debug_dsm					= false,
			.debug_protocol				= true,
			.timer_scaler				= 1,
			.dsm_start_bind				= false,
			.dsm_max_channel			= DSM_MAX_CHANNEL,
			.dsm_bind_channel			= -1,
			.dsm_bind_mfg_id			= {0xF2, 0x39, 0xB7, 0x77},
			.dsm_protocol				= 0xA2,
			.dsm_num_channels			= 6,
			.dsm_force_dsm2				= true,
			.dsm_max_missed_packets 	= 3,
			.dsm_bind_packets			= DSM_BIND_PACKETS,
			.dsm_mitm_both_data			= false,
			.dsm_mitm_has_uplink		= true
	};

	// For now just copy the init config FIXME
	memcpy(&usbrf_config, &init_config, sizeof(init_config));
}
