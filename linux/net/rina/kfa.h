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

/* Only requests the pidm to reserve a valid port-id */
port_id_t    kfa_port_id_reserve(struct kfa *     instance,
                                 ipc_process_id_t id);

/* Rquest the pidm to release a valid port-id */
int          kfa_port_id_release(struct kfa * instance,
                                 port_id_t    port_id);

int          kfa_flow_deallocate(struct kfa * instance,
                                 port_id_t    id);

int          kfa_flow_sdu_write(struct ipcp_instance_data * data,
                                port_id_t     id,
                                struct sdu *  sdu);

int          kfa_flow_sdu_read(struct kfa *  instance,
                               port_id_t     id,
                               struct sdu ** sdu);

#if 0
struct ipcp_flow * kfa_flow_find_by_pid(struct kfa * instance,
                                        port_id_t    pid);
#endif

struct rmt;

int          kfa_flow_rmt_bind(struct kfa * instance,
                               port_id_t    pid,
                               struct rmt * rmt);

struct ipcp_instance * kfa_ipcp_instance(struct kfa * instance);
#endif
