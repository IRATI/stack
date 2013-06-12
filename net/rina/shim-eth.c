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

#include "logs.h"
#include "common.h"
#include "shim.h"

LIST_HEAD(shim_eth);

/* Holds the configuration of one shim IPC process */
struct shim_eth_info_t {
	struct name_t * name;
	uint16_t        vlan_id;
	char *      interface_name;
};

enum port_id_state_t {
	PORT_STATE_NULL,
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
	/* FIXME: Will probably be a sk_buff_head or a linked list 
	   holding only the SDUs */

        /* QUEUE(sdu_queue, sdu_t *); */
};

/*
 * Contains all the information associated to an instance of a
 * shim Ethernet IPC Process, as well as pointers to the
 * functions to manipulate it
 */
struct shim_eth_instance_t {
        /* The configuration of the shim IPC Process */
	struct shim_eth_info_t * info;

	/* FIXME: Pointer to the device driver data_structure */
	/* device_t * device_driver; */

	/* FIXME: Stores the state of flows indexed by port_id */
	/* HASH_TABLE(flow_to_port_id, port_id_t, shim_eth_flow_t); */
};

/*
 * The container for the shim IPC Process over Ethernet
 * component
 */
/* Stores the state of shim IPC Process instances */
struct shim_eth_t {
	ipc_process_id_t ipc_process_id;
	struct shim_eth_instance_t * shim_eth_instance;
	struct list_head list;
};

int shim_eth_init(void)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

void shim_eth_exit(void)
{
        LOG_FBEGN;
        LOG_FEXIT;
}

static int name_cpy(struct name_t * dst, 
		     const struct name_t *src)
{
	struct name_t * temp;
	LOG_FBEGN;
	temp = kmalloc(sizeof(*temp), GFP_KERNEL);
	if (!temp) {
                LOG_ERR("Cannot allocate memory");
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

int shim_eth_create(ipc_process_id_t      ipc_process_id,
                    const struct name_t * name)
{
	struct shim_eth_t * list_item; 
	struct shim_eth_instance_t * shim_instance; 	
	struct shim_eth_info_t * shim_info;

	LOG_FBEGN;

	list_item = kmalloc(sizeof(*list_item), GFP_KERNEL);
	if (!list_item) {
                LOG_ERR("Cannot allocate memory");
                LOG_FEXIT;
                return -1;
        }

	shim_instance = kmalloc(sizeof(*shim_instance), GFP_KERNEL);
	if (!shim_instance) {
                LOG_ERR("Cannot allocate memory");
                LOG_FEXIT;
                return -1;
        }

	shim_info = kmalloc(sizeof(*shim_info), GFP_KERNEL);
	if (!shim_info) {
                LOG_ERR("Cannot allocate memory");
                LOG_FEXIT;
                return -1;
        }

	if(!name_cpy(shim_info->name, name)){
		LOG_FEXIT;
		return -1;
	}
	shim_instance->info = shim_info;

	list_item->ipc_process_id = ipc_process_id;
	list_item->shim_eth_instance = shim_instance;
     	list_add(&(list_item->list), &shim_eth);
        LOG_FEXIT;
	return 0;
}

int shim_eth_configure(ipc_process_id_t          ipc_process_id,
                      const struct shim_conf_t * configuration)
{

#if 0
	struct list_head *pos;
	struct shim_config_entry_t tmp;

	/* Retrieve configuration of IPC process from params */
	list_for_each(pos, configuration.list){


		tmp = list_entry(pos, struct shim_config_entry_t, list);
		


		switch (ipc_config->type) {
		case IPC_CONFIG_UINT:
			shim_eth_info.vlan_id =
                                * (uint16_t *) ipc_config->value;
			break;
		case IPC_CONFIG_STRING:
			shim_eth_info.interface_name = 
				(string_t *) ipc_config->value;
			break;
		default:
			break;
		}
	}

	/* FIXME: Add handler to correct interface and vlan id */

	LOG_DBG("Configured shim ETH IPC Process");
#endif
	return 0;
}

int shim_eth_destroy(ipc_process_id_t ipc_process_id)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_flow_allocate_request(const struct name_t *      source,
                                   const struct name_t *      dest,
                                   const struct flow_spec_t * flow_spec,
                                   port_id_t                * port_id)
{
	LOG_FBEGN;
	LOG_FEXIT;

	return 0;
}

int shim_eth_flow_allocate_response(port_id_t           port_id,
                                    response_reason_t * response)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_flow_deallocate(port_id_t port_id)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_application_register(const struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_application_unregister(const struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_sdu_write(port_id_t            port_id, 
		       const struct sdu_t * sdu)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_sdu_read(port_id_t      id,
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
        LOG_FBEGN;
        LOG_FEXIT;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Shim IPC over Ethernet");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
