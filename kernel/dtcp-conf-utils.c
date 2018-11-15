/*
 * DTCP CONFIG Utils (Data Transfer Control Protocol)
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

#include <linux/kernel.h>

#define RINA_PREFIX "dtcp-conf-utils"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtcp-conf-utils.h"
#include "policies.h"

/* FIXME: These externs have to disappear from here */
extern struct policy *    policy_create_gfp(gfp_t flags);

static int window_fctrl_config_destroy(struct window_fctrl_config * cfg)
{
        if (!cfg)
                return -1;
        if (cfg->rcvr_flow_control)
                policy_destroy(cfg->rcvr_flow_control);
        if (cfg->tx_control)
                policy_destroy(cfg->tx_control);

        rkfree(cfg);

        return 0;
}

static struct window_fctrl_config * window_fctrl_config_create_gfp(gfp_t flags)
{
        struct window_fctrl_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->rcvr_flow_control = policy_create_gfp(flags);
        tmp->tx_control        = policy_create_gfp(flags);

        if (!tmp->rcvr_flow_control || !tmp->tx_control) {
                window_fctrl_config_destroy(tmp);
                return NULL;
        }

        return tmp;
}

struct window_fctrl_config * dtcp_window_fctrl_config_create(void)
{ return window_fctrl_config_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(dtcp_window_fctrl_config_create);

struct window_fctrl_config * dtcp_window_fctrl_config_create_ni(void)
{ return window_fctrl_config_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(dtcp_window_fctrl_config_create_ni);

static int rate_fctrl_config_destroy(struct rate_fctrl_config * cfg)
{
        if (!cfg)
                return -1;
        if (cfg->no_rate_slow_down)
                policy_destroy(cfg->no_rate_slow_down);
        if (cfg->no_override_default_peak)
                policy_destroy(cfg->no_override_default_peak);
        if (cfg->rate_reduction)
                policy_destroy(cfg->rate_reduction);

        rkfree(cfg);

        return 0;
}

static struct rate_fctrl_config * rate_fctrl_config_create_gfp(gfp_t flags)
{
        struct rate_fctrl_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->no_rate_slow_down        = policy_create_gfp(flags);
        tmp->no_override_default_peak = policy_create_gfp(flags);
        tmp->rate_reduction           = policy_create_gfp(flags);

        if (!tmp->no_rate_slow_down        ||
            !tmp->no_override_default_peak ||
            !tmp->rate_reduction) {
                rate_fctrl_config_destroy(tmp);
                return NULL;
        }

        return tmp;
}

struct rate_fctrl_config * dtcp_rate_fctrl_config_create(void)
{ return rate_fctrl_config_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(dtcp_rate_fctrl_config_create);

struct rate_fctrl_config * dtcp_rate_fctrl_config_create_ni(void)
{ return rate_fctrl_config_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(dtcp_rate_fctrl_config_create_ni);

static int dtcp_fctrl_config_destroy(struct dtcp_fctrl_config * cfg)
{
        if (!cfg)
                return -1;

        if (cfg->wfctrl_cfg) window_fctrl_config_destroy(cfg->wfctrl_cfg);
        if (cfg->rfctrl_cfg) rate_fctrl_config_destroy(cfg->rfctrl_cfg);

        if (cfg->closed_window)
                policy_destroy(cfg->closed_window);
        if (cfg->flow_control_overrun)
                policy_destroy(cfg->flow_control_overrun);
        if (cfg->reconcile_flow_conflict)
                policy_destroy(cfg->reconcile_flow_conflict);
        if (cfg->receiving_flow_control)
                policy_destroy(cfg->receiving_flow_control);

        rkfree(cfg);

        return 0;
}

static struct dtcp_fctrl_config * dtcp_fctrl_config_create_gfp(gfp_t flags)
{
        struct dtcp_fctrl_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->wfctrl_cfg = window_fctrl_config_create_gfp(flags);
        if (!tmp->wfctrl_cfg) {
                LOG_ERR("Could not create wfctrl_cfg");
                goto clean;
        }

        tmp->rfctrl_cfg = rate_fctrl_config_create_gfp(flags);
        if (!tmp->rfctrl_cfg) {
                LOG_ERR("Could not create rfctrl_cfg");
                goto clean;
        }

        tmp->closed_window            = policy_create_gfp(flags);
        /*tmp->flow_control_overrun     = policy_create_gfp(flags);*/
        tmp->reconcile_flow_conflict  = policy_create_gfp(flags);
        tmp->receiving_flow_control   = policy_create_gfp(flags);
        if (!tmp->closed_window              ||
            /*!tmp->flow_control_overrun       ||*/
            !tmp->reconcile_flow_conflict    ||
            !tmp->receiving_flow_control) {
                LOG_ERR("Could not create a policy in "
                        "dtcp_fctrl_config_create");
                goto clean;
        }

        return tmp;

 clean:

        dtcp_fctrl_config_destroy(tmp);
        return NULL;
}

struct dtcp_fctrl_config * dtcp_fctrl_config_create_ni(void)
{ return dtcp_fctrl_config_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(dtcp_fctrl_config_create_ni);

static int dtcp_rxctrl_config_destroy(struct dtcp_rxctrl_config * cfg)
{
        if (!cfg)
                return -1;
        if (cfg->retransmission_timer_expiry)
                policy_destroy(cfg->retransmission_timer_expiry);
        if (cfg->sender_ack)
                policy_destroy(cfg->sender_ack);
        if (cfg->receiving_ack_list)
                policy_destroy(cfg->receiving_ack_list);
        if (cfg->rcvr_ack)
                policy_destroy(cfg->rcvr_ack);
        if (cfg->sending_ack)
                policy_destroy(cfg->sending_ack);
        if (cfg->rcvr_control_ack)
                policy_destroy(cfg->rcvr_control_ack);

        rkfree(cfg);

        return 0;
}

static struct dtcp_rxctrl_config * dtcp_rxctrl_config_create_gfp(gfp_t flags)
{
        struct dtcp_rxctrl_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->retransmission_timer_expiry = policy_create_gfp(flags);
        tmp->sender_ack                  = policy_create_gfp(flags);
        tmp->receiving_ack_list          = policy_create_gfp(flags);
        tmp->rcvr_ack                    = policy_create_gfp(flags);
        tmp->sending_ack                 = policy_create_gfp(flags);
        tmp->rcvr_control_ack            = policy_create_gfp(flags);

        if (!tmp->retransmission_timer_expiry ||
            !tmp->sender_ack                  ||
            !tmp->receiving_ack_list          ||
            !tmp->rcvr_ack                    ||
            !tmp->sending_ack                 ||
            !tmp->rcvr_control_ack) {
                LOG_ERR("Could not create policy");
                dtcp_rxctrl_config_destroy(tmp);
                return NULL;
        }

        return tmp;
}

struct dtcp_rxctrl_config * dtcp_rxctrl_config_create_ni(void)
{ return dtcp_rxctrl_config_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(dtcp_rxctrl_config_create_ni);

int dtcp_config_destroy(struct dtcp_config * cfg)
{
        if (!cfg)
                return -1;
        if (cfg->fctrl_cfg)
                dtcp_fctrl_config_destroy(cfg->fctrl_cfg);
        if (cfg->rxctrl_cfg)
                dtcp_rxctrl_config_destroy(cfg->rxctrl_cfg);
        if (cfg->dtcp_ps)
                policy_destroy(cfg->dtcp_ps);
        if (cfg->lost_control_pdu)
                policy_destroy(cfg->lost_control_pdu);
        if (cfg->rtt_estimator)
                policy_destroy(cfg->rtt_estimator);

        rkfree(cfg);

        return 0;
}
EXPORT_SYMBOL(dtcp_config_destroy);

static struct dtcp_config * dtcp_config_create_gfp(gfp_t flags)
{
        struct dtcp_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;
        /*
          tmp->fctrl_cfg = dtcp_fctrl_config_create_gfp(flags);
          if (!tmp->fctrl_cfg) {
          LOG_ERR("Could not create fctrl_cfg in dtcp_config_create");
          goto clean;
          }

          tmp->rxctrl_cfg = dtcp_rxctrl_config_create_gfp(flags);
          if (!tmp->rxctrl_cfg) {
          LOG_ERR("Could not create rxctrl_cfg in dtcp_config_create");
          goto clean;
          }
        */

        tmp->dtcp_ps = policy_create();
        if (!tmp->dtcp_ps) {
                LOG_ERR("Could not create dtp_ps"
                        "in dtcp_config_create");
                goto clean;
        }


        tmp->lost_control_pdu = policy_create_gfp(flags);
        if (!tmp->lost_control_pdu) {
                LOG_ERR("Could not create lost_control_pdu"
                        "in dtcp_config_create");
                goto clean;
        }

        tmp->rtt_estimator = policy_create_gfp(flags);
        if (!tmp->rtt_estimator) {
                LOG_ERR("Could not create rtt_estimator"
                        "in dtcp_config_create");
                goto clean;
        }

        return tmp;

 clean:

        dtcp_config_destroy(tmp);
        return NULL;
}

struct dtcp_config * dtcp_config_create_ni(void)
{ return dtcp_config_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(dtcp_config_create_ni);

/* Setters */
/* FIXME: check better the inputs */
/* window_fctrl_config */
int dtcp_max_closed_winq_length_set(struct dtcp_config * cfg, uint_t len)
{
        if (!cfg)
                return -1;

        cfg->fctrl_cfg->wfctrl_cfg->max_closed_winq_length = len;

        return 0;
}
EXPORT_SYMBOL(dtcp_max_closed_winq_length_set);

int dtcp_initial_credit_set(struct dtcp_config * cfg, uint_t ic)
{
        if (!cfg)
                return -1;

        cfg->fctrl_cfg->wfctrl_cfg->initial_credit = ic;

        return 0;
}
EXPORT_SYMBOL(dtcp_initial_credit_set);

int dtcp_rcvr_flow_control_set(struct dtcp_config * cfg,
                               struct policy * rcvr_flow_control)
{
        if (!cfg) return -1;
        if (!rcvr_flow_control) return -1;

        cfg->fctrl_cfg->wfctrl_cfg->rcvr_flow_control = rcvr_flow_control;

        return 0;
}
EXPORT_SYMBOL(dtcp_rcvr_flow_control_set);

int dtcp_tx_control_set(struct dtcp_config * cfg,
                        struct policy * tx_control)
{
        if (!cfg) return -1;
        if (!tx_control) return -1;

        cfg->fctrl_cfg->wfctrl_cfg->tx_control = tx_control;

        return 0;
}
EXPORT_SYMBOL(dtcp_tx_control_set);

/* rate_fctrl_cfg */
int dtcp_sending_rate_set(struct dtcp_config * cfg, uint_t sending_rate)
{
        if (!cfg)
                return -1;

        cfg->fctrl_cfg->rfctrl_cfg->sending_rate = sending_rate;

        return 0;
}
EXPORT_SYMBOL(dtcp_sending_rate_set);

int dtcp_time_period_set(struct dtcp_config * cfg, uint_t time_period)
{
        if (!cfg)
                return -1;

        cfg->fctrl_cfg->rfctrl_cfg->time_period = time_period;

        return 0;
}
EXPORT_SYMBOL(dtcp_time_period_set);

int dtcp_no_rate_slow_down_set(struct dtcp_config * cfg,
                               struct policy * no_rate_slow_down)
{
        if (!cfg) return -1;
        if (!no_rate_slow_down) return -1;

        cfg->fctrl_cfg->rfctrl_cfg->no_rate_slow_down = no_rate_slow_down;

        return 0;
}
EXPORT_SYMBOL(dtcp_no_rate_slow_down_set);

int dtcp_no_override_default_peak_set(struct dtcp_config * cfg,
                                      struct policy * no_override_def_peak)
{
        if (!cfg) return -1;
        if (!no_override_def_peak) return -1;

        cfg->fctrl_cfg->rfctrl_cfg->no_override_default_peak =
                no_override_def_peak;

        return 0;
}
EXPORT_SYMBOL(dtcp_no_override_default_peak_set);

int dtcp_rate_reduction_set(struct dtcp_config * cfg,
                            struct policy * rate_reduction)
{
        if (!cfg) return -1;
        if (!rate_reduction) return -1;

        cfg->fctrl_cfg->rfctrl_cfg->rate_reduction = rate_reduction;

        return 0;
}
EXPORT_SYMBOL(dtcp_rate_reduction_set);

/* fctrl_cfg */
int dtcp_window_based_fctrl_set(struct dtcp_config * cfg, bool wb_fctrl)
{
        if (!cfg)
                return -1;

        if (!cfg->fctrl_cfg)
                return -1;

        cfg->fctrl_cfg->window_based_fctrl = wb_fctrl;

        return 0;
}
EXPORT_SYMBOL(dtcp_window_based_fctrl_set);

int dtcp_wfctrl_cfg_set(struct dtcp_config * cfg,
                        struct window_fctrl_config * wfctrl_cfg)
{
        if (!cfg) return -1;
        if (!wfctrl_cfg) return -1;

        cfg->fctrl_cfg->wfctrl_cfg = wfctrl_cfg;

        return 0;
}
EXPORT_SYMBOL(dtcp_wfctrl_cfg_set);

int dtcp_rate_based_fctrl_set(struct dtcp_config * cfg,
                              bool rate_based_fctrl)
{
        if (!cfg)
                return -1;
        if (!cfg->fctrl_cfg)
                return -1;

        cfg->fctrl_cfg->rate_based_fctrl = rate_based_fctrl;

        return 0;
}
EXPORT_SYMBOL(dtcp_rate_based_fctrl_set);

int dtcp_rfctrl_cfg_set(struct dtcp_config * cfg,
                        struct rate_fctrl_config * rfctrl_cfg)
{
        if (!cfg) return -1;
        if (!rfctrl_cfg) return -1;

        cfg->fctrl_cfg->rfctrl_cfg = rfctrl_cfg;

        return 0;
}
EXPORT_SYMBOL(dtcp_rfctrl_cfg_set);

int dtcp_sent_bytes_th_set(struct dtcp_config * cfg, uint_t sent_bytes_th)
{
        if (!cfg)            return -1;
        if (!cfg->fctrl_cfg) return -1;

        cfg->fctrl_cfg->sent_bytes_th = sent_bytes_th;

        return 0;
}
EXPORT_SYMBOL(dtcp_sent_bytes_th_set);

int dtcp_sent_bytes_percent_th_set(struct dtcp_config * cfg,
                                   uint_t sent_bytes_percent_th)
{
        if (!cfg)            return -1;
        if (!cfg->fctrl_cfg) return -1;

        cfg->fctrl_cfg->sent_bytes_percent_th = sent_bytes_percent_th;

        return 0;
}
EXPORT_SYMBOL(dtcp_sent_bytes_percent_th_set);

int dtcp_sent_buffers_th_set(struct dtcp_config * cfg, uint_t sent_buffers_th)
{
        if (!cfg)            return -1;
        if (!cfg->fctrl_cfg) return -1;

        cfg->fctrl_cfg->sent_buffers_th = sent_buffers_th;

        return 0;
}
EXPORT_SYMBOL(dtcp_sent_buffers_th_set);

int dtcp_rcvd_bytes_th_set(struct dtcp_config * cfg, uint_t rcvd_bytes_th)
{
        if (!cfg)            return -1;
        if (!cfg->fctrl_cfg) return -1;

        cfg->fctrl_cfg->rcvd_bytes_th = rcvd_bytes_th;

        return 0;
}
EXPORT_SYMBOL(dtcp_rcvd_bytes_th_set);

int dtcp_rcvd_bytes_percent_th_set(struct dtcp_config * cfg,
                                   uint_t rcvd_byte_pth)
{
        if (!cfg)            return -1;
        if (!cfg->fctrl_cfg) return -1;

        cfg->fctrl_cfg->rcvd_bytes_percent_th = rcvd_byte_pth;

        return 0;
}
EXPORT_SYMBOL(dtcp_rcvd_bytes_percent_th_set);

int dtcp_rcvd_buffers_th_set(struct dtcp_config * cfg,
                             uint_t rcvd_buffers_th)
{
        if (!cfg)            return -1;
        if (!cfg->fctrl_cfg) return -1;

        cfg->fctrl_cfg->rcvd_buffers_th = rcvd_buffers_th;

        return 0;
}
EXPORT_SYMBOL(dtcp_rcvd_buffers_th_set);

int dtcp_closed_window_set(struct dtcp_config * cfg,
                           struct policy * closed_window)
{
        if (!cfg)            return -1;
        if (!cfg->fctrl_cfg) return -1;
        if (!closed_window)  return -1;

        cfg->fctrl_cfg->closed_window = closed_window;

        return 0;
}
EXPORT_SYMBOL(dtcp_closed_window_set);

/*
int dtcp_flow_control_overrun_set(struct dtcp_config * cfg,
                                  struct policy * flow_control_overrun)
{
        if (!cfg)                  return -1;
        if (!cfg->fctrl_cfg)       return -1;
        if (!flow_control_overrun) return -1;

        cfg->fctrl_cfg->flow_control_overrun = flow_control_overrun;

        return 0;
}
EXPORT_SYMBOL(dtcp_flow_control_overrun_set);
*/

int dtcp_reconcile_flow_conflict_set(struct dtcp_config * cfg,
                                     struct policy * reconcile_flow_conflict)
{
        if (!cfg)                     return -1;
        if (!cfg->fctrl_cfg)          return -1;
        if (!reconcile_flow_conflict) return -1;

        cfg->fctrl_cfg->reconcile_flow_conflict = reconcile_flow_conflict;

        return 0;
}
EXPORT_SYMBOL(dtcp_reconcile_flow_conflict_set);

int dtcp_receiving_flow_control_set(struct dtcp_config * cfg,
                                    struct policy * receiving_flow_control)
{
        if (!cfg)                    return -1;
        if (!cfg->fctrl_cfg)         return -1;
        if (!receiving_flow_control) return -1;

        cfg->fctrl_cfg->receiving_flow_control =
                receiving_flow_control;

        return 0;
}
EXPORT_SYMBOL(dtcp_receiving_flow_control_set);

/* dtcp_rxctrl_config */
int dtcp_max_time_retry_set(struct dtcp_config * cfg,
                            uint_t max_time_retry)
{
        if (!cfg)
                return -1;

        cfg->rxctrl_cfg->max_time_retry = max_time_retry;

        return 0;
}
EXPORT_SYMBOL(dtcp_max_time_retry_set);

int dtcp_data_retransmit_max_set(struct dtcp_config * cfg,
                                 uint_t data_retransmit_max)
{
        if (!cfg)
                return -1;

        cfg->rxctrl_cfg->data_retransmit_max = data_retransmit_max;

        return 0;
}
EXPORT_SYMBOL(dtcp_data_retransmit_max_set);

int dtcp_initial_tr_set(struct dtcp_config * cfg,
                        uint_t               tr)
{
        if (!cfg)
                return -1;

        cfg->rxctrl_cfg->initial_tr = tr;

        return 0;
}
EXPORT_SYMBOL(dtcp_initial_tr_set);

int dtcp_retransmission_timer_expiry_set(struct dtcp_config * cfg,
                                         struct policy * rtx_timer_expiry)
{
        if (!cfg) return -1;
        if (!rtx_timer_expiry) return -1;

        cfg->rxctrl_cfg->retransmission_timer_expiry = rtx_timer_expiry;

        return 0;
}
EXPORT_SYMBOL(dtcp_retransmission_timer_expiry_set);

int dtcp_sender_ack_set(struct dtcp_config * cfg,
                        struct policy * sender_ack)
{
        if (!cfg) return -1;
        if (!sender_ack) return -1;

        cfg->rxctrl_cfg->sender_ack = sender_ack;

        return 0;
}
EXPORT_SYMBOL(dtcp_sender_ack_set);

int dtcp_receiving_ack_list_set(struct dtcp_config * cfg,
                                struct policy * receiving_ack_list)
{
        if (!cfg) return -1;
        if (!receiving_ack_list) return -1;

        cfg->rxctrl_cfg->receiving_ack_list = receiving_ack_list;

        return 0;
}
EXPORT_SYMBOL(dtcp_receiving_ack_list_set);

int dtcp_rcvr_ack_set(struct dtcp_config * cfg, struct policy * rcvr_ack)
{
        if (!cfg) return -1;
        if (!rcvr_ack) return -1;

        cfg->rxctrl_cfg->rcvr_ack = rcvr_ack;

        return 0;
}
EXPORT_SYMBOL(dtcp_rcvr_ack_set);

int dtcp_sending_ack_set(struct dtcp_config * cfg, struct policy * sending_ack)
{
        if (!cfg) return -1;
        if (!sending_ack) return -1;

        cfg->rxctrl_cfg->sending_ack = sending_ack;

        return 0;
}
EXPORT_SYMBOL(dtcp_sending_ack_set);

int dtcp_rcvr_control_ack_set(struct dtcp_config * cfg,
                              struct policy * rcvr_control_ack)
{
        if (!cfg) return -1;
        if (!rcvr_control_ack) return -1;

        cfg->rxctrl_cfg->rcvr_control_ack = rcvr_control_ack;

        return 0;
}
EXPORT_SYMBOL(dtcp_rcvr_control_ack_set);

/* dtcp_config */
int dtcp_flow_ctrl_set(struct dtcp_config * cfg, bool flow_ctrl)
{
        if (!cfg)
                return -1;

        cfg->flow_ctrl = flow_ctrl;

        return 0;
}
EXPORT_SYMBOL(dtcp_flow_ctrl_set);

int dtcp_rtx_ctrl_set(struct dtcp_config * cfg, bool rtx_ctrl)
{
        if (!cfg)
                return -1;

        cfg->rtx_ctrl = rtx_ctrl;

        return 0;
}
EXPORT_SYMBOL(dtcp_rtx_ctrl_set);

int dtcp_fctrl_cfg_set(struct dtcp_config * cfg,
                       struct dtcp_fctrl_config * fctrl_cfg)
{
        if (!cfg) return -1;
        if (!fctrl_cfg) return -1;

        cfg->fctrl_cfg = fctrl_cfg;

        return 0;
}
EXPORT_SYMBOL(dtcp_fctrl_cfg_set);

int dtcp_rxctrl_cfg_set(struct dtcp_config * cfg,
                        struct dtcp_rxctrl_config * rxctrl_cfg)
{
        if (!cfg) return -1;
        if (!rxctrl_cfg) return -1;

        cfg->rxctrl_cfg = rxctrl_cfg;

        return 0;
}
EXPORT_SYMBOL(dtcp_rxctrl_cfg_set);

int dtcp_lost_control_pdu_set(struct dtcp_config * cfg,
                              struct policy * lost_control_pdu)
{
        if (!cfg) return -1;
        if (!lost_control_pdu) return -1;

        cfg->lost_control_pdu = lost_control_pdu;

        return 0;
}
EXPORT_SYMBOL(dtcp_lost_control_pdu_set);

int dtcp_ps_set(struct dtcp_config * cfg,
                struct policy * dtcp_ps)
{
        if (!cfg) return -1;
        if (!dtcp_ps) return -1;

        cfg->dtcp_ps = dtcp_ps;

        return 0;
}
EXPORT_SYMBOL(dtcp_ps_set);


int dtcp_rtt_estimator_set(struct dtcp_config * cfg,
                           struct policy * rtt_estimator)
{
        if (!cfg) return -1;
        if (!rtt_estimator) return -1;

        cfg->rtt_estimator = rtt_estimator;

        return 0;
}
EXPORT_SYMBOL(dtcp_rtt_estimator_set);

/* Getters */
/* window_fctrl_config */
uint_t dtcp_max_closed_winq_length(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg || !cfg->fctrl_cfg->wfctrl_cfg)
                return 0;

        return cfg->fctrl_cfg->wfctrl_cfg->max_closed_winq_length;
}
EXPORT_SYMBOL(dtcp_max_closed_winq_length);

uint_t dtcp_initial_credit(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg || !cfg->fctrl_cfg->wfctrl_cfg)
                return UINT_MAX;

        return cfg->fctrl_cfg->wfctrl_cfg->initial_credit;
}
EXPORT_SYMBOL(dtcp_initial_credit);

struct policy * dtcp_rcvr_flow_control(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg || !cfg->fctrl_cfg->wfctrl_cfg)
                return NULL;

        return cfg->fctrl_cfg->wfctrl_cfg->rcvr_flow_control;
}
EXPORT_SYMBOL(dtcp_rcvr_flow_control);

struct policy * dtcp_tx_control(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg || !cfg->fctrl_cfg->wfctrl_cfg)
                return NULL;

        return cfg->fctrl_cfg->wfctrl_cfg->tx_control;
}
EXPORT_SYMBOL(dtcp_tx_control);

/* rate_fctrl_cfg */
uint_t dtcp_sending_rate (struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg || !cfg->fctrl_cfg->rfctrl_cfg)
                return 0;

        return cfg->fctrl_cfg->rfctrl_cfg->sending_rate;
}
EXPORT_SYMBOL(dtcp_sending_rate);

uint_t dtcp_time_period(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg || !cfg->fctrl_cfg->rfctrl_cfg)
                return 0;

        return cfg->fctrl_cfg->rfctrl_cfg->time_period;
}
EXPORT_SYMBOL(dtcp_time_period);

struct policy * dtcp_no_rate_slow_down(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg || !cfg->fctrl_cfg->rfctrl_cfg)
                return NULL;

        return cfg->fctrl_cfg->rfctrl_cfg->no_rate_slow_down;
}
EXPORT_SYMBOL(dtcp_no_rate_slow_down);

struct policy * dtcp_no_override_default_peak(struct dtcp_config * cfg)
{
        ASSERT(cfg);
        return cfg->fctrl_cfg->rfctrl_cfg->no_override_default_peak;
}
EXPORT_SYMBOL(dtcp_no_override_default_peak);

struct policy * dtcp_rate_reduction(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg || !cfg->fctrl_cfg->rfctrl_cfg)
                return NULL;

        return cfg->fctrl_cfg->rfctrl_cfg->rate_reduction;
}
EXPORT_SYMBOL(dtcp_rate_reduction);

/* fctrl_cfg */
bool dtcp_window_based_fctrl(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return false;

        return cfg->fctrl_cfg->window_based_fctrl;
}
EXPORT_SYMBOL(dtcp_window_based_fctrl);

struct window_fctrl_config * dtcp_wfctrl_cfg(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return NULL;

        return cfg->fctrl_cfg->wfctrl_cfg;
}
EXPORT_SYMBOL(dtcp_wfctrl_cfg);

bool dtcp_rate_based_fctrl(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return false;

        return cfg->fctrl_cfg->rate_based_fctrl;
}
EXPORT_SYMBOL(dtcp_rate_based_fctrl);

struct rate_fctrl_config * dtcp_rfctrl_cfg(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return NULL;

        return cfg->fctrl_cfg->rfctrl_cfg;
}
EXPORT_SYMBOL(dtcp_rfctrl_cfg);

uint_t dtcp_sent_bytes_th(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return 0;

        return cfg->fctrl_cfg->sent_bytes_th;
}
EXPORT_SYMBOL(dtcp_sent_bytes_th);

uint_t dtcp_sent_bytes_percent_th(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return 0;

        return cfg->fctrl_cfg->sent_bytes_percent_th;
}
EXPORT_SYMBOL(dtcp_sent_bytes_percent_th);

uint_t dtcp_sent_buffers_th(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return 0;

        return cfg->fctrl_cfg->sent_buffers_th;
}
EXPORT_SYMBOL(dtcp_sent_buffers_th);

uint_t dtcp_rcvd_bytes_th(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return 0;

        return cfg->fctrl_cfg->rcvd_bytes_th;
}
EXPORT_SYMBOL(dtcp_rcvd_bytes_th);

uint_t dtcp_rcvd_bytes_percent_th(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return 0;

        return cfg->fctrl_cfg->rcvd_bytes_percent_th;
}
EXPORT_SYMBOL(dtcp_rcvd_bytes_percent_th);

uint_t dtcp_rcvd_buffers_th(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return 0;

        return cfg->fctrl_cfg->rcvd_buffers_th;
}
EXPORT_SYMBOL(dtcp_rcvd_buffers_th);

struct policy * dtcp_closed_window(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return NULL;

        return cfg->fctrl_cfg->closed_window;
}
EXPORT_SYMBOL(dtcp_closed_window);

/*
struct policy * dtcp_flow_control_overrun(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return NULL;

        return cfg->fctrl_cfg->flow_control_overrun;
}
EXPORT_SYMBOL(dtcp_flow_control_overrun);
*/

struct policy * dtcp_reconcile_flow_conflict(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return NULL;

        return cfg->fctrl_cfg->reconcile_flow_conflict;
}
EXPORT_SYMBOL(dtcp_reconcile_flow_conflict);

struct policy * dtcp_receiving_flow_control(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->fctrl_cfg)
                return NULL;

        return cfg->fctrl_cfg->receiving_flow_control;
}
EXPORT_SYMBOL(dtcp_receiving_flow_control);

/* dtcp_rxctrl_config */
uint_t dtcp_max_time_retry(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->rxctrl_cfg) return 0;

        return cfg->rxctrl_cfg->max_time_retry;
}
EXPORT_SYMBOL(dtcp_max_time_retry);

uint_t dtcp_data_retransmit_max(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->rxctrl_cfg) return 0;

        return cfg->rxctrl_cfg->data_retransmit_max;
}
EXPORT_SYMBOL(dtcp_data_retransmit_max);

uint_t dtcp_initial_tr(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->rxctrl_cfg) return 0;

        return cfg->rxctrl_cfg->initial_tr;
}
EXPORT_SYMBOL(dtcp_initial_tr);

struct policy * dtcp_retransmission_timer_expiry(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->rxctrl_cfg)
                return NULL;

        return cfg->rxctrl_cfg->retransmission_timer_expiry;
}
EXPORT_SYMBOL(dtcp_retransmission_timer_expiry);

struct policy * dtcp_sender_ack(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->rxctrl_cfg)
                return NULL;

        return cfg->rxctrl_cfg->sender_ack;
}
EXPORT_SYMBOL(dtcp_sender_ack);

struct policy * dtcp_receiving_ack_list(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->rxctrl_cfg)
                return NULL;

        return cfg->rxctrl_cfg->receiving_ack_list;
}
EXPORT_SYMBOL(dtcp_receiving_ack_list);

struct policy * dtcp_rcvr_ack(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->rxctrl_cfg)
                return NULL;

        return cfg->rxctrl_cfg->rcvr_ack;
}
EXPORT_SYMBOL(dtcp_rcvr_ack);

struct policy * dtcp_sending_ack(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->rxctrl_cfg)
                return NULL;

        return cfg->rxctrl_cfg->sending_ack;
}
EXPORT_SYMBOL(dtcp_sending_ack);

struct policy * dtcp_rcvr_control_ack(struct dtcp_config * cfg)
{
        if (!cfg || !cfg->rxctrl_cfg)
                return NULL;

        return cfg->rxctrl_cfg->rcvr_control_ack;
}
EXPORT_SYMBOL(dtcp_rcvr_control_ack);

/* dtcp_config */
bool dtcp_flow_ctrl(struct dtcp_config * cfg)
{
        if (!cfg)
                return false;

        return cfg->flow_ctrl;
}
EXPORT_SYMBOL(dtcp_flow_ctrl);

bool dtcp_rtx_ctrl(struct dtcp_config * cfg)
{
        if (!cfg)
                return false;

        return cfg->rtx_ctrl;
}
EXPORT_SYMBOL(dtcp_rtx_ctrl);

struct dtcp_fctrl_config * dtcp_fctrl_cfg(struct dtcp_config * cfg)
{
        if (!cfg)
                return NULL;

        return cfg->fctrl_cfg;
}
EXPORT_SYMBOL(dtcp_fctrl_cfg);

struct dtcp_rxctrl_config * dtcp_rxctrl_cfg(struct dtcp_config * cfg)
{
        if (!cfg)
                return NULL;

        return cfg->rxctrl_cfg;
}
EXPORT_SYMBOL(dtcp_rxctrl_cfg);

struct policy * dtcp_lost_control_pdu(struct dtcp_config * cfg)
{
        if (!cfg)
                return NULL;

        return cfg->lost_control_pdu;
}
EXPORT_SYMBOL(dtcp_lost_control_pdu);

struct policy * dtcp_rtt_estimator(struct dtcp_config * cfg)
{
        if (!cfg)
                return NULL;

        return cfg->rtt_estimator;
}
EXPORT_SYMBOL(dtcp_rtt_estimator);
