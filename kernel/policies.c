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

struct policy * policy_create_ni(void)
{ return policy_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(policy_create_ni);

struct policy * policy_dup_name_version(const struct policy * src)
{
	struct policy * tmp;

	if (!src) {
		LOG_ERR("Bogus policy passed");
		return NULL;
	}

	tmp = policy_create();
	if (!tmp)
		return NULL;

	if (string_dup(src->name, &tmp->name)){
		policy_destroy(tmp);
		return NULL;
	}

	if (string_dup(src->version, &tmp->version)){
		policy_destroy(tmp);
		return NULL;
	}

	return tmp;
}
EXPORT_SYMBOL(policy_dup_name_version);

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

struct policy_parm * policy_param_find(struct policy *  policy,
                                       const string_t * name)
{
        struct policy_parm * pos;

        if (!policy || ! name)
                return NULL;

        list_for_each_entry(pos, &policy->params, next) {
                if (!strcmp(pos->name, name))
                        return pos;
        }

        return NULL;
}
EXPORT_SYMBOL(policy_param_find);

int policy_param_bind(struct policy *      policy,
                      struct policy_parm * param)
{
        if (!policy || ! param)
                return -1;

        list_add_tail(&param->next, &policy->params);

        return 0;
}
EXPORT_SYMBOL(policy_param_bind);

int policy_param_unbind(struct policy *      policy,
                        struct policy_parm * param)
{
        if (!policy || ! param)
                return -1;

        if (param->name)
                return -1;

        if (!policy_param_find(policy, param->name))
                return -1;

        list_del(&param->next);

        return 0;
}
EXPORT_SYMBOL(policy_param_unbind);

const string_t * policy_param_name(const struct policy_parm * param)
{
        if (!param)
                return NULL;

        return param->name;
}
EXPORT_SYMBOL(policy_param_name);

const string_t * policy_param_value(const struct policy_parm * param)
{
        if (!param)
                return NULL;

        return param->value;
}
EXPORT_SYMBOL(policy_param_value);

const string_t * policy_name(struct policy * policy)
{
        if (!policy)
                return NULL;

        if (!policy->name)
        	return NULL;

        if (string_len(policy->name) == 0)
        	return NULL;

        return policy->name;
}
EXPORT_SYMBOL(policy_name);

const string_t * policy_version(struct policy * policy)
{
        if (!policy)
                return NULL;

        return policy->version;
}
EXPORT_SYMBOL(policy_version);

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

void policy_for_each(struct policy * policy,
                     void * opaque,
                     int        (* f)(struct policy_parm * entry, void * opaque))
{
        struct policy_parm * entry;

        list_for_each_entry(entry, &policy->params, next)
                f(entry, opaque);

}
EXPORT_SYMBOL(policy_for_each);
