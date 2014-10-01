/*
 * Policy set publication/unpublication functions
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/string.h>

#define RINA_PREFIX "ps-factory"

#include "logs.h"
#include "ps-factory.h"

int ps_publish(struct policy_set_list * list,
               struct base_ps_factory * factory)
{
        struct base_ps_factory * cur;

        if (!factory || !factory->name) {
                LOG_ERR("Wrong factory");
                return -1;
        }

        if (!list) {
                LOG_ERR("Null policy set list");
                return -1;
        }

        list_for_each_entry(cur, &list->head, node) {
                if (strcmp(factory->name, cur->name) == 0) {
                        LOG_ERR("%s: policy set '%s' already published",
                                __func__, factory->name);
                        return -1;
                }
        }

        list_add(&factory->node, &list->head);

        LOG_INFO("policy-set '%s' published successfully", factory->name);

        return 0;
}

struct base_ps_factory * ps_lookup(struct policy_set_list * list,
                                   const char *             name)
{
        struct base_ps_factory * cur;

        if (name == NULL) {
                LOG_ERR("%s: NULL name", __func__);
                return NULL;
        }

        if (!list) {
                LOG_ERR("Null policy set list");
                return -1;
        }

        list_for_each_entry(cur, &list->head, node) {
                if (strcmp(name, cur->name) == 0)
                        return cur;
        }

        LOG_ERR("policy set '%s' has not been published", name);

        return NULL;
}

int ps_unpublish(struct policy_set_list * list, const char * name)
{
        struct base_ps_factory * factory;

        factory = ps_lookup(list, name);
        if (factory == NULL)
                return -1;

        /* Don't free here, the plug-in has the ownership. */
        list_del(&factory->node);

        LOG_INFO("policy-set '%s' unpublished successfully",
                 factory->name);

        return 0;
}
