/*
 * Default policy set for RMT
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#define RINA_PREFIX "rmt-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "rmt.h"
#include <linux/interrupt.h>

/* Used for the old round roubin implementation */
struct sched_substate {
        int last_bucket;
        struct rfifo * last_queue;
};

struct sched_state {
        struct sched_substate rx;
        struct sched_substate tx;
};
/* */

struct rmt_ps_data {
        struct sched_state *   ss;
        struct rmt_queue_set * outqs;
};

static void
default_max_q_policy_tx(struct rmt_ps * ps,
                        struct pdu *    pdu,
                        struct rmt_n1_port * n1_port)
{ }

static void
default_max_q_policy_rx(struct rmt_ps * ps,
                        struct sdu *    sdu,
                        struct rmt_n1_port * n1_port)
{ }

static void
default_rmt_q_monitor_policy_tx(struct rmt_ps *      ps,
                                struct pdu *         pdu,
                                struct rmt_n1_port * n1_port)
{ }

static void
default_rmt_q_monitor_policy_rx(struct rmt_ps *      ps,
                                struct sdu *         sdu,
                                struct rmt_n1_port * n1_port)
{ }

static int
default_rmt_scheduling_create_policy_tx(struct rmt_ps *        ps,
                                        struct rmt_n1_port *   n1_port)
{
        struct rmt_qgroup *  qgroup;
        struct rmt_kqueue *  kqueue;
        struct rmt_ps_data * data;

        if (!ps || !n1_port || !ps->priv) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_scheduling_create_policy_common");
                return -1;
        }

        data = ps->priv;

        qgroup = rmt_qgroup_create(n1_port->port_id);
        if (!qgroup) {
                LOG_ERR("Could not create queues group struct for n1_port %u",
                        n1_port->port_id);
                return -1;
        }
        hash_add(data->outqs->qgroups, &qgroup->hlist, qgroup->pid);

        kqueue = rmt_kqueue_create(0);
        if (!kqueue) {
                LOG_ERR("Could not create kqueue for n1_port %u and key %u",
                        n1_port->port_id, 0);
                return -1;
        }
        /* FIXME this is not used in this implementation so far */
        kqueue->max_q   = 256;
        kqueue->min_qth = 0;
        kqueue->max_qth = 0;
        hash_add(qgroup->queues, &kqueue->hlist, 0);

        LOG_DBG("Structures for scheduling policies created...");
        return 0;
}

static int
default_rmt_scheduling_destroy_policy_tx(struct rmt_ps *        ps,
                                         struct rmt_n1_port *   n1_port)
{
        struct rmt_qgroup *  qgroup;
        struct rmt_ps_data * data;

        if (!ps || !n1_port || !ps->priv) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_scheduling_destroy_policy_common");
                return -1;
        }

        data = ps->priv;

        qgroup = rmt_qgroup_find(data->outqs, n1_port->port_id);
        if (!qgroup) {
                LOG_ERR("Could not find queues group for n1_port %u",
                        n1_port->port_id);
                return -1;
        }
        hash_del(&qgroup->hlist);
        rmt_qgroup_destroy(qgroup);

        return 0;
}

/* NOTE: To be used when rmt_n1_port struct has several queues */
/* Round robin scheduler. */
/*
static struct rmt_queue *
rmt_scheduling_policy_common(struct sched_substate * sss,
                             struct rmt_qmap * qmap, bool restart)
{
        struct rmt_queue *queue;
        struct rmt_queue *selected_queue = NULL;
        int bkt;

        if (restart) {
                sss->last_bucket = 0;
                sss->last_queue = NULL;
        }

        LOG_DBG("SCHED-START: lb = %d, lq = %p",
                sss->last_bucket, sss->last_queue);

        for (bkt = sss->last_bucket, queue = NULL;
                        queue == NULL && bkt < HASH_SIZE(qmap->queues); bkt++) {
                if (sss->last_queue) {
                        queue = sss->last_queue;
                        hlist_for_each_entry_continue(queue, hlist) {
                                selected_queue = queue;
                                break;
                        }
                } else {
                        hlist_for_each_entry(queue, &qmap->queues[bkt], hlist) {
                                selected_queue = queue;
                                break;
                        }
                }
                if (selected_queue) {
                        break;
                } else {
                        // When the bucket changes we must invalidate
                        // sss->last_queue.
                        sss->last_queue = NULL;
                }
        }

        sss->last_bucket = bkt;
        sss->last_queue = selected_queue;

        LOG_DBG("SCHED-END: lb = %d, lq = %p",
                sss->last_bucket, sss->last_queue);

        return selected_queue;
}
*/

static int
default_rmt_enqueue_scheduling_policy_tx(struct rmt_ps *      ps,
                                         struct rmt_n1_port * n1_port,
                                         struct pdu *         pdu)
{
        struct rmt_kqueue *  q;
        struct rmt_qgroup *  qg;
        struct rmt_ps_data * data = ps->priv;

        if (!ps || !n1_port || !pdu) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_enqueu_scheduling_policy_tx");
                return -1;
        }

        /* NOTE: The policy is called with the n1_port lock taken */
        qg = rmt_qgroup_find(data->outqs, n1_port->port_id);
        if (!qg) {
                LOG_ERR("Could not find queue group for n1_port %u",
                        n1_port->port_id);
                pdu_destroy(pdu);
                return -1;
        }
        q = rmt_kqueue_find(qg, 0);
        if (!qg) {
                LOG_ERR("Could not find queue in the group for n1_port %u",
                        n1_port->port_id);
                pdu_destroy(pdu);
                return -1;
        }

        rfifo_push_ni(q->queue, pdu);
        return 0;
}

static struct pdu *
default_rmt_next_scheduled_policy_tx(struct rmt_ps *      ps,
                                     struct rmt_n1_port * n1_port)
{
        struct rmt_kqueue *  q;
        struct rmt_qgroup *  qg;
        struct rmt_ps_data * data = ps->priv;
        struct pdu *         ret_pdu;

        if (!ps || !n1_port) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_next_scheduled_policy_tx");
                return NULL;
        }

        /* NOTE: The policy is called with the n1_port lock taken */
        qg = rmt_qgroup_find(data->outqs, n1_port->port_id);
        if (!qg) {
                LOG_ERR("Could not find queue group for n1_port %u",
                        n1_port->port_id);
                return NULL;
        }
        q = rmt_kqueue_find(qg, 0);
        if (!qg) {
                LOG_ERR("Could not find queue in the group for n1_port %u",
                        n1_port->port_id);
                return NULL;
        }

        ret_pdu = rfifo_pop(q->queue);
        if (!ret_pdu) {
                LOG_ERR("Could not dequeue scheduled pdu");
                return NULL;
        }
        return ret_pdu;
}

static int
default_rmt_scheduling_policy_rx(struct rmt_ps *      ps,
                                 struct rmt_n1_port * n1_port,
                                 struct sdu *         sdu)
{
/*
        struct rmt_ps_data * data = ps->priv;
        atomic_inc(&in_n1_port->n_sdus);

        tasklet_hi_schedule(&instance->ingress.ingress_tasklet);

        LOG_DBG("RMT tasklet scheduled");
*/
        return 0;
}

static int
rmt_ps_set_policy_set_param(struct ps_base * bps,
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
rmt_ps_default_create(struct rina_component * component)
{
        struct rmt * rmt = rmt_from_component(component);
        struct rmt_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        struct rmt_ps_data * data = rkzalloc(sizeof(*data), GFP_KERNEL);

        if (!ps || !data) {
                return NULL;
        }

        /* Allocate policy-set private data. */
        data->ss = rkzalloc(sizeof(*data->ss), GFP_KERNEL);
        if (!data->ss) {
                rkfree(ps);
                rkfree(data);
                return NULL;
        }
        data->outqs = rmt_queue_set_create();
        if (!data->outqs) {
                rkfree(data->ss);
                rkfree(ps);
                rkfree(data);
                return NULL;
        }
        ps->priv = data;

        ps->base.set_policy_set_param = rmt_ps_set_policy_set_param;
        ps->dm          = rmt;

        ps->max_q_policy_tx                  = default_max_q_policy_tx;
        ps->max_q_policy_rx                  = default_max_q_policy_rx;
        ps->rmt_q_monitor_policy_tx          = default_rmt_q_monitor_policy_tx;
        ps->rmt_q_monitor_policy_rx          = default_rmt_q_monitor_policy_rx;
        ps->rmt_next_scheduled_policy_tx     = default_rmt_next_scheduled_policy_tx;
        ps->rmt_enqueue_scheduling_policy_tx = default_rmt_enqueue_scheduling_policy_tx;
        ps->rmt_scheduling_policy_rx         = default_rmt_scheduling_policy_rx;
        ps->rmt_scheduling_create_policy_tx  = default_rmt_scheduling_create_policy_tx;
        ps->rmt_scheduling_destroy_policy_tx = default_rmt_scheduling_destroy_policy_tx;

        return &ps->base;
}

static void
rmt_ps_default_destroy(struct ps_base * bps)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
        struct rmt_ps_data * data = ps->priv;

        if (bps) {
                if (data) {
                        if (data->ss)    rkfree(data->ss);
                        if (data->outqs) rmt_queue_set_destroy(data->outqs);
                        rkfree(data);
                }
                rkfree(ps);
        }
}

struct ps_factory rmt_factory = {
        .owner   = THIS_MODULE,
        .create  = rmt_ps_default_create,
        .destroy = rmt_ps_default_destroy,
};
