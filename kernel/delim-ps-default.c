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

/* Easy case, the PDU will contain a single, complete SDU.
 * Hence the syntax is <SDUDelimiterFlags> <SDUData>
 */
static int fragment_single_full_sdu(struct du * du,
		   	   	    struct du_list * du_list)
{
	char flags;

	/* Grow DU with 1 byte, to add the SDU Flags */
	if (du_head_grow(du,1)) {
		LOG_ERR("Problems growing du");
		du_destroy(du);
		return -1;
	}

	/* The second byte is 0 (no seqnum) 1 (nolenth) 11 (full SDU) */
	flags = 0x07;

	memcpy(du_buffer(du), &flags, 1);

	if (add_du_to_list(du_list, du)) {
		LOG_ERR("Problems adding DU to list");
		du_destroy(du);
		return -1;
	}

	return 0;
}

/* Does not use SDU sequence numbers, assumes that max SDU gap
 * for the flow is either 0 or -1 (don't care). It relies on PDU
 * sequence numbers for in-order delivery.
 */
int default_delim_fragment(struct delim_ps * ps, struct du * du,
			   struct du_list * du_list)
{
	struct delim * delim;

	delim = ps->dm;
	if (!delim) {
		LOG_ERR("No instance passed, cannot run policy");
		du_destroy(du);
		return -1;
	}

	if (du_len(du) > delim->max_fragment_size) {
		/* TODO deal with this more difficult case */
		LOG_WARN("SDU is too large %d", du_len(du));
		/*du_destroy(du);
		return -1;*/
	}

	return fragment_single_full_sdu(du, du_list);
}

/* Easy case, the UDF contains a single, complete SDU.
 * Hence the syntax is <SDUDelimiterFlags> <SDUData>
 */
static int process_single_full_sdu(struct du * du,
		   	   	   struct du_list * du_list)
{
	if (du_head_shrink(du, 1)) {
		LOG_ERR("Error shrinking DU (User Data Field)");
		du_destroy(du);
		return -1;
	}

	if (add_du_to_list(du_list, du)) {
		LOG_ERR("Problems adding DU to list");
		du_destroy(du);
		return -1;
	}

	return 0;
}

/* Does not use SDU sequence numbers, assumes that max SDU gap
 * for the flow is either 0 or -1 (don't care). It relies on PDU
 * sequence numbers for in-order delivery.
 */
int default_delim_process_udf(struct delim_ps * ps, struct du * du,
			      struct du_list * du_list)
{
	struct delim * delim;
	char flags;

	delim = ps->dm;
	if (!delim) {
		LOG_ERR("No instance passed, cannot run policy");
		du_destroy(du);
		return -1;
	}

	memcpy(&flags, du_buffer(du), 1);

	if (flags & 0x08) {
		LOG_ERR("SDU sequence number not supported by this policy");
		du_destroy(du);
		return -1;
	}

	if (flags & 0x04) {
		/* No length flag, the User Data Field contains a single
		 * SDU or fragment
		 */
		if ((flags & 0x02) && (flags & 0x01)) {
			/*UDF contains a full SDU*/
			return process_single_full_sdu(du, du_list);
		} else {
			/* TODO handle this */
			LOG_ERR("Cannot handle this  yet");
			du_destroy(du);
			return -1;
		}
	} else {
		/* TODO handle this */
		LOG_ERR("Cannot handle multiple fragments yet");
		du_destroy(du);
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
        ps->delim_process_udf		= default_delim_process_udf;

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
