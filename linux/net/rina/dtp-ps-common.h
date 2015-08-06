/*
 * Common function for DTP's policy set
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *    Miquel Tarzan <miquel.tarzan@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option); any later version.
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

#ifndef RINA_DTP_PS_COMMON_H
#define RINA_DTP_PS_COMMON_H

#include "dtp-ps.h"
#include "dtcp.h"

int
common_transmission_control(struct dtp_ps * ps, struct pdu * pdu);

int
common_closed_window(struct dtp_ps * ps, struct pdu * pdu);

int
common_flow_control_overrun(struct dtp_ps * ps,
                            struct pdu *    pdu);

int
common_initial_sequence_number(struct dtp_ps * ps);

int
common_receiver_inactivity_timer(struct dtp_ps * ps);

int
common_sender_inactivity_timer(struct dtp_ps * ps);

bool
common_reconcile_flow_conflict(struct dtp_ps * ps);

int dtp_ps_common_set_policy_set_param(struct ps_base * bps,
                                       const char    * name,
                                       const char    * value);
#endif
