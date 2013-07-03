/*
 * Utilities (workarounds)
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
#include <linux/kobject.h>
#include <linux/export.h>

#if CONFIG_RINA_PERSONALITY_DEFAULT_MODULE
/* FIXME:
 *   In kernel 3.9.2, there's no EXPORT_SYMBOL for kset_find_object so this
 *   is a copy and paste from lib/kobject.c. Please fix that ASAP
 */
static struct kobject *kobject_get_unless_zero(struct kobject *kobj)
{
        if (!kref_get_unless_zero(&kobj->kref))
                kobj = NULL;
        return kobj;
}

struct kobject *kset_find_obj(struct kset *kset, const char *name)
{
        struct kobject *k;
        struct kobject *ret = NULL;

        spin_lock(&kset->list_lock);

        list_for_each_entry(k, &kset->list, entry) {
                if (kobject_name(k) && !strcmp(kobject_name(k), name)) {
                        ret = kobject_get_unless_zero(k);
                        break;
                }
        }

        spin_unlock(&kset->list_lock);
        return ret;
}
EXPORT_SYMBOL(kset_find_obj);

#endif
