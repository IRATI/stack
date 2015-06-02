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
#include "ps-factory.h"

struct pft_ps {
        struct ps_base base;

        int (* pft_add)(struct pft_ps *          ps,
                        struct modpdufwd_entry * entry);
        int (* pft_remove)(struct pft_ps *          ps,
                           struct modpdufwd_entry * entry);

        bool (* pft_is_empty)(struct pft_ps * ps);
        int  (* pft_flush)(struct pft_ps * ps);

        /* NOTE: ports and entries are in-out parms */
        int  (* pft_nhop)(struct pft_ps * ps,
                          struct pci *    pci,
                          port_id_t **    ports,
                          size_t *        count);

        /* NOTE: entries are of the type modpdufwd_entry */
        int  (* pft_dump)(struct pft_ps *    ps,
                          struct list_head * entries);

        /* Reference used to access the PFT data model. */
        struct pft * pft;

        /* Data private to the policy-set implementation. */
        void *       dm;
};

/*
 * The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary.
 */
int pft_ps_publish(struct ps_factory * factory);
int pft_ps_unpublish(const char * name);

#endif
