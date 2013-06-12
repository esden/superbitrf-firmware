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

#define CONVERT_MAX_BUFFER			512

struct TransmitConvert {
	u8 in_buffer[CONVERT_MAX_BUFFER];
	u8 out_buffer[CONVERT_MAX_BUFFER];
	u16 in_ptr;
	u16 in_max_ptr;
	u16 out_ptr;
	u16 out_max_ptr;
};

struct ReceiveConvert {
	u8 read_buffer[CONVERT_MAX_BUFFER];
	u8 write_buffer[CONVERT_MAX_BUFFER];
	int read_ptr;
	int read_max_ptr;
	int write_ptr;
	int write_max_ptr;
};

/* The convert structures */
extern struct TransmitConvert transmit_convert;
extern struct ReceiveConvert receive_convert;

/* The external functions */
void convert_init(void);
void convert_pprz_dsmp(void);
void convert_dsmp_pprz(void);

#endif /* PROTOCOL_CONVERT_H_ */
