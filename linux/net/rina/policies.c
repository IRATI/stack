/*
 * Policies
 *
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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
#include <linux/types.h>

#define RINA_PREFIX "policies"

#include "utils.h"
#include "logs.h"
#include "policies.h"

struct policy_parm {
        string_t *       name;
        string_t *       value;
        struct list_head next;
};

struct policy {
        string_t *       name;
        string_t *       version;
        struct list_head params;
};

struct policy * policy_create_gfp(gfp_t flags)
{
        struct policy * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->params);

        return tmp;
}
EXPORT_SYMBOL(policy_create_gfp);

struct policy * policy_create(void)
{ return policy_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(policy_create);

struct policy * policy_create_ni(void)
{ return policy_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(policy_create_ni);

static struct policy_parm * policy_param_create_gfp(gfp_t flags)
{
        struct policy_parm * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

struct policy_parm * policy_param_create(void)
{ return policy_param_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(policy_param_create);

struct policy_parm * policy_param_create_ni(void)
{ return policy_param_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(policy_param_create_ni);

int policy_param_destroy(struct policy_parm * param)
{
        if (!param)
                return -1;

        if (param->name)  rkfree(param->name);
        if (param->value) rkfree(param->value);
        rkfree(param);

        return 0;
}
EXPORT_SYMBOL(policy_param_destroy);

int policy_destroy(struct policy * p)
{
        struct policy_parm * pos, * nxt;

        if (!p)
                return -1;

        if (p->name) rkfree(p->name);
        if (p->version) rkfree(p->version);

        list_for_each_entry_safe(pos, nxt, &p->params, next) {
                list_del(&pos->next);
                if (policy_param_destroy(pos))
                        return -1;
        }

        rkfree(p);

        return 0;
}
EXPORT_SYMBOL(policy_destroy);

struct policy_parm * policy_param_find_by_name(struct policy *      policy,
                                               struct policy_parm * param)
{
        struct policy_parm * pos;

        if (!policy || ! param)
                return NULL;

        list_for_each_entry(pos, &policy->params, next) {
                if (!strcmp(pos->name, param->name))
                        return pos;
        }

        return NULL;
}
EXPORT_SYMBOL(policy_param_find_by_name);

int policy_param_is_present(struct policy *      policy,
                            struct policy_parm * param)
{
        if (!policy || ! param)
                return 0;

        if (policy_param_find_by_name(policy, param))
                return 1;
        else
                return 0;

}
EXPORT_SYMBOL(policy_param_is_present);

int policy_param_add(struct policy *      policy,
                     struct policy_parm * param)
{
        if (!policy || ! param)
                return -1;

        list_add_tail(&param->next, &policy->params);

        return 0;
}
EXPORT_SYMBOL(policy_param_add);

int policy_param_rem(struct policy *      policy,
                     struct policy_parm * param)
{
        if (!policy || ! param)
                return -1;

        if (!policy_param_is_present(policy, param))
                return -1;

        list_del(&param->next);

        return 0;
}
EXPORT_SYMBOL(policy_param_rem);

int policy_param_rem_and_del(struct policy *      policy,
                             struct policy_parm * param)
{
        if (policy_param_rem(policy, param))
                return -1;

        return policy_param_destroy(param);
}
EXPORT_SYMBOL(policy_param_rem_and_del);

string_t * policy_param_name(struct policy_parm * param)
{
        if (!param)
                return NULL;

        return param->name;
}
EXPORT_SYMBOL(policy_param_name);

string_t * policy_param_value(struct policy_parm * param)
{
        if (!param)
                return NULL;

        return param->value;
}
EXPORT_SYMBOL(policy_param_value);

string_t * policy_name(struct policy * policy)
{
        if (!policy)
                return NULL;

        return policy->name;
}
EXPORT_SYMBOL(policy_name);

string_t * policy_version(struct policy * policy)
{
        if (!policy)
                return NULL;

        return policy->version;
}
EXPORT_SYMBOL(policy_version);

struct list_head * policy_parameters(struct policy * policy)
{
        if (!policy)
                return NULL;

        return &policy->params;
}
EXPORT_SYMBOL(policy_parameters);

int policy_param_name_set(struct policy_parm * param,
                          string_t *           name)
{
        if (!param)
                return -1;

        param->name = name;

        return 0;
}
EXPORT_SYMBOL(policy_param_name_set);

int policy_param_value_set(struct policy_parm * param,
                           string_t *           value)
{
        if (!param)
                return -1;

        param->value = value;

        return 0;
}
EXPORT_SYMBOL(policy_param_value_set);

int policy_name_set(struct policy * policy,
                    string_t *      name)
{
        if (!policy)
                return -1;

        policy->name = name;

        return 0;
}
EXPORT_SYMBOL(policy_name_set);

int policy_version_set(struct policy * policy,
                       string_t *      version)
{
        if (!policy)
                return -1;

        policy->version = version;

        return 0;
}
EXPORT_SYMBOL(policy_version_set);
