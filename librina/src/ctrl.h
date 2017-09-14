/*
 * Functionalities to interact with the IRATI ctrl-device
 *
 * Copyright (C) 2015-2016 Nextworks
 * Author: Vincenzo Maffione <v.maffione@gmail.com>
 * Author: Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This file is part of rlite.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef LIBRINA_CTRL_H
#define LIBRINA_CTRL_H

#include "irati/kucommon.h"

#ifdef __cplusplus
extern "C" {
#endif

struct irati_msg_base * irati_read_next_msg(int cfd);
int irati_write_msg(int cfd, struct irati_msg_base *msg);
int irati_open_ctrl_port(irati_msg_port_t port_id);
void irati_ctrl_msg_free(struct irati_msg_base *msg);
int close_port(int cfd);
irati_msg_port_t get_app_ctrl_port_from_cfd(int cfd);
int irati_open_io_port(int port_id);

#ifdef __cplusplus
}
#endif

#endif /* LIBRINA_CTRL_H */
