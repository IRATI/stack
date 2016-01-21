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
#include "dtp.h"
#include "dt-utils.h"
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

/* This is the DT-SV part maintained by DTCP */
struct dtcp_sv {
        /* SV lock */
        spinlock_t   lock;

        /* TimeOuts */
        /*
         * When flow control is rate based this timeout may be
         * used to pace number of PDUs sent in TimeUnit
         */
        uint_t       pdus_per_time_unit;

        /* Sequencing */

        /*
         * Outbound: NextSndCtlSeq contains the Sequence Number to
         * be assigned to a control PDU
         */
        seq_num_t    next_snd_ctl_seq;

        /*
         * Inbound: LastRcvCtlSeq - Sequence number of the next
         * expected // Transfer(? seems an error in the spec’s
         * doc should be Control) PDU received on this connection
         */
        seq_num_t    last_rcv_ctl_seq;

        /*
         * Retransmission: There’s no retransmission queue,
         * when a lost PDU is detected a new one is generated
         */

        /* Outbound */
        seq_num_t    last_snd_data_ack;

        /*
         * Seq number of the lowest seq number expected to be
         * Acked. Seq number of the first PDU on the
         * RetransmissionQ. My LWE thus.
         */
        seq_num_t    snd_lft_win;

        /*
         * Maximum number of retransmissions of PDUs without a
         * positive ack before declaring an error
         */
        uint_t       data_retransmit_max;

        /* Inbound */
        seq_num_t    last_rcv_data_ack;

        /* Time (ms) over which the rate is computed */
        uint_t       time_unit;

        /* Flow Control State */

        /* Outbound */
        uint_t       sndr_credit;

        /* snd_rt_wind_edge = LastSendDataAck + PDU(credit) */
        seq_num_t    snd_rt_wind_edge;

        /* PDUs per TimeUnit */
        uint_t       sndr_rate;

        /* PDUs already sent in this time unit */
        uint_t       pdus_sent_in_time_unit;

        /* Inbound */

        /*
         * PDUs receiver believes sender may send before extending
         * credit or stopping the flow on the connection
         */
        uint_t       rcvr_credit;

        /* Value of credit in this flow */
        seq_num_t    rcvr_rt_wind_edge;

        /*
         * Current rate receiver has told sender it may send PDUs
         * at.
         */
        uint_t       rcvr_rate;

        /*
         * PDUs received in this time unit. When it equals
         * rcvr_rate, receiver is allowed to discard any PDUs
         * received until a new time unit begins
         */
        uint_t       pdus_rcvd_in_time_unit;

        /* Rate based both in and out-bound */

	/* Last time-instant when the credit check has been done.
	 * This is used by rate-based flow control mechanism.
	 */
	struct timespec last_time;

        /*
         * Control of duplicated control PDUs
         * */
        uint_t       acks;
        uint_t       flow_ctl;

        /* RTT estimation */
        uint_t       rtt;
        uint_t       srtt;
        uint_t       rttvar;
};

struct dtcp {
        struct dt *            parent;

        /*
         * NOTE: The DTCP State Vector can be discarded during long periods of
         *       no traffic
         */
        struct dtcp_sv *       sv; /* The state-vector */
        struct rina_component  base;
        struct dtcp_config *   cfg;
        struct rmt *           rmt;

        atomic_t               cpdus_in_transit;
	struct robject         robj;
};

struct dt * dtcp_dt(struct dtcp * dtcp)
{
        return dtcp->parent;
}
EXPORT_SYMBOL(dtcp_dt);

struct dtcp_config * dtcp_config_get(struct dtcp * dtcp)
{
        if (!dtcp)
                return NULL;
        return dtcp->cfg;
}
EXPORT_SYMBOL(dtcp_config_get);

int dtcp_pdu_send(struct dtcp * dtcp, struct pdu * pdu)
{
        ASSERT(dtcp);
        ASSERT(dtcp->rmt);
        ASSERT(dtcp->parent);

        return dt_pdu_send(dtcp->parent,
                           dtcp->rmt,
                           pdu);
}
EXPORT_SYMBOL(dtcp_pdu_send);

static uint_t dtcp_pdus_per_time_unit(struct dtcp * dtcp)
{
	uint_t ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	ret = dtcp->sv->pdus_per_time_unit;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return ret;
}

static uint_t dtcp_time_unit(struct dtcp * dtcp)
{
	uint_t ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	ret = dtcp->sv->time_unit;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return ret;
}

uint_t dtcp_time_frame(struct dtcp * dtcp)
{
	uint_t ret = 0;
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return 0;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	ret = dtcp->sv->time_unit;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return ret;
}
EXPORT_SYMBOL(dtcp_time_frame);

int dtcp_time_frame_set(struct dtcp * dtcp, uint_t sec)
{
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return -1;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	dtcp->sv->time_unit = sec;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return 0;
}
EXPORT_SYMBOL(dtcp_time_frame_set);

int dtcp_last_time(struct dtcp * dtcp, struct timespec * s)
{
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return -1;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	s->tv_sec = dtcp->sv->last_time.tv_sec;
	s->tv_nsec = dtcp->sv->last_time.tv_nsec;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return 0;
}
EXPORT_SYMBOL(dtcp_last_time);

int dtcp_last_time_set(struct dtcp * dtcp, struct timespec * s)
{
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return -1;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	dtcp->sv->last_time.tv_sec = s->tv_sec;
	dtcp->sv->last_time.tv_nsec = s->tv_nsec;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return 0;
}
EXPORT_SYMBOL(dtcp_last_time_set);

uint_t dtcp_sndr_rate(struct dtcp * dtcp)
{
	uint_t ret;
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return 0;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	ret = dtcp->sv->sndr_rate;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return ret;
}
EXPORT_SYMBOL(dtcp_sndr_rate);

int dtcp_sndr_rate_set(struct dtcp * dtcp, uint_t rate)
{
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return -1;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	dtcp->sv->sndr_rate = rate;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return 0;
}
EXPORT_SYMBOL(dtcp_sndr_rate_set);

uint_t dtcp_rcvr_rate(struct dtcp * dtcp)
{
	uint_t ret;
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return 0;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	ret = dtcp->sv->rcvr_rate;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return ret;
}
EXPORT_SYMBOL(dtcp_rcvr_rate);

int dtcp_rcvr_rate_set(struct dtcp * dtcp, uint_t rate)
{
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return -1;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	dtcp->sv->rcvr_rate = rate;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return 0;
}
EXPORT_SYMBOL(dtcp_rcvr_rate_set);

uint_t dtcp_recv_itu(struct dtcp * dtcp)
{
	uint_t ret = 0;
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return 0;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	ret = dtcp->sv->pdus_rcvd_in_time_unit;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return ret;
}
EXPORT_SYMBOL(dtcp_recv_itu);

int dtcp_recv_itu_set(struct dtcp * dtcp, uint_t recv)
{
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return -1;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	dtcp->sv->pdus_rcvd_in_time_unit = recv;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return 0;
}
EXPORT_SYMBOL(dtcp_recv_itu_set);

int dtcp_recv_itu_inc(struct dtcp * dtcp, uint_t recv)
{
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return -1;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	dtcp->sv->pdus_rcvd_in_time_unit += recv;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return 0;
}
EXPORT_SYMBOL(dtcp_recv_itu_inc);

uint_t dtcp_sent_itu(struct dtcp * dtcp)
{
	uint_t ret = 0;
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return 0;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	ret = dtcp->sv->pdus_sent_in_time_unit;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return ret;
}
EXPORT_SYMBOL(dtcp_sent_itu);

int dtcp_sent_itu_set(struct dtcp * dtcp, uint_t sent)
{
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return -1;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	dtcp->sv->pdus_sent_in_time_unit = sent;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return 0;
}
EXPORT_SYMBOL(dtcp_sent_itu_set);

int dtcp_sent_itu_inc(struct dtcp * dtcp, uint_t sent)
{
	unsigned long flags;

	if (!dtcp || !dtcp->sv)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK.",
			__FUNCTION__,
			dtcp);

		return -1;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	dtcp->sv->pdus_sent_in_time_unit += sent;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return 0;
}
EXPORT_SYMBOL(dtcp_sent_itu_inc);

int dtcp_rate_fc_reset(struct dtcp * dtcp, struct timespec * now)
{
	unsigned long flags;

	if (!dtcp || !dtcp->sv || !now)
	{
		LOG_DBG("%s, Wrong arguments; dtcp: %pK, now: %pK.",
			__FUNCTION__,
			dtcp,
			now);

		return -1;
	}

	spin_lock_irqsave(&dtcp->sv->lock, flags);
	dtcp->sv->pdus_sent_in_time_unit = 0;
	dtcp->sv->pdus_rcvd_in_time_unit = 0;
	dtcp->sv->last_time.tv_sec = now->tv_sec;
	dtcp->sv->last_time.tv_nsec = now->tv_nsec;
	spin_unlock_irqrestore(&dtcp->sv->lock, flags);

	return 0;
}
EXPORT_SYMBOL(dtcp_rate_fc_reset);

uint_t dtcp_rtt(struct dtcp * dtcp)
{
        unsigned long flags;
        uint_t        tmp;

        if (!dtcp || !dtcp->sv)
                return 0;

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->rtt;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dtcp_rtt);

int dtcp_rtt_set(struct dtcp * dtcp, uint_t rtt)
{
        unsigned long flags;

        if (!dtcp || !dtcp->sv)
                return -1;

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->rtt = rtt;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return 0;
}
EXPORT_SYMBOL(dtcp_rtt_set);

uint_t dtcp_srtt(struct dtcp * dtcp)
{
        unsigned long flags;
        uint_t        tmp;

        if (!dtcp || !dtcp->sv)
                return 0;

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->srtt;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dtcp_srtt);

int dtcp_srtt_set(struct dtcp * dtcp, uint_t srtt)
{
        unsigned long flags;

        if (!dtcp || !dtcp->sv)
                return -1;

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->srtt = srtt;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return 0;
}
EXPORT_SYMBOL(dtcp_srtt_set);

uint_t dtcp_rttvar(struct dtcp * dtcp)
{
        unsigned long flags;
        uint_t        tmp;

        if (!dtcp || !dtcp->sv)
                return 0;

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->rttvar;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dtcp_rttvar);

int dtcp_rttvar_set(struct dtcp * dtcp, uint_t rttvar)
{
        unsigned long flags;

        if (!dtcp || !dtcp->sv)
                return -1;

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->rtt = rttvar;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return 0;
}
EXPORT_SYMBOL(dtcp_rttvar_set);

static int last_rcv_ctrl_seq_set(struct dtcp * dtcp,
                                 seq_num_t     last_rcv_ctrl_seq)
{
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->last_rcv_ctl_seq = last_rcv_ctrl_seq;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return 0;
}

seq_num_t last_rcv_ctrl_seq(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->last_rcv_ctl_seq;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(last_rcv_ctrl_seq);

static void flow_ctrl_inc(struct dtcp * dtcp)
{
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->flow_ctl++;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
}

static void acks_inc(struct dtcp * dtcp)
{
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->acks++;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
}

static int snd_rt_wind_edge_set(struct dtcp * dtcp, seq_num_t new_rt_win)
{
        unsigned long flags;
        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->snd_rt_wind_edge = new_rt_win;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return 0;
}

seq_num_t snd_rt_wind_edge(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->snd_rt_wind_edge;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(snd_rt_wind_edge);

seq_num_t snd_lft_win(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->snd_lft_win;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(snd_lft_win);

seq_num_t rcvr_rt_wind_edge(struct dtcp * dtcp)
{
        unsigned long flags;
        seq_num_t     tmp;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->rcvr_rt_wind_edge;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(rcvr_rt_wind_edge);

int pdus_sent_in_t_unit_set(struct dtcp * dtcp, uint_t s)
{
        unsigned long flags;

        if (!dtcp || !dtcp->sv) {
                LOG_ERR("Bogus DTCP instance");
                return -1;
        }

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->pdus_sent_in_time_unit = s;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return 0;
}
EXPORT_SYMBOL(pdus_sent_in_t_unit_set);

static seq_num_t next_snd_ctl_seq(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;


        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = ++dtcp->sv->next_snd_ctl_seq;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}

static seq_num_t last_snd_data_ack(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->last_snd_data_ack;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}

static seq_num_t last_rcv_data_ack(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->last_rcv_data_ack;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}

static void last_snd_data_ack_set(struct dtcp * dtcp, seq_num_t seq_num)
{
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->last_snd_data_ack = seq_num;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
}

static uint_t dtcp_sndr_credit(struct dtcp * dtcp) {
        unsigned long flags;
        seq_num_t credit;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);
        spin_lock_irqsave(&dtcp->sv->lock, flags);
        credit = dtcp->sv->sndr_credit;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
        return credit;
}

uint_t dtcp_rcvr_credit(struct dtcp * dtcp) {
        unsigned long flags;
        seq_num_t credit;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);
        spin_lock_irqsave(&dtcp->sv->lock, flags);
        credit = dtcp->sv->rcvr_credit;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
        return credit;
}
EXPORT_SYMBOL(dtcp_rcvr_credit);

void dtcp_rcvr_credit_set(struct dtcp * dtcp, uint_t credit)
{
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->rcvr_credit = credit;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
}
EXPORT_SYMBOL(dtcp_rcvr_credit_set);

void update_rt_wind_edge(struct dtcp * dtcp)
{
        seq_num_t     seq;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        seq = dt_sv_rcv_lft_win(dtcp->parent);
        spin_lock_irqsave(&dtcp->sv->lock, flags);
        seq += dtcp->sv->rcvr_credit;
        dtcp->sv->rcvr_rt_wind_edge = seq;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
}
EXPORT_SYMBOL(update_rt_wind_edge);

void update_credit_and_rt_wind_edge(struct dtcp * dtcp, uint_t credit)
{
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->rcvr_credit = credit;
	/* applying the TCP rule of not shrinking the window */
	if (dt_sv_rcv_lft_win(dtcp->parent) + credit > dtcp->sv->rcvr_rt_wind_edge)
        	dtcp->sv->rcvr_rt_wind_edge = dt_sv_rcv_lft_win(dtcp->parent) + credit;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
}
EXPORT_SYMBOL(update_credit_and_rt_wind_edge);

static ssize_t dtcp_attr_show(struct robject *		     robj,
                         	     struct robj_attribute * attr,
                                     char *		     buf)
{
	struct dtcp * instance;

	instance = container_of(robj, struct dtcp, robj);
	if (!instance || !instance->sv || !instance->parent || !instance->cfg)
		return 0;

	if (strcmp(robject_attr_name(attr), "rtt") == 0) {
		return sprintf(buf, "%u\n", dtcp_rtt(instance));
	}
	if (strcmp(robject_attr_name(attr), "srtt") == 0) {
		return sprintf(buf, "%u\n", dtcp_srtt(instance));
	}
	if (strcmp(robject_attr_name(attr), "rttvar") == 0) {
		return sprintf(buf, "%u\n", dtcp_rttvar(instance));
	}
	/* Flow control */
	if (strcmp(robject_attr_name(attr), "closed_win_q_length") == 0) {
		return sprintf(buf, "%zu\n", cwq_size(dt_cwq(instance->parent)));
	}
	if (strcmp(robject_attr_name(attr), "closed_win_q_size") == 0) {
		return sprintf(buf, "%u\n",
			dtcp_max_closed_winq_length(instance->cfg));
	}
	/* Win based */
	if (strcmp(robject_attr_name(attr), "sndr_credit") == 0) {
		return sprintf(buf, "%u\n", dtcp_sndr_credit(instance));
	}
	if (strcmp(robject_attr_name(attr), "rcvr_credit") == 0) {
		return sprintf(buf, "%u\n", dtcp_rcvr_credit(instance));
	}
	if (strcmp(robject_attr_name(attr), "snd_rt_win_edge") == 0) {
		return sprintf(buf, "%u\n", dtcp_snd_rt_win(instance));
	}
	if (strcmp(robject_attr_name(attr), "rcv_rt_win_edge") == 0) {
		return sprintf(buf, "%u\n", dtcp_rcv_rt_win(instance));
	}
	/* Rate based */
	if (strcmp(robject_attr_name(attr), "pdus_per_time_unit") == 0) {
		return sprintf(buf, "%u\n", dtcp_pdus_per_time_unit(instance));
	}
	if (strcmp(robject_attr_name(attr), "time_unit") == 0) {
		return sprintf(buf, "%u\n", dtcp_time_unit(instance));
	}
	if (strcmp(robject_attr_name(attr), "sndr_rate") == 0) {
		return sprintf(buf, "%u\n", dtcp_sndr_rate(instance));
	}
	if (strcmp(robject_attr_name(attr), "rcvr_rate") == 0) {
		return sprintf(buf, "%u\n", dtcp_rcvr_rate(instance));
	}
	if (strcmp(robject_attr_name(attr), "pdus_rcvd_in_time_unit") == 0) {
		return sprintf(buf, "%u\n", dtcp_recv_itu(instance));
	}
	if (strcmp(robject_attr_name(attr), "last_time") == 0) {
		struct timespec s;
		dtcp_last_time(instance, &s);
		return sprintf(buf, "%lld.%.9ld\n",
			(long long) s.tv_sec, s.tv_nsec);
	}
	/* Rtx control */
	if (strcmp(robject_attr_name(attr), "data_retransmit_max") == 0) {
		return sprintf(buf, "%u\n", dtcp_data_retransmit_max(instance->cfg));
	}
	if (strcmp(robject_attr_name(attr), "last_snd_data_ack") == 0) {
		return sprintf(buf, "%u\n", last_snd_data_ack(instance));
	}
	if (strcmp(robject_attr_name(attr), "last_rcv_data_ack") == 0) {
		return sprintf(buf, "%u\n", last_rcv_data_ack(instance));
	}
	if (strcmp(robject_attr_name(attr), "snd_lf_win") == 0) {
		return sprintf(buf, "%u\n", dtcp_snd_lf_win(instance));
	}
	if (strcmp(robject_attr_name(attr), "rtx_q_length") == 0) {
		return sprintf(buf, "%u\n", rtxq_size(dt_rtxq(instance->parent)));
	}
	if (strcmp(robject_attr_name(attr), "rtx_drop_pdus") == 0) {
		return sprintf(buf, "%u\n",
			rtxq_drop_pdus(dt_rtxq(instance->parent)));
	}
	if (strcmp(robject_attr_name(attr), "ps_name") == 0) {
		return sprintf(buf, "%s\n",instance->base.ps_factory->name);
	}
	return 0;
}
RINA_SYSFS_OPS(dtcp);
RINA_ATTRS(dtcp, rtt, srtt, rttvar, ps_name);
RINA_KTYPE(dtcp);

static int push_pdus_rmt(struct dtcp * dtcp)
{
        struct cwq *  q;

        ASSERT(dtcp);

        q = dt_cwq(dtcp->parent);
        if (!q) {
                LOG_ERR("No Closed Window Queue");
                return -1;
        }

        cwq_deliver(q,
                    dtcp->parent,
                    dtcp->rmt);

        return 0;
}

struct pdu * pdu_ctrl_create_ni(struct dtcp * dtcp, pdu_type_t type)
{
        struct pdu *    pdu;
        struct pci *    pci;
        struct buffer * buffer;
        seq_num_t       seq;
        struct efcp *   efcp;

        if (!pdu_type_is_control(type))
                return NULL;

        efcp = dt_efcp(dtcp->parent);
        if (!efcp) {
                LOG_ERR("Passed instance has no EFCP, bailing out");
                return NULL;
        }

        buffer = buffer_create_ni(1);
        if (!buffer)
                return NULL;

        pdu = pdu_create_ni();
        if (!pdu) {
                buffer_destroy(buffer);
                return NULL;
        }

        pci = pci_create_ni();
        if (!pci) {
                pdu_destroy(pdu);
                return NULL;
        }

        seq = next_snd_ctl_seq(dtcp);
        if (pci_format(pci,
                       efcp_src_cep_id(efcp),
                       efcp_dst_cep_id(efcp),
                       efcp_src_addr(efcp),
                       efcp_dst_addr(efcp),
                       seq,
                       efcp_qos_id(efcp),
                       type)) {
                pdu_destroy(pdu);
                pci_destroy(pci);
                return NULL;
        }

        if (pci_control_last_seq_num_rcvd_set(pci,last_rcv_ctrl_seq(dtcp))) {
                pci_destroy(pci);
                pdu_destroy(pdu);
                return NULL;
        }

        if (pdu_pci_set(pdu, pci)) {
                pdu_destroy(pdu);
                pci_destroy(pci);
                return NULL;
        }

        if (pdu_buffer_set(pdu, buffer)) {
                pdu_destroy(pdu);
                return NULL;
        }

        return pdu;
}
EXPORT_SYMBOL(pdu_ctrl_create_ni);

static int populate_ctrl_pci(struct pci *  pci,
                             struct dtcp * dtcp)
{
        struct dtcp_config * dtcp_cfg;
        seq_num_t snd_lft;
        seq_num_t snd_rt;
        seq_num_t LWE;
        uint_t rt;
        uint_t tf;

        dtcp_cfg = dtcp_config_get(dtcp);
        if (!dtcp_cfg) {
                LOG_ERR("No dtcp cfg...");
                return -1;
        }

        /*
         * FIXME: Shouldn't we check if PDU_TYPE_ACK_AND_FC or
         * PDU_TYPE_NACK_AND_FC ?
         */
        LWE = dt_sv_rcv_lft_win(dtcp->parent);

        if (dtcp_flow_ctrl(dtcp_cfg)) {
                if (dtcp_window_based_fctrl(dtcp_cfg)) {
                        snd_lft = snd_lft_win(dtcp);
                        snd_rt  = snd_rt_wind_edge(dtcp);

                        pci_control_new_left_wind_edge_set(pci, LWE);
                        pci_control_new_rt_wind_edge_set(pci,
				rcvr_rt_wind_edge(dtcp));

                        pci_control_my_left_wind_edge_set(pci, snd_lft);
                        pci_control_my_rt_wind_edge_set(pci, snd_rt);
                }

                if (dtcp_rate_based_fctrl(dtcp_cfg)) {
                	rt = dtcp_sndr_rate(dtcp);
                	tf = dtcp_time_frame(dtcp);

                	LOG_DBG("Populating control pci with rate "
                		"settings, rate: %u, time: %u",
                		rt, tf);

                        pci_control_sndr_rate_set(pci, rt);
                        pci_control_time_frame_set(pci, tf);
                }
        }

        switch (pci_type(pci)) {
        case PDU_TYPE_ACK_AND_FC:
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
        default:
                break;
        }

        return 0;
}

struct pdu * pdu_ctrl_generate(struct dtcp * dtcp, pdu_type_t type)
{
        struct pdu * pdu;
        struct pci * pci;

        if (!dtcp || !type) {
                LOG_ERR("wrong parameters, can't generate ctrl PDU...");
                return NULL;
        }
        pdu  = pdu_ctrl_create_ni(dtcp, type);
        if (!pdu) {
                LOG_ERR("No Ctrl PDU created...");
                return NULL;
        }

        pci = pdu_pci_get_rw(pdu);
        if (populate_ctrl_pci(pci, dtcp)) {
                LOG_ERR("Could not populate ctrl PCI");
                pdu_destroy(pdu);
                return NULL;
        }

        return pdu;
}
EXPORT_SYMBOL(pdu_ctrl_generate);

void dump_we(struct dtcp * dtcp, struct pci *  pci)
{
        struct dtp * dtp;
        seq_num_t    snd_rt_we;
        seq_num_t    snd_lf_we;
        seq_num_t    rcv_rt_we;
        seq_num_t    rcv_lf_we;
        seq_num_t    new_rt_we;
        seq_num_t    new_lf_we;
        seq_num_t    pci_seqn;
        seq_num_t    my_rt_we;
        seq_num_t    my_lf_we;
        seq_num_t    ack;
        uint_t	     rate;
        uint_t	     tframe;

        ASSERT(dtcp);
        ASSERT(pci);

        dtp = dt_dtp(dtcp->parent);
        ASSERT(dtp);

        snd_rt_we = snd_rt_wind_edge(dtcp);
        snd_lf_we = dtcp_snd_lf_win(dtcp);
        rcv_rt_we = rcvr_rt_wind_edge(dtcp);
        rcv_lf_we = dt_sv_rcv_lft_win(dtcp->parent);
        new_rt_we = pci_control_new_rt_wind_edge(pci);
        new_lf_we = pci_control_new_left_wind_edge(pci);
        my_lf_we  = pci_control_my_left_wind_edge(pci);
        my_rt_we  = pci_control_my_rt_wind_edge(pci);
        pci_seqn  = pci_sequence_number_get(pci);
        ack       = pci_control_ack_seq_num(pci);
        rate	  = pci_control_sndr_rate(pci);
        tframe	  = pci_control_time_frame(pci);

        LOG_DBG("SEQN: %u N/Ack: %u SndRWE: %u SndLWE: %u "
                "RcvRWE: %u RcvLWE: %u "
                "newRWE: %u newLWE: %u "
                "myRWE: %u myLWE: %u "
                "rate: %u tframe: %u",
                pci_seqn, ack, snd_rt_we, snd_lf_we, rcv_rt_we, rcv_lf_we,
                new_rt_we, new_lf_we, my_rt_we, my_lf_we,
                rate, tframe);
}
EXPORT_SYMBOL(dump_we);

/* not a policy according to specs */
static int rcv_nack_ctl(struct dtcp * dtcp,
                        struct pdu *  pdu)
{
        struct rtxq *    q;
        struct dtcp_ps * ps;
        seq_num_t        seq_num;
        struct pci *     pci;

        pci     = pdu_pci_get_rw(pdu);
        seq_num = pci_control_ack_seq_num(pci);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);

        if (ps->rtx_ctrl) {
                q = dt_rtxq(dtcp->parent);
                if (!q) {
                        rcu_read_unlock();
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }
                rtxq_nack(q, seq_num, dt_sv_tr(dtcp->parent));
		if (ps->rtt_estimator)
                	ps->rtt_estimator(ps, pci_control_ack_seq_num(pci));
        }
        rcu_read_unlock();

        LOG_DBG("DTCP received NACK (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pci);

        pdu_destroy(pdu);
        return 0;
}

static int rcv_ack(struct dtcp * dtcp,
                   struct pdu *  pdu)
{
        struct dtcp_ps * ps;
        seq_num_t        seq;
        int              ret;
        struct pci *     pci;

        ASSERT(dtcp);
        ASSERT(pdu);

        pci = pdu_pci_get_rw(pdu);
        ASSERT(pci);

        seq = pci_control_ack_seq_num(pci);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        ret = ps->sender_ack(ps, seq);
	if (ps->rtt_estimator)
        	ps->rtt_estimator(ps, pci_control_ack_seq_num(pci));
        rcu_read_unlock();

        LOG_DBG("DTCP received ACK (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pci);

        pdu_destroy(pdu);
        return ret;
}

static int rcv_flow_ctl(struct dtcp * dtcp,
                        struct pdu *  pdu)
{
        struct pci * pci;
        uint_t 	     rt;
        uint_t       tf;

        ASSERT(dtcp);
        ASSERT(pdu);

        pci = pdu_pci_get_rw(pdu);
        ASSERT(pci);

        rt = pci_control_sndr_rate(pci);
        tf = pci_control_time_frame(pci);

        if(dtcp_window_based_fctrl(dtcp_config_get(dtcp))) {
        	snd_rt_wind_edge_set(dtcp, pci_control_new_rt_wind_edge(pci));
        }

        if(dtcp_rate_based_fctrl(dtcp_config_get(dtcp))) {
		if(tf && rt) {
			dtcp_sndr_rate_set(dtcp, rt);
			dtcp_time_frame_set(dtcp, tf);

			LOG_DBG("rbfc Rate based fields sets on flow ctl, "
				"rate: %u, time: %u",
				rt, tf);
		}
        }

        push_pdus_rmt(dtcp);

        LOG_DBG("DTCP received FC (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pci);

        pdu_destroy(pdu);
        return 0;
}

static int rcv_ack_and_flow_ctl(struct dtcp * dtcp,
                                struct pdu *  pdu)
{
        struct dtcp_ps * ps;
        seq_num_t        seq;
        struct pci *     pci;
        uint_t		 rt;
        uint_t           tf;

        ASSERT(dtcp);

        pci = pdu_pci_get_rw(pdu);
        ASSERT(pci);

        LOG_DBG("Updating Window Edges for DTCP: %pK", dtcp);

        seq = pci_control_ack_seq_num(pci);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        /* This updates sender LWE */
        if (ps->sender_ack(ps, seq))
                LOG_ERR("Could not update RTXQ and LWE");
	if (ps->rtx_ctrl && ps->rtt_estimator)
        	ps->rtt_estimator(ps, pci_control_ack_seq_num(pci));
        rcu_read_unlock();

	if(dtcp_window_based_fctrl(dtcp_config_get(dtcp))) {
		snd_rt_wind_edge_set(dtcp, pci_control_new_rt_wind_edge(pci));
		LOG_DBG("Right Window Edge: %u", snd_rt_wind_edge(dtcp));
	}

	if(dtcp_rate_based_fctrl(dtcp_config_get(dtcp))) {
	        rt = pci_control_sndr_rate(pci);
	        tf = pci_control_time_frame(pci);

	        if(tf && rt) {
			dtcp_sndr_rate_set(dtcp, rt);
			dtcp_time_frame_set(dtcp, tf);
			LOG_DBG("Rate based fields sets on flow ctl and "
				"ack, rate: %u, time: %u",
				rt, tf);
	        }
	}

        LOG_DBG("Calling CWQ_deliver for DTCP: %pK", dtcp);
        push_pdus_rmt(dtcp);

        /* FIXME: Verify values for the receiver side */
        LOG_DBG("DTCP received ACK-FC (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pci);

        pdu_destroy(pdu);
        return 0;
}

int dtcp_common_rcv_control(struct dtcp * dtcp, struct pdu * pdu)
{
        struct dtcp_ps * ps;
        struct pci *     pci;
        pdu_type_t       type;
        seq_num_t        sn;
        seq_num_t        last_ctrl;
        int              ret;

        LOG_DBG("dtcp_common_rcv_control called");

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("PDU is not ok");
                pdu_destroy(pdu);
                return -1;
        }

        if (!dtcp) {
                LOG_ERR("DTCP instance bogus");
                pdu_destroy(pdu);
                return -1;
        }

        atomic_inc(&dtcp->cpdus_in_transit);

        pci = pdu_pci_get_rw(pdu);
        if (!pci_is_ok(pci)) {
                LOG_ERR("PCI couldn't be retrieved");
                atomic_dec(&dtcp->cpdus_in_transit);
                pdu_destroy(pdu);
                return -1;
        }

        type = pci_type(pci);

        if (!pdu_type_is_control(type)) {
                LOG_ERR("CommonRCVControl policy received a non-control PDU");
                atomic_dec(&dtcp->cpdus_in_transit);
                pdu_destroy(pdu);
                return -1;
        }

        sn = pci_sequence_number_get(pci);
        last_ctrl = last_rcv_ctrl_seq(dtcp);

        if (sn <= last_ctrl) {
                switch (type) {
                case PDU_TYPE_FC:
                        flow_ctrl_inc(dtcp);
                        break;
                case PDU_TYPE_ACK:
                        acks_inc(dtcp);
                        break;
                case PDU_TYPE_ACK_AND_FC:
                        acks_inc(dtcp);
                        flow_ctrl_inc(dtcp);
                        break;
                default:
                        break;
                }

                pdu_destroy(pdu);
                return 0;

        }
        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        if (sn > (last_ctrl + 1)) {
                ps->lost_control_pdu(ps);
        }
        rcu_read_unlock();

        /* We are in sn >= last_ctrl + 1 */

        last_rcv_ctrl_seq_set(dtcp, sn);

        LOG_DBG("dtcp_common_rcv_control sending to proper function...");

        switch (type) {
        case PDU_TYPE_ACK:
                ret = rcv_ack(dtcp, pdu);
                break;
        case PDU_TYPE_NACK:
                ret = rcv_nack_ctl(dtcp, pdu);
                break;
        case PDU_TYPE_FC:
                ret = rcv_flow_ctl(dtcp, pdu);
                break;
        case PDU_TYPE_ACK_AND_FC:
                ret = rcv_ack_and_flow_ctl(dtcp, pdu);
                break;
        default:
                ret = -1;
                break;
        }

        atomic_dec(&dtcp->cpdus_in_transit);
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
        struct dtcp_config * dtcp_cfg;
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

        dtcp_cfg = dtcp_config_get(dtcp);
        ASSERT(dtcp_cfg);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        flow_ctrl = ps->flow_ctrl;
        rcu_read_unlock();

        a = dt_sv_a(dtcp->parent);

        LWE = dt_sv_rcv_lft_win(dtcp->parent);
        if (last_snd_data_ack(dtcp) < LWE) {
                last_snd_data_ack_set(dtcp, LWE);
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

        return 0;
}
EXPORT_SYMBOL(pdu_ctrl_type_get);

int dtcp_ack_flow_control_pdu_send(struct dtcp * dtcp, seq_num_t seq)
{
        struct pdu *   pdu;
        pdu_type_t     type;

        seq_num_t      dbg_seq_num;

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

        pdu  = pdu_ctrl_generate(dtcp, type);
        if (!pdu) {
                atomic_dec(&dtcp->cpdus_in_transit);
                return -1;
        }

        dbg_seq_num = pci_sequence_number_get(pdu_pci_get_rw(pdu));

        LOG_DBG("DTCP Sending ACK (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pdu_pci_get_rw(pdu));

        if (dtcp_pdu_send(dtcp, pdu)){
                atomic_dec(&dtcp->cpdus_in_transit);
                return -1;
        }

        atomic_dec(&dtcp->cpdus_in_transit);

        return 0;
}
EXPORT_SYMBOL(dtcp_ack_flow_control_pdu_send);

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
};

/* FIXME: this should be completed with other parameters from the config */
static int dtcp_sv_init(struct dtcp * instance, struct dtcp_sv sv)
{
        struct dtcp_config * cfg;
        struct dtcp_ps * ps;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!instance->sv) {
                LOG_ERR("Bogus sv passed");
                return -1;
        }

        cfg = dtcp_config_get(instance);
        if (!cfg)
                return -1;

        *instance->sv = sv;
        spin_lock_init(&instance->sv->lock);

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
                                dtp_sv_last_nxt_seq_nr(dt_dtp(instance->parent));
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
                        ps->rtt_estimator = default_rtt_estimator;
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

struct dtcp * dtcp_create(struct dt *          dt,
                          struct rmt *         rmt,
                          struct dtcp_config * dtcp_cfg,
			  struct robject *     parent)
{
        struct dtcp * tmp;
        string_t *    ps_name;

        if (!dt) {
                LOG_ERR("No DT passed, bailing out");
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

        tmp->parent = dt;

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

        rina_component_init(&tmp->base);

        ps_name = (string_t *) policy_name(dtcp_ps(dtcp_cfg));
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
        /* FIXME: this is horrible*/
        while(atomic_read(&instance->cpdus_in_transit))
                msleep(20);

        if (!instance) {
                LOG_ERR("Bad instance passed, bailing out");
                return -1;
        }

        if (instance->sv)       rkfree(instance->sv);
        if (instance->cfg)      dtcp_config_destroy(instance->cfg);
        rina_component_fini(&instance->base);
	robject_del(&instance->robj);
        rkfree(instance);

        LOG_DBG("Instance %pK destroyed successfully", instance);

        return 0;
}

int dtcp_sv_update(struct dtcp * dtcp, const struct pci * pci)
{
        struct dtcp_ps *     ps;
        int                  retval = 0;
        struct dtcp_config * dtcp_cfg;
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

        dtcp_cfg = dtcp_config_get(dtcp);
        if (!dtcp_cfg)
                return -1;

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

seq_num_t dtcp_rcv_rt_win(struct dtcp * dtcp)
{
        if (!dtcp || !dtcp->sv)
                return -1;

        return rcvr_rt_wind_edge(dtcp);
}
EXPORT_SYMBOL(dtcp_rcv_rt_win);

seq_num_t dtcp_snd_rt_win(struct dtcp * dtcp)
{
        if (!dtcp || !dtcp->sv)
                return -1;

        return snd_rt_wind_edge(dtcp);
}
EXPORT_SYMBOL(dtcp_snd_rt_win);

int dtcp_snd_rt_win_set(struct dtcp * dtcp, seq_num_t rt_win_edge)
{
        if (!dtcp || !dtcp->sv)
                return -1;

        return snd_rt_wind_edge_set(dtcp, rt_win_edge);
}
EXPORT_SYMBOL(dtcp_snd_rt_win_set);

seq_num_t dtcp_snd_lf_win(struct dtcp * dtcp)
{
        if (!dtcp || !dtcp->sv)
                return -1;

        return snd_lft_win(dtcp);
}

int dtcp_snd_lf_win_set(struct dtcp * instance, seq_num_t seq_num)
{
        unsigned long flags;

        if (!instance || !instance->sv)
                return -1;

        spin_lock_irqsave(&instance->sv->lock, flags);
        instance->sv->snd_lft_win = seq_num;
        spin_unlock_irqrestore(&instance->sv->lock, flags);

        return 0;
}
EXPORT_SYMBOL(dtcp_snd_lf_win_set);

int dtcp_rcv_rt_win_set(struct dtcp * instance, seq_num_t seq_num)
{
        unsigned long flags;

        if (!instance || !instance->sv)
                return -1;

        spin_lock_irqsave(&instance->sv->lock, flags);
        instance->sv->rcvr_rt_wind_edge = seq_num;
        spin_unlock_irqrestore(&instance->sv->lock, flags);

        return 0;
}
EXPORT_SYMBOL(dtcp_rcv_rt_win_set);

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

/* Returns the ms represented by the timespec given. */
unsigned long dtcp_ms(struct timespec * ts) {
	return (ts->tv_sec * 1000) + (ts->tv_nsec / 1000000);
}

/* Is the given rate exceeded? Reset if the time frame given elapses. */
bool dtcp_rate_exceeded(struct dtcp * dtcp, int send) {
	struct timespec now  = {0, 0};
	struct timespec last = {0, 0};
	struct timespec sub  = {0, 0};
	uint_t rate = 0;
	uint_t lim = 0;

	dtcp_last_time(dtcp, &last);
	getnstimeofday(&now);
	sub = timespec_sub(now, last);

	if (dtcp_ms(&sub) >= dtcp_time_frame(dtcp))
	{
		dtcp_rate_fc_reset(dtcp, &now);
	}

	if (send) {
		rate = dtcp_sent_itu(dtcp);
		lim = dtcp_sndr_rate(dtcp);
	} else {
		rate = dtcp_recv_itu(dtcp);
		lim = dtcp_rcvr_rate(dtcp);
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
