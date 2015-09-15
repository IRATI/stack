/*
 * Congestion avoidance Scheme based on Jain's report PS
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *
 * This program is free software; you can dummyistribute it and/or modify
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
#include <linux/time.h>

#define RINA_PREFIX "cas-rmt-ps"

#include "logs.h"
#include "debug.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "rmt-ps-common.h"
#include "pci.h"

#define  N1_CYCLE_DURATION 100
#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

struct reg_cycle_t {
        struct timespec t_start;
        struct timespec t_last_start;
        struct timespec t_end;
        ulong           sum_area;
        ulong           avg_len;
};

struct cas_rmt_queue {
        struct rfifo *    queue;
        port_id_t         port_id;
        unsigned int      max_q;
        struct {
                struct reg_cycle_t prev_cycle;
                struct reg_cycle_t cur_cycle;
        } reg_cycles;
	bool 		 first_run;
        struct hlist_node hlist;
};

struct cas_rmt_ps_data {
        DECLARE_HASHTABLE(queues, RMT_PS_HASHSIZE);
};

static struct cas_rmt_queue * cas_queue_create(port_id_t port_id)
{
        struct cas_rmt_queue * tmp;
        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;
        tmp->queue = rfifo_create_ni();
        if (!tmp->queue) {
                rkfree(tmp);
                return NULL;
        }

        tmp->port_id                                    = port_id;
        tmp->reg_cycles.prev_cycle.t_start.tv_sec       = 0;
        tmp->reg_cycles.prev_cycle.t_start.tv_nsec      = 0;
        tmp->reg_cycles.prev_cycle.t_last_start.tv_sec  = 0;
        tmp->reg_cycles.prev_cycle.t_last_start.tv_nsec = 0;
        tmp->reg_cycles.prev_cycle.t_end.tv_sec         = 0;
        tmp->reg_cycles.prev_cycle.t_end.tv_nsec        = 0;
        tmp->reg_cycles.prev_cycle.sum_area             = 0;
        tmp->reg_cycles.prev_cycle.avg_len              = 0;
        tmp->reg_cycles.cur_cycle                       = tmp->reg_cycles.prev_cycle;
	tmp->first_run                                  = true;

        INIT_HLIST_NODE(&tmp->hlist);

        return tmp;
}

static int cas_rmt_queue_destroy(struct cas_rmt_queue * q)
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

struct cas_rmt_queue * cas_rmt_queue_find(struct cas_rmt_ps_data * data,
                                          port_id_t                port_id)
{
        struct cas_rmt_queue *    entry;
        const struct hlist_head * head;

        ASSERT(data);

        head = &data->queues[rmap_hash(data->queues, port_id)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->port_id == port_id)
                        return entry;
        }

        return NULL;
}


static void cas_max_q_policy_tx(struct rmt_ps *      ps,
                                struct pdu *         pdu,
                                struct rmt_n1_port * port)
{  }

static void cas_max_q_policy_rx(struct rmt_ps *      ps,
                                struct sdu *         sdu,
                                struct rmt_n1_port * port)
{  }

static void cas_rmt_q_monitor_policy_tx_common(struct rmt_ps *      ps,
                                               struct pdu *         pdu,
                                               struct rmt_n1_port * port,
					       bool                 enqueue)
{
        struct cas_rmt_queue *   q;
        struct cas_rmt_ps_data * data;
        ssize_t                  cur_qlen;
        struct reg_cycle_t *     prev_cycle, * cur_cycle;
        struct pci *             pci;
        pdu_flags_t              pci_flags;
        struct timespec          t_sub;
        s64			 t_sub_ns;

        ASSERT(ps);
        ASSERT(ps->priv);
        ASSERT(pdu);
        ASSERT(port);

        data = ps->priv;

        q = cas_rmt_queue_find(data, port->port_id);
        if (!q) {
                LOG_ERR("Monitoring: could not find CAS queue for N-1 port %u",
                        port->port_id);
                return;
        }

        cur_qlen   = rfifo_length(q->queue);
        prev_cycle = &q->reg_cycles.prev_cycle;
        cur_cycle  = &q->reg_cycles.cur_cycle;

        /* new cycle or end cycle */
        if (cur_qlen == 0) {
                /* new cycle */
                if (enqueue) {
                        LOG_DBG("new cycle");
                        *prev_cycle = *cur_cycle;
			if (q->first_run) {
				getnstimeofday(&cur_cycle->t_start);
				getnstimeofday(&prev_cycle->t_start);
				prev_cycle->t_start.tv_nsec -= N1_CYCLE_DURATION;
				q->first_run = false;
			}

                        getnstimeofday(&cur_cycle->t_start);
                        cur_cycle->t_last_start = cur_cycle->t_start;
                        cur_cycle->t_end        = cur_cycle->t_start;
                        cur_cycle->sum_area     = 0;
                /* end cycle */
                } else {
                        LOG_DBG("end cycle");
                        getnstimeofday(&cur_cycle->t_end);
                        t_sub = timespec_sub(cur_cycle->t_end,
                                             cur_cycle->t_last_start);
                        cur_cycle->sum_area  +=
                                (ulong) atomic_read(&port->n_sdus) *          \
                                timespec_to_ns(&t_sub);
                        cur_cycle->t_last_start = cur_cycle->t_end;
                        LOG_DBG("n_sdus: %u",
                                (uint_t) atomic_read(&port->n_sdus));
                        LOG_DBG("t_end - t_last_start = %llu - %llu = %llu",
                                timespec_to_ns(&cur_cycle->t_end),
                                timespec_to_ns(&cur_cycle->t_last_start),
                                timespec_to_ns(&t_sub));
                        LOG_DBG("sum_area: %u",cur_cycle->sum_area);
                }
        } else {
                LOG_DBG("In middle cycle");
                cur_cycle->t_last_start = cur_cycle->t_end;
                getnstimeofday(&cur_cycle->t_end);
                t_sub = timespec_sub(cur_cycle->t_end, cur_cycle->t_last_start);
                LOG_DBG("Prev sum area: %u", cur_cycle->sum_area);
                cur_cycle->sum_area +=(ulong) atomic_read(&port->n_sdus) *    \
                                      timespec_to_ns(&t_sub);
                LOG_DBG("n_sdus: %u", (uint_t) atomic_read(&port->n_sdus));
                LOG_DBG("t_end - t_last_start = %llu - %llu = %llu",
                        timespec_to_ns(&cur_cycle->t_end),
                        timespec_to_ns(&cur_cycle->t_last_start),
                        timespec_to_ns(&t_sub));
                LOG_DBG("sum_area: %lu",cur_cycle->sum_area);
        }

        LOG_DBG(" Avg len inputs");
        LOG_DBG("cur_cycle->avg_len = (cur_cycle->sum_area + prev_cycle->sum_area) / (cur_cycle->t_end - prev_cycle->t_start) =>");
        LOG_DBG("%u = (%u + %u) / (%llu - %llu)",
                (uint_t) atomic_read(&port->n_sdus),
                cur_cycle->sum_area,
                prev_cycle->sum_area,
                timespec_to_ns(&cur_cycle->t_end),
                timespec_to_ns(&prev_cycle->t_start));


        if (timespec_equal(&cur_cycle->t_end, &prev_cycle->t_start)) {
                LOG_WARN("Division by 0 avoided..");
        }
        t_sub = timespec_sub(cur_cycle->t_end, prev_cycle->t_start);
	t_sub_ns = timespec_to_ns(&t_sub);
        cur_cycle->avg_len = (cur_cycle->sum_area + prev_cycle->sum_area);

	/* This raise a warning: WARNING: "__divdi3" undefined. For some reason
	 * it can not divide by a s64 variable but can do it by an insigned
	 * long, both of size 64bits */
	if (t_sub_ns < 0)
		LOG_ERR("Time delta is < 0!");

	cur_cycle->avg_len /=  (ulong) abs64(t_sub_ns);

        LOG_DBG("The length for N-1 port %u just calculated is: %lu",
                port->port_id, cur_cycle->avg_len);

        if (cur_cycle->avg_len >= 1) {
                LOG_DBG("Congestion detected in port %u, marking packets...",
                         port->port_id);
                pci = pdu_pci_get_rw(pdu);
                if (!pci) {
                        LOG_ERR("No PCI to mark in this PDU...");
                        return;
                }
                pci_flags = pci_flags_get(pci);
                pci_flags_set(pci, pci_flags |= PDU_FLAGS_EXPLICIT_CONGESTION);
                LOG_DBG("ECN bit marked");
        }

        return;

}

static void cas_rmt_q_monitor_policy_tx_enq(struct rmt_ps *      ps,
                                            struct pdu *         pdu,
                                            struct rmt_n1_port * port)
{ return cas_rmt_q_monitor_policy_tx_common(ps, pdu, port, true); }

static void cas_rmt_q_monitor_policy_tx_deq(struct rmt_ps *      ps,
                                            struct pdu *         pdu,
                                            struct rmt_n1_port * port)
{ return cas_rmt_q_monitor_policy_tx_common(ps, pdu, port, false); }

static struct pdu *
cas_rmt_next_scheduled_policy_tx(struct rmt_ps *      ps,
                                 struct rmt_n1_port * port)
{
        struct cas_rmt_queue *   q;
        struct cas_rmt_ps_data * data = ps->priv;
        struct pdu *             ret_pdu;

        if (!ps || !port || !data) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_next_scheduled_policy_tx");
                return NULL;
        }

        q = cas_rmt_queue_find(data, port->port_id);
        if (!q) {
                LOG_ERR("Could not find queue for n1_port %u",
                        port->port_id);
                return NULL;
        }

        ret_pdu = rfifo_pop(q->queue);
        if (!ret_pdu) {
                LOG_ERR("Could not dequeue scheduled pdu");
                return NULL;
        }

        return ret_pdu;
}

static int cas_rmt_enqueue_scheduling_policy_tx(struct rmt_ps *      ps,
                                                struct rmt_n1_port * port,
                                                struct pdu *         pdu)
{
        struct cas_rmt_queue *   q;
        struct cas_rmt_ps_data * data = ps->priv;

        if (!ps || !port || !pdu || !data) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_enqueu_scheduling_policy_tx");
                return -1;
        }

        /* NOTE: The policy is called with the n1_port lock taken */
        q = cas_rmt_queue_find(data, port->port_id);
        if (!q) {
                LOG_ERR("Could not find queue for n1_port %u",
                        port->port_id);
                pdu_destroy(pdu);
                return -1;
        }

        rfifo_push_ni(q->queue, pdu);
        return 0;
}

static int cas_rmt_requeue_scheduling_policy_tx(struct rmt_ps *      ps,
                                        	struct rmt_n1_port * port,
                                        	struct pdu *         pdu)
{
        struct cas_rmt_queue *   q;
        struct cas_rmt_ps_data * data = ps->priv;

        if (!ps || !port || !pdu) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_requeu_scheduling_policy_tx");
                return -1;
        }

        /* NOTE: The policy is called with the n1_port lock taken */
        q = cas_rmt_queue_find(data, port->port_id);
        if (!q) {
                LOG_ERR("Could not find queue for n1_port %u",
                        port->port_id);
                pdu_destroy(pdu);
                return -1;
        }

        rfifo_head_push_ni(q->queue, pdu);
        return 0;
}

static int cas_rmt_scheduling_create_policy_tx(struct rmt_ps *      ps,
                                               struct rmt_n1_port * port)
{
        struct cas_rmt_queue *   q;
        struct cas_rmt_ps_data * data;

        if (!ps || !port || !ps->priv) {
                LOG_ERR("Wrong input parameters for "
                        "cas_rmt_scheduling_create_policy");
                return -1;
        }

        data = ps->priv;

        q = cas_queue_create(port->port_id);
        if (!q) {
                LOG_ERR("Could not create queue for n1_port %u",
                        port->port_id);
                return -1;
        }

        getnstimeofday(&q->reg_cycles.prev_cycle.t_start);
        getnstimeofday(&q->reg_cycles.prev_cycle.t_last_start);
        getnstimeofday(&q->reg_cycles.prev_cycle.t_end);
        q->reg_cycles.prev_cycle.sum_area = 0;
        q->reg_cycles.prev_cycle.avg_len = 0;

        q->reg_cycles.cur_cycle = q->reg_cycles.prev_cycle;


        /* FIXME this is not used in this implementation so far */
        hash_add(data->queues, &q->hlist, port->port_id);

        LOG_DBG("Structures for scheduling policies created...");
        return 0;
}

static int cas_rmt_scheduling_destroy_policy_tx(struct rmt_ps *      ps,
                                                struct rmt_n1_port * port)
{
        struct cas_rmt_ps_data * data;
        struct cas_rmt_queue *   q;

        if (!ps || !port || !ps->priv) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_scheduling_destroy_policy_common");
                return -1;
        }

        data = ps->priv;
        ASSERT(data);

        q = cas_rmt_queue_find(data, port->port_id);
        if (q) return cas_rmt_queue_destroy(q);

        return -1;
}

static int rmt_ps_set_policy_set_param(struct ps_base * bps,
                                       const char    * name,
                                       const char    * value)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);

        (void) ps;

        if (!name) {
                LOG_ERR("Null parameter name");
                return -1;
        }

        if (!value) {
                LOG_ERR("Null parameter value");
                return -1;
        }

        LOG_ERR("No such parameter to set");

        return -1;
}

static struct ps_base *
rmt_ps_cas_create(struct rina_component * component)
{
        struct rmt * rmt = rmt_from_component(component);
        struct rmt_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        struct cas_rmt_ps_data * data = rkzalloc(sizeof(*data), GFP_KERNEL);

        if (!ps || !data) {
                return NULL;
        }

        ps->base.set_policy_set_param = rmt_ps_set_policy_set_param;
        ps->dm = rmt;

        hash_init(data->queues);
        ps->priv = data;

        ps->max_q_policy_tx = cas_max_q_policy_tx;
        ps->max_q_policy_rx = cas_max_q_policy_rx;
        ps->rmt_q_monitor_policy_tx_enq = cas_rmt_q_monitor_policy_tx_enq;
        ps->rmt_q_monitor_policy_tx_deq = cas_rmt_q_monitor_policy_tx_deq;
        ps->rmt_q_monitor_policy_rx = NULL;
        ps->rmt_next_scheduled_policy_tx     = cas_rmt_next_scheduled_policy_tx;
        ps->rmt_enqueue_scheduling_policy_tx = cas_rmt_enqueue_scheduling_policy_tx;
        ps->rmt_requeue_scheduling_policy_tx = cas_rmt_requeue_scheduling_policy_tx;
        ps->rmt_scheduling_create_policy_tx  = cas_rmt_scheduling_create_policy_tx;
        ps->rmt_scheduling_destroy_policy_tx = cas_rmt_scheduling_destroy_policy_tx;

        return &ps->base;
}

static void rmt_ps_cas_destroy(struct ps_base * bps)
{
        struct rmt_ps *          ps = container_of(bps, struct rmt_ps, base);
        struct cas_rmt_queue *   entry;
        struct hlist_node *      tmp;
        int                      bucket;
        struct cas_rmt_ps_data * data;

        data = ps->priv;

        if (!ps || !data) {
                LOG_ERR("PS or PS Data to destroy");
                return;
        }

        if (bps) {

                if (data) {
                        hash_for_each_safe(data->queues, bucket, tmp, entry, hlist) {
                                ASSERT(entry);
                                if (cas_rmt_queue_destroy(entry)) {
                                        LOG_ERR("Could not destroy entry %pK", entry);
                                        return;
                                }
                        }
                        rkfree(data);
                }
                rkfree(ps);
        }
}

struct ps_factory rmt_factory = {
        .owner          = THIS_MODULE,
        .create  = rmt_ps_cas_create,
        .destroy = rmt_ps_cas_destroy,
};
