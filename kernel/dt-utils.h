/*
 * DT (Data Transfer) related utilities
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

#ifndef RINA_DT_UTILS_H
#define RINA_DT_UTILS_H

#include <linux/list.h>

#include "common.h"
#include "pdu.h"
#include "rmt.h"

struct cwq;
struct dtp;
struct dt;
struct rtxq_entry;

struct cwq *        cwq_create(void);
struct cwq *        cwq_create_ni(void);
int                 cwq_destroy(struct cwq * q);

bool                cwq_write_enable(struct cwq * queue);
void                cwq_write_enable_set(struct cwq * queue,
                                         bool         flag);
int                 cwq_push(struct cwq * q,
                             struct pdu * pdu);
struct pdu *        cwq_pop(struct cwq * q);
bool                cwq_is_empty(struct cwq * q);
int                 cwq_flush(struct cwq * q);
ssize_t             cwq_size(struct cwq * q);
void                cwq_deliver(struct cwq * queue,
                                struct dt *  dt,
                                struct rmt * rmt);
seq_num_t            cwq_peek(struct cwq * queue);

struct rtxq;

struct rtxq *       rtxq_create(struct dt *  dt,
                                struct rmt * rmt);
struct rtxq *       rtxq_create_ni(struct dt *  dt,
                                   struct rmt * rmt);
int                 rtxq_destroy(struct rtxq * q);

int		    rtxq_size(struct rtxq * q);
int		    rtxq_drop_pdus(struct rtxq * q);
/* FIXME: Where do we keep the rexmsntimer for the PDU? */
struct rtxq_entry * rtxq_entry_peek(struct rtxq * q,
                                    seq_num_t sn);
unsigned long       rtxq_entry_timestamp(struct rtxq_entry * entry);
int                 rtxq_entry_retries(struct rtxq_entry * entry);
int                 rtxq_entry_destroy(struct rtxq_entry * entry);
int                 rtxq_push_sn(struct rtxq * q,
                                 seq_num_t sn);
int                 rtxq_push_ni(struct rtxq * q,
                                 struct pdu *  pdu);
int                 rtxq_ack(struct rtxq * q,
                             seq_num_t     seq_num,
                             timeout_t     tr);
int                 rtxq_nack(struct rtxq * q,
                              seq_num_t     seq_num,
                              timeout_t     tr);
int                 rtxq_flush(struct rtxq * q);

int 		    dt_pdu_send(struct dt *  dt,
        	    	        struct rmt * rmt,
                                struct pdu * pdu);
#endif
