/*
 * System calls 
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

#include <linux/linkage.h>
#include <linux/kernel.h>

#define RINA_PREFIX "rina_syscalls"

#include "logs.h" 
#include "common.h" 
#include "kipcm.h" 

asmlinkage long sys_ipc_process_create(const name_t __user * name,
                                       ipc_process_id_t      ipcp_id,
                                       dif_type_t            type)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

asmlinkage long sys_ipc_process_configure(ipc_process_id_t                         ipcp_id,
                                          const struct ipc_process_conf_t __user * configuration)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

asmlinkage long sys_ipc_process_destroy(ipc_process_id_t ipcp_id)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

asmlinkage long sys_read_sdu(port_id_t      port_id,
                             bool_t         block,
                             sdu_t __user * sdu)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

asmlinkage long sys_write_sdu(port_id_t            port_id,
                              const sdu_t __user * sdu)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

asmlinkage long sys_efcp_create(const connection_t __user * connection)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

asmlinkage long sys_efcp_destroy(cep_id_t cep_id)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

asmlinkage long sys_efcp_update(cep_id_t cep_id,
                                cep_id_t dest_cep_id)
{
        LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}
