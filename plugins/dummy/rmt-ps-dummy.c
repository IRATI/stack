/*
 * Dummy RMT PS
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

#define RINA_PREFIX "dummy-rmt-ps"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"

static void *
dummy_rmt_q_create_policy(struct rmt_ps      *ps,
			  struct rmt_n1_port *port)
{
        printk("%s: called()\n", __func__);
        return NULL;
}

static int
dummy_rmt_q_destroy_policy(struct rmt_ps      *ps,
			   struct rmt_n1_port *port)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
dummy_rmt_enqueue_policy(struct rmt_ps	    *ps,
			 struct rmt_n1_port *port,
			 struct du	    *du,
			 bool                must_enqueue)
{
        printk("%s: called()\n", __func__);
        return RMT_PS_ENQ_SCHED;
}

static struct du *
dummy_rmt_dequeue_policy(struct rmt_ps	    *ps,
			 struct rmt_n1_port *port)
{
	printk("%s: called()\n", __func__);
	return NULL;
}

static int
dummy_rmt_ps_set_policy_set_param(struct ps_base *bps,
				  const char	 *name,
				  const char	 *value)
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
rmt_ps_dummy_create(struct rina_component *component)
{
	struct rmt *rmt = rmt_from_component(component);
	struct rmt_ps *ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

	if (!ps) {
		return NULL;
	}

	ps->base.set_policy_set_param = dummy_rmt_ps_set_policy_set_param; /* default */
	ps->dm = rmt;
	ps->priv = NULL;
	ps->rmt_q_create_policy = dummy_rmt_q_create_policy;
	ps->rmt_q_destroy_policy = dummy_rmt_q_destroy_policy;
	ps->rmt_enqueue_policy = dummy_rmt_enqueue_policy;
	ps->rmt_dequeue_policy = dummy_rmt_dequeue_policy;

	return &ps->base;
}

static void rmt_ps_dummy_destroy(struct ps_base *bps)
{
	struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);

	if (bps) {
		rkfree(ps);
	}
}

struct ps_factory rmt_factory = {
	.owner	 = THIS_MODULE,
	.create  = rmt_ps_dummy_create,
	.destroy = rmt_ps_dummy_destroy,
};
