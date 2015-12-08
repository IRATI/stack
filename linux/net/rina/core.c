/*
 * Core
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
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define RINA_PREFIX "core"

#include "logs.h"
#include "debug.h"
#include "rnl.h"
#include "kipcm.h"
#include "utils.h"

#define MK_RINA_VERSION(MAJOR, MINOR, MICRO)                            \
        (((MAJOR & 0xFF) << 24) | ((MINOR & 0xFF) << 16) | (MICRO & 0xFFFF))

#define RINA_VERSION_MAJOR(V) ((V >> 24) & 0xFF)
#define RINA_VERSION_MINOR(V) ((V >> 16) & 0xFF)
#define RINA_VERSION_MICRO(V) ((V      ) & 0xFFFF)

static struct kset * root_kset = NULL;
static uint32_t      version   = MK_RINA_VERSION(1, 4, 1);

#if 0
uint32_t rina_version(void)
{ return version; }
EXPORT_SYMBOL(rina_version);
#endif

static ssize_t version_show(struct kobject *        kobj,
                            struct kobj_attribute * attr,
                            char *                  buff)
{
        return sprintf(buff, "%d.%d.%d\n",
                       RINA_VERSION_MAJOR(version),
                       RINA_VERSION_MINOR(version),
                       RINA_VERSION_MICRO(version));
}
RINA_ATTR_RO(version);

static struct attribute * root_attrs[] = {
        &version_attr.attr,
        NULL
};

static struct attribute_group root_attr_group = {
        .attrs = root_attrs
};

static int __init rina_core_init(void)
{
        LOG_DBG("RINA stack initializing");

        if (rina_debug_init())
                return -1;

        LOG_DBG("Creating root kset");
        root_kset = kset_create_and_add("rina", NULL, NULL);
        if (!root_kset) {
                LOG_ERR("Cannot initialize root kset, bailing out");
                rina_debug_exit();
                return -1;
        }

        LOG_DBG("Creating sysfs group");
        if (sysfs_create_group(&root_kset->kobj, &root_attr_group)) {
                LOG_ERR("Cannot create root sysfs group");
                kset_unregister(root_kset);
                rina_debug_exit();
                return -1;
        }

        LOG_DBG("Initializing RNL");
        if (rnl_init()) {
                kset_unregister(root_kset);
                rina_debug_exit();
                return -1;
        }

	LOG_DBG("Initializing KIPCM");
        if (kipcm_init(&root_kset->kobj)) {
                rnl_exit();
                kset_unregister(root_kset);
                rina_debug_exit();
                return -1;
        }

        LOG_INFO("RINA stack v%d.%d.%d initialized",
                 RINA_VERSION_MAJOR(version),
                 RINA_VERSION_MINOR(version),
                 RINA_VERSION_MICRO(version));

        LOG_INFO("Don't panic ...");

        return 0;
}

__initcall(rina_core_init);
