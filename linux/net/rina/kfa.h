/*
 * KFA (Kernel Flow Allocator)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

/* Returns a flow-id, the flow is uncommitted yet */
flow_id_t    kfa_flow_create(struct kfa * instance);

/* Commits the flow, binds the flow to a port-id, frees the flow-id */
int          kfa_flow_bind(struct kfa * 	  instance,
                           flow_id_t    	  fid,
                           port_id_t    	  pid,
                           struct ipcp_instance * ipc_process,
                           ipc_process_id_t       ipc_id);

/*
 * Un-commits the flow, binds the flow to a flow-id (different from the one
 * obtained during creation
 */
flow_id_t    kfa_flow_unbind(struct kfa * instance,
                             port_id_t    id);

/* Finally destroys the flow */
int          kfa_flow_destroy(struct kfa * instance,
                              flow_id_t    id);

/* Once the flow is bound to a port, we can write/read SDUs */
int          kfa_flow_sdu_write(struct kfa *  instance,
                                port_id_t     id,
                                struct sdu *  sdu);

struct sdu * kfa_flow_sdu_read(struct kfa *  instance,
                               port_id_t     id);

#endif
