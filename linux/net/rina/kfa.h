/*
 * KFA (Kernel Flow Allocator)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#ifndef RINA_KFA_H
#define RINA_KFA_H

#include "common.h"
#include "du.h"
#include "ipcp-factories.h"

struct kfa;

struct kfa * kfa_create(void);
int          kfa_destroy(struct kfa * instance);

/* Returns a port-id, the flow is uncommitted yet */
port_id_t    kfa_flow_create(struct kfa *     instance,
                             ipc_process_id_t id,
                             bool             to_app);

/* Commits the flow, binds the flow to a port-id */
int          kfa_flow_bind(struct kfa *           instance,
                           port_id_t              pid,
                           struct ipcp_instance * ipc_process,
                           ipc_process_id_t       ipc_id);

int          kfa_flow_deallocate(struct kfa * instance,
                                 port_id_t    id);

int          kfa_remove_all_for_id(struct kfa *     instance,
                                   ipc_process_id_t id);

/* Once the flow is bound to a port, we can write/read SDUs */
int          kfa_flow_sdu_write(struct kfa *  instance,
                                port_id_t     id,
                                struct sdu *  sdu);

int          kfa_flow_sdu_read(struct kfa *  instance,
                               port_id_t     id,
                               struct sdu ** sdu);

int          kfa_sdu_post(struct kfa * instance,
                          port_id_t    id,
                          struct sdu * sdu);

struct ipcp_flow * kfa_find_flow_by_pid(struct kfa * instance,
                                        port_id_t    pid);

/*
 * Used by the RMT to push SDU intended for user
 * space apps, not to follow the flow through the DIF stack
 */
int          kfa_sdu_post_to_user_space(struct kfa * instance,
                                        struct sdu * sdu,
                                        port_id_t    to);

#endif
