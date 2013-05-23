/*
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

#define RINA_PREFIX "ipcm"

#include "logs.h"
#include "ipcm.h"

int ipcm_init()
{
        LOG_DBG("init");

        return 0;
}

void ipcm_exit()
{
        LOG_DBG("exit");
}

SYSCALL_DEFINE3(ipc_process_create, const name_t *name, ipc_process_id_t ipcp_id, dif_type_t type);

