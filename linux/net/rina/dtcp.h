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

#ifndef RINA_DTCP_H
#define RINA_DTCP_H

#include "common.h"
#include "du.h"
#include "dt.h"
#include "ps-factory.h"
#include "rds/robjects.h"

struct dtcp_config;
struct dtcp_fctrl_config;
struct dtcp_rctrl_config;
struct connection;
struct rmt;

struct dtcp *        dtcp_create(struct dt *          dt,
                                 struct rmt *         rmt,
                                 struct dtcp_config * dtcp_cfg,
				 struct robject *     parent);

int                  dtcp_destroy(struct dtcp * instance);

int                  dtcp_send(struct dtcp * instance,
                               struct sdu *  sdu);

/* Used by the DTP to notify the DTCP about events */
int                  dtcp_sv_update(struct dtcp *      dtcp,
                                    const struct pci * pci);

/* Used by EFCP to send an incoming DTCP PDU */
int                  dtcp_common_rcv_control(struct dtcp * dtcp,
                                             struct pdu *  pdu);

/* Used by DTP to have an ack-control PDU sent by DTCP */
int                  dtcp_ack_flow_control_pdu_send(struct dtcp * instance,
                                                    seq_num_t     seq);

seq_num_t            dtcp_rcv_rt_win(struct dtcp * instance);
seq_num_t            dtcp_snd_rt_win(struct dtcp * instance);
seq_num_t            dtcp_snd_lf_win(struct dtcp * instance);
int                  dtcp_snd_lf_win_set(struct dtcp * instance,
                                         seq_num_t     seq_num);
int                  dtcp_snd_rt_win_set(struct dtcp * dtcp,
                                         seq_num_t rt_win_edge);
int                  dtcp_rcv_rt_win_set(struct dtcp * instance,
                                         seq_num_t     seq_num);
void                 dtcp_rcvr_credit_set(struct dtcp * dtcp,
                                          uint_t credit);

struct dtcp_config * dtcp_config_get(struct dtcp * dtcp);

/* begin SDK */
int          dtcp_select_policy_set(struct dtcp * dtcp, const string_t *path,
                                    const string_t * name);

int          dtcp_set_policy_set_param(struct dtcp * dtcp,
                                       const string_t * path,
                                       const string_t * name,
                                       const string_t * value);

struct dtcp *    dtcp_from_component(struct rina_component * component);
struct dtcp_ps * dtcp_ps_get(struct dtcp * dtcp);
struct dt *      dtcp_dt(struct dtcp * dtcp);
pdu_type_t       pdu_ctrl_type_get(struct dtcp * dtcp, seq_num_t seq);
struct pdu *     pdu_ctrl_create_ni(struct dtcp * dtcp, pdu_type_t type);
seq_num_t        snd_lft_win(struct dtcp * dtcp);
seq_num_t        snd_rt_wind_edge(struct dtcp * dtcp);
seq_num_t        rcvr_rt_wind_edge(struct dtcp * dtcp);
void             dump_we(struct dtcp * dtcp, struct pci *  pci);
int              dtcp_pdu_send(struct dtcp * dtcp, struct pdu * pdu);
seq_num_t        last_rcv_ctrl_seq(struct dtcp * dtcp);
struct pdu *     pdu_ctrl_ack_create(struct dtcp * dtcp,
                                     seq_num_t     last_ctrl_seq_rcvd,
                                     seq_num_t     snd_left_wind_edge,
                                     seq_num_t     snd_rt_wind_edge);
struct pdu *     pdu_ctrl_generate(struct dtcp * dtcp, pdu_type_t type);
uint_t           dtcp_rcvr_credit(struct dtcp * dtcp);
void             dtcp_rcvr_credit_set(struct dtcp * dtcp, uint_t credit);
void             update_rt_wind_edge(struct dtcp * dtcp);
void 		 update_credit_and_rt_wind_edge(struct dtcp * dtcp, uint_t credit);
uint_t           dtcp_rtt(struct dtcp * dtcp);
int              dtcp_rtt_set(struct dtcp * dtcp, uint_t rtt);
uint_t           dtcp_srtt(struct dtcp * dtcp);
int              dtcp_srtt_set(struct dtcp * dtcp, uint_t srtt);
uint_t           dtcp_rttvar(struct dtcp * dtcp);
int              dtcp_rttvar_set(struct dtcp * dtcp, uint_t rttvar);

uint_t 		 dtcp_time_frame(struct dtcp * dtcp);
int 		 dtcp_time_frame_set(struct dtcp * dtcp, uint_t sec);
int 		 dtcp_last_time(struct dtcp * dtcp, struct timespec * s);
int 		 dtcp_last_time_set(struct dtcp * dtcp, struct timespec * s);
uint_t 		 dtcp_sndr_rate(struct dtcp * dtcp);
int 		 dtcp_sndr_rate_set(struct dtcp * dtcp, uint_t rate);
uint_t 		 dtcp_rcvr_rate(struct dtcp * dtcp);
int 		 dtcp_rcvr_rate_set(struct dtcp * dtcp, uint_t rate);
uint_t 		 dtcp_recv_itu(struct dtcp * dtcp);
int 		 dtcp_recv_itu_set(struct dtcp * dtcp, uint_t recv);
int 		 dtcp_recv_itu_inc(struct dtcp * dtcp, uint_t recv);
uint_t 		 dtcp_sent_itu(struct dtcp * dtcp);
int 		 dtcp_sent_itu_set(struct dtcp * dtcp, uint_t sent);
int 		 dtcp_sent_itu_inc(struct dtcp * dtcp, uint_t sent);
int 		 dtcp_rate_fc_reset(struct dtcp * dtcp, struct timespec * now);
bool 		 dtcp_rate_exceeded(struct dtcp * dtcp, int send);

/* end SDK */

int              pdus_sent_in_t_unit_set(struct dtcp * dtcp, uint_t s);

/*FIXME: wrapper to be called by dtp in the post_worker */
int              dtcp_sending_ack_policy(struct dtcp * dtcp);
#endif
