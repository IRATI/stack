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

struct sv {
        uint_t    max_flow_pdu_size;
        uint_t    max_flow_sdu_size;
        timeout_t MPL;
        timeout_t R;
        timeout_t A;
        seq_num_t rcv_left_window_edge;
};

struct dt {
        struct sv *     sv;
        struct dtp *    dtp;
        struct dtcp *   dtcp;

        struct cwq *    cwq;
        struct rtxq *   rtxq;

        spinlock_t      lock;
};

struct dt * dt_create(void)
{
        struct dt * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        spin_lock_init(&tmp->lock);

        return tmp;
}

int dt_destroy(struct dt * dt)
{
        if (!dt)
                return -1;

        if (dt->dtp) {
                LOG_ERR("DTP %pK is still bound to instace %pK, "
                        "unbind it first", dt->dtp, dt);
                return -1;
        }
        if (dt->dtcp) {
                LOG_ERR("DTCP %pK is still bound to DT %pK, "
                        "unbind it first", dt->dtcp, dt);
                return -1;
        }

        if (dt->cwq) {
                if (cwq_destroy(dt->cwq)) {
                        LOG_ERR("Failed to destroy closed window queue");
                        return -1;
                }
                dt->cwq = NULL; /* Useful */
        }

        if (dt->rtxq) {
                if (rtxq_destroy(dt->rtxq)) {
                        LOG_ERR("Failed to destroy rexmsn queue");
                        return -1;
                }
                dt->rtxq = NULL; /* Useless */
        }

        rkfree(dt);

        LOG_DBG("Instance %pK destroyed successfully", dt);

        return 0;
}

int dt_dtp_bind(struct dt * dt, struct dtp * dtp)
{
        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot bind DTP");
                return -1;
        }
        if (!dtp) {
                LOG_ERR("Cannot bind NULL DTP to instance %pK", dt);
                return -1;
        }

        spin_lock(&dt->lock);
        if (dt->dtp) {
                LOG_ERR("A DTP instance is already bound to instance %pK, "
                        "unbind it first", dt);
                spin_unlock(&dt->lock);
                return -1;
        }
        dt->dtp = dtp;
        spin_unlock(&dt->lock);

        return 0;
}

struct dtp * dt_dtp_unbind(struct dt * dt)
{
        struct dtp * tmp;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot unbind DTP");
                return NULL;
        }

        spin_lock(&dt->lock);
        if (!dt->dtp) {
                LOG_ERR("No DTP instance bound to instance %pK, "
                        "cannot bind", dt);
                spin_unlock(&dt->lock);
                return NULL;
        }

        tmp     = dt->dtp;
        dt->dtp = NULL;
        spin_unlock(&dt->lock);

        return tmp;
}

int dt_dtcp_bind(struct dt * dt, struct dtcp * dtcp)
{
        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot bind DTCP");
                return -1;
        }
        if (!dtcp) {
                LOG_ERR("Cannot bind NULL DTCP to instance %pK", dt);
                return -1;
        }

        spin_lock(&dt->lock);
        if (dt->dtcp) {
                LOG_ERR("A DTCP instance already bound to instance %pK, "
                        "unbind it first", dt);
                spin_unlock(&dt->lock);
                return -1;
        }

        dt->cwq = cwq_create();
        if (!dt->cwq) {
                LOG_ERR("Failed to create closed window queue");
                spin_unlock(&dt->lock);
                return -1;
        }

        dt->rtxq = rtxq_create();
        if (!dt->rtxq) {
                LOG_ERR("Failed to create rexmsn queue");
                if (cwq_destroy(dt->cwq))
                        LOG_ERR("Failed to destroy closed window queue");
                spin_unlock(&dt->lock);
                return -1;
        }

        dt->dtcp = dtcp;
        spin_unlock(&dt->lock);

        return 0;
}

struct dtcp * dt_dtcp_unbind(struct dt * dt)
{
        struct dtcp * tmp;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot unbind DTCP");
                return NULL;
        }

        spin_lock(&dt->lock);
        if (!dt->dtcp) {
                LOG_ERR("No DTCP bound to instance %pK", dt);
                spin_unlock(&dt->lock);
                return NULL;
        }

        if (dt->cwq) {
                if (cwq_destroy(dt->cwq)) {
                        LOG_ERR("Failed to destroy closed window queue");
                        spin_unlock(&dt->lock);
                        return NULL;
                }
                dt->cwq = NULL;
        }

        if (dt->rtxq) {
                if (rtxq_destroy(dt->rtxq)) {
                        LOG_ERR("Failed to destroy rexmsn queue");
                        spin_unlock(&dt->lock);
                        return NULL;
                }
                dt->rtxq = NULL;
        }

        tmp      = dt->dtcp;
        dt->dtcp = NULL;
        spin_unlock(&dt->lock);

        return tmp;
}

struct dtp * dt_dtp(struct dt * dt)
{
        struct dtp * tmp;

        if (!dt)
                return NULL;

        spin_lock(&dt->lock);
        tmp = dt->dtp;
        spin_unlock(&dt->lock);

        return tmp;
}

struct dtcp * dt_dtcp(struct dt * dt)
{
        struct dtcp * tmp;

        if (!dt)
                return NULL;

        spin_lock(&dt->lock);
        tmp = dt->dtcp;
        spin_unlock(&dt->lock);

        return tmp;
}

struct cwq * dt_cwq(struct dt * dt)
{
        struct cwq * tmp;

        if (!dt)
                return NULL;

        spin_lock(&dt->lock);
        tmp = dt->cwq;
        spin_unlock(&dt->lock);

        return tmp;
}

struct rtxq * dt_rtxq(struct dt * dt)
{
        struct rtxq * tmp;

        if (!dt)
                return NULL;

        spin_lock(&dt->lock);
        tmp = dt->rtxq;
        spin_unlock(&dt->lock);

        return tmp;
}

/* DTP API for DTCP */
int dt_dtp_rcv_flow_ctl(struct dt * dt)
{
        LOG_MISSING;

        /* FIXME: should call dtp_rcv_flow_ctl */

        return 0;
}

uint_t dt_sv_max_pdu_size(struct dt * dt)
{
        uint_t tmp;

        if (!dt || !dt->sv)
                return 0;

        spin_lock(&dt->lock);
        tmp = dt->sv->max_flow_pdu_size;
        spin_unlock(&dt->lock);

        return tmp;
}

uint_t dt_sv_max_sdu_size(struct dt * dt)
{
        uint_t tmp;

        if (!dt || !dt->sv)
                return 0;

        spin_lock(&dt->lock);
        tmp = dt->sv->max_flow_sdu_size;
        spin_unlock(&dt->lock);

        return tmp;
}

timeout_t dt_sv_mpl(struct dt * dt)
{
        uint_t tmp;

        if (!dt || !dt->sv)
                return 0;

        spin_lock(&dt->lock);
        tmp = dt->sv->MPL;
        spin_unlock(&dt->lock);

        return tmp;
}

timeout_t dt_sv_r(struct dt * dt)
{
        uint_t tmp;

        if (!dt || !dt->sv)
                return 0;

        spin_lock(&dt->lock);
        tmp = dt->sv->R;
        spin_unlock(&dt->lock);

        return tmp;
}

timeout_t dt_sv_a(struct dt * dt)
{
        uint_t tmp;

        if (!dt || !dt->sv)
                return 0;

        spin_lock(&dt->lock);
        tmp = dt->sv->A;
        spin_unlock(&dt->lock);

        return tmp;
}

seq_num_t dt_sv_rcv_lft_win(struct dt * dt)
{
        seq_num_t tmp;

        if (!dt || !dt->sv)
                return 0;

        spin_lock(&dt->lock);
        tmp = dt->sv->rcv_left_window_edge;
        spin_unlock(&dt->lock);

        return tmp;
}

int dt_sv_rcv_lft_win_set(struct dt * dt, seq_num_t rcv_lft_win)
{
    if (!dt || !dt->sv)
            return -1;

    spin_lock(&dt->lock);
    dt->sv->rcv_left_window_edge = rcv_lft_win;
    spin_unlock(&dt->lock);

    return 0;
}
