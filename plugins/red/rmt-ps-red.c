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
#include "policies.h"
#include "rmt-ps-debug.h"

#define MIN_TH_P_DEFAULT 2
#define MAX_TH_P_DEFAULT 5
#define WP_P_DEFAULT 2


struct red_rmt_queue {
        struct rfifo *    queue;
        port_id_t         port_id;

	struct red_parms  parms;
	struct red_vars   vars;
	struct red_stats  stats;

#if RMT_DEBUG
	struct red_rmt_debug * debug;
#endif
};

struct red_rmt_ps_data {
	struct tc_red_qopt conf_data;
	u8 *   stab;
};

static struct red_rmt_queue * red_queue_create(port_id_t          port_id,
					       struct tc_red_qopt cfg,
					       u8 *               stab)
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

	/* 0 is max_P, it will use Plog to calculate it */
	red_set_parms(&tmp->parms, cfg.qth_min, cfg.qth_max, cfg.Wlog,
		      cfg.Plog, cfg.Scell_log, stab, 0);

	red_set_vars(&tmp->vars);

#if RMT_DEBUG
	tmp->debug = red_rmt_debug_create(port_id);
#endif

        return tmp;
}

static int red_rmt_queue_destroy(struct red_rmt_queue * q)
{
        if (!q) {
                LOG_ERR("No RMT Key-queue to destroy...");
                return -1;
        }
        if (q->queue) rfifo_destroy(q->queue, (void (*)(void *)) pdu_destroy);

        rkfree(q);

        return 0;
}

static struct pdu *
red_rmt_dequeue_policy(struct rmt_ps *      ps,
		       struct rmt_n1_port * port)
{
        struct red_rmt_queue *   q;
        struct red_rmt_ps_data * data = ps->priv;
        struct pdu *             ret_pdu;
	unsigned int             qlen;

        if (!ps || !port || !data) {
                LOG_ERR("Wrong input parameters for "
                        "red_rmt_dequeue_policy");
                return NULL;
        }

        q = port->rmt_ps_queues;
        if (!q) {
                LOG_ERR("Could not find queue for n1_port %u",
                        port->port_id);
                return NULL;
        }

	qlen = rfifo_length(q->queue);
        ret_pdu = rfifo_pop(q->queue);
        if (!ret_pdu)
                LOG_ERR("Could not dequeue scheduled pdu");

	/* Compute average queue usage (see RED)
	 * RED does not do this here and instead uses an approximation
	 * when the idle period is over. But doing this the qavg
	 * calculation seems to be distorted. For this PS we can afford to
	 * calculate the avg also when pdus are dequeued.
	 */
	q->vars.qavg = red_calc_qavg(&q->parms,
				     &q->vars,
				     qlen);

#if RMT_DEBUG
	if (q->debug->q_index < RMT_DEBUG_SIZE && ret_pdu) {
		q->debug->q_log[q->debug->q_index][0] = rfifo_length(q->queue);
		q->debug->q_log[q->debug->q_index++][1] = q->vars.qavg >> q->parms.Wlog;
	}
	q->debug->stats = q->stats;
#endif
        return ret_pdu;
}

static int red_rmt_enqueue_policy(struct rmt_ps *      ps,
                                  struct rmt_n1_port * port,
                                  struct pdu *         pdu)
{
        struct red_rmt_queue *   q;
        struct red_rmt_ps_data * data = ps->priv;
        struct pci *             pci;
        unsigned long            pci_flags;
	int			 ret;
	unsigned int             qlen;

        if (!ps || !port || !pdu || !data) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_enqueu_scheduling_policy_tx");
                return RMT_PS_ENQ_ERR;
        }

        q = port->rmt_ps_queues;
        if (!q) {
                LOG_ERR("Could not find queue for n1_port %u",
                        port->port_id);
                pdu_destroy(pdu);
                return RMT_PS_ENQ_ERR;
        }

	qlen = rfifo_length(q->queue);

	/* Compute average queue usage (see RED)
	 * Formula is qavg = qavg*(1-W) + backlog*W;
	 * backlog is the current occupation of the queue
	 */
	q->vars.qavg = red_calc_qavg(&q->parms,
				     &q->vars,
				     qlen);
	LOG_DBG("qavg':  %lu, qlen: %lu", q->vars.qavg >> (q->parms.Wlog),
					  qlen);

	if(qlen >= data->conf_data.limit) {
		q->stats.forced_drop++;
		ret = RMT_PS_ENQ_DROP;
        	pdu_destroy(pdu);
        	LOG_DBG("PDU dropped, qmax reached...");
		goto exit;
	}

	switch (red_action(&q->parms, &q->vars, q->vars.qavg)) {
	case RED_DONT_MARK:
		LOG_DBG("RED_DONT_MARK");
		break;

	case RED_PROB_MARK:
		LOG_DBG("RED_PROB_MARK");
		q->stats.prob_mark++;
		/* mark ECN bit */
                pci = pdu_pci_get_rw(pdu);
                pci_flags = pci_flags_get(pci);
                pci_flags_set(pci, pci_flags |= PDU_FLAGS_EXPLICIT_CONGESTION);
		break;

	case RED_HARD_MARK:
		LOG_DBG("RED_HARD_MARK");
		q->stats.forced_mark++;
		/* mark ECN bit */
                pci = pdu_pci_get_rw(pdu);
                pci_flags = pci_flags_get(pci);
                pci_flags_set(pci, pci_flags |= PDU_FLAGS_EXPLICIT_CONGESTION);
		break;
	}

	rfifo_push_ni(q->queue, pdu);
	ret = RMT_PS_ENQ_SCHED;

exit:
#if RMT_DEBUG
	if (q->debug->q_index < RMT_DEBUG_SIZE && pdu) {
		q->debug->q_log[q->debug->q_index][0] = rfifo_length(q->queue);
		q->debug->q_log[q->debug->q_index++][1] = q->vars.qavg >> q->parms.Wlog;
	}
	q->debug->stats = q->stats;
#endif
        return ret;
}

static void * red_rmt_q_create_policy(struct rmt_ps *      ps,
                                   struct rmt_n1_port * port)
{
        struct red_rmt_queue *   q;
        struct red_rmt_ps_data * data;

        if (!ps || !port || !ps->priv) {
                LOG_ERR("Wrong input parameters for "
                        "red_rmt_scheduling_create_policy");
                return NULL;
        }

        data = ps->priv;

        q = red_queue_create(port->port_id,
	                     data->conf_data,
			     data->stab);
        if (!q) {
                LOG_ERR("Could not create queue for n1_port %u",
                        port->port_id);
                return NULL;
        }

        LOG_DBG("Structures for scheduling policies created...");
        return q;
}

static int red_rmt_q_destroy_policy(struct rmt_ps *      ps,
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

        q = port->rmt_ps_queues;
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

static int
rmt_ps_load_param(struct rmt_ps *ps, const char *param_name, const char *dflt)
{
	struct rmt_config * rmt_cfg;
	struct policy_parm * ps_param;

	rmt_cfg = rmt_config_get(ps->dm);

        if (rmt_cfg) {
		/* This is availble at assign-to-dif time but it is not
		 * available at select-policy-set time. */
		ps_param = policy_param_find(rmt_cfg->policy_set, param_name);
        } else {
		ps_param = NULL;
	}

	if (!ps_param) {
		LOG_WARN("No PS param %s", param_name);
		(void)dflt;
	} else {
		red_rmt_ps_set_policy_set_param(&ps->base,
						policy_param_name(ps_param),
						policy_param_value(ps_param));
	}

	return 0;
}

static struct ps_base *
rmt_ps_red_create(struct rina_component * component)
{
        struct rmt * rmt = rmt_from_component(component);
        struct rmt_ps * ps;
        struct red_rmt_ps_data * data;

        ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        if (!ps) {
                return NULL;
        }

        data = rkzalloc(sizeof(*data), GFP_KERNEL);
        if (!data) {
            kfree(ps);
            return NULL;
        }

        ps->base.set_policy_set_param = red_rmt_ps_set_policy_set_param;
        ps->dm = rmt;

	ps->priv = data;

	rmt_ps_load_param(ps, "qmax_p", NULL);
	rmt_ps_load_param(ps, "qth_min_p", NULL);
	rmt_ps_load_param(ps, "qth_max_p", NULL);
	rmt_ps_load_param(ps, "Wlog_p", NULL);
	rmt_ps_load_param(ps, "Plog_p", NULL);
	rmt_ps_load_param(ps, "Scell_log_p", NULL);
	rmt_ps_load_param(ps, "stab_address_p", NULL);

        ps->rmt_enqueue_policy = red_rmt_enqueue_policy;
        ps->rmt_dequeue_policy = red_rmt_dequeue_policy;
	ps->rmt_q_create_policy = red_rmt_q_create_policy;
	ps->rmt_q_destroy_policy = red_rmt_q_destroy_policy;

        return &ps->base;
}

static void rmt_ps_red_destroy(struct ps_base * bps)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
        struct red_rmt_ps_data * data;

        data = ps->priv;

        if (bps) {
                if (data)
                        rkfree(data);
                rkfree(ps);
        }
}

struct ps_factory rmt_factory = {
        .owner   = THIS_MODULE,
        .create  = rmt_ps_red_create,
        .destroy = rmt_ps_red_destroy,
};
