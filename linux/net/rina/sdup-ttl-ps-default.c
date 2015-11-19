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
#include "sdup-ttl-ps-default.h"
#include "debug.h"
#include "policies.h"

struct sdup_ttl_ps_default_data {
	u_int32_t  	initial_ttl_value;
};

int default_sdup_set_lifetime_limit_policy(struct sdup_ttl_ps * ps,
					   struct pdu_ser * pdu,
					   size_t pci_ttl)
{
	unsigned char * data;
	size_t          len;
	size_t		ttl;
	struct sdup_ttl_ps_default_data * priv_data = ps->priv;

	if (!ps || !pdu){
		LOG_ERR("Failed set_lifetime_limit");
		return -1;
	}

	ttl = 0;

	if (pci_ttl > 0)
		ttl = pci_ttl;
	else if (priv_data->initial_ttl_value > 0){
		ttl = priv_data->initial_ttl_value;
	}

	if (priv_data->initial_ttl_value > 0){
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
EXPORT_SYMBOL(default_sdup_set_lifetime_limit_policy);

int default_sdup_get_lifetime_limit_policy(struct sdup_ttl_ps * ps,
					   struct pdu_ser * pdu,
					   size_t * ttl)
{
	unsigned char * data;
	size_t          len;
	struct sdup_ttl_ps_default_data * priv_data = ps->priv;

	if (!ps || !pdu || !ttl){
		LOG_ERR("Failed get_lifetime_limit");
		return -1;
	}

	if (priv_data->initial_ttl_value > 0){
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
EXPORT_SYMBOL(default_sdup_get_lifetime_limit_policy);

int default_sdup_dec_check_lifetime_limit_policy(struct sdup_ttl_ps * ps,
						 struct pdu * pdu)
{
	struct pci * pci;
	size_t ttl;
	struct sdup_ttl_ps_default_data * priv_data = ps->priv;

	if (!ps || !pdu){
		LOG_ERR("Failed dec_check_lifetime_limit");
		return -1;
	}

	if (priv_data->initial_ttl_value > 0){
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
EXPORT_SYMBOL(default_sdup_dec_check_lifetime_limit_policy);

struct ps_base * sdup_ttl_ps_default_create(struct rina_component * component)
{
	struct dup_config_entry * conf;
	struct sdup_comp * sdup_comp;
	struct sdup_ttl_ps * ps;
	struct sdup_port * sdup_port;
	struct sdup_ttl_ps_default_data * data;
	struct policy_parm * parameter;

	sdup_comp = sdup_comp_from_component(component);
	if (!sdup_comp)
		return NULL;

	sdup_port = sdup_comp->parent;
	if (!sdup_port)
		return NULL;

	conf = sdup_port->conf;
	if (!conf)
		return NULL;

	ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
	if (!ps)
		return NULL;

	data = rkzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		rkfree(ps);
		return NULL;
	}

	ps->dm          = sdup_port;
        ps->priv        = data;

        /* Parse parameters from config */
	if (conf->ttl_policy) {
		parameter = policy_param_find(conf->ttl_policy, "initialValue");
		if (!parameter) {
			LOG_ERR("Could not find 'initialValue' in TTL policy");
			rkfree(ps);
			rkfree(data);
			return NULL;
		}

		if (kstrtouint(policy_param_value(parameter),
			       10,
			       &data->initial_ttl_value)) {
			LOG_ERR("Failed to convert TTL string to int");
			rkfree(ps);
			rkfree(data);
			return NULL;
		}

		LOG_DBG("Initial TTL value is %u", data->initial_ttl_value);
	} else {
		LOG_ERR("TTL policy is NULL");
		rkfree(ps);
		rkfree(data);
		return NULL;
	}

	/* SDUP policy functions*/
	ps->sdup_set_lifetime_limit_policy	 = default_sdup_set_lifetime_limit_policy;
	ps->sdup_get_lifetime_limit_policy	 = default_sdup_get_lifetime_limit_policy;
	ps->sdup_dec_check_lifetime_limit_policy = default_sdup_dec_check_lifetime_limit_policy;

	return &ps->base;
}
EXPORT_SYMBOL(sdup_ttl_ps_default_create);

void sdup_ttl_ps_default_destroy(struct ps_base * bps)
{
	struct sdup_ttl_ps_default_data * data;
	struct sdup_ttl_ps *ps;

	ps = container_of(bps, struct sdup_ttl_ps, base);
	data = ps->priv;

	if (bps) {
		if (data)
			rkfree(data);
		rkfree(ps);
	}
}
EXPORT_SYMBOL(sdup_ttl_ps_default_destroy);
