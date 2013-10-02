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
#include "fidm.h"

struct ipcp_imap;
struct seqn_fmap;

struct ipcp_imap     * ipcp_imap_create(void);
int                    ipcp_imap_destroy(struct ipcp_imap * map);

int                    ipcp_imap_empty(struct ipcp_imap * map);
int                    ipcp_imap_add(struct ipcp_imap *     map,
                                     ipc_process_id_t       key,
                                     struct ipcp_instance * value);
struct ipcp_instance * ipcp_imap_find(struct ipcp_imap * map,
                                      ipc_process_id_t   key);
int                    ipcp_imap_update(struct ipcp_imap *     map,
                                        ipc_process_id_t       key,
                                        struct ipcp_instance * value);
int                    ipcp_imap_remove(struct ipcp_imap * map,
                                        ipc_process_id_t   key);

struct seqn_fmap     * seqn_fmap_create(void);
int                    seqn_fmap_destroy(struct seqn_fmap * map);
int                    seqn_fmap_empty(struct seqn_fmap * map);
rnl_sn_t               seqn_fmap_find(struct seqn_fmap * map,
                                      flow_id_t          key);
int                    seqn_fmap_update(struct seqn_fmap * map,
                                        flow_id_t          key,
                                        rnl_sn_t           value);
int                    seqn_fmap_add(struct seqn_fmap * map,
                                     flow_id_t          key,
                                     rnl_sn_t           value);
int                    seqn_fmap_remove(struct seqn_fmap * map,
                                        flow_id_t          key);


#endif
