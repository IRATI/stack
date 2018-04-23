/*
 * RMT (Relaying and Multiplexing Task) kRPI
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef RINA_RMT_PS_H
#define RINA_RMT_PS_H

#include <linux/types.h>

#include "rmt.h"
#include "du.h"
#include "rds/rfifo.h"
#include "ps-factory.h"

/* rmt_enqueue_policy return values */
#define RMT_PS_ENQ_SEND  0	/* PDU can be transmitted by the RMT */
#define RMT_PS_ENQ_SCHED 1	/* PDU enqueued and RMT needs to schedule */
#define RMT_PS_ENQ_ERR   2	/* Error */
#define RMT_PS_ENQ_DROP  3	/* PDU dropped due to queue full occupation */

struct rmt_ps {
	struct ps_base base;

	/* Behavioural policies. */
	struct du *(*rmt_dequeue_policy)(struct rmt_ps *,
					  struct rmt_n1_port *);
	/*
	 * If must_enqueue is true the policy must either enqueue or drop
	 * the PDU, otherwise it may also return RMT_PS_ENQ_SEND
	 */
	int (*rmt_enqueue_policy)(struct rmt_ps *,
				  struct rmt_n1_port *,
				  struct du *,
				  bool must_enqueue);
	void* (*rmt_q_create_policy)(struct rmt_ps *,
				   struct rmt_n1_port *);
	int (*rmt_q_destroy_policy)(struct rmt_ps *,
				    struct rmt_n1_port *);

	/* Reference used to access the RMT data model. */
	struct rmt *dm;

	/* Data private to the policy-set implementation. */
	void *priv;
};

/*
 * The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary.
 */
int rmt_ps_publish(struct ps_factory *factory);
int rmt_ps_unpublish(const char *name);

#endif
