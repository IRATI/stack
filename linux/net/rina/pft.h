/*
 * PDU-FWD-T (PDU Forwarding Table)
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

#ifndef RINA_PFT_H
#define RINA_PFT_H

#include "common.h"
#include "qos.h"

/* FIXME: This definition is bounded to RNL and must be removed from here */
struct pdu_ft_entry {
        address_t destination;
        qos_id_t  qos_id;
        /* list_head ports; */
};

struct pft;

struct pft * pft_create(void);
int          pft_destroy(struct pft * instance);

int          pft_entry_add(struct pft * instance,
                           address_t       destination,
                           qos_id_t        qos_id,
                           port_id_t       port_id);
int          pft_entry_remove(struct pft * instance,
                              address_t    destination,
                              qos_id_t     qos_id,
                              port_id_t    port_id);

port_id_t    pft_next_hop(struct pft * instance,
                          address_t    destination,
                          qos_id_t     qos_id);

#endif
