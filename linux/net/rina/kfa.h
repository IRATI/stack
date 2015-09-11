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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

struct kfa *kfa_create(void);
int	    kfa_destroy(struct kfa *instance);

/* Only requests the pidm to reserve a valid port-id */
port_id_t   kfa_port_id_reserve(struct kfa      *instance,
				ipc_process_id_t id);

/* Rquest the pidm to release a valid port-id */
int	    kfa_port_id_release(struct kfa *instance,
				port_id_t   port_id);

int	    kfa_flow_sdu_write(struct ipcp_instance_data *data,
			       port_id_t   id,
			       struct sdu *sdu);

int	    kfa_flow_sdu_read(struct kfa  *instance,
			      port_id_t    id,
			      struct sdu **sdu);

#if 0
struct ipcp_flow *kfa_flow_find_by_pid(struct kfa *instance,
				       port_id_t   pid);
#endif /* 0 */

struct rmt;

/* structure automatically freed when there are no more readers or writers */
int	    kfa_flow_create(struct kfa           *instance,
			    port_id_t		  pid,
			    struct ipcp_instance *ipcp);

/* set options associated with a flow e.g
 * int kfa_flow_opts_set(i, pid, FLOW_O_NONBLOCK)
 */
int	    kfa_flow_opts_set(struct kfa *instance,
			      port_id_t   pid,
			      flow_opts_t flow_opts);

/* returns the current options for a flow */
flow_opts_t kfa_flow_opts(struct kfa *instance,
			  port_id_t   pid);

struct ipcp_instance *kfa_ipcp_instance(struct kfa *instance);
#endif /* RINA_KFA_H */
