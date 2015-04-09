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
#include "dtp.h"
#include "dt.h"
#include "dt-utils.h"
#include "dtcp.h"
#include "dtcp-ps.h"
#include "dtcp-utils.h"
#include "ps-factory.h"
#include "dtp-ps.h"

static struct policy_set_list policy_sets = {
        .head = LIST_HEAD_INIT(policy_sets.head)
};

/* This is the DT-SV part maintained by DTP */
struct dtp_sv {
        spinlock_t          lock;

        /* Configuration values */
        struct connection * connection; /* FIXME: Are we really sure ??? */

        uint_t              seq_number_rollover_threshold;
        uint_t              dropped_pdus;
        seq_num_t           max_seq_nr_rcv;
        seq_num_t           seq_nr_to_send;
        seq_num_t           max_seq_nr_sent;

        bool                window_based;
        bool                rexmsn_ctrl;
        bool                rate_based;
        timeout_t           a;
        bool                drf_required;
};

struct dtp {
        struct dt *               parent;
        /*
         * NOTE: The DTP State Vector is discarded only after and explicit
         *       release by the AP or by the system (if the AP crashes).
         */
        struct dtp_sv *           sv; /* The state-vector */

        struct rina_component     base;
        struct rmt *              rmt;
        struct efcp *             efcp;
        struct squeue *           seqq;
        struct {
                struct rtimer * sender_inactivity;
                struct rtimer * receiver_inactivity;
                struct rtimer * a;
        } timers;
};

static struct dtp_sv default_sv = {
        .connection                    = NULL,
        .seq_nr_to_send                = 0,
        .max_seq_nr_sent               = 0,
        .seq_number_rollover_threshold = 0,
        .dropped_pdus                  = 0,
        .max_seq_nr_rcv                = 0,
        .rexmsn_ctrl                   = false,
        .rate_based                    = false,
        .window_based                  = false,
        .a                             = 0,
        .drf_required                  = true,
};

struct dt * dtp_dt(struct dtp * dtp)
{
        return dtp->parent;
}
EXPORT_SYMBOL(dtp_dt);

struct rmt * dtp_rmt(struct dtp * dtp)
{
        return dtp->rmt;
}
EXPORT_SYMBOL(dtp_rmt);

struct dtp_sv * dtp_dtp_sv(struct dtp * dtp)
{
        return dtp->sv;
}
EXPORT_SYMBOL(dtp_dtp_sv);

struct connection * dtp_sv_connection(struct dtp_sv * sv)
{
        return sv->connection;
}
EXPORT_SYMBOL(dtp_sv_connection);

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
EXPORT_SYMBOL(nxt_seq_reset);

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
EXPORT_SYMBOL(dtp_sv_last_nxt_seq_nr);

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
EXPORT_SYMBOL(dtp_sv_max_seq_nr_sent);

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
EXPORT_SYMBOL(dtp_sv_max_seq_nr_set);

#if 0
static uint_t dropped_pdus(struct dtp_sv * sv)
{
        uint_t tmp;

        ASSERT(sv);

        spin_lock(&sv->lock);
        tmp = sv->dropped_pdus;
        spin_unlock(&sv->lock);

        return tmp;
}
#endif

static void dropped_pdus_inc(struct dtp_sv * sv)
{
        unsigned long flags;

        ASSERT(sv);

        spin_lock_irqsave(&sv->lock, flags);
        sv->dropped_pdus++;
        spin_unlock_irqrestore(&sv->lock, flags);
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
EXPORT_SYMBOL(dtp_initial_sequence_number);

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
EXPORT_SYMBOL(dtp_squeue_flush);

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
                        "the rtx queue!");
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

        ASSERT(instance->sv);

        buffer = pdu_buffer_get_rw(pdu);
        sdu    = sdu_create_buffer_with_ni(buffer);
        if (!sdu) {
                pdu_destroy(pdu);
                return -1;
        }

        pdu_buffer_disown(pdu);
        pdu_destroy(pdu);

        ASSERT(instance->sv->connection);

        if (efcp_enqueue(instance->efcp,
                         instance->sv->connection->port_id,
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
seq_num_t process_A_expiration(struct dtp * dtp, struct dtcp * dtcp)
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
        seq_num_t                ret;
        unsigned long            flags;
        /*struct rqueue *          to_pos*/

        ASSERT(dtp);

        sv = dtp->sv;
        ASSERT(sv);

        dt = dtp->parent;
        ASSERT(dt);

        seqq = dtp->seqq;
        ASSERT(seqq);

        a = dt_sv_a(dt);

        ASSERT(sv->connection);
        ASSERT(sv->connection->policies_params);

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

        /*to_post = rqueue_create_ni();
        if (!to_post) {
                LOG_ERR("Could not create to_post list in A timer");
                return -1;
        }*/

        spin_lock_irqsave(&seqq->lock, flags);
        LWE = dt_sv_rcv_lft_win(dt);
        ret = LWE;
        LOG_DBG("LWEU: Original LWE = %u", LWE);
        LOG_DBG("LWEU: MAX GAPS     = %u", max_sdu_gap);

        list_for_each_entry_safe(pos, n, &seqq->queue->head, next) {
                pdu = pos->pdu;
                if (!pdu_is_ok(pdu)) {
                        spin_unlock_irqrestore(&seqq->lock, flags);

                        LOG_ERR("Bogus data, bailing out");
                        return LWE;
                }

                seq_num = pci_sequence_number_get(pdu_pci_get_ro(pdu));
                LOG_DBG("Seq number: %u", seq_num);

                if (seq_num - LWE - 1 <= max_sdu_gap) {

                        if (dt_sv_rcv_lft_win_set(dt, seq_num))
                                LOG_ERR("Could not update LWE while A timer");

                        pos->pdu = NULL;
                        list_del(&pos->next);
                        seq_queue_entry_destroy(pos);


                        /*spin_unlock(&seqq->lock);*/
                        if (pdu_post(dtp, pdu)) {
                                LOG_ERR("Could not post PDU %u while A timer"
                                        "(in-order)", seq_num);
                        }

                        LOG_DBG("Atimer: PDU %u posted", seq_num);

                        /*spin_lock(&seqq->lock);
                        rqueue_tail_push_ni(to_post, pdu);*/

                        LWE = dt_sv_rcv_lft_win(dt);
                        ret = LWE;
                        continue;
                }

                if (time_before_eq(pos->time_stamp + msecs_to_jiffies(a),
                                   jiffies)) {
                        LOG_DBG("Processing A timer expired");
                        if (dtcp && dtcp_rtx_ctrl(dtcp_config_get(dtcp))) {
                                LOG_DBG("Retransmissions will be required");
                                ret = seq_num;
                                goto finish;
                        }

                        if (dt_sv_rcv_lft_win_set(dt, seq_num)) {
                                LOG_ERR("Failed to set new "
                                        "left window edge");
                                goto finish;
                        }
                        pos->pdu = NULL;
                        list_del(&pos->next);
                        seq_queue_entry_destroy(pos);

                        /*spin_unlock(&seqq->lock);*/
                        if (pdu_post(dtp, pdu)) {
                                LOG_ERR("Could not post PDU %u while A timer"
                                        "(expiration)", seq_num);
                        }

                        /*spin_lock(&seqq->lock);
                        rqueue_tail_push_ni(to_post, pdu);*/

                        LWE = dt_sv_rcv_lft_win(dt);
                        ret = LWE;

                        continue;
                }

                break;

        }
finish:
        spin_unlock_irqrestore(&seqq->lock, flags);

        /*while (!rqueue_is_empty(to_post)) {
                pdu = (struct pdu *) rqueue_head_pop(to_post);
                if (pdu)
                        pdu_post(dtp, pdu);
        }
        rqueue_destroy(to_post, (void (*)(void *)) pdu_destroy);*/
        LOG_DBG("Finish process_Atimer_expiration");
        return ret;
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

int dtp_sv_init(struct dtp * dtp,
                bool         rexmsn_ctrl,
                bool         window_based,
                bool         rate_based,
                timeout_t    a)
{
        ASSERT(dtp);
        ASSERT(dtp->sv);

        dtp->sv->rexmsn_ctrl  = rexmsn_ctrl;
        dtp->sv->window_based = window_based;
        dtp->sv->rate_based   = rate_based;
        dtp->sv->a            = a;

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
        struct conn_policies *params = dtp->sv->connection->policies_params;
        struct dtp_ps * ps;
        int ret;

        if (path && strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        ret = base_select_policy_set(&dtp->base, &policy_sets, name);
        if (ret) {
                return ret;
        }

        /* Copy the connection parameter to the policy-set. From now on
         * these connection parameters must be accessed by the DTP policy set,
         * and not from the struct connection. */
        mutex_lock(&dtp->base.ps_lock);
        ps = container_of(dtp->base.ps, struct dtp_ps, base);
        ps->dtcp_present        = params->dtcp_present;
        ps->seq_num_ro_th       = params->seq_num_ro_th;
        ps->initial_a_timer     = params->initial_a_timer;
        ps->partial_delivery    = params->partial_delivery;
        ps->incomplete_delivery = params->incomplete_delivery;
        ps->in_order_delivery   = params->in_order_delivery;
        ps->max_sdu_gap         = params->max_sdu_gap;
        mutex_unlock(&dtp->base.ps_lock);

        return 0;
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

struct dtp * dtp_create(struct dt *         dt,
                        struct rmt *        rmt,
                        struct efcp *       efcp,
                        struct connection * connection)
{
        struct dtp * tmp;

        if (!dt) {
                LOG_ERR("No DT passed, bailing out");
                return NULL;
        }
        dt_sv_drf_flag_set(dt, true);

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

        tmp->sv = rkmalloc(sizeof(*tmp->sv), GFP_KERNEL);
        if (!tmp->sv) {
                LOG_ERR("Cannot create DTP state-vector");

                dtp_destroy(tmp);
                return NULL;
        }
        *tmp->sv = default_sv;
        /* FIXME: fixups to the state-vector should be placed here */

        spin_lock_init(&tmp->sv->lock);

        tmp->sv->connection = connection;

        tmp->rmt            = rmt;
        tmp->efcp           = efcp;
        tmp->seqq           = squeue_create(tmp);
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
        if (!tmp->timers.sender_inactivity   ||
            !tmp->timers.receiver_inactivity ||
            !tmp->timers.a) {
                dtp_destroy(tmp);
                return NULL;
        }

        rina_component_init(&tmp->base);

        /* Try to select the default policy-set. */
        if (dtp_select_policy_set(tmp, "", RINA_PS_DEFAULT_NAME)) {
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

        if (instance->seqq) squeue_destroy(instance->seqq);
        if (instance->sv)   rkfree(instance->sv);
        rina_component_fini(&instance->base);

        rkfree(instance);

        LOG_DBG("Instance %pK destroyed successfully", instance);

        return 0;
}

static bool window_is_closed(struct dtp_sv * sv,
                             struct dt *     dt,
                             struct dtcp *   dtcp,
                             seq_num_t       seq_num)
{
        bool retval = false;

        ASSERT(sv);
        ASSERT(dt);
        ASSERT(dtcp);

        if (dt_sv_window_closed(dt))
                return true;

        if (sv->window_based && seq_num > dtcp_snd_rt_win(dtcp)) {
                dt_sv_window_closed_set(dt, true);
                retval = true;
        }

        if (sv->rate_based)
                LOG_MISSING;

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
        struct dtp_ps * ps;
        seq_num_t               sn;

        if (!sdu_is_ok(sdu))
                return -1;

        if (!instance || !instance->rmt) {
                LOG_ERR("Bogus instance passed, bailing out");
                sdu_destroy(sdu);
                return -1;
        }

        dt = instance->parent;
        if (!dt) {
                LOG_ERR("Bogus DT passed, bailing out");
                sdu_destroy(sdu);
                return -1;
        }

        /* State Vector must not be NULL */
        sv = instance->sv;
        if (!sv) {
                LOG_ERR("Bogus DTP-SV passed, bailing out");
                sdu_destroy(sdu);
                return -1;
        }

        if (!sv->connection) {
                LOG_ERR("Bogus SV connection passed, bailing out");
                sdu_destroy(sdu);
                return -1;
        }

        dtcp = dt_dtcp(dt);

#if DTP_INACTIVITY_TIMERS_ENABLE
        /* Stop SenderInactivityTimer */
        if (rtimer_stop(instance->timers.sender_inactivity)) {
                LOG_ERR("Failed to stop timer");
                /* sdu_destroy(sdu);
                   return -1; */
        }
#endif
        pci = pci_create_ni();
        if (!pci) {
                sdu_destroy(sdu);
                return -1;
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

        if (pci_format(pci,
                       sv->connection->source_cep_id,
                       sv->connection->destination_cep_id,
                       sv->connection->source_address,
                       sv->connection->destination_address,
                       nxt_seq_get(sv),
                       sv->connection->qos_id,
                       PDU_TYPE_DT)) {
                pci_destroy(pci);
                sdu_destroy(sdu);
                return -1;
        }
        sn = dtcp_snd_lf_win(dtcp);
        if (dt_sv_drf_flag(dt)                         ||
            (sn == (pci_sequence_number_get(pci) - 1)) ||
            !sv->rexmsn_ctrl)
                pci_flags_set(pci, PDU_FLAGS_DATA_RUN);

        pdu = pdu_create_ni();
        if (!pdu) {
                pci_destroy(pci);
                sdu_destroy(sdu);
                return -1;
        }

        if (pdu_buffer_set(pdu, sdu_buffer_rw(sdu))) {
                pci_destroy(pci);
                sdu_destroy(sdu);
                return -1;
        }

        if (pdu_pci_set(pdu, pci)) {
                sdu_buffer_disown(sdu);
                pdu_destroy(pdu);
                sdu_destroy(sdu);
                pci_destroy(pci);
                return -1;
        }

        sdu_buffer_disown(sdu);
        sdu_destroy(sdu);

        LOG_DBG("DTP Sending PDU %u (CPU: %d)",
                pci_sequence_number_get(pci), smp_processor_id());

        if (dtcp) {
                rcu_read_lock();
                ps = container_of(rcu_dereference(instance->base.ps),
                                  struct dtp_ps, base);
                if (sv->window_based || sv->rate_based) {
                        /* NOTE: Might close window */
                        if (window_is_closed(sv,
                                             dt,
                                             dtcp,
                                             pci_sequence_number_get(pci))) {
                                if (ps->closed_window(ps, pdu)) {
                                        rcu_read_unlock();
                                        LOG_ERR("Problems with the "
                                                "closed window policy");
                                        return -1;
                                }
                                rcu_read_unlock();
                                return 0;
                        }
                }
                if (sv->rexmsn_ctrl) {
                        /* FIXME: Add timer for PDU */
                        rtxq = dt_rtxq(dt);
                        if (!rtxq) {
                                LOG_ERR("Failed to get rtxq");
                                rcu_read_unlock();
                                pdu_destroy(pdu);
                                return -1;
                        }

                        cpdu = pdu_dup_ni(pdu);
                        if (!cpdu) {
                                LOG_ERR("Failed to copy PDU");
                                LOG_ERR("PDU ok? %d", pdu_pci_present(pdu));
                                LOG_ERR("PDU type: %d",
                                        pci_type(pdu_pci_get_ro(pdu)));
                                rcu_read_unlock();
                                pdu_destroy(pdu);
                                return -1;
                        }

                        if (rtxq_push_ni(rtxq, cpdu)) {
                                LOG_ERR("Couldn't push to rtxq");
                                rcu_read_unlock();
                                pdu_destroy(pdu);
                                return -1;
                        }
                }
                if (ps->transmission_control(ps, pdu)) {
                        LOG_ERR("Problems with transmission "
                                "control");
                        rcu_read_unlock();
                        return -1;
                }

#if DTP_INACTIVITY_TIMERS_ENABLE
                /* Start SenderInactivityTimer */
                if (rtimer_restart(instance->timers.sender_inactivity,
                                   3 * (dt_sv_mpl(dt) +
                                        dt_sv_r(dt)   +
                                        dt_sv_a(dt)))) {
                        LOG_ERR("Failed to start sender_inactiviy timer");
                        rcu_read_unlock();
                        return -1;
                }
#endif
                rcu_read_unlock();
                return 0;
        }

        return dt_pdu_send(dt,
                           instance->rmt,
                           pci_destination(pci),
                           pci_qos_id(pci),
                           pdu);
}

void dtp_drf_required_set(struct dtp * dtp)
{ dtp->sv->drf_required = true; }
EXPORT_SYMBOL(dtp_drf_required_set);

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
        /*struct rqueue *       to_post;*/

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
            !instance->efcp            ||
            !instance->sv              ||
            !instance->sv->connection) {
                LOG_ERR("Bogus instance passed, bailing out");
                pdu_destroy(pdu);
                return -1;
        }

        sv = instance->sv;
        ASSERT(sv); /* State Vector must not be NULL */

        dt = instance->parent;
        ASSERT(dt);

        dtcp = dt_dtcp(dt);

        if (!pdu_pci_present(pdu)) {
                LOG_DBG("Couldn't find PCI in PDU");
                pdu_destroy(pdu);
                return -1;
        }
        pci = pdu_pci_get_rw(pdu);

        a           = instance->sv->a;
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
                        pdu_post(instance, pdu);
                        spin_unlock_irqrestore(&instance->seqq->lock, flags);
                        LOG_DBG("Data run flag DRF");
                        if (dtcp) {
                                if (dtcp_sv_update(dtcp, seq_num)) {
                                        LOG_ERR("Failed to update dtcp sv");
                                        return -1;
                                }
                        }

                        return 0;
                }
                LOG_DBG("Expecting DRF but not present, dropping PDU %d...",
                        seq_num);
                pdu_destroy(pdu);
                return 0;
        }
        /*
         * NOTE:
         *   no need to check presence of in_order or dtcp because in case
         *   they are not, LWE is not updated and always 0
         */
        if ((seq_num <= LWE) || (dtcp && (seq_num > dtcp_rcv_rt_win(dtcp)))) {
                pdu_destroy(pdu);

                dropped_pdus_inc(sv);

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

        /*NOTE: Just for debugging */
        if (dtcp && seq_num > dtcp_rcv_rt_win(dtcp)) {
                LOG_INFO("PDU Scep-id %u Dcep-id %u SeqN %u, RWE: %u",
                         pci_cep_source(pci), pci_cep_destination(pci),
                         seq_num, dtcp_rcv_rt_win(dtcp));
        }

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
                                     ((seq_num -LWE) > max_sdu_gap));

                if (set_lft_win_edge) {
                        if (dt_sv_rcv_lft_win_set(dt, seq_num)) {
                                LOG_ERR("Failed to set new left window edge");
                                goto fail;
                        }
                }

                if (dtcp) {
                        if (dtcp_sv_update(dtcp, seq_num)) {
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

                return 0;

        fail:
                pdu_destroy(pdu);
                return -1;
        }

        spin_lock_irqsave(&instance->seqq->lock, flags);
        LWE = dt_sv_rcv_lft_win(dt);
        LOG_DBG("DTP receive LWE: %u", LWE);
        while (pdu && (seq_num == LWE + 1)) {
                dt_sv_rcv_lft_win_set(dt, seq_num);

                pdu_post(instance, pdu);

                pdu     = seq_queue_pop(instance->seqq->queue);
                LWE     = dt_sv_rcv_lft_win(dt);
                if (!pdu)
                        break;
                seq_num = pci_sequence_number_get(pdu_pci_get_rw(pdu));
        }
        if (pdu)
                seq_queue_push_ni(instance->seqq->queue, pdu);
        spin_unlock_irqrestore(&instance->seqq->lock, flags);

        if (!pdu && a)
                rtimer_stop(instance->timers.a);
        else if (pdu && a) {
                LOG_DBG("Going to restart A timer with t = %d", a/AF);
                rtimer_restart(instance->timers.a, a/AF);
        }

        if (dtcp) {
                if (dtcp_sv_update(dtcp, seq_num)) {
                        LOG_ERR("Failed to update dtcp sv");
                }
        }

        LOG_DBG("DTP receive ended...");
        return 0;
}

struct rtimer * dtp_sender_inactivity_timer(struct dtp * instance)
{
        return instance->timers.sender_inactivity;
}
EXPORT_SYMBOL(dtp_sender_inactivity_timer);

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
