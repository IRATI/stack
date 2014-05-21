/*
 * PFT (PDU Forwarding Table)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

struct pft;

struct pft * pft_create(void);
int          pft_destroy(struct pft * instance);

bool         pft_is_ok(struct pft * instance);
bool         pft_is_empty(struct pft * instance);
int          pft_flush(struct pft * instance);

int          pft_add(struct pft *       instance,
                     address_t          destination,
                     qos_id_t           qos_id,
                     const port_id_t  * ports,
                     size_t             count);
int          pft_remove(struct pft *       instance,
                        address_t          destination,
                        qos_id_t           qos_id,
                        const port_id_t  * ports,
                        size_t             count);

/* NOTE: ports and entries are in-out parms */
int          pft_nhop(struct pft * instance,
                      address_t    destination,
                      qos_id_t     qos_id,
                      port_id_t ** ports,
                      size_t *     count);

int          pft_dump(struct pft *       instance,
                      struct list_head * entries);
#endif
