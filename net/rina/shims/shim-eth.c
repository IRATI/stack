/*
 *  Shim IPC Process for Ethernet
 *
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
 *    Miquel Tarzan   <miquel.tarzan@i2cat.net>
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

#include <linux/if_ether.h>

#define RINA_PREFIX "shim-eth"

#include "../logs.h"
#include "shim-eth.h"

LIST_HEAD(shim_eth);
/* FIXME : This isn't needed */
static ipc_process_id_t count = 0;

ipc_process_id_t shim_eth_create(struct ipc_config_t ** config)
{
        /* Unsure if I can call return count++ and count gets incremented after
	   function call? This is a workaround, fix if possible. */
	ipc_process_id_t nr = count++;

	/* Retrieve configuration of IPC process from params */
	struct shim_eth_info_t shim_eth_info;
	struct ipc_config_t *ipc_config = config[0];
	while (ipc_config != 0) {
		switch (ipc_config->type) {
		case IPC_CONFIG_NAME:
			shim_eth_info.name =
                                (struct name_t *) ipc_config->value;
			break;
		case IPC_CONFIG_UINT:
			shim_eth_info.vlan_id =
                                * (uint16_t *) ipc_config->value;
			break;
		case IPC_CONFIG_STRING:
			shim_eth_info.interface_name = 
				(string_t *) ipc_config->value;
		}
		++ipc_config;
	}

	struct shim_eth_instance_t instance = {
		.info = shim_eth_info
	};
	
	struct shim_eth_t tmp = {
		.shim_eth_instance = instance,
		.ipc_process_id = nr,
		.list = LIST_HEAD_INIT(tmp.list),
	};
       
	list_add(&tmp.list, &shim_eth);
	/* FIXME: Add handler to correct interface and vlan id */

	LOG_DBG("A new shim IPC process was created");
	return nr;
}

int shim_eth_destroy(ipc_process_id_t ipc_process_id)
{
	LOG_DBG("A shim IPC process was destroyed");

	return 0;
}

port_id_t shim_eth_allocate_flow_request(struct name_t *      source,
                                         struct name_t *      dest,
                                         struct flow_spec_t * flow_spec)
{
	LOG_DBG("Allocate flow request");

	return 0;
}

int shim_eth_allocate_flow_response(port_id_t *         port_id,
                                    response_reason_t * response)
{
	LOG_DBG("Allocate flow response");

	return 0;
}

int shim_eth_deallocate_flow(port_id_t port_id)
{
	LOG_DBG("Deallocate flow");

	return 0;
}

int shim_eth_register_application(struct name_t * name)
{
	LOG_DBG("Application registered");

	return 0;
}

int shim_eth_unregister_application(struct name_t * name)
{
	LOG_DBG("Application unregistered");

	return 0;
}

int shim_eth_write_sdu(port_id_t port_id, const struct sdu_t * sdu)
{
	LOG_DBG("Written SDU");

	return 0;
}

int shim_eth_init(void)
{
        LOG_DBG("init");

        return 0;
}

void shim_eth_exit(void)
{
        LOG_DBG("exit");
}

int shim_eth_ipc_create(const struct name_t * name,
			ipc_process_id_t      ipcp_id)
{
        /*struct shim_eth_info_t *shim_eth_info;
        struct shim_eth_t *tmp;
        struct shim_eth_instance_t *instance;
        shim_eth_info = kmalloc(sizeof(*shim_eth_info),GFP_KERNEL);
        if (!shim_eth_info) {
                LOG_DBG("Failed creation of the shim ethernet info structure");
                return -1;
        }
        shim_eth_info->name = name;
        instance = kmalloc(sizeof(*instance),GFP_KERNEL);
        if (!shim_eth_info) {
                LOG_DBG("Failed creation of the shim ethernet instance");
                return -1;
        }
        instance->info = shim_eth_info;
        tmp = kmalloc(sizeof(*tmp),GFP_KERNEL);
        if (!tmp) {
                LOG_DBG("Failed creation of the shim ethernet structure");
                return -1;
        }
        tmp->ipc_process_id = ipcp_id;
        tmp->shim_eth_instance = instance;
        INIT_LIST_HEAD(&tmp->list);
        list_add(&tmp->list, &shim_eth);*/
        /* FIXME: Add handler to correct interface and vlan id */
        LOG_DBG("Created shim ETH IPC process");

	return 0;
}

int shim_eth_ipc_configure(ipc_process_id_t ipcp_id,
                           const struct ipc_process_shim_ethernet_conf_t *configuration)
{
	LOG_DBG("Configured shim ETH IPC Process");
	configuration->device_name;

	return 0;
}
