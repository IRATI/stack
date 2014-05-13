/*
 * DTCP Utils (Data Transfer Control Protocol)
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

#define RINA_PREFIX "dtcp-utils"

#include "logs.h"
#include "policies.h"

struct window_fctrl_config {
        uint_t          max_closed_winq_length; /* in cwq */
        uint_t          initial_credit; /* to initialize sv */
        struct policy * rcvr_flow_control;
        struct policy * receiving_flow_control; 
};

struct rate_fctrl_config {
        uint_t          sending_rate; /* FIXME: to initialize sv? does not seem so*/
        uint_t          time_period;
        struct policy * no_rate_slow_down;
        struct policy * no_override_default_peak;
        struct policy * rate_reduction;
};

struct dtcp_fctrl_config {
        bool                         window_based_fctrl;
        struct window_fctrl_config * wfctrl_cfg;
        bool                         rate_based_fctrl;
        struct rate_fctrl_config *   rfctrl_cfg;
        uint_t                       sent_bytes_th;
        uint_t                       sent_bytes_percent_th;
        uint_t                       sent_buffers_th;
        uint_t                       rcvd_bytes_th;
        uint_t                       rcvd_bytes_percent_th;
        uint_t                       rcvd_buffers_th;
        struct policy *              closed_window;
        struct policy *              flow_control_overrun;
        struct policy *              reconcile_flow_conflict;
        struct policy *              receiver_inactivity_timer;
        struct policy *              sender_inactivity_timer;
};

struct dtcp_rxctrl_config {
        uint_t          data_retransmit_max;
        timeout_t       intial_a;
        struct policy * rtt_estimator;
        struct policy * retransmission_timer_expiry;
        struct policy * sender_ack;
        struct policy * receiving_ack_list;
        struct policy * rcvr_ack;
        struct policy * sending_ack;
        struct policy * rcvr_control_ack;
};

/* This is the DTCP configurations from connection policies */
struct dtcp_config {
        bool                        flow_ctrl;
        bool                        rtx_ctrl;
        struct dtcp_fctrl_config *  fctrl_cfg;
        struct dtcp_rxctrl_config * rxctrl_cfg;
        timeout_t                   receiver_inactivity;
        timeout_t                   sender_inactivity;
        struct policy *             lost_control_pdu;
};

/* constructors */

/* Setters & getters */

/* window_fctrl_config */
uint_t dtcp_max_closed_winq_length(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->wfctrl_cfg->max_closed_winq_length; }
EXPORT_SYMBOL(dtcp_max_closed_winq_length);

uint_t dtcp_initial_credit(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->wfctrl_cfg->initial_credit; }
EXPORT_SYMBOL(dtcp_initial_credit);

struct policy * dtcp_rcvr_flow_control(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->wfctrl_cfg->rcvr_flow_control; }
EXPORT_SYMBOL(dtcp_rcvr_flow_control);

struct policy * dtcp_receiving_flow_control(struct dtcp_config * cfg) 
{ return cfg->fctrl_cfg->wfctrl_cfg->receiving_flow_control; }
EXPORT_SYMBOL(dtcp_receiving_flow_control);

/* rate_fctrl_cfg */
uint_t dtcp_sending_rate (struct dtcp_config * cfg) 
{ return cfg->fctrl_cfg->rfctrl_cfg->sending_rate; }
EXPORT_SYMBOL(dtcp_sending_rate);

uint_t dtcp_time_period(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->rfctrl_cfg->time_period; }
EXPORT_SYMBOL(dtcp_time_period);

struct policy * dtcp_no_rate_slow_down(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->rfctrl_cfg->no_rate_slow_down; }
EXPORT_SYMBOL(dtcp_no_rate_slow_down);

struct policy * dtcp_no_override_default_peak(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->rfctrl_cfg->no_override_default_peak; }
EXPORT_SYMBOL(dtcp_no_override_default_peak);

struct policy * dtcp_rate_reduction(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->rfctrl_cfg->rate_reduction; }
EXPORT_SYMBOL(dtcp_rate_reduction);

/* fctrl_cfg */
bool dtcp_window_based_fctrl(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->window_based_fctrl; }
EXPORT_SYMBOL(dtcp_window_based_fctrl);

struct window_fctrl_config * dtcp_wfctrl_cfg(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->wfctrl_cfg; }
EXPORT_SYMBOL(dtcp_wfctrl_cfg);

bool dtcp_rate_based_fctrl(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->rate_based_fctrl; }
EXPORT_SYMBOL(dtcp_rate_based_fctrl);

struct rate_fctrl_config * dtcp_rfctrl_cfg(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->rfctrl_cfg; }
EXPORT_SYMBOL(dtcp_rfctrl_cfg);

uint_t dtcp_sent_bytes_th(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->sent_bytes_th; }
EXPORT_SYMBOL(dtcp_sent_bytes_th);

uint_t dtcp_sent_bytes_percent_th(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->sent_bytes_percent_th; }
EXPORT_SYMBOL(dtcp_sent_bytes_percent_th);

uint_t dtcp_sent_buffers_th(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->sent_buffers_th; }
EXPORT_SYMBOL(dtcp_sent_buffers_th);

uint_t dtcp_rcvd_bytes_th(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->rcvd_bytes_th; }
EXPORT_SYMBOL(dtcp_rcvd_bytes_th);

uint_t dtcp_rcvd_bytes_percent_th(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->rcvd_bytes_percent_th; }
EXPORT_SYMBOL(dtcp_rcvd_bytes_percent_th);

uint_t dtcp_rcvd_buffers_th(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->rcvd_buffers_th; }
EXPORT_SYMBOL(dtcp_rcvd_buffers_th);

struct policy * dtcp_closed_window(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->closed_window; }
EXPORT_SYMBOL(dtcp_closed_window);

struct policy * dtcp_flow_control_overrun(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->flow_control_overrun; }
EXPORT_SYMBOL(dtcp_flow_control_overrun);

struct policy * dtcp_reconcile_flow_conflict(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->reconcile_flow_conflict; }
EXPORT_SYMBOL(dtcp_reconcile_flow_conflict);

struct policy * dtcp_receiver_inactivity_timer(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->receiver_inactivity_timer; }
EXPORT_SYMBOL(dtcp_receiver_inactivity_timer);

struct policy * dtcp_sender_inactivity_timer(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg->sender_inactivity_timer; }
EXPORT_SYMBOL(dtcp_sender_inactivity_timer);

/* dtcp_rxctrl_config */
uint_t dtcp_data_retransmit_max(struct dtcp_config * cfg)
{ return cfg->rxctrl_cfg->data_retransmit_max; }
EXPORT_SYMBOL(dtcp_data_retransmit_max);

timeout_t dtcp_intial_a(struct dtcp_config * cfg)
{ return cfg->rxctrl_cfg->intial_a; }
EXPORT_SYMBOL(dtcp_intial_a);

struct policy * dtcp_rtt_estimator(struct dtcp_config * cfg)
{ return cfg->rxctrl_cfg->rtt_estimator; }
EXPORT_SYMBOL(dtcp_rtt_estimator);

struct policy * dtcp_retransmission_timer_expiry(struct dtcp_config * cfg)
{ return cfg->rxctrl_cfg->retransmission_timer_expiry; }
EXPORT_SYMBOL(dtcp_retransmission_timer_expiry);

struct policy * dtcp_sender_ack(struct dtcp_config * cfg)
{ return cfg->rxctrl_cfg->sender_ack; }
EXPORT_SYMBOL(dtcp_sender_ack);

struct policy * dtcp_receiving_ack_list(struct dtcp_config * cfg)
{ return cfg->rxctrl_cfg->receiving_ack_list; }
EXPORT_SYMBOL(dtcp_receiving_ack_list);

struct policy * dtcp_rcvr_ack(struct dtcp_config * cfg)
{ return cfg->rxctrl_cfg->rcvr_ack; }
EXPORT_SYMBOL(dtcp_rcvr_ack);

struct policy * dtcp_sending_ack(struct dtcp_config * cfg)
{ return cfg->rxctrl_cfg->sending_ack; }
EXPORT_SYMBOL(dtcp_sending_ack);

struct policy * dtcp_rcvr_control_ack(struct dtcp_config * cfg)
{ return cfg->rxctrl_cfg->rcvr_control_ack; }
EXPORT_SYMBOL(dtcp_rcvr_control_ack);

/* dtcp_config */
bool dtcp_flow_ctrl(struct dtcp_config * cfg)
{ return cfg->flow_ctrl; }
EXPORT_SYMBOL(dtcp_flow_ctrl);

bool dtcp_rtx_ctrl(struct dtcp_config * cfg)
{ return cfg->rtx_ctrl; }
EXPORT_SYMBOL(dtcp_rtx_ctrl);

struct dtcp_fctrl_config * dtcp_fctrl_cfg(struct dtcp_config * cfg)
{ return cfg->fctrl_cfg; }
EXPORT_SYMBOL(dtcp_fctrl_cfg);

struct dtcp_rxctrl_config * dtcp_rxctrl_cfg(struct dtcp_config * cfg)
{ return cfg->rxctrl_cfg; }
EXPORT_SYMBOL(dtcp_rxctrl_cfg);

timeout_t dtcp_receiver_inactivity(struct dtcp_config * cfg)
{ return cfg->receiver_inactivity; }
EXPORT_SYMBOL(dtcp_receiver_inactivity);

timeout_t dtcp_sender_inactivity(struct dtcp_config * cfg)
{ return cfg->sender_inactivity; }
EXPORT_SYMBOL(dtcp_sender_inactivity);

struct policy * dtcp_lost_control_pdu(struct dtcp_config * cfg)
{ return cfg->lost_control_pdu; }
EXPORT_SYMBOL(dtcp_lost_control_pdu);

