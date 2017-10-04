/*
 * DTCP Utils (Data Transfer Control Protocol)
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

#ifndef RINA_DTCP_CONF_UTILS_H
#define RINA_DTCP_CONF_UTILS_H

#include "common.h"

/* Constructors */
struct dtcp_config *         dtcp_config_create_ni(void);
struct window_fctrl_config * dtcp_window_fctrl_config_create(void);
struct window_fctrl_config * dtcp_window_fctrl_config_create_ni(void);
struct rate_fctrl_config *   dtcp_rate_fctrl_config_create(void);
struct rate_fctrl_config *   dtcp_rate_fctrl_config_create_ni(void);
struct dtcp_fctrl_config *   dtcp_fctrl_config_create_ni(void);
struct dtcp_rxctrl_config *  dtcp_rxctrl_config_create_ni(void);
/* Destructors */
int dtcp_config_destroy(struct dtcp_config * cfg);

/* Setters */
/* window_fctrl_config */
int dtcp_max_closed_winq_length_set(struct dtcp_config * cfg, uint_t len);
int dtcp_initial_credit_set(struct dtcp_config * cfg, uint_t ic);
int dtcp_rcvr_flow_control_set(struct dtcp_config * cfg,
                               struct policy * rcvr_flow_control);
int dtcp_tx_control_set(struct dtcp_config * cfg,
                        struct policy * tx_control);
/* rate_fctrl_cfg */
int dtcp_sending_rate_set(struct dtcp_config * cfg, uint_t sending_rate);
int dtcp_time_period_set(struct dtcp_config * cfg, uint_t time_period);
int dtcp_no_rate_slow_down_set(struct dtcp_config * cfg,
                               struct policy * no_rate_slow_down);
int dtcp_no_override_default_peak_set(struct dtcp_config * cfg,
                                      struct policy * no_override_def_peak);
int dtcp_rate_reduction_set(struct dtcp_config * cfg,
                            struct policy * rate_reduction);

/* fctrl_cfg */
int dtcp_window_based_fctrl_set(struct dtcp_config * cfg, bool wb_fctrl);
int dtcp_wfctrl_cfg_set(struct dtcp_config * cfg,
                        struct window_fctrl_config * wfctrl_cfg);
int dtcp_rate_based_fctrl_set(struct dtcp_config * cfg,
                              bool rate_based_fctrl);
int dtcp_rfctrl_cfg_set(struct dtcp_config * cfg,
                        struct rate_fctrl_config * rfctrl_cfg);
int dtcp_sent_bytes_th_set(struct dtcp_config * cfg, uint_t sent_bytes_th);
int dtcp_sent_bytes_percent_th_set(struct dtcp_config * cfg,
                                   uint_t sent_bytes_percent_th);
int dtcp_sent_buffers_th_set(struct dtcp_config * cfg, uint_t sent_buffers_th);
int dtcp_rcvd_bytes_th_set(struct dtcp_config * cfg, uint_t rcvd_bytes_th);
int dtcp_rcvd_bytes_percent_th_set(struct dtcp_config * cfg,
                                   uint_t rcvd_byte_pth);
int dtcp_rcvd_buffers_th_set(struct dtcp_config * cfg,
                             uint_t rcvd_buffers_th);
int dtcp_closed_window_set(struct dtcp_config * cfg,
                           struct policy * closed_window);
int dtcp_flow_control_overrun_set(struct dtcp_config * cfg,
                                  struct policy * flow_control_overrun);
int dtcp_reconcile_flow_conflict_set(struct dtcp_config * cfg,
                                     struct policy * reconcile_flow_conflict);
int dtcp_receiver_inactivity_timer_set(struct dtcp_config * cfg,
                                       struct policy * rcvr_inactivity_timer);
int dtcp_sender_inactivity_timer_set(struct dtcp_config * cfg,
                                     struct policy * sender_inactivity_timer);
int dtcp_receiving_flow_control_set(struct dtcp_config * cfg,
                                    struct policy * receiving_flow_control);

/* dtcp_rxctrl_config */
int dtcp_max_time_retry_set(struct dtcp_config * cfg,
                            uint_t max_time_retry);
int dtcp_data_retransmit_max_set(struct dtcp_config * cfg,
                                 uint_t data_retransmit_max);
int dtcp_initial_tr_set(struct dtcp_config * cfg,
                        uint_t               tr);
int dtcp_retransmission_timer_expiry_set(struct dtcp_config * cfg,
                                         struct policy * rtx_timer_expiry);
int dtcp_sender_ack_set(struct dtcp_config * cfg,
                        struct policy * sender_ack);
int dtcp_receiving_ack_list_set(struct dtcp_config * cfg,
                                struct policy * receiving_ack_list);
int dtcp_rcvr_ack_set(struct dtcp_config * cfg, struct policy * rcvr_ack);
int dtcp_sending_ack_set(struct dtcp_config * cfg, struct policy * sending_ack);
int dtcp_rcvr_control_ack_set(struct dtcp_config * cfg,
                              struct policy * rcvr_control_ack);

/* dtcp_config */
int dtcp_flow_ctrl_set(struct dtcp_config * cfg, bool flow_ctrl);
int dtcp_rtx_ctrl_set(struct dtcp_config * cfg, bool rtx_ctrl);
int dtcp_fctrl_cfg_set(struct dtcp_config * cfg,
                       struct dtcp_fctrl_config * fctrl_cfg);
int dtcp_rxctrl_cfg_set(struct dtcp_config * cfg,
                        struct dtcp_rxctrl_config * rxctrl_cfg);
int dtcp_lost_control_pdu_set(struct dtcp_config * cfg,
                              struct policy * lost_control_pdu);
int dtcp_ps_set(struct dtcp_config * cfg,
                struct policy * dtcp_ps);
int dtcp_rtt_estimator_set(struct dtcp_config * cfg,
                           struct policy * rtt_estimator);

/* window_fctrl_config */
uint_t          dtcp_max_closed_winq_length(struct dtcp_config * cfg);
uint_t          dtcp_initial_credit(struct dtcp_config * cfg);
struct policy * dtcp_rcvr_flow_control(struct dtcp_config * cfg);
struct policy * dtcp_tx_control(struct dtcp_config * cfg);

/* rate_fctrl_config */
uint_t          dtcp_sending_rate (struct dtcp_config * cfg);
uint_t          dtcp_time_period(struct dtcp_config * cfg);
struct policy * dtcp_no_rate_slow_down(struct dtcp_config * cfg);
struct policy * dtcp_no_override_default_peak(struct dtcp_config * cfg);
struct policy * dtcp_rate_reduction(struct dtcp_config * cfg);

/* dtcp_fctrl_config */
bool dtcp_window_based_fctrl(struct dtcp_config * cfg);
bool dtcp_rate_based_fctrl(struct dtcp_config * cfg);
struct window_fctrl_config * dtcp_wfctrl_cfg(struct dtcp_config * cfg);
struct rate_fctrl_config *   dtcp_rfctrl_cfg(struct dtcp_config * cfg);
uint_t dtcp_sent_bytes_th(struct dtcp_config * cfg);
uint_t dtcp_sent_bytes_percent_th(struct dtcp_config * cfg);
uint_t dtcp_sent_buffers_th(struct dtcp_config * cfg);
uint_t dtcp_rcvd_bytes_th(struct dtcp_config * cfg);
uint_t dtcp_rcvd_bytes_percent_th(struct dtcp_config * cfg);
uint_t dtcp_rcvd_buffers_th(struct dtcp_config * cfg);
struct policy * dtcp_closed_window(struct dtcp_config * cfg);
struct policy * dtcp_flow_control_overrun(struct dtcp_config * cfg);
struct policy * dtcp_reconcile_flow_conflict(struct dtcp_config * cfg);
struct policy * dtcp_receiving_flow_control(struct dtcp_config * cfg);

/* dtcp_rxctrl_config */
uint_t          dtcp_max_time_retry(struct dtcp_config * cfg);
uint_t          dtcp_data_retransmit_max(struct dtcp_config * cfg);
uint_t          dtcp_initial_tr(struct dtcp_config * cfg);
struct policy * dtcp_retransmission_timer_expiry(struct dtcp_config * cfg);
struct policy * dtcp_sender_ack(struct dtcp_config * cfg);
struct policy * dtcp_receiving_ack_list(struct dtcp_config * cfg);
struct policy * dtcp_rcvr_ack(struct dtcp_config * cfg);
struct policy * dtcp_sending_ack(struct dtcp_config * cfg);
struct policy * dtcp_rcvr_control_ack(struct dtcp_config * cfg);

/* dtcp_config */
bool                        dtcp_flow_ctrl(struct dtcp_config * cfg);
bool                        dtcp_rtx_ctrl(struct dtcp_config * cfg);
struct dtcp_fctrl_config *  dtcp_fctrl_cfg(struct dtcp_config * cfg);
struct dtcp_rxctrl_config * dtcp_rxctrl_cfg(struct dtcp_config * cfg);
struct policy *             dtcp_lost_control_pdu(struct dtcp_config * cfg);
struct policy *             dtcp_rtt_estimator(struct dtcp_config * cfg);

#endif
