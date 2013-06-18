/*
 *  Shim IPC Process over Ethernet
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
#define RINA_PREFIX "shim-eth"

#include <linux/module.h>
#include <linux/if_ether.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/rbtree.h>

#include "logs.h"
#include "common.h"
#include "shim.h"

/* Holds all the shim instances */
static struct rb_root shim_eth_root = RB_ROOT;

/* Holds the configuration of one shim IPC process */
struct shim_eth_info_t {
	uint16_t        vlan_id;
	char *      interface_name;
	struct name_t * name;
};

enum port_id_state_t {
	PORT_STATE_NULL = 1,
	PORT_STATE_RECIPIENT_ALLOCATE_PENDING,
	PORT_STATE_INITIATOR_ALLOCATE_PENDING,
	PORT_STATE_ALLOCATED
};

/* Hold the information related to one flow*/
struct shim_eth_flow_t {
	uint64_t             src_mac;
	uint64_t             dst_mac;
	port_id_t            port_id;
	enum port_id_state_t port_id_state;

	/* FIXME: Will be a kfifo holding the SDUs or a sk_buff_head */
        /* QUEUE(sdu_queue, sdu_t *); */
};

/*
 * Contains all the information associated to an instance of a
 * shim Ethernet IPC Process
 */
struct shim_eth_instance_t { 
	struct rb_node node;

	/* IPC process id and DIF name */
	ipc_process_id_t ipc_process_id;

        /* The configuration of the shim IPC Process */
	struct shim_eth_info_t * info;

	/* FIXME: Pointer to the device driver data_structure */
	/* device_t * device_driver; */

	/* FIXME: Stores the state of flows indexed by port_id */
	/* HASH_TABLE(flow_to_port_id, port_id_t, shim_eth_flow_t); */
	/* rbtree or hash table? */
};

struct shim_instance_t * shim_eth_create(ipc_process_id_t ipc_process_id)
{
	struct shim_instance_t * instance;
	struct shim_eth_instance_t * shim_instance; 	
	struct rb_node **p = &shim_eth_root.rb_node;
	struct rb_node *parent = NULL;
	struct shim_eth_instance_t * s;

	LOG_FBEGN;

	instance = kmalloc(sizeof(*instance), GFP_KERNEL);
	if (!instance) {
                LOG_ERR("Cannot allocate memory for shim_instance");
                LOG_FEXIT;
                return 0;
        }

	shim_instance = kmalloc(sizeof(*shim_instance), GFP_KERNEL);
	if (!shim_instance) {
                LOG_ERR("Cannot allocate memory for shim_eth_instance");
                LOG_FEXIT;
                return instance;
        }
	instance->opaque = shim_instance;

	shim_instance->ipc_process_id = ipc_process_id;

	while(*p){
		parent = *p;
		s = rb_entry(parent, struct shim_eth_instance_t, node);
		if(unlikely(ipc_process_id == s->ipc_process_id)){
			LOG_ERR("Shim instance with id %x already exists", 
				ipc_process_id);
			kfree(shim_instance);
			kfree(instance);
			LOG_FEXIT;
                       	return 0;
		}
		else if(ipc_process_id < s->ipc_process_id)
			p = &(*p)->rb_left;
		else
			p = &(*p)->rb_right;
	}
	rb_link_node(&shim_instance->node,parent,p);
	rb_insert_color(&shim_instance->node,&shim_eth_root);        

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

	strcpy(temp->process_name, src->process_name);
	strcpy(temp->process_instance, src->process_instance);
	strcpy(temp->entity_name, src->entity_name);
	strcpy(temp->entity_instance, src->entity_instance);
	dst = temp;
	return 0;
}

struct shim_instance_t * shim_eth_configure
        (struct shim_instance_t *   inst,
        const struct shim_conf_t * configuration)
{
	struct shim_eth_instance_t * instance;
	struct shim_eth_info_t * shim_info;
	struct list_head * pos;
	struct shim_conf_t * c;
	struct shim_config_entry_t * tmp;
	struct shim_config_value_t * val;
	bool_t reconfigure;

	/* Check if instance is not null, check if opaque is not null */
	if(!inst){
		LOG_WARN("Configure called on empty shim instance");
		LOG_FEXIT;
		return inst;
	}

	instance = (struct shim_eth_instance_t *) inst->opaque;
	if(!instance){
		LOG_WARN("Configure called on empty eth vlan shim instance");
		LOG_FEXIT;
		return inst;
	}
	
	/* If reconfigure = 1, break down all communication and setup again */
	reconfigure = 0;

	/* Get configuration struct pertaining to this shim instance */
	shim_info = instance->info; 
	if(!shim_info) {
		shim_info = kmalloc(sizeof(*shim_info), GFP_KERNEL);
		reconfigure = 1;
	}
	if (!shim_info) {
                LOG_ERR("Cannot allocate memory for shim_info");
                LOG_FEXIT;
                return inst;
        }

	/* Retrieve configuration of IPC process from params */
	list_for_each(pos, &(configuration->list)){
		c = list_entry(pos, struct shim_conf_t, list);
		tmp = c->entry;
		val = tmp->value;
		if(strcmp(tmp->name, "difname") == 0 
			&& val->type == SHIM_CONFIG_STRING) {
			if(!name_cpy(shim_info->name, 
					(struct name_t *)val->data)){
				LOG_FEXIT;
				return inst;
			}
		} else if (strcmp(tmp->name, "vlanid") == 0 
			&& val->type == SHIM_CONFIG_UINT) {
			shim_info->vlan_id = * (uint16_t *) val->data;
			reconfigure = 1;
		} else if(strcmp(tmp->name,"interfacename") == 0 
			&& val->type == SHIM_CONFIG_STRING) {
			shim_info->interface_name = (string_t *) val->data;
			reconfigure = 1;
		} else {
			LOG_WARN("Unknown config param for eth shim");
		}
	}
	instance->info = shim_info;
	
	if(reconfigure) {
		/* FIXME: Add handler to correct interface and vlan id */
		/* Check if correctness VLAN id and interface name */
	}
	LOG_DBG("Configured shim ETH IPC Process");

	return inst;
}

int shim_eth_destroy(struct shim_instance_t * inst)
{
	struct shim_eth_instance_t * instance;
	LOG_FBEGN;
	if(inst){
                /**
		 *  FIXME: Need to ask instance to clean up as well
		 * Don't know yet in full what to delete
		 */
		instance = (struct shim_eth_instance_t *) inst->opaque;
		if(instance){
			rb_erase(&instance->node, &shim_eth_root);
			kfree(instance);
		}
		kfree(inst);
	}
        LOG_FEXIT;
	return 0;
}

int shim_eth_flow_allocate_request(void *                     opaque, 
				   const struct name_t *      source,
                                   const struct name_t *      dest,
                                   const struct flow_spec_t * flow_spec,
                                   port_id_t                * port_id)
{
	LOG_FBEGN;
	LOG_FEXIT;

	return 0;
}

int shim_eth_flow_allocate_response(void *              opaque,
				    port_id_t           port_id,
                                    response_reason_t * response)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_flow_deallocate(void *    opaque,
			     port_id_t port_id)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_application_register(void *                opaque,
				  const struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_application_unregister(void *               opaque,
				   const struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_sdu_write(void *               opaque,
		       port_id_t            port_id, 
		       const struct sdu_t * sdu)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_sdu_read(void *         opaque,
		      port_id_t      id,
		      struct sdu_t * sdu)
{
	LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

static int __init mod_init(void)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static void __exit mod_exit(void)
{
	struct rb_node * s;
	struct rb_node * e;
	struct shim_eth_instance_t * i;
        LOG_FBEGN;

	/* Destroy all shim instances */
	s = rb_first(&shim_eth_root);
	while(s){
                /* Get next node and keep pointer to this one */
		e = s;
		rb_next(s);
		/**
		 * Get the shim_instance 
		 * FIXME: Need to ask it to clean up as well
		 * Don't know yet in full what to delete
		 */
		i = rb_entry(e,struct shim_eth_instance_t, node);
		rb_erase(e, &shim_eth_root);
		kfree(i);
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
