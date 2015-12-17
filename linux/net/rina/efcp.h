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
#include "connection.h"
#include "rds/robjects.h"

struct rmt;

/* The container holding all the EFCP instances for an IPC Process */
struct efcp_container;

struct efcp_container * efcp_container_create(struct kfa * kfa,
					      struct robject * parent);
int                     efcp_container_destroy(struct efcp_container * c);
int                     efcp_container_config_set(struct efcp_config *   efcpc,
                                                  struct efcp_container * c);
int                     efcp_container_write(struct efcp_container * container,
                                             cep_id_t                cep_id,
                                             struct sdu *            sdu);
int                     efcp_container_receive(struct efcp_container * c,
                                               cep_id_t                cep_id,
                                               struct pdu *            pdu);

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
                                               struct ipcp_instance *  user_ipcp,
                                               cep_id_t                from,
                                               cep_id_t                to);
int                     efcp_container_unbind_user_ipcp(struct efcp_container * cont,
                                                       cep_id_t cep_id);

struct efcp;

struct efcp *           efcp_container_find(struct efcp_container * container,
                                            cep_id_t                id);
struct efcp_config *    efcp_container_config(struct efcp_container * c);

int                     efcp_bind_rmt(struct efcp_container * container,
                                      struct rmt *            rmt);
int                     efcp_unbind_rmt(struct efcp_container * container);
struct efcp_container * efcp_container_get(struct efcp * efcp);
int                     efcp_enqueue(struct efcp * efcp,
                                     port_id_t     port,
                                     struct sdu *  sdu);
int                     efcp_enable_write(struct efcp * efcp);
int                     efcp_disable_write(struct efcp * efcp);

cep_id_t                efcp_src_cep_id(struct efcp * efcp);
cep_id_t                efcp_dst_cep_id(struct efcp * efcp);
address_t               efcp_src_addr(struct efcp * efcp);
address_t               efcp_dst_addr(struct efcp * efcp);
qos_id_t                efcp_qos_id(struct efcp * efcp);
port_id_t               efcp_port_id(struct efcp * efcp);

struct dt *             efcp_dt(struct efcp * efcp);

struct efcp_imap * efcp_container_get_instances(struct efcp_container *efcpc);

#endif
