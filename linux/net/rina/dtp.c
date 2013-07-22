/*
 * DTP (Data Transfer Protocol)
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

#define RINA_PREFIX "dtp"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtp.h"

struct dtp * dtp_create(port_id_t id)
{
        struct dtp * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp) {
                LOG_ERR("Cannot create DTP instance");
                return NULL;
        }

        tmp->id = id;
        tmp->state_vector = rkzalloc(sizeof(*tmp->state_vector), GFP_KERNEL);
        if (!tmp->state_vector) {
                LOG_ERR("Cannot create DTP state-vector");

                rkfree(tmp);
                return NULL;
        }

        LOG_DBG("DTP instance created successfully");

        return tmp;
}

int dtp_state_vector_bind(struct dtp *               instance,
                          struct dtcp_state_vector * state_vector)
{
        ASSERT(instance);
        ASSERT(state_vector);

        if (!instance->state_vector) {
                LOG_ERR("DTP instance has no state vector, "
                        "cannot bind DTCP state vector");
                return -1;
        }

        if (instance->state_vector->dtcp_state_vector) {
                if (instance->state_vector->dtcp_state_vector != state_vector) {
                        LOG_ERR("DTP instance already bound to a different "
                                "DTCP state-vector, unbind it first");
                        return -1;
                }

                return 0;
        }

        instance->state_vector->dtcp_state_vector = state_vector;

        return 0;
}

int dtp_state_vector_unbind(struct dtp * instance)
{
        ASSERT(instance);

        if (instance->state_vector)
                instance->state_vector->dtcp_state_vector = NULL;

        return 0;

}

int dtp_destroy(struct dtp * instance)
{
        ASSERT(instance);

        /*
         * NOTE: The state vector can be discarded during long periods of
         *       no traffic
         */
        if (instance->state_vector)
                rkfree(instance->state_vector);
        rkfree(instance);

        LOG_DBG("DTP instance destroyed successfully");

        return 0;
}

int dtp_send(struct dtp *       dtp,
             const struct sdu * sdu)
{
        ASSERT(dtp);
        ASSERT(sdu);

#if 0
        if (dtp_sdu_delimit(instance->dtp, sdu))
                return -1;

        if (dtp_fragment_concatenate_sdu(instance->dtp, sdu))
                return -1;

        if (dtp_sequence_address_sdu(instance->dtp, sdu))
                retun -1;

        if (dtp_crc_sdu(instance->dtp, sdu))
                return -1;

        if (dtp_apply_policy_CsldWinQ(instance->dtp, sdu))
                return -1;
        
        //return instance->dtcp->ops->write(sdu);
#endif

        LOG_MISSING;

        return -1;
}

#if 0
static int sdu_delimit(struct dtp * dtp,
                       struct sdu * sdu)
{
        ASSERT(dtp);
        ASSERT(sdu);

        LOG_MISSING;

        return -1;
}

static sdu_fragment_concatenate(struct dtp * dtp,
                                struct sdu * sdu)
{
        ASSERT(dtp);
        ASSERT(sdu);

        LOG_MISSING;

        return -1;
}

static int sdu_sequence_address(struct dtp * dtp,
                                struct sdu * sdu)
{
        ASSERT(dtp);
        ASSERT(sdu);

        LOG_MISSING;

        return -1;
}

static int sdu_crc(struct dtp * dtp,
                   struct sdu * sdu)
{
        ASSERT(dtp);
        ASSERT(sdu);

        LOG_MISSING;

        return -1;
}

int sdu_apply_policy_CsldWinQ(struct dtp * dtp,
                              struct sdu * sdu)
{
        ASSERT(dtp);
        ASSERT(sdu);

        LOG_MISSING;

        return -1;
}

#endif
