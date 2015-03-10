/*
 * KFA related utilities
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

#ifndef RINA_KFA_UTILS_H
#define RINA_KFA_UTILS_H

#include "common.h"
#include "kfa.h"

struct ipcp_flow;

/* PMAPs */
struct kfa_pmap;

struct kfa_pmap *  kfa_pmap_create(void);
int                kfa_pmap_destroy(struct kfa_pmap * map);

int                kfa_pmap_empty(struct kfa_pmap * map);
int                kfa_pmap_add(struct kfa_pmap *  map,
                                port_id_t          key,
                                struct ipcp_flow * value_flow);
int                kfa_pmap_add_ni(struct kfa_pmap *  map,
                                   port_id_t          key,
                                   struct ipcp_flow * value_flow);
struct ipcp_flow * kfa_pmap_find(struct kfa_pmap * map,
                                 port_id_t         key);
int                kfa_pmap_update(struct kfa_pmap *  map,
                                   port_id_t          key,
                                   struct ipcp_flow * value_flow);
int                kfa_pmap_remove(struct kfa_pmap * map,
                                   port_id_t         key);

#endif
