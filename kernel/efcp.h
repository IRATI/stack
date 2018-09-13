/*
 * EFCP (Error and Flow Control Protocol)
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

#ifndef RINA_EFCP_H
#define RINA_EFCP_H

#include "common.h"
#include "du.h"
#include "ipcp-instances.h"
#include "kfa.h"
#include "rds/robjects.h"

struct rmt;

/* The container holding all the EFCP instances for an IPC Process */
struct efcp_container;

struct efcp_container * efcp_container_create(struct kfa * kfa,
					      struct robject * parent);
int                     efcp_container_destroy(struct efcp_container * c);
int                     efcp_container_config_set(struct efcp_container * c,
                                                  struct efcp_config *    cfg);
int                     efcp_container_write(struct efcp_container * container,
                                             cep_id_t                cep_id,
                                             struct du *             du);
int                     efcp_container_receive(struct efcp_container * c,
                                               cep_id_t                cep_id,
                                               struct du *             du);

/* FIXME: Rename efcp_connection_*() as efcp_*() */
cep_id_t                efcp_connection_create(struct efcp_container * container,
                                               struct ipcp_instance *  user_ipcp,
                                               address_t               src_addr,
                                               address_t               dst_addr,
                                               port_id_t               port_id,
                                               qos_id_t                qos_id,
                                               cep_id_t                src_cep_id,
                                               cep_id_t                dst_cep_id,
                                               struct dtp_config *     dtp_cfg,
                                               struct dtcp_config *    dtcp_cfg);
int                     efcp_connection_destroy(struct efcp_container * cont,
                                                cep_id_t                id);
int                     efcp_connection_update(struct efcp_container * cont,
                                               cep_id_t                from,
                                               cep_id_t                to);
int                     efcp_connection_modify(struct efcp_container * cont,
					       cep_id_t		       cep_id,
                                               address_t               src,
                                               address_t               dst);
int                     efcp_container_unbind_user_ipcp(struct efcp_container * cont,
                                                       cep_id_t cep_id);

struct efcp;

struct efcp *           efcp_container_find(struct efcp_container * container,
                                            cep_id_t                id);

int                     efcp_enqueue(struct efcp * efcp,
                                     port_id_t     port,
                                     struct du *   du);
int                     efcp_enable_write(struct efcp * efcp);
int                     efcp_disable_write(struct efcp * efcp);

int                     efcp_address_change(struct efcp_container * c,
					    address_t new_address);

struct efcp_imap * efcp_container_get_instances(struct efcp_container *efcpc);

#endif
