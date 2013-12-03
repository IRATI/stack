/*
 * Ingress/Egress port-2-IPCP instance mapping
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef RINA_IEPIM_H
#define RINA_IEPIM_H

#include "common.h"
#include "efcp.h"
#include "rmt.h"

/* Egress Port/IPCP-Instance Mapping */
struct epim;

struct epim *           epim_create(void);
int                     epim_destroy(struct epim * instance);

/* NOTE: NULLs are allowed here */
struct efcp_container * epim_egress_get(struct epim * instance,
                                        port_id_t    id);
int                     epim_egress_set(struct epim *            instance,
                                        port_id_t               id,
                                        struct efcp_container * container);

/* Ingress Port/IPCP-Instance Mapping */
struct ipim;

struct ipim *           ipim_create(void);
int                     ipim_destroy(struct ipim * instance);

/* NOTE: NULLs are allowed here */
struct rmt *            ipim_ingress_get(struct ipim * instance,
                                         port_id_t    id);
int                     ipim_ingress_set(struct ipim * instance,
                                         port_id_t    id,
                                         struct rmt * rmt);

#endif
