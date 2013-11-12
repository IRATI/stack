/*
 * DTP (Data Transfer Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef RINA_DTP_H
#define RINA_DTP_H

#include "common.h"
#include "du.h"
#include "rmt.h"

struct dtcp;
struct dtp;

struct dtp * dtp_create(struct rmt * rmt,
                        struct kfa * kfa);
int          dtp_destroy(struct dtp * instance);

int          dtp_bind(struct dtp *  instance,
                      struct dtcp * peer);
int          dtp_unbind(struct dtp * instance);

/* Sends a SDU to the DTP (DTP takes the ownership of the passed SDU) */
int          dtp_send(struct dtp * instance,
                      struct sdu * sdu);

/* DTP receives a PDU from RMT */
int          dtp_receive(struct dtp * instance,
                         struct pdu * pdu);

#endif
