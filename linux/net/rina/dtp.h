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

#ifndef RINA_DTP_H
#define RINA_DTP_H

#include "common.h"
#include "rmt.h"
#include "kfa.h"
#include "dt.h"
#include "ps-factory.h"
#include "rds/robjects.h"

#define DTP_INACTIVITY_TIMERS_ENABLE 1

struct dtp * dtp_create(struct dt *         dt,
                        struct rmt *        rmt,
                        struct dtp_config * dtp_cfg,
			struct robject *    parent);
int          dtp_destroy(struct dtp * instance);

int          dtp_sv_init(struct dtp * dtp,
                         bool         rexmsn_ctrl,
                         bool         window_based,
                         bool         rate_based);
/* Sends a SDU to the DTP (DTP takes the ownership of the passed SDU) */
int          dtp_write(struct dtp * instance,
                       struct sdu * sdu);

/* DTP receives a PDU from RMT */
int          dtp_receive(struct dtp * instance,
                         struct pdu * pdu);

/*FIXME: This may be changed depending on the discussion around
 * RcvrInactivityTimer Policy */
/* DTP Policies called in DTCP */
int          dtp_initial_sequence_number(struct dtp * instance);

seq_num_t    dtp_sv_max_seq_nr_sent(struct dtp * instance);

int          dtp_sv_max_seq_nr_set(struct dtp * instance, seq_num_t num);
seq_num_t    dtp_sv_last_nxt_seq_nr(struct dtp * instance);
void         dtp_squeue_flush(struct dtp * dtp);
void         dtp_drf_required_set(struct dtp * dtp);
bool         dtp_sv_rate_fulfiled(struct dtp * instance);
int          dtp_sv_rate_fulfiled_set(struct dtp * instance, bool fulfiled);
// Does not start the timer(return false) if it's not necessary and packets can
// be processed.
void         dtp_start_rate_timer(struct dtp * dtp, struct dtcp * dtcp);
struct rtimer * dtp_sender_inactivity_timer(struct dtp * instance);

/* FIXME: temporal addition so that DTCP's sending ack can call this function
 * that was originally static */
struct pci * process_A_expiration(struct dtp * dtp, struct dtcp * dtcp);
int dtp_pdu_ctrl_send(struct dtp * dtp, struct pdu * pdu);

int                dtp_select_policy_set(struct dtp * dtp, const string_t *path,
                                         const string_t * name);

int                dtp_set_policy_set_param(struct dtp* dtp,
                                            const string_t * path,
                                            const string_t * name,
                                            const string_t * value);

struct dtp_ps * dtp_ps_get(struct dtp * dtp);

struct dtp* dtp_from_component(struct rina_component * component);

struct dt * dtp_dt(struct dtp * dtp);
struct rmt * dtp_rmt(struct dtp * dtp);
struct dtp_sv * dtp_dtp_sv(struct dtp * dtp);
struct connection * dtp_sv_connection(struct dtp_sv * sv);
int nxt_seq_reset(struct dtp_sv * sv, seq_num_t sn);
struct dtp_config * dtp_config_get(struct dtp * dtp);
#endif
