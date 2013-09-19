/*
 *  IPC Processes layer
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

#ifndef RINA_IPCP_H
#define RINA_IPCP_H

#include <linux/list.h>
#include <linux/kobject.h>

#include "du.h"

enum ipcp_config_type {
        IPCP_CONFIG_UINT   = 1,
        IPCP_CONFIG_STRING,
};

struct ipcp_config_value {
        enum ipcp_config_type type;
        void *                data;
};

struct ipcp_config_entry {
        char *                     name;
        struct ipcp_config_value * value;
};

struct ipcp_config { 
	struct list_head           list;
	struct ipcp_config_entry * entry;
};

/* Pre-declared, the shim should define it properly */
struct ipcp_instance_data;

struct ipcp_instance_ops {
	int  (* flow_allocate_request)(struct ipcp_instance_data * data,
                                       const struct name *         source,
                                       const struct name *         dest,
                                       const struct flow_spec *    flow_spec,
                                       port_id_t                   id,
                                       uint_t			   seq_num);
	int  (* flow_allocate_response)(struct ipcp_instance_data * data,
                                        port_id_t                   id,
                                        uint_t			    seq_num,
                                        response_reason_t *         response);
	int  (* flow_deallocate)(struct ipcp_instance_data * data,
                                 port_id_t                   id);

	int  (* application_register)(struct ipcp_instance_data *   data,
                                      const struct name *           source);
	int  (* application_unregister)(struct ipcp_instance_data * data,
                                        const struct name *         source);

	int  (* assign_to_dif)(struct ipcp_instance_data * data,
                               const struct name * 	   dif_name,
                               const struct ipcp_config *  configuration);

        /* Takes the ownership of the passed SDU */
	int  (* sdu_write)(struct ipcp_instance_data * data,
                           port_id_t                   id,
                           struct sdu *                sdu);
};

#endif
