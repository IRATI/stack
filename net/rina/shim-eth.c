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

#include <linux/module.h>
#include <linux/if_ether.h>
#include <linux/kfifo.h>

#define RINA_PREFIX "shim-eth"

#include "logs.h"

/* FIXME: Should be removed !!! */
#include "shim-eth.h"

LIST_HEAD(shim_eth);

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

int shim_eth_create(ipc_process_id_t      ipc_process_id,
                    const struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;
	return 0;
}

int shim_eth_configure(ipc_process_id_t          ipc_process_id,
                      const struct shim_conf_t * configuration)
{

#if 0
	struct shim_eth_info_t shim_eth_info;
	struct ipc_config_t *ipc_config = config[0];
	struct shim_eth_instance_t instance;

	/* Retrieve configuration of IPC process from params */
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
			break;
		default:
			break;
		}
		++ipc_config;
	}

	instance.info = shim_eth_info;
	
	struct shim_eth_t tmp;

	tmp.shim_eth_instance = instance;
	tmp.ipc_process_id = nr;
	tmp.list = LIST_HEAD_INIT(tmp.list);
       
	list_add(&tmp.list, &shim_eth);
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
