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
#include "dtcp-utils.h"

#define DTP_INACTIVITY_TIMERS_ENABLE 0

/* This is the DT-SV part maintained by DTP */
struct dtp_sv {
        spinlock_t          lock;

        /* Configuration values */
        struct connection * connection; /* FIXME: Are we really sure ??? */

        uint_t              seq_number_rollover_threshold;
        uint_t              dropped_pdus;
        seq_num_t           max_seq_nr_rcv;
        seq_num_t           last_seq_nr_sent;

        bool                window_based;
        bool                rexmsn_ctrl;
        bool                rate_based;
        timeout_t           a;
};

/* FIXME: Has to be rearranged */
struct dtp_policies {
        int (* transmission_control)(struct dtp * instance,
                                     struct pdu * pdu);
        int (* closed_window)(struct dtp * instance,
                              struct pdu * pdu);
        int (* flow_control_overrun)(struct dtp * instance,
                                     struct pdu * pdu);
        int (* initial_sequence_number)(struct dtp * instance);
        int (* receiver_inactivity_timer)(struct dtp * instance);
        int (* sender_inactivity_timer)(struct dtp * instance);
};

struct dtp {
        struct dt *               parent;
        /*
         * NOTE: The DTP State Vector is discarded only after and explicit
         *       release by the AP or by the system (if the AP crashes).
         */
        struct dtp_sv *           sv; /* The state-vector */

        struct dtp_policies *     policies;
        struct rmt *              rmt;
        struct kfa *              kfa;
        struct squeue *           seqq;
        struct workqueue_struct * twq;
        struct workqueue_struct * rcv_wq;
        struct {
                struct rtimer * sender_inactivity;
                struct rtimer * receiver_inactivity;
                struct rtimer * a;
        } timers;
};

static struct dtp_sv default_sv = {
        .connection                    = NULL,
        .last_seq_nr_sent              = 0,
        .seq_number_rollover_threshold = 0,
        .dropped_pdus                  = 0,
        .max_seq_nr_rcv                = 0,
        .rexmsn_ctrl                   = false,
        .rate_based                    = false,
        .window_based                  = false,
        .a                             = 0,
};

static void nxt_seq_reset(struct dtp_sv * sv, seq_num_t sn)
{
        ASSERT(sv);

        spin_lock(&sv->lock);
        sv->last_seq_nr_sent = sn;
        spin_unlock(&sv->lock);

        return;
}

static seq_num_t nxt_seq_get(struct dtp_sv * sv)
{
        seq_num_t tmp;

        ASSERT(sv);

        spin_lock(&sv->lock);
        tmp = ++sv->last_seq_nr_sent;
        spin_unlock(&sv->lock);

        return tmp;
}

seq_num_t dtp_sv_last_seq_nr_sent(struct dtp * instance)
{
        seq_num_t       tmp;
        struct dtp_sv * sv;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }
        sv = instance->sv;
        ASSERT(sv);

        spin_lock(&sv->lock);
        tmp = sv->last_seq_nr_sent;
        spin_unlock(&sv->lock);

        return tmp;
}

static uint_t dropped_pdus(struct dtp_sv * sv)
{
        uint_t tmp;

        ASSERT(sv);

        spin_lock(&sv->lock);
        tmp = sv->dropped_pdus;
        spin_unlock(&sv->lock);

        return tmp;
}

static void dropped_pdus_inc(struct dtp_sv * sv)
{
        ASSERT(sv);

        spin_lock(&sv->lock);
        sv->dropped_pdus++;
        spin_unlock(&sv->lock);
}

static int default_flow_control_overrun(struct dtp * dtp, struct pdu * pdu)
{
        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        /* FIXME: How to block further write API calls? */

        LOG_MISSING;

        LOG_DBG("Default Flow Control");

#if 0
        /* FIXME: Re-enable or remove depending on the missing code */
        if (!pdu_is_ok(pdu)) {
                LOG_ERR("PDU is not ok, cannot run policy");
                return -1;
        }
#endif
        pdu_destroy(pdu);

        return 0;
}

static int default_closed_window(struct dtp * dtp, struct pdu * pdu)
{
        struct cwq * cwq;
        struct dt *  dt;
        uint_t       max_len;

        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }
        if (!pdu_is_ok(pdu)) {
                LOG_ERR("PDU is not ok, cannot run policy");
                return -1;
        }

        dt = dtp->parent;
        ASSERT(dt);

        cwq = dt_cwq(dt);
        if (!cwq) {
                LOG_ERR("Failed to get cwq");
                pdu_destroy(pdu);
                return -1;
        }

        LOG_DBG("Closed Window Queue");

        ASSERT(dtp);

        ASSERT(dtp->sv);
        ASSERT(dtp->sv->connection);
        ASSERT(dtp->sv->connection->policies_params);

        max_len = dtcp_max_closed_winq_length(dtp->
                                              sv->
                                              connection->
                                              policies_params->
                                              dtcp_cfg);
        if (cwq_size(cwq) < max_len - 1) {
                if (cwq_push(cwq, pdu)) {
                        LOG_ERR("Failed to push into cwq");
                        return -1;
                }

                return 0;
        }

        ASSERT(dtp->policies);
        ASSERT(dtp->policies->flow_control_overrun);

        if (dtp->policies->flow_control_overrun(dtp, pdu)) {
                LOG_ERR("Failed Flow Control Overrun");
                return -1;
        }

        return 0;
}

static int default_transmission(struct dtp * dtp, struct pdu * pdu)
{

        struct dt  *  dt;
        struct dtcp * dtcp;

        /*  VARIABLES FOR SYSTEM TIME DBG MESSAGE BELOW */
        struct timeval te;
        long long milliseconds;

        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        dt = dtp->parent;
        if (!dt) {
                LOG_ERR("Passed instance has no parent, cannot run policy");
                return -1;
        }

        dtcp = dt_dtcp(dt);

#if DTP_INACTIVITY_TIMERS_ENABLE
        /* Start SenderInactivityTimer */
        if (dtcp &&
            rtimer_restart(dtp->timers.sender_inactivity,
                           3 * (dt_sv_mpl(dt) + dt_sv_r(dt) + dt_sv_a(dt)))) {
                LOG_ERR("Failed to start sender_inactiviy timer");
                return 0;
        }
#endif
        /* Post SDU to RMT */
        LOG_DBG("defaultTxPolicy - sending to rmt");
        if (dtcp_snd_lf_win_set(dtcp,
                                pci_sequence_number_get(pdu_pci_get_ro(pdu))))
                LOG_ERR("Problems setting sender left window edge "
                        "in default_transmission");

        /*  SYSTEM TIME DBG_MESSAGE */
       do_gettimeofday(&te);
       milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
       LOG_DBG("DTP (default tx) Sent PDU %d of type %d at %lld",
                pci_sequence_number_get(pdu_pci_get_ro(pdu)),
                pci_type(pdu_pci_get_ro(pdu)),
                milliseconds);

        return rmt_send(dtp->rmt,
                        pci_destination(pdu_pci_get_ro(pdu)),
                        pci_qos_id(pdu_pci_get_ro(pdu)),
                        pdu);
}

static int default_initial_seq_number(struct dtp * dtp)
{
        seq_num_t seq_num;

        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        get_random_bytes(&seq_num, sizeof(seq_num_t));
        nxt_seq_reset(dtp->sv, seq_num);

        LOG_DBG("initial_seq_number reset");
        return seq_num;
}

static int default_receiver_inactivity(struct dtp * dtp)
{
        struct dt *          dt;
        struct dtcp *        dtcp;
        struct dtcp_config * cfg;

        LOG_DBG("default_receiver_inactivity launched");

        if (!dtp) return 0;

        dt = dtp->parent;
        if (!dt)
                return -1;

        dtcp = dt_dtcp(dt);
        if (!dtcp)
                return -1;

        dt_sv_drf_flag_set(dt, true);
        dtp_initial_sequence_number(dtp);

        cfg = dtcp_config_get(dtcp);
        if (!cfg)
                return -1;

        if (dtcp_rtx_ctrl(cfg)) {
                struct rtxq * q;

                q = dt_rtxq(dt);
                if (!q) {
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }
                rtxq_flush(q);
        }
        if (dtcp_flow_ctrl(cfg)) {
                struct cwq * cwq;

                cwq = dt_cwq(dt);
                ASSERT(cwq);
                if (cwq_flush(cwq)) {
                        LOG_ERR("Coudln't flush cwq");
                        return -1;
                }
        }

        /*FIXME: Missing sending the control ack pdu */
        return 0;
}

static int default_sender_inactivity(struct dtp * dtp)
{
        struct dt *          dt;
        struct dtcp *        dtcp;
        struct dtcp_config * cfg;

        LOG_DBG("default_sender_inactivity launched");

        if (!dtp) return 0;

        dt = dtp->parent;
        if (!dt)
                return -1;

        dtcp = dt_dtcp(dt);
        if (!dtp)
                return -1;

        dt_sv_drf_flag_set(dt, true);
        dtp_initial_sequence_number(dtp);

        cfg = dtcp_config_get(dtcp);
        if (!cfg)
                return -1;

        if (dtcp_rtx_ctrl(cfg)) {
                struct rtxq * q;

                q = dt_rtxq(dt);
                if (!q) {
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }
                rtxq_flush(q);
        }
        if (dtcp_flow_ctrl(cfg)) {
                struct cwq * cwq;

                cwq = dt_cwq(dt);
                ASSERT(cwq);
                if (cwq_flush(cwq)) {
                        LOG_ERR("Coudln't flush cwq");
                        return -1;
                }
        }

        /*FIXME: Missing sending the control ack pdu */
        return 0;
}

static struct dtp_policies default_policies = {
        .transmission_control      = default_transmission,
        .closed_window             = default_closed_window,
        .flow_control_overrun      = default_flow_control_overrun,
        .initial_sequence_number   = default_initial_seq_number,
        .receiver_inactivity_timer = default_receiver_inactivity,
        .sender_inactivity_timer   = default_sender_inactivity,
};

int dtp_initial_sequence_number(struct dtp * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        ASSERT(instance->policies);
        ASSERT(instance->policies->initial_sequence_number);

        if (instance->policies->initial_sequence_number(instance))
                return -1;

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

static struct pdu * seq_queue_pop(struct seq_queue * q)
{
        struct seq_queue_entry * p;
        struct pdu *             pdu;

        if (list_empty(&q->head)) {
                LOG_WARN("Seq Queue is empty!");
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
                LOG_DBG("First PDU with seqnum: %d push to seqq at: %pk",
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
                LOG_DBG("Last PDU with seqnum: %d push to seqq at: %pk",
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
                        LOG_DBG("Middle PDU with seqnum: %d push to "
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

        ASSERT(instance->sv->connection);

        if (kfa_sdu_post(instance->kfa,
                         instance->sv->connection->port_id,
                         sdu)) {
                LOG_ERR("Could not post SDU to KFA");
                pdu_destroy(pdu);
                return -1;
        }

        pdu_destroy(pdu);

        return 0;
}

/* Runs the SenderInactivityTimerPolicy */
static void tf_sender_inactivity(void * data)
{
        struct dtp * dtp;

        dtp = (struct dtp *) data;
        if (!dtp) {
                LOG_ERR("No dtp to work with");
                return;
        }
        if (!dtp->policies) {
                LOG_ERR("No DTP policies");
                return;
        }
        if (!dtp->policies->sender_inactivity_timer) {
                LOG_ERR("No DTP sender inactivity policy");
                return;
        }

        if (dtp->policies->sender_inactivity_timer(dtp))
                LOG_ERR("Problems executing the sender inactivity policy");

        return;
}

/* Runs the ReceiverInactivityTimerPolicy */
static void tf_receiver_inactivity(void * data)
{
        struct dtp * dtp;

        dtp = (struct dtp *) data;
        if (!dtp) {
                LOG_ERR("No dtp to work with");
                return;
        }
        if (!dtp->policies) {
                LOG_ERR("No DTP policies");
                return;
        }
#if DTP_INACTIVITY_TIMERS_ENABLE
        if (!dtp->policies->receiver_inactivity_timer) {
                LOG_ERR("No DTP sender inactivity policy");
                return;
        }

        if (dtp->policies->receiver_inactivity_timer(dtp))
                LOG_ERR("Problems executing receiver inactivity policy");
#endif
        return;
}

/*
 * NOTE:
 *   AF is the factor to which A is divided in order to obtain the
 *   period of the A-timer: Ta = A / AF
 */
#define AF 1

static seq_num_t process_A_expiration(struct dtp * dtp, struct dtcp * dtcp)
{
        struct dt *              dt;
        struct dtp_sv *          sv;
        struct squeue *          seqq;
        seq_num_t                LWE;
        seq_num_t                seq_num;
        struct pdu *             pdu;
        bool                     in_order_del;
        bool                     incomplete_del;
        seq_num_t                max_sdu_gap;
        timeout_t                a;
        struct seq_queue_entry * pos, * n;
        struct rqueue *          to_post;
        seq_num_t                ret;

        ASSERT(dtp);

        sv = dtp->sv;
        ASSERT(sv);

        dt = dtp->parent;
        ASSERT(dt);

        seqq = dtp->seqq;
        ASSERT(seqq);

        /* dtcp = dt_dtcp(dtp->parent); */

        a              = dt_sv_a(dt);

        ASSERT(sv->connection);
        ASSERT(sv->connection->policies_params);

        in_order_del   = sv->connection->policies_params->in_order_delivery;
        incomplete_del = sv->connection->policies_params->incomplete_delivery;

        max_sdu_gap    = sv->connection->policies_params->max_sdu_gap;

        /* FIXME: Invoke delimiting */

        LOG_DBG("Processing A timer expiration");

        to_post = rqueue_create();
        if (!to_post) {
                LOG_ERR("Could not create to_post list in A timer");
                return -1;
        }

        spin_lock(&seqq->lock);
        LWE = dt_sv_rcv_lft_win(dt);
        ret = LWE;
        LOG_DBG("LWEU: Original LWE = %u", LWE);
        LOG_DBG("LWEU: MAX GAPS     = %u", max_sdu_gap);

        list_for_each_entry_safe(pos, n, &seqq->queue->head, next) {

                pdu = pos->pdu;
                if (!pdu_is_ok(pdu)) {
                        spin_unlock(&seqq->lock);

                        LOG_ERR("Bogus data, bailing out");
                        return LWE;
                }

                seq_num = pci_sequence_number_get(pdu_pci_get_ro(pdu));
                LOG_DBG("Seq number: %u", seq_num);

                if (seq_num - LWE - 1 <= max_sdu_gap) {

                        if (dt_sv_rcv_lft_win_set(dt, seq_num)) {
                                LOG_ERR("Could not update LWE while A timer");
                        }
                        pos->pdu = NULL;
                        list_del(&pos->next);
                        seq_queue_entry_destroy(pos);

                        /*
                        spin_unlock(&seqq->lock);
                        if (pdu_post(dtp, pdu)) {
                                LOG_ERR("Could not post PDU %u while A timer"
                                        "(in-order)", seq_num);
                                return -1;
                        }

                        LOG_DBG("Atimer: PDU %u posted", seq_num);

                        spin_lock(&seqq->lock);
                        */

                        rqueue_tail_push_ni(to_post, pdu);

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
                                ret = -1;
                                goto finish;
                        }
                        pos->pdu = NULL;
                        list_del(&pos->next);
                        seq_queue_entry_destroy(pos);
                        /*
                        spin_unlock(&seqq->lock);
                        if (pdu_post(dtp, pdu)) {
                                LOG_ERR("Could not post PDU %u while A timer"
                                        "(expiration)", seq_num);
                                return -1;
                        }

                        spin_lock(&seqq->lock);
                        */
                        rqueue_tail_push_ni(to_post, pdu);

                        LWE = dt_sv_rcv_lft_win(dt);
                        ret = LWE;

                        continue;
                }

                break;

        }
finish:
        spin_unlock(&seqq->lock);

        while (!rqueue_is_empty(to_post)) {
                pdu = (struct pdu *) rqueue_head_pop(to_post);
                if (pdu)
                        pdu_post(dtp, pdu);
        }
        rqueue_destroy(to_post, (void (*)(void *)) pdu_destroy);

        return ret;
}

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

static int post_worker(void * o)
{
        struct dtp *  dtp;
        struct dtcp * dtcp;
        seq_num_t     seq_num_sv_update;
        timeout_t     a;

        LOG_DBG("TWQ Post worker called");

        dtp = (struct dtp *) o;
        if (!dtp) {
                LOG_ERR("No instance passed to post worker !!!");
                return -1;
        }

        dtcp = dt_dtcp(dtp->parent);

        /* Invoke delimiting and update left window edge */

        a = dt_sv_a(dtp->parent);
        seq_num_sv_update =  process_A_expiration(dtp, dtcp);
        if (dtcp) {
                if ((int) seq_num_sv_update < 0) {
                        LOG_ERR("ULWE returned no seq num to update");
                        rtimer_start(dtp->timers.a, a/AF);
                        return -1;
                }

                if (dtcp_sv_update(dtcp, seq_num_sv_update)) {
                        LOG_ERR("Failed to update dtcp sv");
                        rtimer_start(dtp->timers.a, a/AF);
                        return -1;
                }
        }

        if (!seqq_is_empty(dtp->seqq)) {
                LOG_DBG("Going to restart A timer with a = %d and a/AF = %d",
                        a, a/AF);
                rtimer_start(dtp->timers.a, a/AF);
        }
        LOG_DBG("Finished post worker for dtp: %pK", dtp);

        return 0;
}

static void tf_a(void * data)
{
        struct dtp *           dtp;
        struct rwq_work_item * item;

        dtp = (struct dtp *) data;
        if (!dtp) {
                LOG_ERR("No dtp to work with");
                return;
        }

        item = rwq_work_create_ni(post_worker, dtp);
        if (!item) {
                LOG_ERR("Could not create twq item");
                return;
        }

        if (rwq_work_post(dtp->twq, item)) {
                LOG_ERR("Could not add twq item to the wq");
                return;
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

        return 0;
}

#define MAX_NAME_SIZE 128

/* FIXME: This function is not re-entrant */
static const char * twq_name_format(const char *       prefix,
                                    const struct dtp * instance)
{
        static char name[MAX_NAME_SIZE];

        ASSERT(prefix);
        ASSERT(instance);

        if (snprintf(name, sizeof(name), RINA_PREFIX "-%s-%pK",
                     prefix, instance) >=
            sizeof(name))
                return NULL;

        return name;
}

struct dtp * dtp_create(struct dt *         dt,
                        struct rmt *        rmt,
                        struct kfa *        kfa,
                        struct connection * connection)
{
        struct dtp * tmp;
        const char * twq_name;
        const char * rwq_name;

        if (!dt) {
                LOG_ERR("No DT passed, bailing out");
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

        tmp->policies       = &default_policies;
        /* FIXME: fixups to the policies should be placed here */

        tmp->rmt            = rmt;
        tmp->kfa            = kfa;
        tmp->seqq           = squeue_create(tmp);
        if (!tmp->seqq) {
                LOG_ERR("Could not create Sequencing queue");
                dtp_destroy(tmp);
                return NULL;
        }

        twq_name = twq_name_format("twq", tmp);
        if (!twq_name) {
                dtp_destroy(tmp);
                return NULL;
        }
        tmp->twq = rwq_create(twq_name);
        if (!tmp->twq) {
                dtp_destroy(tmp);
                return NULL;
        }
	/* FIXME: This function must change */
        rwq_name = twq_name_format("rwq", tmp);
        if (!rwq_name) {
                dtp_destroy(tmp);
                return NULL;
        }
        tmp->rcv_wq = rwq_create(rwq_name);
        if (!tmp->rcv_wq) {
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

        LOG_DBG("Instance %pK created successfully", tmp);

        return tmp;
}

int dtp_destroy(struct dtp * instance)
{
        if (!instance) {
                LOG_ERR("Bad instance passed, bailing out");
                return -1;
        }

        if (instance->timers.sender_inactivity)
                rtimer_destroy(instance->timers.sender_inactivity);
        if (instance->timers.receiver_inactivity)
                rtimer_destroy(instance->timers.receiver_inactivity);
        if (instance->timers.a)
                rtimer_destroy(instance->timers.a);

        if (instance->twq)    rwq_destroy(instance->twq);
        if (instance->rcv_wq) rwq_destroy(instance->rcv_wq);
        if (instance->seqq)   squeue_destroy(instance->seqq);
        if (instance->sv)     rkfree(instance->sv);
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

        if (sv->window_based && seq_num >= dtcp_snd_rt_win(dtcp)) {
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
        struct pdu *          pdu;
        struct pci *          pci;
        struct dtp_sv *       sv;
        struct dt *           dt;
        struct dtcp *         dtcp;
        struct rtxq *         rtxq;
        struct pdu *          cpdu;
        struct dtp_policies * policies;

        /*  VARIABLES FOR SYSTEM TIME DBG MESSAGE BELOW */
        struct timeval te;
        long long milliseconds;

        if (!sdu_is_ok(sdu))
                return -1;

        if (!instance) {
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

        policies = instance->policies;
        if (!policies) {
                LOG_ERR("Bogus DTP policies passed, bailing out");
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
        if (dtcp && rtimer_stop(instance->timers.sender_inactivity)) {
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

        /* FIXME: Check if we need to set DRF */

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

        if (dtcp) {
                if (sv->window_based || sv->rate_based) {
                        /* NOTE: Might close window */
                        if (window_is_closed(sv,
                                             dt,
                                             dtcp,
                                             pci_sequence_number_get(pci))) {
                                if (policies->closed_window(instance, pdu)) {
                                        LOG_ERR("Problems with the "
                                                "closed window policy");
                                        return -1;
                                }
                                return 0;
                        }
                }
                if (sv->rexmsn_ctrl) {
                        /* FIXME: Add timer for PDU */
                        rtxq = dt_rtxq(dt);
                        if (!rtxq) {
                                LOG_ERR("Failed to get rtxq");
                                pdu_destroy(pdu);
                                return -1;
                        }

                        cpdu = pdu_dup(pdu);
                        if (!cpdu) {
                                LOG_ERR("Failed to copy PDU");
                                LOG_ERR("PDU ok? %d", pdu_pci_present(pdu));
                                LOG_ERR("PDU type: %d",
                                        pci_type(pdu_pci_get_ro(pdu)));
                                pdu_destroy(pdu);
                                return -1;
                        }

                        if (rtxq_push_ni(rtxq, cpdu)) {
                                LOG_ERR("Couldn't push to rtxq");
                                pdu_destroy(pdu);
                                return -1;
                        }
                }
                if (policies->transmission_control(instance,
                                                   pdu)) {
                        LOG_ERR("Problems with transmission "
                                "control");
                        return -1;
                }

#if DTP_INACTIVITY_TIMERS_ENABLE
                /* Start SenderInactivityTimer */
                if (rtimer_restart(instance->timers.sender_inactivity,
                                   2 * (dt_sv_mpl(dt) +
                                        dt_sv_r(dt)   +
                                        dt_sv_a(dt)))) {
                        LOG_ERR("Failed to start sender_inactiviy timer");
                        return -1;
                }
#endif
                return 0;
        }

        /* Post SDU to RMT */

       /* SYSTEM TIME DBG MESSAGE */
       do_gettimeofday(&te); // get current time
       milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
       LOG_DBG("DTP Sent PDU %d of type %d at %lld",
                pci_sequence_number_get(pci),
                pci_type(pdu_pci_get_ro(pdu)),
                milliseconds);

        return rmt_send(instance->rmt,
                        pci_destination(pci),
                        pci_qos_id(pci),
                        pdu);
}

int dtp_mgmt_write(struct rmt * rmt,
                   address_t    src_address,
                   port_id_t    port_id,
                   struct sdu * sdu)
{
        struct pci * pci;
        struct pdu * pdu;
        address_t    dst_address;

        /*
         * NOTE:
         *   DTP should build the PCI header src and dst cep_ids = 0
         *   ask FT for the dst address the N-1 port is connected to
         *   pass to the rmt
         */

        if (!sdu) {
                LOG_ERR("No data passed, bailing out");
                return -1;
        }

        dst_address = 0; /* FIXME: get from PFT */

        /*
         * FIXME:
         *   We should avoid to create a PCI only to have its fields to use
         */
        pci = pci_create();
        if (!pci)
                return -1;

        if (pci_format(pci,
                       0,
                       0,
                       src_address,
                       dst_address,
                       0,
                       0,
                       PDU_TYPE_MGMT)) {
                pci_destroy(pci);
                return -1;
        }

        pdu = pdu_create();
        if (!pdu) {
                pci_destroy(pci);
                return -1;
        }

        if (pdu_buffer_set(pdu, sdu_buffer_rw(sdu))) {
                pci_destroy(pci);
                pdu_destroy(pdu);
                return -1;
        }

        if (pdu_pci_set(pdu, pci)) {
                pci_destroy(pci);
                return -1;
        }

        /* Give the data to RMT now ! */

        /* FIXME: What about sequencing (and all the other procedures) ? */
        return rmt_send(rmt,
                        pci_destination(pci),
                        pci_cep_destination(pci),
                        pdu);

}

struct rcv_item {
        struct dtp * dtp;
        struct pdu * pdu;
};

static int rcv_worker(void * o)
{
        struct dtp *          instance;
        struct pdu *          pdu;
        struct rcv_item *     ritem;
        struct dtp_policies * policies;
        struct pci *          pci;
        struct dtp_sv *       sv;
        struct dtcp *         dtcp;
        struct dt *           dt;
        seq_num_t             seq_num;
        timeout_t             a;
        timeout_t             LWE;
        bool                  in_order;
        seq_num_t             max_sdu_gap;
        struct rqueue *       to_post;

        /* VARiABLES FOR SYSTEM TIMESTAMP DBG MESSAGE BELOW*/
        struct timeval te;
        long long milliseconds;

        ritem = (struct rcv_item *) o;
        if (!ritem) {
                LOG_ERR("Bogus rcv_item...");
                return -1;
        }

        pdu = ritem->pdu;
        if (!pdu_is_ok(pdu)) {
                LOG_ERR("Receive_item contained bogus pdu");
                rkfree(ritem);
                return -1;
        }

        instance = ritem->dtp;
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                rkfree(ritem);
                pdu_destroy(pdu);
                return -1;
        }

        rkfree(ritem);

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("Bogus data, bailing out");
                return -1;
        }

        if (!instance                  ||
            !instance->kfa             ||
            !instance->sv              ||
            !instance->sv->connection) {
                LOG_ERR("Bogus instance passed, bailing out");
                pdu_destroy(pdu);
                return -1;
        }

        policies = instance->policies;
        ASSERT(policies);

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
        in_order    = sv->connection->policies_params->in_order_delivery;
        max_sdu_gap = sv->connection->policies_params->max_sdu_gap;
#if DTP_INACTIVITY_TIMERS_ENABLE
        /* Stop ReceiverInactivityTimer */
        if (dtcp && rtimer_stop(instance->timers.receiver_inactivity)) {
                LOG_ERR("Failed to stop timer");
                /*pdu_destroy(pdu);
                  return -1;*/
        }
#endif
        seq_num = pci_sequence_number_get(pci);

        /* SYSTEM TIMESTAMP DBG MESSAGE */
        do_gettimeofday(&te); // get current time
        milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
        LOG_DBG("DTP Received PDU %d at %lld", seq_num, milliseconds);

        if (!(pci_flags_get(pci) ^ PDU_FLAGS_DATA_RUN)) {
                LOG_DBG("Data run flag DRF");
                /* This is wrong after discussions with John */
                /* dt_sv_drf_flag_set(dt, true); */
                policies->initial_sequence_number(instance);
                if (dtcp) {
                        if (dtcp_sv_update(dtcp, seq_num)) {
                                LOG_ERR("Failed to update dtcp sv");
                                pdu_destroy(pdu);
                                return -1;
                        }
                }

                return 0;
        }

        /*
         * NOTE:
         *   no need to check presence of in_order or dtcp because in case
         *   they are not, LWE is not updated and always 0
         */
        if (seq_num <= LWE) {
                pdu_destroy(pdu);

                dropped_pdus_inc(sv);
                LOG_ERR("PDU SeqN %u, LWE: %u. Dropped PDUs: %d",
                        seq_num, LWE, dropped_pdus(sv));

                /* Send an ACK/Flow Control PDU with current window values */
                if (dtcp) {
                        if (dtcp_ack_flow_control_pdu_send(dtcp)) {
                                LOG_ERR("Failed to send ack / flow "
                                        "control pdu");
                                return -1;
                        }
#if DTP_INACTIVITY_TIMERS_ENABLE
                        /* Start ReceiverInactivityTimer */
                        if (rtimer_restart(instance->
                                           timers.receiver_inactivity,
                                           3 * (dt_sv_mpl(dt) +
                                                dt_sv_r(dt)   +
                                                dt_sv_a(dt))))
                                LOG_ERR("Failed restart RcvrInactivity timer");
#endif
                }
                return 0;
        }

        if (!a) {
                bool set_lft_win_edge;

                /* FIXME: delimiting goes here */
                if (!in_order && !dtcp) {
                        LOG_DBG("DTP Receive deliver, seq_num: %d, LWE: %d",
                                seq_num, LWE);
                        if (pdu_post(instance, pdu))
                                return -1;

                        goto exit;
                }

                set_lft_win_edge = !(dtcp                                 &&
                                     dtcp_rtx_ctrl(dtcp_config_get(dtcp)) &&
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
                                goto exit;
                        }
                }

                if (pdu_post(instance, pdu))
                        return -1;

                goto exit;

        fail:
                pdu_destroy(pdu);
                return -1;
        }

        to_post = rqueue_create();
        if (!to_post) {
                LOG_ERR("Could not create to_post list at reception");
                pdu_destroy(pdu);
                return -1;
        }
        spin_lock(&instance->seqq->lock);
        LWE = dt_sv_rcv_lft_win(dt);
        while (pdu && (seq_num == LWE + 1)) {
                dt_sv_rcv_lft_win_set(dt, seq_num);

               /*
                spin_unlock(&instance->seqq->lock);
                pdu_post(instance, pdu);
                spin_lock(&instance->seqq->lock); */

                rqueue_tail_push_ni(to_post, pdu);

                pdu     = seq_queue_pop(instance->seqq->queue);
                seq_num = pci_sequence_number_get(pdu_pci_get_rw(pdu));
                LWE     = dt_sv_rcv_lft_win(dt);
        }
        if (pdu)
                seq_queue_push_ni(instance->seqq->queue, pdu);
        spin_unlock(&instance->seqq->lock);

        if (a) {
                LOG_DBG("Going to start A timer with t = %d", a/AF);
                rtimer_start(instance->timers.a, a/AF);
        }

        while (!rqueue_is_empty(to_post)) {
                pdu = (struct pdu *) rqueue_head_pop(to_post);
                if (pdu)
                        pdu_post(instance, pdu);
        }
        rqueue_destroy(to_post, (void (*)(void *)) pdu_destroy);

 exit:
#if DTP_INACTIVITY_TIMERS_ENABLE
        /* Start ReceiverInactivityTimer */
        if (dtcp && rtimer_restart(instance->timers.receiver_inactivity,
                                   2 * (dt_sv_mpl(dt) +
                                        dt_sv_r(dt)   +
                                        dt_sv_a(dt)))) {
                LOG_ERR("Failed to start Receiver Inactivity timer");
                return -1;
        }
#endif
        return 0;
}

int dtp_receive(struct dtp * instance,
                struct pdu * pdu)
{
        struct rwq_work_item * item;
        struct rcv_item *      ritem;

        if (!pdu_is_ok(pdu)) {
                pdu_destroy(pdu);
                return -1;
        }

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                pdu_destroy(pdu);
                return -1;
        }

        ritem = rkzalloc(sizeof(*ritem), GFP_KERNEL);
        if (!ritem) {
                pdu_destroy(pdu);
                LOG_ERR("Could not create receive item");
                return -1;
        }

        ritem->dtp = instance;
        ritem->pdu = pdu;

        item = rwq_work_create_ni(rcv_worker, ritem);
        if (!item) {
                LOG_ERR("Could not create wwq item");
                pdu_destroy(pdu);
                rkfree(ritem);
                return -1;
        }

        if (rwq_work_post(instance->rcv_wq, item)) {
                LOG_ERR("Could not add rcv wq item to the wq");
                pdu_destroy(pdu);
                return -1;
        }

        return 0;
}
