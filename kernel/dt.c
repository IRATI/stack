/*
 * DT (Data Transfer)
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

#define RINA_PREFIX "dt"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dt.h"
#include "dt-utils.h"

static struct dt_sv default_sv = {
        .max_flow_pdu_size    = UINT_MAX,
        .max_flow_sdu_size    = UINT_MAX,
        .MPL                  = 1000,
        .R                    = 100,
        .A                    = 0,
        .tr                   = 0,
        .rcv_left_window_edge = 0,
        .window_closed        = false,
        .drf_flag             = false,
};

int dt_sv_init(struct dt * instance,
               uint_t      mfps,
               uint_t      mfss,
               u_int32_t   mpl,
               timeout_t   a,
               timeout_t   r,
               timeout_t   tr)
{
        ASSERT(instance);

        instance->sv->max_flow_pdu_size = mfps;
        instance->sv->max_flow_sdu_size = mfss;
        instance->sv->MPL               = mpl;
        instance->sv->A                 = a;
        instance->sv->R                 = r;
        instance->sv->tr                = tr;

        return 0;
}

struct dt * dt_create(void)
{
        struct dt * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->sv = rkzalloc(sizeof(*tmp->sv), GFP_KERNEL);
        if (!tmp->sv) {
                rkfree(tmp);
                return NULL;
        }

        *tmp->sv = default_sv;
        spin_lock_init(&tmp->lock);

        return tmp;
}

int dt_destroy(struct dt * dt)
{
	struct dtp * dtp = NULL;
	struct dtcp * dtcp = NULL;
	struct cwq * cwq = NULL;
	struct rtxq * rtxq = NULL;
	int ret = 0;

        if (!dt)
                return -1;

	spin_lock_bh(&dt->lock);

        if (dt->dtp) {
                dtp = dt->dtp;
                dt->dtp = NULL; /* Useful */
        }

        if (dt->dtcp) {
                dtcp = dt->dtcp;
                dt->dtcp = NULL; /* Useful */
        }

        if (dt->cwq) {
                cwq = dt->cwq;
                dt->cwq = NULL; /* Useful */
        }

        if (dt->rtxq) {
        	rtxq = dt->rtxq;
        	dt->rtxq = NULL; /* Useful */
        }

        spin_unlock_bh(&dt->lock);

        if (dtp) {
        	if (dtp_destroy(dtp)) {
        		LOG_WARN("Error destroying DTP");
        		ret = -1;
        	}
        }

        if (dtcp) {
        	if (dtcp_destroy(dtcp)) {
        		LOG_WARN("Error destroying DTCP");
        		ret = -1;
        	}
        }

        if (cwq) {
        	if (cwq_destroy(cwq)) {
        		LOG_WARN("Error destroying CWQ");
        		ret = -1;
        	}
        }

        if (rtxq) {
                if (rtxq_destroy(rtxq)) {
                        LOG_ERR("Failed to destroy rexmsn queue");
                        ret = -1;
                }
        }

        rkfree(dt->sv);
        rkfree(dt);

        LOG_DBG("Instance %pK destroyed. Result %d", dt, ret);

        return ret;
}
