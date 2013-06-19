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

#ifndef PROTOCOL_CONVERT_H_
#define PROTOCOL_CONVERT_H_

#define MAX_BUFFER			2048

struct Buffer {
	u16 insert_idx;
	u16 extract_idx;
	u8 buffer[MAX_BUFFER];
};

/* The two buffers used */
extern struct Buffer cdcacm_to_radio;
extern struct Buffer radio_to_cdcacm;

/* The external functions */
void convert_init(void);
bool convert_cdcacm_to_radio_insert(u8 *data, u16 length);
bool convert_radio_to_cdcacm_insert(u8 *data, u16 length);
u16 convert_extract(struct Buffer *buffer, u8 *data, u16 length);
u16 convert_insert_size(struct Buffer *buffer);
u16 convert_extract_size(struct Buffer *buffer);
void convert_cdcacm_receive_cb(char *data, int size);
void convert_cdcacm_send_cb(void);

#endif /* PROTOCOL_CONVERT_H_ */
