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

struct delim_def_priv {
	struct du_list * pending_dus;
	bool reassembly_in_process;
	int total_length;
};

static struct delim_def_priv * delim_def_priv_create(void)
{
	struct delim_def_priv * priv;

	priv = rkzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return NULL;

	priv->pending_dus = du_list_create();
	priv->reassembly_in_process = false;
	priv->total_length = 0;

	return priv;
}

static void delim_def_priv_destroy(struct delim_def_priv * priv)
{
	if (!priv)
		return;

	if (priv->pending_dus) {
		du_list_destroy(priv->pending_dus, true);
	}

	rkfree(priv);
}

static void delim_def_priv_reset(struct delim_def_priv * priv)
{
	if (!priv)
		return;

	du_list_clear(priv->pending_dus, true);
	priv->reassembly_in_process = false;
	priv->total_length = 0;
}

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

	if (add_du_to_list_ni(du_list, du)) {
		LOG_ERR("Problems adding DU to list");
		du_destroy(du);
		return -1;
	}

	return 0;
}

static int copy_du_fragment_to_list(int length, int offset, char * flags,
				    struct du * du, struct du_list * du_list)
{
	struct du * frag_du;

	frag_du = du_create(length + 1);
	if (!frag_du) {
		LOG_ERR("Problems creating du");
		du_destroy(du);
		return -1;
	}

	memcpy(du_buffer(frag_du), flags, 1);
	memcpy(du_buffer(frag_du) + 1, du_buffer(du) + offset, length);
	frag_du->cfg = du->cfg;

	if (add_du_to_list_ni(du_list, frag_du)) {
		LOG_ERR("Problems adding DU to list");
		du_destroy(frag_du);
		du_destroy(du);
		return -1;
	}

	LOG_DBG("Created fragment of length %d, offset %d and flags %d",
		 length, offset, *flags);

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
	int pending_du_len;
	int length;
	int offset;
	bool first_frag;
	char flags;

	delim = ps->dm;
	if (!delim) {
		LOG_ERR("No instance passed, cannot run policy");
		du_destroy(du);
		return -1;
	}

	pending_du_len = du_len(du);
	if (pending_du_len <= delim->max_fragment_size) {
		return fragment_single_full_sdu(du, du_list);
	}

	first_frag = true;
	offset = 0;
	length = 0;
	while(pending_du_len > 0) {
		if (first_frag) {
			flags = 0x05;
			first_frag = false;
		} else if (pending_du_len <= delim->max_fragment_size) {
			flags = 0x06;
		} else {
			flags = 0x04;
		}

		if (pending_du_len > delim->max_fragment_size) {
			length = delim->max_fragment_size;
		} else {
			length = pending_du_len;
		}

		if (copy_du_fragment_to_list(length, offset,
					     &flags, du, du_list)) {
			return -1;
		}

		offset = offset + length;
		pending_du_len = pending_du_len - length;
	}

	du_destroy(du);

	return 0;
}

int default_delim_num_udfs(struct delim_ps * ps, struct du * du)
{
	struct delim * delim;
	int dulen;
	int num_fragments = 0;

 	delim = ps->dm;
	if (!delim) {
		LOG_ERR("No instance passed, cannot run policy");
		du_destroy(du);
		return -1;
	}

 	dulen = du_len(du);
	while (dulen > 0) {
		num_fragments ++;
		dulen = dulen - delim->max_fragment_size;
	}

 	return num_fragments;
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

	if (add_du_to_list_ni(du_list, du)) {
		LOG_ERR("Problems adding DU to list");
		du_destroy(du);
		return -1;
	}

	return 0;
}

static int append_pending_du(struct delim_def_priv * priv, struct du * du)
{
	priv->total_length = priv->total_length + du_len(du) - 1;

	if (add_du_to_list_ni(priv->pending_dus, du)) {
		LOG_ERR("Problems adding DU to pending DUs list");
		delim_def_priv_reset(priv);
		du_destroy(du);
		return -1;
	}

	LOG_DBG("Appended DU, total length is %d", priv->total_length);

	return 0;
}

static int process_first_fragment(struct delim_def_priv * priv, struct du * du)
{
	if (priv->reassembly_in_process) {
		LOG_WARN("Received a first fragment and "
				"reassembly buffer was not empty");

		delim_def_priv_reset(priv);
	}

	priv->reassembly_in_process = true;

	return append_pending_du(priv, du);
}

static int process_mid_fragment(struct delim_def_priv * priv, struct du * du)
{
	if (!priv->reassembly_in_process) {
		LOG_WARN("Received a mid/last fragment and "
				"reassembly buffer is empty");
		delim_def_priv_reset(priv);
		du_destroy(du);
		return -1;
	}

	return append_pending_du(priv, du);
}

static int process_last_fragment(struct delim_def_priv * priv,
			         struct du * du, struct du_list * du_list)
{
	struct du * frag_sdu;
	struct du_list_item * next_du = NULL;
	int length;
	int offset;

	if (process_mid_fragment(priv, du)) {
		return -1;
	}

	frag_sdu = du_create_ni(priv->total_length);
	if (!frag_sdu) {
		LOG_ERR("Could not create SDU");
		delim_def_priv_reset(priv);
		return -1;
	}

	offset = 0;
	list_for_each_entry(next_du, &(priv->pending_dus->dus), next) {
		length = du_len(next_du->du) - 1;
		memcpy(du_buffer(frag_sdu) + offset,
		       du_buffer(next_du->du) + 1, length);
		offset = offset + length;
	}

	if (add_du_to_list(du_list, frag_sdu)) {
		LOG_ERR("Problems adding DU to list");
		delim_def_priv_reset(priv);
		du_destroy(frag_sdu);
		return -1;
	}

	LOG_DBG("Reassembled SDU with length %d", priv->total_length);

	delim_def_priv_reset(priv);

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
	struct delim_def_priv * priv;
	char * flags;

	delim = ps->dm;
	if (!delim) {
		LOG_ERR("No instance passed, cannot run policy");
		du_destroy(du);
		return -1;
	}

	priv = (struct delim_def_priv *) ps->priv;
	if (!priv) {
		LOG_ERR("No private data structure, cannot run policy");
		du_destroy(du);
		return -1;
	}

	flags = (char *) du_buffer(du);

	LOG_DBG("Received UDF with length %d and flags %d",
		du_len(du) - 1, *flags);

	if (*flags & 0x08) {
		LOG_ERR("SDU sequence number not supported by this policy");
		du_destroy(du);
		return -1;
	}

	if (*flags & 0x04) {
		/* No length flag, the User Data Field contains a single
		 * SDU or fragment
		 */
		if ((*flags & 0x02) && (*flags & 0x01)) {
			/* UDF contains a full SDU */
			return process_single_full_sdu(du, du_list);
		} else if (*flags & 0x01) {
			/* UDF contains the first fragment */
			return process_first_fragment(priv, du);
		} else if (*flags & 0x02) {
			/* UDF contains the last fragment */
			return process_last_fragment(priv, du, du_list);
		} else {
			/* UDF contains a middle fragment */
			process_mid_fragment(priv, du);
		}
	} else {
		/* We don't handle this */
		LOG_ERR("Cannot handle SDUs + fragment combinations yet");
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
        ps->priv                        = delim_def_priv_create();
        if (!ps->priv) {
        	return NULL;
        }

        ps->delim_fragment		= default_delim_fragment;
        ps->delim_num_udfs	        = default_delim_num_udfs;
        ps->delim_process_udf		= default_delim_process_udf;

        return &ps->base;
}
EXPORT_SYMBOL(delim_ps_default_create);

void delim_ps_default_destroy(struct ps_base * bps)
{
        struct delim_ps * ps = container_of(bps, struct delim_ps, base);

        if (!ps)
        	return;

        if (ps->priv) {
        	delim_def_priv_destroy(ps->priv);
        }

        rkfree(ps);
}
EXPORT_SYMBOL(delim_ps_default_destroy);
