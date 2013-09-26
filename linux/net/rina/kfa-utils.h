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

/* FMAPs */
struct kfa_fmap *  kfa_fmap_create(void);
int                kfa_fmap_destroy(struct kfa_fmap * map);

int                kfa_fmap_empty(struct kfa_fmap * map);
int                kfa_fmap_add(struct kfa_fmap *  map,
                                flow_id_t          key,
                                struct ipcp_flow * value);
struct ipcp_flow * kfa_fmap_find(struct kfa_fmap * map,
                                 flow_id_t         key);
int                kfa_fmap_update(struct kfa_fmap *  map,
                                   flow_id_t          key,
                                   struct ipcp_flow * value);
int                kfa_fmap_remove(struct kfa_fmap * map,
                                   flow_id_t         key);

/* PMAPs */
struct kfa_pmap;

struct kfa_pmap *  kfa_pmap_create(void);
int                kfa_pmap_destroy(struct kfa_pmap * map);

int                kfa_pmap_empty(struct kfa_pmap * map);
int                kfa_pmap_add(struct kfa_pmap *  map,
                                port_id_t          key,
                                struct ipcp_flow * value,
                                ipc_process_id_t   id);
struct ipcp_flow * kfa_pmap_find(struct kfa_pmap * map,
                                 port_id_t          key);
int                kfa_pmap_update(struct kfa_pmap *  map,
                                   port_id_t          key,
                                   struct ipcp_flow * value);
int                kfa_pmap_remove(struct kfa_pmap * map,
                                   port_id_t          key);
int                kfa_pmap_remove_all_for_id(struct kfa_pmap * map,
                                              ipc_process_id_t   id);

#endif
