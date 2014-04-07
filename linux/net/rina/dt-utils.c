/*
 * DT (Data Transfer)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#define RINA_PREFIX "dt-utils"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dt-utils.h"

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

        if (rqueue_destroy(queue->q,
                           (void (*)(void *)) pdu_destroy)) {
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

        spin_lock(&queue->lock);
        if (rqueue_tail_push(queue->q, pdu)) {
                LOG_ERR("Failed to add PDU");
                pdu_destroy(pdu);
                spin_unlock(&queue->lock);
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

size_t cwq_size(struct cwq * queue)
{
        LOG_MISSING;
        return 0;
}

struct rtxq {
        struct rqueue * queue;
        spinlock_t      lock;
        struct rtimer * r_timer;
        struct dt *     parent;
};

struct rtxq_entry {
        unsigned long time_stamp;
        unsigned long last_time;
        struct pdu *  pdu;
        int           retries;
};

/*
 * FIXME: We are allocating memory for this entry and
 * we'll be allocating memory again when we push the entry
 * into the rtxq rqueue. We need to do this in a better way.
 */
static struct rtxq_entry * rtxq_entry_create(struct pdu * pdu)
{
        struct rtxq_entry * tmp;

        ASSERT(pdu_is_ok(pdu));

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->pdu        = pdu;
        tmp->time_stamp = jiffies;
        tmp->retries    = 1;

        return tmp;
}

static int rtxq_entry_destroy(struct rtxq_entry * entry)
{
        if (!entry)
                return -1;

        pdu_destroy(entry->pdu);
        rkfree(entry);

        return 0;
}

static void Rtimer_handler(void * data)
{
        struct rtxq * q;

        q = (struct rtxq *) data;
        if (!q) {
                LOG_ERR("No RTXQ to work with");
                return;
        }
}

int rtxq_destroy(struct rtxq * q)
{
        if (!q)
                return -1;

        if (q->queue) rqueue_destroy(q->queue,
                                     (void (*)(void *)) rtxq_entry_destroy);

        rkfree(q);

        return 0;
}

struct rtxq * rtxq_create(struct dt * dt)
{
        struct rtxq * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->queue = rqueue_create();
        if (!tmp->queue) {
                LOG_ERR("Failed to create retransmission queue");
                rkfree(tmp);
                return NULL;
        }

        tmp->r_timer = rtimer_create(Rtimer_handler, tmp);
        if (!tmp->r_timer) {
                LOG_ERR("Failed to create retransmission queue");
                rtxq_destroy(tmp);
                return NULL;
        }

        tmp->parent = dt;

        spin_lock_init(&tmp->lock);

        return tmp;
}

int rtxq_push(struct rtxq * q,
              struct pdu *  pdu)
{
        struct rtxq_entry * tmp;

        if (!q || !pdu)
                return -1;

        tmp = rtxq_entry_create(pdu);
        if (!tmp)
                return -1;

        spin_lock(&q->lock);
        if (rqueue_tail_push(q->queue, tmp)) {
                LOG_ERR("Failed to add PDU to rtxq %pK", q);
                rtxq_entry_destroy(tmp);
                spin_unlock(&q->lock);
                return -1;
        }
        spin_unlock(&q->lock);

        return 0;
}

int rtxq_ack(struct rtxq * q,
             seq_num_t     seq_num,
             timeout_t     tr)
{
        if (!q)
                return -1;

        if (rtimer_restart(q->r_timer, tr))
                return -1;

        return 0;
}

int rtxq_nack(struct rtxq * q,
              seq_num_t     seq_num,
              timeout_t     tr)
{
        if (!q)
                return -1;

        if (rtimer_restart(q->r_timer, tr))
                return -1;

        return 0;
}

int rtxq_set_pop(struct rtxq *      q,
                 seq_num_t          from,
                 seq_num_t          to,
                 struct list_head * p)
{
        if (!q)
                return -1;

        LOG_MISSING;

        return -1;
}

