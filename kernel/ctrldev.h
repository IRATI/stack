/*
 * IRATI Control device
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef IRATI_CTRLDEV_H
#define IRATI_CTRLDEV_H

#include "common.h"

int ctrldev_init(void);
void ctrldev_fini(void);

/* The signature of a message handler. */
typedef int (* irati_msg_handler_t)(irati_msg_port_t ctrl_port,
                                    struct irati_msg_base *bmsg,
				    void * data);

int irati_handler_register(irati_msg_t msg_type,
		           irati_msg_handler_t handler,
			   void * data);
int irati_handler_unregister(irati_msg_t msg_type);

void irati_ctrl_dev_msg_free(struct irati_msg_base *bmsg);

int irati_ctrl_dev_snd_resp_msg(irati_msg_port_t port,
				struct irati_msg_base *bmsg);

uint32_t ctrl_dev_get_next_seqn(void);

#endif
