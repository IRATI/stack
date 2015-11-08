/*
 * Default policy set for SDUP
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

#define RINA_PREFIX "sdup-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "sdup.h"
#include "sdup-ps.h"
#include "sdup-ps-common.h"

int default_sdup_add_padding_policy(struct sdup_ps * ps,
				    struct pdu_ser * pdu,
				    struct sdup_port_conf * port_conf)
{
	return common_sdup_add_padding_policy(ps, pdu, port_conf);
}

int default_sdup_remove_padding_policy(struct sdup_ps * ps,
				       struct pdu_ser * pdu,
				       struct sdup_port_conf * port_conf)
{
	return common_sdup_remove_padding_policy(ps, pdu, port_conf);
}

int default_sdup_encrypt_policy(struct sdup_ps * ps,
				struct pdu_ser * pdu,
				struct sdup_port_conf * port_conf)
{
	return common_sdup_encrypt_policy(ps, pdu, port_conf);
}

int default_sdup_decrypt_policy(struct sdup_ps * ps,
				struct pdu_ser * pdu,
				struct sdup_port_conf * port_conf)
{
	return common_sdup_decrypt_policy(ps, pdu, port_conf);
}

int default_sdup_set_lifetime_limit_policy(struct sdup_ps * ps,
					   struct pdu_ser * pdu,
					   struct sdup_port_conf * port_conf,
					   size_t ttl)
{
	return common_sdup_set_lifetime_limit_policy(ps, pdu, port_conf, ttl);
}

int default_sdup_get_lifetime_limit_policy(struct sdup_ps * ps,
					   struct pdu_ser * pdu,
					   struct sdup_port_conf * port_conf,
					   size_t * ttl)
{
	return common_sdup_get_lifetime_limit_policy(ps, pdu, port_conf, ttl);
}

int default_sdup_dec_check_lifetime_limit_policy(struct sdup_ps * ps,
						 struct pdu * pdu,
						 struct sdup_port_conf * port_conf)
{
	return common_sdup_dec_check_lifetime_limit_policy(ps, pdu, port_conf);
}

int default_sdup_add_error_check_policy(struct sdup_ps * ps,
					struct pdu_ser * pdu,
					struct sdup_port_conf * port_conf)
{
	return common_sdup_add_error_check_policy(ps, pdu, port_conf);
}

int default_sdup_check_error_check_policy(struct sdup_ps * ps,
					  struct pdu_ser * pdu,
					  struct sdup_port_conf * port_conf)
{
	return common_sdup_check_error_check_policy(ps, pdu, port_conf);
}

int default_sdup_add_hmac(struct sdup_ps * ps,
			  struct pdu_ser * pdu,
			  struct sdup_port_conf * port_conf)
{
	return common_sdup_add_hmac(ps, pdu, port_conf);
}

int default_sdup_verify_hmac(struct sdup_ps * ps,
			     struct pdu_ser * pdu,
			     struct sdup_port_conf * port_conf)
{
	return common_sdup_verify_hmac(ps, pdu, port_conf);
}

int default_sdup_compress(struct sdup_ps * ps,
			  struct pdu_ser * pdu,
			  struct sdup_port_conf * port_conf)
{
	return common_sdup_compress(ps, pdu, port_conf);
}

int default_sdup_decompress(struct sdup_ps * ps,
			    struct pdu_ser * pdu,
			    struct sdup_port_conf * port_conf)
{
	return common_sdup_decompress(ps, pdu, port_conf);
}

static struct ps_base *
sdup_ps_default_create(struct rina_component * component)
{
	struct sdup * sdup = sdup_from_component(component);
	struct sdup_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

	if (!ps) {
		return NULL;
	}

	ps->dm          = sdup;

	/* SDUP policy functions*/
	ps->sdup_add_padding_policy		= default_sdup_add_padding_policy;
	ps->sdup_remove_padding_policy		= default_sdup_remove_padding_policy;
	ps->sdup_encrypt_policy			= default_sdup_encrypt_policy;
	ps->sdup_decrypt_policy			= default_sdup_decrypt_policy;
	ps->sdup_set_lifetime_limit_policy	= default_sdup_set_lifetime_limit_policy;
	ps->sdup_get_lifetime_limit_policy	= default_sdup_get_lifetime_limit_policy;
	ps->sdup_dec_check_lifetime_limit_policy= default_sdup_dec_check_lifetime_limit_policy;
	ps->sdup_add_error_check_policy		= default_sdup_add_error_check_policy;
	ps->sdup_check_error_check_policy	= default_sdup_check_error_check_policy;
	ps->sdup_add_hmac			= default_sdup_add_hmac;
	ps->sdup_verify_hmac			= default_sdup_verify_hmac;
	ps->sdup_compress			= default_sdup_compress;
	ps->sdup_decompress			= default_sdup_decompress;

	return &ps->base;
}

static void
sdup_ps_default_destroy(struct ps_base * bps)
{
	struct sdup_ps *ps = container_of(bps, struct sdup_ps, base);

	if (bps) {
		rkfree(ps);
	}
}

struct ps_factory default_sdup_ps_factory = {
	.owner   = THIS_MODULE,
	.create  = sdup_ps_default_create,
	.destroy = sdup_ps_default_destroy,
};
EXPORT_SYMBOL(default_sdup_ps_factory);
