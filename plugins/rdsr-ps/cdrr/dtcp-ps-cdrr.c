/*
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

#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/spinlock.h>

#define CDRR_NAME	"cdrr"
#define RINA_PREFIX 	"dtcp-ps-cdrr"

#include "logs.h"
#include "connection.h"
#include "dt.h"
#include "dt-utils.h"
#include "dtp.h"
#include "rds/rmem.h"
#include "dtcp-ps.h"
#include "dtcp-conf-utils.h"

/* Uncomment this to have the policy work without ECN.
 * #define CDRR_NO_ECN
 */

#define CDRR_VERBOSE_LOG
#define CDRR_LOG(x, ARGS...) LOG_INFO(" CDRR "x, ##ARGS)

#define CDRR_DEFAULT_RESET_RATE (1125000) /* ~9 Mb/s every time frame. */
#define CDRR_DEFAULT_LINK_CAPACITY (1250000) /* 10 Mb/s every time frame. */
#define CDRR_DEFAULT_TIME_FRAME	(100) /* Rate reset every 100 msec. */
#define CDRR_DEFAULT_RESET_TIME (2000) /* MSec. */

/******************************************************************************
 * Provides a mechanism to guess the possible incoming congestion and reacts to
 * it.
 ******************************************************************************/

/*#define CDRR_NO_ECN */

#ifdef CDRR_NO_ECN
#define CDRR_MAX_FLOWS 32

/* A single flow. */
struct cdrr_sf {
	/* Its source addr. */
	unsigned int source;
	/* Its port. */
	cep_id_t port;
};

/* Active flows. */
static struct cdrr_sf cdrr_flows[CDRR_MAX_FLOWS];

/* Global flows last activity. */
static struct timespec cdrr_fa[CDRR_MAX_FLOWS];

/* Active flows mgb. */
static unsigned int cdrr_mgb[CDRR_MAX_FLOWS];

/* Global lock. */
DEFINE_SPINLOCK(cdrr_lock);

static inline unsigned long cdrr_ts_to_msec(struct timespec * t) {
	return (t->tv_sec * 1000) + (t->tv_nsec / 1000000);
}
#endif

/******************************************************************************/

/* Rate that the flow will follow after the reset. Reset occurs when ECN are not
 * received for a certain time.
 */
static unsigned int cdrr_reset_rate = CDRR_DEFAULT_RESET_RATE;

/* Time after that the flow attempt to go full rate.
 */
static unsigned int cdrr_reset_time = CDRR_DEFAULT_RESET_TIME;

#ifdef CDRR_NO_ECN
/* The link capacity assumed. Bytes per seconds.
 */
 static unsigned int cdrr_link_capacity = CDRR_DEFAULT_LINK_CAPACITY;

/* Time frame to assign to the efcp connections.
 */
static unsigned int cdrr_time_frame = CDRR_DEFAULT_TIME_FRAME;
#endif

/* Size of elements currently active in the catalog.
 */
static atomic_t cdrr_flows_no = {0};

/* Count up the sum of the min. granted bw between all the flows.
 * NOTE: 64 bit could be better; we are bound to ~4kkk bytes per seconds.
 */
static atomic_t cdrr_mgb_alloc = {0};

/* Rate info. */
struct cdrr_rate_info {
	/* The DTCP bound to the rate. */
	struct dtcp * dtcp;
	/* Frame of time, in msec, after that the credit will be renewed. */
	unsigned int time_frame;
	/* Minimum granted bandwidth. */
	unsigned int mgb;
	/* Rate which will be set when ECN condition elapses. */
	unsigned int reset_rate;
	/* Time in msec after that, if no ECN are detected, the rate is
	 *  enlarged (if possible).
	 */
	unsigned int reset_time;
	/* Last moment that an ECN has been received on this dtcp. */
	struct timespec last_ecn;
};

static inline void cdrr_add_rate(struct cdrr_rate_info * ri) {
	atomic_inc(&cdrr_flows_no);
	atomic_add(ri->mgb, &cdrr_mgb_alloc);

	CDRR_LOG("Total min. granted BW is %u", atomic_read(&cdrr_mgb_alloc));
}

static inline void cdrr_rem_rate(struct cdrr_rate_info * ri) {
	atomic_dec(&cdrr_flows_no);
	atomic_sub(ri->mgb, &cdrr_mgb_alloc);

	CDRR_LOG("Total min. granted BW is %u", atomic_read(&cdrr_mgb_alloc));
}

static int cdrr_lost_control_pdu(struct dtcp_ps * ps) {
	return 0;
}

#ifdef CONFIG_RINA_DTCP_RCVR_ACK
static int cdrr_rcvr_ack(struct dtcp_ps * ps, const struct pci * pci) {
	struct dtcp * dtcp = ps->dm;
	seq_num_t     seq;

	if (!pci) {
		LOG_ERR("No PCI passed, cannot run policy");
		return -1;
	}
	seq = pci_sequence_number_get(pci);

	return dtcp_ack_flow_control_pdu_send(dtcp, seq);
}
#endif

#ifdef CONFIG_RINA_DTCP_RCVR_ACK_ATIMER
static int cdrr_rcvr_ack_atimer(struct dtcp_ps * ps, const struct pci * pci) {
	return 0;
}
#endif


static int cdrr_sender_ack(struct dtcp_ps * ps, seq_num_t seq_num) {
	struct dtcp * dtcp = ps->dm;

	if (!dtcp) {
		LOG_ERR("No instance passed, cannot run policy");
		return -1;
	}

	if (ps->rtx_ctrl) {
		struct rtxq * q;

		q = dt_rtxq(dtcp_dt(dtcp));
		if (!q) {
			LOG_ERR("Couldn't find the Retransmission queue");
			return -1;
		}
		rtxq_ack(q, seq_num, dt_sv_tr(dtcp_dt(dtcp)));
	}

	return 0;
}

static int cdrr_sending_ack(struct dtcp_ps * ps, seq_num_t seq) {
	struct dtp *       dtp;
	const struct pci * pci;

	struct dtcp * dtcp = ps->dm;

	if (!dtcp) {
		LOG_ERR("No instance passed, cannot run policy");
		return -1;
	}

	dtp = dt_dtp(dtcp_dt(dtcp));
	if (!dtp) {
		LOG_ERR("No DTP from dtcp->parent");
		return -1;
	}

	/* Invoke delimiting and update left window edge */

	pci = process_A_expiration(dtp, dtcp);

	return dtcp_sv_update(ps->dm, pci);
}

static int cdrr_receiving_flow_control(
	struct dtcp_ps * ps, const struct pci * pci) {

	struct dtcp * dtcp = ps->dm;
	struct pdu * pdu;

	if (!dtcp) {
		LOG_ERR("No instance passed, cannot run policy");
		return -1;
	}
	if (!pci) {
		LOG_ERR("No PCI passed, cannot run policy");
		return -1;
	}
	pdu = pdu_ctrl_generate(dtcp, PDU_TYPE_FC);
	if (!pdu)
		return -1;

	LOG_DBG("DTCP Sending FC (CPU: %d)", smp_processor_id());
	dump_we(dtcp, pdu_pci_get_rw(pdu));

	if (dtcp_pdu_send(dtcp, pdu))
		return -1;

	return 0;
}

static int cdrr_rcvr_flow_control(struct dtcp_ps * ps, const struct pci * pci) {
        struct dtcp * dtcp = ps->dm;
        seq_num_t LWE;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }
        if (!pci) {
                LOG_ERR("No PCI passed, cannot run policy");
                return -1;
        }

        LWE = dt_sv_rcv_lft_win(dtcp_dt(dtcp));
        update_rt_wind_edge(dtcp);

        LOG_DBG("DTCP: %pK", dtcp);
        LOG_DBG("LWE: %u  RWE: %u", LWE, rcvr_rt_wind_edge(dtcp));

        return 0;
}

static int cdrr_rate_reduction(struct dtcp_ps * ps, const struct pci * pci) {
	struct cdrr_rate_info * ri = (struct cdrr_rate_info *)ps->priv;
	struct dtcp * dtcp = ri->dtcp;

	struct timespec ts = {0, 0};

#ifdef CDRR_NO_ECN
	int i;
	int f = 0; // Found?
	unsigned int dif = 0;
	unsigned int pr = 0;
#endif

	getnstimeofday(&ts);

	// Do not proceed on non-data pdu.
	if(!(pci_type(pci) & PDU_TYPE_DT)) {
		return 0;
	}

#ifdef CDRR_NO_ECN
/* ---- LOCK ---------------------------------------------------------------- */
	spin_lock(&cdrr_lock);

	/* Update the last activity of this flow. */
	for(i = 0; i < CDRR_MAX_FLOWS; i++) {
		/* Not an empty seat. */
		if(cdrr_flows[i].source != 0) {
			/* We are already there. */
			if(cdrr_flows[i].source == pci_source(pci) &&
				cdrr_flows[i].port == pci_cep_source(pci)) {

				/* Update our last activity. */
				cdrr_fa[i].tv_sec  = ts.tv_sec;
				cdrr_fa[i].tv_nsec = ts.tv_nsec;

				f = 1;
				break;
			}
			/* That is not us. Keep searching.... */
			else {
				continue;
			}
		}
	}

	/* Flow not yet handled? Insert! */
	if(!f) {
		for(i = 0; i < CDRR_MAX_FLOWS; i++) {
			/* Take the first empty seat. */
			if(cdrr_flows[i].source == 0) {
				cdrr_flows[i].source = pci_source(pci);
				cdrr_flows[i].port   = pci_cep_source(pci);

				/* Set our first activity. */
				cdrr_fa[i].tv_sec  = ts.tv_sec;
				cdrr_fa[i].tv_nsec = ts.tv_nsec;

				/* First packet incoming will save the mgb. */
				cdrr_mgb[i] = ri->mgb;

				CDRR_LOG("Guessing heuristic added flow, "
					"source: %d, cep: %u, mgb: %u",
					cdrr_flows[i].source,
					cdrr_flows[i].port,
					cdrr_mgb[i]);

				break;
			}
		}
	}

	/* Build up the potential incoming rate.
	 * If no messages are received from reset_time then we consider that
	 * flow as no-emitting/dead.
	 */
	for(i = 0; i < CDRR_MAX_FLOWS; i++) {
		if(cdrr_flows[i].source != 0) {
			dif = cdrr_ts_to_msec(&ts) -
				cdrr_ts_to_msec(&cdrr_fa[i]);

			if(dif <= cdrr_reset_time) {
				/* More than one cause problems! */
				pr += cdrr_reset_rate;
			} else {
				/* We consider them as dead. */
				cdrr_flows[i].source = 0;
				cdrr_flows[i].port   = 0;
			}
		}
	}

	spin_unlock(&cdrr_lock);
/* ---- UNLOCK -------------------------------------------------------------- */

	if(pr > cdrr_link_capacity) {
		dif = ri->mgb;

		if(dtcp_sndr_rate(dtcp) != dif) {
			CDRR_LOG("Congestion guessed! Adjusting 0x%pK to %u",
				dtcp, dif);

			dtcp_sndr_rate_set(dtcp, dif);
			dtcp_time_frame_set(dtcp, cdrr_time_frame);
		}
	} else {
		dif = cdrr_reset_rate;

		if(dtcp_sndr_rate(dtcp) != dif) {
			CDRR_LOG("No congestion! Adjusting 0x%pK to %u",
				dtcp, dif);

			dtcp_sndr_rate_set(dtcp, dif);
			dtcp_time_frame_set(dtcp, cdrr_time_frame);
		}
	}
#else
	/*
	 * Congestion detected!
	 */
	if(pci_flags_get(pci) & PDU_FLAGS_EXPLICIT_CONGESTION) {
		/* Update when the last ECN has been received. */
		ri->last_ecn.tv_sec  = ts.tv_sec;
		ri->last_ecn.tv_nsec = ts.tv_nsec;

#ifdef CDRR_VERBOSE_LOG
		if(dtcp_sndr_rate(dtcp) != ri->mgb) {
			CDRR_LOG("Congestion detected! Switch to min. granted"
				"bandwidth for 0x%pK", dtcp);
		}
#endif

		/* Adapt to the min. granted bandwidth. */
		dtcp_sndr_rate_set(dtcp, ri->mgb);
		dtcp_time_frame_set(dtcp, CDRR_DEFAULT_TIME_FRAME);
	}
	/*
	 * No congestion case here!
	 */
	else {
		/* Already set, stop. */
		if(dtcp_sndr_rate(dtcp) == ri->reset_rate) {
			return 0;
		}

		/* Insert here any(if necessary) time-based default adjustment.
		 */

		CDRR_LOG("Reset rating! Switch to max for 0x%pK", dtcp);

		/* Adapt to the min. granted bandwidth. */
		dtcp_sndr_rate_set(dtcp, ri->reset_rate);
		dtcp_time_frame_set(dtcp, CDRR_DEFAULT_TIME_FRAME);
	}
#endif

	return 0;
}

static int cdrr_rtt_estimator(struct dtcp_ps * ps, seq_num_t sn) {
	struct dtcp *       dtcp;
	struct dt *         dt;
	uint_t              rtt, new_rtt, srtt, rttvar, trmsecs;
	timeout_t           start_time;
	int                 abs;
	struct rtxq_entry * entry;

	if (!ps)
		return -1;
	dtcp = ps->dm;
	if (!dtcp)
		return -1;
	dt = dtcp_dt(dtcp);
	if (!dt)
		return -1;

	LOG_DBG("RTT Estimator...");

	entry = rtxq_entry_peek(dt_rtxq(dt), sn);
	if (!entry) {
		LOG_ERR("Could not retrieve timestamp of Seq num: %u for RTT "
			"estimation", sn);
		return -1;
	}

	/* if it is a retransmission we do not consider it*/
	if (rtxq_entry_retries(entry) != 0) {
		LOG_DBG("RTTestimator PDU %u has been retransmitted %u",
			sn, rtxq_entry_retries(entry));
		rtxq_entry_destroy(entry);
		return 0;
	}

	start_time = rtxq_entry_timestamp(entry);
	new_rtt    = jiffies_to_msecs(jiffies - start_time);

	/* NOTE: the acking process has alrady deleted old entries from rtxq
	 * except for the one with the sn we need, here we have to detroy just
	 * the one we use */
	rtxq_entry_destroy(entry);

	rtt        = dtcp_rtt(dtcp);
	srtt       = dtcp_srtt(dtcp);
	rttvar     = dtcp_rttvar(dtcp);

	if (!rtt) {
		rttvar = new_rtt >> 1;
		srtt   = new_rtt;
	} else {
		abs = srtt - new_rtt;
		abs = abs < 0 ? -abs : abs;
		rttvar = ((3 * rttvar) >> 2) + (((uint_t)abs) >> 2);
	}

	/*FIXME: k, G, alpha and betha should be parameters passed to the policy
	 * set. Probably moving them to ps->priv */

	/* k * rttvar = 4 * rttvar */
	trmsecs  = rttvar << 2;
	/* G is 0.1s according to RFC6298, then 100ms */
	trmsecs  = 100 > trmsecs ? 100 : trmsecs;
	trmsecs += srtt + jiffies_to_msecs(dt_sv_a(dt));
	/* RTO (tr) less than 1s? (not for the common policy) */
	/*trmsecs  = trmsecs < 1000 ? 1000 : trmsecs;*/

	dtcp_rtt_set(dtcp, new_rtt);
	dtcp_rttvar_set(dtcp, rttvar);
	dtcp_srtt_set(dtcp, srtt);
	dt_sv_tr_set(dt, msecs_to_jiffies(trmsecs));
	LOG_DBG("TR set to %u msecs", trmsecs);

	return 0;
}

static int cdrr_param(
	struct ps_base * bps, const char * name, const char * value) {

	struct dtcp_ps * ps = container_of(bps, struct dtcp_ps, base);
	struct cdrr_rate_info * ri = ps->priv;

	int ret = 0;
	int ival = 0;

	if (strcmp(name, "mgb") == 0) {
		ret = kstrtoint(value, 10, &ival);
		if (!ret) {
			ri->mgb = ival;
		}
	}

	if (strcmp(name, "reset_rate") == 0) {
		ret = kstrtoint(value, 10, &ival);
		if (!ret) {
			ri->reset_rate = ival;
		}
	}


	return 0;
	/*return dtcp_ps_common_set_policy_set_param(bps, name, value); */
}

/*
 * Creation/destruction of the policy.
 */

static struct ps_base * cdrr_create(struct rina_component * component) {
        struct dtcp * dtcp = dtcp_from_component(component);
        struct dtcp_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        struct cdrr_rate_info * ri = 0;

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param   = cdrr_param;
        ps->dm                          = dtcp;

        ps->flow_init                   = NULL;
        ps->lost_control_pdu            = cdrr_lost_control_pdu;
        ps->rtt_estimator               = cdrr_rtt_estimator;
        ps->retransmission_timer_expiry = NULL;
        ps->received_retransmission     = NULL;
        ps->sender_ack                  = cdrr_sender_ack;
        ps->sending_ack                 = cdrr_sending_ack;
        ps->receiving_ack_list          = NULL;
        ps->initial_rate                = NULL;
        ps->receiving_flow_control      = cdrr_receiving_flow_control;
        ps->update_credit               = NULL;
#ifdef CONFIG_RINA_DTCP_RCVR_ACK
        ps->rcvr_ack                    = cdrr_rcvr_ack,
#endif
#ifdef CONFIG_RINA_DTCP_RCVR_ACK_ATIMER
        ps->rcvr_ack                    = cdrr_rcvr_ack_atimer,
#endif
        ps->rcvr_flow_control           = cdrr_rcvr_flow_control;
        ps->rate_reduction              = cdrr_rate_reduction;
        ps->rcvr_control_ack            = NULL;
        ps->no_rate_slow_down           = NULL;
        ps->no_override_default_peak    = NULL;

        ri = rkzalloc(sizeof(struct cdrr_rate_info), GFP_KERNEL);

        if(!ri) {
        	CDRR_LOG("No more memory to create new rate info.");
        	return 0;
        }

        ps->priv = ri;

        ri->reset_time = cdrr_reset_time;
        ri->reset_rate = cdrr_reset_rate;
        ri->dtcp = dtcp;
        ri->mgb  = dtcp_sending_rate(dtcp_config_get(dtcp));
        ri->time_frame = dtcp_time_period(dtcp_config_get(dtcp));

        cdrr_add_rate(ri);

        CDRR_LOG("Created a new DTCP instance, "
		"dtcp: 0x%pK, "
		"mgb: %u, "
		"time frame: %u msec, "
		"reset rate: %u, "
		"reset time: %u",
		ri->dtcp,
		ri->mgb,
		ri->time_frame,
		ri->reset_rate,
		ri->reset_time);

        return &ps->base;
}

static void cdrr_destroy(struct ps_base * bps) {
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);
        struct cdrr_rate_info * ri = (struct cdrr_rate_info *)ps->priv;
        struct dtcp * dtcp = ri->dtcp;

        if(ri) {
		cdrr_rem_rate(ri);
		dtcp = ri->dtcp;

		CDRR_LOG("Removing a rate info, dtcp: 0x%pK", ri->dtcp);
		rkfree(ri);
	}

        if (bps) {
                rkfree(ps);
        }

	return;
}

struct ps_factory dtcp_factory = {
        .owner	 = THIS_MODULE,
        .create  = cdrr_create,
        .destroy = cdrr_destroy,
};

/*
 * Module initialization/release.
 */

static int __init mod_init(void) {
	int ret = 0;

#ifdef CDRR_NO_ECN
	memset(cdrr_flows, 0, sizeof(struct cdrr_sf) * CDRR_MAX_FLOWS);
	memset(cdrr_mgb, 0, sizeof(unsigned int) * CDRR_MAX_FLOWS);
	memset(cdrr_fa, 0, sizeof(struct timespec) * CDRR_MAX_FLOWS);
#endif

        strcpy(dtcp_factory.name, CDRR_NAME);

        CDRR_LOG("CDRR policy set loaded successfully");

        ret = dtcp_ps_publish(&dtcp_factory);
        if (ret) {
        	CDRR_LOG("Failed to publish CDRR policy set factory");
                return -1;
        }

        return 0;
}

static void __exit mod_exit(void) {
        int ret;

        ret = dtcp_ps_unpublish(CDRR_NAME);

        if (ret) {
        	CDRR_LOG("Failed to unpublish CDRR policy set factory");
                return;
        }
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Congestion Detection Rate Reduction policies");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kewin Rausch <kewin.rausch@create-net.org>");
MODULE_VERSION("1");
