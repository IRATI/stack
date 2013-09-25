/*
 * EFCP (Error and Flow Control Protocol)
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

#ifndef RINA_EFCP_H
#define RINA_EFCP_H

#include "common.h"
#include "du.h"

struct efcp;

/* NOTE: There's one EFCP for each flow */

/* FIXME: efcp_create() creates an EFCP-PM ... */
struct efcp * efcp_create(void);
int           efcp_destroy(struct efcp * instance);

/* FIXME: Should a cep_id_t be returned instead ? */
int           efcp_connection_create(struct efcp *             instance,
                                     const struct connection * connection,
                                     cep_id_t *                id);
int           efcp_connection_destroy(struct efcp *   instance,
                                      cep_id_t id);
int           efcp_connection_update(struct efcp * instance,
                                     cep_id_t      from,
                                     cep_id_t      to);

/* FIXME: Should these functions work over a struct connection * instead ? */

/* NOTE: efcp_send() takes the ownership of the passed SDU */
int           efcp_send(struct efcp * instance,
                        port_id_t     id,
                        struct sdu *  sdu);

/* NOTE: efcp_receive() gives the ownership of the returned PDU */
struct pdu *  efcp_receive(struct efcp * instance);

#endif
