/*
 * K-IPCM (Kernel-IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#ifndef RINA_KIPCM_H
#define RINA_KIPCM_H

#include "common.h"
#include "ipcp.h"
#include "ipcp-factories.h"

struct kipcm;

/* The following functions represent the KIPCM northbound interface */
struct kipcm * kipcm_init(struct kobject * parent);
int            kipcm_fini(struct kipcm * kipcm);

/*
 * NOTE: factory_name must be the string published by the choosen IPC
 *       factory (module). if type is NULL a default factory will be assumed
 */
int            kipcm_ipcp_create(struct kipcm *      kipcm,
                                 const struct name * name,
                                 ipc_process_id_t    id,
                                 const char *        factory_name);
int            kipcm_ipcp_configure(struct kipcm *             kipcm,
                                    ipc_process_id_t           id,
                                    const struct ipcp_config * config);
int            kipcm_ipcp_destroy(struct kipcm *   kipcm,
                                  ipc_process_id_t id);

/* If successful: takes the ownership of the SDU */
int            kipcm_sdu_write(struct kipcm * kipcm,
                               port_id_t      id,
                               struct sdu *   sdu);
/* If successful: passes the ownership of the SDU */
int            kipcm_sdu_read(struct kipcm * kipcm,
                              port_id_t      id,
                              struct sdu **  sdu);

/* The following functions represent the KIPCM southbound interface */
struct ipcp_factory *
kipcm_ipcp_factory_register(struct kipcm *             kipcm,
                            const char *               name,
                            struct ipcp_factory_data * data,
                            struct ipcp_factory_ops *  ops);
int            kipcm_ipcp_factory_unregister(struct kipcm *        kipcm,
                                             struct ipcp_factory * factory);

int            kipcm_flow_add(struct kipcm *   kipcm,
                              ipc_process_id_t ipc_id,
                              port_id_t        id);
int            kipcm_flow_remove(struct kipcm * kipcm,
                                 port_id_t      id);

/* If successul: takes the ownership of the SDU */
int            kipcm_sdu_post(struct kipcm * kipcm,
                              port_id_t      id,
                              struct sdu *   sdu);

#endif
