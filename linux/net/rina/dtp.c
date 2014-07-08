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

#define RINA_PREFIX "dtp"
/* FIXME remove these defines */
#define TEMP_MAX_CWQ_LEN 100

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtp.h"
#include "dt.h"
#include "dt-utils.h"
#include "dtcp.h"
#include "dtcp-utils.h"

/* Sequencing/reassembly queue */

struct seq_queue {
        struct list_head head;
};

struct seq_q_entry {
        unsigned long    time_stamp;
        struct pdu *     pdu;
        struct list_head next;
};

struct sequencingQ {
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

static void seq_q_entry_destroy(struct seq_q_entry * seq_entry)
{
        ASSERT(seq_entry);

        if (seq_entry->pdu) pdu_destroy(seq_entry->pdu);
        rkfree(seq_entry);

        return;
}

static int seq_queue_destroy(struct seq_queue * seq_queue)
{
        struct seq_q_entry * cur, * n;

        ASSERT(seq_queue);

        list_for_each_entry_safe(cur, n, &seq_queue->head, next) {
                list_del(&cur->next);
                seq_q_entry_destroy(cur);
        }

        rkfree(seq_queue);

        return 0;
}

int seqQ_destroy(struct sequencingQ * seqQ)
{
        if (!seqQ)
                return -1;

        if (seqQ->queue) seq_queue_destroy(seqQ->queue);

        rkfree(seqQ);

        return 0;
}

struct sequencingQ * seqQ_create(struct dtp * dtp)
{
        struct sequencingQ * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->queue = seq_queue_create();
        if (!tmp->queue) {
                seqQ_destroy(tmp);
                return NULL;
        }

        tmp->dtp = dtp;

        spin_lock_init(&tmp->lock);

        return tmp;
}

/* This is the DT-SV part maintained by DTP */
struct dtp_sv {
        spinlock_t lock;

        /* Configuration values */
        struct connection * connection; /* FIXME: Are we really sure ??? */

        uint_t              seq_number_rollover_threshold;
        uint_t              dropped_pdus;
        seq_num_t           max_seq_nr_rcv;
        seq_num_t           nxt_seq;
        bool                drf_flag;

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
};

struct dtp {
        struct dt *           parent;
        /*
         * NOTE: The DTP State Vector is discarded only after and explicit
         *       release by the AP or by the system (if the AP crashes).
         */
        struct dtp_sv *           sv; /* The state-vector */

        struct dtp_policies *     policies;
        struct rmt *              rmt;
        struct kfa *              kfa;
        struct sequencingQ *      seqQ;
        struct workqueue_struct * twq;
        struct {
#if 0
                struct rtimer * sender_inactivity;
                struct rtimer * receiver_inactivity;
#endif
                struct rtimer * a;
        } timers;
};

static struct dtp_sv default_sv = {
        .connection                    = NULL,
        .nxt_seq                       = 0,
        .seq_number_rollover_threshold = 0,
        .dropped_pdus                  = 0,
        .max_seq_nr_rcv                = 0,
        .drf_flag                      = false,
        .rexmsn_ctrl                   = false,
        .rate_based                    = false,
        .window_based                  = false,
        .a                             = 0,
};

static int default_flow_control_overrun(struct dtp * dtp, struct pdu * pdu)
{
        /* FIXME: How to block further write API calls? */
        LOG_MISSING;
        LOG_DBG("Default Flow Control");

        pdu_destroy(pdu);

        return 0;
}

static int default_closed_window(struct dtp * dtp, struct pdu * pdu)
{
        struct cwq *    cwq;
        struct dt *     dt;
        uint_t          max_len;

        ASSERT(dtp);
        ASSERT(pdu_is_ok(pdu));

        dt = dtp->parent;
        ASSERT(dt);

        cwq = dt_cwq(dt);
        if (!cwq) {
                LOG_ERR("Failed to get cwq");
                pdu_destroy(pdu);
                return -1;
        }

        LOG_DBG("Closed Window Queue");

        /* FIXME: Please add ASSERTs here ... */
        max_len = dtcp_max_closed_winq_length(dtp->
                                              sv->
                                              connection->
                                              policies_params->
                                              dtcp_cfg);
        if (cwq_size(cwq) < max_len -1) {
                if (cwq_push(cwq, pdu)) {
                        LOG_ERR("Failed to push to cwq");
                        pdu_destroy(pdu);
                        return -1;
                }

                return 0;
        }

        if (dtp->policies->flow_control_overrun(dtp, pdu)) {
                LOG_ERR("Failed Flow Control Overrun");
                return -1;
        }

        return 0;
}

static int default_transmission(struct dtp * dtp, struct pdu * pdu)
{

        struct dt * dt;
        ASSERT(dtp);
        ASSERT(pdu_is_ok(pdu));

        dt = dtp->parent;
        ASSERT(dt);
#if 0
        /* Start SenderInactivityTimer */
        if (rtimer_restart(dtp->timers.sender_inactivity,
                           2 * (dt_sv_mpl(dt) + dt_sv_r(dt) + dt_sv_a(dt)))) {
                LOG_ERR("Failed to start sender_inactiviy timer");
                return -1;
        }
#endif
        /* Post SDU to RMT */
        LOG_DBG("defaultTxPolicy - sending to rmt");
        return rmt_send(dtp->rmt,
                        pci_destination(pdu_pci_get_ro(pdu)),
                        pci_qos_id(pdu_pci_get_ro(pdu)),
                        pdu);
}

static int default_initial_seq_number(struct dtp * dtp)
{
        return 0;
}

static struct dtp_policies default_policies = {
        .transmission_control      = default_transmission,
        .closed_window             = default_closed_window,
        .flow_control_overrun      = default_flow_control_overrun,
        .initial_sequence_number   = default_initial_seq_number,
};

bool dtp_drf_flag(struct dtp * instance)
{
        bool flag;

        if (!instance || !instance->sv)
                return false;

        spin_lock(&instance->sv->lock);
        flag = instance->sv->drf_flag;
        spin_unlock(&instance->sv->lock);

        return flag;
}

static void drf_flag_set(struct dtp_sv * sv, bool value)
{
        ASSERT(sv);

        spin_lock(&sv->lock);
        sv->drf_flag = value;
        spin_unlock(&sv->lock);
}

static seq_num_t nxt_seq_get(struct dtp_sv * sv)
{
        seq_num_t tmp;

        ASSERT(sv);

        spin_lock(&sv->lock);
        tmp = ++sv->nxt_seq;
        spin_unlock(&sv->lock);

        return tmp;
}

#ifdef CONFIG_RINA_RELIABLE_FLOW_SUPPORT
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
        ASSERT(sv);

        spin_lock(&sv->lock);
        sv->dropped_pdus++;
        spin_unlock(&sv->lock);
}
#endif

#ifdef CONFIG_RINA_RELIABLE_FLOW_SUPPORT
#if 0
static seq_num_t max_seq_nr_rcv(struct dtp_sv * sv)
{
        seq_num_t tmp;

        ASSERT(sv);

        spin_lock(&sv->lock);
        tmp = sv->max_seq_nr_rcv;
        spin_unlock(&sv->lock);

        return tmp;
}

static void max_seq_nr_rcv_set(struct dtp_sv * sv, seq_num_t nr)
{
        ASSERT(sv);

        spin_lock(&sv->lock);
        sv->max_seq_nr_rcv = nr;
        spin_unlock(&sv->lock);
}
#endif
#endif

static int pdu_post(struct dtp * instance,
                    struct pdu * pdu)
{
        struct sdu *          sdu;
        struct buffer *       buffer;

        ASSERT(instance->sv);

        buffer = pdu_buffer_get_rw(pdu);
        sdu    = sdu_create_buffer_with_ni(buffer);
        if (!sdu) {
                pdu_destroy(pdu);
                return -1;
        }

        pdu_buffer_disown(pdu);

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

static struct pdu * seq_queue_pop(struct seq_queue * q)
{
        struct seq_q_entry * p;
        struct pdu * pdu;

        if (list_empty(&q->head)) {
                LOG_WARN("Seq Queue is empty!");
                return NULL;
        }

        p = list_first_entry(&q->head, struct seq_q_entry, next);
        if (!p)
                return NULL;

        pdu = p->pdu;
        p->pdu = NULL;

        list_del(&p->next);
        seq_q_entry_destroy(p);

        return pdu;
}

static bool seq_queue_is_empty(struct seq_queue * q)
{
        ASSERT(q);
        return list_empty(&q->head);
}

struct pdu * seqQ_pop(struct sequencingQ * seqQ)
{
        struct pdu * pdu;

        ASSERT(seqQ);

        spin_lock(&seqQ->lock);
        pdu = seq_queue_pop(seqQ->queue);
        if (!pdu) {
                spin_unlock(&seqQ->lock);
                LOG_ERR("Cannot pop PDU from sequencing queue %pK", seqQ);
                return NULL;
        }
        spin_unlock(&seqQ->lock);

        return pdu;
}

static struct seq_q_entry * seq_q_entry_create_gfp(struct pdu * pdu,
                                                   gfp_t        flags)
{
        struct seq_q_entry * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->next);

        tmp->pdu = pdu;
        tmp->time_stamp = jiffies;

        return tmp;
}

static int seq_queue_push_ni(struct seq_queue * q, struct pdu * pdu)
{
        static struct seq_q_entry * tmp, * cur, * last = NULL;
        seq_num_t                   csn, psn;
        const struct pci *          pci;

        ASSERT(pdu);
        ASSERT(q);

        tmp = seq_q_entry_create_gfp(pdu, GFP_ATOMIC);
        if (!tmp) {
                LOG_ERR("Could not create sequencing queue entry");
                return -1;
        }

        pci  = pdu_pci_get_ro(pdu);
        csn  = pci_sequence_number_get((struct pci *) pci);

        if (list_empty(&q->head)) {
                list_add(&tmp->next, &q->head);
                LOG_DBG("First PDU with seqnum: %d push to seqQ at: %pk",
                        csn, q);
                return 0;
        }

        last = list_last_entry(&q->head, struct seq_q_entry, next);
        if (!last) {
                LOG_ERR("Could not retrieve last pdu from seqQ");
                seq_q_entry_destroy(tmp);
                return 0;
        }

        pci  = pdu_pci_get_ro(last->pdu);
        psn  = pci_sequence_number_get((struct pci *) pci);
        if (csn == psn) {
                LOG_ERR("Another PDU with the same seq_num is in "
                        "the rtx queue!");
                seq_q_entry_destroy(tmp);
                return -1;
        }
        if (csn > psn) {
                list_add_tail(&tmp->next, &q->head);
                LOG_DBG("Last PDU with seqnum: %d push to seqQ at: %pk",
                        csn, q);
                return 0;
        }

        list_for_each_entry(cur, &q->head, next) {
                pci = pdu_pci_get_ro(cur->pdu);
                psn = pci_sequence_number_get((struct pci *) pci);
                if (csn == psn) {
                        LOG_ERR("Another PDU with the same seq_num is in "
                                "the rtx queue!");
                        seq_q_entry_destroy(tmp);
                        return 0;
                }
                if (csn > psn) {
                        list_add(&tmp->next, &cur->next);
                        LOG_DBG("Middle PDU with seqnum: %d push to "
                                "seqQ at: %pk", csn, q);
                        return 0;
                }
        }

        return 0;
}

int seqQ_deliver(struct sequencingQ * seqQ)
{
        LOG_MISSING;
        return 0;
}

int seqQ_push(struct dtp * dtp, struct pdu * pdu)
{
        struct sequencingQ * seqQ;
        struct dt *          dt;

        ASSERT(dtp);
        ASSERT(pdu);

        seqQ = dtp->seqQ;
        ASSERT(seqQ);

        dt = dtp->parent;
        ASSERT(dt);

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("No PDU to be pushed");
                return -1;
        }

        spin_lock(&seqQ->lock);
        if (seq_queue_push_ni(seqQ->queue, pdu)) {
                spin_unlock(&seqQ->lock);
                dropped_pdus_inc(dtp->sv);
                LOG_ERR("Cannot push PDU into sequencing queue %pK", seqQ);
                return -1;
        }
        spin_unlock(&seqQ->lock);

        return 0;
}

bool seqQ_is_empty(struct sequencingQ * seqQ)
{
        bool empty;
        ASSERT(seqQ);

        spin_lock(&seqQ->lock);
        empty = seq_queue_is_empty(seqQ->queue);
        spin_unlock(&seqQ->lock);

        return empty;
}

static seq_num_t seq_queue_last_to_ack(struct seq_queue * q, timeout_t t)
{
        LOG_MISSING;
        return 0;
}

seq_num_t seqQ_last_to_ack(struct sequencingQ * seqQ, timeout_t t)
{
        seq_num_t tmp;

        LOG_MISSING;
        /* FIXME: change this as it should be. It should return those PDUs with
         * timestam < time - A plus those that with seq_num exactly consecutive
         * to the last one */
        spin_lock(&seqQ->lock);
        tmp = seq_queue_last_to_ack(seqQ->queue, t);
        spin_unlock(&seqQ->lock);

        return tmp;
}
#if 0
static void tf_sender_inactivity(void * data)
{ /* Runs the SenderInactivityTimerPolicy */ }

static void tf_receiver_inactivity(void * data)
{ /* Runs the ReceiverInactivityTimerPolicy */ }

static void seqQ_cleanup(struct dtp * dtp)
{
        struct sequencingQ * seqQ;
        seq_num_t            seq_num;
        seq_num_t            LWE;
        seq_num_t            max_sdu_gap;
        struct dtp_sv *      sv;
        struct dt *          dt;
        struct pdu *         pdu;

        ASSERT(dtp);

        seqQ = dtp->seqQ;
        ASSERT(seqQ);

        sv = dtp->sv;
        ASSERT(sv);

        dt = dtp->parent;
        ASSERT(dt);

        max_sdu_gap    = sv->connection->policies_params->max_sdu_gap;

        spin_lock(&seqQ->lock);
        pdu = seq_queue_pop(seqQ->queue);
        seq_num = pci_sequence_number_get(pdu_pci_get_rw(pdu));
        LWE = dt_sv_rcv_lft_win(dt);
        while (pdu && (seq_num == LWE + 1)) {
                dt_sv_rcv_lft_win_set(dt, seq_num);
                spin_unlock(&seqQ->lock);
                pdu_post(dtp, pdu);

                spin_lock(&seqQ->lock);
                pdu = seq_queue_pop(seqQ->queue);
                seq_num = pci_sequence_number_get(pdu_pci_get_rw(pdu));
                LWE = dt_sv_rcv_lft_win(dt);
        }
        if (pdu) seq_queue_push_ni(seqQ->queue, pdu);

        spin_unlock(&seqQ->lock);

}
#endif

#define seqQ_for_each_entry_safe(seqQ, pos, n)                          \
        spin_lock(&seqQ->lock);                                         \
        list_for_each_entry_safe(pos, n, &seqQ->queue->head, next)

#define seqQ_for_each_entry_safe_end(seqQ)      \
        spin_unlock(&seqQ->lock)

/* AF is the factor to which A is devided in order to obtain the
 * period of the A-timer:
 *      Ta = A / AF
 */
#define AF 1

static seq_num_t process_A_expiration(struct dtp * dtp, struct dtcp * dtcp)
{
        struct dt *          dt;
        //struct dtcp *        dtcp;
        struct dtp_sv *      sv;
        struct sequencingQ * seqQ;
        seq_num_t            LWE;
        seq_num_t            seq_num;
        struct pdu *         pdu;
        bool                 in_order_del;
        bool                 incomplete_del;
        seq_num_t            max_sdu_gap;
        timeout_t            a;

        struct seq_q_entry * pos, * n;


        ASSERT(dtp);

        sv = dtp->sv;
        ASSERT(sv);

        dt = dtp->parent;
        ASSERT(dt);

        seqQ = dtp->seqQ;
        ASSERT(seqQ);

        //dtcp = dt_dtcp(dtp->parent);

        a              = dt_sv_a(dt);
        in_order_del   = sv->connection->policies_params->in_order_delivery;
        incomplete_del = sv->connection->policies_params->incomplete_delivery;
        /* FIXME: This has to be fixed from user-space */
        //max_sdu_gap    = sv->connection->policies_params->max_sdu_gap;
        max_sdu_gap    = 0;

        /* FIXME: Invoke delimiting */

        LOG_DBG("Processing A timer expiration\n\n");

        LWE = dt_sv_rcv_lft_win(dt);
        LOG_DBG("LWEU: Original LWE = %d", LWE);
        LOG_DBG("LWEU: MAX GAPS = %d", max_sdu_gap);

        seqQ_for_each_entry_safe(seqQ, pos, n) {
                LOG_DBG("LWEU: Loop LWE = %d", LWE);

                pdu = pos->pdu;
                if (!pdu_is_ok(pdu)) {
                        LOG_ERR("Bogus data, bailing out");
                        return -1;
                }
                seq_num = pci_sequence_number_get(pdu_pci_get_rw(pdu));

                if (seq_num - LWE - 1 <= max_sdu_gap) {
                        LOG_DBG("Processing A timer order or in gap");
                        if (dt_sv_rcv_lft_win_set(dt, seq_num)) {
                                LOG_ERR("Failed to set new "
                                        "left window edge");
                                spin_unlock(&seqQ->lock);
                                return 0;
                        }
                        pos->pdu = NULL;
                        list_del(&pos->next);
                        seq_q_entry_destroy(pos);

                        spin_unlock(&seqQ->lock);
                        if (pdu_post(dtp, pdu))
                                return 0;

                        spin_lock(&seqQ->lock);
                        LWE = dt_sv_rcv_lft_win(dt);
                        continue;
                }

                if (time_before_eq(jiffies, pos->time_stamp + msecs_to_jiffies(a))) {
                        LOG_DBG("Processing A timer expired");
                        /* FIXME: this have to work differently when DTCP is
                         * here */
                        if (!dtcp) {
                                if (dt_sv_rcv_lft_win_set(dt, seq_num)) {
                                        LOG_ERR("Failed to set new "
                                                "left window edge");
                                        spin_unlock(&seqQ->lock);
                                        return 0;
                                }
                                pos->pdu = NULL;
                                list_del(&pos->next);
                                seq_q_entry_destroy(pos);
                                spin_unlock(&seqQ->lock);
                                if (pdu_post(dtp, pdu))
                                        return 0;

                                spin_lock(&seqQ->lock);
                                LWE = dt_sv_rcv_lft_win(dt);
                        }
                        continue;
                }

                break;

        }
        seqQ_for_each_entry_safe_end(seqQ);

        return LWE;
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

        seq_num_sv_update =  process_A_expiration(dtp, dtcp);
        if (dtcp) {
                if (!seq_num_sv_update) {
                        LOG_ERR("ULWE returned no seq num to update");
                        return -1;
                }

                if (dtcp_sv_update(dtcp, seq_num_sv_update)) {
                        LOG_ERR("Failed to update dtcp sv");
                        return -1;
                }
        }
        a = dt_sv_a(dtp->parent);
        LOG_DBG("Going to restart A timer with a = %d and a/AF = %d", a, a/AF);
        rtimer_start(dtp->timers.a, a/AF);

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

        dtp->sv->rexmsn_ctrl  = rexmsn_ctrl;
        dtp->sv->window_based = window_based;
        dtp->sv->rate_based   = rate_based;
        dtp->sv->a            = a;

        if (a) {
                LOG_DBG("Going to start A timer with t = %d", a/AF);
                rtimer_start(dtp->timers.a, a/AF);
        }

        return 0;
}

#define MAX_NAME_SIZE 128
static const char * create_twq_name(const char *       prefix,
                                    const struct dtp * instance)
{
        static char name[MAX_NAME_SIZE];

        ASSERT(prefix);
        ASSERT(instance);

        if (snprintf(name, sizeof(name),
                     RINA_PREFIX "-%s-%pK", prefix, instance) >=
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
        *tmp->sv            = default_sv;
        /* FIXME: fixups to the state-vector should be placed here */

        spin_lock_init(&tmp->sv->lock);

        tmp->sv->connection = connection;

        tmp->policies       = &default_policies;
        /* FIXME: fixups to the policies should be placed here */

        tmp->rmt            = rmt;
        tmp->kfa            = kfa;
        tmp->seqQ           = seqQ_create(tmp);
        if (!tmp->seqQ) {
                LOG_ERR("Could not create Sequencing queue");
                dtp_destroy(tmp);
                return NULL;
        }

        twq_name = create_twq_name("twq", tmp);
        if (!twq_name) {
                dtp_destroy(tmp);
                return NULL;
        }
        tmp->twq = rwq_create(twq_name);
        if (!tmp->twq) {
                dtp_destroy(tmp);
                return NULL;
        }
#if 0
        LOG_DBG("Inactivity Timers created....\n\n");
        tmp->timers.sender_inactivity   = rtimer_create(tf_sender_inactivity,
                                                        tmp);
        tmp->timers.receiver_inactivity = rtimer_create(tf_receiver_inactivity,
                                                        tmp);
#endif
        tmp->timers.a                   = rtimer_create(tf_a, tmp);
        if (
#if 0
            !tmp->timers.sender_inactivity   ||
            !tmp->timers.receiver_inactivity ||
#endif
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
#if 0
        if (instance->timers.sender_inactivity)
                rtimer_destroy(instance->timers.sender_inactivity);
        if (instance->timers.receiver_inactivity)
                rtimer_destroy(instance->timers.receiver_inactivity);
#endif
        if (instance->timers.a)
                rtimer_destroy(instance->timers.a);

        if (instance->twq)  rwq_destroy(instance->twq);
        if (instance->seqQ) seqQ_destroy(instance->seqQ);
        if (instance->sv)
                rkfree(instance->sv);
        rkfree(instance);

        LOG_DBG("Instance %pK destroyed successfully", instance);

        return 0;
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

        if (!sdu_is_ok(sdu))
                return -1;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                sdu_destroy(sdu);
                return -1;
        }
#if 0
#ifdef CONFIG_RINA_RELIABLE_FLOW_SUPPORT
        /* Stop SenderInactivityTimer */
        if (rtimer_stop(instance->timers.sender_inactivity)) {
                LOG_ERR("Failed to stop timer");
                /* sdu_destroy(sdu);
                   return -1; */
        }
#endif
#endif

        sv = instance->sv;
        ASSERT(sv); /* State Vector must not be NULL */

        dt = instance->parent;
        ASSERT(dt);

        dtcp = dt_dtcp(dt);

        policies = instance->policies;
        ASSERT(policies);

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

                        if (rtxq_push(rtxq, cpdu)) {
                                LOG_ERR("Couldn't push to rtxq");
                                pdu_destroy(pdu);
                                return -1;
                        }
                }
                if (sv->window_based) {
                        if (!dt_sv_window_closed(dt) &&
                            pci_sequence_number_get(pci) <
                            dtcp_snd_rt_win(dtcp)) {
                                /*
                                 * Might close window
                                 */
                                if (policies->transmission_control(instance,
                                                                   pdu)) {
                                        LOG_ERR("Problems with transmission "
                                                "control");
                                        return -1;
                                }
                        } else {
                                dt_sv_window_closed_set(dt, true);
                                if (policies->closed_window(instance, pdu)) {
                                        LOG_ERR("Problems with the "
                                                "closed window policy");
                                        return -1;
                                }
                        }
                }

                if (sv->rate_based) {
                        LOG_MISSING;
                }
#if 0
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
#if 0
        /* Start SenderInactivityTimer */
        if (rtimer_restart(instance->timers.sender_inactivity,
                           2 * (dt_sv_mpl(dt) + dt_sv_r(dt) + dt_sv_a(dt)))) {
                LOG_ERR("Failed to start sender_inactiviy timer");
                return -1;
        }
#endif
        /* Post SDU to RMT */
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

int dtp_receive(struct dtp * instance,
                struct pdu * pdu)
{
        struct dtp_policies * policies;
        struct pci *          pci;
        struct dtp_sv *       sv;
        struct dtcp *         dtcp;
        struct dt *           dt;
        seq_num_t             seq_num;
        timeout_t             a;
        timeout_t             LWE;

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

        a       = instance->sv->a;
        LWE     = dt_sv_rcv_lft_win(dt);
#if 0
        /* Stop ReceiverInactivityTimer */
        if (rtimer_stop(instance->timers.receiver_inactivity)) {
                LOG_ERR("Failed to stop timer");
                /*pdu_destroy(pdu);
                  return -1;*/
        }
#endif
        seq_num = pci_sequence_number_get(pci);

        if (!(pci_flags_get(pci) ^ PDU_FLAGS_DATA_RUN)) {
                LOG_DBG("Data run flag DRF");
                drf_flag_set(sv, true);
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

        if (seq_num <= LWE) {
                LOG_DBG("DTP Receive Duplicate");
                pdu_destroy(pdu);
                dropped_pdus_inc(sv);
                LOG_DBG("Dropped a PDU, total: %d", sv->dropped_pdus);

                /* Send an ACK/Flow Control PDU with current window values */
                if (dtcp) {
                        if (dtcp_ack_flow_control_pdu_send(dtcp)) {
                                LOG_ERR("Failed to send ack / flow "
                                        "control pdu");
                                return -1;
                        }
                }
#if 0
                /* Start ReceiverInactivityTimer */
                if (rtimer_restart(instance->timers.receiver_inactivity,
                                   3 * (dt_sv_mpl(dt) +
                                        dt_sv_r(dt)   +
                                        dt_sv_a(dt))))
                        LOG_ERR("Failed to start timer");
#endif
                return 0;
        }
        /* This op puts the PDU in seq number order  and duplicates
         * considered */

        if (!a) {
                /* FIXME: delimiting goes here */
                LOG_DBG("DTP Receive deliver, seq_num: %d, LWE: %d",
                        seq_num, LWE);
                if (dt_sv_rcv_lft_win_set(dt, seq_num)) {
                        LOG_ERR("Failed to set new "
                                "left window edge");
                        return -1;
                }
                if (pdu_post(instance, pdu))
                        return -1;
                if (dtcp) {
                        if (dtcp_sv_update(dtcp, seq_num)) {
                                LOG_ERR("Failed to update dtcp sv");
                                return -1;
                        }
                }

                return 0;
        }

        spin_lock(&instance->seqQ->lock);
        LWE = dt_sv_rcv_lft_win(dt);
        while (pdu && (seq_num == LWE + 1)) {
                dt_sv_rcv_lft_win_set(dt, seq_num);
                spin_unlock(&instance->seqQ->lock);
                pdu_post(instance, pdu);

                spin_lock(&instance->seqQ->lock);
                pdu = seq_queue_pop(instance->seqQ->queue);
                seq_num = pci_sequence_number_get(pdu_pci_get_rw(pdu));
                LWE = dt_sv_rcv_lft_win(dt);
        }
        if (pdu) seq_queue_push_ni(instance->seqQ->queue, pdu);

        spin_unlock(&instance->seqQ->lock);
#if 0
        /* Start ReceiverInactivityTimer */
        if (dtcp && rtimer_restart(instance->timers.receiver_inactivity,
                                   3 * (dt_sv_mpl(dt) + dt_sv_r(dt) + dt_sv_a(dt)))) {
                LOG_ERR("Failed to start Receiver Inactivity timer");
                return -1;
        }
#endif
        return 0;
}
