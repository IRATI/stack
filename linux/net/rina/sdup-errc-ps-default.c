/*
 * Default policy set for SDUP Error check
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/crc32.h>

#define RINA_PREFIX "sdup-errc-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "sdup-errc-ps-default.h"
#include "debug.h"

static bool pdu_ser_crc32(struct pdu_ser * pdu,
			  u32 *            crc)
{
	ssize_t         len;
	unsigned char * data;

	ASSERT(pdu_ser_is_ok(pdu));
	ASSERT(crc);

	data = 0;
	len  = 0;

	if (pdu_ser_data_and_length(pdu, &data, &len))
		return -1;

	*crc = crc32_le(0, data, len - sizeof(*crc));
	LOG_DBG("CRC32(%p, %zd) = %X", data, len, *crc);

	return 0;
}

int default_sdup_add_error_check_policy(struct sdup_errc_ps * ps,
					struct pdu_ser * pdu)
{
	u32             crc;
	unsigned char * data;
	ssize_t         len;

	if (!ps || !pdu){
		LOG_ERR("Error check arguments not initialized!");
		return -1;
	}

	if (pdu_ser_tail_grow_gfp(pdu, sizeof(u32))) {
		LOG_ERR("Failed to grow ser PDU");
		return -1;
	}

	crc  = 0;
	data = 0;
	len  = 0;

	if (!pdu_ser_is_ok(pdu))
		return -1;

	if (pdu_ser_crc32(pdu, &crc))
		return -1;

	if (pdu_ser_data_and_length(pdu, &data, &len))
		return -1;

	memcpy(data+len-sizeof(crc), &crc, sizeof(crc));

	return 0;
}
EXPORT_SYMBOL(default_sdup_add_error_check_policy);

int default_sdup_check_error_check_policy(struct sdup_errc_ps * ps,
					  struct pdu_ser * pdu)
{
	u32             crc;
	unsigned char * data;
	ssize_t         len;

	if (!ps || !pdu ){
		LOG_ERR("Error check arguments not initialized!");
		return -1;
	}

	if (!pdu_ser_is_ok(pdu))
		return -1;

	data = 0;
	crc  = 0;
	len  = 0;

	if (pdu_ser_crc32(pdu, &crc))
		return -1;

	if (pdu_ser_data_and_length(pdu, &data, &len))
		return -1;

	if (memcmp(&crc, data+len-sizeof(crc), sizeof(crc)))
		return -1;

	if (pdu_ser_tail_shrink_gfp(pdu, sizeof(u32))) {
		LOG_ERR("Failed to shrink ser PDU");
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(default_sdup_check_error_check_policy);

struct ps_base * sdup_errc_ps_default_create(struct rina_component * component)
{
	struct sdup_comp * sdup_comp;
	struct sdup_errc_ps * ps;

	sdup_comp = sdup_comp_from_component(component);
	if (!sdup_comp)
		return NULL;


	ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
	if (!ps)
		return NULL;

	ps->dm          = sdup_comp->parent;
        ps->priv        = NULL;

	/* SDUP policy functions*/
	ps->sdup_add_error_check_policy		= default_sdup_add_error_check_policy;
	ps->sdup_check_error_check_policy	= default_sdup_check_error_check_policy;

	return &ps->base;
}
EXPORT_SYMBOL(sdup_errc_ps_default_create);

void sdup_errc_ps_default_destroy(struct ps_base * bps)
{
	struct sdup_errc_ps *ps = container_of(bps, struct sdup_errc_ps, base);

	if (bps) {
		rkfree(ps);
	}
}
EXPORT_SYMBOL(sdup_errc_ps_default_destroy);
