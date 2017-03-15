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
#include "pci.h"
#include "policies.h"

#define  N1_CYCLE_DURATION 100

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
        struct {
                struct reg_cycle_t prev_cycle;
                struct reg_cycle_t cur_cycle;
        } reg_cycles;
	bool 		 first_run;
};

struct cas_rmt_ps_data {
        unsigned int q_max;
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

        return tmp;
}

static int cas_rmt_queue_destroy(struct cas_rmt_queue * q)
{
        if (!q) {
                LOG_ERR("No RMT Key-queue to destroy...");
                return -1;
        }

        if (q->queue) rfifo_destroy(q->queue, (void (*)(void *)) pdu_destroy);

        rkfree(q);

        return 0;
}

static int update_cycles(struct reg_cycle_t * prev_cycle,
		  	 struct reg_cycle_t * cur_cycle,
			 ssize_t cur_qlen,
			 bool enqueue)
{
	ssize_t		end_len;
        struct timespec	t_sub;
        s64		t_sub_ns;

	end_len = 0;
	if (!enqueue) {
		if (!cur_qlen)
			return -1;
		end_len = 1;
	}

        else if (cur_qlen == end_len) {
                /* end cycle */
		getnstimeofday(&cur_cycle->t_end);
		t_sub = timespec_sub(cur_cycle->t_end, cur_cycle->t_last_start);
		cur_cycle->sum_area += (ulong) cur_qlen * timespec_to_ns(&t_sub);
		cur_cycle->t_last_start = cur_cycle->t_end;
        } else {
                /* middle cycle */
                cur_cycle->t_last_start = cur_cycle->t_end;
                getnstimeofday(&cur_cycle->t_end);
                t_sub = timespec_sub(cur_cycle->t_end, cur_cycle->t_last_start);
                cur_cycle->sum_area +=(ulong) cur_qlen * timespec_to_ns(&t_sub);
        }

        t_sub = timespec_sub(cur_cycle->t_end, prev_cycle->t_start);
	t_sub_ns = timespec_to_ns(&t_sub);
        cur_cycle->avg_len = (cur_cycle->sum_area + prev_cycle->sum_area);

	if (t_sub_ns <= 0) {
		LOG_ERR("Time delta is <= 0!");
		return -1;
	}

	cur_cycle->avg_len /= (ulong) abs64(t_sub_ns);
	return 0;
}

static int mark_pdu(struct pdu * ret_pdu)
{
        struct pci *    pci;
        pdu_flags_t     pci_flags;

        pci = pdu_pci_get_rw(ret_pdu);
        if (!pci) {
        	LOG_ERR("No PCI to mark in this PDU...");
		pdu_destroy(ret_pdu);
                return -1;
        }
        pci_flags = pci_flags_get(pci);
        pci_flags_set(pci, pci_flags |= PDU_FLAGS_EXPLICIT_CONGESTION);
        LOG_DBG("ECN bit marked");
	return 0;
}

static int cas_rmt_enqueue_policy(struct rmt_ps      *ps,
				  struct rmt_n1_port *port,
				  struct pdu	     *pdu)
{
        struct cas_rmt_queue *   q;
        struct cas_rmt_ps_data * data;
        ssize_t                  cur_qlen;
        struct reg_cycle_t *     prev_cycle, * cur_cycle;

        ASSERT(ps);
        ASSERT(ps->priv);
        ASSERT(pdu);
        ASSERT(port);

        data = ps->priv;

        q = port->rmt_ps_queues;
        if (!q) {
                LOG_ERR("Monitoring: could not find CAS queue for N-1 port %u",
                        port->port_id);
                return RMT_PS_ENQ_ERR;
        }

        cur_qlen = rfifo_length(q->queue);
	if (cur_qlen >= data->q_max) {
		pdu_destroy(pdu);
		return RMT_PS_ENQ_DROP;
	}

        prev_cycle = &q->reg_cycles.prev_cycle;
        cur_cycle  = &q->reg_cycles.cur_cycle;

	if (update_cycles(prev_cycle, cur_cycle, cur_qlen, true)) {
		pdu_destroy(pdu);
		return RMT_PS_ENQ_ERR;
	}

        LOG_DBG("The length for N-1 port %u just calculated is: %lu",
                port->port_id, cur_cycle->avg_len);

        if (cur_cycle->avg_len >= 1)
		if (mark_pdu(pdu))
			return RMT_PS_ENQ_ERR;

	rfifo_push_ni(q->queue, pdu);
        return RMT_PS_ENQ_SCHED;
}

static struct pdu * cas_rmt_dequeue_policy(struct rmt_ps      *ps,
					   struct rmt_n1_port *port)
{
        struct cas_rmt_queue *   q;
        struct cas_rmt_ps_data * data;
        ssize_t                  cur_qlen;
        struct reg_cycle_t *     prev_cycle, * cur_cycle;
	struct pdu *             ret_pdu;

        ASSERT(ps);
        ASSERT(ps->priv);
        ASSERT(port);

        data = ps->priv;

        q = port->rmt_ps_queues;
        if (!q) {
                LOG_ERR("Monitoring: could not find CAS queue for N-1 port %u",
                        port->port_id);
                return NULL;
        }

        cur_qlen = rfifo_length(q->queue);
        ret_pdu = rfifo_pop(q->queue);
        if (!ret_pdu) {
        	LOG_ERR("Could not dequeue scheduled PDU");
        	return NULL;
        }

        prev_cycle = &q->reg_cycles.prev_cycle;
        cur_cycle  = &q->reg_cycles.cur_cycle;

	if (update_cycles(prev_cycle, cur_cycle, cur_qlen, false)) {
		pdu_destroy(ret_pdu);
		return NULL;
	}

        LOG_DBG("The length for N-1 port %u just calculated is: %lu",
                port->port_id, cur_cycle->avg_len);

	return ret_pdu;
}

static void * cas_rmt_q_create_policy(struct rmt_ps      *ps,
				      struct rmt_n1_port *port)
{
        struct cas_rmt_queue *   q;
        struct cas_rmt_ps_data * data;

        if (!ps || !port || !ps->priv) {
                LOG_ERR("Wrong input parms for cas_rmt_q_create_policy_tx");
		return NULL;
        }

        data = ps->priv;

        q = cas_queue_create(port->port_id);
        if (!q) {
                LOG_ERR("Could not create queue for n1_port %u",
                        port->port_id);
                return NULL;
        }

        getnstimeofday(&q->reg_cycles.prev_cycle.t_start);
        getnstimeofday(&q->reg_cycles.prev_cycle.t_last_start);
        getnstimeofday(&q->reg_cycles.prev_cycle.t_end);
        q->reg_cycles.prev_cycle.sum_area = 0;
        q->reg_cycles.prev_cycle.avg_len = 0;
        q->reg_cycles.cur_cycle = q->reg_cycles.prev_cycle;

        LOG_DBG("Structures for scheduling policies created...");
        return q;
}

static int cas_rmt_q_destroy_policy(struct rmt_ps      *ps,
				    struct rmt_n1_port *port)
{
        struct cas_rmt_ps_data * data;
        struct cas_rmt_queue *   q;

        if (!ps || !port || !ps->priv) {
                LOG_ERR("Wrong input parms for rmt_q_destroy_policy");
                return -1;
        }

        data = ps->priv;
        ASSERT(data);

        q = port->rmt_ps_queues;
        if (q) return cas_rmt_queue_destroy(q);

        return -1;
}

static int rmt_cas_set_policy_set_param(struct ps_base *bps,
					const char *name,
					const char *value)
{
	struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
	struct cas_rmt_ps_data *data = ps->priv;
	int bool_value;
	int ret;

	(void) ps;

	if (!name) {
		LOG_ERR("Null parameter name");
		return -1;
	}

	if (!value) {
		LOG_ERR("Null parameter value");
		return -1;
	}

	if (strcmp(name, "q_max") == 0) {
		ret = kstrtoint(value, 10, &bool_value);
		if (!ret)
			data->q_max = bool_value;
	}

	return 0;
}

static struct ps_base *
rmt_ps_cas_create(struct rina_component * component)
{
        struct rmt * rmt = rmt_from_component(component);
        struct rmt_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        struct cas_rmt_ps_data * data = rkzalloc(sizeof(*data), GFP_KERNEL);
	struct rmt_config * rmt_cfg;
	struct policy_parm * ps_param = NULL;

        if (!ps || !data) {
                return NULL;
        }

        ps->base.set_policy_set_param = rmt_cas_set_policy_set_param;
        ps->dm = rmt;

        ps->priv = data;

	rmt_cfg = rmt_config_get(rmt);

	if (rmt_cfg)
		ps_param = policy_param_find(rmt_cfg->policy_set, "q_max");

	if (!ps_param)
		data->q_max = 200;
	else
		rmt_cas_set_policy_set_param(&ps->base,
						policy_param_name(ps_param),
						policy_param_value(ps_param));

        ps->rmt_q_create_policy = cas_rmt_q_create_policy;
        ps->rmt_q_destroy_policy = cas_rmt_q_destroy_policy;
        ps->rmt_enqueue_policy = cas_rmt_enqueue_policy;
        ps->rmt_dequeue_policy = cas_rmt_dequeue_policy;

        return &ps->base;
}

static void rmt_ps_cas_destroy(struct ps_base * bps)
{
        struct rmt_ps *          ps = container_of(bps, struct rmt_ps, base);
        struct cas_rmt_ps_data * data;

        data = ps->priv;

        if (!ps || !data) {
                LOG_ERR("PS or PS Data to destroy");
                return;
        }

        if (bps) {

                if (data) {
                        rkfree(data);
                }
                rkfree(ps);
        }
}

struct ps_factory rmt_factory = {
        .owner   = THIS_MODULE,
        .create  = rmt_ps_cas_create,
        .destroy = rmt_ps_cas_destroy,
};
