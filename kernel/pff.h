/*
 * PFF (PDU Forwarding Function)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#ifndef RINA_PFF_H
#define RINA_PFF_H

#include "common.h"
#include "ps-factory.h"
#include "rds/robjects.h"
#include "ipcp-instances.h"

struct pff;
struct pci;

struct pff *    pff_create(struct robject * parent,
			   struct ipcp_instance * ipcp);
int             pff_destroy(struct pff * instance);

bool            pff_is_ok(struct pff * instance);
bool            pff_is_empty(struct pff * instance);
int             pff_flush(struct pff * instance);

int             pff_add(struct pff *           instance,
                        struct mod_pff_entry * entry);
int             pff_remove(struct pff *           instance,
                           struct mod_pff_entry * entry);

int		pff_port_state_change(struct pff *	pff,
				      port_id_t		port_id,
				      bool		up);

/* NOTE: ports and entries are in-out parms */
int             pff_nhop(struct pff * instance,
                         struct pci * pci,
                         port_id_t ** ports,
                         size_t *     count);

/* NOTE: entries are of the type mod_pff_entry */
int             pff_dump(struct pff *       instance,
                         struct list_head * entries);

int             pff_modify(struct pff *       instance,
                           struct list_head * entries);

int             pff_select_policy_set(struct pff * pff,
                                      const char * path,
                                      const char * name);

int             pff_set_policy_set_param(struct pff * pff,
                                         const char * path,
                                         const char * name,
                                         const char * value);

struct pff_ps * pff_ps_get(struct pff * pff);
struct rset *   pff_rset(struct pff * pff);
struct pff *    pff_from_component(struct rina_component * component);
struct ipcp_instance * pff_ipcp_get(struct pff * pff);

#endif
