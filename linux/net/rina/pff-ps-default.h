/*
 * Default policy set for PFF
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
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

#ifndef PFF_PS_DEFAULT_H
#define PFF_PS_DEFAULT_H

#include "pff.h"
#include "pff-ps.h"

int              default_add(struct pff_ps *        ps,
                             struct mod_pff_entry * entry);
int              default_remove(struct pff_ps *        ps,
                                struct mod_pff_entry * entry);
bool             default_is_empty(struct pff_ps * ps);
int              default_flush(struct pff_ps * ps);
int              default_nhop(struct pff_ps * ps,
                              struct pci *    pci,
                              port_id_t **    ports,
                              size_t *        count);
int              default_dump(struct pff_ps *    ps,
                              struct list_head * entries);
int              default_modify(struct pff_ps *    ps,
                                struct list_head * entries);
struct ps_base * pff_ps_default_create(struct rina_component * component);
void             pff_ps_default_destroy(struct ps_base * bps);

#endif
