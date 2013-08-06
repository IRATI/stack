/*
 * System calls
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

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/stringify.h>
#include <linux/uaccess.h>

#define RINA_PREFIX "syscalls"

#include "logs.h"
#include "utils.h"
#include "personality.h"
#include "debug.h"
#include "ipcp-utils.h"

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

#define CALL_DEFAULT_PERSONALITY(RETVAL, HOOK, ARGS...)                 \
        CALL_PERSONALITY(RETVAL, default_personality, HOOK, ##ARGS)

static char * strdup_from_user(const char __user * src)
{
        size_t size;
        char * tmp;

        if (!src)
                return NULL;

        size = strlen_user(src); /* Includes the terminating NUL */
        if (!size)
                return NULL;

        tmp = rkmalloc(size, GFP_KERNEL);
        if (!tmp)
                return NULL;

        /* strncpy_from_user() copies the terminating NUL */
        if (strncpy_from_user(tmp, src, size)) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static int string_dup_from_user(const string_t __user * src, string_t ** dst)
{
        ASSERT(dst);

        /*
         * An empty source is allowed (ref. the chain of calls) and it must
         * provoke no consequeunces
         */
        if (src) {
                *dst = strdup_from_user(src);
                if (!*dst) {
                        LOG_ERR("Cannot duplicate source string");
                        return -1;
                }
        } else
                *dst = NULL;

        return 0;
}

static int name_cpy_from_user(const struct name __user * src,
                              struct name *              dst)
{
        if (!src || !dst)
                return -1;

        name_fini(dst);

        /* We rely on short-boolean evaluation ... :-) */
        if (string_dup_from_user(src->process_name,
                                 &dst->process_name)     ||
            string_dup_from_user(src->process_instance,
                                 &dst->process_instance) ||
            string_dup_from_user(src->entity_name,
                                 &dst->entity_name)      ||
            string_dup_from_user(src->entity_instance,
                                 &dst->entity_instance)) {
                name_fini(dst);
                return -1;
        }

        return 0;
}

static struct name * name_dup_from_user(const struct name __user * src)
{
        struct name * tmp;

        if (!src)
                return NULL;

        tmp = name_create();
        if (!tmp)
                return NULL;
        if (name_cpy_from_user(src, tmp)) {
                name_destroy(tmp);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(name_dup_from_user);

SYSCALL_DEFINE3(ipc_create,
                const struct name __user *, name,
                ipc_process_id_t,           id,
                const char __user *,        type)
{
        long          retval;
        struct name * tn;
        char *        tt;

        tn = name_dup_from_user(name);
        if (!tn)
                return -EFAULT;
        tt = strdup_from_user(type);
        if (!tt) {
                name_destroy(tn);
                return -EFAULT;
        }
        CALL_DEFAULT_PERSONALITY(retval, ipc_create, tn, id, tt);

        name_destroy(tn);
        rkfree(tt);

        return retval;
}

SYSCALL_DEFINE2(ipc_configure,
                ipc_process_id_t,                  id,
                const struct ipcp_config __user *, config)
{
        long retval;

        LOG_MISSING; /* FIXME: Rearrange for copy_from_user() */

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

        LOG_MISSING; /* FIXME: Rearrange for copy_from_user() */

        CALL_DEFAULT_PERSONALITY(retval, sdu_read, id, sdu);

        return retval;
}

SYSCALL_DEFINE2(sdu_write,
                port_id_t,                 id,
                const struct sdu __user *, sdu)
{
        long retval;

        LOG_MISSING; /* FIXME: Rearrange for copy_to_user() */

        CALL_DEFAULT_PERSONALITY(retval, sdu_write, id, sdu);

        return retval;
}

SYSCALL_DEFINE1(connection_create,
                const struct connection __user *, connection)
{
        long              retval;
        struct connection tmp;

        if (copy_from_user(&tmp, connection, sizeof(*connection)))
                return -EFAULT;

        CALL_DEFAULT_PERSONALITY(retval, connection_create, &tmp);

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
