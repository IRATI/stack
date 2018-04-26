/*
 * Default policy set for RMT
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

#ifndef RMT_PS_DEFAULT_H
#define RMT_PS_DEFAULT_H

#include "rmt.h"
#include "rmt-ps.h"

struct ps_base * rmt_ps_default_create(struct rina_component *component);
void rmt_ps_default_destroy(struct ps_base *bps);
void *default_rmt_q_create_policy(struct rmt_ps      *ps,
				  struct rmt_n1_port *n1_port);
int default_rmt_q_destroy_policy(struct rmt_ps      *ps,
				 struct rmt_n1_port *n1_port);
int default_rmt_enqueue_policy(struct rmt_ps	 *ps,
			       struct rmt_n1_port *n1_port,
			       struct du	 *du,
			       bool               must_enqueue);
struct du *default_rmt_dequeue_policy(struct rmt_ps	 *ps,
				      struct rmt_n1_port *n1_port);
#endif
