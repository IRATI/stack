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
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/if_packet.h>

#define RINA_PREFIX "shim-eth"

#include "logs.h"
#include "common.h"
#include "shim.h"
#include "kipcm.h"

/* Holds the configuration of one shim IPC process */
struct eth_vlan_info {
        uint16_t        vlan_id;
        char *          interface_name;
        struct name_t * name;
};

enum port_id_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_RECIPIENT_ALLOCATE_PENDING,
        PORT_STATE_INITIATOR_ALLOCATE_PENDING,
        PORT_STATE_ALLOCATED
};

/* Hold the information related to one flow*/
struct shim_eth_flow {
        uint64_t             src_mac;
        uint64_t             dst_mac;
        port_id_t            port_id;
        enum port_id_state   port_id_state;

        /* FIXME: Will be a kfifo holding the SDUs or a sk_buff_head */
        /* QUEUE(sdu_queue, sdu_t *); */
};

/*
 * Contains all the information associated to an instance of a
 * shim Ethernet IPC Process
 */
struct eth_vlan_instance {
        struct rb_node         node;

        /* IPC process id and DIF name */
        ipc_process_id_t       id;

        /* The configuration of the shim IPC Process */
        struct eth_vlan_info * info;

        /* FIXME: Pointer to the device driver data_structure */
        /* Unsure if really needed */
        /* device_t * device_driver; */

        /* FIXME: Stores the state of flows indexed by port_id */
        /* HASH_TABLE(flow_to_port_id, port_id_t, shim_eth_flow_t); */
        /* rbtree or hash table? */
};


static int eth_vlan_flow_allocate_request(struct shim_instance_data * data,
                                          const struct name_t *       source,
                                          const struct name_t *       dest,
                                          const struct flow_spec_t *  flowspec,
                                          port_id_t *                 port_id)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int eth_vlan_flow_allocate_response(struct shim_instance_data * data,
                                           port_id_t                   port_id,
                                           response_reason_t *         resp)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int eth_vlan_flow_deallocate(struct shim_instance_data * data,
                                    port_id_t                   port_id)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int eth_vlan_application_register(struct shim_instance_data * data,
                                         const struct name_t *       name)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int eth_vlan_application_unregister(struct shim_instance_data * data,
                                           const struct name_t *       name)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int eth_vlan_sdu_write(struct shim_instance_data * data,
                              port_id_t                   port_id,
                              const struct sdu_t *        sdu)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int eth_vlan_sdu_read(struct shim_instance_data * data,
                             port_id_t                   id,
                             struct sdu_t *              sdu)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

/* Filter the devices here. Accept packets from VLANs that are configured */
static int eth_vlan_rcv(struct sk_buff *     skb,
                        struct net_device *  dev,
                        struct packet_type * pt,
                        struct net_device *  orig_dev)
{
        if (skb->pkt_type == PACKET_OTHERHOST ||
            skb->pkt_type == PACKET_LOOPBACK) {
                kfree_skb(skb);
                return 0;
        }

        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb)
                return 0;

        /* Get the SDU out of the sk_buff */

        kfree_skb(skb);
        return 0;
};

static struct shim_instance * eth_vlan_create(struct shim_data * data,
                                              ipc_process_id_t   id)
{
        struct shim_instance * instance;
        struct shim_instance_ops ops;
        struct eth_vlan_instance * eth_instance;
        struct rb_node **p;
        struct rb_node *parent;
        struct eth_vlan_instance * s;
        struct rb_root * eth_root;
        LOG_FBEGN;

        eth_root = (struct rb_root *) data;
        p = &eth_root->rb_node;
        parent = NULL;

        instance = kzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance) {
                LOG_ERR("Cannot allocate memory for shim_instance");
                LOG_FEXIT;
                return NULL;
        }

        eth_instance = kzalloc(sizeof(*eth_instance), GFP_KERNEL);
        if (!eth_instance) {
                LOG_ERR("Cannot allocate memory for eth_vlan_instance");
                LOG_FEXIT;

                /* FIXME:
                 *   Are you sure ? instance might be deallocated and a
                 *   NULL returned
                 */
                return instance;
        }

        eth_instance->id = id;

        while (*p) {
                parent = *p;
                s = rb_entry(parent, struct eth_vlan_instance, node);
                if (unlikely(id == s->id)) {
                        LOG_ERR("Shim instance with id %x already exists", id);
                        kfree(eth_instance);
                        kfree(instance);
                        LOG_FEXIT;
                        return NULL;
                } else if (id < s->id)
                        p = &(*p)->rb_left;
                else
                        p = &(*p)->rb_right;
        }
        rb_link_node(&eth_instance->node,parent,p);
        rb_insert_color(&eth_instance->node,eth_root);

        instance->data             = eth_instance;

        /* ops should be bzero-ed, or statically initialized into bss or ... */
        ops.flow_allocate_request  = eth_vlan_flow_allocate_request;
        ops.flow_allocate_response = eth_vlan_flow_allocate_response;
        ops.flow_deallocate        = eth_vlan_flow_deallocate;
        ops.application_register   = eth_vlan_application_register;
        ops.application_unregister = eth_vlan_application_unregister;
        ops.sdu_write              = eth_vlan_sdu_write;
        ops.sdu_read               = eth_vlan_sdu_read;

        /* FIXME: ops will be destroyed at the end of this scope !!! */
        instance->ops              = &ops;

        LOG_FEXIT;
        return instance;
}

static int name_cpy(struct name_t * dst,
                    const struct name_t *src)
{
        struct name_t * temp;
        LOG_FBEGN;
        temp = kmalloc(sizeof(*temp), GFP_KERNEL);
        if (!temp) {
                LOG_ERR("Cannot allocate memory for name");
                LOG_FEXIT;
                return -1;
        }

        /* FIXME: Check strcpy return values */
        strcpy(temp->process_name, src->process_name);
        strcpy(temp->process_instance, src->process_instance);
        strcpy(temp->entity_name, src->entity_name);
        strcpy(temp->entity_instance, src->entity_instance);
        dst = temp;
        return 0;
}

struct shim_instance * eth_vlan_configure(struct shim_data *          data,
                                           struct shim_instance *     inst,
                                           const struct shim_config * cfg)
{
        struct eth_vlan_instance * eth_instance;
        struct eth_vlan_info *     info;
        struct list_head *         pos;
        struct shim_config *       c;
        struct shim_config_entry * tmp;
        struct shim_config_value * val;
        bool_t                     reconfigure;
        uint16_t                   old_vlan_id;
        string_t *                 old_interface_name;

        /* Check if instance is not null, check if data is not null */
        if (!inst) {
                LOG_WARN("Configure called on empty shim instance");

                LOG_FEXIT;
                return inst;
        }

        eth_instance = (struct eth_vlan_instance *) inst->data;
        if (!eth_instance) {
                LOG_WARN("Configure called on empty eth vlan shim instance");
                LOG_FEXIT;
                return inst;
        }

        /* If reconfigure = 1, break down all communication and setup again */
        reconfigure = 0;

        /* Get configuration struct pertaining to this shim instance */
        info = eth_instance->info;
        old_vlan_id = 0;
        old_interface_name = NULL;
        if (!info) {
                info = kmalloc(sizeof(*info), GFP_KERNEL);
                reconfigure = 1;
        } else {
                old_vlan_id = info->vlan_id;
                old_interface_name = info->interface_name;
        }
        if (!info) {
                LOG_ERR("Cannot allocate memory for shim_info");
                LOG_FEXIT;
                return inst;
        }

        /* Retrieve configuration of IPC process from params */
        list_for_each(pos, &(cfg->list)) {
                c   = list_entry(pos, struct shim_config, list);
                tmp = c->entry;
                val = tmp->value;
                if (!strcmp(tmp->name, "dif-name")
                    && val->type == SHIM_CONFIG_STRING) {
                        if (!name_cpy(info->name,
                                      (struct name_t *) val->data)) {
                                LOG_FEXIT;
                                return inst;
                        }
                } else if (!strcmp(tmp->name, "vlan-id")
                           && val->type == SHIM_CONFIG_UINT) {
                        info->vlan_id = * (uint16_t *) val->data;
                        if (!reconfigure &&
                            info->vlan_id != old_vlan_id) {
                                reconfigure = 1;
                        }
                } else if (!strcmp(tmp->name,"interface-name")
                           && val->type == SHIM_CONFIG_STRING) {
                        /* FIXME: Should probably be strcpy */
                        info->interface_name = (string_t *) val->data;
                        if (!reconfigure && !strcmp(info->interface_name,
                                                    old_interface_name)) {
                                reconfigure = 1;
                        }
                } else {
                        LOG_WARN("Unknown config param for eth shim");
                }
        }
        eth_instance->info = info;

        if (reconfigure) {
                struct packet_type eth_vlan_packet_type;

                eth_vlan_packet_type.type = cpu_to_be16(ETH_P_RINA);
                eth_vlan_packet_type.func = eth_vlan_rcv;

                /* Remove previous handler if there's one */
                if (old_interface_name && old_vlan_id != 0) {
                        char string_old_vlan_id[4];
                        char * complete_interface;
                        struct net_device *dev;

                        /* First construct the complete interface name */
                        complete_interface =
                                kmalloc(sizeof(*complete_interface), GFP_KERNEL);
                        if (!complete_interface) {
                                LOG_ERR("Cannot allocate memory for string");
                                LOG_FEXIT;
                                return inst;
                        }

                        sprintf(string_old_vlan_id,"%d",old_vlan_id);
                        strcat(complete_interface, ".");
                        strcat(complete_interface, string_old_vlan_id);

                        /* Remove the handler */
                        read_lock(&dev_base_lock);
                        dev = __dev_get_by_name(&init_net, complete_interface);
                        if (!dev) {
                                LOG_ERR("Invalid device specified to configure");
                                LOG_FEXIT;
                                return inst;
                        }
                        eth_vlan_packet_type.dev = dev;
                        dev_remove_pack(&eth_vlan_packet_type);
                        read_unlock(&dev_base_lock);
                        kfree(complete_interface);
                }
                /* FIXME: Add handler to correct interface and vlan id */
                /* Check if correctness VLAN id and interface name */


                dev_add_pack(&eth_vlan_packet_type);

        }
        LOG_DBG("Configured shim ETH IPC Process");

        return inst;
}

static int eth_vlan_destroy(struct shim_data *     data,
                            struct shim_instance * inst)
{
        struct eth_vlan_instance * instance;

        struct rb_root * eth_root;
        LOG_FBEGN;

        eth_root = (struct rb_root *) data;

        if (inst) {
                /*
                 * FIXME: Need to ask instance to clean up as well
                 * Don't know yet in full what to delete
                 */
                instance = (struct eth_vlan_instance *) inst->data;
                if (instance) {
                        rb_erase(&instance->node, eth_root);
                        kfree(instance);
                }
                kfree(inst);
        }

        LOG_FEXIT;
        return 0;
}

/* FIXME: should have its roots into struct shim_data */
/* Holds all shim instances */
static struct rb_root * eth_vlan_data;

static int eth_vlan_init(struct shim_data * data)
{
        LOG_FBEGN;

        LOG_INFO("Shim-eth-vlan module v%d.%d loaded",0,1);

        eth_vlan_data = kmalloc(sizeof(*eth_vlan_data), GFP_KERNEL);
        if (!eth_vlan_data) {
                LOG_ERR("Cannot allocate %zu bytes of memory",
                        sizeof(*eth_vlan_data));
                LOG_FEXIT;
                return -1;
        }

        *eth_vlan_data = RB_ROOT;

        LOG_FEXIT;

        return 0;
}

static int eth_vlan_fini(struct shim_data * data)
{

        struct rb_node * s;
        struct rb_node * e;
        struct eth_vlan_instance * i;
        struct rb_root * eth_vlan_data;
        LOG_FBEGN;

        eth_vlan_data = (struct rb_root *) data;

        /* Destroy all shim instances */
        s = rb_first(eth_vlan_data);
        while(s) {
                /* Get next node and keep pointer to this one */
                e = s;
                rb_next(s);
                /*
                 * Get the shim_instance
                 * FIXME: Need to ask it to clean up as well
                 * Don't know yet in full what to delete
                 */
                i = rb_entry(e,struct eth_vlan_instance, node);
                rb_erase(e, eth_vlan_data);
                kfree(i);
        }

        LOG_FEXIT;

        return 0;
}

static struct shim_ops eth_vlan_ops = {
        .init      = eth_vlan_init,
        .fini      = eth_vlan_fini,
        .create    = eth_vlan_create,
        .destroy   = eth_vlan_destroy,
        .configure = eth_vlan_configure,
};


static struct shim *  eth_vlan_shim = NULL;

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

static int __init mod_init(void)
{
        LOG_FBEGN;

        eth_vlan_shim = kipcm_shim_register(default_kipcm,
                                            "shim-eth-vlan",
                                            &eth_vlan_data,
                                            &eth_vlan_ops);
        if (!eth_vlan_shim) {
                LOG_CRIT("Initialization failed");

                LOG_FEXIT;
                return -1;
        }

        LOG_FEXIT;

        return 0;
}

static void __exit mod_exit(void)
{
        LOG_FBEGN;

        if (kipcm_shim_unregister(default_kipcm,
                                  eth_vlan_shim)) {
                LOG_CRIT("Cannot unregister");
                return;
        }

        LOG_FEXIT;
}


module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Shim IPC over Ethernet");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
