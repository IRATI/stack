/*
 *  KIPCM (Kernel-IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_IPCM_H
#define RINA_IPCM_H


#include	"common.h"
#include	"efcp.h"
#include	"rmt.h"

struct normal_ipc_process_conf_t{

	
	/*-----------------------------------------------------------------------------
	 * Configuration of the kernel components of a normal IPC Process.
	 * Defines the struct for the kernel components of a fully RINA IPC
	 * Process.
 	 *-----------------------------------------------------------------------------*/

	/* The address of the IPC Process */
	ipc_process_address_t address;

	/* The configuration of the EFCP component */
	struct efcp_conf_t *efcp_config;

	/* The configuration of the RMT component */

	struct rmt_conf_t *rmt_config;

};

int  kipcm_init(void);
void kipcm_exit(void);
int  kipcm_add_entry(port_id_t port_id, const flow_t *flow);
int  kipcm_remove_entry(port_id_t port_id);
int  kipcm_post_sdu(port_id_t port_id, const sdu_t *sdu);


#endif
