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

#ifndef RINA_POLICIES_H
#define RINA_POLICIES_H

#include <linux/list.h>

#include "rds/rstr.h"

struct policy_parm * policy_param_create(void);
struct policy_parm * policy_param_create_ni(void);
int                  policy_param_destroy(struct policy_parm * param);
const string_t *     policy_param_name(const struct policy_parm * param);
int                  policy_param_name_set(struct policy_parm * param,
                                           string_t *           name);
const string_t *     policy_param_value(const struct policy_parm * param);
int                  policy_param_value_set(struct policy_parm * param,
                                            string_t *           value);

struct policy;

struct policy *      policy_dup_name_version(const struct policy * policy);
struct policy *      policy_create_ni(void);
int                  policy_destroy(struct policy * p);
const string_t *     policy_name(struct policy * policy);
int                  policy_name_set(struct policy * policy,
                                     string_t *      name);
const string_t *     policy_version(struct policy * policy);
int                  policy_version_set(struct policy * policy,
                                        string_t *      version);
struct policy_parm * policy_param_find(struct policy *  policy,
                                       const string_t * name);
int                  policy_param_bind(struct policy *      policy,
                                       struct policy_parm * param);
int                  policy_param_unbind(struct policy *      policy,
                                         struct policy_parm * param);
void policy_for_each(struct policy * policy,
                     void * opaque,
                     int        (* f)(struct policy_parm * entry, void * opaque));

#endif
