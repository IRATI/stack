/*
 * Connection
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
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

#include <linux/export.h>
#include <linux/types.h>

#define RINA_PREFIX "connection"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "common.h"
#include "connection.h"
#include "dtcp-utils.h"
#include "policies.h"

struct connection * connection_create(void)
{
        struct connection * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}
EXPORT_SYMBOL(connection_create);

struct conn_policies * conn_policies_create(void)
{
        struct conn_policies * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->dtcp_cfg = dtcp_config_create();
        if (!tmp->dtcp_cfg) {
                LOG_ERR("Could not create dtcp_config");
                rkfree(tmp);
                return NULL;
        }

        tmp->initial_sequence_number = policy_create();
        if (!tmp->initial_sequence_number) {
                LOG_ERR("Could not create initial_sequence_number");
                dtcp_config_destroy(tmp->dtcp_cfg);
                rkfree(tmp);
                return NULL;
        }

        tmp->receiver_inactivity_timer = policy_create();
        if (!tmp->receiver_inactivity_timer) {
                LOG_ERR("Could not create receiver_inactivity_timer policy");
                dtcp_config_destroy(tmp->dtcp_cfg);
                rkfree(tmp);
                return NULL;
        }

        tmp->sender_inactivity_timer = policy_create();
        if (!tmp->sender_inactivity_timer) {
                LOG_ERR("Could not create sender_inactivity_timer policy");
                dtcp_config_destroy(tmp->dtcp_cfg);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

struct connection *
connection_dup_from_user(const struct connection __user * conn)
{
        struct connection * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        if (copy_from_user(tmp, conn, sizeof(*tmp)))
                return NULL;

        return tmp;
}

int conn_policies_destroy(struct conn_policies * cp_params)
{
        int retval = 0;

        if (!cp_params)
                return -1;

        if (cp_params->dtcp_cfg)
                if (dtcp_config_destroy(cp_params->dtcp_cfg))
                        retval = -1;

        if (cp_params->initial_sequence_number)
                if (policy_destroy(cp_params->initial_sequence_number))
                        retval = -1;

        rkfree(cp_params);
        return retval;
}
EXPORT_SYMBOL(conn_policies_destroy);

int connection_destroy(struct connection * conn)
{
        if (!conn)
                return -1;

        if (conn->policies_params)
                if (conn_policies_destroy(conn->policies_params))
                        return -1;

        rkfree(conn);

        return 0;
}
EXPORT_SYMBOL(connection_destroy);
