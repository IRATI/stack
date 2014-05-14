/*
 * Policies Configuration
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#include "common.h"

struct p_param;
struct policy;

struct policy *  policy_create(void);
struct policy *  policy_create_ni(void);
struct p_param * policy_param_create(void);
struct p_param * policy_param_create_ni(void);
int              policy_param_destroy(struct p_param * param);
int              policy_destroy(struct policy * p);
int              policy_param_add (struct policy *  policy,
                                   struct p_param * param);
int              policy_param_rem (struct policy *  policy,
                                   struct p_param * param);


#endif
