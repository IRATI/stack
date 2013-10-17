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
#include "fidm.h"
#include "efcp.h"

enum ipcp_config_type {
        IPCP_CONFIG_UINT   = 1,
        IPCP_CONFIG_STRING,
};

struct ipcp_config_value {
        enum ipcp_config_type type;
        void *                data;
};

struct ipcp_config_entry {
        string_t * name;
        string_t * value;
};

struct ipcp_config {
        struct list_head           next;
        struct ipcp_config_entry * entry;
};

/* Represents the information about a DIF (name, type, configuration) */
struct dif_info {
        /* The DIF type. Can be 'NORMAL' or one of the shims */
        string_t *          type;

        /* The DIF Distributed Application Name (DAN) */
        struct name *       dif_name;

        /* The DIF configuration (policies, parameters, etc) */
        struct dif_config * configuration;
};

/* Represents a DIF configuration (policies, parameters, etc) */
struct dif_config {

        /* List of configuration entries */
        struct list_head ipcp_config_entries;
};

/* Pre-declared, the implementation should define it properly */
struct ipcp_instance_data;

struct ipcp_instance_ops {
        int  (* flow_allocate_request)(struct ipcp_instance_data * data,
                                       const struct name *         source,
                                       const struct name *         dest,
                                       const struct flow_spec *    flow_spec,
                                       port_id_t                   id,
                                       flow_id_t                   fid);
        int  (* flow_allocate_response)(struct ipcp_instance_data * data,
                                        flow_id_t                   flow_id,
                                        port_id_t                   port_id,
                                        int                         result);
        int  (* flow_deallocate)(struct ipcp_instance_data * data,
                                 port_id_t                   id);

        int  (* application_register)(struct ipcp_instance_data *   data,
                                      const struct name *           source);
        int  (* application_unregister)(struct ipcp_instance_data * data,
                                        const struct name *         source);

        int  (* assign_to_dif)(struct ipcp_instance_data * data,
                               const struct dif_info *     information);

        int  (* update_dif_config)(struct ipcp_instance_data * data,
                                   const struct dif_config *   configuration);

        /* Takes the ownership of the passed SDU */
        int  (* sdu_write)(struct ipcp_instance_data * data,
                           port_id_t                   id,
                           struct sdu *                sdu);

        cep_id_t  (* connection_create)(struct ipcp_instance_data * data,
                                        port_id_t                   port_id,
                                        address_t                   source,
                                        address_t                   dest,
                                        qos_id_t                    qos_id,
                                        int                         policies);

        int  (* connection_update)(struct ipcp_instance_data * data,
                                   cep_id_t                    src_cep_id,
                                   cep_id_t                    dst_cep_id);

        int  (* connection_destroy)(struct ipcp_instance_data * data,
                                    cep_id_t                    src_cep_id);

        cep_id_t
        (* connection_create_arrived)(struct ipcp_instance_data * data,
                                      port_id_t                   port_id,
                                      address_t                   source,
                                      address_t                   dest,
                                      qos_id_t                    qos_id,
                                      cep_id_t                    dst_cep_id,
                                      int                         policies);
};

#endif
