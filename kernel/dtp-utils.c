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
#include "dtp-utils.h"
/* FIXME: Maybe dtcp_cfg should be moved somewhere else and then delete this */
#include "dtcp.h"
#include "dtcp-ps.h"
#include "dtcp-conf-utils.h"
#include "dtp.h"
#include "rmt.h"
#include "dtp-ps.h"

/* Maximum retransmission time is 60 seconds */
#define MAX_RTX_WAIT_TIME msecs_to_jiffies(60000)

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

        if (rqueue_destroy(queue->q, (void (*)(void *)) du_destroy)) {
                LOG_ERR("Failed to destroy closed window queue");
                return -1;
        }

        rkfree(queue);

        return 0;
}

int cwq_push(struct cwq * queue,
             struct du * du)
{
        LOG_DBG("Pushing in the Closed Window Queue");

        spin_lock_bh(&queue->lock);
        if (rqueue_tail_push_ni(queue->q, du)) {
                du_destroy(du);
                spin_unlock_bh(&queue->lock);
                LOG_ERR("Failed to add PDU");
                return -1;
        }
        spin_unlock_bh(&queue->lock);

        return 0;
}
EXPORT_SYMBOL(cwq_push);

struct du * cwq_pop(struct cwq * queue)
{
        struct du * tmp;

        if (!queue)
                return NULL;

        spin_lock(&queue->lock);
        tmp = (struct du *) rqueue_head_pop(queue->q);
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
        struct du * tmp;

        if (!queue)
                return -1;

        spin_lock(&queue->lock);
        while (!rqueue_is_empty(queue->q)) {
                tmp = (struct du *) rqueue_head_pop(queue->q);
                if (!tmp) {
                        spin_unlock(&queue->lock);
                        LOG_ERR("Failed to retrieve PDU");
                        return -1;
                }
                du_destroy(tmp);
        }
        spin_unlock(&queue->lock);

        return 0;
}
EXPORT_SYMBOL(cwq_flush);

ssize_t cwq_size(struct cwq * queue)
{
        ssize_t       tmp;

        if (!queue)
                return -1;

        spin_lock_bh(&queue->lock);
        tmp = rqueue_length(queue->q);
        spin_unlock_bh(&queue->lock);

        return tmp;
}
EXPORT_SYMBOL(cwq_size);

static bool can_deliver(struct dtp * dtp, struct dtcp * dtcp)
{
        bool to_ret = false, w_ret = false, r_ret = false;
        bool is_wb, is_rb;
        struct dtp_ps * ps;

        is_wb = dtcp_window_based_fctrl(dtcp->cfg);
        is_rb = dtcp_rate_based_fctrl(dtcp->cfg);

        if (is_wb)
                w_ret = (dtp->sv->max_seq_nr_sent < dtcp->sv->snd_rt_wind_edge);

	if (is_rb)
                r_ret = !dtcp_rate_exceeded(dtcp, 0);

        LOG_DBG("Can cwq still deliver something, win: %d, rate: %d",
        	w_ret, r_ret);

        to_ret = (w_ret || r_ret);

        if ((is_wb && is_rb) && (w_ret != r_ret)) {
        	LOG_DBG("Delivering conflict...");
                rcu_read_lock();
                ps = dtp_ps_get(dtp);
                to_ret = ps->reconcile_flow_conflict(ps);
                rcu_read_unlock();
        }

        return to_ret;
}

void cwq_deliver(struct cwq * queue,
                 struct dtp * dtp,
                 struct rmt * rmt)
{
        struct dtcp *  dtcp;
        struct du  *   tmp;
        bool           rtx_ctrl;
        bool           flow_ctrl;

        bool	       rate_ctrl = false;
        int 	       sz = 0;
	uint_t 	       sc = 0;
	ssize_t	       cwq_length = 0;

	dtcp = dtp->dtcp;

        rcu_read_lock();
        rtx_ctrl  = dtcp_ps_get(dtcp)->rtx_ctrl;
        flow_ctrl = dtcp_ps_get(dtcp)->flow_ctrl;
        rcu_read_unlock();

        if(flow_ctrl) {
        	rate_ctrl = dtcp_rate_based_fctrl(dtcp->cfg);
        }

        spin_lock(&queue->lock);
        while (!rqueue_is_empty(queue->q) && can_deliver(dtp, dtcp)) {
                struct du *       du;

                du = (struct du *) rqueue_head_pop(queue->q);
                if (!du) {
                        spin_unlock(&queue->lock);
                        return;
                }
                if (rtx_ctrl) {
                        if (!dtp->rtxq) {
                                spin_unlock(&queue->lock);
                                LOG_ERR("Couldn't find the RTX queue");
                                return;
                        }
                        tmp = du_dup_ni(du);
                        if (!tmp) {
                                spin_unlock(&queue->lock);
                                return;
                        }
                        rtxq_push_ni(dtp->rtxq, tmp);
                } else if (dtp->rttq) {
                	if (rttq_push(dtp->rttq,
                		      pci_sequence_number_get(du_pci(du)))) {
                		LOG_ERR("Failed to push SN to RTT Queue");
                	}
                }
                if(rate_ctrl) {
                	sz = du_data_len(du);
			sc = dtcp->sv->pdus_sent_in_time_unit;

			if(sz >= 0) {
				if (sz + sc >= dtcp->sv->sndr_rate) {
					dtcp->sv->pdus_sent_in_time_unit =
						dtcp->sv->sndr_rate;

					break;
				} else {
					dtcp->sv->pdus_sent_in_time_unit += sz;
				}
			}
                }
                dtp->sv->max_seq_nr_sent = pci_sequence_number_get(&du->pci);
                dtcp->sv->snd_lft_win = dtp->sv->max_seq_nr_sent;

                spin_unlock(&queue->lock);
                dtp_pdu_send(dtp, rmt, du);
                spin_lock(&queue->lock);
        }

        if (!rqueue_is_empty(queue->q)) {
        	if(dtcp_window_based_fctrl(dtcp->cfg)) {
			dtp->sv->window_closed = true;
        	}

                if(dtcp_rate_based_fctrl(dtcp->cfg)) {
                	LOG_DBG("rbfc Cannot deliver anymore, closing...");
                	dtp->sv->rate_fulfiled = true;
                	dtp_start_rate_timer(dtp, dtcp);
                }

                spin_unlock(&queue->lock);
                return;
        }

        if(dtcp_window_based_fctrl(dtcp->cfg)) {
		dtp->sv->window_closed = false;
        }

        if(dtcp_rate_based_fctrl(dtcp->cfg)) {
                dtp->sv->rate_fulfiled = true;
        }

        cwq_length = rqueue_length(queue->q);

        spin_unlock(&queue->lock);

        if (cwq_length < dtcp_max_closed_winq_length(dtp->dtcp->cfg))
                efcp_enable_write(dtp->efcp);

        LOG_DBG("CWQ has delivered until %u", dtp->sv->max_seq_nr_sent);

        return;
}

static struct rtxq_entry * rtxq_entry_create_gfp(struct du * du, gfp_t flag)
{
        struct rtxq_entry * tmp;

        tmp = rkzalloc(sizeof(*tmp), flag);
        if (!tmp)
                return NULL;

        tmp->du        = du;
        tmp->time_stamp = jiffies;
        tmp->retries    = 0;

        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

#if 0
static struct rtxq_entry * rtxq_entry_create(struct du * du)
{ return rtxq_entry_create_gfp(du, GFP_KERNEL); }
#endif

static struct rtxq_entry * rtxq_entry_create_ni(struct du * du)
{ return rtxq_entry_create_gfp(du, GFP_ATOMIC); }

int rtxq_entry_destroy(struct rtxq_entry * entry)
{
        if (!entry)
                return -1;

        du_destroy(entry->du);
        list_del(&entry->next);
        rkfree(entry);

        return 0;
}
EXPORT_SYMBOL(rtxq_entry_destroy);

static struct rtxqueue * rtxqueue_create_gfp(gfp_t flags)
{
        struct rtxqueue * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->head);
	tmp->len = 0;
	tmp->drop_pdus = 0;

        return tmp;
}

static struct rtxqueue * rtxqueue_create(void)
{ return rtxqueue_create_gfp(GFP_KERNEL); }

static void rtxqueue_flush(struct rtxqueue * q)
{
        struct rtxq_entry * cur, * n;

        ASSERT(q);

        list_for_each_entry_safe(cur, n, &q->head, next) {
                rtxq_entry_destroy(cur);
		q->len --;
        }
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
                seq_num_t    seq;

                seq = pci_sequence_number_get(&cur->du->pci);
                if (seq <= seq_num) {
                        LOG_DBG("Seq num acked: %u. Size %d", seq, q->len);
                        rtxq_entry_destroy(cur);
			q->len--;
                } else
                        return 0;
        }

        return 0;
}

static int rtxqueue_entries_nack(struct rtxqueue * q,
                                 struct dtp *      dtp,
                                 struct rmt *      rmt,
                                 seq_num_t         seq_num,
                                 uint_t            data_rtx_max)
{
        struct rtxq_entry * cur, * p;
        struct du *        tmp;
        // Used by rbfc.
        struct dtcp *	    dtcp;

        int sz;
	uint_t sc;

        dtcp = dtp->dtcp;

        /*
         * FIXME: this should be change since we are sending in inverse order
         * and it could be problematic because of gaps and A timer
         */
        list_for_each_entry_safe_reverse(cur, p, &q->head, next) {
                if (pci_sequence_number_get(&cur->du->pci) >=
                    seq_num) {
                        cur->retries++;
                        if (cur->retries >= data_rtx_max) {
                                LOG_ERR("Maximum number of rtx has been "
                                        "achieved. Can't maintain QoS");
                                rtxq_entry_destroy(cur);
				q->len--;
				q->drop_pdus++;
                                continue;
                        }
			if(dtp &&
				dtcp &&
				dtcp_rate_based_fctrl(dtcp->cfg)) {

				sz = du_data_len(cur->du);
				sc = dtcp->sv->pdus_sent_in_time_unit;

				if(sz >= 0) {
					if ( (sz + sc) >= dtcp->sv->sndr_rate) {
						dtcp->sv->pdus_sent_in_time_unit =
							dtcp->sv->sndr_rate;
					} else {
						dtcp->sv->pdus_sent_in_time_unit += sz;
					}
				}

				if(dtcp_rate_exceeded(dtcp, 1)) {
					dtp->sv->rate_fulfiled = true;
					dtp_start_rate_timer(dtp, dtcp);
					break;
				}
			}
                        tmp = du_dup_ni(cur->du);
                        if (dtp_pdu_send(dtp,
					 rmt,
					 tmp))
                                continue;
                } else
                        return 0;
        }

        return 0;
}

unsigned long rtxqueue_entry_timestamp(struct rtxqueue * q, seq_num_t sn)
{
        struct rtxq_entry * cur;
        seq_num_t           csn;

        list_for_each_entry(cur, &q->head, next) {
                csn = pci_sequence_number_get(&cur->du->pci);
                if (csn > sn) {
                        LOG_WARN("PDU not in rtxq. Received "
                        		"SN: %u, RtxQ SN: %u. Size: %u",
					sn, csn, q->len);
                        return -1;
                }
                if (csn == sn) {
                	/* Ignore time_stamps from retransmitted PDUs */
                        if (cur->retries != 0)
                        	return 0;

                        return cur->time_stamp;
                }
        }

        return -1;
}

/* push in seq_num order */
static int rtxqueue_push_ni(struct rtxqueue * q, struct du * du)
{
        struct rtxq_entry * tmp, * cur, * last = NULL;
        seq_num_t           csn, psn;

        csn  = pci_sequence_number_get(&du->pci);

        tmp = rtxq_entry_create_ni(du);
        if (!tmp)
                return -1;

        if (list_empty(&q->head)) {
                list_add(&tmp->next, &q->head);
		q->len++;
                LOG_DBG("First PDU with seqnum: %u push to rtxq at: %pk",
                        csn, q);
                return 0;
        }

        last = list_last_entry(&q->head, struct rtxq_entry, next);
        if (!last)
                return -1;

        psn = pci_sequence_number_get(&last->du->pci);
        if (csn == psn) {
                LOG_ERR("Another PDU with the same seq_num %u, is in "
                        "the rtx queue!", csn);
                return -1;
        }
        if (csn > psn) {
                list_add_tail(&tmp->next, &q->head);
		q->len++;
		LOG_DBG("Last PDU with seqnum: %u push to rtxq at: %pk",
                        csn, q);
                return 0;
        }

        list_for_each_entry(cur, &q->head, next) {
                psn = pci_sequence_number_get(&cur->du->pci);
                if (csn == psn) {
                        LOG_ERR("Another PDU with the same seq_num is in "
                                "the rtx queue!");
                        return -1;
                }
                if (csn > psn) {
                        list_add(&tmp->next, &cur->next);
			q->len++;
			LOG_DBG("Middle PDU with seqnum: %u push to "
                                "rtxq at: %pk", csn, q);
                        return 0;
                }
        }

        LOG_ERR("PDU not pushed!");

        return -1;
}

/* Exponential backoff after each retransmission */
static unsigned long time_to_rtx(struct rtxq_entry * cur, unsigned int tr)
{
	unsigned long rtx_wtime;

	rtx_wtime = (1 + cur->retries*cur->retries)*tr;
	if (rtx_wtime > MAX_RTX_WAIT_TIME)
		rtx_wtime = MAX_RTX_WAIT_TIME;

	return cur->time_stamp + rtx_wtime;
}

/* Called while holding the rtx queueu lock */
static int rtxqueue_rtx(struct rtxq * q,
                        unsigned int tr,
                        struct dtp * dtp,
                        uint_t       data_rtx_max)
{
        struct rtxq_entry * cur, * n;
        struct du *        tmp;
        seq_num_t           seq = 0;
        // Used by rbfc.
        struct dtcp *	    dtcp;
        int sz, res;
        uint_t sc, dropped_sn, dropped_pdus, cwq_max_size;
        bool start_rv_timer;
        timeout_t rv;

        ASSERT(q);
        ASSERT(dt);
        ASSERT(rmt);

        dtcp = dtp->dtcp;
        dropped_pdus = 0;
        dropped_sn = 0;

        list_for_each_entry_safe(cur, n, &q->queue->head, next) {
                seq = pci_sequence_number_get(&cur->du->pci);

                LOG_DBG("Checking RTX PDU %u, now: %lu >?< %lu + %u",
                        seq, jiffies, cur->time_stamp, tr);

                if (time_before_eq(time_to_rtx(cur, tr), jiffies)) {
                        cur->retries++;
                        if (cur->retries >= data_rtx_max) {
                                LOG_WARN("Maximum number of rtx has been "
                                        "achieved for SeqN %u. Dropping "
                                        "PDU, data is lost", seq);
                                rtxq_entry_destroy(cur);
				q->queue->len--;
				q->queue->drop_pdus++;
				dropped_pdus++;
				if (seq > dropped_sn)
					dropped_sn = seq;
                                continue;
                        }

                        if (dtp && dtcp &&
                            dtcp_rate_based_fctrl(dtcp->cfg)) {
                        	sz = du_data_len(cur->du);
				sc = dtcp->sv->pdus_sent_in_time_unit;

				if(sz >= 0) {
					if ( (sz + sc) >= dtcp->sv->sndr_rate) {
						dtcp->sv->pdus_sent_in_time_unit = 
							dtcp->sv->sndr_rate;
					} else {
						dtcp->sv->pdus_sent_in_time_unit += sz; 
					}
				}

				if(dtcp_rate_exceeded(dtcp, 1)) {
					dtp->sv->rate_fulfiled = true;
					dtp_start_rate_timer(dtp, dtcp);
					break;
				}
                        }

                        tmp = du_dup_ni(cur->du);

                        spin_unlock(&q->lock);
                        res = dtp_pdu_send(dtp, q->rmt, tmp);
                        spin_lock(&q->lock);

                        if (res) continue;

                        LOG_DBG("Retransmitted PDU with seqN %u", seq);
                } else {
                        LOG_DBG("RTX timer: from here PDUs still have time,"
                                "finishing...");
                        break;
                }
        }

        LOG_DBG("RTXQ %pK has delivered until %u", q, seq);

        start_rv_timer = false;
        if (dropped_pdus) {
        	spin_lock_bh(&dtcp->parent->sv_lock);

        	/* Move LWE */
        	if (dtcp->sv->snd_lft_win < dropped_sn)
        		dtcp->sv->snd_lft_win = dropped_sn;

        	/* TODO, check if we need to move RWE? */

        	/* If RTXQ is empty and CWQ is full, activate rendezvous */
        	cwq_max_size = dtcp_max_closed_winq_length(dtcp->cfg);
        	if (q->queue->len == 0 &&
        			cwq_size(dtcp->parent->cwq) == cwq_max_size) {
        		/* Check if rendezvous PDU needs to be sent*/
        		if (!dtcp->sv->rendezvous_sndr) {
        			dtcp->sv->rendezvous_sndr = true;

        			LOG_DBG("RV at the sender (CPU: %d)",
        				  smp_processor_id());

        			/* Start rendezvous timer, wait for Tr to fire */
        			start_rv_timer = true;
        			rv = jiffies_to_msecs(dtcp->parent->sv->tr);
        		}
        	}

        	spin_unlock_bh(&dtcp->parent->sv_lock);
        }

	if (start_rv_timer) {
		LOG_DBG("Window is closed. SND LWE: %u | SND RWE: %u | TR: %u",
				dtcp->sv->snd_lft_win,
				dtcp->sv->snd_rt_wind_edge,
				dtcp->parent->sv->tr);

		/* Send rendezvous PDU and start time */
		rtimer_start(&dtcp->parent->timers.rendezvous, rv);
	}

        return 0;
}

static bool rtxqueue_empty(struct rtxqueue * q)
{
        if (!q)
                return true;

        return list_empty(&q->head);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
static void rtx_timer_func(void * data)
#else
static void rtx_timer_func(struct timer_list * tl)
#endif
{
        struct rtxq *        q;
        unsigned int         tr;
	struct dtp *         dtp;

        LOG_DBG("RTX timer triggered...");

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        dtp = (struct dtp *) data;
#else
        dtp = from_timer(dtp, tl, timers.rtx);
#endif
        if (!dtp) {
                LOG_ERR("No DTP data to work with");
                return;
        }

        q = dtp->rtxq;
        tr = dtp->sv->tr;

        spin_lock(&q->lock);
        if (rtxqueue_rtx(q, tr, dtp,
			 dtp->dtcp->cfg->rxctrl_cfg->data_retransmit_max))
                LOG_ERR("RTX failed");

        if (!rtxqueue_empty(q->queue))
                rtimer_restart(&dtp->timers.rtx, tr);

        spin_unlock(&q->lock);
}

int rtxq_destroy(struct rtxq * q)
{
	unsigned long flags;

        if (!q)
                return -1;

        spin_lock_irqsave(&q->lock, flags);
        if (q->queue && rtxqueue_destroy(q->queue))
                LOG_ERR("Problems destroying queue for RTXQ %pK", q->queue);

        spin_unlock_irqrestore(&q->lock, flags);

        rkfree(q);

        return 0;
}

struct rtxq * rtxq_create(struct dtp * dtp,
                          struct rmt * rmt,
			  struct efcp_container * container,
			  struct dtcp_config * dtcp_cfg,
			  cep_id_t cep_id)
{
        struct rtxq * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        rtimer_init(rtx_timer_func, &dtp->timers.rtx, dtp);

        tmp->queue = rtxqueue_create();
        if (!tmp->queue) {
                LOG_ERR("Failed to create retransmission queue");
                rtxq_destroy(tmp);
                return NULL;
        }

        ASSERT(rmt);

        tmp->parent = dtp;
        tmp->rmt    = rmt;

        spin_lock_init(&tmp->lock);

        return tmp;
}

int rtxq_size(struct rtxq * q)
{
	unsigned int ret;
        if (!q)
                return -1;

        spin_lock_bh(&q->lock);
        ret = q->queue->len;
        spin_unlock_bh(&q->lock);
        return ret;
}
EXPORT_SYMBOL(rtxq_size);

int rtxq_drop_pdus(struct rtxq * q)
{
	int ret;
        if (!q)
                return -1;

        spin_lock_bh(&q->lock);
        ret = q->queue->drop_pdus;
        spin_unlock_bh(&q->lock);
        return ret;
}

unsigned long rtxq_entry_timestamp(struct rtxq * q, seq_num_t sn)
{
        unsigned long timestamp;

        if (!q)
                return 0;

        spin_lock_bh(&q->lock);
        timestamp = rtxqueue_entry_timestamp(q->queue, sn);
        spin_unlock_bh(&q->lock);

        return timestamp;
}
EXPORT_SYMBOL(rtxq_entry_timestamp);

int rtxq_push_ni(struct rtxq * q,
                 struct du *  du)
{
	int res;

        spin_lock_bh(&q->lock);

        /* is the first transmitted PDU */
        rtimer_start(&q->parent->timers.rtx, q->parent->sv->tr);

        res = rtxqueue_push_ni(q->queue, du);

        spin_unlock_bh(&q->lock);

        return res;
}
EXPORT_SYMBOL(rtxq_push_ni);

int rtxq_flush(struct rtxq * q)
{
        if (!q || !q->queue)
                return -1;

        rtimer_stop(&q->parent->timers.rtx);

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
	int res;

        if (!q)
                return -1;

        spin_lock_bh(&q->lock);

        res = rtxqueue_entries_ack(q->queue, seq_num);

        rtimer_restart(&q->parent->timers.rtx, tr);

        spin_unlock_bh(&q->lock);

        return res;
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

        dtcp_cfg = q->parent->dtcp->cfg;
        if (!dtcp_cfg)
                return -1;

        rcu_read_lock();
        data_retransmit_max = dtcp_ps_get(q->parent->dtcp)->
                                        rtx.data_retransmit_max;
        rcu_read_unlock();

        spin_lock(&q->lock);
        rtxqueue_entries_nack(q->queue,
                              q->parent,
                              q->rmt,
                              seq_num,
                              data_retransmit_max);
        if (rtimer_restart(&q->parent->timers.rtx, tr)) {
                spin_unlock(&q->lock);
                return -1;
        }

        spin_unlock(&q->lock);

        return 0;
}

int dtp_pdu_send(struct dtp *  dtp,
        	 struct rmt *  rmt,
		 struct du *   du)
{
	struct efcp_container * efcpc;
	cep_id_t		dest_cep_id;

	/* Remote flow case */
	if (pci_source(&du->pci) != pci_destination(&du->pci)) {
		if (dtp->dtcp->sv->rendezvous_rcvr) {
			LOG_INFO("Sending to RMT in RV at RCVR");
		}

		if (rmt_send(rmt, du)) {
			LOG_ERR("Problems sending PDU to RMT");
			return -1;
		}

		return 0;
	}

	/* Local flow case */
	dest_cep_id = pci_cep_destination(&du->pci);
	efcpc = dtp->efcp->container;
	if (unlikely(!efcpc || du_decap(du) || !is_du_ok(du))) { /*Decap PDU */
	        LOG_ERR("Could not retrieve the EFCP container in"
	        "loopback operation");
	        du_destroy(du);
	        return -1;
 	}
	if (efcp_container_receive(efcpc, dest_cep_id, du)) {
	        LOG_ERR("Problems sending PDU to loopback EFCP");
	        return -1;
	}

	return 0;
}
EXPORT_SYMBOL(dtp_pdu_send);

/* Here begins the RTT estimator when there is not RTX*/
static struct rtt_entry * rtt_entry_create_gfp(seq_num_t sn, gfp_t flag)
{
        struct rtt_entry * tmp;

        tmp = rkzalloc(sizeof(*tmp), flag);
        if (!tmp)
                return NULL;

        tmp->sn = sn;
        tmp->time_stamp = jiffies;

        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

static struct rtt_entry * rtt_entry_create(seq_num_t sn)
{ return rtt_entry_create_gfp(sn, GFP_ATOMIC); }

static int rtt_entry_destroy(struct rtt_entry * entry)
{
        if (!entry)
                return -1;

        list_del(&entry->next);
        rkfree(entry);

        return 0;
}

static struct rttq * rttq_create_gfp(gfp_t flags)
{
        struct rttq * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->head);

        return tmp;
}

struct rttq * rttq_create(void)
{ return rttq_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(rttq_create);

static int rttq_entry_destroy(struct rtt_entry * entry)
{
	list_del(&entry->next);
	rkfree(entry);
	return 0;
}

/* No locking required, it's always called with DTP-SV lock taken */
int rttq_flush(struct rttq * q)
{
        struct rtt_entry * cur, * n;

        ASSERT(q);

        list_for_each_entry_safe(cur, n, &q->head, next) {
                rttq_entry_destroy(cur);
        }

        return 0;
}
EXPORT_SYMBOL(rttq_flush);

int rttq_destroy(struct rttq * q)
{
        if (!q)
                return -1;

        spin_lock(&q->lock);
        rttq_flush(q);
        spin_unlock(&q->lock);

        rkfree(q);

        return 0;
}
EXPORT_SYMBOL(rttq_destroy);

static unsigned long rttqueue_entry_timestamp(struct rttq * q, seq_num_t sn)
{
        struct rtt_entry * cur;
        seq_num_t           csn;

        list_for_each_entry(cur, &q->head, next) {
                csn = cur->sn;
                if (csn > sn) {
                        return 0;
                }
                if (csn == sn) {
                        return cur->time_stamp;
                }
        }

        return 0;
}

unsigned long rttq_entry_timestamp(struct rttq * q, seq_num_t sn)
{
        unsigned long timestamp;

        if (!q)
                return 0;

        spin_lock_bh(&q->lock);
        timestamp = rttqueue_entry_timestamp(q, sn);
        spin_unlock_bh(&q->lock);

        return timestamp;
}
EXPORT_SYMBOL(rttq_entry_timestamp);

static int rttq_push_ni(struct rttq * q, seq_num_t sn)
{
	struct rtt_entry * new, * cur, * last = NULL;
	seq_num_t          psn;

	new = rtt_entry_create(sn);
	if (!new) {
		LOG_ERR("Could not create an rtt queue entry");
		return -1;
	}

	if (list_empty(&q->head)) {
		list_add(&new->next, &q->head);
		return 0;
	}

    	last = list_last_entry(&q->head, struct rtt_entry, next);
    	if (!last)
            	return -1;

    	psn = last->sn;
    	if (sn == psn) {
            	LOG_ERR("Another PDU with the same seq_num %u, is in "
            			"the RTT queue!", sn);
            	return 0;
    	}
    	if (sn > psn) {
            	list_add_tail(&new->next, &q->head);
            	return 0;
    	}

    	list_for_each_entry(cur, &q->head, next) {
            	psn = cur->sn;
            	if (sn == psn) {
                    LOG_ERR("Another PDU with the same seq_num is in "
                            "the RTT queue!");
                    return 0;
            }
            if (sn > psn) {
                    list_add(&new->next, &cur->next);
                    return 0;
            }
    	}

    	LOG_ERR("SN not pushed!");

	return 0;
}

int rttq_push(struct rttq * q, seq_num_t sn)
{
	int to_return;

	spin_lock_bh(&q->lock);
	to_return = rttq_push_ni(q, sn);
	spin_unlock_bh(&q->lock);

	return to_return;
}
EXPORT_SYMBOL(rttq_push);

int rttq_drop(struct rttq * q, seq_num_t sn)
{
	struct rtt_entry * cur, * n;

	spin_lock_bh(&q->lock);
	if (list_empty(&q->head)) {
		spin_unlock_bh(&q->lock);
		return 0;
	}
	cur = NULL;
	list_for_each_entry_safe(cur, n, &q->head, next) {
		if (cur->sn <= sn) {
			rtt_entry_destroy(cur);
		} else {
			spin_unlock_bh(&q->lock);
			return 0;
		}
	}
	spin_unlock_bh(&q->lock);
	return 0;
}
EXPORT_SYMBOL(rttq_drop);
