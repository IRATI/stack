/*
 * System calls 
 * 
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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
#include <linux/syscalls.h>
#include <linux/stringify.h>

#define RINA_PREFIX "syscalls"

#include "logs.h"
#include "utils.h"
#include "personality.h"

/* FIXME: To be removed ASAP */
extern struct personality_t * rina_personality;

#define CALL_PERSONALITY(HOOK, ARGS...)                                   \
do {                                                                      \
	if (rina_personality == NULL) {                                   \
                LOG_ERR("No personality registered");                     \
                return -1;                                                \
        }                                                                 \
                                                                          \
	ASSERT(rina_personality);                                         \
                                                                          \
        if (rina_personality -> HOOK == NULL) {                           \
                LOG_ERR("No %s hook present", __stringify(HOOK));         \
                return -1;                                                \
        }                                                                 \
                                                                          \
        ASSERT(rina_personality -> HOOK);                                 \
        return rina_personality -> HOOK (rina_personality->data, ##ARGS); \
} while (0)

SYSCALL_DEFINE3(ipc_create,
                const struct name_t __user *, name,
                ipc_process_id_t,             id,
                dif_type_t,                   type)
{ CALL_PERSONALITY(ipc_create, name, id, type); }
                
SYSCALL_DEFINE2(ipc_configure,
                ipc_process_id_t,                         id,
                const struct ipc_process_conf_t __user *, config)
{ CALL_PERSONALITY(ipc_configure, id, config); }

SYSCALL_DEFINE1(ipc_destroy,
                ipc_process_id_t, id)
{ CALL_PERSONALITY(ipc_destroy, id); }

SYSCALL_DEFINE2(sdu_read,
                port_id_t,             id,
                struct sdu_t __user *, sdu)
{ CALL_PERSONALITY(sdu_read, id, sdu); }

SYSCALL_DEFINE2(sdu_write,
                port_id_t,                   id,
                const struct sdu_t __user *, sdu)
{ CALL_PERSONALITY(sdu_write, id, sdu); }

SYSCALL_DEFINE1(connection_create,
                const struct connection_t __user *, connection)
{ CALL_PERSONALITY(connection_create, connection); }

SYSCALL_DEFINE1(connection_destroy,
                cep_id_t, id)
{ CALL_PERSONALITY(connection_destroy, id); }

SYSCALL_DEFINE2(connection_update,
                cep_id_t, from_id,
                cep_id_t, to_id)
{ CALL_PERSONALITY(connection_update, from_id, to_id); }
