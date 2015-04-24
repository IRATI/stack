/*
 * PFT (PDU Forwarding Table) kRPI
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#ifndef RINA_PFT_PS_H
#define RINA_PFT_PS_H

#include <linux/types.h>

#include "pft.h"
#include "pdu.h"
#include "rds/rfifo.h"
#include "ps-factory.h"

struct pft_ps {
        struct ps_base base;

        int (* next_hop)(struct pft_ps * ps,
                         struct pci *    pci,
                         port_id_t **    ports,
                         size_t *        count);

        /* Reference used to access the DTP data model. */
        struct pft * dm;

        /* Data private to the policy-set implementation. */
        void *       priv;
};

/*
 * The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary.
 */
int pft_ps_publish(struct ps_factory * factory);
int pft_ps_unpublish(const char * name);

#endif
