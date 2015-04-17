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

struct dt_sv {
        uint_t       max_flow_pdu_size;
        uint_t       max_flow_sdu_size;
        timeout_t    MPL;
        timeout_t    R;
        timeout_t    A;
        timeout_t    tr;
        seq_num_t    rcv_left_window_edge;
        bool         window_closed;
        bool         drf_flag;
};

struct dt {
        struct dt_sv *      sv;
        struct dtp *        dtp;
        struct dtcp *       dtcp;
        struct efcp *       efcp;

        struct cwq *        cwq;
        struct rtxq *       rtxq;

        spinlock_t          lock;
};

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
                dt->rtxq = NULL; /* Useful */
        }

        rkfree(dt->sv);
        rkfree(dt);

        LOG_DBG("Instance %pK destroyed successfully", dt);

        return 0;
}

int dt_dtp_bind(struct dt * dt, struct dtp * dtp)
{
        unsigned long flags;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot bind DTP");
                return -1;
        }
        if (!dtp) {
                LOG_ERR("Cannot bind NULL DTP to instance %pK", dt);
                return -1;
        }

        spin_lock_irqsave(&dt->lock, flags);
        if (dt->dtp) {
                spin_unlock_irqrestore(&dt->lock, flags);

                LOG_ERR("A DTP instance is already bound to instance %pK, "
                        "unbind it first", dt);
                return -1;
        }
        dt->dtp = dtp;
        spin_unlock_irqrestore(&dt->lock, flags);

        return 0;
}

struct dtp * dt_dtp_unbind(struct dt * dt)
{
        struct dtp *  tmp;
        unsigned long flags;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot unbind DTP");
                return NULL;
        }

        spin_lock_irqsave(&dt->lock, flags);
        if (!dt->dtp) {
                spin_unlock_irqrestore(&dt->lock, flags);

                LOG_DBG("No DTP bound to instance %pK", dt);
                return NULL;
        }

        tmp     = dt->dtp;
        dt->dtp = NULL;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}

int dt_dtcp_bind(struct dt * dt, struct dtcp * dtcp)
{
        unsigned long flags;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot bind DTCP");
                return -1;
        }
        if (!dtcp) {
                LOG_ERR("Cannot bind NULL DTCP to instance %pK", dt);
                return -1;
        }

        spin_lock_irqsave(&dt->lock, flags);
        if (dt->dtcp) {
                spin_unlock_irqrestore(&dt->lock, flags);

                LOG_ERR("A DTCP instance already bound to instance %pK, "
                        "unbind it first", dt);
                return -1;
        }

        dt->dtcp = dtcp;
        spin_unlock_irqrestore(&dt->lock, flags);

        return 0;
}

struct dtcp * dt_dtcp_unbind(struct dt * dt)
{
        struct dtcp * tmp;
        unsigned long flags;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot unbind DTCP");
                return NULL;
        }

        spin_lock_irqsave(&dt->lock, flags);
        if (!dt->dtcp) {
                spin_unlock_irqrestore(&dt->lock, flags);

                LOG_DBG("No DTCP bound to instance %pK", dt);
                return NULL;
        }

        tmp      = dt->dtcp;
        dt->dtcp = NULL;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}

int dt_cwq_bind(struct dt * dt, struct cwq * cwq)
{
        unsigned long flags;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot bind CWQ");
                return -1;
        }

        if (!cwq) {
                LOG_ERR("Cannot bind NULL CWQ to instance %pK", dt);
                return -1;
        }

        spin_lock_irqsave(&dt->lock, flags);
        if (dt->cwq) {
                spin_unlock_irqrestore(&dt->lock, flags);

                LOG_ERR("A CWQ already bound to instance %pK", dt);
                return -1;
        }
        dt->cwq = cwq;
        spin_unlock_irqrestore(&dt->lock, flags);

        return 0;
}

struct cwq * dt_cwq_unbind(struct dt * dt)
{
        struct cwq *  tmp;
        unsigned long flags;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot unbind CWQ");
                return NULL;
        }

        spin_lock_irqsave(&dt->lock, flags);
        if (!dt->cwq) {
                spin_unlock_irqrestore(&dt->lock, flags);

                LOG_DBG("No CWQ bound to instance %pK", dt);
                return NULL;
        }

        tmp     = dt->cwq;
        dt->cwq = NULL;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}

struct rtxq * dt_rtxq_unbind(struct dt * dt)
{
        struct rtxq * tmp;
        unsigned long flags;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot unbind RTXQ");
                return NULL;
        }

        spin_lock_irqsave(&dt->lock, flags);
        if (!dt->rtxq) {
                spin_unlock_irqrestore(&dt->lock, flags);

                LOG_DBG("No RTXQ bound to instance %pK", dt);
                return NULL;
        }

        tmp      = dt->rtxq;
        dt->rtxq = NULL;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}

int dt_rtxq_bind(struct dt * dt, struct rtxq * rtxq)
{
        unsigned long flags;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot bind RTXQ");
                return -1;
        }

        if (!rtxq) {
                LOG_ERR("Cannot bind NULL RTXQ to instance %pK", dt);
                return -1;
        }

        spin_lock_irqsave(&dt->lock, flags);
        if (dt->rtxq) {
                spin_unlock_irqrestore(&dt->lock, flags);

                LOG_ERR("A RTXQ already bound to instance %pK", dt);
                return -1;
        }
        dt->rtxq = rtxq;
        spin_unlock_irqrestore(&dt->lock, flags);

        return 0;
}

struct efcp * dt_efcp_unbind(struct dt * dt)
{
        struct efcp * tmp;
        unsigned long flags;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot unbind EFCP");
                return NULL;
        }

        spin_lock_irqsave(&dt->lock, flags);
        if (!dt->efcp) {
                spin_unlock_irqrestore(&dt->lock, flags);

                LOG_ERR("No EFCP bound to instance %pK", dt);
                return NULL;
        }
        tmp = dt->efcp;
        dt->efcp = NULL;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}

int dt_efcp_bind(struct dt * dt, struct efcp * efcp)
{
        unsigned long flags;

        if (!dt) {
                LOG_ERR("Bogus instance passed, cannot bind EFCP");
                return -1;
        }

        if (!efcp) {
                LOG_ERR("Cannot bind NULL EFCP to instance %pK", dt);
                return -1;
        }

        spin_lock_irqsave(&dt->lock, flags);
        if (dt->efcp) {
                spin_unlock_irqrestore(&dt->lock, flags);

                LOG_ERR("A EFCP already bound to instance %pK", dt);
                return -1;
        }
        dt->efcp = efcp;
        spin_unlock_irqrestore(&dt->lock, flags);

        return 0;
}

struct dtp * dt_dtp(struct dt * dt)
{
        unsigned long flags;

        struct dtp * tmp;

        if (!dt)
                return NULL;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->dtp;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dt_dtp);

struct dtcp * dt_dtcp(struct dt * dt)
{
        struct dtcp * tmp;
        unsigned long flags;

        if (!dt)
                return NULL;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->dtcp;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dt_dtcp);

struct cwq * dt_cwq(struct dt * dt)
{
        struct cwq *  tmp;
        unsigned long flags;

        if (!dt)
                return NULL;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->cwq;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dt_cwq);

struct rtxq * dt_rtxq(struct dt * dt)
{
        struct rtxq * tmp;
        unsigned long flags;

        if (!dt)
                return NULL;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->rtxq;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dt_rtxq);

struct efcp * dt_efcp(struct dt * dt)
{
        struct efcp * tmp;
        unsigned long flags;

        if (!dt)
                return NULL;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->efcp;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dt_efcp);

uint_t dt_sv_max_pdu_size(struct dt * dt)
{
        uint_t        tmp;
        unsigned long flags;

        if (!dt || !dt->sv)
                return 0;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->sv->max_flow_pdu_size;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}

uint_t dt_sv_max_sdu_size(struct dt * dt)
{
        uint_t        tmp;
        unsigned long flags;

        if (!dt || !dt->sv)
                return 0;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->sv->max_flow_sdu_size;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}

timeout_t dt_sv_mpl(struct dt * dt)
{
        uint_t        tmp;
        unsigned long flags;

        if (!dt || !dt->sv)
                return 0;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->sv->MPL;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dt_sv_mpl);

timeout_t dt_sv_r(struct dt * dt)
{
        uint_t        tmp;
        unsigned long flags;

        if (!dt || !dt->sv)
                return 0;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->sv->R;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dt_sv_r);

timeout_t dt_sv_a(struct dt * dt)
{
        uint_t        tmp;
        unsigned long flags;

        if (!dt || !dt->sv)
                return 0;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->sv->A;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dt_sv_a);

seq_num_t dt_sv_rcv_lft_win(struct dt * dt)
{
        seq_num_t     tmp;
        unsigned long flags;

        if (!dt || !dt->sv)
                return 0;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->sv->rcv_left_window_edge;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dt_sv_rcv_lft_win);

int dt_sv_rcv_lft_win_set(struct dt * dt, seq_num_t rcv_lft_win)
{
        unsigned long flags;

        if (!dt || !dt->sv)
                return -1;

        spin_lock_irqsave(&dt->lock, flags);
        dt->sv->rcv_left_window_edge = rcv_lft_win;
        spin_unlock_irqrestore(&dt->lock, flags);

        return 0;
}
EXPORT_SYMBOL(dt_sv_rcv_lft_win_set);

bool dt_sv_window_closed(struct dt * dt)
{
        bool          tmp;
        unsigned long flags;

        if (!dt || !dt->sv)
                return false;

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->sv->window_closed;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}

int dt_sv_window_closed_set(struct dt * dt, bool closed)
{
        unsigned long flags;

        if (!dt || !dt->sv)
                return -1;

        spin_lock_irqsave(&dt->lock, flags);
        dt->sv->window_closed = closed;
        spin_unlock_irqrestore(&dt->lock, flags);

        return 0;
}
EXPORT_SYMBOL(dt_sv_window_closed_set);

timeout_t dt_sv_tr(struct dt * dt)
{
        unsigned long flags;
        timeout_t     tmp;

        ASSERT(dt);
        ASSERT(dt->sv);

        spin_lock_irqsave(&dt->lock, flags);
        tmp = dt->sv->tr;
        spin_unlock_irqrestore(&dt->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(dt_sv_tr);

bool dt_sv_drf_flag(struct dt * dt)
{
        bool          flag;
        unsigned long flags;

        if (!dt || !dt->sv)
                return false;

        spin_lock_irqsave(&dt->lock, flags);
        flag = dt->sv->drf_flag;
        spin_unlock_irqrestore(&dt->lock, flags);

        return flag;
}

void dt_sv_drf_flag_set(struct dt * dt, bool value)
{
        unsigned long flags;
        if (!dt || !dt->sv)
                return;

        spin_lock_irqsave(&dt->lock, flags);
        dt->sv->drf_flag = value;
        spin_unlock_irqrestore(&dt->lock, flags);

        return;
}
EXPORT_SYMBOL(dt_sv_drf_flag_set);

