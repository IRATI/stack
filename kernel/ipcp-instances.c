/*
 *  IPC Process Instances
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

#include <linux/types.h>

#define RINA_PREFIX "ipcp-instances"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "ipcp-instances.h"

struct dt_cons * dt_cons_dup_gfp(gfp_t                  flags,
                                 const struct dt_cons * dt_cons)
{
        struct dt_cons * tmp;

        if (!dt_cons)
                return NULL;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        memcpy(tmp, dt_cons, sizeof(*tmp));

        return tmp;
}

struct dt_cons * dt_cons_dup(const struct dt_cons * dt_cons)
{ return dt_cons_dup_gfp(GFP_KERNEL, dt_cons); }
EXPORT_SYMBOL(dt_cons_dup);

static bool has_common_hooks(struct ipcp_instance_ops * ops)
{
        ASSERT(ops);

        if (ops->assign_to_dif     &&
            ops->update_dif_config &&
            ops->du_write         &&
            ops->ipcp_name)
                return true;

        return false;
}

bool ipcp_instance_is_shim(struct ipcp_instance_ops * ops)
{
        if (!ops)
                return false;

        if (!has_common_hooks(ops))
                return false;

        if (ops->flow_deallocate        &&
            ops->flow_allocate_request  &&
            ops->flow_allocate_response &&
            ops->application_register   &&
            ops->application_unregister &&
            ops->flow_unbinding_ipcp    &&
            ops->du_write)
                return true;

        return false;
}
/* EXPORT_SYMBOL(ipcp_instance_is_shim); */

bool ipcp_instance_is_normal(struct ipcp_instance_ops * ops)
{
        if (!ops)
                return false;

        if (!has_common_hooks(ops))
                return false;

        if (ops->connection_create         ||
            ops->connection_update         ||
            ops->connection_destroy        ||
            ops->connection_create_arrived ||
            ops->du_write                 ||
            ops->mgmt_du_write            ||
            ops->mgmt_du_post             ||
            ops->pff_add                   ||
            ops->pff_remove                ||
            ops->pff_dump                  ||
            ops->flow_prebind              ||
            ops->flow_binding_ipcp         ||
            ops->flow_unbinding_ipcp       ||
            ops->flow_deallocate           ||
            ops->du_enqueue)
                return true;

        return false;
}
/* EXPORT_SYMBOL(ipcp_instance_is_normal); */

/* FIXME: Bogus way to check for functionalities */
bool ipcp_instance_is_ok(struct ipcp_instance_ops * ops)
{ return ops && (ipcp_instance_is_shim(ops) || ipcp_instance_is_normal(ops)); }
/* EXPORT_SYMBOL(ipcp_instance_is_ok); */
