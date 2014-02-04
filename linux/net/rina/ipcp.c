/*
 *  IPC Processes layer
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

#define RINA_PREFIX "ipcp"

#include "logs.h"
#include "debug.h"
#include "ipcp.h"

static bool has_common_hooks(struct ipcp_instance_ops * ops)
{
        ASSERT(ops);

        LOG_MISSING;

        return false;
}

bool ipcp_instance_is_shim(struct ipcp_instance_ops * ops)
{
        if (!ops)
                return false;

        if (!has_common_hooks(ops))
                return false;

        LOG_MISSING;

        return false;
}
EXPORT_SYMBOL(ipcp_instance_is_shim);

bool ipcp_instance_is_normal(struct ipcp_instance_ops * ops)
{
        if (!ops)
                return false;

        if (!has_common_hooks(ops))
                return false;

        LOG_MISSING;

        return false;
}
EXPORT_SYMBOL(ipcp_instance_is_normal);

/* FIXME: Bogus way to check for functionalities */
bool ipcp_instance_is_ok(struct ipcp_instance_ops * ops)
{ return ops && (ipcp_instance_is_shim(ops) || ipcp_instance_is_normal(ops)); }
EXPORT_SYMBOL(ipcp_instance_is_ok);
