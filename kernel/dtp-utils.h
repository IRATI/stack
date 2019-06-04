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

#include "efcp-str.h"
#include "du.h"
#include "rmt.h"

struct cwq *        cwq_create(void);
struct cwq *        cwq_create_ni(void);
int                 cwq_destroy(struct cwq * q);

bool                cwq_write_enable(struct cwq * queue);
void                cwq_write_enable_set(struct cwq * queue,
                                         bool         flag);
int                 cwq_push(struct cwq * q,
                             struct du * du);
struct du *         cwq_pop(struct cwq * q);
bool                cwq_is_empty(struct cwq * q);
int                 cwq_flush(struct cwq * q);
ssize_t             cwq_size(struct cwq * q);
void                cwq_deliver(struct cwq * queue,
                                struct dtp * dtp,
                                struct rmt * rmt);

struct rtxq;

struct rtxq *       rtxq_create(struct dtp * dtp,
                                struct rmt * rmt,
				struct efcp_container * container,
				struct dtcp_config * dtcp_cfg,
				cep_id_t cep_id);
int                 rtxq_destroy(struct rtxq * q);

int		    rtxq_size(struct rtxq * q);
int		    rtxq_drop_pdus(struct rtxq * q);
unsigned long       rtxq_entry_timestamp(struct rtxq * q,
                                         seq_num_t sn);
int                 rtxq_entry_destroy(struct rtxq_entry * entry);
int                 rtxq_push_ni(struct rtxq * q,
                                 struct du *  du);
int                 rtxq_ack(struct rtxq * q,
                             seq_num_t     seq_num,
                             timeout_t     tr);
int                 rtxq_nack(struct rtxq * q,
                              seq_num_t     seq_num,
                              timeout_t     tr);
int                 rtxq_flush(struct rtxq * q);

int 		    dtp_pdu_send(struct dtp *  dtp,
				 struct rmt * rmt,
                                 struct du * du);

struct rttq;

struct rttq * 	    rttq_create(void);
int 		    rttq_destroy(struct rttq * q);
unsigned long 	    rttq_entry_timestamp(struct rttq * q, seq_num_t sn);
int 		    rttq_drop(struct rttq * q, seq_num_t sn);
int 		    rttq_push(struct rttq * q, seq_num_t sn);
int 		    rttq_flush(struct rttq * q);
#endif
