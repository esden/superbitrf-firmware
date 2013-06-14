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

#ifndef PROTOCOL_DSM_RECEIVER_H_
#define PROTOCOL_DSM_RECEIVER_H_

enum dsm_receiver_status {
	DSM_RECEIVER_BIND			= 0x0,			/**< The receiver bind status */
	DSM_RECEIVER_SYNC_A			= 0x1,			/**< The receiver syncing channel A status */
	DSM_RECEIVER_SYNC_B			= 0x2,			/**< The receiver syncing channel B status */
	DSM_RECEIVER_RECV			= 0x3,			/**< The receiver is receiving status */
};

struct DsmReceiver {
	enum dsm_receiver_status status;			/**< The receiver status */
	u8 num_channels;							/**< The number of channels the transmitter is sending information over (not RF channels) */
	bool missed_packet;							/**< When it misses one packet it is true, and doesn't try to resync right away */
};

/* External functions */
void dsm_receiver_start_bind(void);
void dsm_receiver_start(void);
void dsm_receiver_on_timer(void);
void dsm_receiver_on_receive(bool error);
void dsm_receiver_on_send(bool error);

#endif /* PROTOCOL_DSM_RECEIVER_H_ */
