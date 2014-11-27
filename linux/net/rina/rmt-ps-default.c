/*
 * Default policy set for RMT
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>

#define RINA_PREFIX "rmt-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "rmt.h"

struct sched_substate {
        int last_bucket;
        struct rmt_queue * last_queue;
};

struct sched_state {
        struct sched_substate rx;
        struct sched_substate tx;
};

static void
default_max_q_policy_tx(struct rmt_ps * ps,
                        struct pdu *    pdu,
                        struct rfifo *  queue)
{ }

static void
default_max_q_policy_rx(struct rmt_ps * ps,
                        struct sdu *    sdu,
                        struct rfifo *  queue)
{ }

static void
default_rmt_q_monitor_policy_tx(struct rmt_ps * ps,
                                struct pdu *    pdu,
                                struct rfifo *  queue)
{ }

static void
default_rmt_q_monitor_policy_rx(struct rmt_ps * ps,
                                struct sdu *    sdu,
                                struct rfifo *  queue)
{ }

/* Round robin scheduler. */
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
                        /* When the bucket changes we must invalidate
                         * sss->last_queue. */
                        sss->last_queue = NULL;
                }
        }

        sss->last_bucket = bkt;
        sss->last_queue = selected_queue;

        LOG_DBG("SCHED-END: lb = %d, lq = %p",
                sss->last_bucket, sss->last_queue);

        return selected_queue;
}


static struct rmt_queue *
default_rmt_scheduling_policy_tx(struct rmt_ps * ps,
                                 struct rmt_qmap * qmap,
                                 bool restart)
{
        struct sched_state * ss = ps->priv;

        return rmt_scheduling_policy_common(&ss->tx, qmap, restart);
}

static struct rmt_queue *
default_rmt_scheduling_policy_rx(struct rmt_ps * ps,
                                 struct rmt_qmap * qmap,
                                 bool restart)
{
        struct sched_state * ss = ps->priv;

        return rmt_scheduling_policy_common(&ss->rx, qmap, restart);
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
        struct sched_state *ss;

        if (!ps) {
                return NULL;
        }

        /* Allocate policy-set private data. */
        ss = rkzalloc(sizeof(*ss), GFP_KERNEL);
        if (!ss) {
                rkfree(ps);
                return NULL;
        }
        ps->priv = ss;

        ps->base.set_policy_set_param = rmt_ps_set_policy_set_param;
        ps->dm          = rmt;

        ps->max_q_policy_tx             = default_max_q_policy_tx;
        ps->max_q_policy_rx             = default_max_q_policy_rx;
        ps->rmt_q_monitor_policy_tx     = default_rmt_q_monitor_policy_tx;
        ps->rmt_q_monitor_policy_rx     = default_rmt_q_monitor_policy_rx;
        ps->rmt_scheduling_policy_tx    = default_rmt_scheduling_policy_tx;
        ps->rmt_scheduling_policy_rx    = default_rmt_scheduling_policy_rx;

        ps->max_q       = 256;

        return &ps->base;
}

static void
rmt_ps_default_destroy(struct ps_base * bps)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);

        if (bps) {
                rkfree(ps->priv);
                rkfree(ps);
        }
}

struct ps_factory rmt_factory = {
        .owner          = THIS_MODULE,
        .create  = rmt_ps_default_create,
        .destroy = rmt_ps_default_destroy,
};
