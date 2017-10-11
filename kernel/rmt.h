/*
 * RMT (Relaying and Multiplexing Task)
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

#ifndef RINA_RMT_H
#define RINA_RMT_H

#include <linux/hashtable.h>

#include "common.h"
#include "du.h"
#include "efcp.h"
#include "ipcp-factories.h"
#include "ipcp-instances.h"
#include "ps-factory.h"
#include "sdup.h"
#include "rds/robjects.h"

struct rmt;

/*
 * NOTEs:
 *
 * QoS-id - An identifier unambiguous within this DIF that identifies a
 * QoS-hypercube. As QoS-cubes are created they are sequentially enumerated.
 * QoS-id is an element of Data Transfer PCI that may be used by the RMT to
 * classify PDUs.
 *
 * RMT - This task is an element of the data transfer function of a DIF.
 * Logically, it sits between the EFCP and SDU Protection.  RMT performs the
 * real time scheduling of sending PDUs on the appropriate (N-1)-ports of the
 * (N-1)-DIFs available to the RMT.
 */

/* NOTE: There's one RMT for each IPC Process */

/* Plugin support */

#define RMT_PS_HASHSIZE 7

/* FIXME: Hide these structs */
enum flow_state {
	N1_PORT_STATE_ENABLED = 0,
	N1_PORT_STATE_DISABLED,
	N1_PORT_STATE_DO_NOT_DISABLE,
	N1_PORT_STATE_DEALLOCATED,
};

struct n1_port_stats {
	unsigned int plen; /* port len, all pdus enqueued in PS queue/s */
	unsigned int drop_pdus;
	unsigned int err_pdus;
	unsigned int tx_pdus;
	unsigned int tx_bytes;
	unsigned int rx_pdus;
	unsigned int rx_bytes;
};

struct rmt_n1_port {
	spinlock_t		lock;
	port_id_t		port_id;
	struct ipcp_instance	*n1_ipcp;
	struct hlist_node	hlist;
	enum flow_state		state;
	atomic_t		refs_c;
	struct du		*pending_du;
	struct sdup_port 	*sdup_port;
	struct n1_port_stats	stats;
	bool			wbusy;
	void 			*rmt_ps_queues;
	struct robject		robj;
};

struct rmt	  *rmt_create(struct kfa *kfa,
			      struct efcp_container *efcpc,
			      struct sdup *sdup,
			      struct robject *parent);
int		   rmt_destroy(struct rmt *instance);
int		   rmt_config_set(struct rmt *instance,
				  struct rmt_config *rmt_config);
struct rmt_config *rmt_config_get(struct rmt *instance);
int		   rmt_n1port_bind(struct rmt *instance,
				   port_id_t id,
				   struct ipcp_instance *n1_ipcp);
int		   rmt_n1port_unbind(struct rmt *instance,
				     port_id_t id);
int		   rmt_pff_add(struct rmt *instance,
			       struct mod_pff_entry *entry);
int		   rmt_pff_remove(struct rmt *instance,
				  struct mod_pff_entry *entry);
int		   rmt_pff_port_state_change(struct rmt *rmt,
					     port_id_t	port_id,
					     bool up);
int		   rmt_pff_dump(struct rmt *instance,
				struct list_head *entries);
int		   rmt_pff_flush(struct rmt *instance);
int		   rmt_pff_modify(struct rmt *instance,
				  struct list_head *entries);
int		   rmt_send(struct rmt *instance,
			    struct du * du);
int		   rmt_send_port_id(struct rmt *instance,
				    port_id_t id,
				    struct du * du);
int		   rmt_receive(struct rmt *instance,
			       struct du *du,
			       port_id_t from);
int		   rmt_enable_port_id(struct rmt *instance,
				      port_id_t id);
int		   rmt_disable_port_id(struct rmt *instance,
				       port_id_t id);
int		   rmt_select_policy_set(struct rmt *rmt,
					 const string_t *path,
					 const string_t *name);
int		   rmt_set_policy_set_param(struct rmt *rmt,
					    const string_t *path,
					    const string_t *name,
					    const string_t *value);
int		   rmt_address_add(struct rmt *instance,
				   address_t address);
int		   rmt_address_remove(struct rmt *instance,
				      address_t address);
struct rmt	  *rmt_from_component(struct rina_component *component);
struct robject    *rmt_robject(struct rmt * instance);
#endif
