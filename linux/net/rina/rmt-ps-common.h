/*
 * Common parts of the common policy set for RMT.
 * This is envisioned for reurilization in other custom policy sets
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef RINA_RMT_PS_COMMON_H
#define RINA_RMT_PS_COMMON_H

#include "rmt.h"
#include "pdu.h"
#include "ps-factory.h"

struct rmt_ps_common_data {
	struct rmt_queue_set *outqs;
};

int common_rmt_scheduling_create_policy_tx(struct rmt_ps *ps,
					   struct rmt_n1_port *n1_port);
int common_rmt_scheduling_destroy_policy_tx(struct rmt_ps *ps,
					    struct rmt_n1_port *n1_port);
/* NOTE: The policy is called with the n1_port lock taken */
int common_rmt_enqueue_scheduling_policy_tx(struct rmt_ps *ps,
					    struct rmt_n1_port *n1_port,
					    struct pdu *pdu);
/* NOTE: The policy is called with the n1_port lock taken */
int common_rmt_requeue_scheduling_policy_tx(struct rmt_ps *ps,
					    struct rmt_n1_port *n1_port,
					    struct pdu *pdu);
struct pdu *common_rmt_next_scheduled_policy_tx(struct rmt_ps *ps,
						struct rmt_n1_port *n1_port);
int common_rmt_scheduling_policy_rx(struct rmt_ps *ps,
				    struct rmt_n1_port *n1_port,
				    struct sdu *sdu);
/* FIXME: This name should be common_X as well */
int rmt_ps_common_set_policy_set_param(struct ps_base *bps,
				       const char *name,
				       const char *value);
#endif
