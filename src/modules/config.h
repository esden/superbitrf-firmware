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

#ifndef DEBUG
#define DEBUG(a ,...) if(false){};
#endif

#define CONFIG_ITEMS 50
struct ConfigItem {
	char *name;
  char *format;
  float value;
};
extern struct ConfigItem usbrf_config[CONFIG_ITEMS];

/**
 * External functions
 */
void config_init(void);
void config_store(void);
void config_load(struct ConfigItem config[]);

#endif /* MODULES_CONFIG_H_ */
