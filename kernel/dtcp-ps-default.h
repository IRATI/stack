/*
 * Common implementation of a default policy set for DTCP
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

#ifndef RINA_DTCP_PS_COMMON_H
#define RINA_DTCP_PS_COMMON_H

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/random.h>

#include "dtcp-ps.h"

struct ps_base * dtcp_ps_default_create(struct rina_component *component);
void dtcp_ps_default_destroy(struct ps_base *bps);
int default_lost_control_pdu(struct dtcp_ps * ps);

#ifdef CONFIG_RINA_DTCP_RCVR_ACK
int default_rcvr_ack(struct dtcp_ps * ps, const struct pci * pci);
#endif /* CONFIG_RINA_DTCP_RCVR_ACK */

#ifdef CONFIG_RINA_DTCP_RCVR_ACK_ATIMER
int default_rcvr_ack_atimer(struct dtcp_ps * ps, const struct pci * pci);
#endif

int default_sender_ack(struct dtcp_ps * ps, seq_num_t seq_num);

int default_sending_ack(struct dtcp_ps * ps, seq_num_t seq);

int default_receiving_flow_control(struct dtcp_ps * ps, const struct pci * pci);

int default_rcvr_flow_control(struct dtcp_ps * ps, const struct pci * pci);

int default_rate_reduction(struct dtcp_ps * ps, const struct pci * pci);

int default_rtt_estimator(struct dtcp_ps * ps, seq_num_t sn);

int default_rtt_estimator_nortx(struct dtcp_ps * ps, seq_num_t sn);

int default_rcvr_rendezvous(struct dtcp_ps * ps, const struct pci * pci);

#endif /* RINA_DTCP_PS_COMMON_H */
