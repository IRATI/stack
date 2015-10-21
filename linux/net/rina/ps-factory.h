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
#include <linux/module.h>
#include <linux/mutex.h>
#include "common.h"

#define RINA_PS_DEFAULT_NAME            "default"

#define POLICY_SET_NAME_MAX_LEN 64

struct ps_factory;

/* Base class for the IPCP components. */
struct rina_component {
        struct ps_factory *       ps_factory;
        struct ps_base *          ps;
        struct mutex              ps_lock;
};

/* A base class for policy sets. */
struct ps_base {
        /* Method for setting policy-set-specific parameters. */
        int (*set_policy_set_param)(struct ps_base * ps,
                                    const char * name,
                                    const char * value);
};

struct ps_factory {
        /* A name for this policy-set. */
        char                    name[POLICY_SET_NAME_MAX_LEN];

        /* Factory callbacks. */
        struct ps_base * (*create)(struct rina_component * component);
        void (*destroy)(struct ps_base *);

        /* The module that published this policy-set. */
        struct module           * owner;

        /* Used to implement per-component list of available
         * policy sets. */
        struct list_head        node;
};

struct policy_set_list {
        struct list_head head;
};

#define PS_SEL_TRANS_PENDING            0x02
#define PS_SEL_TRANS_COMMITTED          0x09
#define PS_SEL_TRANS_ABORTED            0x10

struct ps_select_transaction {
        unsigned int state;
        struct ps_base *candidate_ps;
        struct ps_factory *candidate_ps_factory;
};

int                      ps_publish(struct policy_set_list * list,
                                    struct ps_factory * factory);
struct ps_factory * ps_lookup(struct policy_set_list * list,
                                   const char *       name);
int                      ps_unpublish(struct policy_set_list * list,
                                      const char *       name);

void rina_component_init(struct rina_component * comp);

void rina_component_fini(struct rina_component * comp);

void base_select_policy_set_start(struct rina_component * comp,
                                  struct ps_select_transaction * trans,
                                  struct policy_set_list * list,
                                  const string_t * name);

void base_select_policy_set_finish(struct rina_component * comp,
                                   struct ps_select_transaction * trans);

int base_set_policy_set_param(struct rina_component *comp,
                              const char * path,
                              const char * name,
                              const char * value);

void ps_factory_parse_component_id(const string_t *path, size_t *cmplen,
				   size_t *offset);

void ps_factory_nop_policy(void);

#endif
