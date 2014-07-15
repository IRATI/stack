/*
 * DT (Data Transfer)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#define RINA_PREFIX "dt-utils"

#include <linux/list.h>

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dt-utils.h"
/* FIXME: Maybe dtcp_cfg should be moved somewhere else and then delete this */
#include "dtcp.h"
#include "dtcp-utils.h"
#include "dt.h"
#include "dtp.h"
#include "rmt.h"

struct cwq {
        struct rqueue * q;
        spinlock_t      lock;
};

struct cwq * cwq_create(void)
{
        struct cwq * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->q = rqueue_create();
        if (!tmp->q) {
                LOG_ERR("Failed to create closed window queue");
                rkfree(tmp);
                return NULL;
        }

        spin_lock_init(&tmp->lock);

        return tmp;
}

struct cwq * cwq_create_ni(void)
{
        struct cwq * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;

        tmp->q = rqueue_create_ni();
        if (!tmp->q) {
                LOG_ERR("Failed to create closed window queue");
                rkfree(tmp);
                return NULL;
        }

        spin_lock_init(&tmp->lock);

        return tmp;
}

int cwq_destroy(struct cwq * queue)
{
        if (!queue)
                return -1;

        ASSERT(queue->q);

        if (rqueue_destroy(queue->q, (void (*)(void *)) pdu_destroy)) {
                LOG_ERR("Failed to destroy closed window queue");
                return -1;
        }

        rkfree(queue);

        return 0;
}

int cwq_push(struct cwq * queue,
             struct pdu * pdu)
{
        if (!queue)
                return -1;

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }

        LOG_DBG("Pushing in the Closed Window Queue");

        spin_lock(&queue->lock);
        if (rqueue_tail_push_ni(queue->q, pdu)) {
                pdu_destroy(pdu);
                spin_unlock(&queue->lock);
                LOG_ERR("Failed to add PDU");
                return -1;
        }
        spin_unlock(&queue->lock);

        return 0;
}

struct pdu * cwq_pop(struct cwq * queue)
{
        struct pdu * tmp;

        if (!queue)
                return NULL;

        spin_lock(&queue->lock);
        tmp = (struct pdu *) rqueue_head_pop(queue->q);
        spin_unlock(&queue->lock);

        if (!tmp) {
                LOG_ERR("Failed to retrieve PDU");
                return NULL;
        }

        return tmp;
}

bool cwq_is_empty(struct cwq * queue)
{
        bool ret;

        if (!queue)
                return false;

        spin_lock(&queue->lock);
        ret = rqueue_is_empty(queue->q);
        spin_unlock(&queue->lock);

        return ret;
}

ssize_t cwq_size(struct cwq * queue)
{
        ssize_t tmp;

        if (!queue)
                return -1;

        spin_lock(&queue->lock);
        tmp = rqueue_length(queue->q);
        spin_unlock(&queue->lock);

        return tmp;
}

void cwq_deliver(struct cwq * queue,
                 struct dt *  dt,
                 struct rmt * rmt,
                 address_t    address,
                 qos_id_t     qos_id)
{
        struct rtxq * rtxq;
        struct dtcp * dtcp;

        if (!queue)
                return;

        if (!queue->q)
                return;

        if (!dt)
                return;

        dtcp = dt_dtcp(dt);
        if (!dtcp)
                return;

        spin_lock(&queue->lock);
        while (!rqueue_is_empty(queue->q) &&
               (dtcp_snd_lf_win(dtcp) < dtcp_snd_rt_win(dtcp))) {
                struct pdu *       pdu;
                const struct pci * pci;

                pdu = (struct pdu *) rqueue_head_pop(queue->q);
                if (!pdu) {
                        spin_unlock(&queue->lock);
                        return;
                }

                if (dtcp_rtx_ctrl(dtcp_config_get(dtcp))) {
                        rtxq = dt_rtxq(dt);
                        if (!rtxq) {
                                spin_unlock(&queue->lock);
                                LOG_ERR("Couldn't find the RTX queue");
                                return;
                        }
                        rtxq_push_ni(rtxq, pdu);
                }
                pci = pdu_pci_get_ro(pdu);
                if (dtcp_snd_lf_win_set(dtcp,
                                        pci_sequence_number_get(pci)))
                        LOG_ERR("Problems setting sender left window edge");

                if (rmt_send(rmt,
                             address,
                             qos_id,
                             pdu))
                        LOG_ERR("Problems sending PDU");
        }
        spin_unlock(&queue->lock);

        if ((dtcp_snd_lf_win(dtcp) >= dtcp_snd_rt_win(dtcp))) {
                dt_sv_window_closed_set(dt, true);
                return;
        }
        dt_sv_window_closed_set(dt, false);

        return;
}

struct rtxq_entry {
        unsigned long    time_stamp;
        struct pdu *     pdu;
        int              retries;
        struct list_head next;
};

static struct rtxq_entry * rtxq_entry_create_gfp(struct pdu * pdu, gfp_t flag)
{
        struct rtxq_entry * tmp;

        ASSERT(pdu_is_ok(pdu));

        tmp = rkzalloc(sizeof(*tmp), flag);
        if (!tmp)
                return NULL;

        tmp->pdu        = pdu;
        tmp->time_stamp = jiffies;
        tmp->retries    = 1;

        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

#if 0
static struct rtxq_entry * rtxq_entry_create(struct pdu * pdu)
{ return rtxq_entry_create_gfp(pdu, GFP_KERNEL); }
#endif

static struct rtxq_entry * rtxq_entry_create_ni(struct pdu * pdu)
{ return rtxq_entry_create_gfp(pdu, GFP_ATOMIC); }

static int rtxq_entry_destroy(struct rtxq_entry * entry)
{
        if (!entry)
                return -1;

        pdu_destroy(entry->pdu);
        rkfree(entry);

        return 0;
}

struct rtxqueue {
        struct list_head head;
};

static struct rtxqueue * rtxqueue_create_gfp(gfp_t flags)
{
        struct rtxqueue * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->head);

        return tmp;
}

static struct rtxqueue * rtxqueue_create(void)
{ return rtxqueue_create_gfp(GFP_KERNEL); }

static struct rtxqueue * rtxqueue_create_ni(void)
{ return rtxqueue_create_gfp(GFP_ATOMIC); }

static int rtxqueue_destroy(struct rtxqueue * q)
{
        struct rtxq_entry * cur;
        struct rtxq_entry * n;

        if (!q)
                return -1;

        list_for_each_entry_safe(cur, n, &q->head, next) {
                rtxq_entry_destroy(cur);
        }

        rkfree(q);

        return 0;

}

static int rtxqueue_entries_ack(struct rtxqueue * q,
                                seq_num_t         seq_num)
{
        struct rtxq_entry * cur, * n;

        ASSERT(q);

        list_for_each_entry_safe(cur, n, &q->head, next) {
                if (pci_sequence_number_get(pdu_pci_get_rw((cur->pdu))) <=
                    seq_num) {
                        list_del(&cur->next);
                        rtxq_entry_destroy(cur);
                } else
                        return 0;
        }

        return 0;
}

static int rtxqueue_entries_nack(struct rtxqueue * q,
                                 struct rmt *      rmt,
                                 seq_num_t         seq_num,
                                 uint_t            data_rtx_max)
{
        struct rtxq_entry * cur, * p;
        struct pdu *        tmp;

        ASSERT(q);

        list_for_each_entry_safe_reverse(cur, p, &q->head, next) {
                if (pci_sequence_number_get(pdu_pci_get_rw((cur->pdu))) >
                    seq_num) {
                        cur->retries++;
                        if (cur->retries >= data_rtx_max) {
                                LOG_ERR("Maximum number of rtx has been "
                                        "achieved. Can't maintain QoS");
                                rtxq_entry_destroy(cur);
                                continue;
                        }

                        tmp = pdu_dup_ni(cur->pdu);
                        if (rmt_send(rmt,
                                     pci_destination(pdu_pci_get_ro(tmp)),
                                     pci_qos_id(pdu_pci_get_ro(tmp)),
                                     tmp)) {
                                LOG_ERR("Could not send NACKed PDU to RMT");
                                pdu_destroy(tmp);
                                continue;
                        }
                } else
                        return 0;
        }

        return 0;
}

/* push in seq_num order */
static int rtxqueue_push_ni(struct rtxqueue * q, struct pdu * pdu)
{
        struct rtxq_entry * tmp, * cur, * last = NULL;
        seq_num_t           csn, psn;
        const struct pci *  pci;

        if (!q)
                return -1;

        if (!pdu_is_ok(pdu))
                return -1;

        tmp = rtxq_entry_create_ni(pdu);
        if (!tmp)
                return -1;

        pci  = pdu_pci_get_ro(pdu);
        csn  = pci_sequence_number_get((struct pci *) pci);

        if (list_empty(&q->head)) {
                list_add(&tmp->next, &q->head);
                LOG_DBG("First PDU with seqnum: %d push to rtxq at: %pk",
                        csn, q);
                return 0;
        }

        last = list_last_entry(&q->head, struct rtxq_entry, next);
        if (!last)
                return -1;

        pci  = pdu_pci_get_ro(last->pdu);
        psn  = pci_sequence_number_get((struct pci *) pci);
        if (csn == psn) {
                LOG_ERR("Another PDU with the same seq_num is in "
                        "the rtx queue!");
                return -1;
        }
        if (csn > psn) {
                list_add_tail(&tmp->next, &q->head);
                LOG_DBG("Last PDU with seqnum: %d push to rtxq at: %pk",
                        csn, q);
                return 0;
        }

        list_for_each_entry(cur, &q->head, next) {
                pci = pdu_pci_get_ro(cur->pdu);
                psn = pci_sequence_number_get((struct pci *) pci);
                if (csn == psn) {
                        LOG_ERR("Another PDU with the same seq_num is in "
                                "the rtx queue!");
                        return -1;
                }
                if (csn > psn) {
                        list_add(&tmp->next, &cur->next);
                        LOG_DBG("Middle PDU with seqnum: %d push to "
                                "rtxq at: %pk", csn, q);
                        return 0;
                }
        }

        return 0;
}

static int rtxqueue_drop(struct rtxqueue * q,
                         seq_num_t          from,
                         seq_num_t          to)
{
        struct rtxq_entry * cur, * n;
        seq_num_t           tsn;
        const struct pci *  pci;

        if (!q)
                return -1;

        list_for_each_entry_safe(cur, n, &q->head, next) {

                pci = pdu_pci_get_ro(cur->pdu);
                tsn = pci_sequence_number_get((struct pci *) pci);

                if (tsn < from && from !=0)
                        continue;
                if (tsn > to && to !=0)
                        break;
                list_del(&cur->next);
                rtxq_entry_destroy(cur);
        }

        return 0;
}

static int rtxqueue_rtx(struct rtxqueue * q,
                        unsigned int      tr,
                        struct rmt *      rmt,
                        uint_t            data_rtx_max)
{
        struct rtxq_entry * cur, * n;

        list_for_each_entry_safe(cur, n, &q->head, next) {
                if (cur->time_stamp < tr) {
                        cur->retries++;
                        if (cur->retries >= data_rtx_max) {
                                LOG_ERR("Maximum number of rtx has been "
                                        "achieved. Can't maintain QoS");
                                rtxq_entry_destroy(cur);
                                continue;
                        }
                        if (rmt_send(rmt,
                                     pci_destination(pdu_pci_get_ro(cur->pdu)),
                                     pci_qos_id(pdu_pci_get_ro(cur->pdu)),
                                     cur->pdu)) {
                                LOG_ERR("Could not send rtxed PDU to RMT");
                                continue;
                        }
                }
        }

        return 0;
}

struct rtxq {
        spinlock_t        lock;
        struct rtimer *   r_timer;
        struct dt *       parent;
        struct rmt *      rmt;
        struct rtxqueue * queue;
};

static void Rtimer_handler(void * data)
{
        struct rtxq *        q;
        struct dtcp_config * dtcp_cfg;

        q = (struct rtxq *) data;
        if (!q) {
                LOG_ERR("No RTXQ to work with");
                return;
        }

        dtcp_cfg = dtcp_config_get(dt_dtcp(q->parent));
        if (!dtcp_cfg) {
                LOG_ERR("RTX failed");
                return;
        }

        if (rtxqueue_rtx(q->queue,
                         dt_sv_tr(q->parent),
                         q->rmt,
                         dtcp_data_retransmit_max(dtcp_cfg)))
                LOG_ERR("RTX failed");

        rtimer_restart(q->r_timer, dt_sv_tr(q->parent));
}

int rtxq_destroy(struct rtxq * q)
{
        if (!q)
                return -1;

        if (q->r_timer && rtimer_destroy(q->r_timer))
                LOG_ERR("Problems destroying timer for RTXQ %pK", q->r_timer);

        if (q->queue && rtxqueue_destroy(q->queue))
                LOG_ERR("Problems destroying queue for RTXQ %pK", q->queue);

        rkfree(q);

        return 0;
}

struct rtxq * rtxq_create(struct dt *  dt,
                          struct rmt * rmt)
{
        struct rtxq * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->r_timer = rtimer_create(Rtimer_handler, tmp);
        if (!tmp->r_timer) {
                LOG_ERR("Failed to create retransmission queue");
                rtxq_destroy(tmp);
                return NULL;
        }

        tmp->queue = rtxqueue_create();
        if (!tmp->queue) {
                LOG_ERR("Failed to create retransmission queue");
                rtxq_destroy(tmp);
                return NULL;
        }

        ASSERT(dt);
        ASSERT(rmt);

        tmp->parent = dt;
        tmp->rmt    = rmt;

        spin_lock_init(&tmp->lock);

        return tmp;
}

struct rtxq * rtxq_create_ni(struct dt *  dt,
                             struct rmt * rmt)
{
        struct rtxq * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;

        tmp->r_timer = rtimer_create_ni(Rtimer_handler, tmp);
        if (!tmp->r_timer) {
                LOG_ERR("Failed to create retransmission queue");
                rtxq_destroy(tmp);
                return NULL;
        }

        tmp->queue = rtxqueue_create_ni();
        if (!tmp->queue) {
                LOG_ERR("Failed to create retransmission queue");
                rtxq_destroy(tmp);
                return NULL;
        }

        ASSERT(dt);
        ASSERT(rmt);

        tmp->parent = dt;
        tmp->rmt    = rmt;

        spin_lock_init(&tmp->lock);

        return tmp;
}

int rtxq_push_ni(struct rtxq * q,
                 struct pdu *  pdu)
{

        if (!q || !pdu_is_ok(pdu))
                return -1;

        /* is the first transmitted PDU */
        if (!rtimer_is_pending(q->r_timer))
                rtimer_restart(q->r_timer, dt_sv_tr(q->parent));

        spin_lock(&q->lock);
        rtxqueue_push_ni(q->queue, pdu);
        spin_unlock(&q->lock);
        return 0;
}

int rtxq_drop(struct rtxq * q,
              seq_num_t     from,
              seq_num_t     to)
{
        if (!q)
                return -1;

        rtimer_stop(q->r_timer);

        spin_lock(&q->lock);
        rtxqueue_drop(q->queue, from, to);
        spin_unlock(&q->lock);
        return 0;

}

int rtxq_ack(struct rtxq * q,
             seq_num_t     seq_num,
             unsigned int  tr)
{
        if (!q)
                return -1;

        spin_lock(&q->lock);
        rtxqueue_entries_ack(q->queue, seq_num);
        if (rtimer_restart(q->r_timer, tr)) {
                spin_unlock(&q->lock);
                return -1;
        }
        spin_unlock(&q->lock);

        return 0;
}

int rtxq_nack(struct rtxq * q,
              seq_num_t     seq_num,
              unsigned int  tr)
{
        struct dtcp_config * dtcp_cfg;

        if (!q)
                return -1;

        dtcp_cfg = dtcp_config_get(dt_dtcp(q->parent));
        if (!dtcp_cfg)
                return -1;

        spin_lock(&q->lock);
        rtxqueue_entries_nack(q->queue,
                              q->rmt,
                              seq_num,
                              dtcp_data_retransmit_max(dtcp_cfg));
        if (rtimer_restart(q->r_timer, tr)) {
                spin_unlock(&q->lock);
                return -1;
        }
        spin_unlock(&q->lock);

        return 0;
}
