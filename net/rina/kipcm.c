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

#include "logs.h"
#include "kipcm.h"
#include <linux/syscalls.h>

int kipcm_init()
{
        LOG_FBEGN;

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

/*FIXME: Define ipc_process_create
//SYSCALL_DEFINE3(ipc_process_create, const struct name_t *name, ipc_process_id_t ipcp_id, dif_type_t type)
//{
//	LOG_DBG("syscall ipc_process_create");
//	
//	return 0;
//}*/
SYSCALL_DEFINE1(kipcm_call, int, param)
{
        LOG_DBG("TESTING SYSCALL: %d", param);
	return 0;
};

int  read_sdu(port_id_t port_id,
	      bool_t block,
	      struct sdu_t * sdu)
{
	return 0;
}

int  write_sdu(port_id_t            port_id,
	       const struct sdu_t * sdu)
{
	return 0;
}

int  ipc_process_create(const struct name_t * name,
			ipc_process_id_t      ipcp_id,
			enum dif_type_t       type)
{
	return 0;
}

int  ipc_process_configure(ipc_process_id_t ipcp_id,
			   const struct     ipc_process_conf_t *configuration)
{
	return 0;
}

int  ipc_process_destroy(ipc_process_id_t ipcp_id)
{
	return 0;
}
