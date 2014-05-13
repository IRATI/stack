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

#ifndef RINA_DTCP_UTILS_H
#define RINA_DTCP_UTILS_H

struct dtcp_config;
struct dtcp_fctrl_config;
struct dtcp_rctrl_config;
struct window_fctrl_config;
struct rate_fctrl_config;


/* window_fctrl_config */
uint_t dtcp_max_closed_winq_length(struct dtcp_config * cfg);
uint_t dtcp_initial_credit(struct dtcp_config * cfg);
struct policy * dtcp_rcvr_flow_control(struct dtcp_config * cfg);
struct policy * dtcp_receiving_flow_control(struct dtcp_config * cfg); 

/* rate_fctrl_config */
uint_t dtcp_sending_rate (struct dtcp_config * cfg); 
uint_t dtcp_time_period(struct dtcp_config * cfg);
struct policy * dtcp_no_rate_slow_down(struct dtcp_config * cfg);
struct policy * dtcp_no_override_default_peak(struct dtcp_config * cfg);
struct policy * dtcp_rate_reduction(struct dtcp_config * cfg);

/* dtcp_fctrl_config */
bool dtcp_window_based_fctrl(struct dtcp_config * cfg);
struct window_fctrl_config * dtcp_wfctrl_cfg(struct dtcp_config * cfg);
bool dtcp_rate_based_fctrl(struct dtcp_config * cfg);
struct rate_fctrl_config * dtcp_rfctrl_cfg(struct dtcp_config * cfg);
uint_t dtcp_sent_bytes_th(struct dtcp_config * cfg);
uint_t dtcp_sent_bytes_percent_th(struct dtcp_config * cfg);
uint_t dtcp_sent_buffers_th(struct dtcp_config * cfg);
uint_t dtcp_rcvd_bytes_th(struct dtcp_config * cfg);
uint_t dtcp_rcvd_bytes_percent_th(struct dtcp_config * cfg);
uint_t dtcp_rcvd_buffers_th(struct dtcp_config * cfg);
struct policy * dtcp_closed_window(struct dtcp_config * cfg);
struct policy * dtcp_flow_control_overrun(struct dtcp_config * cfg);
struct policy * dtcp_reconcile_flow_conflict(struct dtcp_config * cfg);
struct policy * dtcp_receiver_inactivity_timer(struct dtcp_config * cfg);
struct policy * dtcp_sender_inactivity_timer(struct dtcp_config * cfg);

/* dtcp_rxctrl_config */
uint_t dtcp_data_retransmit_max(struct dtcp_config * cfg);
timeout_t dtcp_intial_a(struct dtcp_config * cfg);
struct policy * dtcp_rtt_estimator(struct dtcp_config * cfg);
struct policy * dtcp_retransmission_timer_expiry(struct dtcp_config * cfg);
struct policy * dtcp_sender_ack(struct dtcp_config * cfg);
struct policy * dtcp_receiving_ack_list(struct dtcp_config * cfg);
struct policy * dtcp_rcvr_ack(struct dtcp_config * cfg);
struct policy * dtcp_sending_ack(struct dtcp_config * cfg);
struct policy * dtcp_rcvr_control_ack(struct dtcp_config * cfg);

/* dtcp_config */
bool dtcp_flow_ctrl(struct dtcp_config * cfg);
bool dtcp_rtx_ctrl(struct dtcp_config * cfg);
struct dtcp_fctrl_config * dtcp_fctrl_cfg(struct dtcp_config * cfg);
struct dtcp_rxctrl_config * dtcp_rxctrl_cfg(struct dtcp_config * cfg);
timeout_t dtcp_receiver_inactivity(struct dtcp_config * cfg);
timeout_t dtcp_sender_inactivity(struct dtcp_config * cfg);
struct policy * dtcp_lost_control_pdu(struct dtcp_config * cfg);

#endif


