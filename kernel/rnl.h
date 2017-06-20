/*
 * RNL (RINA NetLink Layer)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_RNL_H
#define RINA_RNL_H

#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <net/genetlink.h>
#include <net/netlink.h>
#include <net/sock.h>

#include "irati/kernel-msg.h"

typedef uint32_t rnl_sn_t;
typedef uint32_t rnl_port_t;

int              rnl_init(void);
void             rnl_exit(void);

struct rnl_set;

struct rnl_set * rnl_set_create(void);
int              rnl_set_destroy(struct rnl_set * set);

typedef int (* message_handler_cb)(void *             data,
                                   struct sk_buff *   buff,
                                   struct genl_info * info);
int              rnl_handler_register(struct rnl_set *   set,
                                      msg_type_t         msg_type,
                                      void *             data,
                                      message_handler_cb handler);
int              rnl_handler_unregister(struct rnl_set * set,
                                        msg_type_t       msg_type);

int              rnl_set_register(struct rnl_set * set);
int              rnl_set_unregister(struct rnl_set * set);
rnl_sn_t         rnl_get_next_seqn(struct rnl_set * set);
rnl_port_t       rnl_get_ipc_manager_port(void);
void             rnl_set_ipc_manager_port(rnl_port_t port);

#endif
