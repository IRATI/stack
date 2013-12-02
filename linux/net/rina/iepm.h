/*
 * Ingress/Egress ports mapping
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

#ifndef RINA_IE_H
#define RINA_IE_H

#include "common.h"
#include "efcp.h"
#include "rmt.h"

/* Egress Port Mapping */
struct epm;

struct epm *            epm_create(void);
int                     epm_destroy(struct epm * instance);

/* NOTE: NULLs are allowed here */
struct efcp_container * epm_egress_get(struct epm * instance,
                                       port_id_t    id);
int                     epm_egress_set(struct epm *            instance,
                                       port_id_t               id,
                                       struct efcp_container * container);

/* Ingress Port Mapping */
struct ipm;

struct ipm *            ipm_create(void);
int                     ipm_destroy(struct ipm * instance);

/* NOTE: NULLs are allowed here */
struct rmt *            ipm_ingress_get(struct ipm * instance,
                                        port_id_t    id);
int                     ipm_ingress_set(struct ipm * instance,
                                        port_id_t    id,
                                        struct rmt * rmt);

#endif
