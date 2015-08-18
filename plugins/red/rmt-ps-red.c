/*
 * RMT RED PS for Programmable Congestion Control
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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
#include <linux/module.h>
#include <linux/string.h>
#include <linux/bitmap.h>
#include <net/pkt_sched.h>
#include <net/red.h>

#define RINA_PREFIX "rmt-tcp-red"

#include "logs.h"
#include "debug.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "rmt-ps-common.h"
#include "policies.h"

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

#define MIN_TH_P_DEFAULT 2
#define MAX_TH_P_DEFAULT 5
#define WP_P_DEFAULT 2

struct red_rmt_queue {
        struct rfifo *    queue;
        port_id_t         port_id;

	struct red_parms  parms;
	struct red_vars   vars;
	struct red_stats  stats;

        struct hlist_node hlist;
};

struct red_rmt_ps_data {
        DECLARE_HASHTABLE(queues, RMT_PS_HASHSIZE);
	struct tc_red_qopt conf_data;
	u8 * stab;
	u32  max_P;
};

static struct red_rmt_queue * red_queue_create(port_id_t          port_id,
					       struct tc_red_qopt cfg,
					       u8 *               stab,
					       u32                max_P)
{
	struct red_rmt_queue * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;
        tmp->queue = rfifo_create_ni();
        if (!tmp->queue) {
                rkfree(tmp);
                return NULL;
        }

        tmp->port_id = port_id;

	red_set_parms(&tmp->parms, cfg.qth_min, cfg.qth_max, cfg.Wlog,
		      cfg.Plog, cfg.Scell_log, stab, max_P);

	red_set_vars(&tmp->vars);

	red_start_of_idle_period(&tmp->vars);

        INIT_HLIST_NODE(&tmp->hlist);

        return tmp;
}

static int red_rmt_queue_destroy(struct red_rmt_queue * q)
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

struct red_rmt_queue * red_rmt_queue_find(struct red_rmt_ps_data * data,
                                          port_id_t                port_id)
{
        struct red_rmt_queue * entry;
        const struct hlist_head *  head;

        ASSERT(data);

        head = &data->queues[rmap_hash(data->queues, port_id)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->port_id == port_id)
                        return entry;
        }

        return NULL;
}

static struct pdu *
red_rmt_next_scheduled_policy_tx(struct rmt_ps *      ps,
                                 struct rmt_n1_port * port)
{
        struct red_rmt_queue *   q;
        struct red_rmt_ps_data * data = ps->priv;
        struct pdu *             ret_pdu;

        if (!ps || !port || !data) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_next_scheduled_policy_tx");
                return NULL;
        }

        q = red_rmt_queue_find(data, port->port_id);
        if (!q) {
                LOG_ERR("Could not find queue for n1_port %u",
                        port->port_id);
                return NULL;
        }

        ret_pdu = rfifo_pop(q->queue);
        if (!ret_pdu) {
                LOG_ERR("Could not dequeue scheduled pdu");
                ret_pdu = NULL;
	        if (!red_is_idling(&q->vars))
			red_start_of_idle_period(&q->vars);
        }
	if (rfifo_is_empty(q->queue) == 0 &&
	    !red_is_idling(&q->vars))
		red_start_of_idle_period(&q->vars);
        return ret_pdu;
}

static int red_rmt_enqueue_scheduling_policy_tx(struct rmt_ps *      ps,
                                                struct rmt_n1_port * port,
                                                struct pdu *         pdu)
{
        struct red_rmt_queue *   q;
        struct red_rmt_ps_data * data = ps->priv;
        struct pci *                 pci;
        unsigned long                pci_flags;

        if (!ps || !port || !pdu || !data) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_enqueu_scheduling_policy_tx");
                return -1;
        }

        q = red_rmt_queue_find(data, port->port_id);
        if (!q) {
                LOG_ERR("Could not find queue for n1_port %u",
                        port->port_id);
                pdu_destroy(pdu);
                return -1;
        }

	/* Compute average queue usage (see RED) */
	q->vars.qavg = red_calc_qavg(&q->parms, &q->vars, rfifo_length(q->queue));
	LOG_DBG("qavg':  %lu, qlen: %lu", q->vars.qavg, rfifo_length(q->queue));
	/* NOTE: check this! */
	if (red_is_idling(&q->vars))
		red_end_of_idle_period(&q->vars);

	switch (red_action(&q->parms, &q->vars, q->vars.qavg)) {
	case RED_DONT_MARK:
		LOG_DBG("RED_DONT_MARK");
		break;

	case RED_PROB_MARK:
		LOG_DBG("RED_PROB_MARK");
		/*q->red_stats.prob_drop++;*/
		q->stats.prob_mark++;
		/* mark ECN bit */
                pci = pdu_pci_get_rw(pdu);
                pci_flags = pci_flags_get(pci);
                pci_flags_set(pci, pci_flags |= PDU_FLAGS_EXPLICIT_CONGESTION);
		break;

	case RED_HARD_MARK:
		LOG_DBG("RED_HARD_MARK");
		q->stats.forced_mark++;
		goto congestion_drop;
		break;
	}

	rfifo_push_ni(q->queue, pdu);
        return 0;

congestion_drop:

        pdu_destroy(pdu);
        atomic_dec(&port->n_sdus);
        LOG_DBG("PDU dropped, max_th passed...");
        return 0;
}

static int red_rmt_scheduling_create_policy_tx(struct rmt_ps *      ps,
                                               struct rmt_n1_port * port)
{
        struct red_rmt_queue *   q;
        struct red_rmt_ps_data * data;

        if (!ps || !port || !ps->priv) {
                LOG_ERR("Wrong input parameters for "
                        "red_rmt_scheduling_create_policy");
                return -1;
        }

        data = ps->priv;

        q = red_queue_create(port->port_id,
	                     data->conf_data,
			     data->stab,
			     data->max_P);
        if (!q) {
                LOG_ERR("Could not create queue for n1_port %u",
                        port->port_id);
                return -1;
        }

        hash_add(data->queues, &q->hlist, port->port_id);

        LOG_DBG("Structures for scheduling policies created...");
        return 0;
}
static int red_rmt_scheduling_destroy_policy_tx(struct rmt_ps *      ps,
                                                struct rmt_n1_port * port)
{
        struct red_rmt_ps_data * data;
        struct red_rmt_queue *   q;

        if (!ps || !port || !ps->priv) {
                LOG_ERR("Wrong input parameters for "
                        "red_rmt_scheduling_destroy_policy");
                return -1;
        }

        data = ps->priv;
        ASSERT(data);

        q = red_rmt_queue_find(data, port->port_id);
        if (q) return red_rmt_queue_destroy(q);

        return -1;
}

static int red_rmt_ps_set_policy_set_param(struct ps_base * bps,
                                           const char    * name,
                                           const char    * value)
{
        struct rmt_ps * ps = container_of(bps, struct rmt_ps, base);
        struct red_rmt_ps_data * data = ps->priv;
        int bool_value;
        int ret;

        if (!ps || ! data) {
                LOG_ERR("Wrong PS or parameters to set");
                return -1;
        }

        if (!name) {
                LOG_ERR("Null parameter name");
                return -1;
        }

        if (!value) {
                LOG_ERR("Null parameter value");
                return -1;
        }

        if (strcmp(name, "qmax_p") == 0) {
                ret = kstrtoint(value, 10, &bool_value);
                if (!ret)
                        data->conf_data.limit = bool_value;
        }
        if (strcmp(name, "qth_min_p") == 0) {
                ret = kstrtoint(value, 10, &bool_value);
                if (!ret)
                        data->conf_data.qth_min = bool_value;
        }
        if (strcmp(name, "qth_max_p") == 0) {
                ret = kstrtoint(value, 10, &bool_value);
                if (!ret)
                        data->conf_data.qth_max = bool_value;
        }
        if (strcmp(name, "Wlog_p") == 0) {
                ret = kstrtoint(value, 10, &bool_value);
                if (!ret)
                        data->conf_data.Wlog = bool_value;
        }
        if (strcmp(name, "Plog_p") == 0) {
                ret = kstrtoint(value, 10, &bool_value);
                if (!ret)
                        data->conf_data.Plog = bool_value;
        }
        if (strcmp(name, "Scell_log_p") == 0) {
                ret = kstrtoint(value, 10, &bool_value);
                if (!ret)
                        data->conf_data.Scell_log = bool_value;
        }
	if (strcmp(name, "stab_address_p") == 0) {
		void __user * user_pointer;
		uintptr_t  user_address;
		int ret;
		int i;
		size_t size = 256;
		u8 * stab_table = rkmalloc(sizeof(*stab_table)*size, GFP_KERNEL);
		kstrtoul(value, 16, (unsigned long *) &user_address);
		user_pointer = (void __user *) user_address;
		ret = copy_from_user(stab_table, (const void __user *) user_pointer, size*sizeof(u8));
		if (ret !=0) {
			LOG_ERR("Stab table for RMT's RED PS was not fully copied, missing %d out of %u bytes",
				ret, (unsigned int) size);
			LOG_ERR("Padding with 0s");
			for (i = 0; i< size; i++)
				stab_table[i] = 0;
		}
		data->stab = stab_table;
	}
        return 0;
}

static struct ps_base *
rmt_ps_red_create(struct rina_component * component)
{
        struct rmt * rmt = rmt_from_component(component);
        struct rmt_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        struct red_rmt_ps_data * data = rkzalloc(sizeof(*data), GFP_KERNEL);
	struct policy_parm * ps_param;
	struct rmt_config * rmt_cfg;

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = red_rmt_ps_set_policy_set_param;
        ps->dm = rmt;

	ps->priv = data;

	rmt_cfg = rmt_config_get(rmt);
	ps_param = policy_param_find(rmt_cfg->policy_set, "qmax_p");
	if (!ps_param) {
		LOG_WARN("No PS param qmax_p");
	}
	red_rmt_ps_set_policy_set_param(&ps->base,
					policy_param_name(ps_param),
					policy_param_value(ps_param));
	ps_param = policy_param_find(rmt_cfg->policy_set, "qth_min_p");
	if (!ps_param) {
		LOG_WARN("No PS param qth_min_p");
	}
	red_rmt_ps_set_policy_set_param(&ps->base,
					policy_param_name(ps_param),
					policy_param_value(ps_param));
	ps_param = policy_param_find(rmt_cfg->policy_set, "qth_max_p");
	if (!ps_param) {
		LOG_WARN("No PS param qth_max_p");
	}
	red_rmt_ps_set_policy_set_param(&ps->base,
					policy_param_name(ps_param),
					policy_param_value(ps_param));
	ps_param = policy_param_find(rmt_cfg->policy_set, "Wlog_p");
	if (!ps_param) {
		LOG_WARN("No PS param Wlog_p");
	}
	red_rmt_ps_set_policy_set_param(&ps->base,
					policy_param_name(ps_param),
					policy_param_value(ps_param));
	ps_param = policy_param_find(rmt_cfg->policy_set, "Plog_p");
	if (!ps_param) {
		LOG_WARN("No PS param Plog_p");
	}
	red_rmt_ps_set_policy_set_param(&ps->base,
					policy_param_name(ps_param),
					policy_param_value(ps_param));
	ps_param = policy_param_find(rmt_cfg->policy_set, "Scell_log_p");
	if (!ps_param) {
		LOG_WARN("No PS param Scell_log_p");
	}
	red_rmt_ps_set_policy_set_param(&ps->base,
					policy_param_name(ps_param),
					policy_param_value(ps_param));
	ps_param = policy_param_find(rmt_cfg->policy_set, "stab_address_p");
	if (!ps_param) {
		LOG_WARN("No PS param stab_address");
	}
	red_rmt_ps_set_policy_set_param(&ps->base,
					policy_param_name(ps_param),
					policy_param_value(ps_param));

        ps->max_q_policy_tx = NULL;
        ps->max_q_policy_rx = NULL;
        ps->rmt_q_monitor_policy_tx_enq = NULL;
        ps->rmt_q_monitor_policy_tx_deq = NULL;
        ps->rmt_q_monitor_policy_rx = NULL;
        ps->rmt_next_scheduled_policy_tx = red_rmt_next_scheduled_policy_tx;
        ps->rmt_enqueue_scheduling_policy_tx = red_rmt_enqueue_scheduling_policy_tx;
        ps->rmt_scheduling_create_policy_tx  = red_rmt_scheduling_create_policy_tx;
        ps->rmt_scheduling_destroy_policy_tx = red_rmt_scheduling_destroy_policy_tx;

        return &ps->base;
}

static void rmt_ps_red_destroy(struct ps_base * bps)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
        struct red_rmt_ps_data * data;
        int bucket;
        struct red_rmt_queue * entry;
        struct hlist_node * tmp;

        data = ps->priv;

        if (bps) {
                if (data) {
                        hash_for_each_safe(data->queues, bucket, tmp, entry,
                                           hlist) {
                                ASSERT(entry);
                                red_rmt_queue_destroy(entry);
                        }
			if (data->stab) rkfree(data->stab);
                        rkfree(data);
                }
                rkfree(ps);
        }
}

struct ps_factory rmt_factory = {
        .owner   = THIS_MODULE,
        .create  = rmt_ps_red_create,
        .destroy = rmt_ps_red_destroy,
};
