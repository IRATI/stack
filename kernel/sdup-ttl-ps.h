/*
 * SDU Protection TTL Policy Set
 *
 *    Ondrej Lichtner <ilichtner@fit.vutbr.cz>
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

#ifndef RINA_SDUP_TTL_PS_H
#define RINA_SDUP_TTL_PS_H

#include <linux/types.h>

#include "sdup.h"
#include "du.h"
#include "ps-factory.h"

struct sdup_ttl_ps {
	struct ps_base base;

	/* Behavioural policies. */

	int (* sdup_set_lifetime_limit_policy)(struct sdup_ttl_ps *,
					       struct du *);
	int (* sdup_get_lifetime_limit_policy)(struct sdup_ttl_ps *,
					       struct du *);
	int (* sdup_dec_check_lifetime_limit_policy)(struct sdup_ttl_ps *,
						     struct du *);

	/* Reference used to access the SDUP data model. */
	struct sdup_port * dm;

	/* Data private to the policy-set implementation. */
	void *       priv;
};

/*
 * The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary.
 */
int sdup_ttl_ps_publish(struct ps_factory * factory);
int sdup_ttl_ps_unpublish(const char * name);

#endif
