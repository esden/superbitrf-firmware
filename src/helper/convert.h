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

#ifndef MAX_BUFFER
#define MAX_BUFFER			2048
#endif

/**
 * The buffer structure
 */
struct Buffer {
	u16 insert_idx;						/**< The insert index of the buffer */
	u16 extract_idx;					/**< The extract index of the buffer */
	u8 buffer[MAX_BUFFER];				/**< The buffer with the data */

	void (*buffer_insert_cb)(void);		/**< The callback when there is new data in the buffer */
};

/* The external functions */
void convert_init(struct Buffer *buffer);
bool convert_insert(struct Buffer *buffer, u8 *data, u16 length);
u16 convert_extract(struct Buffer *buffer, u8 *data, u16 length);
void convert_set_insert_cb(struct Buffer *buffer, void (*buffer_insert_cb)(void));

u16 convert_insert_size(struct Buffer *buffer);
u16 convert_extract_size(struct Buffer *buffer);

void convert_radio_to_channels(uint8_t* data, uint8_t nb_channels, bool is_11bit, int16_t* channels);

#endif /* PROTOCOL_CONVERT_H_ */
