/*
 * RMT (Relaying and Multiplexing Task)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#include <linux/export.h>
#include <linux/types.h>
#include <linux/hashtable.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/string.h>
/*FIXME: to be re removed after removing tasklets */
#include <linux/interrupt.h>

#define RINA_PREFIX "rmt"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"
#include "rmt.h"
#include "pft.h"
#include "efcp-utils.h"
#include "serdes.h"
#include "pdu-ser.h"
#include "rmt-ps.h"
#include "policies.h"
#include "rds/rstr.h"
#include "ipcp-instances.h"
#include "ipcp-utils.h"

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))
#define MAX_PDUS_SENT_PER_CYCLE 10

static struct policy_set_list policy_sets = {
        .head = LIST_HEAD_INIT(policy_sets.head)
};


static struct rmt_n1_port * n1_port_create(port_id_t id,
                                           struct ipcp_instance * n1_ipcp,
                                           struct dup_config_entry * dup_config)
{
        struct rmt_n1_port * tmp;

        ASSERT(is_port_id_ok(id));

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        INIT_HLIST_NODE(&tmp->hlist);

        tmp->port_id = id;
        tmp->n1_ipcp = n1_ipcp;
        tmp->state   = N1_PORT_STATE_ENABLED;
        tmp->dup_config = dup_config;

        /* dup state init. FIXME: This has to be moved to the
         * specific encryption policy initialization
         */
        if (dup_config != NULL && dup_config->encryption_cipher != NULL){
            tmp->blkcipher = crypto_alloc_blkcipher(dup_config->encryption_cipher, 0, 0);
            if (IS_ERR(tmp->blkcipher)) {
                LOG_ERR("could not allocate blkcipher handle for %s\n", dup_config->encryption_cipher);
                return NULL;
            }
        }else
            tmp->blkcipher = NULL;

        atomic_set(&tmp->n_sdus, 0);
        spin_lock_init(&tmp->lock);

        LOG_DBG("N-1 port %pK created successfully (port-id = %d)", tmp, id);

        return tmp;
}

static int n1_port_n1_user_ipcp_unbind(struct rmt_n1_port * n1p)
{
        struct ipcp_instance * n1_ipcp;

        ASSERT(n1p);

        /* FIXME: this has to be moved to specific encryption policy */
        if (n1p->blkcipher != NULL){
            crypto_free_blkcipher(n1p->blkcipher);
        }

        n1_ipcp = n1p->n1_ipcp;
        if (n1_ipcp                             &&
            n1_ipcp->ops                        &&
            n1_ipcp->data                       &&
            n1_ipcp->ops->flow_unbinding_user_ipcp) {
                if (n1_ipcp->ops->flow_unbinding_user_ipcp(n1_ipcp->data,
                                                           n1p->port_id)) {
                        LOG_ERR("Could not unbind IPCP as user of an N-1 flow");
                        return -1;
                }
        }
        return 0;
}

static int n1_port_destroy(struct rmt_n1_port * n1p)
{
        ASSERT(n1p);
        LOG_DBG("Destroying N-1 port %pK (port-id = %d)", n1p, n1p->port_id);

        hash_del(&n1p->hlist);

        if (n1p->dup_config) {
        	dup_config_entry_destroy(n1p->dup_config);
        }

        /* FIXME: this has to be moved to specific encryption policy */
        if (n1p->blkcipher) {
        	crypto_free_blkcipher(n1p->blkcipher);
        }

        rkfree(n1p);

        return 0;
}

struct n1pmap {
        spinlock_t lock; /* FIXME: Has to be moved in the pipelines */

        DECLARE_HASHTABLE(n1_ports, 7);
};

static struct n1pmap * n1pmap_create(void)
{
        struct n1pmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->n1_ports);
        spin_lock_init(&tmp->lock);

        return tmp;
}

static int n1pmap_destroy(struct n1pmap * m)
{
        struct rmt_n1_port * entry;
        struct hlist_node *  tmp;
        int                  bucket;

        ASSERT(m);

        hash_for_each_safe(m->n1_ports, bucket, tmp, entry, hlist) {
                ASSERT(entry);
                if (n1_port_n1_user_ipcp_unbind(entry) ||
                    n1_port_destroy(entry)) {
                        LOG_ERR("Could not destroy entry %pK", entry);
                        return -1;
                }
        }
        rkfree(m);
        return 0;
}

static struct rmt_n1_port * n1pmap_find(struct n1pmap * m,
                                      port_id_t       id)
{
        struct rmt_n1_port *      entry;
        const struct hlist_head * head;

        ASSERT(m);

        if (!is_port_id_ok(id))
                return NULL;

        head = &m->n1_ports[rmap_hash(m->n1_ports, id)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->port_id == id)
                        return entry;
        }

        return NULL;
}

struct pft_cache {
        port_id_t * pids;  /* Array of port_id_t */
        size_t      count; /* Entries in the pids array */
};

/* RMT DATA MODELS FOR RMT PS */

struct rmt_kqueue * rmt_kqueue_create(unsigned int key)
{
        struct rmt_kqueue * tmp;
        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;
        tmp->queue = rfifo_create_ni();
        if (!tmp->queue) {
                rkfree(tmp);
                return NULL;
        }

        tmp->key = key;
        INIT_HLIST_NODE(&tmp->hlist);

        return tmp;
}
EXPORT_SYMBOL(rmt_kqueue_create);

int rmt_kqueue_destroy(struct rmt_kqueue * q)
{
        if (!q) {
                LOG_ERR("No RMT Key-queue to destroy...");
                return -1;
        }

        hash_del(&q->hlist);

        if (q->queue) rfifo_destroy(q->queue, (void (*)(void *)) pdu_destroy);

        rkfree(q);

        return 0;
}
EXPORT_SYMBOL(rmt_kqueue_destroy);

struct rmt_kqueue * rmt_kqueue_find(struct rmt_qgroup * g,
                                    unsigned int        key)
{
        struct rmt_kqueue * entry;
        const struct hlist_head * head;

        ASSERT(g);

        head = &g->queues[rmap_hash(g->queues, key)];
        hlist_for_each_entry(entry, head, hlist) {
                LOG_DBG("Looking for kqueue, current port: %u",
                         entry->key);
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}
EXPORT_SYMBOL(rmt_kqueue_find);

struct rmt_qgroup * rmt_qgroup_create(port_id_t pid)
{
        struct rmt_qgroup * tmp;

        if (!is_port_id_ok(pid)) {
                LOG_ERR("Could not create qgroup, wrong port id");
                return NULL;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;

        tmp->pid = pid;
        hash_init(tmp->queues);

        return tmp;
}
EXPORT_SYMBOL(rmt_qgroup_create);

int rmt_qgroup_destroy(struct rmt_qgroup * g)
{
        struct rmt_kqueue * entry;
        struct hlist_node * tmp;
        int                 bucket;

        ASSERT(g);
        hash_del(&g->hlist);

        hash_for_each_safe(g->queues, bucket, tmp, entry, hlist) {
                ASSERT(entry);

                if (rmt_kqueue_destroy(entry)) {
                        LOG_ERR("Could not destroy entry %pK", entry);
                        return -1;
                }
        }

        LOG_DBG("Qgroup for port: %u destroyed...", g->pid);
        rkfree(g);

        return 0;
}
EXPORT_SYMBOL(rmt_qgroup_destroy);

struct rmt_qgroup * rmt_qgroup_find(struct rmt_queue_set * qs,
                                    port_id_t              pid)
{
        struct rmt_qgroup * entry;
        const struct hlist_head * head;

        ASSERT(qs);

        head = &qs->qgroups[rmap_hash(qs->qgroups, pid)];

        hlist_for_each_entry(entry, head, hlist) {
                LOG_DBG("Looking for qgroup, current port: %u",
                         entry->pid);
                if (entry->pid == pid)
                        return entry;
        }
        return NULL;
}
EXPORT_SYMBOL(rmt_qgroup_find);


struct rmt_queue_set * rmt_queue_set_create(void)
{
        struct rmt_queue_set * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;

        hash_init(tmp->qgroups);

        return tmp;
}
EXPORT_SYMBOL(rmt_queue_set_create);

int rmt_queue_set_destroy(struct rmt_queue_set * qs)
{
        struct rmt_qgroup * entry;
        struct hlist_node * tmp;
        int                 bucket;

        ASSERT(qs);

        hash_for_each_safe(qs->qgroups, bucket, tmp, entry, hlist) {
                ASSERT(entry);

                if (rmt_qgroup_destroy(entry)) {
                        LOG_ERR("Could not destroy entry %pK", entry);
                        return -1;
                }
        }

        rkfree(qs);

        return 0;
}
EXPORT_SYMBOL(rmt_queue_set_destroy);

/* RMT DATAMODEL FOR RMT PS END */

static int pft_cache_init(struct pft_cache * c)
{
        ASSERT(c);

        c->pids  = NULL;
        c->count = 0;

        LOG_DBG("PFT cache %pK initialized", c);

        return 0;
}

static int pft_cache_fini(struct pft_cache * c)
{
        ASSERT(c);

        if (c->count) {
                ASSERT(c->pids);
                rkfree(c->pids);
        } else {
                ASSERT(!c->pids);
        }

        LOG_DBG("PFT cache %pK destroyed", c);

        return 0;
}

struct rmt {
        struct rina_component     base;
        address_t                 address;
        struct ipcp_instance *    parent;
        struct pft *              pft;
        struct kfa *              kfa;
        struct efcp_container *   efcpc;
        struct serdes *           serdes;
        struct tasklet_struct     egress_tasklet;
        struct n1pmap *           n1_ports;
        struct pft_cache          cache;
        struct sdup_config *      sdup_conf;
};

struct rmt *
rmt_from_component(struct rina_component * component)
{
        return container_of(component, struct rmt, base);
}
EXPORT_SYMBOL(rmt_from_component);

int rmt_select_policy_set(struct rmt * rmt,
                          const string_t * path,
                          const string_t * name)
{
        if (path && strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        return base_select_policy_set(&rmt->base, &policy_sets, name);
}
EXPORT_SYMBOL(rmt_select_policy_set);

int rmt_set_policy_set_param(struct rmt * rmt,
                             const char * path,
                             const char * name,
                             const char * value)
{
        struct rmt_ps *ps;
        int ret = -1;

        if (!rmt || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", rmt, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this RMT instance. */
                rcu_read_lock();
                ps = container_of(rcu_dereference(rmt->base.ps), struct rmt_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this RMT");
                } else {
                        LOG_ERR("Unknown RMT parameter policy '%s'", name);
                }
                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&rmt->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(rmt_set_policy_set_param);

int rmt_destroy(struct rmt * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (instance->n1_ports)
                n1pmap_destroy(instance->n1_ports);
        tasklet_kill(&instance->egress_tasklet);
        pft_cache_fini(&instance->cache);

        if (instance->pft)            pft_destroy(instance->pft);
        if (instance->serdes)         serdes_destroy(instance->serdes);
        if (instance->sdup_conf)      sdup_config_destroy(instance->sdup_conf);

        rina_component_fini(&instance->base);

        rkfree(instance);

        LOG_DBG("Instance %pK finalized successfully", instance);

        return 0;
}
EXPORT_SYMBOL(rmt_destroy);

int rmt_address_set(struct rmt * instance,
                    address_t    address)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (is_address_ok(instance->address)) {
                LOG_ERR("The RMT is already configured");
                return -1;
        }

        instance->address = address;

        return 0;
}
EXPORT_SYMBOL(rmt_address_set);

int rmt_dt_cons_set(struct rmt *     instance,
                    struct dt_cons * dt_cons)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!dt_cons) {
                 LOG_ERR("Bogus dt_cons passed");
                return -1;
        }

        instance->serdes = serdes_create(dt_cons);
        if (!instance->serdes) {
                LOG_ERR("Serdes creation failed");
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rmt_dt_cons_set);

static int extract_policy_parameters(struct dup_config_entry * entry)
{
	struct policy * policy;
	struct policy_parm * parameter;
	const string_t * aux;

	if (!entry) {
		LOG_ERR("Bogus entry passed");
		return -1;
	}

	policy = entry->ttl_policy;
	if (policy) {
		parameter = policy_param_find(policy, "initialValue");
		if (!parameter) {
			LOG_ERR("Could not find parameter 'initialValue' in TTL policy");
			return -1;
		}

		kstrtouint(policy_param_value(parameter), 10, &entry->initial_ttl_value);
		LOG_DBG("Initial TTL value is %u", entry->initial_ttl_value);
	}

	policy = entry->encryption_policy;
	if (policy) {
		parameter = policy_param_find(policy, "encryptAlg");
		if (!parameter) {
			LOG_ERR("Could not find parameter 'encryptAlg' in Encryption policy");
			return -1;
		}

		aux = policy_param_value(parameter);
		if (string_cmp(aux, "AES128") == 0 || string_cmp(aux, "AES256") == 0) {
			if (string_dup("ecb(aes)", &entry->encryption_cipher)) {
				LOG_ERR("Problems copying string ('encryptAlg' parameter value)");
				return -1;
			}
			LOG_DBG("Encryption cipher is %s", entry->encryption_cipher);
		} else {
			LOG_DBG("Unsupported encryption cipher %s", aux);
		}

		parameter = policy_param_find(policy, "macAlg");
		if (!parameter) {
			LOG_ERR("Could not find parameter 'macAlg' in Encryption policy");
			return -1;
		}

		aux = policy_param_value(parameter);
		if (string_cmp(aux, "SHA1") == 0 ) {
			if (string_dup("sha1", &entry->message_digest)) {
				LOG_ERR("Problems copying string ('message_digest' parameter value)");
				return -1;
			}
			LOG_DBG("Message digest is %s", entry->message_digest);
		} else if (string_cmp(aux, "MD5") == 0 ) {
			if (string_dup("md5", &entry->message_digest)) {
				LOG_ERR("Problems copying string ('message_digest' parameter value)");
				return -1;
			}
			LOG_DBG("Message digest is %s", entry->message_digest);
		} else {
			LOG_DBG("Unsupported message digest %s", aux);
		}
	}

	return 0;
}

int rmt_sdup_config_set(struct rmt *         instance,
                    	struct sdup_config * sdup_conf)
{
	struct dup_config * dup_pos;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!sdup_conf) {
                 LOG_ERR("Bogus sdup_conf passed");
                return -1;
        }

	/* FIXME this code should be moved to specific sdup policies */
        if (extract_policy_parameters(sdup_conf->default_dup_conf)) {
        	LOG_DBG("Setting SDU protection policies to NULL");
        	sdup_config_destroy(sdup_conf);
        	return -1;
        }
	list_for_each_entry(dup_pos, &sdup_conf->specific_dup_confs, next){
		if (extract_policy_parameters(dup_pos->entry)) {
			LOG_DBG("Setting sdu protection policies to NULL");
			sdup_config_destroy(sdup_conf);
			return -1;
		}
	}

        instance->sdup_conf = sdup_conf;

        return 0;
}
EXPORT_SYMBOL(rmt_sdup_config_set);

static int n1_port_write(struct serdes *      serdes,
                         struct rmt_n1_port * n1_port,
                         struct pdu * pdu)
{
        struct sdu *           sdu;
        struct pdu_ser *       pdu_ser;
        port_id_t              port_id;
        struct buffer *        buffer;
        struct ipcp_instance * n1_ipcp;
        struct pci *           pci;
        size_t                 ttl;
        struct dup_config_entry * dup_conf;

        ASSERT(n1_port);
        ASSERT(serdes);

        if (!pdu) {
                LOG_DBG("No PDU to work in this queue ...");
                return -1;
        }

        port_id = n1_port->port_id;
        n1_ipcp = n1_port->n1_ipcp;

        dup_conf = n1_port->dup_config;

        pci = 0;
        ttl = 0;

        atomic_dec(&n1_port->n_sdus);

        /* FIXME, this should be moved to specific TTL policy inside serdes*/
        if (dup_conf != NULL && dup_conf->ttl_policy != NULL){
            pci = pdu_pci_get_rw(pdu);
            if (!pci) {
                    LOG_ERR("Cannot get PCI");
                    pdu_destroy(pdu);
                    return -1;
            }

            LOG_DBG("TTL to start with is %d", dup_conf->initial_ttl_value);

            if (pci_ttl_set(pci, dup_conf->initial_ttl_value)) {
                    LOG_ERR("Could not set TTL");
                    pdu_destroy(pdu);
                    return -1;
            }
        }

        pdu_ser = pdu_serialize_ni(serdes, pdu, dup_conf, n1_port->blkcipher);
        if (!pdu_ser) {
                LOG_ERR("Error creating serialized PDU");
                pdu_destroy(pdu);
                return -1;
        }

        buffer = pdu_ser_buffer(pdu_ser);
        if (!buffer_is_ok(buffer)) {
                LOG_ERR("Buffer is not okay");
                pdu_ser_destroy(pdu_ser);
                return -1;
        }

        if (pdu_ser_buffer_disown(pdu_ser)) {
                LOG_ERR("Could not disown buffer");
                        pdu_ser_destroy(pdu_ser);
                        return -1;
        }

        pdu_ser_destroy(pdu_ser);

        sdu = sdu_create_buffer_with_ni(buffer);
        if (!sdu) {
                LOG_ERR("Error creating SDU from serialized PDU, "
                        "dropping PDU!");
                buffer_destroy(buffer);
                return -1;
        }

        LOG_DBG("Gonna send SDU to port-id %d", port_id);
        if (n1_ipcp->ops->sdu_write(n1_ipcp->data,port_id, sdu)) {
                LOG_ERR("Couldn't write SDU to N-1 IPCP");
                return -1;
        }

        return 0;
}

static void send_worker(unsigned long o)
{
        struct rmt *         rmt;
        struct rmt_n1_port * n1_port;
        struct hlist_node *  ntmp;
        int                  bucket;
        int                  reschedule = 0;
        int                  pdus_sent;
        struct rmt_ps *      ps;
        struct pdu  *        pdu = NULL;

        LOG_DBG("Send worker called");

        rmt = (struct rmt *) o;
        if (!rmt) {
                LOG_ERR("No instance passed to send worker !!!");
                return;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(rmt->base.ps),
                          struct rmt_ps,
                          base);
        if (!ps) {
                rcu_read_unlock();
                LOG_ERR("No ps in send_worker");
                return;
        }
        rcu_read_unlock();

        spin_lock(&rmt->n1_ports->lock);
        hash_for_each_safe(rmt->n1_ports->n1_ports,
                           bucket,
                           ntmp,
                           n1_port,
                           hlist) {
                if (!n1_port)
                        continue;

                pdus_sent = 0;
                spin_unlock(&rmt->n1_ports->lock);
                spin_lock(&n1_port->lock);
                if (n1_port->state == N1_PORT_STATE_DISABLED ||
                    atomic_read(&n1_port->n_sdus) == 0) {
                        spin_unlock(&n1_port->lock);
                        spin_lock(&rmt->n1_ports->lock);
                        LOG_DBG("Port state is DISABLED or empty");
                        continue;
                }

                /* there are more than 1 pdu in the queue, we will hace to
                 * reschedule */
                if (atomic_read(&n1_port->n_sdus) >= MAX_PDUS_SENT_PER_CYCLE)
                        reschedule++;

                rcu_read_lock();
                if (ps && ps->rmt_next_scheduled_policy_tx) {
                        do {
                                pdu = ps->rmt_next_scheduled_policy_tx(ps, n1_port);
                                if (pdu) {
                                        if (n1_port_write(rmt->serdes, n1_port, pdu))
                                                LOG_ERR("Could not write scheduled PDU in n1 port");
                                        pdus_sent++;
                                }
                        } while((pdus_sent < MAX_PDUS_SENT_PER_CYCLE) ||
                                (atomic_read(&n1_port->n_sdus) <= 0));
                }
                rcu_read_unlock();
                spin_unlock(&n1_port->lock);
                spin_lock(&rmt->n1_ports->lock);


        }
        spin_unlock(&rmt->n1_ports->lock);

        if (reschedule) {
                LOG_DBG("Sheduling policy will schedule again...");
                tasklet_hi_schedule(&rmt->egress_tasklet);
        }

        return;
}

int rmt_send_port_id(struct rmt * instance,
                     port_id_t    id,
                     struct pdu * pdu)
{
        struct n1pmap *      out_n1_ports;
        struct rmt_n1_port * out_n1_port;
        unsigned long        flags;
        struct rmt_ps *      ps;

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }
        if (!instance) {
                LOG_ERR("Bogus RMT passed");
                pdu_destroy(pdu);
                return -1;
        }

        if (!instance->n1_ports) {
                LOG_ERR("No n1_ports to push into");
                pdu_destroy(pdu);
                return -1;
        }

        out_n1_ports = instance->n1_ports;

        spin_lock_irqsave(&out_n1_ports->lock, flags);
        out_n1_port = n1pmap_find(out_n1_ports, id);
        if (!out_n1_port) {
                spin_unlock_irqrestore(&out_n1_ports->lock, flags);
                pdu_destroy(pdu);
                return -1;
        }
        spin_unlock_irqrestore(&out_n1_ports->lock, flags);

        spin_lock_irqsave(&out_n1_port->lock, flags);
        atomic_inc(&out_n1_port->n_sdus);
        /* Check if it is needed to schedule */
        if (atomic_read(&out_n1_port->n_sdus) > 1 || out_n1_port->state ==
                N1_PORT_STATE_DISABLED) {
                rcu_read_lock();
                ps = container_of(rcu_dereference(instance->base.ps), struct rmt_ps, base);
                if (ps) {
                        /* RMTQMonitorPolicy hook. */
                        if (ps->rmt_q_monitor_policy_tx) {
                                ps->rmt_q_monitor_policy_tx(ps, pdu, out_n1_port);
                        }

                        if (ps->rmt_enqueue_scheduling_policy_tx)
                                ps->rmt_enqueue_scheduling_policy_tx(ps,
                                                                     out_n1_port,
                                                                     pdu);
                }
                rcu_read_unlock();
                spin_unlock_irqrestore(&out_n1_port->lock, flags);
                tasklet_hi_schedule(&instance->egress_tasklet);
                return 0;
        }
        spin_unlock_irqrestore(&out_n1_port->lock, flags);
        return n1_port_write(instance->serdes, out_n1_port, pdu);
}
EXPORT_SYMBOL(rmt_send_port_id);

int rmt_send(struct rmt * instance,
             struct pci * pci,
             struct pdu * pdu)
{
        int i;

        if (!instance) {
                LOG_ERR("Bogus RMT passed");
                return -1;
        }
        if (!pdu) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }

        if (pft_nhop(instance->pft,
                     pci,
                     &(instance->cache.pids),
                     &(instance->cache.count))) {
                LOG_ERR("Cannot get the NHOP for this PDU");

                pdu_destroy(pdu);
                return -1;
        }

        if (instance->cache.count == 0) {
                LOG_WARN("No NHOP for this PDU ...");

                pdu_destroy(pdu);
                return 0;
        }

        /*
         * FIXME:
         *   pdu -> pci-> qos-id | cep_id_t -> connection -> qos-id (former)
         *   address + qos-id (pdu-fwd-t) -> port-id
         */

        for (i = 0; i < instance->cache.count; i++) {
                port_id_t    pid;
                struct pdu * p;

                pid = instance->cache.pids[i];

                if (i == instance->cache.count -1)
                        p = pdu;
                else
                        p = pdu_dup(pdu);

                LOG_DBG("Gonna send PDU to port-id: %d", pid);
                if (rmt_send_port_id(instance, pid, p))
                        LOG_ERR("Failed to send a PDU to port-id %d", pid);
        }

        return 0;
}
EXPORT_SYMBOL(rmt_send);

static struct dup_config_entry * find_dup_config(struct sdup_config * sdup_conf,
						 string_t * n_1_dif_name)
{
	struct dup_config * dup_pos;

	if (!sdup_conf)
		return NULL;

	list_for_each_entry(dup_pos, &sdup_conf->specific_dup_confs, next){
		if (string_cmp(dup_pos->entry->n_1_dif_name, n_1_dif_name)) {
			LOG_DBG("Returning specific SDU Protection config for port over N-1 DIF %s",
					n_1_dif_name);
			return dup_pos->entry;
		}
	}

	LOG_DBG("Returning default SDU Protection config for port over N-1 DIF %s",
			n_1_dif_name);
	return sdup_conf->default_dup_conf;
}

static int __queue_send_add(struct rmt * instance,
                            port_id_t    id,
                            struct ipcp_instance * n1_ipcp)
{
        struct rmt_n1_port * tmp;
        struct rmt_ps *      ps;
        const struct name * n_1_dif_name;
        struct dup_config_entry * dup_config;
        struct dup_config_entry * tmp_dup_config;

        n_1_dif_name = n1_ipcp->ops->dif_name(n1_ipcp->data);
        if (n_1_dif_name) {
        	tmp_dup_config = find_dup_config(instance->sdup_conf,
        				         n_1_dif_name->process_name);
        	if (tmp_dup_config) {
        		LOG_DBG("Found SDU Protection policy configuration, duplicating it");
        		dup_config = dup_config_entry_dup(tmp_dup_config);
        	} else {
        		dup_config = NULL;
        	}
        } else {
        	dup_config = NULL;
        }

        tmp = n1_port_create(id, n1_ipcp, dup_config);
        if (!tmp)
                return -1;

        hash_add(instance->n1_ports->n1_ports, &tmp->hlist, id);

        rcu_read_lock();
        ps = container_of(rcu_dereference(instance->base.ps), struct rmt_ps, base);
        if (ps && ps->rmt_scheduling_create_policy_tx) {
                if (ps->rmt_scheduling_create_policy_tx(ps, tmp)) {
                        LOG_ERR("Problems creating structs for scheduling "
                                "policy");
                        n1_port_destroy(tmp);
                        return -1;
                }
        }
        rcu_read_unlock();

        LOG_DBG("Added send queue to rmt instance %pK for port-id %d",
                instance, id);

        return 0;
}

static int rmt_n1_port_send_add(struct rmt * instance,
                                port_id_t    id,
                                struct ipcp_instance * n1_ipcp)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        if (!instance->n1_ports) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        if (n1pmap_find(instance->n1_ports, id)) {
                LOG_ERR("Queue already exists");
                return -1;
        }

        return __queue_send_add(instance, id, n1_ipcp);
}

int rmt_enable_port_id(struct rmt * instance,
                       port_id_t    id)
{
        struct rmt_n1_port * n1_port;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        if (!instance->n1_ports) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        spin_lock(&instance->n1_ports->lock);
        n1_port = n1pmap_find(instance->n1_ports, id);
        if (!n1_port) {
                spin_unlock(&instance->n1_ports->lock);
                LOG_ERR("No queue associated to this port-id, %d", id);
                return -1;
        }
        spin_unlock(&instance->n1_ports->lock);

        spin_lock(&n1_port->lock);
        if (n1_port->state != N1_PORT_STATE_DISABLED) {
                spin_unlock(&n1_port->lock);
                LOG_DBG("Nothing to do for port-id %d", id);
                return 0;
        }
        n1_port->state = N1_PORT_STATE_ENABLED;
        if (atomic_read(&n1_port->n_sdus) > 0)
                tasklet_hi_schedule(&instance->egress_tasklet);

        spin_unlock(&n1_port->lock);
        LOG_DBG("Changed state to ENABLED");
        return 0;
}
EXPORT_SYMBOL(rmt_enable_port_id);

int rmt_disable_port_id(struct rmt * instance,
                        port_id_t    id)
{
        struct rmt_n1_port * n1_port;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        if (!instance->n1_ports) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        spin_lock(&instance->n1_ports->lock);
        n1_port = n1pmap_find(instance->n1_ports, id);
        if (!n1_port) {
                spin_unlock(&instance->n1_ports->lock);
                LOG_ERR("No n1_port associated to this port-id, %d", id);
                return -1;
        }
        spin_unlock(&instance->n1_ports->lock);

        spin_lock(&n1_port->lock);
        if (n1_port->state == N1_PORT_STATE_DISABLED) {
                spin_unlock(&n1_port->lock);
                LOG_DBG("Nothing to do for port-id %d", id);
                return 0;
        }
        n1_port->state = N1_PORT_STATE_DISABLED;
        spin_unlock(&n1_port->lock);
        LOG_DBG("Changed state to DISABLED");

        return 0;
}
EXPORT_SYMBOL(rmt_disable_port_id);

static int rmt_n1_port_send_delete(struct rmt * instance,
                                 port_id_t    id)
{
        struct rmt_n1_port * n1_port;
        struct rmt_ps *      ps;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!instance->n1_ports) {
                LOG_ERR("Bogus egress instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        n1_port = n1pmap_find(instance->n1_ports, id);
        if (!n1_port) {
                LOG_ERR("Queue does not exist");
                return -1;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(instance->base.ps), struct rmt_ps, base);
        if (ps && ps->rmt_scheduling_destroy_policy_tx) {
                if (ps->rmt_scheduling_destroy_policy_tx(ps, n1_port))
                        return -1;
        }
        rcu_read_unlock();
        n1_port_destroy(n1_port);
        return 0;
}

int rmt_n1port_bind(struct rmt * instance,
                    port_id_t    id,
                    struct ipcp_instance * n1_ipcp)
{
        if (rmt_n1_port_send_add(instance, id, n1_ipcp))
                return -1;

        return 0;
}
EXPORT_SYMBOL(rmt_n1port_bind);

int rmt_n1port_unbind(struct rmt * instance,
                      port_id_t    id)
{
        int retval = 0;

        if (rmt_n1_port_send_delete(instance, id))
                retval = -1;

        return retval;
}
EXPORT_SYMBOL(rmt_n1port_unbind);

/* FIXME: This function is only used in testig and they are disabled */
#if 0
static struct pci * sdu_pci_copy(const struct sdu * sdu)
{
        if (!sdu_is_ok(sdu))
                return NULL;

        return pci_create_from(buffer_data_ro(sdu_buffer_ro(sdu)));
}
#endif
#if 0
static int process_mgmt_pdu(struct rmt * rmt,
                            port_id_t    port_id,
                            struct pdu * pdu)
{
        struct buffer * buffer;
        struct sdu *    sdu;

        ASSERT(rmt);
        ASSERT(is_port_id_ok(port_id));
        ASSERT(pdu_is_ok(pdu));

        buffer = pdu_buffer_get_rw(pdu);
        if (!buffer_is_ok(buffer)) {
                LOG_ERR("PDU has no buffer ???");
                return -1;
        }

        sdu = sdu_create_buffer_with(buffer);
        if (!sdu_is_ok(sdu)) {
                LOG_ERR("Cannot create SDU");
                pdu_destroy(pdu);
                return -1;
        }

        pdu_buffer_disown(pdu);
        pdu_destroy(pdu);

        ASSERT(rmt->parent);
        ASSERT(rmt->parent->ops);
        ASSERT(rmt->parent->ops->mgmt_sdu_post);

        return (rmt->parent->ops->mgmt_sdu_post(rmt->parent->data,
                                                port_id,
                                                sdu) ? -1 : 0);
}
#endif
static int process_mgmt_pdu_ni(struct rmt * rmt,
                               port_id_t    port_id,
                               struct pdu * pdu)
{
        struct buffer * buffer;
        struct sdu *    sdu;

        ASSERT(rmt);
        ASSERT(is_port_id_ok(port_id));
        ASSERT(pdu_is_ok(pdu));

        buffer = pdu_buffer_get_rw(pdu);
        if (!buffer_is_ok(buffer)) {
                LOG_ERR("PDU has no buffer ???");
                return -1;
        }

        sdu = sdu_create_buffer_with_ni(buffer);
        if (!sdu_is_ok(sdu)) {
                LOG_ERR("Cannot create SDU");
                pdu_destroy(pdu);
                return -1;
        }

        pdu_buffer_disown(pdu);
        pdu_destroy(pdu);

        ASSERT(rmt->parent);
        ASSERT(rmt->parent->ops);
        ASSERT(rmt->parent->ops->mgmt_sdu_post);

        return (rmt->parent->ops->mgmt_sdu_post(rmt->parent->data,
                                                port_id,
                                                sdu) ? -1 : 0);
}

static int process_dt_pdu(struct rmt * rmt,
                          port_id_t    port_id,
                          struct pdu * pdu)
{
        address_t  dst_addr;
        cep_id_t   c;
        pdu_type_t pdu_type;

        ASSERT(rmt);
        ASSERT(is_port_id_ok(port_id));
        ASSERT(pdu_is_ok(pdu));

        /* (FUTURE) Address and qos-id are the same, do a single match only */

        dst_addr = pci_destination(pdu_pci_get_ro(pdu));
        if (!is_address_ok(dst_addr)) {
                LOG_ERR("PDU has Wrong destination address");
                pdu_destroy(pdu);
                return -1;
        }

        pdu_type = pci_type(pdu_pci_get_ro(pdu));

        if (pdu_type == PDU_TYPE_MGMT) {
                LOG_ERR("MGMT should not be here");
                pdu_destroy(pdu);
                return -1;
        }
        c = pci_cep_destination(pdu_pci_get_ro(pdu));
        if (!is_cep_id_ok(c)) {
                LOG_ERR("Wrong CEP-id in PDU");
                pdu_destroy(pdu);
                return -1;
        }

        if (efcp_container_receive(rmt->efcpc, c, pdu)) {
                LOG_ERR("EFCP container problems");
                return -1;
        }

        LOG_DBG("process_dt_pdu internally finished");
        return 0;
}

int rmt_receive(struct rmt * rmt,
                struct sdu * sdu,
                port_id_t    from)
{
        pdu_type_t       pdu_type;
        struct pci *     pci;
        struct pdu_ser * pdu_ser;
        struct pdu *     pdu;
        address_t        dst_addr;
        qos_id_t         qos_id;
        struct serdes *  serdes;
        struct buffer *  buf;
        struct rmt_n1_port * n1_port;

        if (!sdu_is_ok(sdu)) {
                LOG_ERR("Bogus SDU passed");
                return -1;
        }
        if (!rmt) {
                LOG_ERR("No RMT passed");
                sdu_destroy(sdu);
                return -1;
        }
        if (!is_port_id_ok(from)) {
                LOG_ERR("Wrong port-id %d", from);
                sdu_destroy(sdu);
                return -1;
        }
        n1_port = n1pmap_find(rmt->n1_ports, from);

        buf = sdu_buffer_rw(sdu);
        if (!buf) {
                LOG_DBG("No buffer present");
                sdu_destroy(sdu);
                return -1;
        }

        if (sdu_buffer_disown(sdu)) {
                LOG_DBG("Could not disown SDU");
                sdu_destroy(sdu);
                return -1;
        }

        sdu_destroy(sdu);

        pdu_ser = pdu_ser_create_buffer_with_ni(buf);
        if (!pdu_ser) {
                LOG_DBG("No ser PDU to work with");
                buffer_destroy(buf);
                return -1;
        }

        serdes = rmt->serdes;
        ASSERT(serdes);

        pdu = pdu_deserialize_ni(serdes, pdu_ser,
                                 n1_port->dup_config, n1_port->blkcipher);
        if (!pdu) {
                LOG_ERR("Failed to deserialize PDU!");
                pdu_ser_destroy(pdu_ser);
                return -1;
        }

        pci = pdu_pci_get_rw(pdu);
        if (!pci) {
                LOG_ERR("No PCI to work with, dropping SDU!");
                pdu_destroy(pdu);
                return -1;
        }

        ASSERT(pdu_is_ok(pdu));

        pdu_type = pci_type(pci);
        dst_addr = pci_destination(pci);
        qos_id   = pci_qos_id(pci);
        if (!pdu_type_is_ok(pdu_type) ||
            !is_address_ok(dst_addr)  ||
            !is_qos_id_ok(qos_id)) {
            LOG_ERR("Wrong PDU type, dst address or qos_id,"
                    " dropping SDU! %u, %u, %u",
                    pdu_type, dst_addr, qos_id);
                pdu_destroy(pdu);
                return -1;
        }

        /* pdu is not for me */
        if (rmt->address != dst_addr) {
                if (!dst_addr) {
                        return process_mgmt_pdu_ni(rmt, from, pdu);
                } else {
                        /* Forward PDU */
                        /*NOTE: we could reuse the serialized pdu when
                         * forwarding */
                        return rmt_send(rmt,
                                        pci,
                                        pdu);
                }
        } else {
                /* pdu is for me */
                switch (pdu_type) {
                case PDU_TYPE_MGMT:
                        return process_mgmt_pdu_ni(rmt, from, pdu);

                case PDU_TYPE_CACK:
                case PDU_TYPE_SACK:
                case PDU_TYPE_NACK:
                case PDU_TYPE_FC:
                case PDU_TYPE_ACK:
                case PDU_TYPE_ACK_AND_FC:
                case PDU_TYPE_DT:
                /*
                 * (FUTURE)
                 *
                 * enqueue PDU in pdus_dt[dest-addr, qos-id]
                 * don't process it now ...
                 */
                        return process_dt_pdu(rmt, from, pdu);

               default:
                        LOG_ERR("Unknown PDU type %d", pdu_type);
                        pdu_destroy(pdu);
                        return -1;
                }
       }
}
EXPORT_SYMBOL(rmt_receive);

struct rmt * rmt_create(struct ipcp_instance *  parent,
                        struct kfa *            kfa,
                        struct efcp_container * efcpc)
{
        struct rmt * tmp;

        if (!parent || !kfa || !efcpc) {
                LOG_ERR("Bogus input parameters");
                return NULL;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->address   = address_bad();
        tmp->parent    = parent;
        tmp->kfa       = kfa;
        tmp->efcpc     = efcpc;
        tmp->sdup_conf = NULL;
        tmp->pft       = pft_create();
        if (!tmp->pft)
                goto fail;

        tmp->n1_ports = n1pmap_create();
        if (!tmp->n1_ports || pft_cache_init(&tmp->cache))
                goto fail;

        tasklet_init(&tmp->egress_tasklet,
                     send_worker,
                     (unsigned long) tmp);

        /* Try to select the default policy set factory. */
        rina_component_init(&tmp->base);
        if (rmt_select_policy_set(tmp, "", RINA_PS_DEFAULT_NAME)) {
                goto fail;
        }

        LOG_DBG("Instance %pK initialized successfully", tmp);
        return tmp;
fail:
        rmt_destroy(tmp);
        return NULL;
}
EXPORT_SYMBOL(rmt_create);

/* FIXME: To be rearranged */
static bool is_rmt_pft_ok(struct rmt * instance)
{ return (instance && instance->pft) ? true : false; }

int rmt_pft_add(struct rmt *       instance,
		struct modpdufwd_entry * entry)
{
        return is_rmt_pft_ok(instance) ? pft_add(instance->pft,
						 entry) : -1;
}
EXPORT_SYMBOL(rmt_pft_add);

int rmt_pft_remove(struct rmt *       instance,
		   struct modpdufwd_entry *entry)
{
        return is_rmt_pft_ok(instance) ? pft_remove(instance->pft,
						    entry) : -1;
}
EXPORT_SYMBOL(rmt_pft_remove);

int rmt_pft_dump(struct rmt *       instance,
                 struct list_head * entries)
{
        return is_rmt_pft_ok(instance) ? pft_dump(instance->pft,
                                                  entries) : -1;
}
EXPORT_SYMBOL(rmt_pft_dump);

int rmt_pft_flush(struct rmt * instance)
{ return is_rmt_pft_ok(instance) ? pft_flush(instance->pft) : -1; }
EXPORT_SYMBOL(rmt_pft_flush);

int rmt_ps_publish(struct ps_factory * factory)
{
        if (factory == NULL) {
                LOG_ERR("%s: NULL factory", __func__);
                return -1;
        }

        return ps_publish(&policy_sets, factory);
}
EXPORT_SYMBOL(rmt_ps_publish);

int rmt_ps_unpublish(const char * name)
{ return ps_unpublish(&policy_sets, name); }
EXPORT_SYMBOL(rmt_ps_unpublish);

int rmt_enable_encryption(struct rmt *    instance,
			  bool 	    	  enable_encryption,
			  bool      	  enable_decryption,
			  struct buffer * encrypt_key,
			  port_id_t 	  port_id)
{
	struct rmt_n1_port * rmt_port;
	unsigned long        flags;

	if (!instance) {
		LOG_ERR("Bogus RMT instance passed");
		return -1;
	}

	if (!encrypt_key) {
		LOG_ERR("Bogus encryption key passed");
		return -1;
	}

	if (!enable_decryption && !enable_encryption) {
		LOG_ERR("Neither encryption nor decryption is being enabled");
		return -1;
	}

	rmt_port = n1pmap_find(instance->n1_ports, port_id);
	if (!rmt_port) {
		LOG_ERR("Could not find N-1 port %d", port_id);
		return -1;
	}

	if (!rmt_port->dup_config) {
		LOG_ERR("SDU Protection for N-1 port %d is NULL", port_id);
		return -1;
	}

	if (!rmt_port->dup_config->encryption_policy) {
		LOG_ERR("Encryption policy for N-1 port %d is NULL", port_id);
		return -1;
	}

	if (!rmt_port->blkcipher) {
		LOG_ERR("Block cipher is not set for N-1 port %d", port_id);
		return -1;
	}

	spin_lock_irqsave(&rmt_port->lock, flags);
	if (!rmt_port->dup_config->enable_decryption &&
			!rmt_port->dup_config->enable_encryption) {
		/* Need to set key. FIXME: Move this to policy specific code */
		if (crypto_blkcipher_setkey(rmt_port->blkcipher,
					    buffer_data_rw(encrypt_key),
					    buffer_length(encrypt_key))) {
			LOG_ERR("Could not set encryption key for N-1 port %d", port_id);
			spin_unlock_irqrestore(&rmt_port->lock, flags);
			return -1;
		}
	}

	rmt_port->dup_config->key = encrypt_key;
	if (!rmt_port->dup_config->enable_decryption) {
		rmt_port->dup_config->enable_decryption = enable_decryption;
	}
	if (!rmt_port->dup_config->enable_encryption) {
		rmt_port->dup_config->enable_encryption = enable_encryption;
	}
	LOG_DBG("Encryption enabled state: %d", enable_encryption);
	LOG_DBG("Decryption enabled state: %d", enable_decryption);
	spin_unlock_irqrestore(&rmt_port->lock, flags);

	return 0;
}
EXPORT_SYMBOL(rmt_enable_encryption);
