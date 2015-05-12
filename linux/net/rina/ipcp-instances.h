/*
 *  IPC Processes Instances
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

#ifndef RINA_IPCP_INSTANCES_H
#define RINA_IPCP_INSTANCES_H

#include <linux/list.h>
#include <linux/kobject.h>

#include "du.h"
#include "connection.h"

struct ipcp_instance;

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

struct dt_cons {
        /* The length of the address field in the DTP PCI, in bytes */
        u_int16_t address_length;

        /* The length of the CEP-id field in the DTP PCI, in bytes */
        u_int16_t cep_id_length;

        /* The length of the length field in the DTP PCI, in bytes */
        u_int16_t length_length;

        /* The length of the Port-id field in the DTP PCI, in bytes */
        u_int16_t port_id_length;

        /* The length of QoS-id field in the DTP PCI, in bytes */
        u_int16_t qos_id_length;

        /* The length of the sequence number field in the DTP PCI, in bytes */
        u_int16_t seq_num_length;

        /* The maximum length allowed for a PDU in this DIF, in bytes */
        u_int32_t max_pdu_size;

        /*
         * The maximum PDU lifetime in this DIF, in milliseconds. This is MPL
         * in delta-T
         */
        u_int32_t max_pdu_life;

        /*
         * True if the PDUs in this DIF have CRC, TTL, and/or encryption.
         * Since headers are encrypted, not just user data, if any flow uses
         * encryption, all flows within the same DIF must do so and the same
         * encryption algorithm must be used for every PDU; we cannot identify
         * which flow owns a particular PDU until it has been decrypted.
         */
        bool      dif_integrity;
};

struct dt_cons * dt_cons_dup(const struct dt_cons * dt_cons);

/* Represents the configuration of the EFCP */
struct efcp_config {
        /* The data transfer constants */
        struct dt_cons * dt_cons;

        struct policy * unknown_flow;
};

/* Represents the configuration of the RMT */
struct rmt_config {
	/* The configuration of the PDU Forwarding policy */
	struct policy * pdu_forwarding;

	/* The configuration of the queue monitor policy */
	struct policy * q_monitor;

	/* The configuration of the max queue policy */
	struct policy * max_q;

	/* The configuration of the scheduling policy */
	struct policy * scheduling;
};

/* Represents a DIF configuration (policies, parameters, etc) */
struct dif_config {
        /* List of configuration entries */
        struct list_head    ipcp_config_entries;

        /* the config of the efcp */
        struct efcp_config * efcp_config;

        /* the config of the rmt */
        struct rmt_config * rmt_config;

        /* The address of the IPC Process*/
        address_t           address;
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

/* Pre-declared, the implementation should define it properly */
struct ipcp_instance_data;

struct ipcp_instance_ops {
        int  (* flow_allocate_request)(struct ipcp_instance_data * data,
                                       struct ipcp_instance *      usr_ipcp,
                                       const struct name *         source,
                                       const struct name *         dest,
                                       const struct flow_spec *    flow_spec,
                                       port_id_t                   id);
        int  (* flow_allocate_response)(struct ipcp_instance_data * data,
                                        struct ipcp_instance *      dest_usr_ipcp,
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

        cep_id_t (* connection_create)(struct ipcp_instance_data * data,
                                       port_id_t                   port_id,
                                       address_t                   source,
                                       address_t                   dest,
                                       qos_id_t                    qos_id,
                                       struct conn_policies *      cp_params);

        int      (* connection_update)(struct ipcp_instance_data * data,
                                       struct ipcp_instance *      user_ipcp,
                                       port_id_t                   port_id,
                                       cep_id_t                    src_id,
                                       cep_id_t                    dst_id);

        int      (* connection_destroy)(struct ipcp_instance_data * data,
                                        cep_id_t                    src_id);

        cep_id_t
        (* connection_create_arrived)(struct ipcp_instance_data * data,
                                      struct ipcp_instance *      user_ipcp,
                                      port_id_t                   port_id,
                                      address_t                   source,
                                      address_t                   dest,
                                      qos_id_t                    qos_id,
                                      cep_id_t                    dst_cep_id,
                                      struct conn_policies *      cp_params);

        int      (* flow_binding_ipcp)(struct ipcp_instance_data * user_data,
                                       port_id_t                   port_id,
                                       struct ipcp_instance *      n1_ipcp);

        int      (* flow_unbinding_ipcp)(struct ipcp_instance_data * user_data,
                                         port_id_t                   port_id);
        int      (* flow_unbinding_user_ipcp)(struct ipcp_instance_data * user_data,
                                              port_id_t                   port_id);

        int      (* sdu_enqueue)(struct ipcp_instance_data * data,
                                 port_id_t                   id,
                                 struct sdu *                sdu);

        /* Takes the ownership of the passed sdu */
        int (* mgmt_sdu_write)(struct ipcp_instance_data * data,
                               address_t                   dst_addr,
                               port_id_t                   port_id,
                               struct sdu *                sdu);

        /* Passes the ownership of the sdu_wpi */
        int (* mgmt_sdu_read)(struct ipcp_instance_data * data,
                              struct sdu_wpi **           sdu_wpi);

        /* Takes the ownership of the passed sdu */
        int (* mgmt_sdu_post)(struct ipcp_instance_data * data,
                              port_id_t                   port_id,
                              struct sdu *                sdu);

        int (* pft_add)(struct ipcp_instance_data * data,
                        address_t                   address,
                        qos_id_t                    qos_id,
                        port_id_t *                 ports,
                        size_t                      size);

        int (* pft_remove)(struct ipcp_instance_data * data,
                           address_t                   address,
                           qos_id_t                    qos_id,
                           port_id_t *                 ports,
                           size_t                      size);

        int (* pft_dump)(struct ipcp_instance_data * data,
                         struct list_head *          entries);

        int (* pft_flush)(struct ipcp_instance_data * data);

        int (* query_rib)(struct ipcp_instance_data * data,
                          struct list_head *          entries,
                          const string_t *            object_class,
                          const string_t *            object_name,
                          uint64_t                    object_instance,
                          uint32_t                    scope,
                          const string_t *            filter);

        const struct name * (* ipcp_name)(struct ipcp_instance_data * data);

        int (* set_policy_set_param)(struct ipcp_instance_data * data,
                                     const string_t * path,
                                     const string_t * param_name,
                                     const string_t * param_value);
        int (* select_policy_set)(struct ipcp_instance_data * data,
                                  const string_t * path,
                                  const string_t * ps_name);

        int (* enable_write)(struct ipcp_instance_data * data, port_id_t id);
        int (* disable_write)(struct ipcp_instance_data * data, port_id_t id);
};

/* FIXME: Should work on struct ipcp_instance, not on ipcp_instance_ops */
bool ipcp_instance_is_shim(struct ipcp_instance_ops * ops);
bool ipcp_instance_is_normal(struct ipcp_instance_ops * ops);
bool ipcp_instance_is_ok(struct ipcp_instance_ops * ops);

struct ipcp_factory;

/* FIXME: Hide this data structure */
struct ipcp_instance {
        struct kobject              kobj;

        /* FIXME: Should be hidden and not fixed up in KIPCM ... */
        struct ipcp_factory *       factory; /* The parent factory */

        struct ipcp_instance_data * data;
        struct ipcp_instance_ops *  ops;
};

#endif
