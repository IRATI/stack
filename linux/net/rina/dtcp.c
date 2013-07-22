/*
 * DTCP (Data Transfer Control Protocol)
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

#define RINA_PREFIX "dtcp"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtcp.h"

struct dtcp * dtcp_create(void)
{
        struct dtcp * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp) {
                LOG_ERR("Cannot create DTCP state-vector");
                return NULL;
        }

        tmp->state_vector = rkzalloc(sizeof(*tmp->state_vector), GFP_KERNEL);
        if (!tmp->state_vector) {
                LOG_ERR("Cannot create DTCP state-vector");

                rkfree(tmp);
                return NULL;
        }

        LOG_DBG("DTCP instance created successfully");

        return tmp;
}

int dtcp_destroy(struct dtcp * instance)
{
        ASSERT(instance);

        /*
         * NOTE: The state vector can be discarded during long periods of
         *       no traffic
         */
        if (instance->state_vector)
                rkfree(instance->state_vector);
        rkfree(instance);

        LOG_DBG("DTCP instance destroyed successfully");

        return 0;
}

int dtcp_state_vector_bind(struct dtcp *             instance,
                           struct dtp_state_vector * state_vector)
{
        ASSERT(instance);
        ASSERT(state_vector);

        if (!instance->state_vector) {
                LOG_ERR("DTCP instance has no state vector, "
                        "cannot bind DTP state vector");
                return -1;
        }

        if (instance->state_vector->dtp_state_vector) {
                if (instance->state_vector->dtp_state_vector != state_vector) {
                        LOG_ERR("DTCP instance already bound to a different "
                                "DTP state-vector, unbind it first");
                        return -1;
                }

                return 0;
        }

        instance->state_vector->dtp_state_vector = state_vector;

        return 0;
}

int dtcp_state_vector_unbind(struct dtcp * instance)
{
        ASSERT(instance);

        if (instance->state_vector)
                instance->state_vector->dtp_state_vector = NULL;

        return 0;
}
