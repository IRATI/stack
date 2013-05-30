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

#ifndef RINA_KIPCM_H
#define RINA_KIPCM_H

#include "common.h"
#include "efcp.h"
#include "rmt.h"
#include "shim-eth.h"
#include "linux/hash.h"
#include <linux/hashtable.h>

struct id_to_ipcp_t {
	ipc_process_id_t      id; /* key */
	struct ipc_process_t *ipcprocess; /* Value*/
	struct list_head      list;
};

struct kipc_t {
	/*
         * Maintained and used by the K-IPC Manager to return the proper flow
	 * instance that contains the modules that provide the Data Transfer
	 * Service in each kind of IPC Process.
         */

	//FIXME Define HASH_TABLE
	//HASH_TABLE(port_id_to_flow, port_id_t, struct flow_t *);
	
	/* A table with all the instances of IPC Processes, indexed by
	 * process_id. */
	//FIXME Define HASH_TABLE
	//HASH_TABLE(id_to_ipcp, ipc_process_id_t, struct ipc_process_t *);
	struct list_head *id_to_ipcp;

};

int  kipcm_init(void);
void kipcm_exit(void);
int  kipcm_add_entry(port_id_t port_id, const struct flow_t * flow);
int  kipcm_remove_entry(port_id_t port_id);
int  kipcm_post_sdu(port_id_t port_id, const struct sdu_t * sdu);
int  read_sdu(port_id_t      port_id,
	      bool_t         block,
	      struct sdu_t * sdu);
int  write_sdu(port_id_t            port_id,
	       const struct sdu_t * sdu);
int  ipc_process_create(const struct name_t * name,
			ipc_process_id_t      ipcp_id,
			dif_type_t       type);
int  ipc_process_configure(ipc_process_id_t ipcp_id,
			   const struct     ipc_process_conf_t *configuration);
int  ipc_process_destroy(ipc_process_id_t ipcp_id);

#endif
