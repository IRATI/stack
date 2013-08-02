/*
 * K-IPCM related utilities
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

#ifndef RINA_KIPCM_UTILS_H
#define RINA_KIPCM_UTILS_H

#include "common.h"
#include "kipcm.h"

/*
 * NOTE:
 *
 *   This is an opaque data structure mapping an IPC Process id to something.
 *   We will replace it to a proper data structure (a hash, at least) as soon
 *   as we get some apre time to do that. Functionalities will be the same, a
 *   bit more performance wise though ...
 *
 *   Francesco
 */
struct ipcp_imap;

struct ipcp_imap *       ipcp_imap_create(void);
int                      ipcp_imap_destroy(struct ipcp_imap * map);

int                      ipcp_imap_empty(struct ipcp_imap * map);

int                      ipcp_imap_add(struct ipcp_imap *     map,
                                       ipc_process_id_t       key,
                                       struct ipcp_instance * value);
struct ipcp_instance *   ipcp_imap_find(struct ipcp_imap * map,
                                        ipc_process_id_t   key);
int                      ipcp_imap_update(struct ipcp_imap *     map,
                                          ipc_process_id_t       key,
                                          struct ipcp_instance * value);
int                      ipcp_imap_remove(struct ipcp_imap * map,
                                          ipc_process_id_t   key);

struct ipcp_fmap;

struct flow;

struct ipcp_fmap *       ipcp_fmap_create(void);
int                      ipcp_fmap_destroy(struct ipcp_fmap * map);

int                      ipcp_fmap_empty(struct ipcp_fmap * map);

int                      ipcp_fmap_add(struct ipcp_fmap * map,
                                       port_id_t          key,
                                       struct flow *      value);
struct flow *            ipcp_fmap_find(struct ipcp_fmap * map,
                                        port_id_t          key);
int                      ipcp_fmap_update(struct ipcp_fmap * map,
                                          port_id_t          key,
                                          struct flow *      value);
int                      ipcp_fmap_remove(struct ipcp_fmap * map,
                                          port_id_t          key);

#endif
