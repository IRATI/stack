/*
 * PFF (PDU Forwarding Function) kRPI
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

#ifndef RINA_PFF_PS_H
#define RINA_PFF_PS_H

#include <linux/types.h>

#include "pff.h"
#include "pci.h"
#include "ps-factory.h"

struct pff_ps {
        struct ps_base base;

        int (* pff_add)(struct pff_ps *        ps,
                        struct mod_pff_entry * entry);
        int (* pff_remove)(struct pff_ps *        ps,
                           struct mod_pff_entry * entry);

	int (* pff_port_state_change)(struct pff_ps * ps,
				      port_id_t port_id, bool up);

        bool (* pff_is_empty)(struct pff_ps * ps);
        int  (* pff_flush)(struct pff_ps * ps);

        /* NOTE: ports and entries are in-out parms */
        int  (* pff_nhop)(struct pff_ps * ps,
                          struct pci *    pci,
                          port_id_t **    ports,
                          size_t *        count);

        /* NOTE: entries are of the type mod_pff_entry */
        int  (* pff_dump)(struct pff_ps *    ps,
                          struct list_head * entries);

        /* Dump PFF contents and add entries in an atomic operation */
        int  (* pff_modify)(struct pff_ps *    ps,
                            struct list_head * entries);

        /* Reference used to access the PFF data model. */
        struct pff * dm;

        /* Data private to the policy-set implementation. */
        void *       priv;
};

/*
 * The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary.
 */
int pff_ps_publish(struct ps_factory * factory);
int pff_ps_unpublish(const char * name);

#endif
