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

        return tmp;
}


int cwq_destroy(struct cwq * queue) 
{
        if (!queue)
                return -1;

        if (queue->q) {
                if (rqueue_destroy(queue->q,
                                   (void (*)(void *)) pdu_destroy)) {
                        LOG_ERR("Failed to destroy closed window queue");
                        return -1;
                }
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

        if (rqueue_tail_push(queue->q, pdu)) {
                LOG_ERR("Failed to add PDU");
                return -1;
        }

        return 0;
}


struct pdu * cwq_pop(struct cwq * queue)
{
        struct pdu * tmp;

        if (!queue)
                return NULL;

        tmp = (struct pdu *) rqueue_head_pop(queue->q);
        if (!tmp) {
                LOG_ERR("Failed to retrieve PDU");
                return NULL;
        }

        return tmp;
}

bool cwq_is_empty(struct cwq * queue)
{
        if (!queue)
                return false;
        
        return rqueue_is_empty(queue->q);
}

struct rtxq {
       
};

struct rtxq * rtxq_create(void) 
{
        struct rtxq * tmp;
        
        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}


int rtxq_destroy(struct rtxq * q)
{
        if (!q)
                return -1;

        rkfree(q);

        return 0;
}
