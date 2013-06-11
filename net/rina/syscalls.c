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
#include <linux/stringify.h>

#define RINA_PREFIX "syscalls"

#include "logs.h"
#include "utils.h"
#include "personality.h"

#define RUN_PERSONALITY(HOOK, ARGS...)                                  \
do {                                                                    \
	if (personality == NULL) {                                      \
                LOG_ERR("No personality registered");                   \
                return -1;                                              \
        }                                                               \
                                                                        \
	ASSERT(personality);                                            \
                                                                        \
        if (personality -> HOOK == NULL) {                              \
                LOG_ERR("No %s hook present", __stringify(HOOK));       \
                return -1;                                              \
        }                                                               \
                                                                        \
        ASSERT(personality -> HOOK);                                    \
        return personality -> HOOK (personality->data, ARGS);           \
} while (0)

asmlinkage long
sys_ipc_process_create(const struct name_t __user * name,
                       ipc_process_id_t             id,
                       dif_type_t                   type)
{ RUN_PERSONALITY(ipc_create, name, id, type); }
                
asmlinkage long
sys_ipc_process_configure(ipc_process_id_t                         id,
                          const struct ipc_process_conf_t __user * config)
{ RUN_PERSONALITY(ipc_configure, id, config); }

asmlinkage long
sys_ipc_process_destroy(ipc_process_id_t id)
{ RUN_PERSONALITY(ipc_destroy, id); }

asmlinkage long
sys_read_sdu(port_id_t             id,
             struct sdu_t __user * sdu)
{ RUN_PERSONALITY(sdu_read, id, sdu); }

asmlinkage long
sys_write_sdu(port_id_t                   id,
              const struct sdu_t __user * sdu)
{ RUN_PERSONALITY(sdu_write, id, sdu); }

asmlinkage long
sys_efcp_create(const struct connection_t __user * connection)
{ RUN_PERSONALITY(connection_create, connection); }

asmlinkage long
sys_efcp_destroy(cep_id_t id)
{ RUN_PERSONALITY(connection_destroy, id); }

asmlinkage long
sys_efcp_update(cep_id_t from_id,
                cep_id_t to_id)
{ RUN_PERSONALITY(connection_update, from_id, to_id); }
