/*
 * Delimiting Policy Set (fragmentation, reassembly, concatenation, fragmentation)
 *
 *    Eduard Grasa    <eduard.grasa@i2cat.net>
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

#ifndef RINA_DELIMITING_PS_H
#define RINA_DELIMITING_PS_H

#include <linux/types.h>

#include "delim.h"
#include "du.h"
#include "ps-factory.h"

struct delim_ps {
	struct ps_base base;

	/* Behavioural policies. */
	int (* delim_fragment)(struct delim_ps *, struct du *,
			       struct du_list *);

	/* Reference used to access the Delimiting data model. */
	struct delim * dm;

	/* Data private to the policy-set implementation. */
	void *       priv;
};

/*
 * The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary.
 */
int delim_ps_publish(struct ps_factory * factory);
int delim_ps_unpublish(const char * name);

#endif
