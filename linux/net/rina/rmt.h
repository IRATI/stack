/*
 * RMT (Relaying and Multiplexing Task)
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

#ifndef RINA_RMT_H
#define RINA_RMT_H

#include "common.h"
#include "du.h"
#include "efcp.h"

struct rmt;

/*
 * NOTEs:
 *
 * QoS-id - An identifier unambiguous within this DIF that identifies a
 * QoS-hypercube. As QoS-cubes are created they are sequentially enumerated.
 * QoS-id is an element of Data Transfer PCI that may be used by the RMT to
 * classify PDUs.
 *
 * RMT - This task is an element of the data transfer function of a DIF.
 * Logically, it sits between the EFCP and SDU Protection.  RMT performs the
 * real time scheduling of sending PDUs on the appropriate (N-1)-ports of the
 * (N-1)-DIFs available to the RMT.
 */

/* NOTE: There's one RMT for each IPC Process */

struct rmt * rmt_create(struct kfa * kfa);
int          rmt_destroy(struct rmt * instance);

/* FIXME: Please check the following APIs */

/*
 * NOTES: Used by EFCP. Sends SDU to DIF-(N).
 *        Takes ownership of the passed SDU
 */
int          rmt_send_sdu(struct rmt * instance,
                          address_t    address,
                          cep_id_t     connection_id,
                          struct sdu * sdu);

/*
 * NOTES: Used by the SDU Protection module, sends PDU to DIF-(N-1).
 *        Takes the ownership of the passed PDU
 */
int          rmt_send_pdu(struct rmt * instance,
                          struct pdu * pdu);

#endif
