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

#include <linux/types.h>

#include "common.h"
#include "pdu.h"

struct cwq;

struct cwq *    cwq_create(void);
int             cwq_destroy(struct cwq * cwq);

int             cwq_push(struct dt * dt,
                         struct pdu * pdu);
struct pdu *    cwq_pop(struct dt * dt);
bool            cwq_is_empty(struct dt *dt);

struct rexmsnq;

struct rexmsn * rexmsnq_create(void);
int             rexmsnq_destroy(struct rexmsn * rexmsn);

/* FIXME: Where do we keep the rexmsntimer for the PDU? */
int             rexmsnq_push(struct dt * dt,
                             struct pdu * pdu);
int             rexmsnq_del(struct dt * dt,
                            seq_num_t from,
                            seq_num_t to);
struct pdu **   rexmsnq_get(struct dt * dt,
                            seq_num_t from,
                            seq_num_t to);

#endif
