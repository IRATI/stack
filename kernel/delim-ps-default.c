/*
 * Default policy set for Delimiting
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/random.h>

#define RINA_PREFIX "delim-ps-default"

#include "rds/rmem.h"
#include "delim.h"
#include "delim-ps.h"
#include "delim-ps-default.h"
#include "du.h"
#include "logs.h"

/* FIXME: Trivial implementation, does nothing */
int default_delim_fragment(struct delim_ps * ps, struct du * du,
			   struct du_list * du_list)
{
	struct delim * delim;

	delim = ps->dm;
	if (!delim) {
		LOG_ERR("No instance passed, cannot run policy");
		return -1;
	}

	if (du_len(du) > delim->max_fragment_size) {
		LOG_WARN("SDU is too large %d", du_len(du));
	}

	if (add_du_to_list(du_list, du)) {
		LOG_ERR("Problems adding DU to list");
		return -1;
	}

	return 0;
}

struct ps_base * delim_ps_default_create(struct rina_component * component)
{
        struct delim * delim = delim_from_component(component);
        struct delim_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param   = NULL;
        ps->dm                          = delim;
        ps->priv                        = NULL;
        ps->delim_fragment		= default_delim_fragment;

        return &ps->base;
}
EXPORT_SYMBOL(delim_ps_default_create);

void delim_ps_default_destroy(struct ps_base * bps)
{
        struct delim_ps *ps = container_of(bps, struct delim_ps, base);

        if (bps) {
                rkfree(ps);
        }
}
EXPORT_SYMBOL(delim_ps_default_destroy);
