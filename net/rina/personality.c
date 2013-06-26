/*
 * Personality
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

#include <linux/slab.h>
#include <linux/export.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define RINA_PREFIX "personality"

#include "logs.h"
#include "utils.h"
#include "personality.h"
#include "shim.h"

/* FIXME: Bogus, to be removed ASAP */
struct personality * default_personality = NULL;

static struct kset * personalities       = NULL;

int rina_personality_init(struct kobject * parent)
{
        LOG_DBG("Initializing personality layer");

        if (personalities) {
                LOG_ERR("The personality layer is already initialized, "
                        "bailing out");
                return -1;
        }

        ASSERT(parent);

        personalities = kset_create_and_add("personalities", NULL, parent);
        if (!personalities) {
                LOG_ERR("Cannot initialize personality layer");
                return -1;
        }

        default_personality = NULL;

        LOG_DBG("Personality layer initialized successfully");

        return 0;
}

void rina_personality_exit(void)
{
        LOG_DBG("Finalizing personality layer");

        kset_unregister(personalities);
        personalities    = NULL;

        default_personality = NULL;        

        LOG_DBG("Personality layer finalized successfully");
}

static int is_label_ok(const char * label)
{
        LOG_DBG("Checking label");

        if (!label) {
                LOG_DBG("Label is empty");
                return 0;
        }
        if (strlen(label) == 0) {
                LOG_DBG("Label has 0 length");
                return 0;
        }

        LOG_DBG("Label is ok");

        return 1;
}

static int are_ops_ok(const struct personality_ops * ops)
{
        LOG_DBG("Checking ops");

        if (!ops) {
                LOG_ERR("Ops are empty");
                return 0;
        }

        if ((ops->init && !ops->fini) || (!ops->init && ops->fini)) {
                LOG_ERR("Bogus ops initializer/finalizer couple");
                return 0;
        }

        if (ops->ipc_create         &&
            ops->ipc_configure      &&
            ops->ipc_destroy        &&
            ops->sdu_read           &&
            ops->sdu_write          &&
            ops->connection_create  &&
            ops->connection_destroy &&
            ops->connection_update) {
                LOG_DBG("Ops are ok");
                return 1;
        }

        LOG_ERR("Bogus ops hooks");

        return 0;
}

struct personality * rina_personality_register(const char *             label,
                                               void *                   data,
                                               struct personality_ops * ops)
{
        struct kobject *     tmp;
        struct personality * pers;

        if (!is_label_ok(label)) {
                LOG_ERR("Label is bogus, cannot register personality");
                return NULL;
        }

        if (!are_ops_ok(ops)) {
                LOG_ERR("Cannot register personality '%s', ops are bogus",
                        label);
                return NULL;
        }

        LOG_DBG("Registering personality '%s'", label);

        ASSERT(personalities);

        tmp = kset_find_obj(personalities, label);
        if (tmp) {
                kobject_put(tmp);
                LOG_ERR("Personality '%s' already registered, bailing out",
                        label);
                return NULL;
        }
        
        pers = kzalloc(sizeof(struct personality), GFP_KERNEL);
        if (!pers) {
                LOG_ERR("Cannot allocate %zu bytes of memory", sizeof(*pers));
                return NULL;
        }

        pers->data      = data;
        pers->ops       = ops;
        pers->hierarchy = kset_create_and_add(label,
                                              NULL,
                                              &personalities->kobj);
        if (!pers->hierarchy) {
                LOG_ERR("Cannot initialize personality '%s' hierarchy", label);
                kfree(pers);
                return NULL;
        }

        /* Double checking for bugs */
        label = pers->hierarchy->kobj.name;

        ASSERT(pers->ops);

        if (pers->ops->init) {
                LOG_DBG("Calling personality '%s' initializer", label);
                if (!pers->ops->init(pers->data)) {
                        LOG_ERR("Could not initialize personality '%s'",
                                label);
                        kset_unregister(pers->hierarchy);
                        kfree(pers);
                        return NULL;
                }
                LOG_DBG("Personality '%s' initialized successfully", label);
        }

        if (!default_personality) {
                default_personality = pers;
                LOG_INFO("Default personality set to '%s'", label);
        }

        LOG_DBG("Personality '%s' registered successfully", label);

        return pers;
}
EXPORT_SYMBOL(rina_personality_register);

int rina_personality_unregister(struct personality * pers)
{
        struct kobject * tmp;
        const char *     label;

        if (!pers) {
                LOG_ERR("Bogus personality, cannot unregister");
                return -1;
        }

        ASSERT(pers);
        ASSERT(pers->hierarchy);
        ASSERT(pers->hierarchy->kobj.name);

        label = pers->hierarchy->kobj.name;

        LOG_DBG("Unregistering personality '%s'", label);

        ASSERT(personalities);

        tmp = kset_find_obj(personalities, label);
        if (!tmp) {
                LOG_ERR("Personality '%s' not registered, bailing out",
                        label);
                return -1;
        }

        ASSERT(pers->ops);

        if (pers->ops->fini) {
                LOG_DBG("Calling personality '%s' finalizer", label);
                pers->ops->fini(pers->data);
                LOG_DBG("Personality '%s' finalized successfully", label);
        }

        kobject_put(tmp);

        kset_unregister(pers->hierarchy);

        if (default_personality == pers) {
                LOG_INFO("Re-setting default personality");
                default_personality = 0;
        }

        kfree(pers);

        LOG_DBG("Personality unregistered successfully");

        return 0;
}
EXPORT_SYMBOL(rina_personality_unregister);
