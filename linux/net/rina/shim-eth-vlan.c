/*
 *  Shim IPC Process over Ethernet (using VLANs)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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
#include <linux/kernel.h>
#include <linux/if_ether.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/kfifo.h>

#define PROTO_LEN   32
#define SHIM_NAME   "shim-eth-vlan"

#define RINA_PREFIX SHIM_NAME

#include "logs.h"
#include "common.h"
#include "kipcm.h"
#include "debug.h"
#include "utils.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"
#include "fidm.h"
#include "rinarp.h"
#include "arp826-utils.h"
#include "du.h"

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

/* Holds the configuration of one shim instance */
struct eth_vlan_info {
        unsigned long   vlan_id;
        char *          interface_name;
};

enum port_id_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_PENDING,
        PORT_STATE_ALLOCATED
};

/* Holds the information related to one flow */
struct shim_eth_flow {
        struct list_head   list;
        struct gha *       dest_ha;
        struct gpa *       dest_pa;
        /* Only used once for allocate_response */
        flow_id_t          flow_id;
        port_id_t          port_id;
        enum port_id_state port_id_state;

        /* Used when flow is not allocated yet */
        struct kfifo       sdu_queue;
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

        /* The IPC Process using the shim-eth-vlan */
        struct name *          app_name;

        /* Stores the state of flows indexed by port_id */
        spinlock_t             lock;
        struct list_head       flows;

        /* FIXME: Remove it as soon as the kipcm_kfa gets removed */
        struct kfa * kfa;

        /* RINARP related */
        struct rinarp_handle *  handle;
};

/* Needed for eth_vlan_rcv function */
struct interface_data_mapping {
        struct list_head            list;

        struct net_device *         dev;
        struct ipcp_instance_data * data;
};

static spinlock_t       data_instances_lock;
static struct list_head data_instances_list;

static struct interface_data_mapping *
inst_data_mapping_get(struct net_device * dev)
{
        struct interface_data_mapping * mapping;
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

static struct shim_eth_flow * find_flow(struct ipcp_instance_data * data,
                                        port_id_t                   id)
{
        struct shim_eth_flow * flow;
        spin_lock(&data->lock);

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->port_id == id) {
                        spin_unlock(&data->lock);
                        return flow;
                }
        }
        spin_unlock(&data->lock);
        return NULL;
}

static struct shim_eth_flow *
find_flow_by_flow_id(struct ipcp_instance_data * data,
                     flow_id_t                   id)
{
        struct shim_eth_flow * flow;
        spin_lock(&data->lock);
        list_for_each_entry(flow, &data->flows, list) {
                if (flow->flow_id == id) {
                        spin_unlock(&data->lock);
                        return flow;
                }
        }
        spin_unlock(&data->lock);
        return NULL;
}

static struct gpa * name_to_gpa(const struct name * name)
{
        char *       delimited_name;
        struct gpa * addr;

        delimited_name = name_tostring(name);
        addr = gpa_create(delimited_name,
                          strlen(delimited_name));


        return addr;
}

static struct shim_eth_flow *
find_flow_by_gha(struct ipcp_instance_data * data,
                 const struct gha *          addr)
{
        struct shim_eth_flow * flow;
        spin_lock(&data->lock);
        list_for_each_entry(flow, &data->flows, list) {
                if (gha_is_equal(addr, flow->dest_ha)) {
                        spin_unlock(&data->lock);
                        return flow;
                }
        }
        spin_unlock(&data->lock);
        return NULL;
}

static struct shim_eth_flow *
find_flow_by_gpa(struct ipcp_instance_data * data,
                 const struct gpa *          addr)
{
        struct shim_eth_flow * flow;
        spin_lock(&data->lock);
        list_for_each_entry(flow, &data->flows, list) {
                if (gpa_is_equal(addr, flow->dest_pa)) {
                        spin_unlock(&data->lock);
                        return flow;
                }
        }
        spin_unlock(&data->lock);
        return NULL;
}

static string_t * create_vlan_interface_name(string_t * interface_name,
                                             unsigned long   vlan_id)
{
        char       string_vlan_id[5];
        string_t * complete_interface;

        sprintf(string_vlan_id,"%lu",vlan_id);

        complete_interface = rkzalloc(
                                      strlen(interface_name) + 2*sizeof(char)
                                      + strlen(string_vlan_id),
                                      GFP_KERNEL);
        strcat(complete_interface, interface_name);
        strcat(complete_interface, ".");
        strcat(complete_interface, string_vlan_id);

        return complete_interface;
}

static int flow_destroy(struct ipcp_instance_data * data,
                        struct shim_eth_flow **     f)
{
        struct shim_eth_flow * flow;
        flow_id_t fid;
        spin_lock(&data->lock);

        flow = *f;

        if(flow->dest_pa)
                gpa_destroy(flow->dest_pa);
        if(flow->dest_ha)
                gha_destroy(flow->dest_ha);
        list_del(&flow->list);

        fid = kfa_flow_unbind(data->kfa,
                              flow->port_id);
        kfa_flow_destroy(data->kfa, fid);

        rkfree(flow);
        spin_unlock(&data->lock);
        return 0;
}



static void rinarp_resolve_handler(void *             opaque,
                                   const struct gpa * dest_pa,
                                   const struct gha * dest_ha)
{
        struct ipcp_instance_data * data;
        struct shim_eth_flow * flow;

        data = (struct ipcp_instance_data *) opaque;
        flow = find_flow_by_gpa(data, dest_pa);

        if (flow && flow->port_id_state == PORT_STATE_PENDING) {
                flow->port_id_state = PORT_STATE_ALLOCATED;
                flow->dest_ha = gha_dup(dest_ha);

                if (kipcm_notify_flow_alloc_req_result(default_kipcm,
                                                       data->id,
                                                       flow->flow_id,
                                                       0)) {
                        flow_destroy(data, &flow);
                        LOG_ERR("Couldn't tell KIPCM flow is allocated");
                }
        }
}



static int eth_vlan_flow_allocate_request(struct ipcp_instance_data * data,
                                          const struct name *         source,
                                          const struct name *         dest,
                                          const struct flow_spec *    fspec,
                                          port_id_t                   id,
                                          flow_id_t                   fid)
{
        struct shim_eth_flow * flow;

        ASSERT(data);
        ASSERT(source);
        ASSERT(dest);

        if (!data->app_name || !name_cmp(source, data->app_name)) {
                LOG_ERR("Not the app that was registered");
                return -1;
        }


        flow = find_flow(data, id);
        if (!flow) {
                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow)
                        return -1;

                flow->port_id = id;
                flow->flow_id = fid;
                flow->port_id_state = PORT_STATE_PENDING;

                flow->dest_pa = name_to_gpa(dest);

                spin_lock(&data->lock);
                INIT_LIST_HEAD(&flow->list);
                list_add(&flow->list, &data->flows);
                spin_unlock(&data->lock);

                if (kfifo_alloc(&flow->sdu_queue, PAGE_SIZE, GFP_KERNEL)) {
                        LOG_ERR("Couldn't create the sdu queue"
                                "for a new flow");
                        flow_destroy(data, &flow);
                        return -1;
                }

                if (!rinarp_resolve_gpa(data->handle,
                                        flow->dest_pa,
                                        rinarp_resolve_handler,
                                        data)) {
                        LOG_ERR("Failed to lookup ARP entry");
                        return -1;
                }
        } else if (flow->port_id_state == PORT_STATE_PENDING) {
                flow->port_id_state = PORT_STATE_ALLOCATED;
                kipcm_flow_add(default_kipcm, data->id,
                               flow->port_id, flow->flow_id);
        } else {
                LOG_ERR("Allocate called in a wrong state. How dare you!");
                return -1;
        }

        return 0;
}

static int eth_vlan_flow_allocate_response(struct ipcp_instance_data * data,
                                           flow_id_t                   flow_id,
                                           port_id_t                   port_id,
                                           int                         result)
{
        struct shim_eth_flow * flow;
        struct sdu * du;

        ASSERT(data);
        ASSERT(is_flow_id_ok(flow_id));

        flow = find_flow_by_flow_id(data, flow_id);
        if (!flow) {
                LOG_ERR("Flow does not exist, you shouldn't call this");
                return -1;
        }

        if (flow->port_id_state != PORT_STATE_PENDING) {
                LOG_ERR("Flow is already allocated");
                return -1;
        }

        /*
         * On positive response, flow should transition to allocated state
         */
        if (!result) {
                flow->port_id = port_id;
                flow->port_id_state = PORT_STATE_ALLOCATED;
                kipcm_flow_add(default_kipcm, data->id,
                               flow->port_id, flow->flow_id);
                if (kipcm_notify_flow_alloc_req_result(default_kipcm,
                                                       data->id,
                                                       flow->flow_id,
                                                       0)) {
                        flow_destroy(data, &flow);
                        return -1;
                }
                du = NULL;
                while(kfifo_get(&flow->sdu_queue, du)) {
                        kfa_sdu_post(data->kfa, flow->port_id, du);
                }
        } else {
                flow->port_id_state = PORT_STATE_NULL;
                kfifo_free(&flow->sdu_queue);
        }

        return 0;
}

static int eth_vlan_flow_deallocate(struct ipcp_instance_data * data,
                                    port_id_t                   id)
{
        struct shim_eth_flow * flow;

        ASSERT(data);
        flow = find_flow(data, id);
        if (!flow) {
                LOG_ERR("Flow does not exist, cannot remove");
                return -1;
        }

        flow_destroy(data, &flow);

        return 0;
}

static int eth_vlan_application_register(struct ipcp_instance_data * data,
                                         const struct name *         name)
{
        struct gpa * pa;
        struct gha * ha;
        ASSERT(data);
        ASSERT(name);

        if (data->app_name) {
                char * tmp = name_tostring(data->app_name);
                LOG_ERR("Application %s is already registered", tmp);
                rkfree(tmp);
                return -1;
        }

        data->app_name = name_dup(name);
        if (!data->app_name) {
                char * tmp = name_tostring(name);
                LOG_ERR("Application %s registration has failed", tmp);
                rkfree(tmp);
                return -1;
        }

        pa = name_to_gpa(name);
        ha = gha_create(MAC_ADDR_802_3, data->dev->dev_addr);
        data->handle = rinarp_add(pa,ha);
        if (!data->handle) {
                LOG_ERR("Failed to register application in ARP");
                name_destroy(data->app_name);
                gpa_destroy(pa);
                gha_destroy(ha);
                return -1;
        }
        gpa_destroy(pa);
        gha_destroy(ha);

        return 0;
}

static int eth_vlan_application_unregister(struct ipcp_instance_data * data,
                                           const struct name *         name)
{
        ASSERT(data);
        ASSERT(name);
        if (!data->app_name) {
                LOG_ERR("Shim-eth-vlan has no application registered");
                return -1;
        }

        if (name_cmp(data->app_name,name)) {
                LOG_ERR("Application registered != application specified");
                return -1;
        }

        /* Remove from ARP cache */
        rinarp_remove(data->handle);

        name_destroy(data->app_name);
        data->app_name = NULL;
        return 0;
}

static int eth_vlan_sdu_write(struct ipcp_instance_data * data,
                              port_id_t                   id,
                              struct sdu *                sdu)
{
        struct shim_eth_flow * flow;
        struct sk_buff *     skb;
        const unsigned char *src_hw;
        struct rinarp_mac_addr *desthw;
        const unsigned char *dest_hw;
        unsigned char * sdu_ptr;
        int hlen, tlen, length;
        ASSERT(data);
        ASSERT(sdu);

        hlen = LL_RESERVED_SPACE(data->dev);
        tlen = data->dev->needed_tailroom;
        length = sdu->buffer->size;
        desthw = 0;

        flow = find_flow(data, id);
        if (!flow) {
                LOG_ERR("Flow does not exist, you shouldn't call this");
                return -1;
        }

        if (flow->port_id_state != PORT_STATE_ALLOCATED) {
                LOG_ERR("Flow is not in the right state to call this");
                return -1;
        }

        src_hw = data->dev->dev_addr;
        dest_hw = gha_address(flow->dest_ha);

        skb = alloc_skb(length + hlen + tlen, GFP_ATOMIC);
        if (skb == NULL)
                return -1;

        skb_reserve(skb, hlen);
        skb_reset_network_header(skb);
        sdu_ptr = (unsigned char *) skb_put(skb, sdu->buffer->size) + 1;
        memcpy(sdu_ptr, sdu->buffer->data, sdu->buffer->size);

        skb->dev = data->dev;
        skb->protocol = htons(ETH_P_RINA);

        if (dev_hard_header(skb, data->dev, ETH_P_RINA,
                            dest_hw, src_hw, skb->len) < 0) {
                kfree_skb(skb);
                rkfree(sdu);
                return -1;
        }

        dev_queue_xmit(skb);

        rkfree(sdu);

        return 0;
}

/* Filter the devices here. Accept packets from VLANs that are configured */
static int eth_vlan_rcv(struct sk_buff *     skb,
                        struct net_device *  dev,
                        struct packet_type * pt,
                        struct net_device *  orig_dev)
{
        struct ethhdr *mh;
        unsigned char * saddr;
        struct ipcp_instance_data * data;
        struct interface_data_mapping * mapping;
        struct shim_eth_flow * flow;
        struct gha * ghaddr;
        const struct gpa * gpaddr;
        struct sdu * du;
        unsigned char * nh;
        struct name * sname;

        /* C-c-c-checks */
        mapping = inst_data_mapping_get(dev);
        if (!mapping) {
                kfree_skb(skb);
                return -1;
        }

        data = mapping->data;
        if (!data) {
                kfree_skb(skb);
                return -1;
        }

        if (!data->app_name) {
                LOG_ERR("No app registered yet! "
                        "Someone is doing something bad on the network");
                kfree_skb(skb);
                return -1;
        }

        if (skb->pkt_type == PACKET_OTHERHOST ||
            skb->pkt_type == PACKET_LOOPBACK) {
                kfree_skb(skb);
                return -1;
        }

        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb) {
                LOG_ERR("Couldn't obtain ownership of the skb");
                return -1;
        }

        mh = eth_hdr(skb);
        saddr = mh->h_source;

        /* Get correct flow based on hwaddr */
        ghaddr = gha_create(MAC_ADDR_802_3, saddr);
        flow = find_flow_by_gha(data, ghaddr);

        /* Get the SDU out of the sk_buff */
        nh = skb_network_header(skb);
        du = sdu_create_from(nh, strlen(nh));

        /* If the flow cannot be found --> New Flow! */
        if (!flow) {
                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow)
                        return -1;
                spin_lock(&data->lock);
                INIT_LIST_HEAD(&flow->list);
                list_add(&flow->list, &data->flows);
                spin_unlock(&data->lock);

                flow->port_id_state = PORT_STATE_PENDING;
                flow->dest_ha = ghaddr;
                flow->flow_id = kfa_flow_create(data->kfa);

                sname  = NULL;
                gpaddr = rinarp_find_gpa(data->handle, flow->dest_ha);
                if (gpaddr) {
                        flow->dest_pa = gpa_dup(gpaddr);
                        sname = string_toname(gpa_address_value(gpaddr));
                }

                if (kfifo_alloc(&flow->sdu_queue, PAGE_SIZE, GFP_KERNEL)) {
                        LOG_ERR("Couldn't create the sdu queue"
                                "for a new flow");
                        flow_destroy(data, &flow);
                        return -1;
                }
                /* Store SDU in queue */
                kfifo_put(&flow->sdu_queue, du);

                if (kipcm_flow_arrived(default_kipcm,
                                       data->id,
                                       flow->flow_id,
                                       data->dif_name,
                                       sname,
                                       data->app_name,
                                       NULL)) {
                        LOG_ERR("Couldn't tell the KIPCM about the flow");
                        kfifo_free(&flow->sdu_queue);
                        flow_destroy(data, &flow);
                        return -1;
                }
        } else {
                gha_destroy(ghaddr);
                if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                        kfa_sdu_post(data->kfa, flow->port_id, du);
                } else if (flow->port_id_state == PORT_STATE_PENDING) {
                        kfifo_put(&flow->sdu_queue, du);
                }
        }

        kfree_skb(skb);
        return 0;
};

static int eth_vlan_assign_to_dif(struct ipcp_instance_data * data,
                                  const struct dif_info *     dif_information)
{
        struct eth_vlan_info *          info;
        struct ipcp_config *            tmp;
        string_t *                      complete_interface;
        struct interface_data_mapping * mapping;
        int result;

        ASSERT(data);
        ASSERT(dif_information);

        info = data->info;

        if (data->dif_name) {
                LOG_ERR("This IPC Process is already assigned to the DIF %s",
                        data->dif_name->process_name);
                LOG_ERR("An IPC Process can only be assigned to a DIF once");
                return -1;
        }

        /* Get vlan id */
        result = kstrtoul(dif_information->dif_name->process_name,
                          10, &(info->vlan_id));


        if (result) {
                LOG_ERR("Error converting DIF Name to VLAN ID: %s",
                        dif_information->dif_name->process_name);
                return -1;
        }

        data->dif_name = name_dup(dif_information->dif_name);
        if (!data->dif_name) {
                LOG_ERR("Error duplicating name, bailing out");
                return -1;
        }

        /* Retrieve configuration of IPC process from params */
        list_for_each_entry(tmp, &(dif_information->
                                   configuration->
                                   ipcp_config_entries), next) {
                const struct ipcp_config_entry * entry;

                entry = tmp->entry;
                if (!strcmp(entry->name,"interface-name")) {
                        info->interface_name =
                                kstrdup(entry->value, GFP_KERNEL);
                        if (!info->interface_name) {
                                LOG_ERR("Cannot copy interface name");
                                name_destroy(data->dif_name);
                                return -1;
                        }
                } else
                        LOG_WARN("Unknown config param for eth shim");
        }

        data->eth_vlan_packet_type->type = cpu_to_be16(ETH_P_RINA);
        data->eth_vlan_packet_type->func = eth_vlan_rcv;

        complete_interface =
                create_vlan_interface_name(info->interface_name,
                                           info->vlan_id);
        if (!complete_interface) {
                name_destroy(data->dif_name);
                rkfree(info->interface_name);
                return -1;
        }

        /* Add the handler */
        read_lock(&dev_base_lock);
        data->dev = __dev_get_by_name(&init_net, complete_interface);
        if (!data->dev) {
                LOG_ERR("Invalid device to configure: %s",
                        complete_interface);
                read_unlock(&dev_base_lock);
                name_destroy(data->dif_name);
                rkfree(info->interface_name);
                rkfree(complete_interface);
                return -1;
        }

        LOG_DBG("Got device driver of %s, trying to register handler",
                complete_interface);

        /* Store in list for retrieval later on */
        mapping = rkmalloc(sizeof(*mapping), GFP_KERNEL);
        if (!mapping) {
                read_unlock(&dev_base_lock);
                name_destroy(data->dif_name);
                rkfree(info->interface_name);
                rkfree(complete_interface);
                return -1;
        }
        mapping->dev = data->dev;
        mapping->data = data;
        INIT_LIST_HEAD(&mapping->list);

        spin_lock(&data_instances_lock);
        list_add(&mapping->list, &data_instances_list);
        spin_unlock(&data_instances_lock);

        data->eth_vlan_packet_type->dev = data->dev;
        dev_add_pack(data->eth_vlan_packet_type);
        read_unlock(&dev_base_lock);
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

        ASSERT(data);
        ASSERT(new_config);

        info        = data->info;

        /* Get configuration struct pertaining to this shim instance */
        old_interface_name = info->interface_name;

        /* Retrieve configuration of IPC process from params */
        list_for_each_entry(tmp, &(new_config->ipcp_config_entries), next) {
                const struct ipcp_config_entry * entry;

                entry = tmp->entry;
                if (!strcmp(entry->name,"interface-name")) {
                        if (!strcpy(info->interface_name,
                                    entry->value)) {
                                LOG_ERR("Cannot copy interface name");
                        }
                } else {
                        LOG_WARN("Unknown config param for eth shim");
                }
        }


        dev_remove_pack(data->eth_vlan_packet_type);
        /* Remove from list */
        mapping = inst_data_mapping_get(data->dev);
        if (mapping) {
                list_del(&mapping->list);
                rkfree(&mapping);
        }

        data->eth_vlan_packet_type->type = cpu_to_be16(ETH_P_RINA);
        data->eth_vlan_packet_type->func = eth_vlan_rcv;

        complete_interface =
                create_vlan_interface_name(info->interface_name,
                                           info->vlan_id);
        if (!complete_interface)
                return -1;

        /* Add the handler */
        read_lock(&dev_base_lock);
        data->dev = __dev_get_by_name(&init_net, complete_interface);
        if (!data->dev) {
                LOG_ERR("Invalid device to configure: %s",
                        complete_interface);
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
        read_unlock(&dev_base_lock);
        rkfree(complete_interface);

        LOG_DBG("Configured shim eth vlan IPC Process");

        return 0;
}

static struct ipcp_instance_ops eth_vlan_instance_ops = {
        .flow_allocate_request  = eth_vlan_flow_allocate_request,
        .flow_allocate_response = eth_vlan_flow_allocate_response,
        .flow_deallocate        = eth_vlan_flow_deallocate,
        .application_register   = eth_vlan_application_register,
        .application_unregister = eth_vlan_application_unregister,
        .sdu_write              = eth_vlan_sdu_write,
        .assign_to_dif          = eth_vlan_assign_to_dif,
        .update_dif_config      = eth_vlan_update_dif_config,
};

static struct ipcp_factory_data {
        struct list_head instances;
} eth_vlan_data;

static int eth_vlan_init(struct ipcp_factory_data * data)
{
        ASSERT(data);

        bzero(&eth_vlan_data, sizeof(eth_vlan_data));
        INIT_LIST_HEAD(&(data->instances));

        INIT_LIST_HEAD(&data_instances_list);

        LOG_INFO("%s intialized", SHIM_NAME);

        return 0;
}

static int eth_vlan_fini(struct ipcp_factory_data * data)
{

        ASSERT(data);

        ASSERT(list_empty(&(data->instances)));

        return 0;
}

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

static struct ipcp_instance * eth_vlan_create(struct ipcp_factory_data * data,
                                              const struct name *        name,
                                              ipc_process_id_t           id)
{
        struct ipcp_instance * inst;

        ASSERT(data);

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
        inst->data = rkzalloc(sizeof(struct ipcp_instance_data), GFP_KERNEL);
        if (!inst->data) {
                rkfree(inst);
                return NULL;
        }

        inst->data->eth_vlan_packet_type =
                rkzalloc(sizeof(struct packet_type), GFP_KERNEL);
        if (!inst->data->eth_vlan_packet_type) {
                LOG_DBG("Failed creation of inst->data->eth_vlan_packet_type");
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->id = id;

        inst->data->name = name_dup(name);
        if (!inst->data->name) {
                LOG_DBG("Failed creation of ipc name");
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->info = rkzalloc(sizeof(*inst->data->info), GFP_KERNEL);
        if (!inst->data->info) {
                LOG_DBG("Failed creation of inst->data->info");
                rkfree(inst->data->eth_vlan_packet_type);
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        /* FIXME: Remove as soon as the kipcm_kfa gets removed*/
        inst->data->kfa = kipcm_kfa(default_kipcm);
        LOG_DBG("KFA instance %pK bound to the shim eth", inst->data->kfa);

        spin_lock_init(&inst->data->lock);

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
        struct ipcp_instance_data * pos, * next;

        ASSERT(data);
        ASSERT(instance);

        /* Retrieve the instance */
        list_for_each_entry_safe(pos, next, &data->instances, list) {
                if (pos->id == instance->data->id) {

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

                        if (pos->info->interface_name)
                                rkfree(pos->info->interface_name);

                        if (pos->info)
                                rkfree(pos->info);

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

struct ipcp_factory * shim = NULL;

static int __init mod_init(void)
{
        shim =  kipcm_ipcp_factory_register(default_kipcm,
                                            SHIM_NAME,
                                            &eth_vlan_data,
                                            &eth_vlan_ops);
        if (!shim) {
                LOG_CRIT("Cannot register %s factory", SHIM_NAME);
                return -1;
        }

        spin_lock_init(&data_instances_lock);

        return 0;
}

static void __exit mod_exit(void)
{
        ASSERT(shim);

        if (kipcm_ipcp_factory_unregister(default_kipcm, shim)) {
                LOG_CRIT("Cannot unregister %s factory", SHIM_NAME);
                return;
        }
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Shim IPC over Ethernet");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Leonardo Bergesio <leonardo.bergesio@i2cat.net>");
