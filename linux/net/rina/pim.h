/*
 * Port-2-IPCP instance mapping
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

#ifndef RINA_PIM_H
#define RINA_PIM_H

#include "common.h"
#include "ipcp-instances.h"

/* Port/IPCP-Instance Mapping */
struct pim;

struct pim *                pim_create(void);
int                         pim_destroy(struct pim * pim);

/* NOTE: NULLs are allowed here */
struct ipcp_instance_data * pim_ingress_get(struct pim * pim,
                                            port_id_t    id);
int                         pim_ingress_set(struct pim *                pim,
                                            port_id_t                   id,
                                            struct ipcp_instance_data * ipcp);


#endif
