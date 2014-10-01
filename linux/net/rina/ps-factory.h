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

#ifndef RINA_PS_FACTORY_H
#define RINA_PS_FACTORY_H

#include <linux/list.h>

#define RINA_PS_DEFAULT_NAME            "default"

#define PARAMETER_DESC_MAX_LEN  32
#define POLICY_SET_NAME_MAX_LEN 64

struct parameter_desc {
        char type;
        char name[PARAMETER_DESC_MAX_LEN];
};

struct base_ps_factory {
        /* A name for this policy-set. */
        char                    name[POLICY_SET_NAME_MAX_LEN];

        /* Policy-set-specific parameters. */
        struct parameter_desc * parameters;
        unsigned int            num_parameters;

        struct list_head        node;
};

struct policy_set_list {
        struct list_head head;
};

int                      ps_publish(struct policy_set_list * list,
                                    struct base_ps_factory * factory);
struct base_ps_factory * ps_lookup(struct policy_set_list * list,
                                   const char *       name);
int                      ps_unpublish(struct policy_set_list * list,
                                      const char *       name);

#endif
