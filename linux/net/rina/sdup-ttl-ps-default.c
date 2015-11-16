/*
 * Default policy set for SDUP TTL
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

#define RINA_PREFIX "sdup-ttl-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "sdup.h"
#include "sdup-ttl-ps.h"
#include "debug.h"

int default_sdup_set_lifetime_limit_policy(struct sdup_ps * ps,
					   struct pdu_ser * pdu,
					   struct sdup_port_conf * port_conf,
					   size_t pci_ttl)
{
	unsigned char * data;
	size_t          len;
	size_t		ttl;

	if (!ps || !pdu || !port_conf){
		LOG_ERR("Failed set_lifetime_limit");
		return -1;
	}

	ttl = 0;

	if (pci_ttl > 0)
		ttl = pci_ttl;
	else if (port_conf->initial_ttl_value > 0){
		ttl = port_conf->initial_ttl_value;
	}

	if (port_conf->initial_ttl_value > 0){
		if (pdu_ser_head_grow_gfp(GFP_ATOMIC, pdu, sizeof(ttl))) {
			LOG_ERR("Failed to grow ser PDU");
			return -1;
		}

		if (!pdu_ser_is_ok(pdu))
			return -1;

		if (pdu_ser_data_and_length(pdu, &data, &len))
			return -1;

		memcpy(data, &ttl, sizeof(ttl));
		LOG_DBG("SETTTL! %d", (int)ttl);
	}

	return 0;
}

int default_sdup_get_lifetime_limit_policy(struct sdup_ps * ps,
					   struct pdu_ser * pdu,
					   struct sdup_port_conf * port_conf,
					   size_t * ttl)
{
	unsigned char * data;
	size_t          len;

	if (!ps || !pdu || !port_conf || !ttl){
		LOG_ERR("Failed get_lifetime_limit");
		return -1;
	}

	if (port_conf->initial_ttl_value > 0){
		if (pdu_ser_data_and_length(pdu, &data, &len))
			return -1;

		memcpy(ttl, data, sizeof(*ttl));

		if (pdu_ser_head_shrink_gfp(GFP_ATOMIC , pdu, sizeof(*ttl))) {
			LOG_ERR("Failed to shrink ser PDU");
			return -1;
		}
		LOG_DBG("GET_TTL! %d", (int)*ttl);
	}

	return 0;
}

int default_sdup_dec_check_lifetime_limit_policy(struct sdup_ps * ps,
						 struct pdu * pdu,
						 struct sdup_port_conf * port_conf)
{
	struct pci * pci;
	size_t ttl;

	if (!ps || !pdu || !port_conf){
		LOG_ERR("Failed dec_check_lifetime_limit");
		return -1;
	}

	if (port_conf->initial_ttl_value > 0){
		pci = pdu_pci_get_rw(pdu);
		ttl = pci_ttl(pci);

		ttl = ttl-1;
		if (ttl == 0){
			LOG_DBG("TTL dropped to 0, dropping pdu.");
			return -1;
		}
		pci_ttl_set(pci, ttl);
		LOG_DBG("DEC_CHECK_TTL! new ttl: %d", (int)ttl);
	}

	return 0;
}

static struct ps_base *
sdup_ttl_ps_default_create(struct rina_component * component)
{
	struct sdup * sdup = sdup_from_component(component);
	struct sdup_ttl_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

	if (!ps) {
		return NULL;
	}

	ps->dm          = sdup;
        ps->priv        = NULL;

	/* SDUP policy functions*/
	ps->sdup_set_lifetime_limit_policy	 = default_sdup_set_lifetime_limit_policy;
	ps->sdup_get_lifetime_limit_policy	 = default_sdup_get_lifetime_limit_policy;
	ps->sdup_dec_check_lifetime_limit_policy = default_sdup_dec_check_lifetime_limit_policy;

	return &ps->base;
}

static void
sdup_ttl_ps_default_destroy(struct ps_base * bps)
{
	struct sdup_ps *ps = container_of(bps, struct sdup_ttl_ps, base);

	if (bps) {
		rkfree(ps);
	}
}

struct ps_factory default_sdup_ttl_ps_factory = {
	.owner   = THIS_MODULE,
	.create  = sdup_ttl_ps_default_create,
	.destroy = sdup_ttl_ps_default_destroy,
};
EXPORT_SYMBOL(default_sdup_ttl_ps_factory);
