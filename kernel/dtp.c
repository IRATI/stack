/*
 * DTP (Data Transfer Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#include <linux/random.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define RINA_PREFIX "dtp"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtp-ps-default.h"
#include "dtp-utils.h"
#include "dtcp.h"
#include "dtcp-ps.h"
#include "dtp-conf-utils.h"
#include "dtcp-conf-utils.h"
#include "ps-factory.h"
#include "dtp-ps.h"
#include "policies.h"
#include "rds/ringq.h"
#include "pci.h"
#include "rds/robjects.h"
#include "efcp-str.h"

#define TO_POST_LENGTH 1000
#define TO_SEND_LENGTH 16

static struct policy_set_list policy_sets = {
        .head = LIST_HEAD_INIT(policy_sets.head)
};

static struct dtp_sv default_sv = {
        .seq_nr_to_send                = 0,
        .max_seq_nr_sent               = 0,
        .seq_number_rollover_threshold = 0,
        .max_seq_nr_rcv                = 0,
	.stats = {
		.drop_pdus = 0,
		.err_pdus = 0,
		.tx_pdus = 0,
		.tx_bytes = 0,
		.rx_pdus = 0,
		.rx_bytes = 0 },
        .rexmsn_ctrl                   = false,
        .rate_based                    = false,
        .window_based                  = false,
        .drf_required                  = true,
        .rate_fulfiled                 = false,
        .max_flow_pdu_size    = UINT_MAX,
        .max_flow_sdu_size    = UINT_MAX,
        .MPL                  = 1000,
        .R                    = 100,
        .A                    = 0,
        .tr                   = 0,
        .rcv_left_window_edge = 0,
        .window_closed        = false,
        .drf_flag             = true,
};

#define stats_get(name, sv, retval)				\
        ASSERT(sv);						\
        retval = sv->stats.name;				\

#define stats_inc(name, sv)					\
        ASSERT(sv);						\
        sv->stats.name##_pdus++;				\
        LOG_DBG("PDUs __STRINGIFY(name) %u",			\
		sv->stats.name##_pdus);				\

#define stats_inc_bytes(name, sv, bytes)			\
        ASSERT(sv);						\
        sv->stats.name##_pdus++;				\
	sv->stats.name##_bytes += (unsigned long) bytes;	\
        LOG_DBG("PDUs __STRINGIFY(name) %u (%u)",		\
		sv->stats.name##_pdus,				\
		sv->stats.name##_bytes);			\

static ssize_t dtp_attr_show(struct robject *		     robj,
                         	     struct robj_attribute * attr,
                                     char *		     buf)
{
	struct dtp * instance;
	unsigned int stats_ret;

	instance = container_of(robj, struct dtp, robj);
	if (!instance || !instance->cfg || !instance->sv)
		return 0;

	if (strcmp(robject_attr_name(attr), "init_a_timer") == 0) {
		return sprintf(buf, "%u\n",
			dtp_conf_initial_a_timer(instance->cfg));
	}
	if (strcmp(robject_attr_name(attr), "max_sdu_gap") == 0) {
		return sprintf(buf, "%u\n",
			dtp_conf_max_sdu_gap(instance->cfg));
	}
	if (strcmp(robject_attr_name(attr), "partial_delivery") == 0) {
		return sprintf(buf, "%u\n",
			dtp_conf_partial_del(instance->cfg) ? 1 : 0);
	}
	if (strcmp(robject_attr_name(attr), "incomplete_delivery") == 0) {
		return sprintf(buf, "%u\n",
			dtp_conf_incomplete_del(instance->cfg) ? 1 : 0);
	}
	if (strcmp(robject_attr_name(attr), "in_order_delivery") == 0) {
		return sprintf(buf, "%u\n",
			dtp_conf_in_order_del(instance->cfg) ? 1 : 0);
	}
	if (strcmp(robject_attr_name(attr), "seq_num_rollover_th") == 0) {
		return sprintf(buf, "%d\n",
			dtp_conf_seq_num_ro_th(instance->cfg));
	}
	if (strcmp(robject_attr_name(attr), "drop_pdus") == 0) {
		spin_lock_bh(&instance->sv_lock);
		stats_get(drop_pdus, instance->sv, stats_ret);
		spin_unlock_bh(&instance->sv_lock);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "err_pdus") == 0) {
		spin_lock_bh(&instance->sv_lock);
		stats_get(err_pdus, instance->sv, stats_ret);
		spin_unlock_bh(&instance->sv_lock);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "tx_pdus") == 0) {
		spin_lock_bh(&instance->sv_lock);
		stats_get(tx_pdus, instance->sv, stats_ret);
		spin_unlock_bh(&instance->sv_lock);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "tx_bytes") == 0) {
		spin_lock_bh(&instance->sv_lock);
		stats_get(tx_bytes, instance->sv, stats_ret);
		spin_unlock_bh(&instance->sv_lock);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "rx_pdus") == 0) {
		spin_lock_bh(&instance->sv_lock);
		stats_get(rx_pdus, instance->sv, stats_ret);
		spin_unlock_bh(&instance->sv_lock);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "rx_bytes") == 0) {
		spin_lock_bh(&instance->sv_lock);
		stats_get(rx_bytes, instance->sv, stats_ret);
		spin_unlock_bh(&instance->sv_lock);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "ps_name") == 0) {
		return sprintf(buf, "%s\n", instance->base.ps_factory->name);
	}
	return 0;
}
RINA_SYSFS_OPS(dtp);
RINA_ATTRS(dtp, init_a_timer, max_sdu_gap, partial_delivery,
		    incomplete_delivery, in_order_delivery, seq_num_rollover_th,
		    drop_pdus, err_pdus, tx_pdus, tx_bytes, rx_pdus, rx_bytes,
		    ps_name);
RINA_KTYPE(dtp);

int dtp_initial_sequence_number(struct dtp * instance)
{
        struct dtp_ps *ps;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(instance->base.ps),
                          struct dtp_ps, base);
        ASSERT(ps->initial_sequence_number);
        if (ps->initial_sequence_number(ps)) {
                rcu_read_unlock();
                return -1;
        }
        rcu_read_unlock();

        return 0;
}

/* Sequencing/reassembly queue */

struct seq_queue {
        struct list_head head;
};

struct seq_queue_entry {
        unsigned long    time_stamp;
        struct du *      du;
        struct list_head next;
};

struct squeue {
        struct dtp *       dtp;
        struct seq_queue * queue;
};

static struct seq_queue * seq_queue_create(void)
{
        struct seq_queue * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->head);

        return tmp;
}

static struct seq_queue_entry * seq_queue_entry_create_gfp(struct du *  du,
                                                           gfp_t        flags)
{
        struct seq_queue_entry * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->next);

        tmp->du = du;
        tmp->time_stamp = jiffies;

        return tmp;
}

static void seq_queue_entry_destroy(struct seq_queue_entry * seq_entry)
{
        ASSERT(seq_entry);

        if (seq_entry->du) du_destroy(seq_entry->du);
        rkfree(seq_entry);

        return;
}

static int seq_queue_destroy(struct seq_queue * seq_queue)
{
        struct seq_queue_entry * cur, * n;

        ASSERT(seq_queue);

        list_for_each_entry_safe(cur, n, &seq_queue->head, next) {
                list_del(&cur->next);
                seq_queue_entry_destroy(cur);
        }

        rkfree(seq_queue);

        return 0;
}

void dtp_squeue_flush(struct dtp * dtp)
{
        struct seq_queue_entry * cur, * n;
        struct seq_queue *       seq_queue;

        if (!dtp)
                return;

        ASSERT(dtp->seqq);

        seq_queue = dtp->seqq->queue;

        list_for_each_entry_safe(cur, n, &seq_queue->head, next) {
                list_del(&cur->next);
                seq_queue_entry_destroy(cur);
        }

        return;
}

static struct du * seq_queue_pop(struct seq_queue * q)
{
        struct seq_queue_entry * p;
        struct du *              du;

        if (list_empty(&q->head)) {
                LOG_DBG("Seq Queue is empty!");
                return NULL;
        }

        p = list_first_entry(&q->head, struct seq_queue_entry, next);
        if (!p)
                return NULL;

        du = p->du;
        p->du = NULL;

        list_del(&p->next);
        seq_queue_entry_destroy(p);

        return du;
}

static int seq_queue_push_ni(struct seq_queue * q, struct du * du)
{
        static struct seq_queue_entry * tmp, * cur, * last = NULL;
        seq_num_t                       csn, psn;

        tmp = seq_queue_entry_create_gfp(du, GFP_ATOMIC);
        if (!tmp) {
                LOG_ERR("Could not create sequencing queue entry");
                return -1;
        }

        csn = pci_sequence_number_get(&du->pci);
        if (list_empty(&q->head)) {
                list_add(&tmp->next, &q->head);
                LOG_DBG("First PDU with seqnum: %u push to seqq at: %pk",
                        csn, q);
                return 0;
        }

        last = list_last_entry(&q->head, struct seq_queue_entry, next);
        if (!last) {
                LOG_ERR("Could not retrieve last pdu from seqq");
                seq_queue_entry_destroy(tmp);
                return 0;
        }

        psn  = pci_sequence_number_get(&last->du->pci);
        if (csn == psn) {
                LOG_ERR("Another PDU with the same seq_num is in the seqq");
                seq_queue_entry_destroy(tmp);
                return -1;
        }

        if (csn > psn) {
                list_add_tail(&tmp->next, &q->head);
                LOG_DBG("Last PDU with seqnum: %u push to seqq at: %pk",
                        csn, q);
                return 0;
        }

        list_for_each_entry(cur, &q->head, next) {
                psn = pci_sequence_number_get(&cur->du->pci);
                if (csn == psn) {
                        LOG_ERR("Another PDU with the same seq_num is in the rtx queue!");
                        seq_queue_entry_destroy(tmp);
                        return 0;
                }
                if (csn > psn) {
                        list_add(&tmp->next, &cur->next);
                        LOG_DBG("Middle PDU with seqnum: %u push to "
                                "seqq at: %pk", csn, q);
                        return 0;
                }
        }

        return 0;
}

static int squeue_destroy(struct squeue * seqq)
{
        if (!seqq)
                return -1;

        if (seqq->queue) seq_queue_destroy(seqq->queue);

        rkfree(seqq);

        return 0;
}

static struct squeue * squeue_create(struct dtp * dtp)
{
        struct squeue * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->queue = seq_queue_create();
        if (!tmp->queue) {
                squeue_destroy(tmp);
                return NULL;
        }

        tmp->dtp = dtp;

        return tmp;
}

static inline int pdu_post(struct dtp * instance,
                    	   struct du * du)
{
        struct efcp *   efcp;

        efcp = instance->efcp;

        if (efcp_enqueue(efcp, efcp->connection->port_id, du)) {
        	LOG_ERR("Could not enqueue SDU to EFCP");
        	return -1;
        }

        LOG_DBG("DTP enqueued to upper IPCP");
        return 0;
}

/* Runs the SenderInactivityTimerPolicy */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
static void tf_sender_inactivity(void * data)
#else
static void tf_sender_inactivity(struct timer_list * tl)
#endif
{
        struct dtp * dtp;
        struct dtp_ps * ps;

        LOG_DBG("Running Stimer...");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        dtp = (struct dtp *) data;
#else
        dtp = from_timer(dtp, tl, timers.sender_inactivity);
#endif
        if (!dtp) {
                LOG_ERR("No dtp to work with");
                return;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtp->base.ps), struct dtp_ps, base);
        if (!ps->sender_inactivity_timer) {
                LOG_ERR("No DTP sender inactivity policy");
                rcu_read_unlock();
                return;
        }

        if (ps->sender_inactivity_timer(ps))
                LOG_ERR("Problems executing the sender inactivity policy");
        rcu_read_unlock();

        return;
}

/* Runs the ReceiverInactivityTimerPolicy */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
static void tf_receiver_inactivity(void * data)
#else
static void tf_receiver_inactivity(struct timer_list * tl)
#endif
{
        struct dtp * dtp;
        struct dtp_ps * ps;

        LOG_DBG("Running Rtimer...");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        dtp = (struct dtp *) data;
#else
        dtp = from_timer(dtp, tl, timers.receiver_inactivity);
#endif
        if (!dtp) {
                LOG_ERR("No dtp to work with");
                return;
        }
        rcu_read_lock();
        ps = container_of(rcu_dereference(dtp->base.ps), struct dtp_ps, base);
        if (!ps->receiver_inactivity_timer) {
                LOG_ERR("No DTP sender inactivity policy");
                rcu_read_unlock();
                return;
        }

        if (ps->receiver_inactivity_timer(ps))
                LOG_ERR("Problems executing receiver inactivity policy");
        rcu_read_unlock();

        return;
}

/* Runs the Rendezvous timer */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
static void tf_rendezvous(void * data)
#else
static void tf_rendezvous(struct timer_list * tl)
#endif
{
        struct dtp * dtp;
        bool         start_rv_timer;
        timeout_t    rv;

        LOG_DBG("Running rendezvous timer...");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        dtp = (struct dtp *) data;
#else
	dtp = from_timer(dtp, tl, timers.rendezvous);
#endif
        if (!dtp) {
                LOG_ERR("No dtp to work with");
                return;
        }

	/* Check if rendezvous PDU needs to be send*/
	start_rv_timer = false;
	spin_lock_bh(&dtp->sv_lock);
	if (dtp->dtcp->sv->rendezvous_sndr) {
		/* Start rendezvous timer, wait for Tr to fire */
		start_rv_timer = true;
		rv = jiffies_to_msecs(dtp->sv->tr);
	}
	spin_unlock_bh(&dtp->sv_lock);

	if (start_rv_timer) {
		/* Send rendezvous PDU and start timer */
		dtcp_rendezvous_pdu_send(dtp->dtcp);
		rtimer_start(&dtp->timers.rendezvous, rv);
	}

        return;
}

/*
 * NOTE:
 *   AF is the factor to which A is divided in order to obtain the
 *   period of the A-timer: Ta = A / AF
 */
#define AF 1
/*
 * FIXME: removed the static so that dtcp's sending_ack policy can use this
 * function. This has to be refactored and evaluate how much code would be
 * repeated
 */
struct pci * process_A_expiration(struct dtp * dtp, struct dtcp * dtcp)
{
        struct dtp_sv *          sv;
        struct squeue *          seqq;
        seq_num_t                LWE;
        seq_num_t                seq_num;
        struct du *              du;
        bool                     in_order_del;
        bool                     incomplete_del;
        bool                     rtx_ctrl = false;
        bool			 a_timer_expired;
        seq_num_t                max_sdu_gap;
        timeout_t                a;
        struct seq_queue_entry * pos, * n;
        struct dtp_ps *          ps;
        struct dtcp_ps *         dtcp_ps;
        struct pci *             pci_ret = NULL;

        ASSERT(dtp);

        sv = dtp->sv;
        seqq = dtp->seqq;

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtp->base.ps), struct dtp_ps, base);
        in_order_del   = ps->in_order_delivery;
        incomplete_del = ps->incomplete_delivery;
        max_sdu_gap    = ps->max_sdu_gap;

        if (dtcp) {
                dtcp_ps = dtcp_ps_get(dtcp);
                rtx_ctrl = dtcp_ps->rtx_ctrl;
        }
        rcu_read_unlock();

        spin_lock_bh(&dtp->sv_lock);
        a = msecs_to_jiffies(dtp->sv->A);

        /* FIXME: Invoke delimiting */

        LOG_DBG("Processing A timer expiration");

        LWE = dtp->sv->rcv_left_window_edge;
        LOG_DBG("LWEU: Original LWE = %u", LWE);
        LOG_DBG("LWEU: MAX GAPS     = %u", max_sdu_gap);

        list_for_each_entry_safe(pos, n, &seqq->queue->head, next) {
                du = pos->du;
                seq_num = pci_sequence_number_get(&du->pci);
                LOG_DBG("Seq number: %u", seq_num);

                a_timer_expired = time_before_eq(pos->time_stamp + a, jiffies);

                if (a_timer_expired || (seq_num - LWE - 1 <= max_sdu_gap)) {
                        if (a_timer_expired &&
                        		dtcp_rtx_ctrl(dtcp->cfg)) {
                                LOG_DBG("Retransmissions will be required");
                                list_del(&pos->next);
                                seq_queue_entry_destroy(pos);
                                continue;
                        }

                	dtp->sv->rcv_left_window_edge = seq_num;
                        pos->du = NULL;
                        list_del(&pos->next);
                        seq_queue_entry_destroy(pos);

                        if (ringq_push(dtp->to_post, du)) {
                                LOG_ERR("Could not post PDU %u while A timer"
                                        "(in-order)", seq_num);
                        }

                        LOG_DBG("Atimer: PDU %u posted", seq_num);

                        LWE = seq_num;
                        pci_ret = &du->pci;
                        continue;
                }

                break;

        }

        spin_unlock_bh(&dtp->sv_lock);

        if (pci_get(pci_ret))
		LOG_ERR("Could not get a reference to PCI");

        while (!ringq_is_empty(dtp->to_post)) {
                du = (struct du *) ringq_pop(dtp->to_post);
                if (du)
                        pdu_post(dtp, du);
        }

        LOG_DBG("Finish process_Atimer_expiration");
        return pci_ret;
}
EXPORT_SYMBOL(process_A_expiration);

static bool seqq_is_empty(struct squeue * queue)
{
        bool ret;

        if (!queue)
                return false;

        spin_lock(&queue->dtp->sv_lock);
        ret = list_empty(&queue->queue->head) ? true : false;
        spin_unlock(&queue->dtp->sv_lock);

        return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
static void tf_a(void * o)
#else
static void tf_a(struct timer_list * tl)
#endif
{
        struct dtp *  dtp;
        struct dtcp * dtcp;
        timeout_t     a;
        timeout_t     r;
        timeout_t     mpl;
        struct pci * pci;

        LOG_DBG("A-timer handler started...");

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        dtp = (struct dtp *) o;
#else
        dtp = from_timer(dtp, tl, timers.a);
#endif
        if (!dtp) {
                LOG_ERR("No instance passed to A-timer handler !!!");
                return;
        }

        dtcp = dtp->dtcp;

        spin_lock_bh(&dtp->sv_lock);
        a = dtp->sv->A;
        r = dtp->sv->R;
        mpl = dtp->sv->MPL;
        spin_unlock_bh(&dtp->sv_lock);

        if (dtcp) {
                if (dtcp_sending_ack_policy(dtcp)){
                        LOG_ERR("sending_ack failed");
                        rtimer_start(&dtp->timers.a, a/AF);
                }

                dtp_send_pending_ctrl_pdus(dtp);
        } else {
                pci = process_A_expiration(dtp, dtcp);

                if (pci)
                	pci_release(pci);

                if (rtimer_restart(&dtp->timers.sender_inactivity,
                                   3 * (mpl + r + a))) {
                        LOG_ERR("Failed to start sender_inactiviy timer");
                        rtimer_start(&dtp->timers.a, a/AF);
                        return;
                }
        }

        if (!seqq_is_empty(dtp->seqq)) {
                LOG_DBG("Going to restart A timer with a = %d and a/AF = %d",
                        a, a/AF);
                rtimer_start(&dtp->timers.a, a/AF);
        }

        return;
}

static unsigned int msec_to_next_rate(uint time_frame, struct timespec * last)
{
	struct timespec now = {0,0};

	if(!last) {
		LOG_ERR("%s arg invalid, last: 0x%pK", __func__, last);
		return -1;
	}

// Algorithm:
// next = last + timeframe
// ret = next - now
// (ASSUMPTION: ret is >= 0)
// Reordering and adjusting units:
// ret = convert_to_ms_units(last - now) + timeframe
// (Being sure to avoid overflow/underflow)
	getnstimeofday(&now);
	return ((int)last->tv_sec - (int)now.tv_sec) * 1000
		+ ((int)last->tv_nsec - (int)now.tv_nsec) / 1000000
		+ time_frame;
}

/* Does not start the timer(return false) if it's not necessary and work can be
 * done.
 */
void dtp_start_rate_timer(struct dtp * dtp, struct dtcp * dtcp)
{
	uint rtf, tf;
	struct timespec t = {0,0};

	tf = 0;

	if(rtimer_is_pending(&dtp->timers.rate_window)) {
		LOG_DBG("rbfc Rate based timer is still pending...");
		return;
	}

	spin_lock_bh(&dtp->sv_lock);
	rtf = dtcp->sv->time_unit;
	t.tv_sec = dtcp->sv->last_time.tv_sec;
	t.tv_nsec = dtcp->sv->last_time.tv_nsec;
	spin_unlock_bh(&dtp->sv_lock);

	tf = msec_to_next_rate(rtf, &t);

	LOG_DBG("Rate based timer start, time %u msec, "
		"last: %lu:%lu", tf, t.tv_sec, t.tv_nsec);

	rtimer_start(&dtp->timers.rate_window, tf);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
static void tf_rate_window(void * o)
#else
static void tf_rate_window(struct timer_list * tl)
#endif
{
        struct dtp *  dtp;
        struct dtcp * dtcp;
        struct timespec now  = {0, 0};

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        dtp = (struct dtp *) o;
#else
        dtp = from_timer(dtp, tl, timers.rate_window);
#endif
        if (!dtp) {
                LOG_ERR("No DTP found. Cannot run rate window timer");
                return;
        }

        dtcp = dtp->dtcp;

        LOG_DBG("rbfc Timer job start...");
        LOG_DBG("rbfc Re-opening the rate mechanism");

        getnstimeofday(&now);

        spin_lock_bh(&dtp->sv_lock);
	dtcp->sv->pdus_sent_in_time_unit = 0;
	dtcp->sv->pdus_rcvd_in_time_unit = 0;
	dtcp->sv->last_time.tv_sec = now.tv_sec;
	dtcp->sv->last_time.tv_nsec = now.tv_nsec;
	dtp->sv->rate_fulfiled = false;
	spin_unlock_bh(&dtp->sv_lock);

	efcp_enable_write(dtp->efcp);
	cwq_deliver(dtp->cwq, dtp, dtp->rmt);

        return;
}

int dtp_sv_init(struct dtp * dtp,
                bool         rexmsn_ctrl,
                bool         window_based,
                bool         rate_based,
		uint_t      mfps,
		uint_t      mfss,
		u_int32_t   mpl,
		timeout_t   a,
		timeout_t   r,
		timeout_t   tr)
{
        ASSERT(dtp);
        ASSERT(dtp->sv);

	dtp->sv->stats.drop_pdus = 0;
	dtp->sv->stats.err_pdus = 0;
	dtp->sv->stats.tx_pdus = 0;
	dtp->sv->stats.tx_bytes = 0;
	dtp->sv->stats.rx_pdus = 0;
	dtp->sv->stats.rx_bytes = 0;
        dtp->sv->rexmsn_ctrl  = rexmsn_ctrl;
        dtp->sv->window_based = window_based;
        dtp->sv->rate_based   = rate_based;

        /* Init seq numbers */
        dtp->sv->max_seq_nr_sent = dtp->sv->seq_nr_to_send;
        dtp->sv->max_seq_nr_rcv  = 0;

        dtp->sv->max_flow_pdu_size = mfps;
        dtp->sv->max_flow_sdu_size = mfss;
        dtp->sv->MPL               = mpl;
        dtp->sv->A                 = a;
        dtp->sv->R                 = r;
        dtp->sv->tr                = tr;

        return 0;
}

/* Must be called under RCU read lock. */
struct dtp_ps *
dtp_ps_get(struct dtp * dtp)
{
        return container_of(rcu_dereference(dtp->base.ps),
                            struct dtp_ps, base);
}

struct dtp *
dtp_from_component(struct rina_component * component)
{
        return container_of(component, struct dtp, base);
}
EXPORT_SYMBOL(dtp_from_component);

int dtp_select_policy_set(struct dtp * dtp,
                          const string_t * path,
                          const string_t * name)
{
        struct ps_select_transaction trans;
        struct dtp_config * cfg = dtp->cfg;

        if (path && strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        base_select_policy_set_start(&dtp->base, &trans, &policy_sets, name);

        if (trans.state == PS_SEL_TRANS_PENDING) {
                struct dtp_ps * ps;

                /* Copy the connection parameters to the policy-set. From now
                 * on these connection parameters must be accessed by the DTP
                 * policy set, and not from the struct connection. */
                ps = container_of(trans.candidate_ps, struct dtp_ps, base);
                ps->dtcp_present        = dtp_conf_dtcp_present(cfg);
                ps->seq_num_ro_th       = dtp_conf_seq_num_ro_th(cfg);
                ps->initial_a_timer     = dtp_conf_initial_a_timer(cfg);
                ps->partial_delivery    = dtp_conf_partial_del(cfg);
                ps->incomplete_delivery = dtp_conf_incomplete_del(cfg);
                ps->in_order_delivery   = dtp_conf_in_order_del(cfg);
                ps->max_sdu_gap         = dtp_conf_max_sdu_gap(cfg);

                /* Fill in default policies. */
                if (!ps->transmission_control) {
                        ps->transmission_control =
                        		default_transmission_control;
                }
                if (!ps->closed_window) {
                        ps->closed_window = default_closed_window;
                }
                if (!ps->snd_flow_control_overrun) {
                        ps->snd_flow_control_overrun =
                        		default_snd_flow_control_overrun;
                }
                if (!ps->initial_sequence_number) {
                        ps->initial_sequence_number =
                        		default_initial_sequence_number;
                }
                if (!ps->receiver_inactivity_timer) {
                        ps->receiver_inactivity_timer =
                        		default_receiver_inactivity_timer;
                }
                if (!ps->sender_inactivity_timer) {
                        ps->sender_inactivity_timer =
                        		default_sender_inactivity_timer;
                }
        }

        base_select_policy_set_finish(&dtp->base, &trans);

        return trans.state == PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(dtp_select_policy_set);

int dtp_set_policy_set_param(struct dtp* dtp,
                             const char * path,
                             const char * name,
                             const char * value)
{
        struct dtp_ps *ps;
        int ret = -1;

        if (!dtp|| !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", dtp, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                int bool_value;

                /* The request addresses this DTP instance. */
                rcu_read_lock();
                ps = container_of(rcu_dereference(dtp->base.ps),
                                  struct dtp_ps, base);

                if (strcmp(name, "dtcp_present") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->dtcp_present = bool_value;
                        }
                } else if (strcmp(name, "seq_num_ro_th") == 0) {
                        ret = kstrtoint(value, 10, &ps->seq_num_ro_th);
                } else if (strcmp(name, "initial_a_timer") == 0) {
                        ret = kstrtouint(value, 10, &ps->initial_a_timer);
                } else if (strcmp(name, "partial_delivery") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->partial_delivery = bool_value;
                        }
                } else if (strcmp(name, "incomplete_delivery") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->incomplete_delivery = bool_value;
                        }
                } else if (strcmp(name, "in_order_delivery") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->in_order_delivery = bool_value;
                        }
                } else if (strcmp(name, "max_sdu_gap") == 0) {
                        ret = kstrtouint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->max_sdu_gap = bool_value;
                        }
                } else {
                        LOG_ERR("Unknown DTP parameter policy '%s'", name);
                }
                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&dtp->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(dtp_set_policy_set_param);

struct dtp * dtp_create(struct efcp *       efcp,
                        struct rmt *        rmt,
                        struct dtp_config * dtp_cfg,
			struct robject *    parent)
{
        struct dtp * dtp;
        string_t *   ps_name;

        if (!efcp) {
                LOG_ERR("No EFCP passed, bailing out");
                return NULL;
        }

        if (!dtp_cfg) {
                LOG_ERR("No DTP conf passed, bailing out");
                return NULL;
        }

        if (!rmt) {
                LOG_ERR("No RMT passed, bailing out");
                return NULL;
        }

        dtp = rkzalloc(sizeof(*dtp), GFP_KERNEL);
        if (!dtp) {
                LOG_ERR("Cannot create DTP instance");
                return NULL;
        }

        dtp->efcp = efcp;

	if (robject_init_and_add(&dtp->robj,
				 &dtp_rtype,
				 parent,
				 "dtp")) {
                dtp_destroy(dtp);
                return NULL;
	}

        dtp->sv = rkmalloc(sizeof(*dtp->sv), GFP_KERNEL);
        if (!dtp->sv) {
                LOG_ERR("Cannot create DTP state-vector");

                dtp_destroy(dtp);
                return NULL;
        }
        *dtp->sv = default_sv;
        /* FIXME: fixups to the state-vector should be placed here */

        spin_lock_init(&dtp->sv_lock);

        dtp->cfg   = dtp_cfg;
        dtp->rmt  = rmt;
        dtp->rttq = NULL;
        dtp->rtxq = NULL;
        dtp->seqq = squeue_create(dtp);
        if (!dtp->seqq) {
                LOG_ERR("Could not create Sequencing queue");
                dtp_destroy(dtp);
                return NULL;
        }

        rtimer_init(tf_sender_inactivity, &dtp->timers.sender_inactivity, dtp);
        rtimer_init(tf_receiver_inactivity, &dtp->timers.receiver_inactivity, dtp);
        rtimer_init(tf_a, &dtp->timers.a, dtp);
        rtimer_init(tf_rate_window, &dtp->timers.rate_window, dtp);
        rtimer_init(tf_rendezvous, &dtp->timers.rendezvous, dtp);

        dtp->to_post = ringq_create(TO_POST_LENGTH);
        if (!dtp->to_post) {
                LOG_ERR("Unable to create to_post queue; bailing out");
               dtp_destroy(dtp);
               return NULL;
        }
        dtp->to_send = ringq_create(TO_SEND_LENGTH);
        if (!dtp->to_send) {
               LOG_ERR("Unable to create to_send queue; bailing out");
               dtp_destroy(dtp);
               return NULL;
        }

        rina_component_init(&dtp->base);

        ps_name = (string_t *) policy_name(dtp_conf_ps_get(dtp_cfg));
        if (!ps_name || !strcmp(ps_name, ""))
                ps_name = RINA_PS_DEFAULT_NAME;

        if (dtp_select_policy_set(dtp, "", ps_name)) {
                LOG_ERR("Could not load DTP PS %s", ps_name);
                dtp_destroy(dtp);
                return NULL;
        }

        if (dtp_initial_sequence_number(dtp)) {
                LOG_ERR("Could not create Sequencing queue");
                dtp_destroy(dtp);
                return NULL;
        }

        spin_lock_init(&dtp->lock);

        return dtp;
}

int dtp_destroy(struct dtp * instance)
{
	struct dtcp * dtcp = NULL;
	struct cwq * cwq = NULL;
	struct rtxq * rtxq = NULL;
	struct rttq * rttq = NULL;
	int ret = 0;

        if (!instance)
                return -1;

	spin_lock_bh(&instance->lock);

        if (instance->dtcp) {
                dtcp = instance->dtcp;
                instance->dtcp = NULL; /* Useful */
        }

        if (instance->cwq) {
                cwq = instance->cwq;
                instance->cwq = NULL; /* Useful */
        }

        if (instance->rtxq) {
        	rtxq = instance->rtxq;
        	instance->rtxq = NULL; /* Useful */
        }

        if (instance->rttq) {
        	rttq = instance->rttq;
        	instance->rttq = NULL;
        }

        spin_unlock_bh(&instance->lock);

        if (dtcp) {
        	if (dtcp_destroy(dtcp)) {
        		LOG_WARN("Error destroying DTCP");
        		ret = -1;
        	}
        }

        if (cwq) {
        	if (cwq_destroy(cwq)) {
        		LOG_WARN("Error destroying CWQ");
        		ret = -1;
        	}
        }

        if (rtxq) {
                if (rtxq_destroy(rtxq)) {
                        LOG_ERR("Failed to destroy rexmsn queue");
                        ret = -1;
                }
        }

        if (rttq) {
        	if (rttq_destroy(rttq)) {
        		LOG_ERR("Failed to destroy rexmsn queue");
        		ret = -1;
        	}
        }

        rtimer_destroy(&instance->timers.a);
        /* tf_a posts workers that restart sender_inactivity timer, so the wq
         * must be flushed before destroying the timer */

        rtimer_destroy(&instance->timers.sender_inactivity);
        rtimer_destroy(&instance->timers.receiver_inactivity);
        rtimer_destroy(&instance->timers.rate_window);
        rtimer_destroy(&instance->timers.rtx);
        rtimer_destroy(&instance->timers.rendezvous);

        if (instance->to_post) ringq_destroy(instance->to_post,
                               (void (*)(void *)) du_destroy);
        if (instance->to_send) ringq_destroy(instance->to_send,
                               (void (*)(void *)) du_destroy);

        if (instance->seqq) squeue_destroy(instance->seqq);
        if (instance->sv)   rkfree(instance->sv);
        if (instance->cfg) dtp_config_destroy(instance->cfg);
        rina_component_fini(&instance->base);

	robject_del(&instance->robj);
        rkfree(instance);

        LOG_DBG("DTP %pK destroyed successfully", instance);

        return 0;
}

static bool window_is_closed(struct dtp *    dtp,
                             struct dtcp *   dtcp,
                             seq_num_t       seq_num,
                             struct dtp_ps * ps)
{
        bool retval = false, w_ret = false, r_ret = false;

        ASSERT(dtp);
        ASSERT(dtcp);

        spin_lock_bh(&dtp->sv_lock);
        if (dtp->sv->window_based && seq_num > dtcp->sv->snd_rt_wind_edge) {
        	dtp->sv->window_closed = true;
                w_ret = true;
        }

        if (dtp->sv->rate_based) {
        	if(dtcp_rate_exceeded(dtcp, 1)) {
        		dtp->sv->rate_fulfiled = true;
        		r_ret = true;
		} else {
			if (dtp->sv->rate_fulfiled) {
				LOG_DBG("rbfc Re-opening the rate mechanism");
				dtp->sv->rate_fulfiled = false;
			}
		}
        }

        spin_unlock_bh(&dtp->sv_lock);

        if (r_ret)
        	dtp_start_rate_timer(dtp, dtcp);

        retval = (w_ret || r_ret);

        if(w_ret && r_ret)
        	retval = ps->reconcile_flow_conflict(ps);

        return retval;
}

void dtp_send_pending_ctrl_pdus(struct dtp * dtp)
{
	struct du * du_ctrl;

	while (!ringq_is_empty(dtp->to_send)) {
		du_ctrl = ringq_pop(dtp->to_send);
		if (du_ctrl && dtp_pdu_send(dtp, dtp->rmt, du_ctrl)) {
			LOG_ERR("Problems sending DTCP Ctrl PDU");
			du_destroy(du_ctrl);
		}
	}
}
EXPORT_SYMBOL(dtp_send_pending_ctrl_pdus);

int dtp_write(struct dtp * instance,
              struct du * du)
{
        struct dtcp *     dtcp;
        struct du *       cdu;
        struct dtp_ps *   ps;
        seq_num_t         sn, csn;
        struct efcp *     efcp;
        int		  sbytes;
        uint_t            sc;
        timeout_t         mpl, r, a, rv;
        bool		  start_rv_timer;

        efcp = instance->efcp;
        dtcp = instance->dtcp;

        /* Stop SenderInactivityTimer */
        if (rtimer_stop(&instance->timers.sender_inactivity)) {
                LOG_ERR("Failed to stop timer");
        }

        /*
         * FIXME: The two ways of carrying out flow control
         * could exist at once, thus reconciliation should be
         * the first and default case if both are present.
         */

        /* FIXME: from now on, this should be done for each user data
         * generated by Delimiting *
         */

        /* Step 2: Sequencing */
        /*
         * Incrementing here means the PDU cannot
         * be just thrown away from this point onwards
         */
        /* Probably needs to be revised */

	sbytes = du_len(du);
	if (du_encap(du, PDU_TYPE_DT)){
		LOG_ERR("Could not encap PDU");
		goto pdu_err_exit;
	}

	spin_lock_bh(&instance->sv_lock);

        csn = ++instance->sv->seq_nr_to_send;
        if (pci_format(&du->pci,
                       efcp->connection->source_cep_id,
                       efcp->connection->destination_cep_id,
                       efcp->connection->source_address,
                       efcp->connection->destination_address,
                       csn,
                       efcp->connection->qos_id,
                       0,
		       sbytes + du->pci.len,
                       PDU_TYPE_DT)) {
		LOG_ERR("Could not format PCI");
		goto pdu_err_exit;
	}
	if (!pci_is_ok(&du->pci)) {
		LOG_ERR("PCI is not ok");
		goto pdu_err_exit;
	}

        sn = dtcp->sv->snd_lft_win;
        if (instance->sv->drf_flag ||
        		((sn == (csn - 1)) && instance->sv->rexmsn_ctrl)) {
		pdu_flags_t pci_flags;
		pci_flags = pci_flags_get(&du->pci);
		pci_flags |= PDU_FLAGS_DATA_RUN;
                pci_flags_set(&du->pci, pci_flags);
	}

        LOG_DBG("DTP Sending PDU %u (CPU: %d)", csn, smp_processor_id());
        mpl = instance->sv->MPL;
        r = instance->sv->R;
        a = instance->sv->A;
        spin_unlock_bh(&instance->sv_lock);

        if (dtcp) {
                rcu_read_lock();
                ps = container_of(rcu_dereference(instance->base.ps),
                                  struct dtp_ps, base);
                if (instance->sv->window_based || instance->sv->rate_based) {
			/* NOTE: Might close window */
			if (window_is_closed(instance,
						dtcp,
						csn,
						ps)) {
				if (ps->closed_window(ps, du)) {
					LOG_ERR("Problems with the closed window policy");
					goto stats_err_exit;
				}
				rcu_read_unlock();

				/* Check if rendezvous PDU needs to be sent*/
				start_rv_timer = false;
				spin_lock_bh(&instance->sv_lock);

				/* If there is rtx control and PDUs at the rtxQ
				 * don't enter the rendezvous state (DTCP will keep
				 * retransmitting the PDUs until acked or the
				 * retransmission timeout fires)
				 */
				if (instance->sv->rexmsn_ctrl &&
						rtxq_size(instance->rtxq) > 0) {
					LOG_DBG("Window is closed but there are PDUs at the RTXQ");
					spin_unlock_bh(&instance->sv_lock);
					return 0;
				}

				/* Else, check if rendezvous PDU needs to be sent */
				if (!instance->dtcp->sv->rendezvous_sndr) {
					instance->dtcp->sv->rendezvous_sndr = true;

					LOG_DBG("RV at the sender %u (CPU: %d)", csn, smp_processor_id());

					/* Start rendezvous timer, wait for Tr to fire */
					start_rv_timer = true;
					rv = jiffies_to_msecs(instance->sv->tr);
				}
				spin_unlock_bh(&instance->sv_lock);

				if (start_rv_timer) {
					LOG_DBG("Window is closed. SND LWE: %u | SND RWE: %u | TR: %u",
							instance->dtcp->sv->snd_lft_win,
							instance->dtcp->sv->snd_rt_wind_edge,
							instance->sv->tr);
					/* Send rendezvous PDU and start time */
					rtimer_start(&instance->timers.rendezvous, rv);
				}

				return 0;
			}
			if(instance->sv->rate_based) {
				spin_lock_bh(&instance->sv_lock);
				sc = dtcp->sv->pdus_sent_in_time_unit;
				if(sbytes >= 0) {
					if (sbytes + sc >= dtcp->sv->sndr_rate)
						dtcp->sv->pdus_sent_in_time_unit =
								dtcp->sv->sndr_rate;
					else
						dtcp->sv->pdus_sent_in_time_unit =
								dtcp->sv->pdus_sent_in_time_unit + sbytes;
				}
				spin_unlock_bh(&instance->sv_lock);
			}
                }
                if (instance->sv->rexmsn_ctrl) {
                        cdu = du_dup_ni(du);
                        if (!cdu) {
                                LOG_ERR("Failed to copy PDU. PDU type: %d",
                                	 pci_type(&du->pci));
                                goto pdu_stats_err_exit;
                        }

                        if (rtxq_push_ni(instance->rtxq, cdu)) {
                                LOG_ERR("Couldn't push to rtxq");
                                goto pdu_stats_err_exit;
                        }
                } else if (instance->rttq) {
                	if (rttq_push(instance->rttq, csn)) {
                		LOG_ERR("Failed to push SN to RTT queue");
                	}
                }

                if (ps->transmission_control(ps, du)) {
                        LOG_ERR("Problems with transmission control");
                        goto stats_err_exit;
                }

                rcu_read_unlock();
                spin_lock_bh(&instance->sv_lock);
                stats_inc_bytes(tx, instance->sv, sbytes);
                spin_unlock_bh(&instance->sv_lock);

                /* Start SenderInactivityTimer */
                if (rtimer_restart(&instance->timers.sender_inactivity,
                                   3 * (mpl + r + a ))) {
                        LOG_ERR("Failed to start sender_inactiviy timer");
                        goto stats_nounlock_err_exit;
                        return -1;
                }

                return 0;
        }

        if (dtp_pdu_send(instance,
                         instance->rmt,
                         du))
		return -1;
        spin_lock_bh(&instance->sv_lock);
	stats_inc_bytes(tx, instance->sv, sbytes);
	spin_unlock_bh(&instance->sv_lock);
	return 0;

pdu_err_exit:
	du_destroy(du);
	return -1;

pdu_stats_err_exit:
	du_destroy(du);
stats_err_exit:
        rcu_read_unlock();
stats_nounlock_err_exit:
	spin_lock_bh(&instance->sv_lock);
	stats_inc(err, instance->sv);
	spin_unlock_bh(&instance->sv_lock);
	return -1;
}

/* Must be called with sv lock taken */
static bool is_fc_overrun(struct dtp * dtp, struct dtcp * dtcp,
			  seq_num_t seq_num, int pdul)
{
        bool to_ret, w_ret = false, r_ret = false;

        if (!dtcp)
                return false;

        if (dtcp_window_based_fctrl(dtcp->cfg)) {
                w_ret = (seq_num > dtcp->sv->rcvr_rt_wind_edge);
        }

        if (dtcp_rate_based_fctrl(dtcp->cfg)){
        	if(pdul >= 0) {
        		dtcp->sv->pdus_rcvd_in_time_unit =
        				dtcp->sv->pdus_rcvd_in_time_unit + pdul;
        	}
        }

        to_ret = w_ret || r_ret;

	LOG_DBG("Is fc overruned? win: %d, rate: %d", w_ret, r_ret);

        if (w_ret || r_ret)
                LOG_DBG("Reconcile flow control RX");

        return to_ret;
}

static bool are_there_pdus(struct seq_queue * queue, seq_num_t LWE)
{
        struct seq_queue_entry * p;
        seq_num_t sn;

        if (list_empty(&queue->head)) {
                LOG_DBG("Seq Queue is empty!");
                return false;
        }
        p = list_first_entry(&queue->head, struct seq_queue_entry, next);
        if (!p)
                return false;

        sn = pci_sequence_number_get(&p->du->pci);
        if (sn == (LWE+1))
                return true;

        return false;
}

int dtp_pdu_ctrl_send(struct dtp * dtp, struct du * du)
{
	return ringq_push(dtp->to_send, du);
}

int dtp_receive(struct dtp * instance,
                struct du * du)
{
        struct dtp_ps *  ps;
        struct dtcp *    dtcp;
        struct dtcp_ps * dtcp_ps;
        seq_num_t        seq_num;
        timeout_t        a, r, mpl;
        seq_num_t        LWE;
        bool             in_order;
        bool             rtx_ctrl = false;
        seq_num_t        max_sdu_gap;
	int              sbytes;
	struct efcp *	 efcp = 0;

        LOG_DBG("DTP receive started...");

        dtcp = instance->dtcp;
        efcp = instance->efcp;

        spin_lock_bh(&instance->sv_lock);

        a           = instance->sv->A;
        r 	    = instance->sv->R;
        mpl	    = instance->sv->MPL;
        LWE         = instance->sv->rcv_left_window_edge;

        rcu_read_lock();
        ps = container_of(rcu_dereference(instance->base.ps),
                          struct dtp_ps, base);
        in_order    = ps->in_order_delivery;
        max_sdu_gap = ps->max_sdu_gap;
        if (dtcp) {
                dtcp_ps = dtcp_ps_get(dtcp);
                rtx_ctrl = dtcp_ps->rtx_ctrl;
        }
        rcu_read_unlock();

        seq_num = pci_sequence_number_get(&du->pci);
	sbytes = du_data_len(du);

        LOG_DBG("local_soft_irq_pending: %d", local_softirq_pending());
        LOG_DBG("DTP Received PDU %u (CPU: %d)",
                seq_num, smp_processor_id());

        if (instance->sv->drf_required) {
                /* Start ReceiverInactivityTimer */
                if (rtimer_restart(&instance->timers.receiver_inactivity,
                                   2 * (mpl + r + a))) {
                        LOG_ERR("Failed to start Receiver Inactivity timer");
                        spin_unlock_bh(&instance->sv_lock);
                        du_destroy(du);
                        return -1;
                }

                if ((pci_flags_get(&du->pci) & PDU_FLAGS_DATA_RUN)) {
                	LOG_DBG("Data Run Flag");

                	instance->sv->drf_required = false;
                        instance->sv->rcv_left_window_edge = seq_num;
                        dtp_squeue_flush(instance);
                        if (instance->rttq) {
                        	rttq_flush(instance->rttq);
                        }

                        spin_unlock_bh(&instance->sv_lock);

                        if (dtcp) {
                                if (dtcp_sv_update(dtcp, &du->pci)) {
                                        LOG_ERR("Failed to update dtcp sv");
                                        return -1;
                                }
                        }

                        dtp_send_pending_ctrl_pdus(instance);
                        pdu_post(instance, du);
			stats_inc_bytes(rx, instance->sv, sbytes);

                        return 0;
                }

                LOG_ERR("Expecting DRF but not present, dropping PDU %d...",
                        seq_num);

		stats_inc(drop, instance->sv);
		spin_unlock_bh(&instance->sv_lock);

                du_destroy(du);
                return 0;
        }

        /*
         * NOTE:
         *   no need to check presence of in_order or dtcp because in case
         *   they are not, LWE is not updated and always 0
         */
        if ((seq_num <= LWE) ||
        		(is_fc_overrun(instance, dtcp, seq_num, sbytes))) {
        	/* Duplicate PDU or flow control overrun */
        	LOG_ERR("Duplicate PDU or flow control overrun.SN: %u, LWE:%u",
        		 seq_num, LWE);
                stats_inc(drop, instance->sv);

                spin_unlock_bh(&instance->sv_lock);

                du_destroy(du);

                if (dtcp) {
                	/* Send an ACK/Flow Control PDU with current window values */
                	/* FIXME: we have to send a Control ACK PDU, not an
                	 * ack flow control one
                	 */
                        /*if (dtcp_ack_flow_control_pdu_send(dtcp, LWE)) {
                                LOG_ERR("Failed to send ack/flow control pdu");
                                return -1;
                        }*/
                }
                return 0;
        }

        /* Start ReceiverInactivityTimer */
        if (rtimer_restart(&instance->timers.receiver_inactivity,
                           2 * (mpl + r + a ))) {
        	spin_unlock_bh(&instance->sv_lock);
                LOG_ERR("Failed to start Receiver Inactivity timer");
                du_destroy(du);
                return -1;
        }

        /* This is an acceptable data PDU, stop reliable ACK timer */
        if (dtcp->sv->rendezvous_rcvr) {
        	LOG_DBG("RV at receiver put to false");
        	dtcp->sv->rendezvous_rcvr = false;
        	rtimer_stop(&dtcp->rendezvous_rcv);
        }
        if (!a) {
                bool set_lft_win_edge;

                if (!in_order && !dtcp) {
                	spin_unlock_bh(&instance->sv_lock);
                        LOG_DBG("DTP Receive deliver, seq_num: %d, LWE: %d",
                                seq_num, LWE);
                        if (pdu_post(instance, du))
                                return -1;

                        return 0;
                }

                set_lft_win_edge = !(dtcp_rtx_ctrl(dtcp->cfg) &&
                                     ((seq_num -LWE) > (max_sdu_gap + 1)));
                if (set_lft_win_edge) {
                	instance->sv->rcv_left_window_edge = seq_num;
                }

                spin_unlock_bh(&instance->sv_lock);

                if (dtcp) {
                        if (dtcp_sv_update(dtcp, &du->pci)) {
                                LOG_ERR("Failed to update dtcp sv");
                                goto fail;
                        }
                        dtp_send_pending_ctrl_pdus(instance);
                        if (!set_lft_win_edge) {
                                du_destroy(du);
                                return 0;
                        }
                }

                if (pdu_post(instance, du))
                        return -1;

                spin_lock_bh(&instance->sv_lock);
		stats_inc_bytes(rx, instance->sv, sbytes);
		spin_unlock_bh(&instance->sv_lock);
                return 0;

        fail:
                du_destroy(du);
                return -1;
        }

        LWE = instance->sv->rcv_left_window_edge;
        LOG_DBG("DTP receive LWE: %u", LWE);
        if (seq_num == LWE + 1) {
        	instance->sv->rcv_left_window_edge = seq_num;
                ringq_push(instance->to_post, du);
                LWE = seq_num;
        } else {
                seq_queue_push_ni(instance->seqq->queue, du);
        }

        while (are_there_pdus(instance->seqq->queue, LWE)) {
                du = seq_queue_pop(instance->seqq->queue);
                if (!du)
                        break;
                seq_num = pci_sequence_number_get(&du->pci);
                LWE     = seq_num;
                instance->sv->rcv_left_window_edge = seq_num;
                ringq_push(instance->to_post, du);
        }

        spin_unlock_bh(&instance->sv_lock);

        if (dtcp) {
                if (dtcp_sv_update(dtcp, &du->pci)) {
                        LOG_ERR("Failed to update dtcp sv");
                }
        }

        dtp_send_pending_ctrl_pdus(instance);

        if (list_empty(&instance->seqq->queue->head))
                rtimer_stop(&instance->timers.a);
        else
                rtimer_start(&instance->timers.a, a/AF);

        while (!ringq_is_empty(instance->to_post)) {
                du = (struct du *) ringq_pop(instance->to_post);
                if (du) {
                	sbytes = du_data_len(du);
                        pdu_post(instance, du);
                        spin_lock_bh(&instance->sv_lock);
			stats_inc_bytes(rx, instance->sv, sbytes);
			spin_unlock_bh(&instance->sv_lock);
		}
        }

        LOG_DBG("DTP receive ended...");
        return 0;
}

int dtp_ps_publish(struct ps_factory * factory)
{
        if (factory == NULL) {
                LOG_ERR("%s: NULL factory", __func__);
                return -1;
        }

        return ps_publish(&policy_sets, factory);
}
EXPORT_SYMBOL(dtp_ps_publish);

int dtp_ps_unpublish(const char * name)
{ return ps_unpublish(&policy_sets, name); }
EXPORT_SYMBOL(dtp_ps_unpublish);
