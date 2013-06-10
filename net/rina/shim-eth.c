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
#include <linux/kfifo.h>

#define RINA_PREFIX "shim-eth"

#include "logs.h"
#include "shim-eth.h"

LIST_HEAD(shim_eth);
/* FIXME : This isn't needed */
static ipc_process_id_t count = 0;

ipc_process_id_t shim_eth_create(struct ipc_config_t ** config)
{
	/* Unsure if I can call return count++ and count gets incremented after
	function call? This is a workaround, fix if possible. */
	ipc_process_id_t nr = count++;
	struct shim_eth_info_t shim_eth_info;
	struct ipc_config_t *ipc_config = config[0];
	struct shim_eth_instance_t instance;

        LOG_FBEGN;

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
		}
		++ipc_config;
	}

	instance.info = shim_eth_info;
	
	struct shim_eth_t tmp = {
		.shim_eth_instance = instance,
		.ipc_process_id = nr,
		.list = LIST_HEAD_INIT(tmp.list),
	};
       
	list_add(&tmp.list, &shim_eth);
	/* FIXME: Add handler to correct interface and vlan id */

        LOG_FEXIT;
	return nr;
}

int shim_eth_destroy(ipc_process_id_t ipc_process_id)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}
/* FIXME : Tentative implementation to compare with the kipcm code. Please
 * 		Sander review it. Miquel.
 */
int shim_eth_allocate_flow_request(struct name_t *      source,
                                   struct name_t *      dest,
                                   struct flow_spec_t * flow_spec,
                                   port_id_t            port_id)
{
	struct flow_t * flow;
	struct kfifo    sdu_ready;
	struct ipc_process_t * ipcp;

	LOG_FBEGN;

	/* FIXME : This reference should be taken from the shim-eth ipc process */
	ipcp = kmalloc(sizeof(*ipcp), GFP_KERNEL);
	if (ipcp == NULL) {
		LOG_ERR("Cannot allocate %d bytes of kernel memory", sizeof(*ipcp));

		LOG_FEXIT;
		return -1;
	}
	flow = kmalloc(sizeof(*flow), GFP_KERNEL);
	if (flow == NULL) {
		LOG_ERR("Cannot allocate %d bytes of kernel memory", sizeof(*flow));

                LOG_FEXIT;
		return -1;
	}
	/* FIXME : This should be an IPC Process already existing */
	ipcp =  kmalloc(sizeof(*ipcp), GFP_KERNEL);
	if (ipcp == NULL) {
		LOG_ERR("Cannot allocate %d bytes of kernel memory", sizeof(*ipcp));

		LOG_FEXIT;
		return -1;
	}
	flow->application_owned = 1;
	flow->ipc_process = ipcp;
	if (kfifo_alloc(&sdu_ready, PAGE_SIZE, GFP_KERNEL)) {
		LOG_FEXIT;
		kfree(flow);
		kfree(ipcp);
		return -1;
	}
/* FIXME: This doesn't compile */
	flow->sdu_ready = &sdu_ready;
	if (kipcm_add_entry(port_id, (const struct flow_t *)flow)) {
		LOG_FEXIT;
		kfree(flow);
		kfree(ipcp);
		kfifo_free(&sdu_ready);
		return -1;
	}

	LOG_FEXIT;

	return 0;
}

int shim_eth_allocate_flow_response(port_id_t           port_id,
                                    response_reason_t * response)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_deallocate_flow(port_id_t port_id)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_register_application(struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_unregister_application(struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int shim_eth_write_sdu(port_id_t port_id, const struct sdu_t * sdu)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

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

int shim_eth_ipc_create(const struct name_t * name,
			ipc_process_id_t      ipcp_id)
{
        LOG_DBG("Created shim ETH IPC process");

	return 0;
}

int shim_eth_ipc_configure(ipc_process_id_t ipcp_id,
                           const struct ipc_process_shim_ethernet_conf_t *configuration)
{
	LOG_DBG("Configured shim ETH IPC Process");

	return 0;
}
