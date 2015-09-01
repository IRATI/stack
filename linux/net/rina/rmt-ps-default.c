/*
 * Default policy set for RMT
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>

#define RINA_PREFIX "rmt-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "rmt.h"
#include "rmt-ps-common.h"

static int default_rmt_scheduling_create_policy_tx(struct rmt_ps *ps,
						   struct rmt_n1_port *n1_port)
{ return common_rmt_scheduling_create_policy_tx(ps, n1_port); }

static int default_rmt_scheduling_destroy_policy_tx(struct rmt_ps *ps,
						    struct rmt_n1_port *n1_port)
{ return common_rmt_scheduling_destroy_policy_tx(ps, n1_port); }

static int default_rmt_enqueue_scheduling_policy_tx(struct rmt_ps *ps,
						    struct rmt_n1_port *n1_port,
						    struct pdu *pdu)
{ return common_rmt_enqueue_scheduling_policy_tx(ps, n1_port, pdu); }

static int default_rmt_requeue_scheduling_policy_tx(struct rmt_ps *ps,
						    struct rmt_n1_port *n1_port,
						    struct pdu *pdu)
{ return common_rmt_requeue_scheduling_policy_tx(ps, n1_port, pdu); }

/* FIXME: Consider renaming this function since its name is too long */
static struct pdu
*default_rmt_next_scheduled_policy_tx(struct rmt_ps *ps,
				      struct rmt_n1_port *n1_port)
{ return common_rmt_next_scheduled_policy_tx(ps, n1_port); }

static int default_rmt_scheduling_policy_rx(struct rmt_ps *ps,
					    struct rmt_n1_port *n1_port,
					    struct sdu *sdu)
{ return common_rmt_scheduling_policy_rx(ps, n1_port, sdu); }

static int rmt_ps_set_policy_set_param(struct ps_base *bps,
				       const char *name,
				       const char *value)
{ return rmt_ps_common_set_policy_set_param(bps, name, value); }

static struct ps_base *rmt_ps_default_create(struct rina_component *component)
{
	struct rmt *rmt;
	struct rmt_ps *ps;
	struct rmt_ps_common_data *data;

	rmt = rmt_from_component(component);
	ps = rkmalloc(sizeof(*ps), GFP_KERNEL);
	if (!ps)
		return NULL;

	data = rkmalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		rkfree(ps);
		return NULL;
	}

	/* Allocate policy-set private data. */
	data->outqs = rmt_queue_set_create();
	if (!data->outqs) {
		rkfree(ps);
		rkfree(data);
		return NULL;
	}

	ps->priv = data;

	ps->base.set_policy_set_param = rmt_ps_set_policy_set_param;
	ps->dm = rmt;

	ps->max_q_policy_tx = NULL;
	ps->max_q_policy_rx = NULL;
	ps->rmt_q_monitor_policy_tx_enq = NULL;
	ps->rmt_q_monitor_policy_tx_deq = NULL;
	ps->rmt_q_monitor_policy_rx = NULL;
	ps->rmt_next_scheduled_policy_tx =
		default_rmt_next_scheduled_policy_tx;
	ps->rmt_enqueue_scheduling_policy_tx =
		default_rmt_enqueue_scheduling_policy_tx;
	ps->rmt_requeue_scheduling_policy_tx =
		default_rmt_requeue_scheduling_policy_tx;
	ps->rmt_scheduling_policy_rx =
		default_rmt_scheduling_policy_rx;
	ps->rmt_scheduling_create_policy_tx  =
		default_rmt_scheduling_create_policy_tx;
	ps->rmt_scheduling_destroy_policy_tx =
		default_rmt_scheduling_destroy_policy_tx;

	return &ps->base;
}

static void rmt_ps_default_destroy(struct ps_base *bps)
{
	struct rmt_ps *ps;
	struct rmt_ps_common_data *data;

	ps = container_of(bps, struct rmt_ps, base);
	data = ps->priv;

	if (bps) {
		if (data) {
			if (data->outqs)
				rmt_queue_set_destroy(data->outqs);
			rkfree(data);
		}
		rkfree(ps);
	}
}

struct ps_factory rmt_factory = {
	.owner = THIS_MODULE,
	.create = rmt_ps_default_create,
	.destroy = rmt_ps_default_destroy,
};
