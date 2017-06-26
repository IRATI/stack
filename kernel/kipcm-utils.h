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

struct ipcp_imap;
struct kipcm_pmap;
struct kipcm_smap;

struct ipcp_imap *     ipcp_imap_create(void);
int                    ipcp_imap_destroy(struct ipcp_imap * map);

int                    ipcp_imap_empty(struct ipcp_imap * map);
int                    ipcp_imap_add(struct ipcp_imap *     map,
                                     ipc_process_id_t       key,
                                     struct ipcp_instance * value);
struct ipcp_instance * ipcp_imap_find(struct ipcp_imap * map,
                                      ipc_process_id_t   key);
struct ipcp_instance * ipcp_imap_find_by_name(struct ipcp_imap *  map,
                                              const struct name * name);
int                    ipcp_imap_update(struct ipcp_imap *     map,
                                        ipc_process_id_t       key,
                                        struct ipcp_instance * value);
int                    ipcp_imap_remove(struct ipcp_imap * map,
                                        ipc_process_id_t   key);
ipc_process_id_t       ipcp_imap_find_factory(struct ipcp_imap *    map,
                                              struct ipcp_factory * factory);

struct kipcm_pmap *    kipcm_pmap_create(void);
int                    kipcm_pmap_destroy(struct kipcm_pmap * map);
int                    kipcm_pmap_empty(struct kipcm_pmap * map);
uint32_t               kipcm_pmap_find(struct kipcm_pmap * map,
                                       port_id_t           key);
int                    kipcm_pmap_update(struct kipcm_pmap * map,
                                         port_id_t           key,
					 uint32_t            value);
int                    kipcm_pmap_add(struct kipcm_pmap * map,
                                      port_id_t           key,
				      uint32_t            value);
int                    kipcm_pmap_remove(struct kipcm_pmap * map,
                                         port_id_t           key);
struct kipcm_smap *    kipcm_smap_create(void);
int                    kipcm_smap_destroy(struct kipcm_smap * map);
int                    kipcm_smap_empty(struct kipcm_smap * map);
port_id_t              kipcm_smap_find(struct kipcm_smap * map,
				       uint32_t            key);
int                    kipcm_smap_update(struct kipcm_smap * map,
					 uint32_t            key,
                                         port_id_t           value);
int                    kipcm_smap_add(struct kipcm_smap * map,
				      uint32_t            key,
                                      port_id_t           value);
int                    kipcm_smap_add_ni(struct kipcm_smap * map,
					 uint32_t            key,
                                         port_id_t           value);
int                    kipcm_smap_remove(struct kipcm_smap * map,
                                         uint32_t            key);
bool                   is_rnl_seq_num_ok(uint32_t sn);


#endif
