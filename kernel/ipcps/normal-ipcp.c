/*
 *  Normal IPC Process
 *
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

#include <linux/module.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/version.h>

#define IPCP_NAME   "normal-ipc"

#define RINA_PREFIX IPCP_NAME

#include "logs.h"
#include "common.h"
#include "debug.h"
#include "efcp-str.h"
#include "utils.h"
#include "kipcm.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"
#include "kfa.h"
#include "efcp.h"
#include "dtp.h"
#include "dtcp.h"
#include "rmt.h"
#include "du.h"
#include "sdup.h"
#include "efcp-utils.h"
#include "rds/rtimer.h"
#include "irati/kernel-msg.h"

/*  FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

/* FIXME: To be solved properly */
static struct workqueue_struct * mgmt_sdu_wq;

struct mgmt_du_work_data {
        struct du *       du;
        ipc_process_id_t  ipcp_id;
        port_id_t	  port_id;
        irati_msg_port_t  irati_port_id;
};

enum mgmt_state {
        MGMT_DATA_READY,
        MGMT_DATA_DESTROYED
};

struct ipcp_instance_data {
        /* FIXME: add missing needed attributes */
        ipc_process_id_t        id;
        irati_msg_port_t        irati_port;
        struct name             name;
        struct name             dif_name;
        struct list_head        flows;
        /*  FIXME: Remove it as soon as the kipcm_kfa gets removed*/
        struct kfa *            kfa;
        struct efcp_container * efcpc;
        struct rmt *            rmt;
        struct sdup *           sdup;
        address_t               address;
        address_t		old_address;
        spinlock_t              lock;
        struct list_head        list;
        /* Timers required for the address change procedure */
        struct {
        	struct timer_list use_naddress;
                struct timer_list kill_oaddress;
        } timers;
};

enum normal_flow_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_PENDING,
        PORT_STATE_ALLOCATED,
        PORT_STATE_DEALLOCATED,
        PORT_STATE_DISABLED
};

struct cep_ids_entry {
        cep_id_t         cep_id;
        struct list_head list;
};

struct normal_flow {
        port_id_t              port_id;
        cep_id_t               active;
        struct list_head       cep_ids_list;
        enum normal_flow_state state;
        struct ipcp_instance * user_ipcp;
        struct list_head       list;
};

static ssize_t normal_ipcp_attr_show(struct robject *        robj,
                         	     struct robj_attribute * attr,
                                     char *                  buf)
{
	struct ipcp_instance * instance;

	instance = container_of(robj, struct ipcp_instance, robj);
	if (!instance || !instance->data)
		return 0;

	if (strcmp(robject_attr_name(attr), "name") == 0)
		return sprintf(buf, "%s\n",
			name_tostring(&instance->data->name));
	if (strcmp(robject_attr_name(attr), "dif") == 0)
		return sprintf(buf, "%s\n",
			name_tostring(&instance->data->dif_name));
	if (strcmp(robject_attr_name(attr), "address") == 0)
		return sprintf(buf, "%u\n", instance->data->address);
	if (strcmp(robject_attr_name(attr), "type") == 0)
		return sprintf(buf, "normal\n");

	return 0;
}
RINA_SYSFS_OPS(normal_ipcp);
RINA_ATTRS(normal_ipcp, name, type, dif, address);
RINA_KTYPE(normal_ipcp);

static struct normal_flow * find_flow(struct ipcp_instance_data * data,
                                      port_id_t                   port_id)
{
        struct normal_flow * flow;

        list_for_each_entry(flow, &(data->flows), list) {
                if (flow->port_id == port_id)
                        return flow;
        }

        return NULL;
}

struct ipcp_factory_data {
        u32    nl_port;
        struct list_head instances;
};

static struct ipcp_factory_data normal_data;

static int normal_init(struct ipcp_factory_data * data)
{
        ASSERT(data);

        bzero(&normal_data, sizeof(normal_data));
        INIT_LIST_HEAD(&data->instances);

        return 0;
}

static int normal_fini(struct ipcp_factory_data * data)
{
        ASSERT(data);
        ASSERT(list_empty(&data->instances));
        return 0;
}

static int normal_du_enqueue(struct ipcp_instance_data * data,
                             port_id_t                   id,
                             struct du *                du)
{
        if (rmt_receive(data->rmt, du, id)) {
                LOG_ERR("Could not enqueue SDU into the RMT");
                return -1;
        }

        return 0;
}

static int normal_du_write(struct ipcp_instance_data * data,
                           port_id_t                   id,
                           struct du *                 du,
                           bool                        blocking)
{
        struct normal_flow * flow;

        spin_lock_bh(&data->lock);
        flow = find_flow(data, id);
        if (!flow || flow->state != PORT_STATE_ALLOCATED) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("Write: There is no flow bound to this port_id: %d",
                        id);
                du_destroy(du);
                return -1;
        }
        spin_unlock_bh(&data->lock);

        if (efcp_container_write(data->efcpc, flow->active, du)) {
                LOG_ERR("Could not send sdu to EFCP Container");
                return -1;
        }

        return 0;
}

static struct ipcp_factory * normal = NULL;

static struct ipcp_instance_data *
find_instance(struct ipcp_factory_data * data,
              ipc_process_id_t           id)
{
        struct ipcp_instance_data * pos;

        list_for_each_entry(pos, &(data->instances), list) {
                if (pos->id == id) {
                        return pos;
                }
        }

        return NULL;
}

static int normal_flow_prebind(struct ipcp_instance_data * data,
                               struct ipcp_instance *      user_ipcp,
                               port_id_t                   port_id)
{
        struct normal_flow * flow;

        if (!data || !is_port_id_ok(port_id)) {
                LOG_ERR("Wrong input parameters...");
                return -1;
        }

        flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
        if (!flow) {
                LOG_ERR("Could not create a flow in normal-ipcp to pre-bind");
                return -1;
        }
        if (!user_ipcp)
                user_ipcp = kfa_ipcp_instance(data->kfa);
        flow->user_ipcp = user_ipcp;
        flow->port_id = port_id;
        INIT_LIST_HEAD(&flow->list);
        INIT_LIST_HEAD(&flow->cep_ids_list);
        flow->state = PORT_STATE_PENDING;
        spin_lock_bh(&data->lock);
        list_add(&flow->list, &data->flows);
        spin_unlock_bh(&data->lock);

        return 0;
}

static
cep_id_t connection_create_request(struct ipcp_instance_data * data,
				   struct ipcp_instance *      user_ipcp,
                                   port_id_t                   port_id,
                                   address_t                   source,
                                   address_t                   dest,
                                   qos_id_t                    qos_id,
                                   struct dtp_config *         dtp_cfg,
                                   struct dtcp_config *        dtcp_cfg)
{
        cep_id_t               cep_id;
        struct normal_flow *   flow;
        struct cep_ids_entry * cep_entry;
        struct ipcp_instance * ipcp;

        if (!user_ipcp)
                return cep_id_bad();

        cep_id = efcp_connection_create(data->efcpc, user_ipcp, source, dest,
                                        port_id, qos_id,
                                        cep_id_bad(), cep_id_bad(),
                                        dtp_cfg, dtcp_cfg);
        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Failed EFCP connection creation");
                return cep_id_bad();
        }

        cep_entry = rkzalloc(sizeof(*cep_entry), GFP_KERNEL);
        if (!cep_entry) {
                LOG_ERR("Could not create a cep_id entry, bailing out");
                efcp_connection_destroy(data->efcpc, cep_id);
                return cep_id_bad();
        }
        INIT_LIST_HEAD(&cep_entry->list);
        cep_entry->cep_id = cep_id;

        ipcp = kipcm_find_ipcp(default_kipcm, data->id);
        if (!ipcp) {
                LOG_ERR("KIPCM could not retrieve this IPCP");
                efcp_connection_destroy(data->efcpc, cep_id);
                return cep_id_bad();
        }

        ASSERT(user_ipcp->ops);
        ASSERT(user_ipcp->ops->flow_binding_ipcp);
        spin_lock_bh(&data->lock);
        flow = find_flow(data, port_id);
        if (!flow) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("Could not retrieve normal flow to create connection");
                efcp_connection_destroy(data->efcpc, cep_id);
                return cep_id_bad();
        }

        if (user_ipcp->ops->flow_binding_ipcp(user_ipcp->data,
                                              port_id,
                                              ipcp)) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("Could not bind flow with user_ipcp");
                efcp_connection_destroy(data->efcpc, cep_id);
                return cep_id_bad();
        }

        list_add(&cep_entry->list, &flow->cep_ids_list);
        flow->active = cep_id;
        flow->state = PORT_STATE_PENDING;

        spin_unlock_bh(&data->lock);

        return cep_id;
}

static int connection_update_request(struct ipcp_instance_data * data,
                                     port_id_t                   port_id,
                                     cep_id_t                    src_cep_id,
                                     cep_id_t                    dst_cep_id)
{
        struct normal_flow *   flow;
        struct ipcp_instance * n1_ipcp;

        n1_ipcp = kipcm_find_ipcp(default_kipcm, data->id);
        if (!n1_ipcp) {
                LOG_ERR("KIPCM cannot retrieve this IPCP");
                efcp_connection_destroy(data->efcpc, src_cep_id);
                return -1;
        }
        if (efcp_connection_update(data->efcpc,
                                   src_cep_id,
                                   dst_cep_id))
                return -1;

        ASSERT(user_ipcp->ops);
        ASSERT(user_ipcp->ops->flow_binding_ipcp);

        spin_lock_bh(&data->lock);

        flow = find_flow(data, port_id);
        if (!flow) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("The flow with port-id %d is not pending, "
                        "cannot commit it", port_id);
                efcp_connection_destroy(data->efcpc, src_cep_id);
                return -1;
        }
        if (flow->state != PORT_STATE_PENDING) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("Flow on port-id %d already committed", port_id);
                efcp_connection_destroy(data->efcpc, src_cep_id);
                return -1;
        }
        flow->state = PORT_STATE_ALLOCATED;

        spin_unlock_bh(&data->lock);

        LOG_DBG("Flow bound to port-id %d", port_id);
        return 0;
}

static int connection_modify_request(struct ipcp_instance_data * data,
				     cep_id_t			 src_cep_id,
			     	     address_t		   	 src_address,
				     address_t		   	 dst_address)
{
	return efcp_connection_modify(data->efcpc, src_cep_id,
				      src_address, dst_address);
}

static struct normal_flow * find_flow_cepid(struct ipcp_instance_data * data,
                                            cep_id_t                    id)
{
        struct normal_flow * pos;

        list_for_each_entry(pos, &(data->flows), list) {
                if (pos->active == id) {
                        return pos;
                }
        }
        return NULL;
}

static int remove_cep_id_from_flow(struct normal_flow * flow,
                                   cep_id_t             id)
{
        struct cep_ids_entry *pos, *next;

        list_for_each_entry_safe(pos, next, &(flow->cep_ids_list), list) {
                if (pos->cep_id == id) {
                        list_del(&pos->list);
                        rkfree(pos);
                        return 0;
                }
        }
        return -1;
}

static int ipcp_flow_binding(struct ipcp_instance_data * user_data,
                             port_id_t                   pid,
                             struct ipcp_instance *      n1_ipcp)
{ return rmt_n1port_bind(user_data->rmt, pid, n1_ipcp); }

static int normal_flow_unbinding_ipcp(struct ipcp_instance_data * user_data,
                                      port_id_t                   pid)
{
        if (rmt_n1port_unbind(user_data->rmt, pid))
                return -1;

        return 0;
}

static int normal_flow_unbinding_user_ipcp(struct ipcp_instance_data * data,
                                           port_id_t                   pid)
{
        struct normal_flow * flow;

        ASSERT(data);

        spin_lock_bh(&data->lock);
        flow = find_flow(data, pid);
        if (!flow || !is_cep_id_ok(flow->active)) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("Could not find flow with port %d to unbind user IPCP",
                        pid);
                return -1;
        }

        if (flow->user_ipcp) {
        	flow->user_ipcp = NULL;
        }
        spin_unlock_bh(&data->lock);

        if (efcp_container_unbind_user_ipcp(data->efcpc, flow->active)){
                spin_unlock_bh(&data->lock);
                return -1;
        }

        return 0;
}

static int normal_nm1_flow_state_change(struct ipcp_instance_data * data,
					port_id_t		    pid,
					bool			    up)
{
	LOG_INFO("N-1 flow with pid %d went %s", pid, (up ? "up" : "down"));

	if (rmt_pff_port_state_change(data->rmt, pid, up)) {
		LOG_ERR("rmt_port_state_change() failed");
	}

	return 0;
}

static int connection_destroy_request(struct ipcp_instance_data * data,
                                      cep_id_t                    src_cep_id)
{
        struct normal_flow * flow;

        if (!data) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }
        if (efcp_connection_destroy(data->efcpc, src_cep_id))
                LOG_ERR("Could not destroy EFCP instance: %d", src_cep_id);

        spin_lock_bh(&data->lock);
        if (!(&data->flows)) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("Could not destroy EFCP instance: %d", src_cep_id);
                return -1;
        }
        flow = find_flow_cepid(data, src_cep_id);
        if (!flow) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("Could not retrieve flow by cep_id :%d", src_cep_id);
                return -1;
        }
        if (remove_cep_id_from_flow(flow, src_cep_id))
                LOG_ERR("Could not remove cep_id: %d", src_cep_id);

        if (list_empty(&flow->cep_ids_list)) {
                list_del(&flow->list);
                rkfree(flow);
        }
        spin_unlock_bh(&data->lock);

        return 0;
}

static cep_id_t
connection_create_arrived(struct ipcp_instance_data * data,
                          struct ipcp_instance *      user_ipcp,
                          port_id_t                   port_id,
                          address_t                   source,
                          address_t                   dest,
                          qos_id_t                    qos_id,
                          cep_id_t                    dst_cep_id,
                          struct dtp_config *         dtp_cfg,
                          struct dtcp_config *        dtcp_cfg)
{
        cep_id_t               cep_id;
        struct normal_flow *   flow;
        struct cep_ids_entry * cep_entry;
        struct ipcp_instance * ipcp;

        if (!user_ipcp)
                return cep_id_bad();

        cep_id = efcp_connection_create(data->efcpc, user_ipcp, source, dest,
                                        port_id, qos_id,
                                        cep_id_bad(), dst_cep_id,
                                        dtp_cfg, dtcp_cfg);
        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Failed EFCP connection creation");
                return cep_id_bad();
        }
        LOG_DBG("Cep_id allocated for the arrived connection request: %d",
                cep_id);

        cep_entry = rkzalloc(sizeof(*cep_entry), GFP_KERNEL);
        if (!cep_entry) {
                LOG_ERR("Could not create a cep_id entry, bailing out");
                efcp_connection_destroy(data->efcpc, cep_id);
                return cep_id_bad();
        }
        INIT_LIST_HEAD(&cep_entry->list);
        cep_entry->cep_id = cep_id;

        ipcp = kipcm_find_ipcp(default_kipcm, data->id);
        if (!ipcp) {
                LOG_ERR("KIPCM could not retrieve this IPCP");
                efcp_connection_destroy(data->efcpc, cep_id);
                return cep_id_bad();
        }

        ASSERT(user_ipcp->ops);
        ASSERT(user_ipcp->ops->flow_binding_ipcp);
        spin_lock_bh(&data->lock);
        flow = find_flow(data, port_id);
        if (!flow) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("Could not create a flow in normal-ipcp");
                efcp_connection_destroy(data->efcpc, cep_id);
                return cep_id_bad();
        }
        if (user_ipcp->ops->flow_binding_ipcp(user_ipcp->data,
                                              port_id,
                                              ipcp)) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("Could not bind flow with user_ipcp");
                efcp_connection_destroy(data->efcpc, cep_id);
                return cep_id_bad();
        }
        list_add(&cep_entry->list, &flow->cep_ids_list);
        flow->active = cep_id;
        flow->state = PORT_STATE_ALLOCATED;
        spin_unlock_bh(&data->lock);

        return cep_id;
}

static int remove_all_cepid(struct ipcp_instance_data * data,
                            struct normal_flow *        flow)
{
        struct cep_ids_entry *pos, *next;

        ASSERT(data);

        ASSERT(flow);
        ASSERT(&flow->cep_ids_list);

        list_for_each_entry_safe(pos, next, &flow->cep_ids_list, list) {
                efcp_connection_destroy(data->efcpc, pos->cep_id);
                list_del(&pos->list);
                rkfree(pos);
        }

        return 0;
}

static int normal_deallocate(struct ipcp_instance_data * data,
                             port_id_t                   port_id)
{
        struct normal_flow *   flow;
        const struct name *    user_ipcp_name;
        enum normal_flow_state state;

        if (!data) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        spin_lock_bh(&data->lock);
        flow = find_flow(data, port_id);
        if (!flow) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("Could not find flow %d to deallocate", port_id);
                return -1;
        }

        if (flow->user_ipcp && flow->user_ipcp->ops->ipcp_name &&
            flow->user_ipcp->data) {
        	user_ipcp_name =
        		flow->user_ipcp->ops->ipcp_name(flow->user_ipcp->data);
        } else {
        	user_ipcp_name = NULL;
        }
        state          = flow->state;
        flow->state    = PORT_STATE_DEALLOCATED;
        list_del(&flow->list);
        spin_unlock_bh(&data->lock);

        if (state == PORT_STATE_PENDING && flow->user_ipcp) {
                flow->user_ipcp->ops->flow_unbinding_ipcp(flow->user_ipcp->data,
                                                          port_id);
        } else {
                remove_all_cepid(data, flow);
        }

        rkfree(flow);

        /*NOTE: KFA will take care of releasing the port */
        if (user_ipcp_name)
                kfa_port_id_release(data->kfa, port_id);

        return 0;
}

static int enable_write(struct ipcp_instance_data * data,
                        port_id_t                   port_id)
{
        if (rmt_enable_port_id(data->rmt, port_id))
                return -1;

        return 0;
}

static int disable_write(struct ipcp_instance_data * data,
                         port_id_t                   port_id) {
        if (rmt_disable_port_id(data->rmt, port_id))
                return -1;

        return 0;
}

static int normal_assign_to_dif(struct ipcp_instance_data * data,
		                const struct name * dif_name,
				const string_t * type,
				struct dif_config * config)
{
        struct efcp_config * efcp_config;
        struct secman_config * sm_config;
        struct rmt_config *  rmt_config;

        if (name_cpy(dif_name, &data->dif_name)) {
                LOG_ERR("%s: name_cpy() failed", __func__);
                return -1;
        }

        data->address  = config->address;

        efcp_config = config->efcp_config;
        config->efcp_config = 0;

        if (!efcp_config) {
                LOG_ERR("No EFCP configuration in the dif_info");
                return -1;
        }

        if (!efcp_config->dt_cons) {
                LOG_ERR("Configuration constants for the DIF are bogus...");
                efcp_config_free(efcp_config);
                return -1;
        }

        efcp_container_config_set(data->efcpc, efcp_config);

        rmt_config = config->rmt_config;
        config->rmt_config = 0;
        if (!rmt_config) {
        	LOG_ERR("No RMT configuration in the dif_info");
        	return -1;
        }

        if (rmt_address_add(data->rmt, data->address)) {
		LOG_ERR("Could not set local Address to RMT");
                return -1;
	}

        if (rmt_config_set(data-> rmt, rmt_config)) {
                LOG_ERR("Could not set RMT conf");
		return -1;
        }

        sm_config = config->secman_config;
        config->secman_config = 0;
	if (!sm_config) {
		LOG_INFO("No SDU protection config specified, using default");
		sm_config = secman_config_create();
		sm_config->default_profile = auth_sdup_profile_create();
	}
	if (sdup_config_set(data->sdup, sm_config)) {
                LOG_ERR("Could not set SDUP conf");
		return -1;
	}
	if (sdup_dt_cons_set(data->sdup, dt_cons_dup(efcp_config->dt_cons))) {
                LOG_ERR("Could not set dt_cons in SDUP");
		return -1;
	}

        return 0;
}

static int normal_mgmt_du_write(struct ipcp_instance_data * data,
                                port_id_t                   port_id,
                                struct du *                 du)
{
	ssize_t sbytes;

        LOG_DBG("Passing SDU to be written to N-1 port %d "
                "from IPC Process %d", port_id, data->id);

        if (!du) {
                LOG_ERR("No data passed, bailing out");
                return -1;
        }

        du->cfg = data->efcpc->config;
        sbytes = du_len(du);

	if (du_encap(du, PDU_TYPE_MGMT)){
		LOG_ERR("Could not encap Mgmt PDU");
		du_destroy(du);
		return -1;
	}

        /* FIXME: qos_id is set to 1 since 0 is QOS_ID_WRONG */
        if (pci_format(&du->pci,
                       0,
                       0,
                       data->address,
                       0,
                       0,
                       1,
                       0,
		       du->pci.len + sbytes,
                       PDU_TYPE_MGMT)) {
        	LOG_ERR("Problems formatting PCI");
                du_destroy(du);
                return -1;
        }

        /*
         * NOTE:
         *   Decide on how to deliver to the RMT depending on
         *   port_id or dst_addr
         */
        if (port_id) {
                if (rmt_send_port_id(data->rmt,
                                     port_id,
                                     du)) {
                        LOG_ERR("Could not send to RMT (using port_id)");
                        return -1;
                }
        } else {
                LOG_ERR("Could not send to RMT: no port_id nor dst_addr");
                du_destroy(du);
                return -1;
        }

        return 0;
}

static int mgmt_sdu_notif_worker(void * o)
{
        struct mgmt_du_work_data * data;
        struct irati_kmsg_ipcp_mgmt_sdu msg;
        unsigned char * sdu_data;

        LOG_DBG("Worker waking up, going to send a Mgmt SDU to IPCP Daemon");

	ASSERT(o);

        data = (struct mgmt_du_work_data *) o;

	msg.msg_type = RINA_C_IPCP_MANAGEMENT_SDU_READ_NOTIF;
	msg.port_id = data->port_id;
	msg.src_ipcp_id = data->ipcp_id;
	msg.sdu = buffer_create(du_len(data->du));
	if (!msg.sdu) {
		LOG_ERR("Problems creating buffer");
		du_destroy(data->du);
		rkfree(data);
		return 0;
	}

	sdu_data = du_buffer(data->du);
	memcpy(msg.sdu->data, sdu_data, msg.sdu->size);

	if (irati_ctrl_dev_snd_resp_msg(data->irati_port_id,
					(struct irati_msg_base *) &msg)) {
		LOG_ERR("Could not send flow_result_msg");
	}

	buffer_destroy(msg.sdu);
	du_destroy(data->du);
	rkfree(data);

        LOG_DBG("Worker ends...");

        return 0;
}

static int normal_mgmt_du_post(struct ipcp_instance_data * data,
                               port_id_t                   port_id,
                               struct du *                 du)
{
        struct mgmt_du_work_data  * wdata;
        struct rwq_work_item      * item;

	if (!data) {
		LOG_ERR("Bogus instance passed");
		du_destroy(du);
		return -1;
	}

	if (!is_port_id_ok(port_id)) {
		LOG_ERR("Wrong port id");
		du_destroy(du);
		return -1;
	}
	if (!is_du_ok(du)) {
		LOG_ERR("Bogus management SDU");
		du_destroy(du);
		return -1;
	}

        wdata = rkzalloc(sizeof(* wdata), GFP_ATOMIC);
        wdata->du  = du;
        wdata->port_id = port_id;
        wdata->irati_port_id = data->irati_port;
        wdata->ipcp_id = data->id;
        item  = rwq_work_create_ni(mgmt_sdu_notif_worker, wdata);

        rwq_work_post(mgmt_sdu_wq, item);

        return 0;
}

static int normal_pff_add(struct ipcp_instance_data * data,
			  struct mod_pff_entry *      entry)

{
        ASSERT(data);

        return rmt_pff_add(data->rmt, entry);
}

static int normal_pff_remove(struct ipcp_instance_data * data,
			     struct mod_pff_entry *      entry)
{
        ASSERT(data);

        return rmt_pff_remove(data->rmt, entry);
}

static int normal_pff_dump(struct ipcp_instance_data * data,
                           struct list_head *          entries)
{
        ASSERT(data);

        return rmt_pff_dump(data->rmt,
                            entries);
}

static int normal_pff_flush(struct ipcp_instance_data * data)
{
        ASSERT(data);

        return rmt_pff_flush(data->rmt);
}

static int normal_pff_modify(struct ipcp_instance_data * data,
                   	     struct list_head * entries)
{
	ASSERT(data);

	return rmt_pff_modify(data->rmt,
			      entries);
}

static const struct name * normal_ipcp_name(struct ipcp_instance_data * data)
{
        ASSERT(data);
        ASSERT(name_is_ok(&data->name));

        return &data->name;
}

static const struct name * normal_dif_name(struct ipcp_instance_data * data)
{
        ASSERT(data);
        ASSERT(name_is_ok(&data->dif_name));

        return &data->dif_name;
}

static size_t normal_max_sdu_size(struct ipcp_instance_data * data)
{
        ASSERT(data);
        if (!data->efcpc || !data->efcpc->config)
        	return 0;

        return data->efcpc->config->dt_cons->max_sdu_size;
}

ipc_process_id_t normal_ipcp_id(struct ipcp_instance_data * data)
{
	ASSERT(data);
        return data->id;
}

typedef const string_t *const_string;

/* Helper function to parse the component id path for EFCP container. */
static struct efcp *
efcp_container_parse_component_id(struct ipcp_instance_data * data,
				  bool assume_port_id,
				  struct efcp_container * container,
                                  const_string * path)
{
	struct normal_flow *flow;
        struct efcp * efcp;
        int xid;
        size_t cmplen;
        size_t offset;
        char numbuf[8];
        int ret;

        if (!*path) {
                LOG_ERR("NULL path");
                return NULL;
        }

        ps_factory_parse_component_id(*path, &cmplen, &offset);
        if (cmplen > sizeof(numbuf)-1) {
                LOG_ERR("Invalid cep-id' %s'", *path);
                return NULL;
        }

        memcpy(numbuf, *path, cmplen);
        numbuf[cmplen] = '\0';
        ret = kstrtoint(numbuf, 10, &xid);
        if (ret) {
                LOG_ERR("Invalid cep-id '%s'", *path);
                return NULL;
        }

	if (assume_port_id) {
		/* Interpret xid as a port-id rather than a cep-id. */
		flow = find_flow(data, xid);
		if (!flow) {
			LOG_ERR("No flow with port-id %d", xid);
			return NULL;
		}
		xid = flow->active;
	}

        efcp = efcp_imap_find(efcp_container_get_instances(container), xid);
        if (!efcp) {
                LOG_ERR("No connection with cep-id %d", xid);
                return NULL;
        }

        *path += offset;

        return efcp;

}

static int efcp_select_policy_set(struct efcp * efcp,
                                  const string_t * path,
                                  const string_t * ps_name)
{
        size_t cmplen;
        size_t offset;

        ps_factory_parse_component_id(path, &cmplen, &offset);

        if (cmplen && strncmp(path, "dtp", cmplen) == 0) {
                return dtp_select_policy_set(efcp->dtp, path + offset,
                                             ps_name);
        } else if (cmplen && strncmp(path, "dtcp", cmplen) == 0
        		&& efcp->dtp->dtcp) {
                return dtcp_select_policy_set(efcp->dtp->dtcp,
                			      path + offset,
                                              ps_name);
        }

        /* Currently there are no policy sets specified for EFCP (strictly
         * speaking). */
        LOG_ERR("The selected component does not exist");

        return -1;
}

static int efcp_container_select_policy_set(struct efcp_container * container,
					    const string_t * path,
					    const string_t * ps_name,
					    struct ipcp_instance_data *data)
{
        struct efcp * efcp;
        const string_t * new_path = path;

        efcp = efcp_container_parse_component_id(data, true, container,
						 &new_path);
        if (!efcp) {
                return -1;
        }

        return efcp_select_policy_set(efcp, new_path, ps_name);
}

static int normal_select_policy_set(struct ipcp_instance_data *data,
                                    const string_t *path,
                                    const string_t *ps_name)
{
        size_t cmplen;
        size_t offset;

        ps_factory_parse_component_id(path, &cmplen, &offset);

        if (cmplen && strncmp(path, "rmt", cmplen) == 0) {
                return rmt_select_policy_set(data->rmt, path + offset,
                                             ps_name);
        } else if (cmplen && strncmp(path, "efcp", cmplen) == 0) {
                return efcp_container_select_policy_set(data->efcpc,
                                                path + offset, ps_name, data);
        } else {
                LOG_ERR("The selected component does not exist");
                return -1;
        }

        return -1;
}

static int efcp_set_policy_set_param(struct efcp * efcp,
                                     const char * path,
                                     const char * name,
                                     const char * value)
{
        size_t cmplen;
        size_t offset;

        ps_factory_parse_component_id(path, &cmplen, &offset);

        if (strncmp(path, "dtp", cmplen) == 0) {
                return dtp_set_policy_set_param(efcp->dtp,
                                        path + offset, name, value);
        } else if (strncmp(path, "dtcp", cmplen) == 0 && efcp->dtp->dtcp) {
                return dtcp_set_policy_set_param(efcp->dtp->dtcp,
                                        path + offset, name, value);
        }

        /* Currently there are no parametric policies specified for EFCP
         * (strictly speaking). */
        LOG_ERR("No parametric policies for this EFCP component");

        return -1;
}

static int efcp_container_set_policy_set_param(struct efcp_container * container,
                                               const char * path, const char * name,
					       const char * value,
					       struct ipcp_instance_data *data)
{

        struct efcp * efcp;
        const string_t * new_path = path;

        efcp = efcp_container_parse_component_id(data, true, container, &new_path);
        if (!efcp) {
                return -1;
        }

        return efcp_set_policy_set_param(efcp, new_path, name, value);
}

static int normal_set_policy_set_param(struct ipcp_instance_data * data,
                                       const string_t *path,
                                       const string_t *param_name,
                                       const string_t *param_value)
{
        size_t cmplen;
        size_t offset;

        ps_factory_parse_component_id(path, &cmplen, &offset);

        if (strncmp(path, "rmt", cmplen) == 0) {
                return rmt_set_policy_set_param(data->rmt, path + offset,
                                                param_name, param_value);
        } else if (strncmp(path, "efcp", cmplen) == 0) {
                return efcp_container_set_policy_set_param(data->efcpc,
                                path + offset, param_name, param_value, data);
        } else {
                LOG_ERR("The selected component does not exist");
                return -1;
        }

        return -1;
}

int normal_update_crypto_state(struct ipcp_instance_data * data,
			       struct sdup_crypto_state * state,
		               port_id_t 	      port_id)
{
	return sdup_update_crypto_state(data->sdup,
				        state,
				        port_id);
}

int normal_address_change(struct ipcp_instance_data * data,
			  address_t new_address,
			  address_t old_address,
			  timeout_t use_new_address_t,
			  timeout_t deprecate_old_address_t)
{
	LOG_INFO("Need to change address from %u to %u",
		 old_address, new_address);

	if (!data) {
		LOG_ERR("Bogus data passed");
		return -1;
	}

	spin_lock_bh(&data->lock);
	data->old_address = old_address;
	data->address = new_address;
	spin_unlock_bh(&data->lock);

	rmt_address_add(data->rmt, new_address);

	/* Set timer to start advertising new address in EFCP connections
	and MGMT PDUs (give time to routing updates to converge) */;
	rtimer_restart(&data->timers.use_naddress, use_new_address_t);
	/* Set timer to stop accepting old address in RMT */
	rtimer_restart(&data->timers.kill_oaddress, deprecate_old_address_t);

	return 0;
}

/* Runs the New Address Timer function */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
static void tf_use_naddress(void * data)
#else
static void tf_use_naddress(struct timer_list * tl)
#endif
{
        struct ipcp_instance_data * inst_data;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        inst_data = (struct ipcp_instance_data *) data;
#else
        inst_data = from_timer(inst_data, tl, timers.kill_oaddress);
#endif
        if (!inst_data) {
                LOG_ERR("No IPCP instance data to work with");
                return;
        }

        LOG_INFO("Running Use New Address Timer, starting to use address %u",
        	  inst_data->address);

        efcp_address_change(inst_data->efcpc, inst_data->address);
}

/* Runs the Kill old address Timer function */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
static void tf_kill_oaddress(void * data)
#else
static void tf_kill_oaddress(struct timer_list * tl)
#endif
{
        struct ipcp_instance_data * inst_data;

        LOG_INFO("Running Kill Old Address Timer...");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        inst_data = (struct ipcp_instance_data *) data;
#else
        inst_data = from_timer(inst_data, tl, timers.use_naddress);
#endif
        if (!inst_data) {
                LOG_ERR("No IPCP instance data to work with");
                return;
        }

        rmt_address_remove(inst_data->rmt, inst_data->old_address);
}

static struct ipcp_instance_ops normal_instance_ops = {
        .flow_allocate_request     = NULL,
        .flow_allocate_response    = NULL,
        .flow_deallocate           = normal_deallocate,
        .flow_prebind              = normal_flow_prebind,
        .flow_binding_ipcp         = ipcp_flow_binding,
        .flow_unbinding_ipcp       = normal_flow_unbinding_ipcp,
        .flow_unbinding_user_ipcp  = normal_flow_unbinding_user_ipcp,
	.nm1_flow_state_change	   = normal_nm1_flow_state_change,

        .application_register      = NULL,
        .application_unregister    = NULL,

        .assign_to_dif             = normal_assign_to_dif,
        .update_dif_config         = NULL,

        .connection_create         = connection_create_request,
        .connection_update         = connection_update_request,
        .connection_destroy        = connection_destroy_request,
        .connection_create_arrived = connection_create_arrived,
	.connection_modify 	   = connection_modify_request,

        .du_enqueue               = normal_du_enqueue,
        .du_write                 = normal_du_write,

        .mgmt_du_write            = normal_mgmt_du_write,
        .mgmt_du_post             = normal_mgmt_du_post,

        .pff_add                   = normal_pff_add,
        .pff_remove                = normal_pff_remove,
        .pff_dump                  = normal_pff_dump,
        .pff_flush                 = normal_pff_flush,
	.pff_modify		   = normal_pff_modify,

        .query_rib		   = NULL,

        .ipcp_name                 = normal_ipcp_name,
        .dif_name                  = normal_dif_name,
	.ipcp_id    		   = normal_ipcp_id,

        .set_policy_set_param      = normal_set_policy_set_param,
        .select_policy_set         = normal_select_policy_set,

        .enable_write              = enable_write,
        .disable_write             = disable_write,
        .update_crypto_state       = normal_update_crypto_state,
	.address_change            = normal_address_change,
        .dif_name		   = normal_dif_name,
	.max_sdu_size		   = normal_max_sdu_size
};

static struct ipcp_instance * normal_create(struct ipcp_factory_data * data,
                                            const struct name *        name,
                                            ipc_process_id_t           id,
					    irati_msg_port_t	       us_nl_port)
{
        struct ipcp_instance * instance;

        ASSERT(data);

        if (find_instance(data, id)) {
                LOG_ERR("There is already a normal ipcp instance with id %d",
                        id);
                return NULL;
        }

        LOG_DBG("Creating normal IPC process...");
        instance = rkzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance) {
                LOG_ERR("Could not allocate memory for normal ipc process "
                        "with id %d", id);
                return NULL;
        }

        instance->ops = &normal_instance_ops;

	if (robject_rset_init_and_add(&instance->robj,
				      &normal_ipcp_rtype,
				      kipcm_rset(default_kipcm),
				      "%u",
				      id)) {
		rkfree(instance);
		return NULL;
	}

        /* FIXME: Rearrange the mess creating the data */
        instance->data = rkzalloc(sizeof(struct ipcp_instance_data),
                                  GFP_KERNEL);
        if (!instance->data) {
                LOG_ERR("Could not allocate memory for normal ipcp "
                        "internal data");
                rkfree(instance);
                return NULL;
        }

        instance->data->id      = id;
        instance->data->irati_port = us_nl_port;

        if (name_cpy(name, &instance->data->name)) {
                LOG_ERR("Failed creation of ipc name");
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        /*  FIXME: Remove as soon as the kipcm_kfa gets removed */
        instance->data->kfa = kipcm_kfa(default_kipcm);

        instance->data->efcpc = efcp_container_create(instance->data->kfa,
						      &instance->robj);
        if (!instance->data->efcpc) {
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        instance->data->sdup = sdup_create(instance);
        if (!instance->data->sdup) {
                LOG_ERR("Failed creation of SDUP instance");
                efcp_container_destroy(instance->data->efcpc);
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        instance->data->rmt = rmt_create(instance->data->kfa,
                                         instance->data->efcpc,
					 instance->data->sdup,
					 &instance->robj);
        if (!instance->data->rmt) {
                LOG_ERR("Failed creation of RMT instance");
		sdup_destroy(instance->data->sdup);
                efcp_container_destroy(instance->data->efcpc);
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        instance->data->efcpc->rmt = instance->data->rmt;

        rtimer_init(tf_use_naddress,
        	    &instance->data->timers.use_naddress,
		    instance->data);
        rtimer_init(tf_kill_oaddress,
        	    &instance->data->timers.kill_oaddress,
		    instance->data);

        INIT_LIST_HEAD(&instance->data->flows);
        INIT_LIST_HEAD(&instance->data->list);
        spin_lock_init(&instance->data->lock);
        list_add(&(instance->data->list), &(data->instances));

        LOG_DBG("Normal IPC process instance created and added to the list");

        return instance;
}

static int normal_deallocate_all(struct ipcp_instance_data * data)
{
        struct normal_flow *flow, *next;

        list_for_each_entry_safe(flow, next, &(data->flows), list) {
                if (remove_all_cepid(data, flow))
                        LOG_ERR("Some efcp structures could not be destroyed"
                                "in flow %d", flow->port_id);

                list_del(&flow->list);
                rkfree(flow);
        }

        return 0;
}

static int normal_destroy(struct ipcp_factory_data * data,
                          struct ipcp_instance *     instance)
{

        struct ipcp_instance_data * tmp;

        ASSERT(data);
        ASSERT(instance);

        tmp = find_instance(data, instance->data->id);
        if (!tmp) {
                LOG_ERR("Could not find normal ipcp instance to destroy");
                return -1;
        }

        list_del(&tmp->list);

        if (normal_deallocate_all(tmp)) {
                LOG_ERR("Could not deallocate normal ipcp flows");
                return -1;
        }

        robject_del(&instance->robj);

        efcp_container_destroy(tmp->efcpc);
        rmt_destroy(tmp->rmt);
        sdup_destroy(tmp->sdup);
        name_fini(&tmp->name);
        name_fini(&tmp->dif_name);

        rtimer_destroy(&tmp->timers.use_naddress);
        rtimer_destroy(&tmp->timers.kill_oaddress);

        rkfree(tmp);
        rkfree(instance);

        return 0;
}

static struct ipcp_factory_ops normal_ops = {
        .init    = normal_init,
        .fini    = normal_fini,
        .create  = normal_create,
        .destroy = normal_destroy,
};

static int __init mod_init(void)
{
	mgmt_sdu_wq = alloc_workqueue(IPCP_NAME,
                                      WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_UNBOUND,
				      1);
        if (!mgmt_sdu_wq) {
                LOG_CRIT("Cannot create a workqueue for the normal IPCP");
                return -1;
        }

        normal = kipcm_ipcp_factory_register(default_kipcm,
                                             IPCP_NAME,
                                             &normal_data,
                                             &normal_ops);
        if (!normal)
                return -1;

        return 0;
}

static void __exit mod_exit(void)
{
        ASSERT(normal);

        flush_workqueue(mgmt_sdu_wq);
        destroy_workqueue(mgmt_sdu_wq);

        kipcm_ipcp_factory_unregister(default_kipcm, normal);
	
	LOG_INFO("IRATI normal IPCP module removed");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA normal IPC Process");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Leonardo Bergesio <leonardo.bergesio@i2cat.net>");
