/*
 * Shim IPC Process over Ethernet (using VLANs)
 *
 *   Francesco Salvestrini <f.salvestrini@nextworks.it>
 *   Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *   Miquel Tarzan         <miquel.tarzan@i2cat.net>
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
#include <linux/workqueue.h>

#define PROTO_LEN   32
#define SHIM_NAME   "shim-eth-vlan"

#define RINA_PREFIX SHIM_NAME

#include "logs.h"
#include "common.h"
#include "kipcm.h"
#include "debug.h"
#include "utils.h"
#include "du.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"
#include "rinarp/rinarp.h"
#include "rinarp/arp826-utils.h"

/* FIXME: To be solved properly */
static struct workqueue_struct * rcv_wq;
static struct work_struct        rcv_work;
static struct list_head          rcv_wq_packets;
static DEFINE_SPINLOCK(rcv_wq_lock);

struct rcv_struct {
        struct list_head     list;
        struct sk_buff *     skb;
        struct net_device *  dev;
        struct packet_type * pt;
        struct net_device *  orig_dev;
};

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

/* Holds the configuration of one shim instance */
struct eth_vlan_info {
        uint16_t     vlan_id;
        char *       interface_name;
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
        port_id_t          port_id;
        enum port_id_state port_id_state;

        /* Used when flow is not allocated yet */
        struct rfifo *     sdu_queue;
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
        struct flow_spec *     fspec;

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

static DEFINE_SPINLOCK(data_instances_lock);
static struct list_head data_instances_list;

static struct interface_data_mapping *
inst_data_mapping_get(struct net_device * dev)
{
        struct interface_data_mapping * mapping;

        if (!dev)
                return NULL;

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

static struct gpa * name_to_gpa(const struct name * name)
{
        char *       tmp;
        struct gpa * gpa;

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

        if (!data || !gha_is_ok(addr))
                return NULL;

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

        if (!data || !gpa_is_ok(addr))
                return NULL;

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

static bool vlan_id_is_ok(uint16_t vlan_id)
{
        if (vlan_id > 4095 /* 0xFFF */) {
                /* Out of bounds */
                return false;
        }

        ASSERT(vlan_id <= 4095);

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

        if (!interface_name)
                return NULL;

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
                        struct shim_eth_flow *     flow)
{
        if (!data || !flow) {
                LOG_ERR("Couldn't destroy flow.");
                return -1;
        }

        spin_lock(&data->lock);
        if (!list_empty(&flow->list)) {
                list_del(&flow->list);
        }
        spin_unlock(&data->lock);

        if (flow->dest_pa) gpa_destroy(flow->dest_pa);
        if (flow->dest_ha) gha_destroy(flow->dest_ha);
        if (flow->sdu_queue)
                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) pdu_destroy);
        rkfree(flow);

        return 0;
}

static void deallocate_and_destroy_flow(struct ipcp_instance_data * data,
                                        struct shim_eth_flow *      flow)
{
        if (kfa_flow_deallocate(data->kfa, flow->port_id))
                LOG_ERR("Failed to destroy KFA flow");
        if (flow_destroy(data, flow))
                LOG_ERR("Failed to destroy shim-eth-vlan flow");
}

static void rinarp_resolve_handler(void *             opaque,
                                   const struct gpa * dest_pa,
                                   const struct gha * dest_ha)
{
        struct ipcp_instance_data * data;
        struct shim_eth_flow *      flow;

        LOG_DBG("Entered the ARP resolve handler of the shim-eth");

        data = (struct ipcp_instance_data *) opaque;
        flow = find_flow_by_gpa(data, dest_pa);
        if (!flow) {
                LOG_ERR("No flow found for this dest_pa");
                return;
        }

        spin_lock(&data->lock);
        if (flow->port_id_state == PORT_STATE_PENDING) {
                flow->port_id_state = PORT_STATE_ALLOCATED;
                spin_unlock(&data->lock);

                flow->dest_ha = gha_dup_ni(dest_ha);

                if (kipcm_flow_commit(default_kipcm,
                                      data->id, flow->port_id)) {
                        LOG_ERR("Cannot add flow");
                        deallocate_and_destroy_flow(data, flow);
                        return;
                }

                ASSERT(flow->sdu_queue);

                while (!rfifo_is_empty(flow->sdu_queue)) {
                        struct sdu * tmp = NULL;

                        tmp = rfifo_pop(flow->sdu_queue);
                        ASSERT(tmp);

                        LOG_DBG("Got a new element from the fifo");

                        if (kfa_sdu_post(data->kfa, flow->port_id, tmp)) {
                                LOG_ERR("Couldn't post SDU to KFA ...");
                                return;
                        }
                }

                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) pdu_destroy);
                flow->sdu_queue = NULL;

                if (kipcm_notify_flow_alloc_req_result(default_kipcm,
                                                       data->id,
                                                       flow->port_id,
                                                       0)) {
                        LOG_ERR("Couldn't tell flow is allocated to KIPCM");
                        deallocate_and_destroy_flow(data, flow);
                        return;
                }
        } else {
                spin_unlock(&data->lock);
        }
}

static int eth_vlan_flow_allocate_request(struct ipcp_instance_data * data,
                                          const struct name *         source,
                                          const struct name *         dest,
                                          const struct flow_spec *    fspec,
                                          port_id_t                   id)
{
        struct shim_eth_flow * flow;

        ASSERT(data);
        ASSERT(source);
        ASSERT(dest);

        if (!data->app_name || !name_is_equal(source, data->app_name)) {
                LOG_ERR("Wrong request, app is not registered");
                if (kfa_flow_deallocate(data->kfa, id))
                        LOG_ERR("Failed to destroy KFA flow");
                return -1;
        }

        flow = find_flow(data, id);
        if (!flow) {
                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow)
                        return -1;

                flow->port_id       = id;
                flow->port_id_state = PORT_STATE_PENDING;
                flow->dest_pa       = name_to_gpa(dest);

                if (!gpa_is_ok(flow->dest_pa)) {
                        LOG_ERR("Destination protocol address is not ok");
                        deallocate_and_destroy_flow(data, flow);
                        return -1;
                }

                INIT_LIST_HEAD(&flow->list);
                spin_lock(&data->lock);
                list_add(&flow->list, &data->flows);
                spin_unlock(&data->lock);

                flow->sdu_queue = rfifo_create();
                if (!flow->sdu_queue) {
                        LOG_ERR("Couldn't create the sdu queue "
                                "for a new flow");
                        deallocate_and_destroy_flow(data, flow);
                        return -1;
                }

                if (rinarp_resolve_gpa(data->handle,
                                       flow->dest_pa,
                                       rinarp_resolve_handler,
                                       data)) {
                        LOG_ERR("Failed to lookup ARP entry");
                        deallocate_and_destroy_flow(data, flow);
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

static int eth_vlan_flow_allocate_response(struct ipcp_instance_data * data,
                                           port_id_t                   port_id,
                                           int                         result)
{
        struct shim_eth_flow * flow;

        ASSERT(data);
        ASSERT(is_port_id_ok(port_id));

        flow = find_flow(data, port_id);
        if (!flow) {
                LOG_ERR("Flow does not exist, you shouldn't call this");
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
                flow->port_id = port_id;
                if (kipcm_flow_commit(default_kipcm, data->id,
                                      flow->port_id)) {
                        LOG_ERR("KIPCM flow add failed");
                        deallocate_and_destroy_flow(data, flow);
                        return -1;
                }

                spin_lock(&data->lock);
                flow->port_id_state = PORT_STATE_ALLOCATED;
                spin_unlock(&data->lock);

                ASSERT(flow->sdu_queue);

                while (!rfifo_is_empty(flow->sdu_queue)) {
                        struct sdu * tmp = NULL;

                        tmp = rfifo_pop(flow->sdu_queue);
                        ASSERT(tmp);

                        LOG_DBG("Got a new element from the fifo");

                        if (kfa_sdu_post(data->kfa, flow->port_id, tmp)) {
                                LOG_ERR("Couldn't post SDU to KFA ...");
                                return -1;
                        }
                }

                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) pdu_destroy);
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
                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) pdu_destroy);
                flow->sdu_queue = NULL;
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

        if (kfa_flow_deallocate(data->kfa, flow->port_id)) {
                LOG_ERR("Failed to deallocate flow in KFA");
                return -1;
        }

        if (flow_destroy(data, flow)) {
                LOG_ERR("Failed to destroy shim-eth-vlan flow");
                return -1;
        }

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
        data->handle = rinarp_add(data->dev, pa, ha);
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

        if (!name_is_equal(data->app_name, name)) {
                LOG_ERR("Application registered != application specified");
                return -1;
        }

        /* Remove from ARP cache */
        if (data->handle) {
                if (rinarp_remove(data->handle)) {
                        LOG_ERR("Failed to remove the entry from the cache");
                        return -1;
                }
                data->handle = NULL;
        }

        name_destroy(data->app_name);
        data->app_name = NULL;
        return 0;
}

static int eth_vlan_sdu_write(struct ipcp_instance_data * data,
                              port_id_t                   id,
                              struct sdu *                sdu)
{
        struct shim_eth_flow *   flow;
        struct sk_buff *         skb;
        const unsigned char *    src_hw;
        struct rinarp_mac_addr * desthw;
        const unsigned char *    dest_hw;
        unsigned char *          sdu_ptr;
        int                      hlen, tlen, length;
        int                      retval;

        ASSERT(data);
        ASSERT(sdu);

        LOG_DBG("Entered the sdu-write");

        hlen   = LL_RESERVED_SPACE(data->dev);
        tlen   = data->dev->needed_tailroom;
        length = buffer_length(sdu->buffer);
        desthw = 0;

        flow = find_flow(data, id);
        if (!flow) {
                LOG_ERR("Flow does not exist, you shouldn't call this");
                sdu_destroy(sdu);
                return -1;
        }

        spin_lock(&data->lock);
        if (flow->port_id_state != PORT_STATE_ALLOCATED) {
                LOG_ERR("Flow is not in the right state to call this");
                sdu_destroy(sdu);
                spin_unlock(&data->lock);
                return -1;
        }
        spin_unlock(&data->lock);

        src_hw = data->dev->dev_addr;
        if (!src_hw) {
                LOG_ERR("Failed to get source HW addr");
                sdu_destroy(sdu);
                return -1;
        }

        dest_hw = gha_address(flow->dest_ha);
        if (!dest_hw) {
                LOG_ERR("Destination HW address is unknown");
                sdu_destroy(sdu);
                return -1;
        }

        skb = alloc_skb(length + hlen + tlen, GFP_ATOMIC);
        if (skb == NULL) {
                LOG_ERR("Cannot allocate a skb");
                sdu_destroy(sdu);
                return -1;
        }

        skb_reserve(skb, hlen);
        skb_reset_network_header(skb);
        sdu_ptr = (unsigned char *) skb_put(skb, buffer_length(sdu->buffer));

        if (!memcpy(sdu_ptr,
                    buffer_data_ro(sdu->buffer),
                    buffer_length(sdu->buffer))) {
                LOG_ERR("Memcpy failed");
                kfree_skb(skb);
                sdu_destroy(sdu);
                return -1;
        }

        sdu_destroy(sdu);

        skb->dev      = data->dev;
        skb->protocol = htons(ETH_P_RINA);

        retval = dev_hard_header(skb, data->dev,
                                 ETH_P_RINA, dest_hw, src_hw, skb->len);
        if (retval < 0) {
                LOG_ERR("Problems in dev_hard_header (%d)", retval);
                kfree_skb(skb);
                return -1;
        }

        retval = dev_queue_xmit(skb);
        if (retval != NET_XMIT_SUCCESS) {
                LOG_ERR("Problems in dev_queue_xmit (%d)", retval);
                return -1;
        }

        LOG_DBG("Packet sent");

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
        const struct gpa *              gpaddr;
        struct sdu *                    du;
        struct buffer *                 buffer;
        struct name *                   sname;
        char *                          sk_data;

        /* C-c-c-checks */
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

        mh    = eth_hdr(skb);
        saddr = mh->h_source;
        if (!saddr) {
                LOG_ERR("Couldn't get source address");
                kfree_skb(skb);
                return -1;
        }

        /* Get correct flow based on hwaddr */
        ghaddr = gha_create_gfp(GFP_KERNEL, MAC_ADDR_802_3, saddr);
        if (!ghaddr) {
                kfree_skb(skb);
                return -1;
        }
        ASSERT(gha_is_ok(ghaddr));

        /* Get the SDU out of the sk_buff */
        sk_data = rkmalloc(skb->len, GFP_KERNEL);
        if (!sk_data) {
                gha_destroy(ghaddr);
                kfree_skb(skb);
                return -1;
        }

        /*
         * FIXME: We should avoid this extra copy, but then we cannot free the
         *        skb at the end of the eth_vlan_rcv function. To do so we
         *        have to either find a way to free all the data of the skb
         *        except for the SDU, or delay freeing the skb until it is
         *        safe to do so.
         */

        if (skb_copy_bits(skb, 0, sk_data, skb->len)) {
                LOG_ERR("Failed to copy data from sk_buff");
                rkfree(sk_data);
                gha_destroy(ghaddr);
                kfree_skb(skb);
                return -1;
        }

        buffer = buffer_create_with_ni(sk_data, skb->len);
        if (!buffer) {
                rkfree(sk_data);
                gha_destroy(ghaddr);
                kfree_skb(skb);
                return -1;
        }

        /* We're done with the skb from this point on so ... let's get rid */
        kfree_skb(skb);

        du = sdu_create_buffer_with_ni(buffer);
        if (!du) {
                LOG_ERR("Couldn't create data unit");
                buffer_destroy(buffer);
                gha_destroy(ghaddr);
                return -1;
        }

        spin_lock(&data->lock);

        flow = find_flow_by_gha(data, ghaddr);

        if (!flow) {
                LOG_DBG("Have to create a new flow");

                flow = rkzalloc(sizeof(*flow), GFP_ATOMIC);
                if (!flow) {
                        flow->port_id_state = PORT_STATE_NULL;
                        spin_unlock(&data->lock);
                        sdu_destroy(du);
                        gha_destroy(ghaddr);
                        return -1;
                }

                flow->port_id_state = PORT_STATE_PENDING;
                INIT_LIST_HEAD(&flow->list);
                list_add(&flow->list, &data->flows);
                flow->dest_ha       = ghaddr;
                flow->port_id       = kfa_port_id_reserve(data->kfa, data->id);

                if (!is_port_id_ok(flow->port_id)) {
                        LOG_DBG("Port id is not ok");
                        flow->port_id_state = PORT_STATE_NULL;
                        spin_unlock(&data->lock);
                        sdu_destroy(du);
                        gha_destroy(ghaddr);
                        if (flow_destroy(data, flow))
                                LOG_ERR("Problems destroying shim-eth-vlan "
                                        "flow");
                        return -1;
                }

                if (kfa_flow_create(data->kfa, data->id, flow->port_id)) {
                        LOG_DBG("Could not create flow in KFA");
                        flow->port_id_state = PORT_STATE_NULL;
                        kfa_port_id_release(data->kfa, flow->port_id);
                        spin_unlock(&data->lock);
                        sdu_destroy(du);
                        gha_destroy(ghaddr);
                        if (flow_destroy(data, flow))
                                LOG_ERR("Problems destroying shim-eth-vlan "
                                        "flow");
                        return -1;
                }

                LOG_DBG("Added flow to the list");

                flow->sdu_queue = rfifo_create();
                if (!flow->sdu_queue) {
                        LOG_ERR("Couldn't create the sdu queue "
                                "for a new flow");
                        spin_unlock(&data->lock);
                        sdu_destroy(du);
                        deallocate_and_destroy_flow(data, flow);
                        return -1;
                }

                LOG_DBG("Created the queue");

                /* Store SDU in queue */
                if (rfifo_push(flow->sdu_queue, du)) {
                        LOG_ERR("Could not push a SDU into the flow queue");
                        spin_unlock(&data->lock);
                        sdu_destroy(du);
                        deallocate_and_destroy_flow(data, flow);
                        return -1;
                }

                spin_unlock(&data->lock);

                sname  = NULL;
                gpaddr = rinarp_find_gpa(data->handle, flow->dest_ha);
                if (gpaddr && gpa_is_ok(gpaddr)) {
                        flow->dest_pa = gpa_dup_gfp(GFP_KERNEL, gpaddr);
                        if (!flow->dest_pa) {
                                LOG_ERR("Failed to duplicate gpa");
                                sdu_destroy(du);
                                deallocate_and_destroy_flow(data, flow);
                                return -1;
                        }

                        sname = string_toname_ni(gpa_address_value(gpaddr));
                        if (!sname) {
                                LOG_ERR("Failed to convert name to string");
                                deallocate_and_destroy_flow(data, flow);
                                return -1;
                        }

                        LOG_DBG("Got the address from ARP");
                } else {
                        sname = name_create_ni();
                        if (!name_init_from_ni(sname,
                                               "Unknown app", "", "", "")) {
                                name_destroy(sname);
                                return -1;
                        }

                        flow->dest_pa = name_to_gpa(sname);
                        if (!flow->dest_pa) {
                                LOG_ERR("Failed to create gpa");
                                deallocate_and_destroy_flow(data, flow);
                                return -1;
                        }

                        LOG_DBG("Flow request from unkown app received");
                }

                if (kipcm_flow_arrived(default_kipcm,
                                       data->id,
                                       flow->port_id,
                                       data->dif_name,
                                       sname,
                                       data->app_name,
                                       data->fspec)) {
                        LOG_ERR("Couldn't tell the KIPCM about the flow");
                        deallocate_and_destroy_flow(data, flow);
                        name_destroy(sname);
                        return -1;
                }
                name_destroy(sname);
        } else {
                gha_destroy(ghaddr);
                LOG_DBG("Flow exists, queueing or delivering or dropping");
                if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                        spin_unlock(&data->lock);

                        if (kfa_sdu_post(data->kfa, flow->port_id, du))
                                return -1;

                } else if (flow->port_id_state == PORT_STATE_PENDING) {
                        LOG_DBG("Queueing frame");

                        if (rfifo_push(flow->sdu_queue, du)) {
                                LOG_ERR("Failed to write %zd bytes"
                                        "into the fifo",
                                        sizeof(struct sdu *));
                                spin_unlock(&data->lock);
                                sdu_destroy(du);
                                return -1;
                        }

                        spin_unlock(&data->lock);
                } else if (flow->port_id_state == PORT_STATE_NULL) {
                        spin_unlock(&data->lock);
                        sdu_destroy(du);
                }
        }

        return 0;
}

static void eth_vlan_rcv_worker(struct work_struct *work)
{
        struct rcv_struct * packet, * next;
        unsigned long       flags;
        int                 num_frames;

        LOG_DBG("Worker waking up");

        spin_lock_irqsave(&rcv_wq_lock, flags);

        num_frames = 0;
        list_for_each_entry_safe(packet, next, &rcv_wq_packets, list) {
                spin_unlock_irqrestore(&rcv_wq_lock, flags);

                /* Call eth_vlan_recv_process_packet */
                if (eth_vlan_recv_process_packet(packet->skb, packet->dev))
                        LOG_DBG("Failed to process packet");

                num_frames++;

                spin_lock_irqsave(&rcv_wq_lock, flags);
                list_del(&packet->list);
                spin_unlock_irqrestore(&rcv_wq_lock, flags);

                rkfree(packet);

#ifdef CONFIG_RINA_SHIM_ETH_VLAN_BURST_LIMITING
                BUILD_BUG_ON(CONFIG_RINA_SHIM_ETH_VLAN_BURST_LIMIT <= 0);

                if (num_frames >= CONFIG_RINA_SHIM_ETH_VLAN_BURST_LIMIT)
                        return;
#endif

                spin_lock_irqsave(&rcv_wq_lock, flags);
        }

        spin_unlock_irqrestore(&rcv_wq_lock, flags);
        LOG_DBG("Worker finished for now, processed %d frames", num_frames);
}

static int eth_vlan_rcv(struct sk_buff *     skb,
                        struct net_device *  dev,
                        struct packet_type * pt,
                        struct net_device *  orig_dev)
{

        struct rcv_struct * packet;

        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb) {
                LOG_ERR("Couldn't obtain ownership of the skb");
                return 0;
        }

        packet = rkmalloc(sizeof(*packet), GFP_ATOMIC);
        if (!packet) {
                LOG_ERR("Couldn't malloc packet");
                return 0;
        }

        packet->skb      = skb;
        packet->dev      = dev;
        packet->pt       = pt;
        packet->orig_dev = orig_dev;
        INIT_LIST_HEAD(&packet->list);

        spin_lock(&rcv_wq_lock);
        list_add_tail(&packet->list, &rcv_wq_packets);
        spin_unlock(&rcv_wq_lock);

        queue_work(rcv_wq, &rcv_work);

        return 0;
};

static int eth_vlan_assign_to_dif(struct ipcp_instance_data * data,
                                  const struct dif_info *     dif_information)
{
        struct eth_vlan_info *          info;
        struct ipcp_config *            tmp;
        string_t *                      complete_interface;
        struct interface_data_mapping * mapping;
        int                             result;
        unsigned int                    temp_vlan;

        ASSERT(data);
        ASSERT(dif_information);

        info = data->info;

        if (data->dif_name) {
                ASSERT(data->dif_name->process_name);

                LOG_ERR("This IPC Process is already assigned to the DIF %s. "
                        "An IPC Process can only be assigned to a DIF once",
                        data->dif_name->process_name);
                return -1;
        }

        /* Get vlan id */
        result = kstrtouint(dif_information->dif_name->process_name,
                            10, &temp_vlan);
        if (result) {
                ASSERT(dif_information->dif_name->process_name);

                LOG_ERR("Error converting DIF Name to VLAN ID: %s",
                        dif_information->dif_name->process_name);
                return -1;
        }
        info->vlan_id = (uint16_t) temp_vlan;

        if (!vlan_id_is_ok(info->vlan_id)) {
                LOG_ERR("Bad vlan id specified: %d", info->vlan_id);
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
                const struct ipcp_config_entry * entry = tmp->entry;
                if (!strcmp(entry->name, "interface-name")) {
                        ASSERT(entry->value);

                        info->interface_name = rkstrdup_ni(entry->value);
                        if (!info->interface_name) {
                                LOG_ERR("Cannot copy interface name");
                                name_destroy(data->dif_name);
                                data->dif_name = NULL;
                                return -1;
                        }
                } else
                        LOG_WARN("Unknown config param for eth shim");
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

        complete_interface = create_vlan_interface_name(info->interface_name,
                                                        info->vlan_id);
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
        if (!data->dev) {
                LOG_ERR("Can't get device '%s'", complete_interface);
                read_unlock(&dev_base_lock);
                name_destroy(data->dif_name);
                data->dif_name = NULL;
                rkfree(info->interface_name);
                info->interface_name = NULL;
                rkfree(complete_interface);
                return -1;
        }

        LOG_DBG("Got device '%s', trying to register handler",
                complete_interface);

        /* Store in list for retrieval later on */
        mapping = rkmalloc(sizeof(*mapping), GFP_ATOMIC);
        if (!mapping) {
                read_unlock(&dev_base_lock);
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

        /* Get configuration struct pertaining to this shim instance */
        info               = data->info;
        old_interface_name = info->interface_name;

        /* Retrieve configuration of IPC process from params */
        list_for_each_entry(tmp, &(new_config->ipcp_config_entries), next) {
                const struct ipcp_config_entry * entry;

                entry = tmp->entry;
                if (!strcmp(entry->name,"interface-name")) {
                        if (!strcpy(info->interface_name,
                                    entry->value)) {
                                LOG_ERR("Cannot copy interface name");
                                return -1;
                        }
                } else {
                        LOG_WARN("Unknown config param for eth shim");
                        continue;
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

        complete_interface = create_vlan_interface_name(info->interface_name,
                                                        info->vlan_id);
        if (!complete_interface)
                return -1;

        /* Add the handler */
        read_lock(&dev_base_lock);
        data->dev = __dev_get_by_name(&init_net, complete_interface);
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
        read_unlock(&dev_base_lock);
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

static struct ipcp_instance_ops eth_vlan_instance_ops = {
        .flow_allocate_request     = eth_vlan_flow_allocate_request,
        .flow_allocate_response    = eth_vlan_flow_allocate_response,
        .flow_deallocate           = eth_vlan_flow_deallocate,
        .flow_binding_ipcp         = NULL,
        .flow_destroy              = NULL,

        .application_register      = eth_vlan_application_register,
        .application_unregister    = eth_vlan_application_unregister,

        .assign_to_dif             = eth_vlan_assign_to_dif,
        .update_dif_config         = eth_vlan_update_dif_config,

        .connection_create         = NULL,
        .connection_update         = NULL,
        .connection_destroy        = NULL,
        .connection_create_arrived = NULL,

        .sdu_enqueue               = NULL,
        .sdu_write                 = eth_vlan_sdu_write,

        .mgmt_sdu_read             = NULL,
        .mgmt_sdu_write            = NULL,
        .mgmt_sdu_post             = NULL,

        .pft_add                   = NULL,
        .pft_remove                = NULL,
        .pft_dump                  = NULL,
        .pft_flush                 = NULL,

        .ipcp_name                 = eth_vlan_ipcp_name,
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

        INIT_LIST_HEAD(&rcv_wq_packets);

        INIT_WORK(&rcv_work, eth_vlan_rcv_worker);

        LOG_INFO("%s initialized", SHIM_NAME);

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
                LOG_ERR("Instance creation failed (#1)");
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->id = id;

        inst->data->name = name_dup(name);
        if (!inst->data->name) {
                LOG_ERR("Failed creation of ipc name");
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->info = rkzalloc(sizeof(*inst->data->info), GFP_KERNEL);
        if (!inst->data->info) {
                LOG_ERR("Instance creation failed (#2)");
                rkfree(inst->data->eth_vlan_packet_type);
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->fspec = rkzalloc(sizeof(*inst->data->fspec), GFP_KERNEL);
        if (!inst->data->fspec) {
                LOG_ERR("Instance creation failed (#3)");
                rkfree(inst->data->info);
                rkfree(inst->data->eth_vlan_packet_type);
                rkfree(inst->data);
                rkfree(inst);
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
        struct ipcp_instance_data * pos, * next;

        ASSERT(data);
        ASSERT(instance);

        LOG_DBG("Looking for the eth-vlan-instance to destroy");

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

                        if (pos->fspec)
                                rkfree(pos->fspec);

                        if (pos->handle) {
                                if (rinarp_remove(pos->handle)) {
                                        LOG_ERR("Failed to remove "
                                                "the entry from the cache");
                                        return -1;
                                }
                        }

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

static struct ipcp_factory * shim = NULL;

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

        shim = kipcm_ipcp_factory_register(default_kipcm,
                                           SHIM_NAME,
                                           &eth_vlan_data,
                                           &eth_vlan_ops);
        if (!shim)
                return -1;

        spin_lock_init(&data_instances_lock);

        return 0;
}

static void __exit mod_exit(void)
{
        struct rcv_struct *  packet;

        ASSERT(shim);

        flush_workqueue(rcv_wq);
        destroy_workqueue(rcv_wq);

        list_for_each_entry(packet, &rcv_wq_packets, list) {
                kfree_skb(packet->skb);
                list_del(&packet->list);
                rkfree(packet);
        }

        kipcm_ipcp_factory_unregister(default_kipcm, shim);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Shim IPC over Ethernet");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Leonardo Bergesio <leonardo.bergesio@i2cat.net>");
