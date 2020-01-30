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
#include "iodev.h"

struct kfa;

struct kfa *kfa_create(void);
int	    kfa_destroy(struct kfa *instance);

/* Only requests the pidm to reserve a valid port-id */
port_id_t   kfa_port_id_reserve(struct kfa      *instance,
				ipc_process_id_t id);

/* Rquest the pidm to release a valid port-id */
int	    kfa_port_id_release(struct kfa *instance,
				port_id_t   port_id);

int	    kfa_flow_skb_write(struct ipcp_instance_data * data,
			       port_id_t   id,
			       struct sk_buff * skb,
			       size_t size,
                               bool blocking);

int	    kfa_flow_ub_write(struct kfa * kfa,
			      port_id_t   id,
			      const char __user *buffer,
			      struct iov_iter * iov,
			      struct sk_buff * skb,
			      size_t size,
                              bool blocking);

/* If the flow is deallocated it returns 0 (EOF), otherwise
 * it may report an error with a negative value or return
 * the number of bytes read (positive value)
 */
int kfa_flow_du_read(struct kfa  * instance,
		     port_id_t    id,
		     struct du ** du,
		     size_t       size,
                     bool blocking);

int kfa_flow_readable(struct kfa       *instance,
                      port_id_t        id,
                      unsigned int     *mask,
                      struct file      *f,
                      poll_table       *wait);

int kfa_flow_set_iowqs(struct kfa      * instance,
		       struct iowaitqs * wqs,
		       port_id_t pid);

int kfa_flow_cancel_iowqs(struct kfa      * instance,
			   port_id_t pid);

#if 0
struct ipcp_flow *kfa_flow_find_by_pid(struct kfa *instance,
				       port_id_t   pid);
#endif /* 0 */

struct rmt;

/* structure automatically freed when there are no more readers or writers */
int kfa_flow_create(struct kfa           *instance,
		    port_id_t		  pid,
		    struct ipcp_instance *ipcp,
		    ipc_process_id_t	 ipc_id,
		    struct name          *user_ipcp_name,
		    bool		  msg_boundaries);

struct ipcp_instance *kfa_ipcp_instance(struct kfa *instance);

bool kfa_flow_exists(struct kfa *kfa, port_id_t port_id);

size_t kfa_flow_max_sdu_size(struct kfa * kfa, port_id_t port_id);
#endif /* RINA_KFA_H */
