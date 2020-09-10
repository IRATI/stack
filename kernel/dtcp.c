/*
 * DTCP (Data Transfer Control Protocol)
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define RINA_PREFIX "dtcp"

#include <linux/delay.h>

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtcp.h"
#include "rmt.h"
#include "efcp-str.h"
#include "dtp.h"
#include "dtp-utils.h"
#include "dtcp-conf-utils.h"
#include "ps-factory.h"
#include "dtcp-ps.h"
#include "dtcp-ps-default.h"
#include "policies.h"
#include "rds/rmem.h"
#include "debug.h"

static struct policy_set_list policy_sets = {
        .head = LIST_HEAD_INIT(policy_sets.head)
};

int dtcp_pdu_send(struct dtcp * dtcp, struct du * du)
{
        return dtp_pdu_ctrl_send(dtcp->parent, du);
}
EXPORT_SYMBOL(dtcp_pdu_send);

int dtcp_last_time_set(struct dtcp * dtcp, struct timespec * s)
{
	ASSERT(dtcp);
	ASSERT(dtcp->sv);
	ASSERT(s);

	spin_lock_bh(&dtcp->parent->sv_lock);
	dtcp->sv->last_time.tv_sec = s->tv_sec;
	dtcp->sv->last_time.tv_nsec = s->tv_nsec;
	spin_unlock_bh(&dtcp->parent->sv_lock);

	return 0;
}
EXPORT_SYMBOL(dtcp_last_time_set);

static seq_num_t next_snd_ctl_seq(struct dtcp * dtcp)
{
        seq_num_t     tmp;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_bh(&dtcp->parent->sv_lock);
        tmp = ++dtcp->sv->next_snd_ctl_seq;
        spin_unlock_bh(&dtcp->parent->sv_lock);

        return tmp;
}

static ssize_t dtcp_attr_show(struct robject *		     robj,
                         	     struct robj_attribute * attr,
                                     char *		     buf)
{
	struct dtcp * instance;

	instance = container_of(robj, struct dtcp, robj);
	if (!instance || !instance->sv || !instance->parent || !instance->cfg)
		return 0;

	if (strcmp(robject_attr_name(attr), "rtt") == 0) {
		return sprintf(buf, "%u\n", instance->sv->rtt);
	}
	if (strcmp(robject_attr_name(attr), "srtt") == 0) {
		return sprintf(buf, "%u\n", instance->sv->srtt);
	}
	if (strcmp(robject_attr_name(attr), "rttvar") == 0) {
		return sprintf(buf, "%u\n", instance->sv->rttvar);
	}
	/* Flow control */
	if (strcmp(robject_attr_name(attr), "closed_win_q_length") == 0) {
		return sprintf(buf, "%zu\n", cwq_size(instance->parent->cwq));
	}
	if (strcmp(robject_attr_name(attr), "closed_win_q_size") == 0) {
		return sprintf(buf, "%u\n",
			dtcp_max_closed_winq_length(instance->cfg));
	}
	/* Win based */
	if (strcmp(robject_attr_name(attr), "sndr_credit") == 0) {
		return sprintf(buf, "%u\n", instance->sv->sndr_credit);
	}
	if (strcmp(robject_attr_name(attr), "rcvr_credit") == 0) {
		return sprintf(buf, "%u\n", instance->sv->rcvr_credit);
	}
	if (strcmp(robject_attr_name(attr), "snd_rt_win_edge") == 0) {
		return sprintf(buf, "%u\n", instance->sv->snd_rt_wind_edge);
	}
	if (strcmp(robject_attr_name(attr), "rcv_rt_win_edge") == 0) {
		return sprintf(buf, "%u\n", instance->sv->rcvr_rt_wind_edge);
	}
	/* Rate based */
	if (strcmp(robject_attr_name(attr), "pdus_per_time_unit") == 0) {
		return sprintf(buf, "%u\n", instance->sv->pdus_per_time_unit);
	}
	if (strcmp(robject_attr_name(attr), "time_unit") == 0) {
		return sprintf(buf, "%u\n", instance->sv->time_unit);
	}
	if (strcmp(robject_attr_name(attr), "sndr_rate") == 0) {
		return sprintf(buf, "%u\n", instance->sv->sndr_rate);
	}
	if (strcmp(robject_attr_name(attr), "rcvr_rate") == 0) {
		return sprintf(buf, "%u\n", instance->sv->rcvr_rate);
	}
	if (strcmp(robject_attr_name(attr), "pdus_rcvd_in_time_unit") == 0) {
		return sprintf(buf, "%u\n", instance->sv->pdus_rcvd_in_time_unit);
	}
	if (strcmp(robject_attr_name(attr), "last_time") == 0) {
		struct timespec s;
		s.tv_sec = instance->sv->last_time.tv_sec;
		s.tv_nsec = instance->sv->last_time.tv_nsec;
		return sprintf(buf, "%lld.%.9ld\n",
			(long long) s.tv_sec, s.tv_nsec);
	}
	/* Rtx control */
	if (strcmp(robject_attr_name(attr), "data_retransmit_max") == 0) {
		return sprintf(buf, "%u\n", dtcp_data_retransmit_max(instance->cfg));
	}
	if (strcmp(robject_attr_name(attr), "last_snd_data_ack") == 0) {
		return sprintf(buf, "%u\n", instance->sv->last_snd_data_ack);
	}
	if (strcmp(robject_attr_name(attr), "last_rcv_data_ack") == 0) {
		return sprintf(buf, "%u\n", instance->sv->last_rcv_data_ack);
	}
	if (strcmp(robject_attr_name(attr), "snd_lf_win") == 0) {
		return sprintf(buf, "%u\n", instance->sv->snd_lft_win);
	}
	if (strcmp(robject_attr_name(attr), "rtx_q_length") == 0) {
		return sprintf(buf, "%u\n", rtxq_size(instance->parent->rtxq));
	}
	if (strcmp(robject_attr_name(attr), "rtx_drop_pdus") == 0) {
		return sprintf(buf, "%u\n",
			rtxq_drop_pdus(instance->parent->rtxq));
	}
	if (strcmp(robject_attr_name(attr), "ps_name") == 0) {
		return sprintf(buf, "%s\n",instance->base.ps_factory->name);
	}
	return 0;
}
RINA_SYSFS_OPS(dtcp);
RINA_ATTRS(dtcp, rtt, srtt, rttvar, ps_name);
RINA_KTYPE(dtcp);

int ctrl_pdu_send(struct dtcp * dtcp, pdu_type_t type, bool direct)
{
        struct du *   du;

        if (dtcp->sv->rendezvous_rcvr) {
        	LOG_DBG("Generating FC PDU in RV at RCVR");
        }

        du  = pdu_ctrl_generate(dtcp, type);
        if (!du) {
        	atomic_dec(&dtcp->cpdus_in_transit);
        	return -1;
        }
        if (dtcp->sv->rendezvous_rcvr) {
        	LOG_DBG("Generated FC PDU in RV at RCVR");
        }

        if (direct) {
        	if (dtp_pdu_send(dtcp->parent, dtcp->rmt, du)){
        		atomic_dec(&dtcp->cpdus_in_transit);
        		du_destroy(du);
        		return -1;
        	}
        } else {
                if (dtcp_pdu_send(dtcp, du)){
                	atomic_dec(&dtcp->cpdus_in_transit);
                	du_destroy(du);
                	return -1;
                }
        }

        atomic_dec(&dtcp->cpdus_in_transit);

        return 0;
}

/* Runs the Rendezvous-at-receiver timer */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
static void tf_rendezvous_rcv(void * data)
#else
static void tf_rendezvous_rcv(struct timer_list * tl)
#endif
{
        struct dtcp * dtcp;
        struct dtp *  dtp;
        bool         start_rv_rcv_timer;
        timeout_t    rv;

        LOG_DBG("Running rendezvous-at-receiver timer...");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        dtcp = (struct dtcp *) data;
#else
	dtcp = from_timer(dtcp, tl, rendezvous_rcv);
#endif
        if (!dtcp) {
                LOG_ERR("No dtcp to work with");
                return;
        }
        dtp = dtcp->parent;

	/* Check if the reliable ACK PDU needs to be sent*/
	start_rv_rcv_timer = false;
	spin_lock_bh(&dtp->sv_lock);
	if (dtcp->sv->rendezvous_rcvr) {
		/* Start rendezvous-at-receiver timer, wait for Tr to fire */
		start_rv_rcv_timer = true;
		rv = jiffies_to_msecs(dtp->sv->tr);
	}
	spin_unlock_bh(&dtp->sv_lock);

	if (start_rv_rcv_timer) {
		LOG_DBG("DTCP Sending FC: RCV LWE: %u | RCV RWE: %u",
			 dtp->sv->rcv_left_window_edge,
			 dtcp->sv->rcvr_rt_wind_edge);

		/* Send rendezvous PDU and start timer */
		atomic_inc(&dtcp->cpdus_in_transit);
			   ctrl_pdu_send(dtcp, PDU_TYPE_FC, true);

		rtimer_start(&dtcp->rendezvous_rcv, rv);
	}

        return;
}

static int push_pdus_rmt(struct dtcp * dtcp)
{
        ASSERT(dtcp);

        if (!dtcp->parent->cwq) {
                LOG_ERR("No Closed Window Queue");
                return -1;
        }

        cwq_deliver(dtcp->parent->cwq,
                    dtcp->parent,
                    dtcp->rmt);

        return 0;
}

static int populate_ctrl_pci(struct pci *  pci,
                             struct dtcp * dtcp)
{
        seq_num_t snd_lft;
        seq_num_t snd_rt;
        seq_num_t LWE;
        seq_num_t last_rcv_ctl_seq;
        uint_t rt;
        uint_t tf;

        if (!dtcp->cfg) {
                LOG_ERR("No dtcp cfg...");
                return -1;
        }

        /*
         * FIXME: Shouldn't we check if PDU_TYPE_ACK_AND_FC or
         * PDU_TYPE_NACK_AND_FC ?
         */
        spin_lock_bh(&dtcp->parent->sv_lock);
        LWE = dtcp->parent->sv->rcv_left_window_edge;
        last_rcv_ctl_seq = dtcp->sv->last_rcv_ctl_seq;

        if (dtcp_flow_ctrl(dtcp->cfg)) {
                if (dtcp_window_based_fctrl(dtcp->cfg)) {
                        snd_lft = dtcp->sv->snd_lft_win;
                        snd_rt  = dtcp->sv->snd_rt_wind_edge;

                        pci_control_new_left_wind_edge_set(pci, LWE);
                        pci_control_new_rt_wind_edge_set(pci,
				dtcp->sv->rcvr_rt_wind_edge);

                        pci_control_my_left_wind_edge_set(pci, snd_lft);
                        pci_control_my_rt_wind_edge_set(pci, snd_rt);
                }

                if (dtcp_rate_based_fctrl(dtcp->cfg)) {
                	rt = dtcp->sv->sndr_rate;
                	tf = dtcp->sv->time_unit;

                	LOG_DBG("Populating control pci with rate "
                		"settings, rate: %u, time: %u",
                		rt, tf);

                        pci_control_sndr_rate_set(pci, rt);
                        pci_control_time_frame_set(pci, tf);
                }
        }
        spin_unlock_bh(&dtcp->parent->sv_lock);

        switch (pci_type(pci)) {
        case PDU_TYPE_ACK_AND_FC:
                if (pci_control_ack_seq_num_set(pci, LWE)) {
                        LOG_ERR("Could not set sn to ACK");
                        return -1;
                }
        	if (pci_control_last_seq_num_rcvd_set(pci, last_rcv_ctl_seq)) {
			LOG_ERR("Could not set last ctrl sn rcvd");
                	return -1;
        	}
                return 0;
        case PDU_TYPE_ACK:
                if (pci_control_ack_seq_num_set(pci, LWE)) {
                        LOG_ERR("Could not set sn to ACK");
                        return -1;
                }
		return 0;
        case PDU_TYPE_NACK_AND_FC:
        case PDU_TYPE_NACK:
                if (pci_control_ack_seq_num_set(pci, LWE + 1)) {
                        LOG_ERR("Could not set sn to NACK");
                        return -1;
                }
                return 0;
        case PDU_TYPE_RENDEZVOUS:
        case PDU_TYPE_CACK:
        	if (pci_control_last_seq_num_rcvd_set(pci, last_rcv_ctl_seq)) {
			LOG_ERR("Could not set last ctrl sn rcvd");
                	return -1;
        	}
		return 0;
        default:
                break;
        }

        return 0;
}

struct du * pdu_ctrl_generate(struct dtcp * dtcp, pdu_type_t type)
{
        struct du *     du;
        seq_num_t       seq;

        du = du_create_efcp_ni(type, dtcp->parent->efcp->container->config);
        if (!du) {
		LOG_ERR("Could not create PDU of type %x", type);
                return NULL;
	}

        seq = next_snd_ctl_seq(dtcp);
        if (pci_format(&du->pci,
                       dtcp->parent->efcp->connection->source_cep_id,
		       dtcp->parent->efcp->connection->destination_cep_id,
		       dtcp->parent->efcp->connection->source_address,
		       dtcp->parent->efcp->connection->destination_address,
                       seq,
		       dtcp->parent->efcp->connection->qos_id,
                       0,
		       du->pci.len,
                       type)) {
		LOG_ERR("Could not format recently created PCI");
                du_destroy(du);
                return NULL;
        }

        if (populate_ctrl_pci(&du->pci, dtcp)) {
                LOG_ERR("Could not populate ctrl PCI");
                du_destroy(du);
                return NULL;
        }

        return du;
}
EXPORT_SYMBOL(pdu_ctrl_generate);

/* not a policy according to specs */
static int rcv_nack_ctl(struct dtcp * dtcp,
                        struct du *  du)
{
        struct dtcp_ps * ps;
        seq_num_t        seq_num;
        timeout_t	 tr;

        seq_num = pci_control_ack_seq_num(&du->pci);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);

        spin_lock_bh(&dtcp->parent->sv_lock);
        tr = dtcp->parent->sv->tr;
        spin_unlock_bh(&dtcp->parent->sv_lock);

        if (ps->rtx_ctrl) {
                if (!dtcp->parent->rtxq) {
                        rcu_read_unlock();
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }
                rtxq_nack(dtcp->parent->rtxq, seq_num, tr);
		if (ps->rtt_estimator)
                	ps->rtt_estimator(ps, pci_control_ack_seq_num(&du->pci));
        }
        rcu_read_unlock();

        LOG_DBG("DTCP received NACK (CPU: %d)", smp_processor_id());

        du_destroy(du);

        return 0;
}

static int rcv_ack(struct dtcp * dtcp,
                   struct du *   du)
{
        struct dtcp_ps * ps;
        seq_num_t        seq;
        int              ret;

        seq = pci_control_ack_seq_num(&du->pci);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
	if (ps->rtx_ctrl && ps->rtt_estimator) {
        	if (ps->rtt_estimator(ps,
        			      pci_control_ack_seq_num(&du->pci)) == -1) {
        		/* Don't process this PDU anymore, it is ACKing a
        		 * DT PDU that is not in the RTX queue (was already
        		 * discarded)
        		 */
        		rcu_read_unlock();
        		return 0;
        	}
	}

	ret = ps->sender_ack(ps, seq);
        rcu_read_unlock();

        LOG_DBG("DTCP received ACK (CPU: %d)", smp_processor_id());

        du_destroy(du);

        return ret;
}

static int update_window_and_rate(struct dtcp * dtcp,
                		  struct du *   du)
{
        uint_t 	     rt;
        uint_t       tf;
        bool         cancel_rv_timer;

        spin_lock_bh(&dtcp->parent->sv_lock);
        if(dtcp_window_based_fctrl(dtcp->cfg)) {
        	dtcp->sv->snd_rt_wind_edge =
        		pci_control_new_rt_wind_edge(&du->pci);
        }

        if(dtcp_rate_based_fctrl(dtcp->cfg)) {
                rt = pci_control_sndr_rate(&du->pci);
                tf = pci_control_time_frame(&du->pci);
		if(tf && rt) {
			dtcp->sv->sndr_rate = rt;
			dtcp->sv->time_unit = tf;

			LOG_DBG("rbfc Rate based fields sets on flow ctl, "
				"rate: %u, time: %u",
				rt, tf);
		}
        }

        LOG_DBG("New SND RWE: %u, New LWE: %u, DTCP: %pK",
        	 dtcp->sv->snd_rt_wind_edge, dtcp->sv->snd_lft_win, dtcp);

        /* Check if rendezvous timer is active */
        cancel_rv_timer = false;
        if (dtcp->sv->rendezvous_sndr) {
        	dtcp->sv->rendezvous_sndr = false;
        	cancel_rv_timer = true;
        	LOG_DBG("Stopping rendezvous timer");
        }
        spin_unlock_bh(&dtcp->parent->sv_lock);

        if (cancel_rv_timer)
        	rtimer_stop(&dtcp->parent->timers.rendezvous);

        push_pdus_rmt(dtcp);

        du_destroy(du);

        return 0;
}

static int rcv_flow_ctl(struct dtcp * dtcp,
                        struct du *   du)
{
	struct dtcp_ps * ps;
	seq_num_t    seq;
	uint_t	     credit;

	/* Update RTT estimation */
	credit = dtcp->sv->sndr_credit;
	seq = pci_control_new_rt_wind_edge(&du->pci);

	rcu_read_lock();
	ps = container_of(rcu_dereference(dtcp->base.ps),
			  struct dtcp_ps, base);

	LOG_DBG("DTCP received FC: New RWE: %u, Credit: %u, SN to drop: %u",
		seq, credit, seq-credit);
	if (!ps->rtx_ctrl && ps->rtt_estimator)
		ps->rtt_estimator(ps, seq - credit);

	rcu_read_unlock();

    	if (dtcp->parent->rttq) {
    		rttq_drop(dtcp->parent->rttq, seq-credit);
    	}

    	return update_window_and_rate(dtcp, du);
}

static int rcv_ack_and_flow_ctl(struct dtcp * dtcp,
                                struct du *   du)
{
        struct dtcp_ps * ps;
        seq_num_t        seq;

        seq = pci_control_ack_seq_num(&du->pci);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);

        if (ps->rtx_ctrl && ps->rtt_estimator) {
        	if (ps->rtt_estimator(ps, seq) == -1) {
        		/* Don't process this PDU anymore, it is ACKing a
        		 * DT PDU that is not in the RTX queue (was already
        		 * discarded)
        		 */
        		rcu_read_unlock();
        		return 0;
        	}
        }

        /* This updates sender LWE */
        if (ps->sender_ack(ps, seq))
                LOG_ERR("Could not update RTXQ and LWE");

        rcu_read_unlock();

        LOG_DBG("DTCP received ACK-FC (CPU: %d)", smp_processor_id());

        return update_window_and_rate(dtcp, du);
}

static int rcvr_rendezvous(struct dtcp * dtcp,
                           struct du *   du)
{
        struct dtcp_ps * ps;
        int              ret;

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
	if (ps->rcvr_rendezvous)
        	ret = ps->rcvr_rendezvous(ps, &du->pci);
	else
		ret = 0;
        rcu_read_unlock();

        LOG_DBG("DTCP received Rendezvous (CPU: %d)", smp_processor_id());

        du_destroy(du);

        return ret;
}

int dtcp_common_rcv_control(struct dtcp * dtcp, struct du * du)
{
        struct dtcp_ps * ps;
        pdu_type_t       type;
        seq_num_t        sn;
        seq_num_t        last_ctrl;
        int              ret;

        atomic_inc(&dtcp->cpdus_in_transit);

        type = pci_type(&du->pci);
        if (!pdu_type_is_control(type)) {
                LOG_ERR("CommonRCVControl policy received a non-control PDU");
                atomic_dec(&dtcp->cpdus_in_transit);
                du_destroy(du);
                return -1;
        }

        /* In case EFCP address of peer has changed */
        dtcp->parent->efcp->connection->destination_address =
        		pci_source(&du->pci);

        sn = pci_sequence_number_get(&du->pci);
        spin_lock_bh(&dtcp->parent->sv_lock);
        last_ctrl = dtcp->sv->last_rcv_ctl_seq;

        if (sn <= last_ctrl) {
                switch (type) {
                case PDU_TYPE_FC:
                	dtcp->sv->flow_ctl++;
                        break;
                case PDU_TYPE_ACK:
                	dtcp->sv->acks++;
                        break;
                case PDU_TYPE_ACK_AND_FC:
                	dtcp->sv->flow_ctl++;
                	dtcp->sv->acks++;
                        break;
                default:
                        break;
                }

                spin_unlock_bh(&dtcp->parent->sv_lock);
                du_destroy(du);
                return 0;

        }
        spin_unlock_bh(&dtcp->parent->sv_lock);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        if (sn > (last_ctrl + 1)) {
                ps->lost_control_pdu(ps);
        }
        rcu_read_unlock();

        /* We are in sn >= last_ctrl + 1 */

        spin_lock_bh(&dtcp->parent->sv_lock);
        dtcp->sv->last_rcv_ctl_seq = sn;
        spin_unlock_bh(&dtcp->parent->sv_lock);

        LOG_DBG("dtcp_common_rcv_control sending to proper function...");

        switch (type) {
        case PDU_TYPE_ACK:
                ret = rcv_ack(dtcp, du);
                break;
        case PDU_TYPE_NACK:
                ret = rcv_nack_ctl(dtcp, du);
                break;
        case PDU_TYPE_FC:
                ret = rcv_flow_ctl(dtcp, du);
                break;
        case PDU_TYPE_ACK_AND_FC:
                ret = rcv_ack_and_flow_ctl(dtcp, du);
                break;
        case PDU_TYPE_RENDEZVOUS:
        	ret = rcvr_rendezvous(dtcp, du);
        	break;
        default:
                ret = -1;
                break;
        }

        atomic_dec(&dtcp->cpdus_in_transit);

        dtp_send_pending_ctrl_pdus(dtcp->parent);

        return ret;
}

/*FIXME: wrapper to be called by dtp in the post_worker */
int dtcp_sending_ack_policy(struct dtcp * dtcp)
{
        struct dtcp_ps *ps;

        if (!dtcp) {
                LOG_ERR("No DTCP passed...");
                return -1;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        rcu_read_unlock();

        return ps->sending_ack(ps, 0);
}

pdu_type_t pdu_ctrl_type_get(struct dtcp * dtcp, seq_num_t seq)
{
        struct dtcp_ps *ps;
        bool flow_ctrl;
        seq_num_t    LWE;
        timeout_t    a;

        if (!dtcp) {
                LOG_ERR("No DTCP passed...");
                return -1;
        }

        if (!dtcp->parent) {
                LOG_ERR("No DTCP parent passed...");
                return -1;
        }

        ASSERT(dtcp->cfg);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        flow_ctrl = ps->flow_ctrl;
        rcu_read_unlock();

        spin_lock_bh(&dtcp->parent->sv_lock);
        a = dtcp->parent->sv->A;
        LWE = dtcp->parent->sv->rcv_left_window_edge;

        if (dtcp->sv->last_snd_data_ack < LWE) {
        	dtcp->sv->last_snd_data_ack = LWE;
        	spin_unlock_bh(&dtcp->parent->sv_lock);

                if (!a) {
#if 0
                        if (seq > LWE) {
                                LOG_DBG("This is a NACK, "
                                        "LWE couldn't be updated");
                                if (flow_ctrl) {
                                        return PDU_TYPE_NACK_AND_FC;
                                }
                                return PDU_TYPE_NACK;
                        }
#endif
                        LOG_DBG("This is an ACK");
                        if (flow_ctrl) {
                                return PDU_TYPE_ACK_AND_FC;
                        }
                        return PDU_TYPE_ACK;
                }
#if 0
                if (seq > LWE) {
                        /* FIXME: This should be a SEL ACK */
                        LOG_DBG("This is a NACK, "
                                "LWE couldn't be updated");
                        if (flow_ctrl) {
                                return PDU_TYPE_NACK_AND_FC;
                        }
                        return PDU_TYPE_NACK;
                }
#endif
                LOG_DBG("This is an ACK");
                if (flow_ctrl) {
                        return PDU_TYPE_ACK_AND_FC;
                }
                return PDU_TYPE_ACK;
        }
        spin_unlock_bh(&dtcp->parent->sv_lock);

        return 0;
}
EXPORT_SYMBOL(pdu_ctrl_type_get);

int dtcp_ack_flow_control_pdu_send(struct dtcp * dtcp, seq_num_t seq)
{
        pdu_type_t    type;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        atomic_inc(&dtcp->cpdus_in_transit);

        type = pdu_ctrl_type_get(dtcp, seq);
        if (!type) {
                atomic_dec(&dtcp->cpdus_in_transit);
                return 0;
        }

        LOG_DBG("DTCP Sending ACK (CPU: %d)", smp_processor_id());

        return ctrl_pdu_send(dtcp, type, false);
}
EXPORT_SYMBOL(dtcp_ack_flow_control_pdu_send);

int dtcp_rendezvous_pdu_send(struct dtcp * dtcp)
{
	atomic_inc(&dtcp->cpdus_in_transit);
	LOG_DBG("DTCP Sending Rendezvous (CPU: %d)", smp_processor_id());
	return ctrl_pdu_send(dtcp, PDU_TYPE_RENDEZVOUS, true);
}
EXPORT_SYMBOL(dtcp_rendezvous_pdu_send);

static struct dtcp_sv default_sv = {
        .pdus_per_time_unit     = 0,
        .next_snd_ctl_seq       = 0,
        .last_rcv_ctl_seq       = 0,
        .last_snd_data_ack      = 0,
        .snd_lft_win            = 0,
        .data_retransmit_max    = 0,
        .last_rcv_data_ack      = 0,
        .time_unit              = 0,
        .sndr_credit            = 1,
        .snd_rt_wind_edge       = 100,
        .sndr_rate              = 0,
        .pdus_sent_in_time_unit = 0,
        .rcvr_credit            = 1,
        .rcvr_rt_wind_edge      = 100,
        .rcvr_rate              = 0,
        .pdus_rcvd_in_time_unit = 0,
        .acks                   = 0,
        .flow_ctl               = 0,
        .rtt                    = 0,
        .srtt                   = 0,
	.rendezvous_sndr        = false,
	.rendezvous_rcvr        = false,
};

/* FIXME: this should be completed with other parameters from the config */
static int dtcp_sv_init(struct dtcp * instance, struct dtcp_sv sv)
{
        struct dtcp_config * cfg;
        struct dtcp_ps * ps;

        ASSERT(instance);
        ASSERT(instance->sv);

        cfg = instance->cfg;
        if (!cfg)
                return -1;

        *instance->sv = sv;

        rcu_read_lock();
        ps = container_of(rcu_dereference(instance->base.ps),
                          struct dtcp_ps, base);
        if (ps->rtx_ctrl)
                instance->sv->data_retransmit_max =
                        ps->rtx.data_retransmit_max;

        if (ps->flow_ctrl) {
                if (ps->flowctrl.window_based) {
                        instance->sv->sndr_credit =
                                ps->flowctrl.window.initial_credit;
                        instance->sv->snd_rt_wind_edge =
                                ps->flowctrl.window.initial_credit +
                                instance->parent->sv->seq_nr_to_send;
                        instance->sv->rcvr_credit =
                                ps->flowctrl.window.initial_credit;
                        instance->sv->rcvr_rt_wind_edge =
                                ps->flowctrl.window.initial_credit;
                }
                if (ps->flowctrl.rate_based) {
                        instance->sv->sndr_rate =
                                ps->flowctrl.rate.sending_rate;

			instance->sv->rcvr_rate =
                                instance->sv->sndr_rate;
                        instance->sv->time_unit =
                                ps->flowctrl.rate.time_period;
                }
        }
        rcu_read_unlock();

        LOG_DBG("DTCP SV initialized with dtcp_conf:");
        LOG_DBG("  data_retransmit_max: %d",
                instance->sv->data_retransmit_max);
        LOG_DBG("  sndr_credit:         %u",
                instance->sv->sndr_credit);
        LOG_DBG("  snd_rt_wind_edge:    %u",
                instance->sv->snd_rt_wind_edge);
        LOG_DBG("  rcvr_credit:         %u",
                instance->sv->rcvr_credit);
        LOG_DBG("  rcvr_rt_wind_edge:   %u",
                instance->sv->rcvr_rt_wind_edge);
        LOG_DBG("  sending_rate:        %u",
                instance->sv->sndr_rate);
	LOG_DBG("  receiving_rate:      %u",
                instance->sv->rcvr_rate);
        LOG_DBG("  time_unit:           %u",
                instance->sv->time_unit);

        return 0;
}

struct dtcp_ps * dtcp_ps_get(struct dtcp * dtcp)
{
        if (!dtcp) {
                LOG_ERR("Could not retrieve DTCP PS, NULL instance passed");
                return NULL;
        }

        return container_of(rcu_dereference(dtcp->base.ps),
                            struct dtcp_ps, base);
}
EXPORT_SYMBOL(dtcp_ps_get);

struct dtcp *
dtcp_from_component(struct rina_component * component)
{
        return container_of(component, struct dtcp, base);
}
EXPORT_SYMBOL(dtcp_from_component);

int dtcp_select_policy_set(struct dtcp * dtcp,
                           const string_t * path,
                           const string_t * name)
{
        struct dtcp_config * cfg = dtcp->cfg;
        struct ps_select_transaction trans;

        if (path && strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        base_select_policy_set_start(&dtcp->base, &trans, &policy_sets, name);

        if (trans.state == PS_SEL_TRANS_PENDING) {
                struct dtcp_ps *ps;

                /* Copy the connection parameters to the policy-set. From now
                 * on these connection parameters must be accessed by the DTCP
                 * policy set, and not from the struct connection. */
                ps = container_of(trans.candidate_ps, struct dtcp_ps, base);
                ps->flow_ctrl                   = dtcp_flow_ctrl(cfg);
                ps->rtx_ctrl                    = dtcp_rtx_ctrl(cfg);
                ps->flowctrl.window_based       = dtcp_window_based_fctrl(cfg);
                ps->flowctrl.rate_based         = dtcp_rate_based_fctrl(cfg);
                ps->flowctrl.sent_bytes_th      = dtcp_sent_bytes_th(cfg);
                ps->flowctrl.sent_bytes_percent_th
                                                = dtcp_sent_bytes_percent_th(cfg);
                ps->flowctrl.sent_buffers_th    = dtcp_sent_buffers_th(cfg);
                ps->flowctrl.rcvd_bytes_th      = dtcp_rcvd_bytes_th(cfg);
                ps->flowctrl.rcvd_bytes_percent_th
                                                = dtcp_rcvd_bytes_percent_th(cfg);
                ps->flowctrl.rcvd_buffers_th    = dtcp_rcvd_buffers_th(cfg);
                ps->rtx.max_time_retry          = dtcp_max_time_retry(cfg);
                ps->rtx.data_retransmit_max     = dtcp_data_retransmit_max(cfg);
                ps->rtx.initial_tr              = dtcp_initial_tr(cfg);
                ps->flowctrl.window.max_closed_winq_length
                                                = dtcp_max_closed_winq_length(cfg);
                ps->flowctrl.window.initial_credit
                                                = dtcp_initial_credit(cfg);
                ps->flowctrl.rate.sending_rate  = dtcp_sending_rate(cfg);
                ps->flowctrl.rate.time_period   = dtcp_time_period(cfg);

                /* Fill in default policies. */
                if (!ps->lost_control_pdu) {
                        ps->lost_control_pdu = default_lost_control_pdu;
                }
#ifdef CONFIG_RINA_DTCP_RCVR_ACK
                if (!ps->rcvr_ack) {
                        ps->rcvr_ack = default_rcvr_ack;
                }
#endif /* CONFIG_RINA_DTCP_RCVR_ACK */
#ifdef CONFIG_RINA_DTCP_RCVR_ACK_ATIMER
                if (!ps->rcvr_ack_atimer) {
                        ps->rcvr_ack_atimer = default_rcvr_ack_atimer;
                }
#endif /* CONFIG_RINA_DTCP_RCVR_ACK_ATIMER */
                if (!ps->sender_ack) {
                        ps->sender_ack = default_sender_ack;
                }
                if (!ps->sending_ack) {
                        ps->sending_ack = default_sending_ack;
                }
                if (!ps->receiving_flow_control) {
                        ps->receiving_flow_control =
                                default_receiving_flow_control;
                }
                if (!ps->rcvr_flow_control) {
                        ps->rcvr_flow_control = default_rcvr_flow_control;
                }
                if (!ps->rate_reduction) {
                        ps->rate_reduction = default_rate_reduction;
                }
                if (!ps->rtt_estimator) {
                	if (ps->rtx_ctrl) {
                		ps->rtt_estimator = default_rtt_estimator;
                	} else {
                		ps->rtt_estimator = default_rtt_estimator_nortx;
                	}
                }
                if (!ps->rcvr_rendezvous) {
                	ps->rcvr_rendezvous = default_rcvr_rendezvous;
                }
        }

        base_select_policy_set_finish(&dtcp->base, &trans);

        return trans.state == PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(dtcp_select_policy_set);

int dtcp_set_policy_set_param(struct dtcp * dtcp,
                              const char * path,
                              const char * name,
                              const char * value)
{
        struct dtcp_ps *ps;
        int ret = -1;

        if (!dtcp|| !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p",
                         dtcp, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                int bool_value;

                /* The request addresses this DTCP instance. */
                rcu_read_lock();
                ps = container_of(rcu_dereference(dtcp->base.ps),
                                  struct dtcp_ps,
                                  base);
                if (strcmp(name, "flow_ctrl") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->flow_ctrl = bool_value;
                        }
                } else if (strcmp(name, "rtx_ctrl") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->rtx_ctrl = bool_value;
                        }
                } else if (strcmp(name, "flowctrl.window_based") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->flowctrl.window_based = bool_value;
                        }
                } else if (strcmp(name, "flowctrl.rate_based") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->flowctrl.rate_based = bool_value;
                        }
                } else if (strcmp(name, "flowctrl.sent_bytes_th") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.sent_bytes_th);
                } else if (strcmp(name, "flowctrl.sent_bytes_percent_th")
                                == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.sent_bytes_percent_th);
                } else if (strcmp(name, "flowctrl.sent_buffers_th") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.sent_buffers_th);
                } else if (strcmp(name, "flowctrl.rcvd_bytes_th") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.rcvd_bytes_th);
                } else if (strcmp(name, "flowctrl.rcvd_bytes_percent_th")
                                == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.rcvd_bytes_percent_th);
                } else if (strcmp(name, "flowctrl.rcvd_buffers_th") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.rcvd_buffers_th);
                } else if (strcmp(name, "rtx.max_time_retry") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->rtx.max_time_retry);
                } else if (strcmp(name, "rtx.data_retransmit_max") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->rtx.data_retransmit_max);
                } else if (strcmp(name, "rtx.initial_tr") == 0) {
                        ret = kstrtouint(value, 10, &ps->rtx.initial_tr);
                } else if (strcmp(name,
                                "flowctrl.window.max_closed_winq_length")
                                        == 0) {
                        ret = kstrtouint(value, 10,
                                &ps->flowctrl.window.max_closed_winq_length);
                } else if (strcmp(name, "flowctrl.window.initial_credit")
                                == 0) {
                        ret = kstrtouint(value, 10,
                                &ps->flowctrl.window.initial_credit);
                } else if (strcmp(name, "flowctrl.rate.sending_rate") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.rate.sending_rate);
                } else if (strcmp(name, "flowctrl.rate.time_period") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.rate.time_period);
                } else {
                        LOG_ERR("Unknown DTP parameter policy '%s'", name);
                }
                rcu_read_unlock();

        } else {
                /* The request addresses the DTP policy-set. */
                ret = base_set_policy_set_param(&dtcp->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(dtcp_set_policy_set_param);

struct dtcp * dtcp_create(struct dtp *         dtp,
                          struct rmt *         rmt,
                          struct dtcp_config * dtcp_cfg,
						  struct robject *     parent)
{
        struct dtcp * tmp;
        string_t *    ps_name;

        if (!dtp) {
                LOG_ERR("No DTP passed, bailing out");
                return NULL;
        }
        if (!dtcp_cfg) {
                LOG_ERR("No DTCP conf, bailing out");
                return NULL;
        }
        if (!rmt) {
                LOG_ERR("No RMT, bailing out");
                return NULL;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp) {
                LOG_ERR("Cannot create DTCP state-vector");
                return NULL;
        }

        tmp->parent = dtp;

	if (robject_init_and_add(&tmp->robj,
				 &dtcp_rtype,
				 parent,
				 "dtcp")) {
                dtcp_destroy(tmp);
                return NULL;
	}


        tmp->sv = rkzalloc(sizeof(*tmp->sv), GFP_ATOMIC);
        if (!tmp->sv) {
                LOG_ERR("Cannot create DTCP state-vector");
                dtcp_destroy(tmp);
                return NULL;
        }

        tmp->cfg  = dtcp_cfg;
        tmp->rmt  = rmt;
        atomic_set(&tmp->cpdus_in_transit, 0);
        rtimer_init(tf_rendezvous_rcv, &tmp->rendezvous_rcv, tmp);

        rina_component_init(&tmp->base);

        ps_name = (string_t *) policy_name(dtcp_cfg->dtcp_ps);
        if (!ps_name || !strcmp(ps_name, ""))
                ps_name = RINA_PS_DEFAULT_NAME;

        if (dtcp_select_policy_set(tmp, "", ps_name)) {
                dtcp_destroy(tmp);
                LOG_ERR("Could not load DTCP PS %s", ps_name);
                return NULL;
        }

        if (dtcp_sv_init(tmp, default_sv)) {
                LOG_ERR("Could not load DTCP config in the SV");
                dtcp_destroy(tmp);
                return NULL;
        }
        /* FIXME: fixups to the state-vector should be placed here */

        if (dtcp_flow_ctrl(dtcp_cfg)) {
		RINA_DECLARE_AND_ADD_ATTRS(&tmp->robj, dtcp, closed_win_q_length, closed_win_q_size);
                if (dtcp_window_based_fctrl(dtcp_cfg)) {
			RINA_DECLARE_AND_ADD_ATTRS(&tmp->robj, dtcp, sndr_credit, rcvr_credit,
				snd_rt_win_edge, rcv_rt_win_edge);
			/* Set closed window queue length to 1 always */
			tmp->cfg->fctrl_cfg->wfctrl_cfg->max_closed_winq_length = 1;
		}
                if (dtcp_rate_based_fctrl(dtcp_cfg)) {
			RINA_DECLARE_AND_ADD_ATTRS(&tmp->robj, dtcp, pdu_per_time_unit, time_unit,
				sndr_rate, rcvr_rate, pdus_rcvd_in_time_unit, last_time);
		}
	}
	if (dtcp_rtx_ctrl(dtcp_cfg)) {
		RINA_DECLARE_AND_ADD_ATTRS(&tmp->robj, dtcp, data_retransmit_max, last_snd_data_ack,
			last_rcv_data_ack, snd_lf_win, rtx_q_length, rtx_drop_pdus);
	}

        LOG_DBG("Instance %pK created successfully", tmp);

        return tmp;
}

int dtcp_destroy(struct dtcp * instance)
{
	int i = 0;

        /* FIXME: this is horrible*/
        while(atomic_read(&instance->cpdus_in_transit) && (i < 3)) {
        	LOG_DBG("Waiting to destroy DTCP");
        	i++;
                msleep(20);
        }

        if (!instance) {
                LOG_ERR("Bad instance passed, bailing out");
                return -1;
        }

        if (instance->sv)       rkfree(instance->sv);
        if (instance->cfg)      dtcp_config_destroy(instance->cfg);
        rtimer_destroy(&instance->rendezvous_rcv);
        rina_component_fini(&instance->base);
        robject_del(&instance->robj);
        rkfree(instance);

        LOG_DBG("Instance %pK destroyed successfully", instance);

        return 0;
}

int dtcp_sv_update(struct dtcp * dtcp, struct pci * pci)
{
        struct dtcp_ps *     ps;
        int                  retval = 0;
        bool                 flow_ctrl;
        bool                 win_based;
        bool                 rate_based;
        bool                 rtx_ctrl;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }
        if (!pci) {
                LOG_ERR("No PCI instance passed, cannot run policy");
                return -1;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        ASSERT(ps);

        flow_ctrl  = ps->flow_ctrl;
        win_based  = ps->flowctrl.window_based;
        rate_based = ps->flowctrl.rate_based;
        rtx_ctrl   = ps->rtx_ctrl;

        if (flow_ctrl) {
                if (win_based) {
                	LOG_DBG("Window based fctrl invoked");
                        if (ps->rcvr_flow_control(ps, pci)) {
                                LOG_ERR("Failed Rcvr Flow Control policy");
                                retval = -1;
                        }
                }

                if (rate_based) {
                        LOG_DBG("Rate based fctrl invoked");
                        if (ps->rate_reduction(ps, pci)) {
                                LOG_ERR("Failed Rate Reduction policy");
                                retval = -1;
                        }
                }

                if (!rtx_ctrl) {
                        LOG_DBG("Receiving flow ctrl invoked");
                        if (ps->receiving_flow_control(ps, pci)) {
                                LOG_ERR("Failed Receiving Flow Control "
                                        "policy");
                                retval = -1;
                        }

        		rcu_read_unlock();
                        return retval;
                }
        }

        if (rtx_ctrl) {
                LOG_DBG("Retransmission ctrl invoked");
                if (ps->rcvr_ack(ps, pci)) {
                        LOG_ERR("Failed Rcvr Ack policy");
                        retval = -1;
                }
        }

        rcu_read_unlock();
        return retval;
}
EXPORT_SYMBOL(dtcp_sv_update);

int dtcp_ps_publish(struct ps_factory * factory)
{
        if (factory == NULL) {
                LOG_ERR("%s: NULL factory", __func__);
                return -1;
        }

        return ps_publish(&policy_sets, factory);
}
EXPORT_SYMBOL(dtcp_ps_publish);

int dtcp_ps_unpublish(const char * name)
{ return ps_unpublish(&policy_sets, name); }
EXPORT_SYMBOL(dtcp_ps_unpublish);

/* Returns the ms represented by the timespec given.
   DEPRECATED, left for reference to show previous non-rounding behavior
unsigned long dtcp_ms(struct timespec * ts) {
	return (ts->tv_sec * 1000) + (ts->tv_nsec / 1000000);
} */

/* Is the given rate exceeded? Reset if the time frame given elapses. */
bool dtcp_rate_exceeded(struct dtcp * dtcp, int send) {
	struct timespec now  = {0, 0};
	uint_t timedif_ms;
	uint_t rate = 0;
	uint_t lim = 0;

	getnstimeofday(&now);

	timedif_ms = (int)(now.tv_sec - dtcp->sv->last_time.tv_sec) * 1000 +
		((int)now.tv_nsec - (int)dtcp->sv->last_time.tv_nsec) / 1000000;
	if (timedif_ms >= dtcp->sv->time_unit)
	{
		dtcp->sv->pdus_sent_in_time_unit = 0;
		dtcp->sv->pdus_rcvd_in_time_unit = 0;
		dtcp->sv->last_time.tv_sec = now.tv_sec;
		dtcp->sv->last_time.tv_nsec = now.tv_nsec;
	}

	if (send) {
		rate = dtcp->sv->pdus_sent_in_time_unit;
		lim = dtcp->sv->sndr_rate;
	} else {
		rate = dtcp->sv->pdus_rcvd_in_time_unit;
		lim = dtcp->sv->rcvr_rate;
	}

	if (rate >= lim)
	{
		LOG_DBG("rbfc: Rate exceeded, send: %d, rate: %d, lim: %d",
			send, rate, lim);

		return true;
	}

	return false;
}
EXPORT_SYMBOL(dtcp_rate_exceeded);
