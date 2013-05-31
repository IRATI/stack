/*
 *  KIPCM (Kernel-IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#define RINA_PREFIX "kipcm"

#include <linux/linkage.h>

#include "logs.h"
#include "kipcm.h"

static LIST_HEAD(id_to_ipcp);
static struct kipc_t *kipcm;

int kipcm_init()
{
        LOG_FBEGN;

        kipcm->id_to_ipcp = &id_to_ipcp;

        LOG_FEXIT;

        return 0;
}

void kipcm_exit()
{
        LOG_FBEGN;

        LOG_FEXIT;
}

int kipcm_add_entry(port_id_t port_id, const struct flow_t * flow)
{
        LOG_FBEGN;

        LOG_FEXIT;

	return 0;
}

int kipcm_remove_entry(port_id_t port_id)
{
        LOG_FBEGN;

        LOG_FEXIT;

	return 0;
}

int kipcm_post_sdu(port_id_t port_id, const struct sdu_t * sdu)
{
        LOG_FBEGN;

        LOG_FEXIT;

	return 0;
}

int  read_sdu(port_id_t      port_id,
	      bool_t         block,
	      struct sdu_t * sdu)
{
	return 0;
}

int  write_sdu(port_id_t            port_id,
	       const struct sdu_t * sdu)
{
	if (shim_eth_write_sdu(port_id, sdu))
		LOG_DBG("Error writing SDU to the SHIM ETH");
	return 0;
}

int  ipc_process_create(const struct name_t * name,
			ipc_process_id_t      ipcp_id,
			dif_type_t 	      type)
{
	struct ipc_process_shim_ethernet_t *ipcp_shim_eth;
	struct ipc_process_t *ipc_process;
	struct ipc_process_data_t *ipcp_data;
	struct id_to_ipcp_t *aux_id_to_ipcp;
	switch (type) {
	case DIF_TYPE_SHIM_ETH :
		ipc_process = kmalloc(sizeof(*ipc_process), GFP_KERNEL);
		ipc_process->type = type;
		ipcp_data = kmalloc(sizeof(*ipcp_data), GFP_KERNEL);
		ipcp_data->type = type;
		ipcp_shim_eth = kmalloc(sizeof(*ipcp_shim_eth), GFP_KERNEL);
		ipcp_shim_eth->ipcp_id = ipcp_id;
		ipcp_data->ipc_process.shim_eth_ipcp = ipcp_shim_eth;
		ipc_process->data = ipcp_data;
		aux_id_to_ipcp = kmalloc(sizeof(*aux_id_to_ipcp), GFP_KERNEL);
		aux_id_to_ipcp->id = ipcp_id;
		aux_id_to_ipcp->ipcprocess = ipc_process;
		INIT_LIST_HEAD(&aux_id_to_ipcp->list);
		list_add(aux_id_to_ipcp,&kipcm->id_to_ipcp);
		break;
	case DIF_TYPE_NORMAL:
		break;
	case DIF_TYPE_SHIM_IP:
		break;
	}
	return 0;
}

int  ipc_process_configure(ipc_process_id_t                 ipcp_id,
			   const struct ipc_process_conf_t *configuration)
{
	return 0;
}

int  ipc_process_destroy(ipc_process_id_t ipcp_id)
{
	return 0;
}
