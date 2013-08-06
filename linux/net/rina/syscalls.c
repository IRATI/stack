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
#include "debug.h"

#define CALL_PERSONALITY(RETVAL, PERS, HOOK, ARGS...)                   \
        do {                                                            \
                LOG_DBG("Handling personality hook %s",                 \
                        __stringify(HOOK));                             \
                                                                        \
                if (PERS == NULL) {                                     \
                        LOG_ERR("No personality registered");           \
                        return -1;                                      \
                }                                                       \
                                                                        \
                ASSERT(PERS);                                           \
                ASSERT(PERS -> ops);                                    \
                                                                        \
                if (PERS -> ops -> HOOK == NULL) {                      \
                        LOG_ERR("Personality has no %s hook",           \
                                __stringify(HOOK));                     \
                        return -1;                                      \
                }                                                       \
                                                                        \
                LOG_DBG("Calling personality hook %s",                  \
                        __stringify(HOOK));                             \
                                                                        \
                RETVAL = PERS -> ops -> HOOK (PERS->data, ##ARGS);      \
        } while (0)

/* FIXME: To be extended later on */
int rcopy_to_user(const char *  kernel_buffer,
                  char __user * user_buffer,
                  size_t        count)
{
        copy_to_user(user_buffer, kernel_buffer, count);
        return 0;
}

/* FIXME: To be extended later on */
int rcopy_from_user(const char __user * user_buffer,
                    char *              kernel_buffer,
                    size_t              count)
{
        char * tmp_page;

        if (count >= PAGE_SIZE) {
                LOG_ERR("Requested buffer size is too long (%zd >= %zd)",
                        count, PAGE_SIZE);
                return -1;
        }

        tmp_page = (char *) __get_free_page(GFP_TEMPORARY);
        if (!tmp_page) {
                LOG_ERR("Cannot get a free page");
                return -1;
        }

        if (copy_from_user(tmp_page, user_buffer, count)) {
                LOG_ERR("Cannot copy from user");
                free_page((unsigned long) tmp_page);
                return -1;
                
        }

        memcpy(kernel_buffer, tmp_page, count);
        
        free_page((unsigned long) tmp_page);
        return 0;
}

#define CALL_DEFAULT_PERSONALITY(RETVAL, HOOK, ARGS...)                 \
        CALL_PERSONALITY(RETVAL, default_personality, HOOK, ##ARGS)

SYSCALL_DEFINE3(ipc_create,
                const struct name __user *, name,
                ipc_process_id_t,           id,
                const char __user *,        type)
{
        long retval;

        CALL_DEFAULT_PERSONALITY(retval, ipc_create, name, id, type);

        return retval;
}

SYSCALL_DEFINE2(ipc_configure,
                ipc_process_id_t,                  id,
                const struct ipcp_config __user *, config)
{
        long retval;

        CALL_DEFAULT_PERSONALITY(retval, ipc_configure, id, config);

        return retval;
}

SYSCALL_DEFINE1(ipc_destroy,
                ipc_process_id_t, id)
{
        long retval;

        CALL_DEFAULT_PERSONALITY(retval, ipc_destroy, id);

        return retval;
}

SYSCALL_DEFINE2(sdu_read,
                port_id_t,           id,
                struct sdu __user *, sdu)
{
        long retval;
        
        CALL_DEFAULT_PERSONALITY(retval, sdu_read, id, sdu);

        return retval;
}

SYSCALL_DEFINE2(sdu_write,
                port_id_t,                 id,
                const struct sdu __user *, sdu)
{
        long retval;

        CALL_DEFAULT_PERSONALITY(retval, sdu_write, id, sdu);

        return retval;
}

SYSCALL_DEFINE1(connection_create,
                const struct connection __user *, connection)
{
        long retval;

        CALL_DEFAULT_PERSONALITY(retval, connection_create, connection);

        return retval;
}

SYSCALL_DEFINE1(connection_destroy,
                cep_id_t, id)
{
        long retval;
        
        CALL_DEFAULT_PERSONALITY(retval, connection_destroy, id);
        
        return retval;
}

SYSCALL_DEFINE2(connection_update,
                cep_id_t, from_id,
                cep_id_t, to_id)
{
        long retval;

        CALL_DEFAULT_PERSONALITY(retval, connection_update, from_id, to_id);

        return retval;
}
