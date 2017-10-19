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

#include "rds/robjects.h"
#include "buffer.h"
#include "du.h"

struct du;
struct dtp_config;
struct dtcp_config;

struct ipcp_instance;

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
                                      const struct name *           source,
				      const struct name *           daf_name);
        int  (* application_unregister)(struct ipcp_instance_data * data,
                                        const struct name *         source);

        int  (* assign_to_dif)(struct ipcp_instance_data * data,
        		       const struct name * dif_name,
			       const string_t * type,
                               struct dif_config * config);

        int  (* update_dif_config)(struct ipcp_instance_data * data,
                                   const struct dif_config *   configuration);

        /* Takes the ownership of the passed SDU */
        int  (* du_write)(struct ipcp_instance_data * data,
                          port_id_t                   id,
                          struct du *                 du,
                          bool                        blocking);

        cep_id_t (* connection_create)(struct ipcp_instance_data * data,
        			       struct ipcp_instance *      user_ipcp,
                                       port_id_t                   port_id,
                                       address_t                   source,
                                       address_t                   dest,
                                       qos_id_t                    qos_id,
                                       struct dtp_config *         dtp_config,
                                       struct dtcp_config *        dtcp_config);

        int      (* connection_update)(struct ipcp_instance_data * data,
                                       port_id_t                   port_id,
                                       cep_id_t                    src_id,
                                       cep_id_t                    dst_id);

        int      (* connection_modify)(struct ipcp_instance_data * data,
        			       cep_id_t			   src_cep_id,
				       address_t		   src_address,
				       address_t		   dst_address);

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
                                      struct dtp_config *         dtp_config,
                                      struct dtcp_config *        dtcp_config);

        int      (* flow_prebind)(struct ipcp_instance_data * data,
                                  struct ipcp_instance *      user_ipcp,
                                  port_id_t                   port_id);

        int      (* flow_binding_ipcp)(struct ipcp_instance_data * user_data,
                                       port_id_t                   port_id,
                                       struct ipcp_instance *      n1_ipcp);

        int      (* flow_unbinding_ipcp)(struct ipcp_instance_data * user_data,
                                         port_id_t                   port_id);
        int      (* flow_unbinding_user_ipcp)(struct ipcp_instance_data * user_data,
                                              port_id_t                   port_id);
	int	(* nm1_flow_state_change)(struct ipcp_instance_data *data,
					  port_id_t port_id, bool up);

        int      (* du_enqueue)(struct ipcp_instance_data * data,
                                port_id_t                   id,
                                struct du *                 du);

        /* Takes the ownership of the passed sdu */
        int (* mgmt_du_write)(struct ipcp_instance_data * data,
                              port_id_t                   port_id,
                              struct du *                 du);

        /* Takes the ownership of the passed sdu */
        int (* mgmt_du_post)(struct ipcp_instance_data * data,
                             port_id_t                   port_id,
                             struct du *                 du);

        int (* pff_add)(struct ipcp_instance_data * data,
			struct mod_pff_entry	  * entry);

        int (* pff_remove)(struct ipcp_instance_data * data,
			   struct mod_pff_entry      * entry);

        int (* pff_dump)(struct ipcp_instance_data * data,
                         struct list_head *          entries);

        int (* pff_flush)(struct ipcp_instance_data * data);

        int (* pff_modify)(struct ipcp_instance_data * data,
                           struct list_head * entries);

        int (* query_rib)(struct ipcp_instance_data * data,
                          struct list_head *          entries,
                          const string_t *            object_class,
                          const string_t *            object_name,
                          uint64_t                    object_instance,
                          uint32_t                    scope,
                          const string_t *            filter);

        const struct name * (* ipcp_name)(struct ipcp_instance_data * data);
        const struct name * (* dif_name)(struct ipcp_instance_data * data);
        ipc_process_id_t (* ipcp_id)(struct ipcp_instance_data * data);

        int (* set_policy_set_param)(struct ipcp_instance_data * data,
                                     const string_t * path,
                                     const string_t * param_name,
                                     const string_t * param_value);
        int (* select_policy_set)(struct ipcp_instance_data * data,
                                  const string_t * path,
                                  const string_t * ps_name);

        int (* update_crypto_state)(struct ipcp_instance_data * data,
        			    struct sdup_crypto_state * state,
        		            port_id_t 	   port_id);

        int (* enable_write)(struct ipcp_instance_data * data, port_id_t id);
        int (* disable_write)(struct ipcp_instance_data * data, port_id_t id);

        /*
         * Start using new address after first timeout, deprecate old
         * address after second timeout
         */
        int (* address_change)(struct ipcp_instance_data * data,
         		       address_t new_address,
 			       address_t old_address,
 			       timeout_t use_new_address_t,
 			       timeout_t deprecate_old_address_t);

        /*
         * The maximum size of SDUs that this IPCP will accept
         */
        size_t (* max_sdu_size)(struct ipcp_instance_data * data);
};

/* FIXME: Should work on struct ipcp_instance, not on ipcp_instance_ops */
bool ipcp_instance_is_shim(struct ipcp_instance_ops * ops);
bool ipcp_instance_is_normal(struct ipcp_instance_ops * ops);
bool ipcp_instance_is_ok(struct ipcp_instance_ops * ops);

struct ipcp_factory;

/* FIXME: Hide this data structure */
struct ipcp_instance {
        struct robject              robj;

        /* FIXME: Should be hidden and not fixed up in KIPCM ... */
        struct ipcp_factory *       factory; /* The parent factory */

        struct ipcp_instance_data * data;
        struct ipcp_instance_ops *  ops;
};

#endif
