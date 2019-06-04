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
#include "efcp-str.h"
#include "rmt.h"
#include "kfa.h"
#include "ps-factory.h"
#include "rds/robjects.h"

struct dtp * dtp_create(struct efcp *       efcp,
                        struct rmt *        rmt,
                        struct dtp_config * dtp_cfg,
			struct robject *    parent);
int          dtp_destroy(struct dtp * instance);

int          dtp_sv_init(struct dtp * dtp,
                         bool         rexmsn_ctrl,
                         bool         window_based,
                         bool         rate_based,
			 uint_t      mfps,
			 uint_t      mfss,
			 u_int32_t   mpl,
			 timeout_t   a,
			 timeout_t   r,
			 timeout_t   tr);

/* Sends a SDU to the DTP (DTP takes the ownership of the passed SDU) */
int          dtp_write(struct dtp * instance,
                       struct du * du);

void dtp_send_pending_ctrl_pdus(struct dtp * dtp);

/* DTP receives a PDU from RMT */
int          dtp_receive(struct dtp * instance,
                         struct du * du);

/*FIXME: This may be changed depending on the discussion around
 * RcvrInactivityTimer Policy */
/* DTP Policies called in DTCP */
int          dtp_initial_sequence_number(struct dtp * instance);

void         dtp_squeue_flush(struct dtp * dtp);

// Does not start the timer(return false) if it's not necessary and packets can
// be processed.
void         dtp_start_rate_timer(struct dtp * dtp, struct dtcp * dtcp);

/* FIXME: temporal addition so that DTCP's sending ack can call this function
 * that was originally static */
struct pci * process_A_expiration(struct dtp * dtp, struct dtcp * dtcp);
int dtp_pdu_ctrl_send(struct dtp * dtp, struct du * du);

int                dtp_select_policy_set(struct dtp * dtp, const string_t *path,
                                         const string_t * name);

int                dtp_set_policy_set_param(struct dtp* dtp,
                                            const string_t * path,
                                            const string_t * name,
                                            const string_t * value);

struct dtp_ps * dtp_ps_get(struct dtp * dtp);

struct dtp* dtp_from_component(struct rina_component * component);

#endif
