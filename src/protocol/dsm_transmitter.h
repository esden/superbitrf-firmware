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

#ifndef PROTOCOL_DSM_TRANSMITTER_H_
#define PROTOCOL_DSM_TRANSMITTER_H_

enum dsm_transmitter_status {
	DSM_TRANSMITTER_BIND			= 0x0,		/**< The transmitter bind status */
	DSM_TRANSMITTER_SENDA			= 0x1,		/**< The transmitter send on channel A status */
	DSM_TRANSMITTER_SENDB			= 0x2,		/**< The transmitter send on channel B status */
};

struct DsmTransmitter {
	enum dsm_transmitter_status status;			/**< The transmitter status */
	bool sending;								/**< If the transmitter is sending */
	u16 packet_count;							/**< The amount of packets send */
	u16 overflow_count;							/**< The amount of packets that didn't got an IRQ after sending */
	u16 error_count;							/**< The number of send packet errors */
};

/* External functions */
void dsm_transmitter_start_bind(void);
void dsm_transmitter_start(void);
void dsm_transmitter_on_timer(void);
void dsm_transmitter_on_receive(bool error);
void dsm_transmitter_on_send(bool error);

#endif /* PROTOCOL_DSM_TRANSMITTER_H_ */
