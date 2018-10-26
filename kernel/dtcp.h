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
#include "efcp-str.h"
#include "ps-factory.h"
#include "rds/robjects.h"

struct dtcp_config;
struct dtcp_fctrl_config;
struct dtcp_rctrl_config;
struct connection;
struct rmt;

struct dtcp *        dtcp_create(struct dtp *         dtp,
                                 struct rmt *         rmt,
                                 struct dtcp_config * dtcp_cfg,
				 struct robject *     parent);

int                  dtcp_destroy(struct dtcp * instance);

int                  dtcp_send(struct dtcp * instance,
                               struct du *  du);

/* Used by the DTP to notify the DTCP about events */
int                  dtcp_sv_update(struct dtcp *      dtcp,
                                    struct pci * pci);

/* Used by EFCP to send an incoming DTCP PDU */
int                  dtcp_common_rcv_control(struct dtcp * dtcp,
                                             struct du *  du);

/* Used by DTP to have an ack-control PDU sent by DTCP */
int                  dtcp_ack_flow_control_pdu_send(struct dtcp * instance,
                                                    seq_num_t     seq);
int		     dtcp_rendezvous_pdu_send(struct dtcp * instance);

/* begin SDK */
int          dtcp_select_policy_set(struct dtcp * dtcp, const string_t *path,
                                    const string_t * name);

int          dtcp_set_policy_set_param(struct dtcp * dtcp,
                                       const string_t * path,
                                       const string_t * name,
                                       const string_t * value);

struct dtcp	*dtcp_from_component(struct rina_component *component);
struct dtcp_ps	*dtcp_ps_get(struct dtcp *dtcp);
pdu_type_t	pdu_ctrl_type_get(struct dtcp *dtcp, seq_num_t seq);
struct du	*pdu_ctrl_create_ni(struct dtcp *dtcp, pdu_type_t type);
int		dtcp_pdu_send(struct dtcp *dtcp, struct du *du);
struct du	*pdu_ctrl_ack_create(struct dtcp *dtcp,
				     seq_num_t last_ctrl_seq_rcvd,
				     seq_num_t snd_left_wind_edge,
				     seq_num_t snd_rt_wind_edge);
struct du	*pdu_ctrl_generate(struct dtcp *dtcp, pdu_type_t type);
int		dtcp_last_time_set(struct dtcp *dtcp,
					   struct timespec *s);
bool		dtcp_rate_exceeded(struct dtcp *dtcp, int send);

/* end SDK */

int		pdus_sent_in_t_unit_set(struct dtcp *dtcp, uint_t s);
int ctrl_pdu_send(struct dtcp * dtcp, pdu_type_t type, bool direct);
/*FIXME: wrapper to be called by dtp in the post_worker */
int			dtcp_sending_ack_policy(struct dtcp *dtcp);
#endif
