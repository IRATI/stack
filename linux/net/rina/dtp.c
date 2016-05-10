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

#define RINA_PREFIX "dtp"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtp-ps-default.h"
#include "dtp.h"
#include "dt.h"
#include "dt-utils.h"
#include "dtcp.h"
#include "dtcp-ps.h"
#include "dtp-conf-utils.h"
#include "dtcp-conf-utils.h"
#include "ps-factory.h"
#include "dtp-ps.h"
#include "policies.h"
#include "serdes.h"

static struct policy_set_list policy_sets = {
        .head = LIST_HEAD_INIT(policy_sets.head)
};

/* This is the DT-SV part maintained by DTP */
struct dtp_sv {
        spinlock_t lock;

        uint_t     seq_number_rollover_threshold;
	/* FIXME: we need to control rollovers...*/
	struct {
		unsigned int drop_pdus;
		unsigned int err_pdus;
		unsigned int tx_pdus;
		unsigned int tx_bytes;
		unsigned int rx_pdus;
		unsigned int rx_bytes;
	} stats;
        seq_num_t  max_seq_nr_rcv;
        seq_num_t  seq_nr_to_send;
        seq_num_t  max_seq_nr_sent;

        bool       window_based;
        bool       rexmsn_ctrl;
        bool       rate_based;
        bool       drf_required;
        bool       rate_fulfiled;
};

struct dtp {
        struct dt *               parent;
        /*
         * NOTE: The DTP State Vector is discarded only after and explicit
         *       release by the AP or by the system (if the AP crashes).
         */
        struct dtp_sv *           sv; /* The state-vector */

        struct rina_component     base;
        struct dtp_config *       cfg;
        struct rmt *              rmt;
        struct squeue *           seqq;
        struct {
                struct rtimer * sender_inactivity;
                struct rtimer * receiver_inactivity;
                struct rtimer * a;
                struct rtimer * rate_window;
        } timers;
	struct robject		  robj;
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
};

#define stats_get(name, sv, retval, flags)			\
        ASSERT(sv);						\
        spin_lock_irqsave(&sv->lock, flags);			\
        retval = sv->stats.name;				\
        spin_unlock_irqrestore(&sv->lock, flags);

#define stats_inc(name, sv, flags)				\
        ASSERT(sv);						\
        spin_lock_irqsave(&sv->lock, flags);			\
        sv->stats.name##_pdus++;				\
        LOG_DBG("PDUs __STRINGIFY(name) %u",			\
		sv->stats.name##_pdus);				\
        spin_unlock_irqrestore(&sv->lock, flags);

#define stats_inc_bytes(name, sv, bytes, flags)			\
        ASSERT(sv);						\
        spin_lock_irqsave(&sv->lock, flags);			\
        sv->stats.name##_pdus++;				\
	sv->stats.name##_bytes += (unsigned long) bytes;	\
        LOG_DBG("PDUs __STRINGIFY(name) %u (%u)",		\
		sv->stats.name##_pdus,				\
		sv->stats.name##_bytes);			\
        spin_unlock_irqrestore(&sv->lock, flags);

static ssize_t dtp_attr_show(struct robject *		     robj,
                         	     struct robj_attribute * attr,
                                     char *		     buf)
{
	struct dtp * instance;
	unsigned int stats_ret;
	unsigned long flags;

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
		stats_get(drop_pdus, instance->sv, stats_ret, flags);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "err_pdus") == 0) {
		stats_get(err_pdus, instance->sv, stats_ret, flags);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "tx_pdus") == 0) {
		stats_get(tx_pdus, instance->sv, stats_ret, flags);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "tx_bytes") == 0) {
		stats_get(tx_bytes, instance->sv, stats_ret, flags);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "rx_pdus") == 0) {
		stats_get(rx_pdus, instance->sv, stats_ret, flags);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "rx_bytes") == 0) {
		stats_get(rx_bytes, instance->sv, stats_ret, flags);
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

struct dt * dtp_dt(struct dtp * dtp)
{
        return dtp->parent;
}

struct rmt * dtp_rmt(struct dtp * dtp)
{
        return dtp->rmt;
}

struct dtp_sv * dtp_dtp_sv(struct dtp * dtp)
{
        return dtp->sv;
}

struct dtp_config * dtp_config_get(struct dtp * dtp)
{
	return dtp->cfg;
}

int nxt_seq_reset(struct dtp_sv * sv, seq_num_t sn)
{
        unsigned long flags;

        if (!sv)
                return -1;

        spin_lock_irqsave(&sv->lock, flags);
        sv->seq_nr_to_send = sn;
        spin_unlock_irqrestore(&sv->lock, flags);

        return 0;
}

static seq_num_t nxt_seq_get(struct dtp_sv * sv)
{
        seq_num_t     tmp;
        unsigned long flags;

        ASSERT(sv);

        spin_lock_irqsave(&sv->lock, flags);
        tmp = ++sv->seq_nr_to_send;
        spin_unlock_irqrestore(&sv->lock, flags);

        return tmp;
}

seq_num_t dtp_sv_last_nxt_seq_nr(struct dtp * instance)
{
        seq_num_t       tmp;
        struct dtp_sv * sv;
        unsigned long   flags;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }
        sv = instance->sv;
        ASSERT(sv);

        spin_lock_irqsave(&sv->lock, flags);
        tmp = sv->seq_nr_to_send;
        spin_unlock_irqrestore(&sv->lock, flags);

        return tmp;
}

seq_num_t dtp_sv_max_seq_nr_sent(struct dtp * instance)
{
        seq_num_t       tmp;
        unsigned long   flags;
        struct dtp_sv * sv;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }
        sv = instance->sv;
        ASSERT(sv);

        spin_lock_irqsave(&sv->lock, flags);
        tmp = sv->max_seq_nr_sent;
        spin_unlock_irqrestore(&sv->lock, flags);

        return tmp;
}

int dtp_sv_max_seq_nr_set(struct dtp * instance, seq_num_t num)
{
        unsigned long   flags;
        struct dtp_sv * sv;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }
        sv = instance->sv;
        ASSERT(sv);

        spin_lock_irqsave(&sv->lock, flags);
        if (sv->max_seq_nr_sent < num)
                sv->max_seq_nr_sent = num;
        spin_unlock_irqrestore(&sv->lock, flags);

        return 0;
}

static bool sv_rate_fulfiled(struct dtp_sv * sv)
{
        unsigned long flags;
        bool          tmp;

        spin_lock_irqsave(&sv->lock, flags);
        tmp = sv->rate_fulfiled;
        spin_unlock_irqrestore(&sv->lock, flags);

        return tmp;
}

bool dtp_sv_rate_fulfiled(struct dtp * instance)
{
        struct dtp_sv * sv;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return false;
        }
        sv = instance->sv;
        ASSERT(sv);

        return sv_rate_fulfiled(sv);
}

int dtp_sv_rate_fulfiled_set(struct dtp * instance, bool fulfiled)
{
        unsigned long   flags;
        struct dtp_sv * sv;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }
        sv = instance->sv;
        ASSERT(sv);

        LOG_DBG("rbfc Rate set to %u (0=some more, 1=consumed)", fulfiled);

        spin_lock_irqsave(&sv->lock, flags);
        sv->rate_fulfiled = fulfiled;
        spin_unlock_irqrestore(&sv->lock, flags);

        return 0;
}

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
        struct pdu *     pdu;
        struct list_head next;
};

struct squeue {
        struct dtp *       dtp;
        struct seq_queue * queue;
        spinlock_t         lock;
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

static struct seq_queue_entry * seq_queue_entry_create_gfp(struct pdu * pdu,
                                                           gfp_t        flags)
{
        struct seq_queue_entry * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->next);

        tmp->pdu = pdu;
        tmp->time_stamp = jiffies;

        return tmp;
}

static void seq_queue_entry_destroy(struct seq_queue_entry * seq_entry)
{
        ASSERT(seq_entry);

        if (seq_entry->pdu) pdu_destroy(seq_entry->pdu);
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

static struct pdu * seq_queue_pop(struct seq_queue * q)
{
        struct seq_queue_entry * p;
        struct pdu *             pdu;

        if (list_empty(&q->head)) {
                LOG_DBG("Seq Queue is empty!");
                return NULL;
        }

        p = list_first_entry(&q->head, struct seq_queue_entry, next);
        if (!p)
                return NULL;

        pdu = p->pdu;
        p->pdu = NULL;

        list_del(&p->next);
        seq_queue_entry_destroy(p);

        return pdu;
}

static int seq_queue_push_ni(struct seq_queue * q, struct pdu * pdu)
{
        static struct seq_queue_entry * tmp, * cur, * last = NULL;
        seq_num_t                       csn, psn;
        const struct pci *              pci;

        ASSERT(q);
        ASSERT(pdu);

        tmp = seq_queue_entry_create_gfp(pdu, GFP_ATOMIC);
        if (!tmp) {
                LOG_ERR("Could not create sequencing queue entry");
                return -1;
        }

        pci = pdu_pci_get_ro(pdu);
        csn = pci_sequence_number_get((struct pci *) pci);

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

        pci  = pdu_pci_get_ro(last->pdu);
        psn  = pci_sequence_number_get((struct pci *) pci);
        if (csn == psn) {
                LOG_ERR("Another PDU with the same seq_num is in "
                        "the seqq");
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
                pci = pdu_pci_get_ro(cur->pdu);
                psn = pci_sequence_number_get((struct pci *) pci);
                if (csn == psn) {
                        LOG_ERR("Another PDU with the same seq_num is in "
                                "the rtx queue!");
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

        spin_lock_init(&tmp->lock);

        return tmp;
}

static int pdu_post(struct dtp * instance,
                    struct pdu * pdu)
{
        struct sdu *    sdu;
        struct buffer * buffer;
        struct efcp *   efcp;

        ASSERT(instance->sv);

        buffer = pdu_buffer_get_rw(pdu);
        sdu    = sdu_create_buffer_with_ni(buffer);
        if (!sdu) {
                pdu_destroy(pdu);
                return -1;
        }

        pdu_buffer_disown(pdu);
        pdu_destroy(pdu);

        efcp = dt_efcp(instance->parent);
        ASSERT(efcp);

        if (efcp_enqueue(efcp,
                         efcp_port_id(efcp),
                         sdu)) {
                LOG_ERR("Could not enqueue SDU to EFCP");
                return -1;
        }
        LOG_DBG("DTP enqueued to upper IPCP");
        return 0;
}

/* Runs the SenderInactivityTimerPolicy */
static void tf_sender_inactivity(void * data)
{
        struct dtp * dtp;
        struct dtp_ps * ps;

        LOG_DBG("Running Stimer...");
        dtp = (struct dtp *) data;
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
static void tf_receiver_inactivity(void * data)
{
        struct dtp * dtp;
        struct dtp_ps * ps;

        LOG_DBG("Running Rtimer...");
        dtp = (struct dtp *) data;
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
const struct pci * process_A_expiration(struct dtp * dtp, struct dtcp * dtcp)
{
        struct dt *              dt;
        struct dtp_sv *          sv;
        struct squeue *          seqq;
        seq_num_t                LWE;
        seq_num_t                seq_num;
        struct pdu *             pdu;
        bool                     in_order_del;
        bool                     incomplete_del;
        bool                     rtx_ctrl = false;
        seq_num_t                max_sdu_gap;
        timeout_t                a;
        struct seq_queue_entry * pos, * n;
        struct dtp_ps *          ps;
        struct dtcp_ps *         dtcp_ps;
        unsigned long            flags;
        struct rqueue *          to_post;
        const struct pci *       pci, * pci_ret = NULL;

        ASSERT(dtp);

        sv = dtp->sv;
        ASSERT(sv);

        dt = dtp->parent;
        ASSERT(dt);

        seqq = dtp->seqq;
        ASSERT(seqq);

        a = dt_sv_a(dt);

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

        /* FIXME: Invoke delimiting */

        LOG_DBG("Processing A timer expiration");

        to_post = rqueue_create_ni();
        if (!to_post) {
                LOG_ERR("Could not create to_post list in A timer");
                return NULL;
        }

        spin_lock_irqsave(&seqq->lock, flags);
        LWE = dt_sv_rcv_lft_win(dt);
        LOG_DBG("LWEU: Original LWE = %u", LWE);
        LOG_DBG("LWEU: MAX GAPS     = %u", max_sdu_gap);

        list_for_each_entry_safe(pos, n, &seqq->queue->head, next) {
                pdu = pos->pdu;
                if (!pdu_is_ok(pdu)) {
                        spin_unlock_irqrestore(&seqq->lock, flags);

                        LOG_ERR("Bogus data, bailing out");
                        return NULL;
                }

                pci     = pdu_pci_get_ro(pdu);
                seq_num = pci_sequence_number_get(pci);
                LOG_DBG("Seq number: %u", seq_num);

                if (seq_num - LWE - 1 <= max_sdu_gap) {

                        if (dt_sv_rcv_lft_win_set(dt, seq_num))
                                LOG_ERR("Could not update LWE while A timer");

                        pos->pdu = NULL;
                        list_del(&pos->next);
                        seq_queue_entry_destroy(pos);

                        if (rqueue_tail_push_ni(to_post, pdu)) {
                                LOG_ERR("Could not post PDU %u while A timer"
                                        "(in-order)", seq_num);
                        }

                        LOG_DBG("Atimer: PDU %u posted", seq_num);

                        LWE = seq_num;
                        pci_ret = pci;
                        continue;
                }

                if (time_before_eq(pos->time_stamp + msecs_to_jiffies(a),
                                   jiffies)) {
                        LOG_DBG("Processing A timer expired");
                        if (dtcp_rtx_ctrl(dtcp_config_get(dtcp))) {
                                LOG_DBG("Retransmissions will be required");
                                list_del(&pos->next);
                                seq_queue_entry_destroy(pos);
                                continue;
                        }

                        if (dt_sv_rcv_lft_win_set(dt, seq_num)) {
                                LOG_ERR("Failed to set new "
                                        "left window edge");
                                list_del(&pos->next);
                                seq_queue_entry_destroy(pos);
                                goto finish;
                        }
                        pos->pdu = NULL;
                        list_del(&pos->next);
                        seq_queue_entry_destroy(pos);

                        if (rqueue_tail_push_ni(to_post, pdu)) {
                                LOG_ERR("Could not post PDU %u while A timer"
                                        "(expiration)", seq_num);
                        }

                        LWE = seq_num;
                        pci_ret = pci;

                        continue;
                }

                break;

        }
finish:
        spin_unlock_irqrestore(&seqq->lock, flags);

        while (!rqueue_is_empty(to_post)) {
                pdu = (struct pdu *) rqueue_head_pop(to_post);
                if (pdu)
                        pdu_post(dtp, pdu);
        }
        rqueue_destroy(to_post, (void (*)(void *)) pdu_destroy);
        LOG_DBG("Finish process_Atimer_expiration");
        return pci_ret;
}
EXPORT_SYMBOL(process_A_expiration);

static bool seqq_is_empty(struct squeue * queue)
{
        bool ret;

        if (!queue)
                return false;

        spin_lock(&queue->lock);
        ret = list_empty(&queue->queue->head) ? true : false;
        spin_unlock(&queue->lock);

        return ret;
}

static void tf_a(void * o)
{
        struct dt *   dt;
        struct dtp *  dtp;
        struct dtcp * dtcp;
        timeout_t     a;

        LOG_DBG("A-timer handler started...");

        dtp = (struct dtp *) o;
        if (!dtp) {
                LOG_ERR("No instance passed to A-timer handler !!!");
                return;
        }

        dt   = dtp->parent;
        dtcp = dt_dtcp(dt);
        a    = dt_sv_a(dt);

        if (dtcp) {
                if (dtcp_sending_ack_policy(dtcp)){
                        LOG_ERR("sending_ack failed");
                        rtimer_start(dtp->timers.a, a/AF);
                        return;
                }
        } else {
                process_A_expiration(dtp, dtcp);
#if DTP_INACTIVITY_TIMERS_ENABLE
                if (rtimer_restart(dtp->timers.sender_inactivity,
                                   3 * (dt_sv_mpl(dt) +
                                        dt_sv_r(dt)   +
                                        dt_sv_a(dt)))) {
                        LOG_ERR("Failed to start sender_inactiviy timer");
                        rtimer_start(dtp->timers.a, a/AF);
                        return;
                }
#endif
        }

        if (!seqq_is_empty(dtp->seqq)) {
                LOG_DBG("Going to restart A timer with a = %d and a/AF = %d",
                        a, a/AF);
                rtimer_start(dtp->timers.a, a/AF);
        }

        return;
}

static unsigned int msec_to_next_rate(uint time_frame, struct timespec * last) {
	struct timespec now = {0,0};
	struct timespec next = {0,0};
	struct timespec diff = {0,0};
	unsigned int ret = 0;

	if(!last) {
		LOG_ERR("%s arg invalid, last: 0x%pK", __func__, last);
		return -1;
	}

	next.tv_sec = last->tv_sec;
	next.tv_nsec = last->tv_nsec;

	while(time_frame >= 1000) {
		next.tv_sec += 1;
		time_frame -= 1000;
	}

	next.tv_nsec += time_frame * 1000000;

	if(next.tv_nsec > 1000000000L) {
		next.tv_sec += 1;
		next.tv_nsec -= 1000000000L;
	}

	getnstimeofday(&now);

	diff = timespec_sub(next, now);
	ret = (diff.tv_sec * 1000) +
		(diff.tv_nsec / 1000000);

	return ret;
}

/* Does not start the timer(return false) if it's not necessary and work can be
 * done.
 */
void dtp_start_rate_timer(struct dtp * dtp, struct dtcp * dtcp) {
	uint rtf = dtcp_time_frame(dtcp);
	uint tf = 0;
	struct timespec t = {0,0};

	if(!rtimer_is_pending(dtp->timers.rate_window)) {
		dtcp_last_time(dtcp, &t);
		tf = msec_to_next_rate(rtf, &t);

		LOG_DBG("Rate based timer start, time %u msec, "
			"last: %lu:%lu",
			tf,
			t.tv_sec,
			t.tv_nsec);

		rtimer_start(dtp->timers.rate_window, tf);
	} else {
		LOG_DBG("rbfc Rate based timer is still pending...");
	}
}

static void tf_rate_window(void * o)
{
        struct dtp *  dtp;
        struct dtcp * dtcp;
        struct cwq *  q;
        struct timespec now  = {0, 0};

        dtp = (struct dtp *) o;

        if (!dtp) {
                LOG_ERR("No DTP found. Cannot run rate window timer");
                return;
        }

        dtcp = dt_dtcp(dtp->parent);

        LOG_DBG("rbfc Timer job start...");
        LOG_DBG("rbfc Re-opening the rate mechanism");

	getnstimeofday(&now);

	efcp_enable_write(dt_efcp(dtp_dt(dtp)));
	dtp_sv_rate_fulfiled_set(dtp, false);
	dtcp_rate_fc_reset(dtcp, &now);

	q = dt_cwq(dtp->parent);
	cwq_deliver(q,
	    dtp->parent,
	    dtp->rmt);

        return;
}

int dtp_sv_init(struct dtp * dtp,
                bool         rexmsn_ctrl,
                bool         window_based,
                bool         rate_based)
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
                        ps->transmission_control = default_transmission_control;
                }
                if (!ps->closed_window) {
                        ps->closed_window = default_closed_window;
                }
                if (!ps->snd_flow_control_overrun) {
                        ps->snd_flow_control_overrun = default_snd_flow_control_overrun;
                }
                if (!ps->initial_sequence_number) {
                        ps->initial_sequence_number = default_initial_sequence_number;
                }
                if (!ps->receiver_inactivity_timer) {
                        ps->receiver_inactivity_timer = default_receiver_inactivity_timer;
                }
                if (!ps->sender_inactivity_timer) {
                        ps->sender_inactivity_timer = default_sender_inactivity_timer;
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

struct dtp * dtp_create(struct dt * dt,
                        struct rmt *        rmt,
                        struct dtp_config * dtp_cfg,
			struct robject *    parent)
{
        struct dtp * tmp;
        string_t *   ps_name;

        if (!dt) {
                LOG_ERR("No DT passed, bailing out");
                return NULL;
        }
        dt_sv_drf_flag_set(dt, true);

        if (!dtp_cfg) {
                LOG_ERR("No DTP conf passed, bailing out");
                return NULL;
        }

        if (!rmt) {
                LOG_ERR("No RMT passed, bailing out");
                return NULL;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp) {
                LOG_ERR("Cannot create DTP instance");
                return NULL;
        }

        tmp->parent = dt;

	if (robject_init_and_add(&tmp->robj,
				 &dtp_rtype,
				 parent,
				 "dtp")) {
                dtp_destroy(tmp);
                return NULL;
	}

        tmp->sv = rkmalloc(sizeof(*tmp->sv), GFP_KERNEL);
        if (!tmp->sv) {
                LOG_ERR("Cannot create DTP state-vector");

                dtp_destroy(tmp);
                return NULL;
        }
        *tmp->sv = default_sv;
        /* FIXME: fixups to the state-vector should be placed here */

        spin_lock_init(&tmp->sv->lock);

        tmp->cfg   = dtp_cfg;

        tmp->rmt  = rmt;
        tmp->seqq = squeue_create(tmp);
        if (!tmp->seqq) {
                LOG_ERR("Could not create Sequencing queue");
                dtp_destroy(tmp);
                return NULL;
        }

        tmp->timers.sender_inactivity   = rtimer_create(tf_sender_inactivity,
                                                        tmp);
        tmp->timers.receiver_inactivity = rtimer_create(tf_receiver_inactivity,
                                                        tmp);
        tmp->timers.a                   = rtimer_create(tf_a, tmp);
        tmp->timers.rate_window         = rtimer_create(tf_rate_window, tmp);
        if (!tmp->timers.sender_inactivity   ||
            !tmp->timers.receiver_inactivity ||
            !tmp->timers.a                   ||
            !tmp->timers.rate_window) {
                dtp_destroy(tmp);
                return NULL;
        }

        rina_component_init(&tmp->base);

        ps_name = (string_t *) policy_name(dtp_conf_ps_get(dtp_cfg));
        if (!ps_name || !strcmp(ps_name, ""))
                ps_name = RINA_PS_DEFAULT_NAME;

        if (dtp_select_policy_set(tmp, "", ps_name)) {
                LOG_ERR("Could not load DTP PS %s", ps_name);
                dtp_destroy(tmp);
                return NULL;
        }

        if (dtp_initial_sequence_number(tmp)) {
                LOG_ERR("Could not create Sequencing queue");
                dtp_destroy(tmp);
                return NULL;
        }

        LOG_DBG("Instance %pK created successfully", tmp);

        return tmp;
}

int dtp_destroy(struct dtp * instance)
{
        if (!instance) {
                LOG_ERR("Bad instance passed, bailing out");
                return -1;
        }

        if (instance->timers.a)
                rtimer_destroy(instance->timers.a);
        /* tf_a posts workers that restart sender_inactivity timer, so the wq
         * must be flushed before destroying the timer */

        if (instance->timers.sender_inactivity)
                rtimer_destroy(instance->timers.sender_inactivity);
        if (instance->timers.receiver_inactivity)
                rtimer_destroy(instance->timers.receiver_inactivity);
        if (instance->timers.rate_window)
                rtimer_destroy(instance->timers.rate_window);

        if (instance->seqq) squeue_destroy(instance->seqq);
        if (instance->sv)   rkfree(instance->sv);
        if (instance->cfg) dtp_config_destroy(instance->cfg);
        rina_component_fini(&instance->base);

	robject_del(&instance->robj);
        rkfree(instance);

        LOG_DBG("Instance %pK destroyed successfully", instance);

        return 0;
}

static bool window_is_closed(struct dtp *    dtp,
                             struct dt *     dt,
                             struct dtcp *   dtcp,
                             seq_num_t       seq_num,
                             struct dtp_ps * ps)
{
        bool retval = false, w_ret = false, r_ret = false;
        bool rb, wb;
        struct dtp_sv * sv;

        ASSERT(dtp);
        ASSERT(dt);
        ASSERT(dtcp);

        sv = dtp->sv;
        ASSERT(sv);

        wb = sv->window_based;
        if (wb && seq_num > dtcp_snd_rt_win(dtcp)) {
                dt_sv_window_closed_set(dt, true);
                w_ret = true;
        }

        rb = sv->rate_based;
        if (rb) {
        	if(dtcp_rate_exceeded(dtcp, 1)) {
        		dtp_sv_rate_fulfiled_set(dtp, true);
        		dtp_start_rate_timer(dtp, dtcp);
			r_ret = true;
		} else {
			if (dtp_sv_rate_fulfiled(dtp)) {
				LOG_DBG("rbfc Re-opening the rate mechanism");
				dtp_sv_rate_fulfiled_set(dtp, false);
			}
		}
        }

        retval = (w_ret || r_ret);

        if(w_ret && r_ret)
        	retval = ps->reconcile_flow_conflict(ps);

        return retval;
}

int dtp_write(struct dtp * instance,
              struct sdu * sdu)
{
        struct pdu *            pdu;
        struct pci *            pci;
        struct dtp_sv *         sv;
        struct dt *             dt;
        struct dtcp *           dtcp;
        struct rtxq *           rtxq;
        struct pdu *            cpdu;
        struct dtp_ps *         ps;
        seq_num_t               sn, csn;
        struct efcp *           efcp;
	unsigned long           flags;
        int			sbytes;
        uint_t                  sc;

        if (!sdu_is_ok(sdu))
		return -1;

        if (!instance || !instance->rmt) {
                LOG_ERR("Bogus instance passed, bailing out");
		goto sdu_err_exit;
        }

        dt = instance->parent;
        if (!dt) {
                LOG_ERR("Bogus DT passed, bailing out");
		goto sdu_err_exit;
        }

        efcp = dt_efcp(dt);
        if (!efcp) {
                LOG_ERR("Bogus EFCP passed, bailing out");
		goto sdu_err_exit;
        }

        /* State Vector must not be NULL */
        sv = instance->sv;
        if (!sv) {
                LOG_ERR("Bogus DTP-SV passed, bailing out");
		goto sdu_err_exit;
        }

        dtcp = dt_dtcp(dt);

#if DTP_INACTIVITY_TIMERS_ENABLE
        /* Stop SenderInactivityTimer */
        if (rtimer_stop(instance->timers.sender_inactivity)) {
                LOG_ERR("Failed to stop timer");
        }
#endif
        pci = pci_create_ni();
        if (!pci) {
		goto sdu_err_exit;
        }

        /* Step 1: Delimiting (fragmentation/reassembly) + Protection */

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

        csn = nxt_seq_get(sv);
        if (pci_format(pci,
                       efcp_src_cep_id(efcp),
                       efcp_dst_cep_id(efcp),
                       efcp_src_addr(efcp),
                       efcp_dst_addr(efcp),
                       csn,
                       efcp_qos_id(efcp),
                       PDU_TYPE_DT))
		goto pci_err_exit;

        sn = dtcp_snd_lf_win(dtcp);
        if (dt_sv_drf_flag(dt)          ||
            (sn == (csn - 1))           ||
            !sv->rexmsn_ctrl) {
		pdu_flags_t pci_flags;
		pci_flags = pci_flags_get(pci);
		pci_flags |= PDU_FLAGS_DATA_RUN;
                pci_flags_set(pci, pci_flags);
	}

        pdu = pdu_create_ni();
        if (!pdu)
		goto pci_err_exit;

        if (pdu_buffer_set(pdu, sdu_buffer_rw(sdu)))
		goto pci_err_exit;

        if (pdu_pci_set(pdu, pci)) {
                sdu_buffer_disown(sdu);
                pdu_destroy(pdu);
		goto pci_err_exit;
        }

        sdu_buffer_disown(sdu);
        sdu_destroy(sdu);

        LOG_DBG("DTP Sending PDU %u (CPU: %d)", csn, smp_processor_id());

	sbytes = buffer_length(pdu_buffer_get_ro(pdu));
        if (dtcp) {
                rcu_read_lock();
                ps = container_of(rcu_dereference(instance->base.ps),
                                  struct dtp_ps, base);
                if (sv->window_based || sv->rate_based) {
			/* NOTE: Might close window */
			if (window_is_closed(instance,
						dt,
						dtcp,
						csn,
						ps)) {
				if (ps->closed_window(ps, pdu)) {
					LOG_ERR("Problems with the "
						"closed window policy");
					goto stats_err_exit;
				}
				rcu_read_unlock();
				return 0;
			}
			if(sv->rate_based) {
				sc = dtcp_sent_itu(dtcp);
				if(sbytes >= 0) {
					if (sbytes + sc >= dtcp_sndr_rate(dtcp))
						dtcp_sent_itu_set(
							dtcp,
							dtcp_sndr_rate(dtcp));
					else
						dtcp_sent_itu_inc(dtcp, sbytes);
				}
			}
                }
                if (sv->rexmsn_ctrl) {
                        /* FIXME: Add timer for PDU */
                        rtxq = dt_rtxq(dt);
                        if (!rtxq) {
                                LOG_ERR("Failed to get rtxq");
				goto pdu_err_exit;
                        }

                        cpdu = pdu_dup_ni(pdu);
                        if (!cpdu) {
                                LOG_ERR("Failed to copy PDU");
                                LOG_ERR("PDU ok? %d", pdu_pci_present(pdu));
                                LOG_ERR("PDU type: %d",
                                        pci_type(pdu_pci_get_ro(pdu)));
				goto pdu_err_exit;
                        }

                        if (rtxq_push_ni(rtxq, cpdu)) {
                                LOG_ERR("Couldn't push to rtxq");
				goto pdu_err_exit;
                        }
                }
                if (ps->transmission_control(ps, pdu)) {
                        LOG_ERR("Problems with transmission "
                                "control");
			goto stats_err_exit;
                }

                rcu_read_unlock();
		stats_inc_bytes(tx, sv, sbytes, flags);
#if DTP_INACTIVITY_TIMERS_ENABLE
                /* Start SenderInactivityTimer */
                if (rtimer_restart(instance->timers.sender_inactivity,
                                   3 * (dt_sv_mpl(dt) +
                                        dt_sv_r(dt)   +
                                        dt_sv_a(dt)))) {
                        LOG_ERR("Failed to start sender_inactiviy timer");
			goto stats_nounlock_err_exit;
                        return -1;
                }
#endif
                return 0;
        }

        if (dt_pdu_send(dt,
                        instance->rmt,
                        pdu))
		return -1;
	stats_inc_bytes(tx, sv, sbytes, flags);
	return 0;

pci_err_exit:
	pci_destroy(pci);
sdu_err_exit:
	sdu_destroy(sdu);
	return -1;

pdu_err_exit:
	pdu_destroy(pdu);
stats_err_exit:
        rcu_read_unlock();
stats_nounlock_err_exit:
	stats_inc(err, sv, flags);
	return -1;
}

void dtp_drf_required_set(struct dtp * dtp)
{ dtp->sv->drf_required = true; }

/*
static bool is_drf_required(struct dtp * dtp)
{
        unsigned long flags;

        spin_lock_irqsave(&dtp->sv->lock, flags);
        ret = dtp->sv->drf_required;
        spin_unlock_irqrestore(&dtp->sv->lock, flags);
        return ret;
}
*/

static bool is_fc_overrun(
	struct dt * dt, struct dtcp * dtcp, seq_num_t seq_num, int pdul)
{
        bool to_ret, w_ret = false, r_ret = false;

        if (!dtcp)
                return false;

        if (dtcp_window_based_fctrl(dtcp_config_get(dtcp))) {
                w_ret = (seq_num > dtcp_rcv_rt_win(dtcp));
        }

        if (dtcp_rate_based_fctrl(dtcp_config_get(dtcp))){
        	if(pdul >= 0) {
        		dtcp_recv_itu_inc(dtcp, pdul);
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

        sn = pci_sequence_number_get(pdu_pci_get_ro(p->pdu));
        if (sn == (LWE+1))
                return true;

        return false;
}

int dtp_receive(struct dtp * instance,
                struct pdu * pdu)
{
        struct dtp_ps *  ps;
        struct pci *     pci;
        struct dtp_sv *  sv;
        struct dtcp *    dtcp;
        struct dtcp_ps * dtcp_ps;
        struct dt *      dt;
        seq_num_t        seq_num;
        timeout_t        a;
        seq_num_t        LWE;
        bool             in_order;
        bool             rtx_ctrl = false;
        seq_num_t        max_sdu_gap;
        unsigned long    flags;
        struct rqueue *  to_post;
	int              sbytes;
	struct efcp *		efcp = 0;

        LOG_DBG("DTP receive started...");

        if (!pdu_is_ok(pdu)) {
                pdu_destroy(pdu);
                return -1;
        }

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                pdu_destroy(pdu);
                return -1;
        }

        if (!instance                  ||
            !instance->parent          ||
            !instance->sv) {
                LOG_ERR("Bogus instance passed, bailing out");
                pdu_destroy(pdu);
                return -1;
        }

        sv = instance->sv;
        ASSERT(sv); /* State Vector must not be NULL */

        dt = instance->parent;
        ASSERT(dt);

        dtcp = dt_dtcp(dt);

	efcp = dt_efcp(dt);
	ASSERT(efcp);

        if (!pdu_pci_present(pdu)) {
                LOG_DBG("Couldn't find PCI in PDU");
                pdu_destroy(pdu);
                return -1;
        }
        pci = pdu_pci_get_rw(pdu);

        a           = dt_sv_a(dt);
        LWE         = dt_sv_rcv_lft_win(dt);
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

        seq_num = pci_sequence_number_get(pci);
	sbytes = buffer_length(pdu_buffer_get_ro(pdu));

        LOG_DBG("local_soft_irq_pending: %d", local_softirq_pending());
        LOG_DBG("DTP Received PDU %u (CPU: %d)",
                seq_num, smp_processor_id());

        if (instance->sv->drf_required) {
#if DTP_INACTIVITY_TIMERS_ENABLE
                /* Start ReceiverInactivityTimer */
                if (rtimer_restart(instance->timers.receiver_inactivity,
                                   2 * (dt_sv_mpl(dt) +
                                        dt_sv_r(dt)   +
                                        dt_sv_a(dt)))) {
                        LOG_ERR("Failed to start Receiver Inactivity timer");
                        pdu_destroy(pdu);
                        return -1;
                }
#endif
                if ((pci_flags_get(pci) & PDU_FLAGS_DATA_RUN)) {
                        instance->sv->drf_required = false;
                        spin_lock_irqsave(&instance->seqq->lock, flags);
                        dtp_squeue_flush(instance);
                        dt_sv_rcv_lft_win_set(dt, seq_num);
                        spin_unlock_irqrestore(&instance->seqq->lock, flags);
                        if (dtcp) {
                                if (dtcp_sv_update(dtcp, pci)) {
                                        LOG_ERR("Failed to update dtcp sv");
                                        return -1;
                                }
                        }
                        pdu_post(instance, pdu);
			stats_inc_bytes(rx, sv, sbytes, flags);
                        LOG_DBG("Data run flag DRF");
                        return 0;
                }
                LOG_ERR("Expecting DRF but not present, dropping PDU %d...",
                        seq_num);
		stats_inc(drop, sv, flags);
                pdu_destroy(pdu);
                return 0;
        }
        /*
         * NOTE:
         *   no need to check presence of in_order or dtcp because in case
         *   they are not, LWE is not updated and always 0
         */

        if ((seq_num <= LWE) || (is_fc_overrun(dt, dtcp, seq_num, sbytes)))
        {
                pdu_destroy(pdu);
                stats_inc(drop, sv, flags);

                /*FIXME: Rtimer should not be restarted here, to be deleted */
#if DTP_INACTIVITY_TIMERS_ENABLE
                /* Start ReceiverInactivityTimer */
                if (rtimer_restart(instance->timers.receiver_inactivity,
                                   3 * (dt_sv_mpl(dt) +
                                        dt_sv_r(dt)   +
                                        dt_sv_a(dt))))
                        LOG_ERR("Failed restart RcvrInactivity timer");
#endif

                /* Send an ACK/Flow Control PDU with current window values */
                if (dtcp) {
                        if (dtcp_ack_flow_control_pdu_send(dtcp, LWE)) {
                                LOG_ERR("Failed to send ack / flow "
                                        "control pdu");
                                return -1;
                        }
                }
                return 0;
        }

        /*NOTE: Just for debugging
        if (dtcp && seq_num > dtcp_rcv_rt_win(dtcp)) {
                LOG_INFO("PDU Scep-id %u Dcep-id %u SeqN %u, RWE: %u",
                         pci_cep_source(pci), pci_cep_destination(pci),
                         seq_num, dtcp_rcv_rt_win(dtcp));
        }
        */

#if DTP_INACTIVITY_TIMERS_ENABLE
        /* Start ReceiverInactivityTimer */
        if (rtimer_restart(instance->timers.receiver_inactivity,
                           2 * (dt_sv_mpl(dt) +
                                dt_sv_r(dt)   +
                                dt_sv_a(dt)))) {
                LOG_ERR("Failed to start Receiver Inactivity timer");
                pdu_destroy(pdu);
                return -1;
        }
#endif
        if (!a) {
                bool set_lft_win_edge;

                /* FIXME: delimiting goes here */
                if (!in_order && !dtcp) {
                        LOG_DBG("DTP Receive deliver, seq_num: %d, LWE: %d",
                                seq_num, LWE);
                        if (pdu_post(instance, pdu))
                                return -1;

                        return 0;
                }

                set_lft_win_edge = !(dtcp_rtx_ctrl(dtcp_config_get(dtcp)) &&
                                     ((seq_num -LWE) > (max_sdu_gap + 1)));

                if (set_lft_win_edge) {
                        if (dt_sv_rcv_lft_win_set(dt, seq_num)) {
                                LOG_ERR("Failed to set new left window edge");
                                goto fail;
                        }
                }

                if (dtcp) {
                        if (dtcp_sv_update(dtcp, pci)) {
                                LOG_ERR("Failed to update dtcp sv");
                                goto fail;
                        }
                        if (!set_lft_win_edge) {
                                pdu_destroy(pdu);
                                return 0;
                        }
                }

                if (pdu_post(instance, pdu))
                        return -1;

		stats_inc_bytes(rx, sv, sbytes, flags);
                return 0;

        fail:
                pdu_destroy(pdu);
                return -1;
        }

        spin_lock_irqsave(&instance->seqq->lock, flags);
        to_post = rqueue_create_ni();
        if (!to_post) {
                LOG_ERR("Could not create to_post queue");
                pdu_destroy(pdu);
                return -1;
        }

        LWE = dt_sv_rcv_lft_win(dt);
        LOG_DBG("DTP receive LWE: %u", LWE);
        if (seq_num == LWE + 1) {
                dt_sv_rcv_lft_win_set(dt, seq_num);
                rqueue_tail_push_ni(to_post, pdu);
                LWE = seq_num;
        } else {
                seq_queue_push_ni(instance->seqq->queue, pdu);
        }
        while (are_there_pdus(instance->seqq->queue, LWE)) {
                pdu = seq_queue_pop(instance->seqq->queue);
                if (!pdu)
                        break;
                pci     = pdu_pci_get_rw(pdu);
                seq_num = pci_sequence_number_get(pci);
                LWE     = seq_num;
                dt_sv_rcv_lft_win_set(dt, seq_num);
                rqueue_tail_push_ni(to_post, pdu);
        }
        spin_unlock_irqrestore(&instance->seqq->lock, flags);

        if (list_empty(&instance->seqq->queue->head))
                rtimer_stop(instance->timers.a);
        else
                rtimer_start(instance->timers.a, a/AF);

        if (dtcp) {
                if (dtcp_sv_update(dtcp, pci)) {
                        LOG_ERR("Failed to update dtcp sv");
                }
        }
        while (!rqueue_is_empty(to_post)) {
                pdu = (struct pdu *) rqueue_head_pop(to_post);
                if (pdu)
                        pdu_post(instance, pdu);
			stats_inc_bytes(rx, sv, sbytes, flags);
        }
        rqueue_destroy(to_post, (void (*)(void *)) pdu_destroy);

        LOG_DBG("DTP receive ended...");
        return 0;
}

struct rtimer * dtp_sender_inactivity_timer(struct dtp * instance)
{
        return instance->timers.sender_inactivity;
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
