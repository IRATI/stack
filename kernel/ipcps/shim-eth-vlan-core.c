/*
 * Shim IPC Process over Ethernet (using VLANs)
 *
 *   Francesco Salvestrini <f.salvestrini@nextworks.it>
 *   Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *   Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *
 * CONTRIBUTORS:
 *
 *   Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *   Eduard Grasa	   <eduard.grasa@i2cat.net>
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
#include <linux/kernel.h>
#include <linux/if_ether.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <net/pkt_sched.h>
#include <net/sch_generic.h>

#define SHIM_NAME   	    "shim-eth-vlan"
#define SHIM_WIFI_STA_NAME  "shim-wifi-sta"
#define SHIM_WIFI_AP_NAME   "shim-wifi-ap"

#define RINA_PREFIX SHIM_NAME

#include "logs.h"
#include "kipcm.h"
#include "debug.h"
#include "utils.h"
#include "du.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"
#include "rinarp/rinarp.h"
#include "rinarp/arp826-utils.h"
#include "rds/robjects.h"

/* FIXME: To be solved properly */
static struct workqueue_struct * rcv_wq;

struct rcv_work_data {
        struct net_device *         dev;
        struct shim_eth_flow *      flow;
        struct ipcp_instance_data * data;
};

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

/* Holds the configuration of one shim instance */
struct eth_vlan_info {
        uint16_t vlan_id;
        char *   interface_name;
};

static struct ipcp_factory_data {
        struct list_head instances;
	struct notifier_block ntfy;
} eth_vlan_data;

enum port_id_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_PENDING,
        PORT_STATE_ALLOCATED
};

/* Holds the information related to one flow */
struct shim_eth_flow {
        struct list_head       list;

        struct gha *           dest_ha;
        struct gpa *           dest_pa;

        /* Only used once for allocate_response */
        port_id_t              port_id;
        enum port_id_state     port_id_state;

        /* Used when flow is not allocated yet */
        struct rfifo *         sdu_queue;
        struct ipcp_instance * user_ipcp;
};

/*
 * Contains all the information associated to an instance of a
 * shim Ethernet IPC Process
 */
struct ipcp_instance_data {
        struct list_head       list;
        ipc_process_id_t       id;

        /* IPC Process name */
        struct name *          name;
        struct name *          dif_name;
        struct eth_vlan_info * info;
        struct packet_type *   eth_vlan_packet_type;
        struct net_device *    dev;
        struct net_device *    phy_dev;
        struct flow_spec *     fspec;

        /* The IPC Process using the shim-eth-vlan */
        struct name *          app_name;
        struct name *          daf_name;

        /* Stores the state of flows indexed by port_id */
        spinlock_t             lock;
        struct list_head       flows;

        /* FIXME: Remove it as soon as the kipcm_kfa gets removed */
        struct kfa *           kfa;

        /* RINARP related */
        struct rinarp_handle * app_handle;
        struct rinarp_handle * daf_handle;

	/* To handle device notifications. */
	struct notifier_block ntfy;

	/* Flow control between this IPCP and the associated netdev. */
	unsigned int tx_busy;
};

/* Needed for eth_vlan_rcv function */
struct interface_data_mapping {
        struct list_head            list;

        struct net_device *         dev;
        struct ipcp_instance_data * data;
};

static ssize_t eth_vlan_ipcp_attr_show(struct robject *        robj,
                         	       struct robj_attribute * attr,
                                       char *                  buf)
{
	struct ipcp_instance * instance;

	instance = container_of(robj, struct ipcp_instance, robj);
	if (!instance || !instance->data)
		return 0;

	if (strcmp(robject_attr_name(attr), "name") == 0)
		return sprintf(buf, "%s\n",
			name_tostring(instance->data->name));
	if (strcmp(robject_attr_name(attr), "dif") == 0)
		return sprintf(buf, "%s\n",
			name_tostring(instance->data->dif_name));
	if (strcmp(robject_attr_name(attr), "address") == 0)
                return sprintf(buf,
		               "%02X:%02X:%02X:%02X:%02X:%02X\n",
                               instance->data->dev->dev_addr[5],
                               instance->data->dev->dev_addr[4],
                               instance->data->dev->dev_addr[3],
                               instance->data->dev->dev_addr[2],
                               instance->data->dev->dev_addr[1],
                               instance->data->dev->dev_addr[0]);
	if (strcmp(robject_attr_name(attr), "type") == 0)
		return sprintf(buf, "shim-eth-vlan\n");
	if (strcmp(robject_attr_name(attr), "vlan_id") == 0)
		return sprintf(buf, "%u\n", instance->data->info->vlan_id);
	if (strcmp(robject_attr_name(attr), "iface") == 0)
		return sprintf(buf, "%s\n",
			instance->data->info->interface_name);
	if (strcmp(robject_attr_name(attr), "tx_busy") == 0)
		return sprintf(buf, "%u\n", instance->data->tx_busy);

	return 0;
}
RINA_SYSFS_OPS(eth_vlan_ipcp);
RINA_ATTRS(eth_vlan_ipcp, name, type, dif, address, vlan_id, iface, tx_busy);
RINA_KTYPE(eth_vlan_ipcp);

static DEFINE_SPINLOCK(data_instances_lock);
static struct list_head data_instances_list;

static struct interface_data_mapping *
inst_data_mapping_get(struct net_device * dev)
{
        struct interface_data_mapping * mapping;

	ASSERT(dev);

        spin_lock(&data_instances_lock);

        list_for_each_entry(mapping, &data_instances_list, list) {
                if (mapping->dev == dev) {
                        spin_unlock(&data_instances_lock);
                        return mapping;
                }
        }

        spin_unlock(&data_instances_lock);

        return NULL;
}

static struct ipcp_instance_data *
find_instance(struct ipcp_factory_data * data,
              ipc_process_id_t           id)
{
        struct ipcp_instance_data * pos;

	ASSERT(data);

        list_for_each_entry(pos, &(data->instances), list) {
                if (pos->id == id) {
                        return pos;
                }
        }

        return NULL;

}

static struct shim_eth_flow * find_flow(struct ipcp_instance_data * data,
                                        port_id_t                   id)
{
        struct shim_eth_flow * flow;

	ASSERT(data);
	ASSERT(is_port_id_ok(id));

        spin_lock_bh(&data->lock);

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->port_id == id) {
                        spin_unlock_bh(&data->lock);
                        return flow;
                }
        }

        spin_unlock_bh(&data->lock);

        return NULL;
}

static struct gpa * name_to_gpa(const struct name * name)
{
        char *       tmp;
        struct gpa * gpa;

	ASSERT(name);

	tmp = name_tostring(name);
        if (!tmp)
                return NULL;

        gpa = gpa_create(tmp, strlen(tmp));
        if (!gpa) {
                rkfree(tmp);
                return NULL;
        }

        rkfree(tmp);

        return gpa;
}

static struct shim_eth_flow *
find_flow_by_gha(struct ipcp_instance_data * data,
                 const struct gha *          addr)
{
        struct shim_eth_flow * flow;

	ASSERT(data);
        ASSERT(gha_is_ok(addr));

        list_for_each_entry(flow, &data->flows, list) {
                if (gha_is_equal(addr, flow->dest_ha)) {
                        return flow;
                }
        }

        return NULL;
}

static struct shim_eth_flow *
find_flow_by_gpa(struct ipcp_instance_data * data,
                 const struct gpa *          addr)
{
        struct shim_eth_flow * flow;

        ASSERT(data);
	ASSERT(gpa_is_ok(addr));

        list_for_each_entry(flow, &data->flows, list) {
                if (gpa_is_equal(addr, flow->dest_pa)) {
                        return flow;
                }
        }

        return NULL;
}

static bool vlan_id_is_ok(uint16_t vlan_id)
{
        if (vlan_id & 0xF000) /* vlan_id > 4095) */
		return false;

        /*
         * Reserved values:
         *   0    Contains user_priority data (802.1Q)
         *   1    Default Port VID (802.1Q)
         *   4095 Reserved (802.1Q)
         */

        if (vlan_id == 0 || vlan_id == 1 || vlan_id == 4095) {
                /* Reserved */
                LOG_DBG("Reserved value used for vlan id");
                return false;
        }

        return true;
}

static string_t * create_vlan_interface_name(string_t * interface_name,
                                             uint16_t   vlan_id)
{
        char       string_vlan_id[5];
        string_t * complete_interface;
        size_t     length;

        ASSERT(interface_name);

	if (!vlan_id_is_ok(vlan_id)) {
		LOG_ERR("Wrong vlan-id %d", vlan_id);
		return NULL;
	}

        bzero(string_vlan_id, sizeof(string_vlan_id)); /* Be safe */
        snprintf(string_vlan_id, sizeof(string_vlan_id), "%d", vlan_id);

        length = strlen(interface_name) +
                sizeof(char)            +
                strlen(string_vlan_id)  +
                1 /* Terminator */;

        complete_interface = rkmalloc(length, GFP_KERNEL);
        if (!complete_interface)
                return NULL;

        complete_interface[0] = 0;
        strcat(complete_interface, interface_name);
        strcat(complete_interface, ".");
        strcat(complete_interface, string_vlan_id);
        complete_interface[length - 1] = 0; /* Final terminator */

        LOG_DBG("Complete vlan-interface-name = '%s'", complete_interface);

        return complete_interface;
}

static int flow_destroy(struct ipcp_instance_data * data,
                        struct shim_eth_flow *      flow)
{
        ASSERT(data);
	ASSERT(flow);

        spin_lock(&data->lock);
        if (!list_empty(&flow->list)) {
        	LOG_DBG("Deleting flow %d from list and destroying it", flow->port_id);
                list_del(&flow->list);
        }
        spin_unlock(&data->lock);

        if (flow->dest_pa) gpa_destroy(flow->dest_pa);
        if (flow->dest_ha) gha_destroy(flow->dest_ha);
        if (flow->sdu_queue)
                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) du_destroy);
        rkfree(flow);

        return 0;
}

static int unbind_and_destroy_flow(struct ipcp_instance_data * data,
                                   struct shim_eth_flow *      flow)
{
	ASSERT(data);
        ASSERT(flow);

        if (flow->user_ipcp) {
                ASSERT(flow->user_ipcp->ops);
                flow->user_ipcp->ops->
                        flow_unbinding_ipcp(flow->user_ipcp->data,
                                            flow->port_id);
        }
        LOG_DBG("Shim ethe vlan unbinded port: %u", flow->port_id);
        if (flow_destroy(data, flow)) {
                LOG_ERR("Failed to destroy shim-eth-vlan flow");
                return -1;
        }

        return 0;
}

static int eth_vlan_unbind_user_ipcp(struct ipcp_instance_data * data,
                                     port_id_t                   id)
{
        struct shim_eth_flow * flow;

	if (!data) {
		LOG_ERR("Bogus data passed, bailing out");
		return -1;
	}
	if (!is_port_id_ok(id)) {
		LOG_ERR("Invalid port ID passed, bailing out");
		return -1;
	}

        flow = find_flow(data, id);

	spin_lock_bh(&data->lock);
	if (!flow) {
		spin_unlock_bh(&data->lock);
		LOG_WARN("Could not find flow %d", id);
                return -1;
	}

        if (flow->user_ipcp) {
                flow->user_ipcp = NULL;
        }
        spin_unlock_bh(&data->lock);

        return 0;
}

static void rinarp_resolve_handler(void *             opaque,
                                   const struct gpa * dest_pa,
                                   const struct gha * dest_ha)
{
        struct ipcp_instance_data * data;
        struct ipcp_instance *      user_ipcp;
        struct ipcp_instance *      ipcp;
        struct shim_eth_flow *      flow;

        LOG_DBG("Entered the ARP resolve handler of the shim-eth");

	ASSERT(opaque);
	ASSERT(dest_pa);
	ASSERT(dest_ha);

        data = (struct ipcp_instance_data *) opaque;

        spin_lock_bh(&data->lock);
        flow = find_flow_by_gpa(data, dest_pa);
        if (!flow) {
                spin_unlock_bh(&data->lock);
                LOG_ERR("No flow found for this dest_pa");
                return;
        }

        if (flow->port_id_state == PORT_STATE_PENDING) {
                flow->port_id_state = PORT_STATE_ALLOCATED;
                spin_unlock_bh(&data->lock);

                flow->dest_ha = gha_dup_ni(dest_ha);

                user_ipcp = flow->user_ipcp;
                ASSERT(user_ipcp);

                ipcp = kipcm_find_ipcp(default_kipcm, data->id);
                if (!ipcp) {
                        LOG_ERR("KIPCM could not retrieve this IPCP");
                        unbind_and_destroy_flow(data, flow);
                        return;
                }

                ASSERT(user_ipcp);
                ASSERT(user_ipcp->ops);
                ASSERT(user_ipcp->ops->flow_binding_ipcp);
                if (user_ipcp->ops->flow_binding_ipcp(user_ipcp->data,
                                                      flow->port_id,
                                                      ipcp)) {
                        LOG_ERR("Could not bind flow with user_ipcp");
                        unbind_and_destroy_flow(data, flow);
                        return;
                }

                ASSERT(flow->sdu_queue);

                while (!rfifo_is_empty(flow->sdu_queue)) {
                        struct du * tmp = NULL;

                        tmp = rfifo_pop(flow->sdu_queue);
                        ASSERT(tmp);

                        LOG_DBG("Got a new element from the fifo");

                        ASSERT(user_ipcp->ops->sdu_enqueue);
                        if (user_ipcp->ops->du_enqueue(user_ipcp->data,
                                                        flow->port_id,
                                                        tmp)) {
                                LOG_ERR("Couldn't enqueue SDU to KFA ...");
                                return;
                        }
                }

                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) du_destroy);
                flow->sdu_queue = NULL;

                if (kipcm_notify_flow_alloc_req_result(default_kipcm,
                                                       data->id,
                                                       flow->port_id,
                                                       0)) {
                        LOG_ERR("Couldn't tell flow is allocated to KIPCM");
                        unbind_and_destroy_flow(data, flow);
                        return;
                }
        } else {
        	spin_unlock_bh(&data->lock);
        }
}

static int
eth_vlan_flow_allocate_request(struct ipcp_instance_data * data,
                               struct ipcp_instance *      user_ipcp,
                               const struct name *         source,
                               const struct name *         dest,
                               const struct flow_spec *    fspec,
                               port_id_t                   id)
{
        struct shim_eth_flow * flow;

	if (!data) {
		LOG_ERR("Bogus data passed, bailing out");
		return -1;
	}

	if (!source) {
		LOG_ERR("Bogus source passed, bailing out");
		return -1;
	}

	if (!dest) {
		LOG_ERR("Bogus dest passed, bailing out");
		return -1;
	}

	if (!is_port_id_ok(id)) {
		LOG_ERR("Invalid port ID passed, bailing out");
		return -1;
	}

        /* if (!data->app_name || !name_is_equal(source, data->app_name)) {
                LOG_ERR("Wrong request, app is not registered");
                return -1;
        } */

        flow = find_flow(data, id);
        if (!flow) {
                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow)
                        return -1;

                flow->port_id       = id;
                flow->port_id_state = PORT_STATE_PENDING;
                flow->user_ipcp     = user_ipcp;
                flow->dest_pa       = name_to_gpa(dest);

                if (!gpa_is_ok(flow->dest_pa)) {
                        LOG_ERR("Destination protocol address is not ok");
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                INIT_LIST_HEAD(&flow->list);
                spin_lock(&data->lock);
                list_add(&flow->list, &data->flows);
                spin_unlock(&data->lock);

                flow->sdu_queue = rfifo_create();
                if (!flow->sdu_queue) {
                        LOG_ERR("Couldn't create the sdu queue for a new flow");
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                if (rinarp_resolve_gpa(data->app_handle,
                                       flow->dest_pa,
                                       rinarp_resolve_handler,
                                       data) &&
                    rinarp_resolve_gpa(data->daf_handle,
                		       flow->dest_pa,
				       rinarp_resolve_handler,
				       data)) {
                        LOG_ERR("Failed to lookup ARP entry");
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }
        } else if (flow->port_id_state == PORT_STATE_PENDING) {
                LOG_ERR("Port-id state is already pending ...");
        } else {
                LOG_ERR("Allocate called in a wrong state, how dare you!");
                return -1;
        }

        return 0;
}

static int
eth_vlan_flow_allocate_response(struct ipcp_instance_data * data,
                                struct ipcp_instance *      user_ipcp,
                                port_id_t                   port_id,
                                int                         result)
{
        struct shim_eth_flow * flow;
        struct ipcp_instance * ipcp;

       	if (!data) {
		LOG_ERR("Bogus data passed, bailing out");
		return -1;
	}

	if (!is_port_id_ok(port_id)) {
		LOG_ERR("Invalid port ID passed, bailing out");
		return -1;
	}

        if (!user_ipcp) {
                LOG_ERR("Wrong user_ipcp passed, bailing out");
                kfa_port_id_release(data->kfa, port_id);
                return -1;
        }

        flow = find_flow(data, port_id);
        if (!flow) {
                LOG_ERR("Flow does not exist, you shouldn't call this");
                kfa_port_id_release(data->kfa, port_id);
                return -1;
        }

        spin_lock(&data->lock);
        if (flow->port_id_state != PORT_STATE_PENDING) {
                LOG_ERR("Flow is already allocated");
                spin_unlock(&data->lock);
                return -1;
        }
        spin_unlock(&data->lock);

        /* On positive response, flow should transition to allocated state */
        if (!result) {
                ipcp = kipcm_find_ipcp(default_kipcm, data->id);
                if (!ipcp) {
                        LOG_ERR("KIPCM could not retrieve this IPCP");
                        kfa_port_id_release(data->kfa, port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }
                ASSERT(user_ipcp->ops);
                ASSERT(user_ipcp->ops->flow_binding_ipcp);
                if (user_ipcp->ops->flow_binding_ipcp(user_ipcp->data,
                                                      port_id,
                                                      ipcp)) {
                        LOG_ERR("Could not bind flow with user_ipcp");
                        kfa_port_id_release(data->kfa, port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                spin_lock(&data->lock);
                flow->port_id_state = PORT_STATE_ALLOCATED;
                flow->port_id   = port_id;
                flow->user_ipcp = user_ipcp;
                spin_unlock(&data->lock);

                ASSERT(flow->sdu_queue);

                while (!rfifo_is_empty(flow->sdu_queue)) {
                        struct du * tmp = NULL;

                        tmp = rfifo_pop(flow->sdu_queue);
                        ASSERT(tmp);

                        LOG_DBG("Got a new element from the fifo");

                        ASSERT(flow->user_ipcp->ops->sdu_enqueue);
                        if (flow->user_ipcp->ops->
                            du_enqueue(flow->user_ipcp->data,
                                        flow->port_id,
                                        tmp)) {
                                LOG_ERR("Couldn't enqueue SDU to KFA ...");
                                return -1;
                        }
                }

                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) du_destroy);
                flow->sdu_queue = NULL;
        } else {
                spin_lock(&data->lock);
                flow->port_id_state = PORT_STATE_NULL;
                spin_unlock(&data->lock);

                /*
                 *  If we would destroy the flow, the application
                 *  we refused would constantly try to allocate
                 *  a flow again. This should only be allowed if
                 *  the IPC manager deallocates the NULL state flow first.
                 */
                ASSERT(flow->sdu_queue);
                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) du_destroy);
                flow->sdu_queue = NULL;
        }

        return 0;
}

static int eth_vlan_flow_deallocate(struct ipcp_instance_data * data,
                                    port_id_t                   id)
{
        struct shim_eth_flow * flow;

	if (!data) {
		LOG_ERR("Bogus data passed, bailing out");
		return -1;
	}

	if (!is_port_id_ok(id)) {
		LOG_ERR("Invalid port ID passed, bailing out");
		return -1;
	}

        flow = find_flow(data, id);
        if (!flow) {
                LOG_ERR("Flow does not exist, cannot remove");
                return -1;
        }

        return unbind_and_destroy_flow(data, flow);
}

static int eth_vlan_application_register(struct ipcp_instance_data * data,
                                         const struct name *         name,
					 const struct name *         daf_name)
{
        struct gpa * pa;
        struct gha * ha;

	if (!data) {
		LOG_ERR("Bogus data passed, bailing out");
		return -1;
	}

	if (!name) {
		LOG_ERR("Invalid name passed, bailing out");
		return -1;
	}

        if (data->app_name) {
                char * tmp = name_tostring(data->app_name);
                LOG_ERR("Application %s is already registered", tmp);
                if (tmp) rkfree(tmp);
                return -1;
        }

        data->app_name = name_dup(name);
        if (!data->app_name) {
                char * tmp = name_tostring(name);
                LOG_ERR("Application %s registration has failed", tmp);
                if (tmp) rkfree(tmp);
                return -1;
        }

        pa = name_to_gpa(name);
        if (!gpa_is_ok(pa)) {
                LOG_ERR("Failed to create gpa");
                name_destroy(data->app_name);
                return -1;
        }
        ha = gha_create(MAC_ADDR_802_3, data->dev->dev_addr);
        if (!gha_is_ok(ha)) {
                LOG_ERR("Failed to create gha");
                name_destroy(data->app_name);
                gpa_destroy(pa);
                return -1;
        }
        data->app_handle = rinarp_add(data->dev, pa, ha);
        if (!data->app_handle) {
                LOG_ERR("Failed to register application in ARP");
                name_destroy(data->app_name);
                gpa_destroy(pa);
                gha_destroy(ha);

                return -1;
        }
        gpa_destroy(pa);

        if (daf_name) {
        	data->daf_name = name_dup(daf_name);
        	if (!data->daf_name) {
                        char * tmp = name_tostring(daf_name);
                        LOG_ERR("DAF %s registration has failed", tmp);
                        if (tmp) rkfree(tmp);
                        rinarp_remove(data->app_handle);
                        data->app_handle = NULL;
                        name_destroy(data->app_name);
                        gha_destroy(ha);
                        return -1;
        	}

        	pa = name_to_gpa(daf_name);
        	if (!gpa_is_ok(pa)) {
        		LOG_ERR("Failed to create gpa");
        		rinarp_remove(data->app_handle);
        		data->app_handle = NULL;
        		name_destroy(data->daf_name);
                        name_destroy(data->app_name);
                        gha_destroy(ha);
        		return -1;
        	}

                data->daf_handle = rinarp_add(data->dev, pa, ha);
                if (!data->daf_handle) {
                        LOG_ERR("Failed to register DAF in ARP");
                        rinarp_remove(data->app_handle);
                        data->app_handle = NULL;
                        name_destroy(data->app_name);
                        name_destroy(data->daf_name);
                        gpa_destroy(pa);
                        gha_destroy(ha);

                        return -1;
                }

                gpa_destroy(pa);
        }

        gha_destroy(ha);

        return 0;
}

static int eth_vlan_application_unregister(struct ipcp_instance_data * data,
                                           const struct name *         name)
{
      	if (!data) {
		LOG_ERR("Bogus data passed, bailing out");
		return -1;
	}

	if (!name) {
		LOG_ERR("Invalid name passed, bailing out");
		return -1;
	}

        if (!data->app_name) {
                LOG_ERR("Shim-eth-vlan has no application registered");
                return -1;
        }

        if (!name_is_equal(data->app_name, name)) {
                LOG_ERR("Application registered != application specified");
                return -1;
        }

        /* Remove from ARP cache */
        if (data->app_handle) {
                if (rinarp_remove(data->app_handle)) {
                        LOG_ERR("Failed to remove the entry from the cache");
                        return -1;
                }
                data->app_handle = NULL;
        }

        if (data->daf_handle) {
                if (rinarp_remove(data->daf_handle)) {
                        LOG_ERR("Failed to remove the entry from the cache");
                        return -1;
                }
                data->daf_handle = NULL;
        }

        name_destroy(data->app_name);
        data->app_name = NULL;
        name_destroy(data->daf_name);
        data->daf_name = NULL;
        return 0;
}

static void enable_all_port_ids(struct ipcp_instance_data * data)
{
	struct shim_eth_flow 	  * flow;

	ASSERT(data);

	spin_lock_bh(&data->lock);
	list_for_each_entry(flow, &data->flows, list) {
		if (flow->user_ipcp && flow->user_ipcp->ops)
			flow->user_ipcp->ops->enable_write(flow->user_ipcp->data,
							   flow->port_id);
	}
	spin_unlock_bh(&data->lock);
}

static void enable_write_all(struct net_device * dev)
{
	struct ipcp_instance_data * pos;

	ASSERT(dev);

        list_for_each_entry(pos, &(eth_vlan_data.instances), list) {
                if (pos->phy_dev == dev)
                	enable_all_port_ids(pos);
        }
}

static void eth_vlan_skb_destructor(struct sk_buff *skb)
{
	struct ipcp_instance_data *data =
		(struct ipcp_instance_data *)(skb_shinfo(skb)->destructor_arg);
	bool notify;

	spin_lock_bh(&data->lock);
	notify = data->tx_busy;
	data->tx_busy = 0;
	spin_unlock_bh(&data->lock);

	if (notify) {
		enable_write_all(data->phy_dev);
	}
}

static int eth_vlan_du_write(struct ipcp_instance_data * data,
                             port_id_t                   id,
                             struct du *                 du,
                             bool                        blocking)
{
        struct shim_eth_flow *   flow;
        struct sk_buff *         skb;
        struct sk_buff *	 bup_skb;
        const unsigned char *    src_hw;
        const unsigned char *    dest_hw;
        int                      hlen, tlen, length;
        int                      retval;

        LOG_DBG("Entered the sdu-write");

	if (unlikely(!data)) {
		LOG_ERR("Bogus data passed, bailing out");
		return -1;
	}

        hlen   = sizeof(struct ethhdr);
        tlen   = data->dev->needed_tailroom;
        length = du_len(du);

        if (unlikely(length > (data->dev->mtu - hlen))) {
        	LOG_ERR("SDU too large (%d), dropping", length);
        	du_destroy(du);
        	return -1;
        }

        flow = find_flow(data, id);
        if (!flow) {
                LOG_ERR("Flow does not exist, you shouldn't call this");
                du_destroy(du);
                return -1;
        }

        spin_lock_bh(&data->lock);
        if (flow->port_id_state != PORT_STATE_ALLOCATED) {
                LOG_ERR("Flow is not in the right state to call this");
                du_destroy(du);
                spin_unlock_bh(&data->lock);
                return -1;
        }

        if (data->tx_busy) {
        	spin_unlock_bh(&data->lock);
        	return -EAGAIN;
        }
        spin_unlock_bh(&data->lock);

        src_hw = data->dev->dev_addr;
        if (!src_hw) {
                LOG_ERR("Failed to get source HW addr");
                du_destroy(du);
                return -1;
        }

        dest_hw = gha_address(flow->dest_ha);
        if (!dest_hw) {
                LOG_ERR("Destination HW address is unknown");
                du_destroy(du);
                return -1;
        }

	/* FIXME: sdu_detach_skb() has to be removed */
        skb = du_detach_skb(du);
        bup_skb = skb_clone(skb, GFP_ATOMIC);
        if (!bup_skb) {
        	LOG_ERR("Error cloning SBK, bailing out");
                kfree_skb(skb);
        	du_destroy(du);
        	return -1;
        }
        du_attach_skb(du, skb);

        if (unlikely(skb_tailroom(bup_skb) < tlen)) {
		LOG_ERR("Missing tail room in SKB, bailing out...");
                kfree_skb(bup_skb);
        	du_destroy(du);
        	return -1;
        }

        skb_reset_network_header(bup_skb);
        bup_skb->protocol = htons(ETH_P_RINA);

        retval = dev_hard_header(bup_skb, data->dev,
                                 ETH_P_RINA, dest_hw, src_hw, bup_skb->len);
        if (retval < 0) {
                LOG_ERR("Problems in dev_hard_header (%d)", retval);
                kfree_skb(bup_skb);
                du_destroy(du);
                return -1;
        }

        bup_skb->dev = data->dev;
	bup_skb->destructor = &eth_vlan_skb_destructor;
	skb_shinfo(bup_skb)->destructor_arg = (void *)data;
        retval = dev_queue_xmit(bup_skb);

        if (retval == -ENETDOWN) {
        	LOG_ERR("dev_q_xmit returned device down");
        	du_destroy(du);
        	return -1;
        }
        if (retval != NET_XMIT_SUCCESS) {
        	spin_lock_bh(&data->lock);
        	LOG_DBG("qdisc cannot enqueue now (%d), try later", retval);
        	data->tx_busy = 1;
        	spin_unlock_bh(&data->lock);
        	return -EAGAIN;
        }

       	du_destroy(du);
        LOG_DBG("Packet sent");
        return 0;
}

static int eth_vlan_rcv_worker(void * o)
{
        struct ipcp_instance_data *     data;
        const struct gpa *              gpaddr;
        struct name *                   sname;
        struct ipcp_instance           *ipcp;
	struct ipcp_instance           *user_ipcp;

        struct shim_eth_flow *          flow;
        struct rcv_work_data *          wdata;
        struct net_device *             dev;
        string_t * 			gpastr;

        LOG_DBG("Worker waking up, going to create a flow");

	ASSERT(o);

        wdata = (struct rcv_work_data *) o;

        dev    = wdata->dev;
        flow   = wdata->flow;
        data   = wdata->data;
        rkfree(wdata);

        if (!data->app_name) {
                LOG_ERR("No app registered yet! Someone is doing something bad on the network");
                return -1;
        }

        user_ipcp = kipcm_find_ipcp_by_name(default_kipcm,
                                                    data->app_name);
        if (!user_ipcp)
                user_ipcp = kfa_ipcp_instance(data->kfa);

        ipcp = kipcm_find_ipcp(default_kipcm, data->id);
        if (!user_ipcp || !ipcp) {
                LOG_ERR("Could not find required ipcps");
                if (flow_destroy(data, flow))
                        LOG_ERR("Problems destroying shim-eth-vlan "
                                "flow");
                return -1;
        }

        /* Continue filling the flow data that was started in the interrupt */
        flow->user_ipcp     = user_ipcp;
        flow->port_id       = kfa_port_id_reserve(data->kfa, data->id);

        if (!is_port_id_ok(flow->port_id)) {
                LOG_DBG("Port id is not ok");
                if (flow_destroy(data, flow))
                        LOG_ERR("Problems destroying shim-eth-vlan "
                                "flow");
                return -1;
        }

        if (!user_ipcp->ops->ipcp_name(user_ipcp->data)) {
                LOG_DBG("This flow goes for an app");
                if (kfa_flow_create(data->kfa, flow->port_id, ipcp, data->id,
                		    NULL, false)) {
                        LOG_ERR("Could not create flow in KFA");
                        kfa_port_id_release(data->kfa, flow->port_id);
                        if (flow_destroy(data, flow))
                                LOG_ERR("Problems destroying shim-eth-vlan "
                                        "flow");
                        return -1;
                }
        }
        LOG_DBG("Added flow to the list");

        sname  = NULL;
        gpaddr = rinarp_find_gpa(data->app_handle, flow->dest_ha);
        if (gpaddr && gpa_is_ok(gpaddr)) {
                flow->dest_pa = gpa_dup_gfp(GFP_KERNEL, gpaddr);
                if (!flow->dest_pa) {
                        LOG_ERR("Failed to duplicate gpa");
                        kfa_port_id_release(data->kfa, flow->port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                gpastr = gpa_address_to_string_gfp(GFP_KERNEL, gpaddr);
                if (!gpastr) {
                	LOG_ERR("Failed to convert GPA address to string");
                        kfa_port_id_release(data->kfa, flow->port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }
                sname = string_toname_ni(gpastr);
                if (!sname) {
                        LOG_ERR("Failed to convert name to string");
                        kfree(gpastr);
                        kfa_port_id_release(data->kfa, flow->port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }
                kfree(gpastr);

                LOG_DBG("Got the address from ARP");
        } else {
                sname = name_create_ni();
                if (!name_init_from_ni(sname,
                        "Unknown app", "", "", "")) {
                         name_destroy(sname);
                         kfa_port_id_release(data->kfa, flow->port_id);
                         unbind_and_destroy_flow(data, flow);
                         return -1;
                }

                flow->dest_pa = name_to_gpa(sname);
                if (!flow->dest_pa) {
                        LOG_ERR("Failed to create gpa");

                        kfa_port_id_release(data->kfa, flow->port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                LOG_DBG("Flow request from unkown app received");
        }

        if (kipcm_flow_arrived(default_kipcm,
                               data->id,
                               flow->port_id,
                               data->dif_name,
			       data->app_name,
                               sname,
                               data->fspec)) {
                LOG_ERR("Couldn't tell the KIPCM about the flow");
                kfa_port_id_release(data->kfa, flow->port_id);
                unbind_and_destroy_flow(data, flow);
                name_destroy(sname);
                return -1;
        }
        name_destroy(sname);

        LOG_DBG("Worker ends...");
        return 0;
}

static int eth_vlan_recv_process_packet(struct sk_buff *    skb,
					struct net_device * dev)
{
        struct ethhdr *                 mh;
        unsigned char *                 saddr;
        struct ipcp_instance_data *     data;
        struct interface_data_mapping * mapping;
        struct shim_eth_flow *          flow;
        struct gha *                    ghaddr;
        struct du *                     du;
	struct sk_buff *                linear_skb;

        struct rcv_work_data          * wdata;
        struct rwq_work_item          * item;

        /* C-c-c-checks */
	if (!skb) {
		LOG_ERR("Bogus skb passed, bailing out");
		return -1;
	}

	if (!dev) {
		LOG_ERR("Bogus dev passed, bailing out");
		return -1;
	}

        mapping = inst_data_mapping_get(dev);
        if (!mapping) {
                LOG_ERR("Failed to get mapping");
                kfree_skb(skb);
                return -1;
        }

        data = mapping->data;
        if (!data) {
                kfree_skb(skb);
                return -1;
        }

        if (!data->app_name) {
                LOG_ERR("No app registered yet! Someone is doing something bad on the network");
                kfree_skb(skb);
                return -1;
        }

        if (skb->pkt_type == PACKET_OTHERHOST ||
            skb->pkt_type == PACKET_LOOPBACK) {
                kfree_skb(skb);
                return -1;
        }

        mh    = eth_hdr(skb);
        saddr = mh->h_source;
        if (!saddr) {
                LOG_ERR("Couldn't get source address");
                kfree_skb(skb);
                return -1;
        }

        /* Get correct flow based on hwaddr */
        ghaddr = gha_create_ni(MAC_ADDR_802_3, saddr);
        if (!ghaddr) {
                kfree_skb(skb);
                return -1;
        }
        ASSERT(gha_is_ok(ghaddr));

	/* FIXME: If skb is not linear we need to make a copy... */
	linear_skb = skb;
	if (skb_is_nonlinear(skb)) {
		LOG_DBG("SKB is fragmented, creating linear SKB");
		linear_skb = skb_copy(skb, GFP_ATOMIC);
		if (!linear_skb) {
			LOG_ERR("Could not linearize received SKB");
			kfree_skb(skb);
			return -1;
		}
		LOG_DBG("Linear SKB fragmens: %d", skb_shinfo(linear_skb)->nr_frags);
		kfree_skb(skb);
	}

	du = du_create_from_skb(linear_skb);
	if (!du) {
		LOG_ERR("Could not create SDU from buffer");
                kfree_skb(linear_skb);
                return -1;
        }

        spin_lock(&data->lock);
        flow = find_flow_by_gha(data, ghaddr);
        if (!flow) {
                spin_unlock(&data->lock);
                /* Create flow and its queue to handle next packets */
                flow = rkzalloc(sizeof(*flow), GFP_ATOMIC);
                if (!flow) {
                        du_destroy(du);
                        gha_destroy(ghaddr);
                        return -1;
                }

                flow->port_id_state = PORT_STATE_PENDING;
                flow->dest_ha       = ghaddr;
                INIT_LIST_HEAD(&flow->list);
                flow->sdu_queue = rfifo_create_ni();
                if (!flow->sdu_queue) {
                        LOG_ERR("Couldn't create the SDU queue "
                                "for a new flow");
                        du_destroy(du);
                        gha_destroy(ghaddr);
                        rkfree(flow);
                        return -1;
                }

                /* Store SDU in queue */
                if (rfifo_push_ni(flow->sdu_queue, du)) {
                        LOG_ERR("Could not push a SDU into the flow queue");
                        du_destroy(du);
                        gha_destroy(ghaddr);
                        rkfree (flow);
                        rfifo_destroy(flow->sdu_queue,
                                      (void (*)(void *)) du_destroy);
                        return -1;
                }

                spin_lock(&data->lock);
                list_add(&flow->list, &data->flows);
                spin_unlock(&data->lock);

                /*FIXME: add checks */

                wdata = rkzalloc(sizeof(* wdata), GFP_ATOMIC);
                wdata->dev  = dev;
                wdata->flow = flow;
                wdata->data = data;
                item  = rwq_work_create_ni(eth_vlan_rcv_worker, wdata);

                rwq_work_post(rcv_wq, item);

                LOG_DBG("eth_vlan_recv_process_packet added work");
        } else {
                gha_destroy(ghaddr);
                LOG_DBG("Flow exists, queueing or delivering or dropping");
                if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                        if (!flow->user_ipcp) {
                        	spin_unlock(&data->lock);
                        	LOG_ERR("Flow is being deallocated, dropping PDU");
                                du_destroy(du);
                                return -1;
                        }

                        spin_unlock(&data->lock);

                        ASSERT(flow->user_ipcp->ops);
                        ASSERT(flow->user_ipcp->ops->sdu_enqueue);
                        if (flow->user_ipcp->ops->
                            du_enqueue(flow->user_ipcp->data,
                                       flow->port_id,
                                       du)) {
                                LOG_ERR("Couldn't enqueue SDU to user IPCP");
                                return -1;
                        }

                } else if (flow->port_id_state == PORT_STATE_PENDING) {
                        LOG_DBG("Queueing frame");

                        if (rfifo_push_ni(flow->sdu_queue, du)) {
                                LOG_ERR("Failed to write %zd bytes"
                                        "into the fifo",
                                        sizeof(struct sdu *));
                                spin_unlock(&data->lock);
                                du_destroy(du);
                                return -1;
                        }

                        spin_unlock(&data->lock);
                } else if (flow->port_id_state == PORT_STATE_NULL) {
                        spin_unlock(&data->lock);
                        du_destroy(du);
                }

                LOG_DBG("eth_vlan_recv_process_packet ends ...");
        }

        return 0;
}

static int eth_vlan_rcv(struct sk_buff *     skb,
                        struct net_device *  dev,
                        struct packet_type * pt,       /* not used */
                        struct net_device *  orig_dev) /* not used */
{
	ASSERT(skb);
	ASSERT(dev);

	LOG_DBG("eth_vlan_rcv started, skb received");
        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb) {
                LOG_ERR("Couldn't obtain ownership of the skb");
                return 0;
        }

        if (eth_vlan_recv_process_packet(skb, dev))
                LOG_DBG("Failed to process packet");

        LOG_DBG("eth_vlan_rcv ends");
        return 0;
};

static int eth_vlan_assign_to_dif(struct ipcp_instance_data * data,
                		  const struct name * dif_name,
				  const string_t * type,
				  struct dif_config * config)
{
        struct eth_vlan_info *          info;
        struct ipcp_config *            tmp;
        string_t *                      complete_interface;
        struct interface_data_mapping * mapping;
        int                             result;
        unsigned int                    temp;

	if (!data) {
		LOG_ERR("Bogus data passed, bailing out");
		return -1;
	}

        if (!config) {
		LOG_ERR("Bogus dif_information passed, bailing out");
		return -1;
	}

        info = data->info;

        if (data->dif_name) {
                ASSERT(data->dif_name->process_name);

                LOG_ERR("IPCP already assigned to DIF %s, can be assigned only once",
                        data->dif_name->process_name);
                return -1;
        }

        /* Get vlan id */
        result = kstrtouint(dif_name->process_name, 10, &temp);
        if (result) {
                ASSERT(dif_name->process_name);

                LOG_ERR("Error converting DIF Name to VLAN ID: %s",
                        dif_name->process_name);
                return -1;
        }
        info->vlan_id = (uint16_t) temp;

        if (!vlan_id_is_ok(info->vlan_id)) {
                if (info->vlan_id != 0) {
                      LOG_ERR("Bad vlan id specified: %d", info->vlan_id);
                      return -1;
                }
        }

        data->dif_name = name_dup(dif_name);
        if (!data->dif_name) {
                LOG_ERR("Error duplicating name, bailing out");
                return -1;
        }

        /* Retrieve configuration of IPC process from params */
        list_for_each_entry(tmp, &(config->ipcp_config_entries), next) {
		const struct ipcp_config_entry * entry = tmp->entry;
		if (!strcmp(entry->name, "interface-name")) {
			ASSERT(entry->value);

			info->interface_name = rkstrdup(entry->value);
			if (!info->interface_name) {
				LOG_ERR("Cannot copy interface name");
				name_destroy(data->dif_name);
				data->dif_name = NULL;
				return -1;
			}
		} else
                	LOG_DBG("Unknown config param for eth shim, ignoring");
        }

        /* Fail here if we didn't get an interface */
        if (!info->interface_name) {
                LOG_ERR("Didn't get an interface name");
                name_destroy(data->dif_name);
                data->dif_name = NULL;
                return -1;
        }

        data->eth_vlan_packet_type->type = cpu_to_be16(ETH_P_RINA);
        data->eth_vlan_packet_type->func = eth_vlan_rcv;

        if (info->vlan_id != 0) {
                complete_interface =
                        create_vlan_interface_name(info->interface_name,
                                                   info->vlan_id);
        } else {
                complete_interface = rkstrdup(info->interface_name);
        }

        if (!complete_interface) {
                name_destroy(data->dif_name);
                data->dif_name = NULL;
                rkfree(info->interface_name);
                info->interface_name = NULL;
                return -1;
        }

        ASSERT(complete_interface);

        /* Add the handler */
        read_lock(&dev_base_lock);
        data->dev = __dev_get_by_name(&init_net, complete_interface);
        if (info->vlan_id != 0)
        	data->phy_dev = __dev_get_by_name(&init_net,
        					  info->interface_name);
        else
        	data->phy_dev = data->dev;
	read_unlock(&dev_base_lock);
        if (!data->dev || !data->phy_dev) {
                LOG_ERR("Can't get device '%s'", complete_interface);
                name_destroy(data->dif_name);
                data->dif_name = NULL;
                rkfree(info->interface_name);
                info->interface_name = NULL;
                rkfree(complete_interface);
                data->dev = NULL;
                data->phy_dev = NULL;
                return -1;
        }

        LOG_DBG("Got device '%s', trying to register handler",
                complete_interface);

        /* Store in list for retrieval later on */
        mapping = rkmalloc(sizeof(*mapping), GFP_ATOMIC);
        if (!mapping) {
                name_destroy(data->dif_name);
                data->dif_name = NULL;
                rkfree(info->interface_name);
                info->interface_name = NULL;
                rkfree(complete_interface);
                return -1;
        }

        mapping->dev  = data->dev;
        mapping->data = data;
        INIT_LIST_HEAD(&mapping->list);

        spin_lock(&data_instances_lock);
        list_add(&mapping->list, &data_instances_list);
        spin_unlock(&data_instances_lock);

        data->eth_vlan_packet_type->dev = data->dev;
        dev_add_pack(data->eth_vlan_packet_type);
        rkfree(complete_interface);

        LOG_DBG("Configured shim eth vlan IPC Process");

        return 0;
}

static int eth_vlan_update_dif_config(struct ipcp_instance_data * data,
                                      const struct dif_config *   new_config)
{
        struct eth_vlan_info *          info;
        struct ipcp_config *            tmp;
        string_t *                      old_interface_name;
        string_t *                      complete_interface;
        struct interface_data_mapping * mapping;

	if (!data) {
		LOG_ERR("Bogus data passed, bailing out");
		return -1;
	}

        if (!new_config) {
		LOG_ERR("Bogus configuration passed, bailing out");
		return -1;
	}

        /* Get configuration struct pertaining to this shim instance */
        info               = data->info;
        old_interface_name = info->interface_name;

        /* Retrieve configuration of IPC process from params */
        list_for_each_entry(tmp, &(new_config->ipcp_config_entries), next) {
                const struct ipcp_config_entry * entry;

                entry = tmp->entry;
                if (!strcmp(entry->name, "interface-name")) {
                	ASSERT(entry->value);

                	info->interface_name = rkstrdup(entry->value);
                	if (!info->interface_name) {
                		LOG_ERR("Cannot copy interface name");
                		return -1;
                	}
		} else
                	LOG_DBG("Unknown config param for eth shim, ignoring");
        }

	dev_remove_pack(data->eth_vlan_packet_type);

	if (data->dev) {
		/* Remove from list */
		mapping = inst_data_mapping_get(data->dev);
		if (mapping) {
			spin_lock(&data_instances_lock);
			list_del(&mapping->list);
			spin_unlock(&data_instances_lock);
			rkfree(mapping);
		}
	}

        data->eth_vlan_packet_type->type = cpu_to_be16(ETH_P_RINA);
        data->eth_vlan_packet_type->func = eth_vlan_rcv;

        if (info->vlan_id != 0) {
                complete_interface =
                        create_vlan_interface_name(info->interface_name,
                                                   info->vlan_id);
        } else {
                complete_interface = rkstrdup(info->interface_name);
        }

        if (!complete_interface)
                return -1;

        /* Add the handler */
        read_lock(&dev_base_lock);
        data->dev = __dev_get_by_name(&init_net, complete_interface);
        if (info->vlan_id != 0) {
        	data->phy_dev = __dev_get_by_name(&init_net,
        					  info->interface_name);
        } else {
        	data->phy_dev = data->dev;
        }
	read_unlock(&dev_base_lock);
        if (!data->dev) {
                LOG_ERR("Invalid device to configure: %s", complete_interface);
                return -1;
        }

        /* Store in list for retrieval later on */
        mapping = rkmalloc(sizeof(*mapping), GFP_KERNEL);
        if (!mapping)
                return -1;

        mapping->dev = data->dev;
        mapping->data = data;
        INIT_LIST_HEAD(&mapping->list);

        spin_lock(&data_instances_lock);
        list_add(&mapping->list, &data_instances_list);
        spin_unlock(&data_instances_lock);

        data->eth_vlan_packet_type->dev = data->dev;
        dev_add_pack(data->eth_vlan_packet_type);
        rkfree(complete_interface);

        LOG_DBG("Configured shim eth vlan IPC Process");

        return 0;
}

static const struct name * eth_vlan_ipcp_name(struct ipcp_instance_data * data)
{
        ASSERT(data);
        ASSERT(name_is_ok(data->name));

        return data->name;
}

static const struct name * eth_vlan_dif_name(struct ipcp_instance_data * data)
{
	if (!data) {
		LOG_ERR("Bogus data passed, bailing out");
		return NULL;
	}

        return data->dif_name;
}

static size_t eth_vlan_max_sdu_size(struct ipcp_instance_data * data)
{
	if (!data) {
		LOG_ERR("Bogus data passed, bailing out");
		return 0;
	}

        return data->dev->mtu - sizeof(struct ethhdr);
}

ipc_process_id_t eth_vlan_ipcp_id(struct ipcp_instance_data * data)
{
	ASSERT(data);
	return data->id;
}

static int eth_vlan_query_rib(struct ipcp_instance_data * data,
                              struct list_head *          entries,
                              const string_t *            object_class,
                              const string_t *            object_name,
                              uint64_t                    object_instance,
                              uint32_t                    scope,
                              const string_t *            filter)
{
	LOG_MISSING;
	return -1;
}

static struct ipcp_instance_ops eth_vlan_instance_ops = {
        .flow_allocate_request     = eth_vlan_flow_allocate_request,
        .flow_allocate_response    = eth_vlan_flow_allocate_response,
        .flow_deallocate           = eth_vlan_flow_deallocate,
        .flow_prebind              = NULL,
        .flow_binding_ipcp         = NULL,
        .flow_unbinding_ipcp       = NULL,
        .flow_unbinding_user_ipcp  = eth_vlan_unbind_user_ipcp,
	.nm1_flow_state_change	   = NULL,

        .application_register      = eth_vlan_application_register,
        .application_unregister    = eth_vlan_application_unregister,

        .assign_to_dif             = eth_vlan_assign_to_dif,
        .update_dif_config         = eth_vlan_update_dif_config,

        .connection_create         = NULL,
        .connection_update         = NULL,
        .connection_destroy        = NULL,
        .connection_create_arrived = NULL,
	.connection_modify 	   = NULL,

        .du_enqueue               = NULL,
        .du_write                 = eth_vlan_du_write,

        .mgmt_du_write            = NULL,
        .mgmt_du_post             = NULL,

        .pff_add                   = NULL,
        .pff_remove                = NULL,
        .pff_dump                  = NULL,
        .pff_flush                 = NULL,
	.pff_modify		   = NULL,

        .query_rib		   = eth_vlan_query_rib,

        .ipcp_name                 = eth_vlan_ipcp_name,
        .dif_name                  = eth_vlan_dif_name,
	.ipcp_id		   = eth_vlan_ipcp_id,

        .set_policy_set_param      = NULL,
        .select_policy_set         = NULL,
        .update_crypto_state	   = NULL,
	.address_change            = NULL,
        .dif_name		   = eth_vlan_dif_name,
	.max_sdu_size		   = eth_vlan_max_sdu_size
};

static int ntfy_user_ipcp_on_if_state_change(struct ipcp_instance_data * data,
					     bool up)
{
        struct shim_eth_flow * flow;

	ASSERT(data);

        list_for_each_entry(flow, &data->flows, list) {
                if (!flow->user_ipcp) {
			/* This flow is used by an userspace application,
			 * we are not able to notify that one for now. */
			continue;
                }

		flow->user_ipcp->ops->
                            nm1_flow_state_change(flow->user_ipcp->data,
                                                  flow->port_id, up);
        }

	return 0;
}

static int eth_vlan_netdev_notify(struct notifier_block *nb,
				  unsigned long event,
				  void *opaque)
{
	struct net_device *dev;
        struct ipcp_instance_data * pos;

	ASSERT(nb);
	ASSERT(opaque);

	dev = netdev_notifier_info_to_dev(opaque);

        list_for_each_entry(pos, &eth_vlan_data.instances, list) {
		if (pos->dev != dev) {
			/* We don't care about this network interface. */
			continue;
		}

		switch (event) {

		case NETDEV_UP:
			LOG_INFO("Device %s goes up", dev->name);
			ntfy_user_ipcp_on_if_state_change(pos, true);
			spin_lock_bh(&pos->lock);
			pos->tx_busy = 0;
			spin_unlock_bh(&pos->lock);
			enable_write_all(pos->phy_dev);
			break;

		case NETDEV_DOWN:
			LOG_INFO("Device %s goes down", dev->name);
			ntfy_user_ipcp_on_if_state_change(pos, false);
			break;

		default:
			LOG_DBG("Ignoring event %lu on device %s",
				event, dev->name);
			break;
		}
	}

	return 0;
}

static int eth_vlan_init(struct ipcp_factory_data * data)
{
	ASSERT(data == &eth_vlan_data);

	bzero(data, sizeof(*data));
	INIT_LIST_HEAD(&(data->instances));

	INIT_LIST_HEAD(&data_instances_list);

	memset(&data->ntfy, 0, sizeof(data->ntfy));
	data->ntfy.notifier_call = eth_vlan_netdev_notify;
	register_netdevice_notifier(&data->ntfy);

        LOG_INFO("%s initialized", SHIM_NAME);

        return 0;
}

static int eth_vlan_init2(struct ipcp_factory_data * data)
{
	ASSERT(data == &eth_vlan_data);

        LOG_INFO("Shim Wifi AP/STA initialized");

        return 0;
}

static int eth_vlan_fini(struct ipcp_factory_data * data)
{

	ASSERT(data == &eth_vlan_data);

	ASSERT(list_empty(&(data->instances)));
	unregister_netdevice_notifier(&data->ntfy);

	return 0;
}

static int eth_vlan_fini2(struct ipcp_factory_data * data)
{

	ASSERT(data == &eth_vlan_data);
	return 0;
}

static void inst_cleanup(struct ipcp_instance * inst)
{
        ASSERT(inst);

        if (inst->data) {
                if (inst->data->fspec)
                        rkfree(inst->data->fspec);
                if (inst->data->info)
                        rkfree(inst->data->info);
                if (inst->data->name)
                        name_destroy(inst->data->name);
                if (inst->data->eth_vlan_packet_type)
                        rkfree(inst->data->eth_vlan_packet_type);

                rkfree(inst->data);
        }

        rkfree(inst);
}

static struct ipcp_instance * eth_vlan_create(struct ipcp_factory_data * data,
                                              const struct name *        name,
                                              ipc_process_id_t           id,
					      uint_t			 us_nl_port)
{
        struct ipcp_instance * inst;

        ASSERT(data);
	ASSERT(name);

        /* Check if there already is an instance with that id */
        if (find_instance(data,id)) {
                LOG_ERR("There's a shim instance with id %d already", id);
                return NULL;
        }

        /* Create an instance */
        inst = rkzalloc(sizeof(*inst), GFP_KERNEL);
        if (!inst)
                return NULL;

        /* fill it properly */
        inst->ops  = &eth_vlan_instance_ops;

	if (robject_rset_init_and_add(&inst->robj,
				      &eth_vlan_ipcp_rtype,
				      kipcm_rset(default_kipcm),
				      "%u",
				      id)) {
		rkfree(inst);
		return NULL;
	}

        inst->data = rkzalloc(sizeof(struct ipcp_instance_data), GFP_KERNEL);
        if (!inst->data) {
                inst_cleanup(inst);
                return NULL;
        }

        inst->data->eth_vlan_packet_type =
                rkzalloc(sizeof(struct packet_type), GFP_KERNEL);
        if (!inst->data->eth_vlan_packet_type) {
                LOG_ERR("Instance creation failed (#1)");
                inst_cleanup(inst);
                return NULL;
        }

        inst->data->id = id;

        inst->data->name = name_dup(name);
        if (!inst->data->name) {
                LOG_ERR("Failed creation of ipc name");
                inst_cleanup(inst);
                return NULL;
        }

        inst->data->info = rkzalloc(sizeof(*inst->data->info), GFP_KERNEL);
        if (!inst->data->info) {
                LOG_ERR("Instance creation failed (#2)");
                inst_cleanup(inst);
                return NULL;
        }
	inst->data->tx_busy = 0;

        inst->data->fspec = rkzalloc(sizeof(*inst->data->fspec), GFP_KERNEL);
        if (!inst->data->fspec) {
                LOG_ERR("Instance creation failed (#3)");
                inst_cleanup(inst);
                return NULL;
        }

        inst->data->fspec->average_bandwidth           = 0;
        inst->data->fspec->average_sdu_bandwidth       = 0;
        inst->data->fspec->delay                       = 0;
        inst->data->fspec->jitter                      = 0;
        inst->data->fspec->max_allowable_gap           = -1;
        inst->data->fspec->max_sdu_size                = 1500;
        inst->data->fspec->ordered_delivery            = 0;
        inst->data->fspec->partial_delivery            = 1;
        inst->data->fspec->peak_bandwidth_duration     = 0;
        inst->data->fspec->peak_sdu_bandwidth_duration = 0;
        inst->data->fspec->undetected_bit_error_rate   = 0;

        /* FIXME: Remove as soon as the kipcm_kfa gets removed*/
        inst->data->kfa = kipcm_kfa(default_kipcm);

        ASSERT(inst->data->kfa);

        LOG_DBG("KFA instance %pK bound to the shim eth", inst->data->kfa);

        spin_lock_init(&inst->data->lock);

        INIT_LIST_HEAD(&(inst->data->flows));

        /*
         * Bind the shim-instance to the shims set, to keep all our data
         * structures linked (somewhat) together
         */
        INIT_LIST_HEAD(&(inst->data->list));
        list_add(&(inst->data->list), &(data->instances));

        return inst;
}

static int eth_vlan_destroy(struct ipcp_factory_data * data,
                            struct ipcp_instance *     instance)
{
        struct interface_data_mapping * mapping;
        struct ipcp_instance_data * pos, * next;
        struct shim_eth_flow * flow, * nflow;

        ASSERT(data);
        ASSERT(instance);

        LOG_DBG("Looking for the eth-vlan-instance to destroy");

        /* Retrieve the instance */
        list_for_each_entry_safe(pos, next, &data->instances, list) {
                if (pos->id == instance->data->id) {

                        /* Destroy existing flows */
                        list_for_each_entry_safe(flow, nflow, &pos->flows, list) {
                                unbind_and_destroy_flow(pos, flow);
                        }

                        /* Remove packet handler if there is one */
                        if (pos->eth_vlan_packet_type->dev)
                                __dev_remove_pack(pos->eth_vlan_packet_type);

                        /* Unbind from the instances set */
                        list_del(&pos->list);

                        /* Destroy it */
                        if (pos->name)
                                name_destroy(pos->name);

                        if (pos->dif_name)
                                name_destroy(pos->dif_name);

                        if (pos->app_name)
                                name_destroy(pos->app_name);

                        if (pos->daf_name)
                                name_destroy(pos->daf_name);

                        if (pos->info->interface_name)
                                rkfree(pos->info->interface_name);

                        if (pos->info)
                                rkfree(pos->info);

                        if (pos->fspec)
                                rkfree(pos->fspec);

                        if (pos->app_handle) {
                                if (rinarp_remove(pos->app_handle)) {
                                        LOG_ERR("Failed to remove "
                                                "the entry from the cache");
                                        return -1;
                                }
                        }

                        if (pos->daf_handle) {
                                if (rinarp_remove(pos->daf_handle)) {
                                        LOG_ERR("Failed to remove "
                                                "the entry from the cache");
                                        return -1;
                                }
                        }

                        if (pos->dev) {
                        	mapping = inst_data_mapping_get(pos->dev);
                        	if (mapping) {
                        		LOG_DBG("removing mapping from list");
                        		spin_lock(&data_instances_lock);
                        		list_del(&mapping->list);
                        		spin_unlock(&data_instances_lock);
                        		rkfree(mapping);
                        	}
                        }

                        robject_del(&instance->robj);

                        /*
                         * Might cause problems:
                         * The packet type might still be in use by receivers
                         * and must not be freed until after all
                         * the CPU's have gone through a quiescent state.
                         */
                        if (pos->eth_vlan_packet_type)
                                rkfree(pos->eth_vlan_packet_type);

                        rkfree(pos);
                        rkfree(instance);

                        LOG_DBG("Eth-vlan instance destroyed, returning");

                        return 0;
                }
        }

        LOG_DBG("Didn't find instance, returning error");

        return -1;
}

static struct ipcp_factory_ops eth_vlan_ops = {
        .init      = eth_vlan_init,
        .fini      = eth_vlan_fini,
        .create    = eth_vlan_create,
        .destroy   = eth_vlan_destroy,
};

static struct ipcp_factory_ops eth_vlan_ops2 = {
        .init      = eth_vlan_init2,
        .fini      = eth_vlan_fini2,
        .create    = eth_vlan_create,
        .destroy   = eth_vlan_destroy,
};

static struct ipcp_factory * shim_eth_vlan = NULL;
static struct ipcp_factory * shim_wifi_sta = NULL;
static struct ipcp_factory * shim_wifi_ap = NULL;

#ifdef CONFIG_RINA_SHIM_ETH_VLAN_REGRESSION_TESTS
static bool regression_test_create_vlan_interface_name(void)
{
        string_t * tmp;

        LOG_DBG("Create-vlan-name regression tests");

        LOG_DBG("Regression test #1");

        LOG_DBG("Regression test #1.1");
        tmp = create_vlan_interface_name("eth0", 0);
        if (tmp) {
                rkfree(tmp);
                return false;
        }

        LOG_DBG("Regression test #1.2");
        tmp = create_vlan_interface_name("eth0", 1);
        if (tmp) {
                rkfree(tmp);
                return false;
        }

        LOG_DBG("Regression test #1.3");
        tmp = create_vlan_interface_name("eth0", 4095);
        if (tmp) {
                rkfree(tmp);
                return false;
        }

        LOG_DBG("Regression test #1.4");
        tmp = create_vlan_interface_name("eth0", 4096);
        if (tmp) {
                rkfree(tmp);
                return false;
        }

        LOG_DBG("Regression test #2");

        LOG_DBG("Regression test #2.1");
        tmp = create_vlan_interface_name("eth0", 2);
        if (!tmp)
                return false;
        if (strlen(tmp) != 6) {
                rkfree(tmp);
                return false;
        }
        rkfree(tmp);

        LOG_DBG("Regression test #2.2");
        tmp = create_vlan_interface_name("eth0", 10);
        if (!tmp)
                return false;
        if (strlen(tmp) != 7) {
                rkfree(tmp);
                return false;
        }
        rkfree(tmp);

        LOG_DBG("Regression test #2.3");
        tmp = create_vlan_interface_name("eth0", 100);
        if (!tmp)
                return false;
        if (strlen(tmp) != 8) {
                rkfree(tmp);
                return false;
        }
        rkfree(tmp);

        LOG_DBG("Regression test #2.4");
        tmp = create_vlan_interface_name("eth0", 1000);
        if (!tmp)
                return false;
        if (strlen(tmp) != 9) {
                rkfree(tmp);
                return false;
        }
        rkfree(tmp);

        return true;
}

static bool regression_tests(void)
{
        if (!regression_test_create_vlan_interface_name()) {
                LOG_ERR("Create-vlan-interface tests failed, bailing out");
                return false;
        }

        return true;
}
#endif

static int __init mod_init(void)
{
#ifdef CONFIG_RINA_SHIM_ETH_VLAN_REGRESSION_TESTS
        LOG_DBG("Starting regression tests");

        if (!regression_tests()) {
                LOG_ERR("Regression tests failed, bailing out");
                return -1;
        }

        LOG_DBG("Regression tests completed successfully");

#endif

        rcv_wq = alloc_workqueue(SHIM_NAME,
                                 WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_UNBOUND,
                                 1);
        if (!rcv_wq) {
                LOG_CRIT("Cannot create a workqueue for shim %s", SHIM_NAME);
                return -1;
        }

        shim_eth_vlan = kipcm_ipcp_factory_register(default_kipcm,
                                           	    SHIM_NAME,
                                          	    &eth_vlan_data,
						    &eth_vlan_ops);
        if (!shim_eth_vlan)
                return -1;

        shim_wifi_ap = kipcm_ipcp_factory_register(default_kipcm,
                                           	   SHIM_WIFI_AP_NAME,
                                          	   &eth_vlan_data,
						   &eth_vlan_ops2);
        if (!shim_wifi_ap) {
        	kipcm_ipcp_factory_unregister(default_kipcm, shim_eth_vlan);
                return -1;
        }

        shim_wifi_sta = kipcm_ipcp_factory_register(default_kipcm,
                                           	    SHIM_WIFI_STA_NAME,
                                          	    &eth_vlan_data,
						    &eth_vlan_ops2);
        if (!shim_wifi_sta) {
        	kipcm_ipcp_factory_unregister(default_kipcm, shim_wifi_ap);
        	kipcm_ipcp_factory_unregister(default_kipcm, shim_eth_vlan);
                return -1;
        }

        spin_lock_init(&data_instances_lock);

        return 0;
}

static void __exit mod_exit(void)
{
        ASSERT(shim_eth_vlan);
        ASSERT(shim_wifi_ap);
        ASSERT(shim_wifi_sta);

        flush_workqueue(rcv_wq);
        destroy_workqueue(rcv_wq);

        kipcm_ipcp_factory_unregister(default_kipcm, shim_eth_vlan);
        kipcm_ipcp_factory_unregister(default_kipcm, shim_wifi_ap);
        kipcm_ipcp_factory_unregister(default_kipcm, shim_wifi_sta);

	LOG_INFO("IRATI shim Ethernet module removed");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Shim IPC Process over Ethernet");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
