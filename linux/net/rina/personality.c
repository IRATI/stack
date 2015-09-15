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

#include <linux/export.h>
#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define RINA_PREFIX "personality"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "personality.h"

/* FIXME: Bogus, to be removed ASAP */
struct personality * default_personality = NULL;

static struct kset * personalities       = NULL;

static ssize_t personality_show(struct kobject *   kobj,
                                struct attribute * attr,
                                char *             buf)
{ return sprintf(buf, "%s", kobject_name(kobj)); }

static const struct sysfs_ops personality_sysfs_ops = {
        .show = personality_show
};

static struct kobj_type personality_ktype = {
        .sysfs_ops     = &personality_sysfs_ops,
        .default_attrs = NULL,
        .release       = NULL,
};

int rina_personality_init(struct kobject * parent)
{
        LOG_DBG("Initializing personality layer");

        if (personalities) {
                LOG_ERR("The personality layer is already initialized, "
                        "bailing out");
                return -1;
        }

        ASSERT(personalities == NULL);

        personalities = kset_create_and_add("personalities", NULL, parent);
        if (!personalities) {
                LOG_ERR("Cannot initialize personality layer");
                return -1;
        }

        ASSERT(personalities       != NULL);
        ASSERT(default_personality == NULL);

        LOG_DBG("Personality layer initialized successfully");

        return 0;
}

void rina_personality_exit(void)
{
        LOG_DBG("Finalizing personality layer");

        ASSERT(personalities       != NULL);
        ASSERT(default_personality != NULL);

        /* FIXME: Check pending objects and flush 'em all */

        kset_unregister(personalities);
        personalities       = NULL;
        default_personality = NULL;

        LOG_DBG("Personality layer finalized successfully");
}

static bool is_string_ok(const char * name)
{
        LOG_DBG("Checking name");

        if (!name) {
                LOG_ERR("Name is empty");
                return false;
        }
        if (strlen(name) == 0) {
                LOG_ERR("Name has 0 length");
                return false;
        }

        LOG_DBG("Name is ok");

        return true;
}

static int are_ops_ok(const struct personality_ops * ops)
{
        LOG_DBG("Checking ops %pK", ops);

        if (!ops) {
                LOG_ERR("Ops are empty");
                return 0;
        }

        if ((ops->init && !ops->fini) || (!ops->init && ops->fini)) {
                LOG_ERR("Bogus ops initializer/finalizer couple");
                return 0;
        }

        if (!(ops->ipc_create    &&
              ops->ipc_destroy)) {
                LOG_ERR("Bogus IPC related ops");
                return 0;
        }

        if (!(ops->sdu_read  &&
              ops->sdu_write)) {
                LOG_ERR("Bogus SDU related ops");
                return 0;
        }

        if (!(ops->flow_create  &&
              ops->flow_destroy)) {
                LOG_ERR("Bogus Port ID related ops");
                return 0;
        }

        if (!(ops->mgmt_sdu_read  &&
              ops->mgmt_sdu_write)) {
                LOG_ERR("Bogus Management SDU related ops");
                return 0;
        }

        LOG_DBG("Ops are ok");

        return 1;
}

#define to_personality(O) container_of(O, struct personality, kobj)

static struct personality * personality_find(const char * name)
{
        struct kobject * k;

        ASSERT(name);

        k = kset_find_obj(personalities, name);
        if (k) {
                kobject_put(k);
                return to_personality(k);
        }

        return NULL;
}

/* FIXME: Has to be rearranged properly, it's bogus at the moment */
static personality_id id_get(void)
{
        static personality_id ids = 0;

        return ids++;
}

static void id_put(personality_id id)
{ }

struct personality * rina_personality_register(const char *              name,
                                               struct personality_data * data,
                                               struct personality_ops *  ops)
{
        struct personality * tmp;
        struct personality * pers;

        if (!is_string_ok(name)) {
                LOG_ERR("Name is bogus, cannot register personality");
                return NULL;
        }

        if (!are_ops_ok(ops)) {
                LOG_ERR("Cannot register personality '%s', ops are bogus",
                        name);
                return NULL;
        }

        LOG_DBG("Registering personality '%s'", name);

        ASSERT(personalities);

        tmp = personality_find(name);
        if (tmp) {
                LOG_ERR("Personality '%s' already registered", name);
                return NULL;
        }

        pers = rkzalloc(sizeof(*pers), GFP_KERNEL);
        if (!pers)
                return NULL;

        pers->id   = id_get();
        pers->data = data;
        pers->ops  = ops;

        pers->kobj.kset = personalities;
        if (kobject_init_and_add(&pers->kobj, &personality_ktype, NULL,
                                 "%s", name)) {
                LOG_ERR("Cannot add personality '%s' to the set", name);
                kobject_put(&pers->kobj);
                /*
                 * FIXME: To be removed once personality_ktype.release
                 * gets implemented
                 */
                rkfree(pers);
                return NULL;
        }

        /* Double checking for bugs */
        name = kobject_name(&pers->kobj);

        ASSERT(pers->ops);

        if (pers->ops->init) {
                LOG_DBG("Calling personality '%s' initializer", name);
                if (pers->ops->init(&pers->kobj, pers->id, pers->data)) {
                        LOG_ERR("Could not initialize personality '%s'", name);
                        kobject_put(&pers->kobj);
                        rkfree(pers); /* FIXME: As the note before */
                        return NULL;
                }
                LOG_DBG("Personality '%s' initialized successfully", name);
        }

        if (!default_personality) {
                default_personality = pers;
                LOG_INFO("Default personality set to '%s'", name);
        }

        LOG_DBG("Personality '%s' registered successfully", name);

        return pers;
}
EXPORT_SYMBOL(rina_personality_register);

int rina_personality_unregister(struct personality * pers)
{
        struct personality * tmp;
        const char *         name;

        if (!pers) {
                LOG_ERR("Bogus personality, cannot unregister");
                return -1;
        }

        ASSERT(pers);

        name = kobject_name(&pers->kobj);

        ASSERT(name);
        ASSERT(is_string_ok(name));

        LOG_DBG("Unregistering personality '%s'", name);

        ASSERT(personalities);

        tmp = personality_find(name);
        if (!tmp) {
                LOG_ERR("Personality '%s' is not registered, bailing out",
                        name);
                return -1;
        }

        ASSERT(pers->ops);

        if (pers->ops->fini) {
                LOG_DBG("Calling personality '%s' finalizer", name);
                if (pers->ops->fini(pers->data)) {
                        LOG_CRIT("Could not finalize personality '%s', "
                                 "the system might become unstable", name);
                } else
                        LOG_DBG("Personality '%s' finalized successfully",
                                name);
        }

        id_put(pers->id);

        /* Do not use name after this point */
        kobject_put(&pers->kobj);

        if (default_personality == pers) {
                LOG_INFO("Re-setting default personality");
                default_personality = NULL;
        }

        rkfree(pers); /* FIXME: To be removed */

        LOG_DBG("Personality unregistered successfully");

        return 0;
}
EXPORT_SYMBOL(rina_personality_unregister);
