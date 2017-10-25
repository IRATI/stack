/*
 *  IPC Process Factories
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

#ifndef RINA_IPCP_FACTORIES_H
#define RINA_IPCP_FACTORIES_H

#include "common.h"
#include "rds/robjects.h"
#include "ipcp-instances.h"

struct ipcp_factory_data;

struct ipcp_factory_ops {
        int                    (* init)(struct ipcp_factory_data * data);
        int                    (* fini)(struct ipcp_factory_data * data);

        struct ipcp_instance * (* create)(struct ipcp_factory_data * data,
                                          const struct name *        name,
                                          ipc_process_id_t           id,
					  irati_msg_port_t	     us_nl_port);
        int                    (* destroy)(struct ipcp_factory_data * data,
                                           struct ipcp_instance *     inst);
};

/* FIXME: Hide this data structure */
struct ipcp_factory {
        struct robject                  robj;
        struct ipcp_factory_data *      data;
        const struct ipcp_factory_ops * ops;
        struct list_head       		list;
        char * 				name;
};

struct ipcp_factories;

struct ipcp_factories * ipcpf_init(struct robject * parent);

int                     ipcpf_fini(struct ipcp_factories * factories);

struct ipcp_factory *   ipcpf_register(struct ipcp_factories *         factrs,
                                       const char *                    name,
                                       struct ipcp_factory_data *      data,
                                       const struct ipcp_factory_ops * ops);
int                     ipcpf_unregister(struct ipcp_factories * factories,
                                         struct ipcp_factory *   factory);
struct ipcp_factory *   ipcpf_find(struct ipcp_factories * factories,
                                   const char *            name);

#endif
