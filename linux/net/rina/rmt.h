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
#include <linux/crypto.h>

#include "common.h"
#include "du.h"
#include "efcp.h"
#include "ipcp-factories.h"
#include "ipcp-instances.h"
#include "ps-factory.h"

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
	N1_PORT_STATE_ENABLED,
	N1_PORT_STATE_DISABLED,
	N1_PORT_STATE_DEALLOCATED,
};

struct rmt_n1_port {
	spinlock_t lock;
	port_id_t port_id;
	struct ipcp_instance *n1_ipcp;
	struct hlist_node hlist;
	enum flow_state	state;
	atomic_t n_sdus;
	struct dup_config_entry *dup_config;
	struct crypto_blkcipher *blkcipher;
	atomic_t pending_ops;
};

/* The key in this struct is used to filter by cep_ids, qos_id, address... */
struct rmt_kqueue {
	struct rfifo *queue;
	unsigned int key;
	unsigned int max_q;
	unsigned int min_qth;
	unsigned int max_qth;
	struct hlist_node hlist;
};

struct rmt_qgroup {
	port_id_t pid;
	struct hlist_node hlist;
	DECLARE_HASHTABLE(queues, RMT_PS_HASHSIZE);
};

struct rmt_queue_set {
	DECLARE_HASHTABLE(qgroups, RMT_PS_HASHSIZE);
};

struct rmt_kqueue *rmt_kqueue_create(unsigned int key);
struct rmt_kqueue *rmt_kqueue_find(struct rmt_qgroup *g,
				   unsigned int key);
int                rmt_kqueue_destroy(struct rmt_kqueue *q);

struct rmt_qgroup *rmt_qgroup_create(port_id_t pid);
struct rmt_qgroup *rmt_qgroup_find(struct rmt_queue_set *qs,
				   port_id_t pid);
int		   rmt_qgroup_destroy(struct rmt_qgroup *g);

struct rmt_queue_set *rmt_queue_set_create(void);
int		      rmt_queue_set_destroy(struct rmt_queue_set *qs);


struct rmt *rmt_create(struct ipcp_instance *parent,
			struct kfa *kfa,
			struct efcp_container *efcpc);
int	    rmt_destroy(struct rmt *instance);
int	    rmt_address_set(struct rmt *instance,
			    address_t address);
int	    rmt_dt_cons_set(struct rmt *instance,
			    struct dt_cons *dt_cons);
int	    rmt_sdup_config_set(struct rmt *instance,
				struct sdup_config *sdup_conf);
int	    rmt_config_set(struct rmt *instance,
			    struct rmt_config *rmt_config);
int	    rmt_n1port_bind(struct rmt *instance,
			     port_id_t id,
			     struct ipcp_instance *n1_ipcp);
int	    rmt_n1port_unbind(struct rmt *instance,
			       port_id_t id);
int	    rmt_pff_add(struct rmt *instance,
			 struct mod_pff_entry *entry);
int	    rmt_pff_remove(struct rmt *instance,
			 struct mod_pff_entry *entry);
int	    rmt_pff_port_state_change(struct rmt *rmt,
				      port_id_t	port_id,
				      bool up);
int	    rmt_pff_dump(struct rmt *instance,
			  struct list_head *entries);
int	    rmt_pff_flush(struct rmt *instance);
int	    rmt_send(struct rmt *instance,
		     struct pdu *pdu);
int	    rmt_send_port_id(struct rmt *instance,
			      port_id_t id,
			      struct pdu *pdu);
int	    rmt_receive(struct rmt *instance,
			 struct sdu *sdu,
			 port_id_t from);
int	    rmt_enable_port_id(struct rmt *instance,
				port_id_t id);
int	    rmt_disable_port_id(struct rmt *instance,
				 port_id_t id);
int	    rmt_select_policy_set(struct rmt *rmt,
				  const string_t *path,
				  const string_t *name);
int	    rmt_set_policy_set_param(struct rmt *rmt,
				      const string_t *path,
				      const string_t *name,
				      const string_t *value);
int	    rmt_enable_encryption(struct rmt *instance,
				   bool	enable_encryption,
				   bool	enable_decryption,
				   struct buffer *encrypt_key,
				   port_id_t port_id);
struct rmt *rmt_from_component(struct rina_component *component);

#endif
