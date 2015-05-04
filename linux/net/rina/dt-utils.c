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

#include <linux/list.h>

#define RINA_PREFIX "dt-utils"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dt-utils.h"
/* FIXME: Maybe dtcp_cfg should be moved somewhere else and then delete this */
#include "dtcp.h"
#include "dtcp-ps.h"
#include "dtcp-utils.h"
#include "dt.h"
#include "dtp.h"
#include "rmt.h"

#define RTIMER_ENABLED 1

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
        unsigned long flags;

        if (!queue)
                return -1;

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }

        LOG_DBG("Pushing in the Closed Window Queue");

        spin_lock_irqsave(&queue->lock, flags);
        if (rqueue_tail_push_ni(queue->q, pdu)) {
                pdu_destroy(pdu);
                spin_unlock_irqrestore(&queue->lock, flags);
                LOG_ERR("Failed to add PDU");
                return -1;
        }
        spin_unlock_irqrestore(&queue->lock, flags);

        return 0;
}
EXPORT_SYMBOL(cwq_push);

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

int cwq_flush(struct cwq * queue)
{
        struct pdu * tmp;

        if (!queue)
                return -1;

        spin_lock(&queue->lock);
        while (!rqueue_is_empty(queue->q)) {
                tmp = (struct pdu *) rqueue_head_pop(queue->q);
                if (!tmp) {
                        spin_unlock(&queue->lock);
                        LOG_ERR("Failed to retrieve PDU");
                        return -1;
                }
                pdu_destroy(tmp);
        }
        spin_unlock(&queue->lock);

        return 0;
}
EXPORT_SYMBOL(cwq_flush);

ssize_t cwq_size(struct cwq * queue)
{
        ssize_t       tmp;
        unsigned long flags;

        if (!queue)
                return -1;

        spin_lock_irqsave(&queue->lock, flags);
        tmp = rqueue_length(queue->q);
        spin_unlock_irqrestore(&queue->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(cwq_size);

static void enable_write(struct cwq * cwq,
                         struct dt *  dt)
{
        struct dtcp *        dtcp;
        struct dtp *         dtp;
        struct dtcp_config * cfg;
        uint_t               max_len;

        if (!dt)
                return;

        dtcp = dt_dtcp(dt);
        if (!dtcp)
                return;

        cfg = dtcp_config_get(dtcp);
        if (!cfg)
                return;

        dtp = dt_dtp(dt);
        if (!dtp)
                return;

        max_len = dtcp_max_closed_winq_length(cfg);
        if (cwq_size(cwq) < max_len)
                efcp_enable_write(dt_efcp(dt));

        return;
}

int dt_pdu_send(struct dt *  dt,
                struct rmt * rmt,
                address_t    address,
                qos_id_t     qos_id,
                struct pdu * pdu)
{
        struct efcp * efcp;

        if (!dt)
                return -1;

        efcp = dt_efcp(dt);
        if (!efcp)
                return -1;

        return common_efcp_pdu_send(efcp, rmt, address, qos_id, pdu);
}
EXPORT_SYMBOL(dt_pdu_send);

void cwq_deliver(struct cwq * queue,
                 struct dt *  dt,
                 struct rmt * rmt,
                 address_t    address,
                 qos_id_t     qos_id)
{
        struct rtxq *           rtxq;
        struct dtcp *           dtcp;
        struct dtp *            dtp;
        struct pdu  *           tmp;
        bool                    rtx_ctrl;

        if (!queue)
                return;

        if (!queue->q)
                return;

        if (!dt)
                return;

        dtcp = dt_dtcp(dt);
        if (!dtcp)
                return;

        rcu_read_lock();
        rtx_ctrl = dtcp_ps_get(dtcp)->rtx_ctrl;
        rcu_read_unlock();

        dtp = dt_dtp(dt);
        if (!dtp)
                return;

        spin_lock(&queue->lock);
        while (!rqueue_is_empty(queue->q) &&
               (dtp_sv_max_seq_nr_sent(dtp) < dtcp_snd_rt_win(dtcp))) {
                struct pdu *       pdu;
                const struct pci * pci;

                pdu = (struct pdu *) rqueue_head_pop(queue->q);
                if (!pdu) {
                        spin_unlock(&queue->lock);
                        return;
                }

                if (rtx_ctrl) {
                        rtxq = dt_rtxq(dt);
                        if (!rtxq) {
                                spin_unlock(&queue->lock);
                                LOG_ERR("Couldn't find the RTX queue");
                                return;
                        }
                        tmp = pdu_dup_ni(pdu);
                        if (!tmp) {
                                spin_unlock(&queue->lock);
                                return;
                        }
                        rtxq_push_ni(rtxq, tmp);
                }
                pci = pdu_pci_get_ro(pdu);
                if (dtp_sv_max_seq_nr_set(dtp,
                                          pci_sequence_number_get(pci)))
                        LOG_ERR("Problems setting sender left window edge");

                dt_pdu_send(dt, rmt, address, qos_id, pdu);
        }
        spin_unlock(&queue->lock);

        LOG_DBG("CWQ has delivered until %u", dtp_sv_max_seq_nr_sent(dtp));

        if ((dtp_sv_max_seq_nr_sent(dtp) >= dtcp_snd_rt_win(dtcp))) {
                dt_sv_window_closed_set(dt, true);
                enable_write(queue, dt);
                return;
        }
        dt_sv_window_closed_set(dt, false);
        enable_write(queue, dt);

        return;
}

/* NOTE: used only by dump_we */
seq_num_t cwq_peek(struct cwq * queue)
{
        seq_num_t          ret;
        struct pdu *       pdu;
        const struct pci * pci;
        unsigned long      flags;

        if (!queue)
                return -1;

        spin_lock_irqsave(&queue->lock, flags);
        if (rqueue_is_empty(queue->q)){
                spin_unlock_irqrestore(&queue->lock, flags);
                return 0;
        }
        pdu = (struct pdu *) rqueue_head_pop(queue->q);
        if (!pdu) {
                spin_unlock_irqrestore(&queue->lock, flags);
                return -1;
        }
        pci = pdu_pci_get_ro(pdu);
        if (!pci) {
                spin_unlock_irqrestore(&queue->lock, flags);
                pdu_destroy(pdu);
                return -1;
        }
        ret = pci_sequence_number_get(pci);
        if (rqueue_head_push_ni(queue->q, pdu)) {
                spin_unlock_irqrestore(&queue->lock, flags);
                pdu_destroy(pdu);
                return ret;
        }
        spin_unlock_irqrestore(&queue->lock, flags);

        return ret;
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
        tmp->retries    = 0;

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
        list_del(&entry->next);
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

static int rtxqueue_flush(struct rtxqueue * q)
{
        struct rtxq_entry * cur, * n;

        ASSERT(q);

        list_for_each_entry_safe(cur, n, &q->head, next) {
                rtxq_entry_destroy(cur);
        }

        return 0;
}

static int rtxqueue_destroy(struct rtxqueue * q)
{
        if (!q)
                return -1;

        rtxqueue_flush(q);
        rkfree(q);

        return 0;

}

static int rtxqueue_entries_ack(struct rtxqueue * q,
                                seq_num_t         seq_num)
{
        struct rtxq_entry * cur, * n;

        ASSERT(q);

        list_for_each_entry_safe(cur, n, &q->head, next) {
                struct pci * pci;
                seq_num_t    seq;

                pci = pdu_pci_get_rw(cur->pdu);
                seq = pci_sequence_number_get(pci);
                if (seq <= seq_num) {
                        LOG_DBG("Seq num acked: %u", seq);
                        rtxq_entry_destroy(cur);
                } else
                        return 0;
        }

        return 0;
}

static int rtxqueue_entries_nack(struct rtxqueue * q,
                                 struct dt *       dt,
                                 struct rmt *      rmt,
                                 seq_num_t         seq_num,
                                 uint_t            data_rtx_max)
{
        struct rtxq_entry * cur, * p;
        struct pdu *        tmp;

        ASSERT(q);
        ASSERT(dt);
        ASSERT(rmt);

        /*
         * FIXME: this should be change since we are sending in inverse order
         * and it could be problematic because of gaps and A timer
         */
        list_for_each_entry_safe_reverse(cur, p, &q->head, next) {
                if (pci_sequence_number_get(pdu_pci_get_rw((cur->pdu))) >=
                    seq_num) {
                        cur->retries++;
                        if (cur->retries >= data_rtx_max) {
                                LOG_ERR("Maximum number of rtx has been "
                                        "achieved. Can't maintain QoS");
                                rtxq_entry_destroy(cur);
                                continue;
                        }

                        tmp = pdu_dup_ni(cur->pdu);
                        if (dt_pdu_send(dt,
                                        rmt,
                                        pci_destination(pdu_pci_get_ro(tmp)),
                                        pci_qos_id(pdu_pci_get_ro(tmp)),
                                        tmp))
                                continue;
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
                return 0;
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
                        return 0;
                }
                if (csn > psn) {
                        list_add(&tmp->next, &cur->next);
                        LOG_DBG("Middle PDU with seqnum: %d push to "
                                "rtxq at: %pk", csn, q);
                        return 0;
                }
        }

        LOG_DBG("PDU not pushed!");

        return 0;
}

static int rtxqueue_rtx(struct rtxqueue * q,
                        unsigned int      tr,
                        struct dt *       dt,
                        struct rmt *      rmt,
                        uint_t            data_rtx_max)
{
        struct rtxq_entry * cur, * n;
        struct pdu *        tmp;
        seq_num_t           seq = 0;
        unsigned long       tr_jiffies;

        ASSERT(q);
        ASSERT(dt);
        ASSERT(rmt);

        tr_jiffies = msecs_to_jiffies(tr);

        list_for_each_entry_safe(cur, n, &q->head, next) {
                seq = pci_sequence_number_get(pdu_pci_get_ro(cur->pdu));
                LOG_DBG("Checking RTX PDU %u, now: %lu >?< %lu + %lu (%u ms)",
                        seq, jiffies, cur->time_stamp, tr_jiffies,
                        tr);
                if (time_before_eq(cur->time_stamp + tr_jiffies,
                                   jiffies)) {
                        cur->retries++;
                        if (cur->retries >= data_rtx_max) {
                                LOG_ERR("Maximum number of rtx has been "
                                        "achieved for SeqN %u. Can't "
                                        "maintain QoS", seq);
                                rtxq_entry_destroy(cur);
                                continue;
                        }
                        tmp = pdu_dup_ni(cur->pdu);
                        if (dt_pdu_send(dt,
                                        rmt,
                                        pci_destination(pdu_pci_get_ro(tmp)),
                                        pci_qos_id(pdu_pci_get_ro(tmp)),
                                        tmp))
                                continue;
                } else {
                        LOG_DBG("RTX timer: from here PDUs still have time,"
                                "finishing...");
                        break;
                }
        }

        LOG_DBG("RTXQ %pK has delivered until %u", q, seq);

        return 0;
}

static bool rtxqueue_empty(struct rtxqueue * q)
{
        if (!q)
                return true;

        return list_empty(&q->head);
}

struct rtxq {
        spinlock_t                lock;
        struct rtimer *           r_timer;
        struct dt *               parent;
        struct rmt *              rmt;
        struct rtxqueue *         queue;
};

static void rtx_timer_func(void * data)
{
        struct rtxq *        q;
        struct dtcp_config * dtcp_cfg;
        unsigned int         tr;
        unsigned int         data_retransmit_max;

        LOG_DBG("RTX timer triggered...");

        q = (struct rtxq *) data;
        if (!q || !q->rmt || !q->parent) {
                LOG_ERR("No RTXQ to work with");
                return;
        }

        dtcp_cfg = dtcp_config_get(dt_dtcp(q->parent));
        if (!dtcp_cfg) {
                LOG_ERR("RTX failed");
                return;
        }

        rcu_read_lock();
        data_retransmit_max = dtcp_ps_get(dt_dtcp(q->parent))
                                        ->rtx.data_retransmit_max;
        rcu_read_unlock();

        tr = dt_sv_tr(q->parent);

        spin_lock(&q->lock);
        if (rtxqueue_rtx(q->queue,
                         tr,
                         q->parent,
                         q->rmt,
                         data_retransmit_max))
                LOG_ERR("RTX failed");
        spin_unlock(&q->lock);

#if RTIMER_ENABLED
        if (!rtxqueue_empty(q->queue))
                rtimer_restart(q->r_timer, tr);
        LOG_DBG("RTX timer ending...");
#endif

        return;
}

int rtxq_destroy(struct rtxq * q)
{
        unsigned long flags;

        if (!q)
                return -1;

        spin_lock_irqsave(&q->lock, flags);
#if RTIMER_ENABLED
        if (q->r_timer && rtimer_destroy(q->r_timer))
                LOG_ERR("Problems destroying timer for RTXQ %pK", q->r_timer);
#endif
        if (q->queue && rtxqueue_destroy(q->queue))
                LOG_ERR("Problems destroying queue for RTXQ %pK", q->queue);
        spin_unlock_irqrestore(&q->lock, flags);

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

#if RTIMER_ENABLED
        tmp->r_timer = rtimer_create(rtx_timer_func, tmp);
        if (!tmp->r_timer) {
                LOG_ERR("Failed to create retransmission queue");
                rtxq_destroy(tmp);
                return NULL;
        }
#endif

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

#if RTIMER_ENABLED
        tmp->r_timer = rtimer_create_ni(rtx_timer_func, tmp);
        if (!tmp->r_timer) {
                LOG_ERR("Failed to create retransmission queue");
                rtxq_destroy(tmp);
                return NULL;
        }
#endif

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
        unsigned long flags;

        if (!q || !pdu_is_ok(pdu))
                return -1;

        spin_lock_irqsave(&q->lock, flags);
#if RTIMER_ENABLED
        /* is the first transmitted PDU */
        rtimer_start(q->r_timer, dt_sv_tr(q->parent));
#endif
        rtxqueue_push_ni(q->queue, pdu);
        spin_unlock_irqrestore(&q->lock, flags);
        return 0;
}

int rtxq_flush(struct rtxq * q)
{
        if (!q || !q->queue)
                return -1;

#if RTIMER_ENABLED
        rtimer_stop(q->r_timer);
#endif
        spin_lock(&q->lock);
        rtxqueue_flush(q->queue);
        spin_unlock(&q->lock);
        return 0;

}
EXPORT_SYMBOL(rtxq_flush);

int rtxq_ack(struct rtxq * q,
             seq_num_t     seq_num,
             unsigned int  tr)
{
        unsigned long flags;

        if (!q)
                return -1;

        spin_lock_irqsave(&q->lock, flags);
        rtxqueue_entries_ack(q->queue, seq_num);
#if RTIMER_ENABLED
        rtimer_restart(q->r_timer, tr);
#endif
        spin_unlock_irqrestore(&q->lock, flags);

        return 0;
}
EXPORT_SYMBOL(rtxq_ack);

int rtxq_nack(struct rtxq * q,
              seq_num_t     seq_num,
              unsigned int  tr)
{
        struct dtcp_config * dtcp_cfg;
        unsigned int         data_retransmit_max;

        if (!q || !q->parent || !q->rmt)
                return -1;

        dtcp_cfg = dtcp_config_get(dt_dtcp(q->parent));
        if (!dtcp_cfg)
                return -1;

        rcu_read_lock();
        data_retransmit_max = dtcp_ps_get(dt_dtcp(q->parent))->
                                        rtx.data_retransmit_max;
        rcu_read_unlock();

        spin_lock(&q->lock);
        rtxqueue_entries_nack(q->queue,
                              q->parent,
                              q->rmt,
                              seq_num,
                              data_retransmit_max);
#if RTIMER_ENABLED
        if (rtimer_restart(q->r_timer, tr)) {
                spin_unlock(&q->lock);
                return -1;
        }
#endif
        spin_unlock(&q->lock);

        return 0;
}

int common_efcp_pdu_send(struct efcp * efcp,
        		 struct rmt *  rmt,
        	         address_t     address,
                         qos_id_t      qos_id,
			 struct pdu *  pdu)
{
        const struct pci *	pci;
	struct efcp_container * efcpc;
	cep_id_t		dest_cep_id;

	if (!pdu)
	        return -1;

	pci = pdu_pci_get_ro(pdu);
	if (!pci)
		return -1;

	/* Remote flow case */
	if (pci_source(pci) != pci_destination(pci)) {
	        if (rmt_send(rmt, address, qos_id, pdu)) {
	                LOG_ERR("Problems sending PDU to RMT");
	                return -1;
	        }
	        return 0;
	}

	/* Local flow case */
	dest_cep_id = pci_cep_destination(pci);
	efcpc = efcp_container_get(efcp);
	if (!efcpc) {
	        LOG_ERR("Could not retrieve the EFCP container in"
	        "loopback operation");
	        pdu_destroy(pdu);
	        return -1;
 	}
	if (efcp_container_receive(efcpc, dest_cep_id, pdu)) {
	        LOG_ERR("Problems sending PDU to loopback EFCP");
	        return -1;
	}

	return 0;
}
EXPORT_SYMBOL(common_efcp_pdu_send);
