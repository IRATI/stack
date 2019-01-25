/*
 * Utilities to serialize and deserialize
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#include "irati/serdes-utils.h"
#include "irati/kernel-msg.h"

#define RINA_PREFIX "serdes-utils"

#ifdef __KERNEL__

#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "rds/rmem.h"
#include "rds/rstr.h"
#include "common.h"
#include "buffer.h"

#define COMMON_ALLOC(_sz, _sl)      rkzalloc(_sz, _sl ? GFP_KERNEL : GFP_ATOMIC)
#define COMMON_FREE(_p)             rkfree(_p)
#define COMMON_STRDUP(_s, _sl)      rkstrdup_gfp(_s, _sl ? GFP_KERNEL : GFP_ATOMIC)
#define COMMON_EXPORT(_n)           EXPORT_SYMBOL(_n)
#define COMMON_STATIC

#else /* user-space */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define COMMON_ALLOC(_sz, _unused)  malloc(_sz)
#define COMMON_FREE(_p)             free(_p)
#define COMMON_STRDUP(_s, _unused)  strdup(_s)
#define COMMON_EXPORT(_n)
#define COMMON_STATIC               static

#endif

/* Serialize a numeric variable _v of type _t. */
#define serialize_obj(_p, _t, _v)       \
        do {                            \
            *((_t *)_p) = _v;           \
            _p += sizeof(_t);           \
        } while (0)

/* Deserialize a numeric variable of type _t from _p into _r. */
#define deserialize_obj(_p, _t, _r)     \
        do {                            \
            *(_r) = *((_t *)_p);        \
            _p += sizeof(_t);           \
        } while (0)

int rina_sername_valid(const char *str)
{
	const char *orig_str = str;
	int cnt = 0;

	if (!str || strlen(str) == 0) {
		return 0;
	}

	while (*str != '\0') {
		if (*str == ':') {
			if (++ cnt > 3) {
				return 0;
			}
		}
		str ++;
	}

	return (*orig_str == ':') ? 0 : 1;
}
COMMON_EXPORT(rina_sername_valid);

/* Size of a serialized string, not including the storage for the size
 * field itself. */
static unsigned int string_prlen(const char *s)
{
	unsigned int slen;

	slen = s ? strlen(s) : 0;

	return slen > 65535 ? 65535 : slen;
}

/* Size of a serialized IRATI name. */
unsigned int
rina_name_serlen(const struct name *name)
{
	unsigned int ret = 4 * sizeof(uint16_t);

	if (!name) {
		return ret;
	}

	return ret + string_prlen(name->process_name)
			+ string_prlen(name->process_instance)
			+ string_prlen(name->entity_name)
			+ string_prlen(name->entity_instance);
}

/* Serialize a C string. */
void serialize_string(void **pptr, const char *s)
{
	uint16_t slen;

	slen = string_prlen(s);
	serialize_obj(*pptr, uint16_t, slen);

	memcpy(*pptr, s, slen);
	*pptr += slen;
}

/* Deserialize a C string. */
int deserialize_string(const void **pptr, char **s)
{
	uint16_t slen;

	deserialize_obj(*pptr, uint16_t, &slen);

	*s = COMMON_ALLOC(slen + 1, 1);
	if (!(*s)) {
		return -1;
	}

	memcpy(*s, *pptr, slen);
	(*s)[slen] = '\0';
	*pptr += slen;

	return 0;
}

/* Size of a serialized buffer, not including the storage for the size
 * field itself. */
static unsigned int buffer_prlen(const struct buffer *b)
{
	if (!b)
		return 0;
	else
		return b->size;
}

void serialize_buffer(void **pptr, const struct buffer * b)
{
	if (!b) {
		serialize_obj(*pptr, uint32_t, 0);
		return;
	}

	serialize_obj(*pptr, uint32_t, b->size);

	memcpy(*pptr, b->data, b->size);
	*pptr += b->size;
}

int deserialize_buffer(const void **pptr, struct buffer **b)
{
	uint32_t blen;

	deserialize_obj(*pptr, uint32_t, &blen);

	if (blen) {
		*b = COMMON_ALLOC(sizeof(struct buffer), 1);
		if (!(*b)) {
			return -1;
		}

		(*b)->size = blen;
		(*b)->data = COMMON_ALLOC(blen, 1);
		if (!(*b)->data) {
			return -1;
		}

		memcpy((*b)->data, *pptr, blen);
		*pptr += blen;
	} else {
		*b = NULL;
	}

	return 0;
}

/* Serialize a RINA name. */
void serialize_rina_name(void **pptr, const struct name *name)
{
	if (!name)
		return;

	serialize_string(pptr, name->process_name);
	serialize_string(pptr, name->process_instance);
	serialize_string(pptr, name->entity_name);
	serialize_string(pptr, name->entity_instance);
}

/* Deserialize a RINA name. */
int deserialize_rina_name(const void **pptr, struct name ** name)
{
	int ret;

	*name = rina_name_create();
	if (!*name)
		return -1;

	ret = deserialize_string(pptr, &(*name)->process_name);

	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &(*name)->process_instance);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &(*name)->entity_name);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &(*name)->entity_instance);

	return ret;
}

void rina_name_free(struct name *name)
{
	if (!name) {
		return;
	}

	if (name->process_name) {
		COMMON_FREE(name->process_name);
		name->process_name = NULL;
	}

	if (name->process_instance) {
		COMMON_FREE(name->process_instance);
		name->process_instance = NULL;
	}

	if (name->entity_name) {
		COMMON_FREE(name->entity_name);
		name->entity_name = NULL;
	}

	if (name->entity_instance) {
		COMMON_FREE(name->entity_instance);
		name->entity_instance = NULL;
	}

	COMMON_FREE(name);
}
COMMON_EXPORT(rina_name_free);

void rina_name_move(struct name *dst, struct name *src)
{
	if (!dst || !src) {
		return;
	}

	dst->process_name = src->process_name;
	src->process_name = NULL;

	dst->process_instance = src->process_instance;
	src->process_instance = NULL;

	dst->entity_name = src->entity_name;
	src->entity_name = NULL;

	dst->entity_instance = src->entity_instance;
	src->entity_instance = NULL;
}
COMMON_EXPORT(rina_name_move);

int rina_name_copy(struct name *dst, const struct name *src)
{
	if (!dst || !src) {
		return 0;
	}

#if 0
	rina_name_free(dst);
#endif

	dst->process_name = src->process_name ?
			COMMON_STRDUP(src->process_name, 1) : NULL;
	dst->process_instance = src->process_instance ?
			COMMON_STRDUP(src->process_instance, 1) : NULL;
	dst->entity_name = src->entity_name ?
			COMMON_STRDUP(src->entity_name, 1) : NULL;
	dst->entity_instance = src->entity_instance ?
			COMMON_STRDUP(src->entity_instance, 1) : NULL;

	return 0;
}
COMMON_EXPORT(rina_name_copy);

COMMON_STATIC char *
__rina_name_to_string(const struct name *name, int maysleep)
{
	char *str = NULL;
	char *cur;
	unsigned int apn_len;
	unsigned int api_len;
	unsigned int aen_len;
	unsigned int aei_len;

	if (!name) {
		return NULL;
	}

	apn_len = name->process_name ? strlen(name->process_name) : 0;
	api_len = name->process_instance ? strlen(name->process_instance) : 0;
	aen_len = name->entity_name ? strlen(name->entity_name) : 0;
	aei_len = name->entity_instance ? strlen(name->entity_instance) : 0;

	str = cur = COMMON_ALLOC(apn_len + 1 + api_len + 1 +
			aen_len + 1 + aei_len + 1, maysleep);
	if (!str) {
		return NULL;
	}

	memcpy(cur, name->process_name, apn_len);
	cur += apn_len;

	*cur = '|';
	cur++;

	memcpy(cur, name->process_instance, api_len);
	cur += api_len;

	*cur = '|';
	cur++;

	memcpy(cur, name->entity_name, aen_len);
	cur += aen_len;

	*cur = '|';
	cur++;

	memcpy(cur, name->entity_instance, aei_len);
	cur += aei_len;

	*cur = '\0';

	return str;
}
COMMON_EXPORT(__rina_name_to_string);

char * rina_name_to_string(const struct name *name)
{
    return __rina_name_to_string(name, 1);
}
COMMON_EXPORT(rina_name_to_string);

COMMON_STATIC int
__rina_name_fill(struct name *name, const char *apn,
		const char *api, const char *aen, const char *aei,
		int maysleep)
{
	if (!name) {
		return -1;
	}

	name->process_name = (apn && strlen(apn)) ?
			COMMON_STRDUP(apn, maysleep) : NULL;
	name->process_instance = (api && strlen(api)) ?
			COMMON_STRDUP(api, maysleep) : NULL;
	name->entity_name = (aen && strlen(aen)) ?
			COMMON_STRDUP(aen, maysleep) : NULL;
	name->entity_instance = (aei && strlen(aei)) ?
			COMMON_STRDUP(aei, maysleep) : NULL;

	if ((apn && strlen(apn) && !name->process_name) ||
			(api && strlen(api) && !name->process_instance) ||
			(aen && strlen(aen) && !name->entity_name) ||
			(aei && strlen(aei) && !name->entity_instance)) {
		rina_name_free(name);
		return -1;
	}

	return 0;
}
COMMON_EXPORT(__rina_name_fill);

int
rina_name_fill(struct name *name, const char *apn,
		const char *api, const char *aen, const char *aei)
{
	return __rina_name_fill(name, apn, api, aen, aei, 1);
}
COMMON_EXPORT(rina_name_fill);

COMMON_STATIC int
__rina_name_from_string(const char *str, struct name *name, int maysleep)
{
	char *apn, *api, *aen, *aei;
	char *strc = COMMON_STRDUP(str, maysleep);
	char *strc_orig = strc;
	char **strp = &strc;

	memset(name, 0, sizeof(*name));

	if (!strc) {
		return -1;
	}

	apn = strsep(strp, "|");
	api = strsep(strp, "|");
	aen = strsep(strp, "|");
	aei = strsep(strp, "|");

	if (!apn) {
		/* The ':' are not necessary if some of the api, aen, aei
		 * are not present. */
		COMMON_FREE(strc_orig);
		return -1;
	}

	__rina_name_fill(name, apn, api, aen, aei, maysleep);
	COMMON_FREE(strc_orig);

	return 0;
}
COMMON_EXPORT(__rina_name_from_string);

int
rina_name_from_string(const char *str, struct name *name)
{
	return __rina_name_from_string(str, name, 1);
}

int
rina_name_cmp(const struct name *one, const struct name *two)
{
	if (!one || !two) {
		return !(one == two);
	}

	if (!!one->process_name ^ !!two->process_name) {
		return -1;
	}
	if (one->process_name &&
			strcmp(one->process_name, two->process_name)) {
		return -1;
	}

	if (!!one->process_instance ^ !!two->process_instance) {
		return -1;
	}
	if (one->process_instance &&
			strcmp(one->process_instance, two->process_instance)) {
		return -1;
	}

	if (!!one->entity_name ^ !!two->entity_name) {
		return -1;
	}
	if (one->entity_name && strcmp(one->entity_name, two->entity_name)) {
		return -1;
	}

	if (!!one->entity_instance ^ !!two->entity_instance) {
		return -1;
	}
	if (one->entity_instance &&
			strcmp(one->entity_instance, two->entity_instance)) {
		return -1;
	}

	return 0;
}
COMMON_EXPORT(rina_name_cmp);

int
rina_name_valid(const struct name *name)
{
	if (!name || !name->process_name || strlen(name->process_name) == 0) {
		return 0;
	}

	return 1;
}
COMMON_EXPORT(rina_name_valid);

struct name * rina_name_create()
{
	struct name * result;

	result = COMMON_ALLOC(sizeof(struct name), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct name));

	return result;
}
COMMON_EXPORT(rina_name_create);

int flow_spec_serlen(const struct flow_spec * fspec)
{
	if (!fspec) return 0;

	return 8 * sizeof(uint32_t) + sizeof(int32_t)
		+ sizeof(uint16_t) + 3* sizeof(bool);
}

void serialize_flow_spec(void **pptr, const struct flow_spec *fspec)
{
	if (!fspec) return;

	serialize_obj(*pptr, uint32_t, fspec->average_bandwidth);
	serialize_obj(*pptr, uint32_t, fspec->average_sdu_bandwidth);
	serialize_obj(*pptr, uint32_t, fspec->delay);
	serialize_obj(*pptr, uint32_t, fspec->jitter);
	serialize_obj(*pptr, uint16_t, fspec->loss);
	serialize_obj(*pptr, int32_t, fspec->max_allowable_gap);
	serialize_obj(*pptr, uint32_t, fspec->max_sdu_size);
	serialize_obj(*pptr, bool, fspec->ordered_delivery);
	serialize_obj(*pptr, bool, fspec->partial_delivery);
	serialize_obj(*pptr, uint32_t, fspec->peak_bandwidth_duration);
	serialize_obj(*pptr, uint32_t, fspec->peak_sdu_bandwidth_duration);
	serialize_obj(*pptr, int32_t, fspec->undetected_bit_error_rate);
	serialize_obj(*pptr, bool, fspec->msg_boundaries);
}

int deserialize_flow_spec(const void **pptr, struct flow_spec ** fspec)
{
	*fspec = rina_fspec_create();
	if (!*fspec)
		return -1;

	deserialize_obj(*pptr, uint32_t, &(*fspec)->average_bandwidth);
	deserialize_obj(*pptr, uint32_t, &(*fspec)->average_sdu_bandwidth);
	deserialize_obj(*pptr, uint32_t, &(*fspec)->delay);
	deserialize_obj(*pptr, uint32_t, &(*fspec)->jitter);
	deserialize_obj(*pptr, uint16_t, &(*fspec)->loss);
	deserialize_obj(*pptr, int32_t, &(*fspec)->max_allowable_gap);
	deserialize_obj(*pptr, uint32_t, &(*fspec)->max_sdu_size);
	deserialize_obj(*pptr, bool, &(*fspec)->ordered_delivery);
	deserialize_obj(*pptr, bool, &(*fspec)->partial_delivery);
	deserialize_obj(*pptr, uint32_t, &(*fspec)->peak_bandwidth_duration);
	deserialize_obj(*pptr, uint32_t, &(*fspec)->peak_sdu_bandwidth_duration);
	deserialize_obj(*pptr, int32_t, &(*fspec)->undetected_bit_error_rate);
	deserialize_obj(*pptr, bool, &(*fspec)->msg_boundaries);

	return 0;
}

void flow_spec_free(struct flow_spec * fspec)
{
	if (!fspec)
		return;

	COMMON_FREE(fspec);
}

struct flow_spec * rina_fspec_create()
{
	struct flow_spec * result;

	result = COMMON_ALLOC(sizeof(struct flow_spec), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct flow_spec));
	return result;
}

int policy_parm_serlen(const struct policy_parm * prm)
{
	if (!prm) return 0;

	return 2 * sizeof(uint16_t) + string_prlen(prm->name)
			+ string_prlen(prm->value);
}

void serialize_policy_parm(void **pptr, const struct policy_parm *prm)
{
	if (!prm) return;

	serialize_string(pptr, prm->name);
	serialize_string(pptr, prm->value);
}

int deserialize_policy_parm(const void **pptr, struct policy_parm *prm)
{
	int ret;

	ret = deserialize_string(pptr, &prm->name);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &prm->value);

	return ret;
}

void policy_parm_free(struct policy_parm * prm)
{
	if (!prm) {
		return;
	}

	if (prm->name) {
		COMMON_FREE(prm->name);
		prm->name = NULL;
	}

	if (prm->value) {
		COMMON_FREE(prm->value);
		prm->value = NULL;
	}

	COMMON_FREE(prm);
}

struct policy_parm * policy_parm_create()
{
	struct policy_parm * result;

	result = COMMON_ALLOC(sizeof(struct policy_parm), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct policy_parm));
	INIT_LIST_HEAD(&result->next);

	return result;
}

int policy_serlen(const struct policy * policy)
{
	struct policy_parm * pos;
	int ret;

	if (!policy) return 0;

	ret = 2 * sizeof(uint16_t) + string_prlen(policy->name)
		  + string_prlen(policy->version)
		  + sizeof(uint16_t);

        list_for_each_entry(pos, &(policy->params), next) {
                ret = ret + policy_parm_serlen(pos);
        }

        return ret;
}

void serialize_policy(void **pptr, const struct policy *policy)
{
	struct policy_parm * pos;
	uint16_t num_parms;

	if (!policy) return;

	serialize_string(pptr, policy->name);
	serialize_string(pptr, policy->version);

	num_parms = 0;
        list_for_each_entry(pos, &(policy->params), next) {
                num_parms ++;
        }

        serialize_obj(*pptr, uint16_t, num_parms);

        list_for_each_entry(pos, &(policy->params), next) {
        	serialize_policy_parm(pptr, pos);
        }
}

int deserialize_policy(const void **pptr, struct policy *policy)
{
	int ret;
	uint16_t num_attrs;
	struct policy_parm * pos;
	int i;

	ret = deserialize_string(pptr, &policy->name);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &policy->version);
	if (ret) {
		return ret;
	}

	deserialize_obj(*pptr, uint16_t, &num_attrs);
	for(i = 0; i < num_attrs; i++) {
		pos = policy_parm_create();
		if (!pos) {
			return -1;
		}

		ret = deserialize_policy_parm(pptr, pos);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &policy->params);
	}

	return ret;
}

void policy_free(struct policy * policy)
{
	struct policy_parm * pos, * npos;

	if (!policy)
		return;

	if (policy->name) {
		COMMON_FREE(policy->name);
		policy->name = NULL;
	}

	if (policy->version) {
		COMMON_FREE(policy->version);
		policy->version = NULL;
	}

	list_for_each_entry_safe(pos, npos, &policy->params, next) {
		list_del(&pos->next);
		policy_parm_free(pos);
	}

	COMMON_FREE(policy);
}

struct policy * policy_create()
{
	struct policy * result;

	result = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct policy));
	INIT_LIST_HEAD(&result->params);

	return result;
}

int dtp_config_serlen(const struct dtp_config * dtp_config)
{
	if (!dtp_config) return 0;

	return 4 * sizeof(bool) + sizeof(int) + sizeof(timeout_t)
	         + sizeof(seq_num_t) + policy_serlen(dtp_config->dtp_ps);
}

void serialize_dtp_config(void **pptr, const struct dtp_config *dtp_config)
{
	if (!dtp_config) return;

	serialize_obj(*pptr, bool, dtp_config->dtcp_present);
	serialize_obj(*pptr, int, dtp_config->seq_num_ro_th);
	serialize_obj(*pptr, timeout_t, dtp_config->initial_a_timer);
	serialize_obj(*pptr, bool, dtp_config->partial_delivery);
	serialize_obj(*pptr, bool, dtp_config->incomplete_delivery);
	serialize_obj(*pptr, bool, dtp_config->in_order_delivery);
	serialize_obj(*pptr, seq_num_t, dtp_config->max_sdu_gap);
	serialize_policy(pptr, dtp_config->dtp_ps);
}

int deserialize_dtp_config(const void **pptr, struct dtp_config ** dtp_config)
{
	*dtp_config = dtp_config_create();
	if (!*dtp_config)
		return -1;

	deserialize_obj(*pptr, bool, &(*dtp_config)->dtcp_present);
	deserialize_obj(*pptr, int, &(*dtp_config)->seq_num_ro_th);
	deserialize_obj(*pptr, timeout_t, &(*dtp_config)->initial_a_timer);
	deserialize_obj(*pptr, bool, &(*dtp_config)->partial_delivery);
	deserialize_obj(*pptr, bool, &(*dtp_config)->incomplete_delivery);
	deserialize_obj(*pptr, bool, &(*dtp_config)->in_order_delivery);
	deserialize_obj(*pptr, seq_num_t, &(*dtp_config)->max_sdu_gap);

	(*dtp_config)->dtp_ps = policy_create();
	if (!(*dtp_config)->dtp_ps) {
		return -1;
	}

	return deserialize_policy(pptr, (*dtp_config)->dtp_ps);
}

void dtp_config_free(struct dtp_config * dtp_config)
{
	if (!dtp_config)
		return;

	if (dtp_config->dtp_ps) {
		policy_free(dtp_config->dtp_ps);
		dtp_config->dtp_ps = 0;
	}

	COMMON_FREE(dtp_config);
}

struct dtp_config * dtp_config_create()
{
	struct dtp_config * result;

	result = COMMON_ALLOC(sizeof(struct dtp_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct dtp_config));

	return result;
}

int window_fctrl_config_serlen(const struct window_fctrl_config * wfc)
{
	if (!wfc) return 0;

	return 2 * sizeof(uint32_t) + policy_serlen(wfc->rcvr_flow_control)
				    + policy_serlen(wfc->tx_control);
}

void serialize_window_fctrl_config(void **pptr,
				   const struct window_fctrl_config *wfc)
{
	if (!wfc) return;

	serialize_obj(*pptr, uint32_t, wfc->initial_credit);
	serialize_obj(*pptr, uint32_t, wfc->max_closed_winq_length);
	serialize_policy(pptr, wfc->rcvr_flow_control);
	serialize_policy(pptr, wfc->tx_control);
}

int deserialize_window_fctrl_config(const void **pptr,
				    struct window_fctrl_config *wfc)
{
	int ret;

	deserialize_obj(*pptr, uint32_t, &wfc->initial_credit);
	deserialize_obj(*pptr, uint32_t, &wfc->max_closed_winq_length);

	wfc->rcvr_flow_control = policy_create();
	if (!wfc->rcvr_flow_control) {
		return -1;
	}

	ret = deserialize_policy(pptr, wfc->rcvr_flow_control);
	if (ret) {
		return ret;
	}

	wfc->tx_control = policy_create();
	if (!wfc->tx_control) {
		return -1;
	}

	return deserialize_policy(pptr, wfc->tx_control);
}

void window_fctrl_config_free(struct window_fctrl_config * wfc)
{
	if (!wfc)
		return;

	if (wfc->rcvr_flow_control) {
		policy_free(wfc->rcvr_flow_control);
		wfc->rcvr_flow_control = 0;
	}

	if (wfc->tx_control) {
		policy_free(wfc->tx_control);
		wfc->tx_control = 0;
	}

	COMMON_FREE(wfc);
}

struct window_fctrl_config * window_fctrl_config_create()
{
	struct window_fctrl_config * result;

	result = COMMON_ALLOC(sizeof(struct window_fctrl_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct window_fctrl_config));

	return result;
}

int rate_fctrl_config_serlen(const struct rate_fctrl_config * rfc)
{
	if (!rfc) return 0;

	return 2 * sizeof(uint32_t) + policy_serlen(rfc->rate_reduction)
                 + policy_serlen(rfc->no_rate_slow_down)
		 + policy_serlen(rfc->no_override_default_peak);
}

void serialize_rate_fctrl_config(void **pptr, const struct rate_fctrl_config *rfc)
{
	if (!rfc) return;

	serialize_obj(*pptr, uint32_t, rfc->sending_rate);
	serialize_obj(*pptr, uint32_t, rfc->time_period);
	serialize_policy(pptr, rfc->no_override_default_peak);
	serialize_policy(pptr, rfc->no_rate_slow_down);
	serialize_policy(pptr, rfc->rate_reduction);
}

int deserialize_rate_fctrl_config(const void **pptr, struct rate_fctrl_config *rfc)
{
	int ret;

	deserialize_obj(*pptr, uint32_t, &rfc->sending_rate);
	deserialize_obj(*pptr, uint32_t, &rfc->time_period);

	rfc->no_override_default_peak = policy_create();
	if (!rfc->no_override_default_peak) {
		return -1;
	}

	ret = deserialize_policy(pptr, rfc->no_override_default_peak);
	if (ret) {
		return ret;
	}

	rfc->no_rate_slow_down = policy_create();
	if (!rfc->no_rate_slow_down) {
		return -1;
	}

	ret = deserialize_policy(pptr, rfc->no_rate_slow_down);
	if (ret) {
		return ret;
	}

	rfc->rate_reduction = policy_create();
	if (!rfc->rate_reduction) {
		return -1;
	}

	return deserialize_policy(pptr, rfc->rate_reduction);
}

void rate_fctrl_config_free(struct rate_fctrl_config * rfc)
{
	if (!rfc)
		return;

	if (rfc->no_override_default_peak) {
		policy_free(rfc->no_override_default_peak);
		rfc->no_override_default_peak = 0;
	}

	if (rfc->no_rate_slow_down) {
		policy_free(rfc->no_rate_slow_down);
		rfc->no_rate_slow_down = 0;
	}

	if (rfc->rate_reduction) {
		policy_free(rfc->rate_reduction);
		rfc->rate_reduction = 0;
	}

	COMMON_FREE(rfc);
}

struct rate_fctrl_config * rate_fctrl_config_create()
{
	struct rate_fctrl_config * result;

	result = COMMON_ALLOC(sizeof(struct rate_fctrl_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct rate_fctrl_config));

	return result;
}

int dtcp_fctrl_config_serlen(const struct dtcp_fctrl_config * dfc)
{
	int ret;

	if (!dfc) return 0;

	ret = 6 * sizeof(uint32_t) + 2 * sizeof(bool) +
		 + policy_serlen(dfc->closed_window)
                 + policy_serlen(dfc->receiving_flow_control)
		 + policy_serlen(dfc->reconcile_flow_conflict)
		 + policy_serlen(dfc->flow_control_overrun);

	if (dfc->window_based_fctrl)
		ret = ret + window_fctrl_config_serlen(dfc->wfctrl_cfg);

	if (dfc->rate_based_fctrl)
		ret = ret + rate_fctrl_config_serlen(dfc->rfctrl_cfg);

	return ret;
}

void serialize_dtcp_fctrl_config(void **pptr, const struct dtcp_fctrl_config *dfc)
{
	if (!dfc) return;

	serialize_obj(*pptr, uint32_t, dfc->rcvd_buffers_th);
	serialize_obj(*pptr, uint32_t, dfc->rcvd_bytes_percent_th);
	serialize_obj(*pptr, uint32_t, dfc->rcvd_bytes_th);
	serialize_obj(*pptr, uint32_t, dfc->sent_buffers_th);
	serialize_obj(*pptr, uint32_t, dfc->sent_bytes_percent_th);
	serialize_obj(*pptr, uint32_t, dfc->sent_bytes_th);
	serialize_obj(*pptr, bool, dfc->window_based_fctrl);
	serialize_obj(*pptr, bool, dfc->rate_based_fctrl);
	serialize_policy(pptr, dfc->closed_window);
	serialize_policy(pptr, dfc->receiving_flow_control);
	serialize_policy(pptr, dfc->reconcile_flow_conflict);
	serialize_policy(pptr, dfc->flow_control_overrun);

	if (dfc->window_based_fctrl)
		serialize_window_fctrl_config(pptr, dfc->wfctrl_cfg);

	if (dfc->rate_based_fctrl)
		serialize_rate_fctrl_config(pptr, dfc->rfctrl_cfg);
}

int deserialize_dtcp_fctrl_config(const void **pptr, struct dtcp_fctrl_config *dfc)
{
	int ret;

	deserialize_obj(*pptr, uint32_t, &dfc->rcvd_buffers_th);
	deserialize_obj(*pptr, uint32_t, &dfc->rcvd_bytes_percent_th);
	deserialize_obj(*pptr, uint32_t, &dfc->rcvd_bytes_th);
	deserialize_obj(*pptr, uint32_t, &dfc->sent_buffers_th);
	deserialize_obj(*pptr, uint32_t, &dfc->sent_bytes_percent_th);
	deserialize_obj(*pptr, uint32_t, &dfc->sent_bytes_th);
	deserialize_obj(*pptr, bool, &dfc->window_based_fctrl);
	deserialize_obj(*pptr, bool, &dfc->rate_based_fctrl);

	dfc->closed_window = policy_create();
	if (!dfc->closed_window) {
		return -1;
	}

	ret = deserialize_policy(pptr, dfc->closed_window);
	if (ret) {
		return ret;
	}

	dfc->flow_control_overrun = policy_create();
	if (!dfc->flow_control_overrun) {
		return -1;
	}

	ret = deserialize_policy(pptr, dfc->flow_control_overrun);
	if (ret) {
		return ret;
	}

	dfc->receiving_flow_control = policy_create();
	if (!dfc->receiving_flow_control) {
		return -1;
	}

	ret = deserialize_policy(pptr, dfc->receiving_flow_control);
	if (ret) {
		return ret;
	}

	dfc->reconcile_flow_conflict = policy_create();
	if (!dfc->reconcile_flow_conflict) {
		return -1;
	}

	ret = deserialize_policy(pptr, dfc->reconcile_flow_conflict);
	if (ret) {
		return ret;
	}

	if (dfc->window_based_fctrl) {
		dfc->wfctrl_cfg = window_fctrl_config_create();
		if (!dfc->wfctrl_cfg) {
			return -1;
		}

		ret = deserialize_window_fctrl_config(pptr, dfc->wfctrl_cfg);
		if (ret) {
			return ret;
		}
	}

	if (dfc->rate_based_fctrl) {
		dfc->rfctrl_cfg = rate_fctrl_config_create();
		if (!dfc->rfctrl_cfg) {
			return -1;
		}

		ret = deserialize_rate_fctrl_config(pptr, dfc->rfctrl_cfg);
		if (ret) {
			return ret;
		}
	}

	return ret;
}

void dtcp_fctrl_config_free(struct dtcp_fctrl_config * dfc)
{
	if (!dfc)
		return;

	if (dfc->closed_window) {
		policy_free(dfc->closed_window);
		dfc->closed_window = 0;
	}

	if (dfc->receiving_flow_control) {
		policy_free(dfc->receiving_flow_control);
		dfc->receiving_flow_control = 0;
	}

	if (dfc->reconcile_flow_conflict) {
		policy_free(dfc->reconcile_flow_conflict);
		dfc->reconcile_flow_conflict = 0;
	}

	if (dfc->flow_control_overrun) {
		policy_free(dfc->flow_control_overrun);
		dfc->flow_control_overrun = 0;
	}

	if (dfc->wfctrl_cfg) {
		window_fctrl_config_free(dfc->wfctrl_cfg);
		dfc->wfctrl_cfg = 0;
	}

	if (dfc->rfctrl_cfg) {
		rate_fctrl_config_free(dfc->rfctrl_cfg);
		dfc->rfctrl_cfg = 0;
	}

	COMMON_FREE(dfc);
}

struct dtcp_fctrl_config * dtcp_fctrl_config_create()
{
	struct dtcp_fctrl_config * result;

	result = COMMON_ALLOC(sizeof(struct dtcp_fctrl_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct dtcp_fctrl_config));

	return result;
}

int dtcp_rxctrl_config_serlen(const struct dtcp_rxctrl_config * rxfc)
{
	if (!rxfc) return 0;

	return 3 * sizeof(uint32_t)
		 + policy_serlen(rxfc->rcvr_ack)
                 + policy_serlen(rxfc->rcvr_control_ack)
		 + policy_serlen(rxfc->receiving_ack_list)
		 + policy_serlen(rxfc->retransmission_timer_expiry)
		 + policy_serlen(rxfc->sender_ack)
		 + policy_serlen(rxfc->sending_ack);
}

void serialize_dtcp_rxctrl_config(void **pptr, const struct dtcp_rxctrl_config *rxfc)
{
	if (!rxfc) return;

	serialize_obj(*pptr, uint32_t, rxfc->data_retransmit_max);
	serialize_obj(*pptr, uint32_t, rxfc->initial_tr);
	serialize_obj(*pptr, uint32_t, rxfc->max_time_retry);
	serialize_policy(pptr, rxfc->rcvr_ack);
	serialize_policy(pptr, rxfc->rcvr_control_ack);
	serialize_policy(pptr, rxfc->receiving_ack_list);
	serialize_policy(pptr, rxfc->retransmission_timer_expiry);
	serialize_policy(pptr, rxfc->sender_ack);
	serialize_policy(pptr, rxfc->sending_ack);
}

int deserialize_dtcp_rxctrl_config(const void **pptr, struct dtcp_rxctrl_config *rxfc)
{
	int ret;

	deserialize_obj(*pptr, uint32_t, &rxfc->data_retransmit_max);
	deserialize_obj(*pptr, uint32_t, &rxfc->initial_tr);
	deserialize_obj(*pptr, uint32_t, &rxfc->max_time_retry);

	rxfc->rcvr_ack = policy_create();
	if (!rxfc->rcvr_ack) {
		return -1;
	}

	ret = deserialize_policy(pptr, rxfc->rcvr_ack);
	if (ret) {
		return ret;
	}

	rxfc->rcvr_control_ack = policy_create();
	if (!rxfc->rcvr_control_ack) {
		return -1;
	}

	ret = deserialize_policy(pptr, rxfc->rcvr_control_ack);
	if (ret) {
		return ret;
	}

	rxfc->receiving_ack_list = policy_create();
	if (!rxfc->receiving_ack_list) {
		return -1;
	}

	ret = deserialize_policy(pptr, rxfc->receiving_ack_list);
	if (ret) {
		return ret;
	}

	rxfc->retransmission_timer_expiry = policy_create();
	if (!rxfc->retransmission_timer_expiry) {
		return -1;
	}

	ret = deserialize_policy(pptr, rxfc->retransmission_timer_expiry);
	if (ret) {
		return ret;
	}

	rxfc->sender_ack = policy_create();
	if (!rxfc->sender_ack) {
		return -1;
	}

	ret = deserialize_policy(pptr, rxfc->sender_ack);
	if (ret) {
		return ret;
	}

	rxfc->sending_ack = policy_create();
	if (!rxfc->sending_ack) {
		return -1;
	}

	return deserialize_policy(pptr, rxfc->sending_ack);
}

void dtcp_rxctrl_config_free(struct dtcp_rxctrl_config * rxfc)
{
	if (!rxfc)
		return;

	if (rxfc->rcvr_ack) {
		policy_free(rxfc->rcvr_ack);
		rxfc->rcvr_ack = 0;
	}

	if (rxfc->rcvr_control_ack) {
		policy_free(rxfc->rcvr_control_ack);
		rxfc->rcvr_control_ack = 0;
	}

	if (rxfc->receiving_ack_list) {
		policy_free(rxfc->receiving_ack_list);
		rxfc->receiving_ack_list = 0;
	}

	if (rxfc->retransmission_timer_expiry) {
		policy_free(rxfc->retransmission_timer_expiry);
		rxfc->retransmission_timer_expiry = 0;
	}

	if (rxfc->sender_ack) {
		policy_free(rxfc->sender_ack);
		rxfc->sender_ack = 0;
	}

	if (rxfc->sending_ack) {
		policy_free(rxfc->sending_ack);
		rxfc->sending_ack = 0;
	}

	COMMON_FREE(rxfc);
}

struct dtcp_rxctrl_config * dtcp_rxctrl_config_create()
{
	struct dtcp_rxctrl_config * result;

	result = COMMON_ALLOC(sizeof(struct dtcp_rxctrl_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct dtcp_rxctrl_config));

	return result;
}

int dtcp_config_serlen(const struct dtcp_config * dtcp_config)
{
	int ret;

	if (!dtcp_config) return 0;

	ret = 2 * sizeof(bool)
		+ policy_serlen(dtcp_config->dtcp_ps)
		+ policy_serlen(dtcp_config->lost_control_pdu)
		+ policy_serlen(dtcp_config->rtt_estimator);

	if (dtcp_config->flow_ctrl)
		ret = ret + dtcp_fctrl_config_serlen(dtcp_config->fctrl_cfg);

	if (dtcp_config->rtx_ctrl)
		ret = ret + dtcp_rxctrl_config_serlen(dtcp_config->rxctrl_cfg);

	return ret;
}

void serialize_dtcp_config(void **pptr, const struct dtcp_config *dtcp_config)
{
	if (!dtcp_config) return;

	serialize_obj(*pptr, bool, dtcp_config->flow_ctrl);
	serialize_obj(*pptr, bool, dtcp_config->rtx_ctrl);
	serialize_policy(pptr, dtcp_config->dtcp_ps);
	serialize_policy(pptr, dtcp_config->lost_control_pdu);
	serialize_policy(pptr, dtcp_config->rtt_estimator);

	if (dtcp_config->flow_ctrl)
		serialize_dtcp_fctrl_config(pptr, dtcp_config->fctrl_cfg);

	if (dtcp_config->rtx_ctrl)
		serialize_dtcp_rxctrl_config(pptr, dtcp_config->rxctrl_cfg);
}

int deserialize_dtcp_config(const void **pptr, struct dtcp_config ** dtcp_config)
{
	int ret;

	*dtcp_config = dtcp_config_create();
	if (!*dtcp_config)
		return -1;

	deserialize_obj(*pptr, bool, &(*dtcp_config)->flow_ctrl);
	deserialize_obj(*pptr, bool, &(*dtcp_config)->rtx_ctrl);

	(*dtcp_config)->dtcp_ps = policy_create();
	if (!(*dtcp_config)->dtcp_ps) {
		return -1;
	}

	ret = deserialize_policy(pptr, (*dtcp_config)->dtcp_ps);
	if (ret) {
		return ret;
	}

	(*dtcp_config)->lost_control_pdu = policy_create();
	if (!(*dtcp_config)->lost_control_pdu) {
		return -1;
	}

	ret = deserialize_policy(pptr, (*dtcp_config)->lost_control_pdu);
	if (ret) {
		return ret;
	}

	(*dtcp_config)->rtt_estimator = policy_create();
	if (!(*dtcp_config)->rtt_estimator) {
		return -1;
	}

	ret = deserialize_policy(pptr, (*dtcp_config)->rtt_estimator);
	if (ret) {
		return ret;
	}

	if ((*dtcp_config)->flow_ctrl) {
		(*dtcp_config)->fctrl_cfg = dtcp_fctrl_config_create();
		if (!(*dtcp_config)->fctrl_cfg)
			return -1;

		ret = deserialize_dtcp_fctrl_config(pptr, (*dtcp_config)->fctrl_cfg);
		if (ret)
			return ret;
	}

	if ((*dtcp_config)->rtx_ctrl) {
		(*dtcp_config)->rxctrl_cfg = dtcp_rxctrl_config_create();
		if (!(*dtcp_config)->rxctrl_cfg)
			return -1;

		ret = deserialize_dtcp_rxctrl_config(pptr, (*dtcp_config)->rxctrl_cfg);
		if (ret)
			return ret;
	}

	return ret;
}

void dtcp_config_free(struct dtcp_config * dtcp_config)
{
	if (!dtcp_config)
		return;

	if (dtcp_config->dtcp_ps) {
		policy_free(dtcp_config->dtcp_ps);
		dtcp_config->dtcp_ps = 0;
	}

	if (dtcp_config->lost_control_pdu) {
		policy_free(dtcp_config->lost_control_pdu);
		dtcp_config->lost_control_pdu = 0;
	}

	if (dtcp_config->rtt_estimator) {
		policy_free(dtcp_config->rtt_estimator);
		dtcp_config->rtt_estimator = 0;
	}

	if (dtcp_config->fctrl_cfg) {
		dtcp_fctrl_config_free(dtcp_config->fctrl_cfg);
		dtcp_config->fctrl_cfg = 0;
	}

	if (dtcp_config->rxctrl_cfg) {
		dtcp_rxctrl_config_free(dtcp_config->rxctrl_cfg);
		dtcp_config->rxctrl_cfg = 0;
	}

	COMMON_FREE(dtcp_config);
}

struct dtcp_config * dtcp_config_create()
{
	struct dtcp_config * result;

	result = COMMON_ALLOC(sizeof(struct dtcp_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct dtcp_config));

	return result;
}

int pff_config_serlen(const struct pff_config * pff)
{
	if (!pff) return 0;

	return policy_serlen(pff->policy_set);
}

void serialize_pff_config(void **pptr, const struct pff_config *pff)
{
	if (!pff) return;

	serialize_policy(pptr, pff->policy_set);
}

int deserialize_pff_config(const void **pptr, struct pff_config *pff)
{
	pff->policy_set = policy_create();
	if (!pff->policy_set) {
		return -1;
	}

	return deserialize_policy(pptr, pff->policy_set);
}

void pff_config_free(struct pff_config * pff)
{
	if (!pff)
		return;

	if (pff->policy_set) {
		policy_free(pff->policy_set);
		pff->policy_set = 0;
	}

	COMMON_FREE(pff);
}

struct pff_config * pff_config_create()
{
	struct pff_config * result;

	result = COMMON_ALLOC(sizeof(struct pff_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct pff_config));

	return result;
}

int rmt_config_serlen(const struct rmt_config * rmt)
{
	if (!rmt) return 0;

	return policy_serlen(rmt->policy_set)
		+ pff_config_serlen(rmt->pff_conf);
}

void serialize_rmt_config(void **pptr, const struct rmt_config *rmt)
{
	if (!rmt) return;

	serialize_policy(pptr, rmt->policy_set);
	serialize_pff_config(pptr, rmt->pff_conf);
}

int deserialize_rmt_config(const void **pptr, struct rmt_config *rmt)
{
	int ret;

	rmt->policy_set = policy_create();
	if (!rmt->policy_set) {
		return -1;
	}

	ret = deserialize_policy(pptr, rmt->policy_set);
	if (ret)
		return ret;

	rmt->pff_conf = pff_config_create();
	if (!rmt->pff_conf) {
		return -1;
	}

	return deserialize_pff_config(pptr, rmt->pff_conf);
}

void rmt_config_free(struct rmt_config * rmt)
{
	if (!rmt)
		return;

	if (rmt->policy_set) {
		policy_free(rmt->policy_set);
		rmt->policy_set = 0;
	}

	if (rmt->pff_conf) {
		pff_config_free(rmt->pff_conf);
		rmt->pff_conf = 0;
	}

	COMMON_FREE(rmt);
}

struct rmt_config * rmt_config_create()
{
	struct rmt_config * result;

	result = COMMON_ALLOC(sizeof(struct rmt_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct rmt_config));

	return result;
}

int dt_cons_serlen(const struct dt_cons * dtc)
{
	if (!dtc) return 0;

	return 9 * sizeof(uint16_t) + 6 * sizeof(uint32_t) + 3 * sizeof(bool);
}

void serialize_dt_cons(void **pptr, const struct dt_cons *dtc)
{
	if (!dtc) return;

	serialize_obj(*pptr, uint16_t, dtc->address_length);
	serialize_obj(*pptr, uint16_t, dtc->cep_id_length);
	serialize_obj(*pptr, uint16_t, dtc->ctrl_seq_num_length);
	serialize_obj(*pptr, bool, dtc->dif_integrity);
	serialize_obj(*pptr, uint16_t, dtc->frame_length);
	serialize_obj(*pptr, uint16_t, dtc->length_length);
	serialize_obj(*pptr, uint32_t, dtc->max_pdu_life);
	serialize_obj(*pptr, uint32_t, dtc->max_pdu_size);
	serialize_obj(*pptr, uint32_t, dtc->max_sdu_size);
	serialize_obj(*pptr, uint16_t, dtc->port_id_length);
	serialize_obj(*pptr, uint16_t, dtc->qos_id_length);
	serialize_obj(*pptr, uint16_t, dtc->rate_length);
	serialize_obj(*pptr, uint16_t, dtc->seq_num_length);
	serialize_obj(*pptr, uint32_t, dtc->seq_rollover_thres);
	serialize_obj(*pptr, uint32_t, dtc->max_time_to_ack_);
	serialize_obj(*pptr, uint32_t, dtc->max_time_to_keep_ret_);
	serialize_obj(*pptr, bool, dtc->dif_frag);
	serialize_obj(*pptr, bool, dtc->dif_concat);
}

int deserialize_dt_cons(const void **pptr, struct dt_cons *dtc)
{
	deserialize_obj(*pptr, uint16_t, &dtc->address_length);
	deserialize_obj(*pptr, uint16_t, &dtc->cep_id_length);
	deserialize_obj(*pptr, uint16_t, &dtc->ctrl_seq_num_length);
	deserialize_obj(*pptr, bool, &dtc->dif_integrity);
	deserialize_obj(*pptr, uint16_t, &dtc->frame_length);
	deserialize_obj(*pptr, uint16_t, &dtc->length_length);
	deserialize_obj(*pptr, uint32_t, &dtc->max_pdu_life);
	deserialize_obj(*pptr, uint32_t, &dtc->max_pdu_size);
	deserialize_obj(*pptr, uint32_t, &dtc->max_sdu_size);
	deserialize_obj(*pptr, uint16_t, &dtc->port_id_length);
	deserialize_obj(*pptr, uint16_t, &dtc->qos_id_length);
	deserialize_obj(*pptr, uint16_t, &dtc->rate_length);
	deserialize_obj(*pptr, uint16_t, &dtc->seq_num_length);
	deserialize_obj(*pptr, uint32_t, &dtc->seq_rollover_thres);
	deserialize_obj(*pptr, uint32_t, &dtc->max_time_to_ack_);
	deserialize_obj(*pptr, uint32_t, &dtc->max_time_to_keep_ret_);
	deserialize_obj(*pptr, bool, &dtc->dif_frag);
	deserialize_obj(*pptr, bool, &dtc->dif_concat);

	return 0;
}

void dt_cons_free(struct dt_cons * dtc)
{
	if (!dtc)
		return;

	COMMON_FREE(dtc);
}

struct dt_cons * dt_cons_create()
{
	struct dt_cons * result;

	result = COMMON_ALLOC(sizeof(struct dt_cons), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct dt_cons));

	return result;
}

int qos_cube_serlen(const struct qos_cube * qos)
{
	int ret;

	if (!qos) return 0;

	ret = 6 * sizeof (uint32_t) + 2 * sizeof(bool) + sizeof(int32_t)
			+ 3*sizeof(uint16_t) + string_prlen(qos->name)
			+ dtp_config_serlen(qos->dtpc)
			+ dtcp_config_serlen(qos->dtcpc);

	return ret;
}

void serialize_qos_cube(void **pptr, const struct qos_cube *qos)
{
	if (!qos) return;

	serialize_obj(*pptr, uint16_t, qos->id);
	serialize_obj(*pptr, uint32_t, qos->avg_bw);
	serialize_obj(*pptr, uint32_t, qos->avg_sdu_bw);
	serialize_obj(*pptr, uint32_t, qos->peak_bw_duration);
	serialize_obj(*pptr, uint32_t, qos->peak_sdu_bw_duration);
	serialize_obj(*pptr, bool, qos->partial_delivery);
	serialize_obj(*pptr, bool, qos->ordered_delivery);
	serialize_obj(*pptr, int32_t, qos->max_allowed_gap);
	serialize_obj(*pptr, uint32_t, qos->delay);
	serialize_obj(*pptr, uint32_t, qos->jitter);
	serialize_obj(*pptr, uint16_t, qos->loss);
	serialize_string(pptr, qos->name);
	serialize_dtp_config(pptr, qos->dtpc);
	serialize_dtcp_config(pptr, qos->dtcpc);
}

int deserialize_qos_cube(const void **pptr, struct qos_cube *qos)
{
	int ret;

	deserialize_obj(*pptr, uint16_t, &qos->id);
	deserialize_obj(*pptr, uint32_t, &qos->avg_bw);
	deserialize_obj(*pptr, uint32_t, &qos->avg_sdu_bw);
	deserialize_obj(*pptr, uint32_t, &qos->peak_bw_duration);
	deserialize_obj(*pptr, uint32_t, &qos->peak_sdu_bw_duration);
	deserialize_obj(*pptr, bool, &qos->partial_delivery);
	deserialize_obj(*pptr, bool, &qos->ordered_delivery);
	deserialize_obj(*pptr, int32_t, &qos->max_allowed_gap);
	deserialize_obj(*pptr, uint32_t, &qos->delay);
	deserialize_obj(*pptr, uint32_t, &qos->jitter);
	deserialize_obj(*pptr, uint16_t, &qos->loss);

	ret = deserialize_string(pptr, &qos->name);
	if (ret)
		return ret;

	ret = deserialize_dtp_config(pptr, &qos->dtpc);
	if (ret)
		return ret;

	return deserialize_dtcp_config(pptr, &qos->dtcpc);
}

void qos_cube_free(struct qos_cube * qos)
{
	if (!qos)
		return;

	if (qos->name) {
		COMMON_FREE(qos->name);
		qos->name = 0;
	}

	if (qos->dtpc) {
		dtp_config_free(qos->dtpc);
		qos->dtpc = 0;
	}

	if (qos->dtcpc) {
		dtcp_config_free(qos->dtcpc);
		qos->dtcpc = 0;
	}

	COMMON_FREE(qos);
}

struct qos_cube * qos_cube_create(void)
{
	struct qos_cube * result;

	result = COMMON_ALLOC(sizeof(struct qos_cube), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct qos_cube));

	return result;
}

int efcp_config_serlen(const struct efcp_config * efc)
{
	int ret;
	struct qos_cube_entry * pos;

	if (!efc) return 0;

	ret = dt_cons_serlen(efc->dt_cons)
			+ sizeof(uint8_t)
			+ sizeof(uint16_t)
			+ policy_serlen(efc->unknown_flow);

	if (efc->pci_offset_table)
		ret = ret + sizeof(ssize_t);

	list_for_each_entry(pos, &(efc->qos_cubes), next) {
		ret = ret + qos_cube_serlen(pos->entry);
	}

	return ret;
}

void serialize_efcp_config(void **pptr, const struct efcp_config *efc)
{
	uint8_t size = 0;
	uint16_t num_cubes = 0;
	struct qos_cube_entry * pos;

	if (!efc) return;

	serialize_dt_cons(pptr, efc->dt_cons);
	serialize_policy(pptr, efc->unknown_flow);

	if (efc->pci_offset_table) {
		size = sizeof(ssize_t);
	}
	serialize_obj(*pptr, uint8_t, size);

	if (size > 0) {
		memcpy(*pptr, efc->pci_offset_table, sizeof(ssize_t));
		*pptr += sizeof(ssize_t);
	}

	list_for_each_entry(pos, &(efc->qos_cubes), next) {
		num_cubes ++;
	}

	serialize_obj(*pptr, uint16_t, num_cubes);

	list_for_each_entry(pos, &(efc->qos_cubes), next) {
		serialize_qos_cube(pptr, pos->entry);
	}
}

int deserialize_efcp_config(const void **pptr, struct efcp_config *efc)
{
	int ret;
	uint8_t size;
	uint16_t num_cubes;
	int i;
	struct qos_cube_entry * pos;

	efc->dt_cons = dt_cons_create();
	if (!efc->dt_cons) {
		return -1;
	}

	ret = deserialize_dt_cons(pptr, efc->dt_cons);
	if (ret)
		return ret;

	efc->unknown_flow = policy_create();
	if (!efc->unknown_flow) {
		return -1;
	}

	ret = deserialize_policy(pptr, efc->unknown_flow);
	if (ret)
		return ret;

	deserialize_obj(*pptr, uint8_t, &size);
	if (size > 0) {
		efc->pci_offset_table = COMMON_ALLOC(size, 1);
		if (!efc->pci_offset_table) {
			return -1;
		}

		memcpy(efc->pci_offset_table, *pptr, size);
		*pptr += size;
	} else {
		efc->pci_offset_table = 0;
	}

	deserialize_obj(*pptr, uint16_t, &num_cubes);
	for(i = 0; i < num_cubes; i++) {
		pos = COMMON_ALLOC(sizeof(struct qos_cube_entry), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		pos->entry = qos_cube_create();
		if (!pos->entry) {
			return -1;
		}

		ret = deserialize_qos_cube(pptr, pos->entry);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &efc->qos_cubes);
	}

	return ret;
}

void efcp_config_free(struct efcp_config * efc)
{
	struct qos_cube_entry * pos, * npos;

	if (!efc)
		return;

	if (efc->dt_cons) {
		dt_cons_free(efc->dt_cons);
		efc->dt_cons = 0;
	}

	if (efc->unknown_flow) {
		policy_free(efc->unknown_flow);
		efc->unknown_flow = 0;
	}

	if (efc->pci_offset_table) {
		COMMON_FREE(efc->pci_offset_table);
		efc->pci_offset_table = 0;
	}

	list_for_each_entry_safe(pos, npos, &efc->qos_cubes, next) {
		list_del(&pos->next);
		if (pos->entry) {
			qos_cube_free(pos->entry);
			pos->entry = 0;
		}

		COMMON_FREE(pos);
	}

	COMMON_FREE(efc);
}
COMMON_EXPORT(efcp_config_free);

struct efcp_config * efcp_config_create(void)
{
	struct efcp_config * result;

	result = COMMON_ALLOC(sizeof(struct efcp_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct efcp_config));
	INIT_LIST_HEAD(&result->qos_cubes);

	return result;
}

int fa_config_serlen(const struct fa_config * fac)
{
	if (!fac) return 0;

	return sizeof(uint32_t) + policy_serlen(fac->allocate_notify)
			+ policy_serlen(fac->allocate_retry)
			+ policy_serlen(fac->new_flow_req)
			+ policy_serlen(fac->ps)
			+ policy_serlen(fac->seq_roll_over);
}

void serialize_fa_config(void **pptr, const struct fa_config *fac)
{
	if (!fac) return;

	serialize_obj(*pptr, uint32_t, fac->max_create_flow_retries);
	serialize_policy(pptr, fac->allocate_notify);
	serialize_policy(pptr, fac->allocate_retry);
	serialize_policy(pptr, fac->new_flow_req);
	serialize_policy(pptr, fac->ps);
	serialize_policy(pptr, fac->seq_roll_over);
}

int deserialize_fa_config(const void **pptr, struct fa_config *fac)
{
	int ret;

	deserialize_obj(*pptr, uint32_t, &fac->max_create_flow_retries);

	fac->allocate_notify = policy_create();
	if (!fac->allocate_notify) {
		return -1;
	}

	ret = deserialize_policy(pptr, fac->allocate_notify);
	if (ret)
		return ret;

	fac->allocate_retry = policy_create();
	if (!fac->allocate_retry) {
		return -1;
	}

	ret = deserialize_policy(pptr, fac->allocate_retry);
	if (ret)
		return ret;

	fac->new_flow_req = policy_create();
	if (!fac->new_flow_req) {
		return -1;
	}

	ret = deserialize_policy(pptr, fac->new_flow_req);
	if (ret)
		return ret;

	fac->ps = policy_create();
	if (!fac->ps) {
		return -1;
	}

	ret = deserialize_policy(pptr, fac->ps);
	if (ret)
		return ret;

	fac->seq_roll_over = policy_create();
	if (!fac->seq_roll_over) {
		return -1;
	}

	return deserialize_policy(pptr, fac->seq_roll_over);
}

void fa_config_free(struct fa_config * fac)
{
	if (!fac)
		return;

	if (fac->allocate_notify) {
		policy_free(fac->allocate_notify);
		fac->allocate_notify = 0;
	}

	if (fac->allocate_retry) {
		policy_free(fac->allocate_retry);
		fac->allocate_retry = 0;
	}

	if (fac->new_flow_req) {
		policy_free(fac->new_flow_req);
		fac->new_flow_req = 0;
	}

	if (fac->ps) {
		policy_free(fac->ps);
		fac->ps = 0;
	}

	if (fac->seq_roll_over) {
		policy_free(fac->seq_roll_over);
		fac->seq_roll_over = 0;
	}

	COMMON_FREE(fac);
}

struct fa_config * fa_config_create()
{
	struct fa_config * result;

	result = COMMON_ALLOC(sizeof(struct fa_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct fa_config));

	return result;
}

int resall_config_serlen(const struct resall_config * resc)
{
	if (!resc) return 0;

	return policy_serlen(resc->pff_gen);
}

void serialize_resall_config(void **pptr, const struct resall_config *resc)
{
	if (!resc) return;

	serialize_policy(pptr, resc->pff_gen);
}

int deserialize_resall_config(const void **pptr, struct resall_config *resc)
{
	resc->pff_gen = policy_create();
	if (!resc->pff_gen) {
		return -1;
	}

	return deserialize_policy(pptr, resc->pff_gen);
}

void resall_config_free(struct resall_config * resc)
{
	if (!resc)
		return;

	if (resc->pff_gen) {
		policy_free(resc->pff_gen);
		resc->pff_gen = 0;
	}

	COMMON_FREE(resc);
}

struct resall_config * resall_config_create(void)
{
	struct resall_config * result;

	result = COMMON_ALLOC(sizeof(struct resall_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct resall_config));

	return result;
}

int et_config_serlen(const struct et_config * etc)
{
	if (!etc) return 0;

	return policy_serlen(etc->ps);
}

void serialize_et_config(void **pptr, const struct et_config *etc)
{
	if (!etc) return;

	serialize_policy(pptr, etc->ps);
}

int deserialize_et_config(const void **pptr, struct et_config *etc)
{
	etc->ps = policy_create();
	if (!etc->ps) {
		return -1;
	}

	return deserialize_policy(pptr, etc->ps);
}

void et_config_free(struct et_config * etc)
{
	if (!etc)
		return;

	if (etc->ps) {
		policy_free(etc->ps);
		etc->ps = 0;
	}

	COMMON_FREE(etc);
}

struct et_config * et_config_create()
{
	struct et_config * result;

	result = COMMON_ALLOC(sizeof(struct et_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct et_config));

	return result;
}

int static_ipcp_addr_serlen(const struct static_ipcp_addr * addr)
{
	if (!addr) return 0;

	return 2*sizeof(uint16_t) + sizeof(uint32_t)
			+ string_prlen(addr->ap_name)
			+ string_prlen(addr->ap_instance);
}

void serialize_static_ipcp_addr(void **pptr, const struct static_ipcp_addr *addr)
{
	if (!addr) return;

	serialize_obj(*pptr, uint32_t, addr->address);
	serialize_string(pptr, addr->ap_name);
	serialize_string(pptr, addr->ap_instance);
}

int deserialize_static_ipcp_addr(const void **pptr, struct static_ipcp_addr *addr)
{
	int ret;

	deserialize_obj(*pptr, uint32_t, &addr->address);

	ret = deserialize_string(pptr, &addr->ap_name);
	if (ret)
		return ret;

	return deserialize_string(pptr, &addr->ap_instance);
}

void static_ipcp_addr_free(struct static_ipcp_addr * addr)
{
	if (!addr)
		return;

	if (addr->ap_name) {
		COMMON_FREE(addr->ap_name);
		addr->ap_name = 0;
	}

	if (addr->ap_instance) {
		COMMON_FREE(addr->ap_instance);
		addr->ap_instance = 0;
	}

	COMMON_FREE(addr);
}

struct static_ipcp_addr * static_ipcp_addr_create()
{
	struct static_ipcp_addr * result;

	result = COMMON_ALLOC(sizeof(struct static_ipcp_addr), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct static_ipcp_addr));

	return result;
}

int address_pref_config_serlen(const struct address_pref_config * apc)
{
	if (!apc) return 0;

	return sizeof(uint16_t) + sizeof(uint32_t)
			+ string_prlen(apc->org);
}

void serialize_address_pref_config(void **pptr, const struct address_pref_config *apc)
{
	if (!apc) return;

	serialize_obj(*pptr, uint32_t, apc->prefix);
	serialize_string(pptr, apc->org);
}

int deserialize_address_pref_config(const void **pptr, struct address_pref_config *apc)
{
	deserialize_obj(*pptr, uint32_t, &apc->prefix);

	return deserialize_string(pptr, &apc->org);
}

void address_pref_config_free(struct address_pref_config * apc)
{
	if (!apc)
		return;

	if (apc->org) {
		COMMON_FREE(apc->org);
		apc->org = 0;
	}

	COMMON_FREE(apc);
}

struct address_pref_config * address_pref_config_create()
{
	struct address_pref_config * result;

	result = COMMON_ALLOC(sizeof(struct address_pref_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct address_pref_config));

	return result;
}

int addressing_config_serlen(const struct addressing_config * ac)
{
	int ret;
	struct static_ipcp_addr_entry * addr_pos;
	struct address_pref_config_entry * pref_pos;

	if (!ac) return 0;

	ret = 2 * sizeof(uint16_t);

        list_for_each_entry(addr_pos, &(ac->static_ipcp_addrs), next) {
                ret = ret + static_ipcp_addr_serlen(addr_pos->entry);
        }

        list_for_each_entry(pref_pos, &(ac->address_prefixes), next) {
                ret = ret + address_pref_config_serlen(pref_pos->entry);
        }

        return ret;
}

void serialize_addressing_config(void **pptr, const struct addressing_config *ac)
{
	struct static_ipcp_addr_entry * addr_pos;
	struct address_pref_config_entry * pref_pos;
	uint16_t size = 0;

	if (!ac) return;

	list_for_each_entry(addr_pos, &(ac->static_ipcp_addrs), next) {
		size ++;
	}

	serialize_obj(*pptr, uint16_t, size);

	list_for_each_entry(addr_pos, &(ac->static_ipcp_addrs), next) {
		serialize_static_ipcp_addr(pptr, addr_pos->entry);
	}

	size = 0;
	list_for_each_entry(pref_pos, &(ac->address_prefixes), next) {
		size ++;
	}

	serialize_obj(*pptr, uint16_t, size);

	list_for_each_entry(pref_pos, &(ac->address_prefixes), next) {
		serialize_address_pref_config(pptr, pref_pos->entry);
	}
}

int deserialize_addressing_config(const void **pptr, struct addressing_config *ac)
{
	int ret;
	struct static_ipcp_addr_entry * addr_pos;
	struct address_pref_config_entry * pref_pos;
	uint16_t size;
	int i;

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		addr_pos = COMMON_ALLOC(sizeof(struct static_ipcp_addr_entry), 1);
		if (!addr_pos) {
			return -1;
		}

		INIT_LIST_HEAD(&addr_pos->next);
		addr_pos->entry = static_ipcp_addr_create();
		if (!addr_pos->entry) {
			return -1;
		}

		ret = deserialize_static_ipcp_addr(pptr, addr_pos->entry);
		if (ret) {
			return ret;
		}

		list_add_tail(&addr_pos->next, &ac->static_ipcp_addrs);
	}

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pref_pos = COMMON_ALLOC(sizeof(struct address_pref_config_entry), 1);
		if (!pref_pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pref_pos->next);
		pref_pos->entry = address_pref_config_create();
		if (!pref_pos->entry) {
			return -1;
		}

		ret = deserialize_address_pref_config(pptr, pref_pos->entry);
		if (ret) {
			return ret;
		}

		list_add_tail(&pref_pos->next, &ac->address_prefixes);
	}

	return ret;
}

void addressing_config_free(struct addressing_config * ac)
{
	struct static_ipcp_addr_entry * addr_pos, * naddr_pos;
	struct address_pref_config_entry * pref_pos, * npref_pos;

	if (!ac)
		return;

	list_for_each_entry_safe(addr_pos, naddr_pos, &ac->static_ipcp_addrs, next) {
		list_del(&addr_pos->next);
		if (addr_pos->entry) {
			static_ipcp_addr_free(addr_pos->entry);
			addr_pos->entry = 0;
		}

		COMMON_FREE(addr_pos);
	}

	list_for_each_entry_safe(pref_pos, npref_pos, &ac->address_prefixes, next) {
		list_del(&pref_pos->next);
		if (pref_pos->entry) {
			address_pref_config_free(pref_pos->entry);
			pref_pos->entry = 0;
		}

		COMMON_FREE(pref_pos);
	}

	COMMON_FREE(ac);
}

struct addressing_config * addressing_config_create()
{
	struct addressing_config * result;

	result = COMMON_ALLOC(sizeof(struct addressing_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct addressing_config));
	INIT_LIST_HEAD(&result->address_prefixes);
	INIT_LIST_HEAD(&result->static_ipcp_addrs);

	return result;
}

int nsm_config_serlen(const struct nsm_config * nsmc)
{
	if (!nsmc) return 0;

	return addressing_config_serlen(nsmc->addr_conf)
			+ policy_serlen(nsmc->ps);
}

void serialize_nsm_config(void **pptr, const struct nsm_config *nsmc)
{
	if (!nsmc) return;

	serialize_policy(pptr, nsmc->ps);
	serialize_addressing_config(pptr, nsmc->addr_conf);
}

int deserialize_nsm_config(const void **pptr, struct nsm_config *nsmc)
{
	int ret;

	nsmc->ps = policy_create();
	if (!nsmc->ps) {
		return -1;
	}

	ret = deserialize_policy(pptr, nsmc->ps);
	if (ret)
		return ret;

	nsmc->addr_conf = addressing_config_create();
	if (!nsmc->addr_conf) {
		return -1;
	}

	return deserialize_addressing_config(pptr, nsmc->addr_conf);
}

void nsm_config_free(struct nsm_config * nsmc)
{
	if (!nsmc)
		return;

	if (nsmc->ps) {
		policy_free(nsmc->ps);
		nsmc->ps = 0;
	}

	if (nsmc->addr_conf) {
		addressing_config_free(nsmc->addr_conf);
		nsmc->addr_conf = 0;
	}

	COMMON_FREE(nsmc);
}

struct nsm_config * nsm_config_create()
{
	struct nsm_config * result;

	result = COMMON_ALLOC(sizeof(struct nsm_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct nsm_config));

	return result;
}

int auth_sdup_profile_serlen(const struct auth_sdup_profile * asp)
{
	if (!asp) return 0;

 	return policy_serlen(asp->auth) + policy_serlen(asp->encrypt)
			+ policy_serlen(asp->crc) + policy_serlen(asp->ttl);
}

void serialize_auth_sdup_profile(void **pptr, const struct auth_sdup_profile *asp)
{
	if (!asp) return;

	serialize_policy(pptr, asp->auth);
	serialize_policy(pptr, asp->encrypt);
	serialize_policy(pptr, asp->crc);
	serialize_policy(pptr, asp->ttl);
}

int deserialize_auth_sdup_profile(const void **pptr, struct auth_sdup_profile *asp)
{
	int ret;

	asp->auth = policy_create();
	if (!asp->auth) {
		return -1;
	}

	ret = deserialize_policy(pptr, asp->auth);
	if (ret)
		return ret;

	asp->encrypt = policy_create();
	if (!asp->encrypt) {
		return -1;
	}

	ret = deserialize_policy(pptr, asp->encrypt);
	if (ret)
		return ret;

	asp->crc = policy_create();
	if (!asp->crc) {
		return -1;
	}

	ret = deserialize_policy(pptr, asp->crc);
	if (ret)
		return ret;

	asp->ttl = policy_create();
	if (!asp->ttl) {
		return -1;
	}

	return deserialize_policy(pptr, asp->ttl);
}

void auth_sdup_profile_free(struct auth_sdup_profile * asp)
{
	if (!asp)
		return;

	if (asp->auth) {
		policy_free(asp->auth);
		asp->auth = 0;
	}

	if (asp->encrypt) {
		policy_free(asp->encrypt);
		asp->encrypt = 0;
	}

	if (asp->crc) {
		policy_free(asp->crc);
		asp->crc = 0;
	}

	if (asp->ttl) {
		policy_free(asp->ttl);
		asp->ttl = 0;
	}

	COMMON_FREE(asp);
}

struct auth_sdup_profile * auth_sdup_profile_create()
{
	struct auth_sdup_profile * result;

	result = COMMON_ALLOC(sizeof(struct auth_sdup_profile), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct auth_sdup_profile));

	return result;
}
COMMON_EXPORT(auth_sdup_profile_create);

int secman_config_serlen(const struct secman_config * sc)
{
	int ret;
	struct auth_sdup_profile_entry * pos;

	if (!sc) return 0;

	ret = sizeof(uint16_t) + policy_serlen(sc->ps)
			+ auth_sdup_profile_serlen(sc->default_profile);

        list_for_each_entry(pos, &(sc->specific_profiles), next) {
                ret = ret + auth_sdup_profile_serlen(pos->entry) +
                		sizeof(uint16_t) + string_prlen(pos->n1_dif_name);
        }

        return ret;
}

void serialize_secman_config(void **pptr, const struct secman_config *sc)
{
	struct auth_sdup_profile_entry * pos;
	uint16_t size = 0;

	if (!sc) return;

	serialize_policy(pptr, sc->ps);
	serialize_auth_sdup_profile(pptr, sc->default_profile);

	list_for_each_entry(pos, &(sc->specific_profiles), next) {
		size ++;
	}

	serialize_obj(*pptr, uint16_t, size);

	list_for_each_entry(pos, &(sc->specific_profiles), next) {
		serialize_string(pptr, pos->n1_dif_name);
		serialize_auth_sdup_profile(pptr, pos->entry);
	}
}

int deserialize_secman_config(const void **pptr, struct secman_config *sc)
{
	int ret = 0;
	struct auth_sdup_profile_entry * pos;
	uint16_t size;
	int i;

	sc->ps = policy_create();
	if (!sc->ps) {
		return -1;
	}

	deserialize_policy(pptr, sc->ps);

	sc->default_profile = auth_sdup_profile_create();
	if (!sc->default_profile) {
		return -1;
	}

	deserialize_auth_sdup_profile(pptr, sc->default_profile);
	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = COMMON_ALLOC(sizeof(struct auth_sdup_profile_entry), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		ret = deserialize_string(pptr, &pos->n1_dif_name);
		if (ret)
			return ret;

		pos->entry = auth_sdup_profile_create();
		if (!pos->entry) {
			return -1;
		}

		ret = deserialize_auth_sdup_profile(pptr, pos->entry);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &sc->specific_profiles);
	}

	return ret;
}

void secman_config_free(struct secman_config * sc)
{
	struct auth_sdup_profile_entry * pos, * npos;

	if (!sc)
		return;

	if (sc->ps) {
		policy_free(sc->ps);
		sc->ps = 0;
	}

	if (sc->default_profile) {
		auth_sdup_profile_free(sc->default_profile);
		sc->default_profile = 0;
	}

	list_for_each_entry_safe(pos, npos, &sc->specific_profiles, next) {
		list_del(&pos->next);
		if (pos->n1_dif_name) {
			COMMON_FREE(pos->n1_dif_name);
			pos->n1_dif_name = 0;
		}
		if (pos->entry) {
			auth_sdup_profile_free(pos->entry);
			pos->entry = 0;
		}

		COMMON_FREE(pos);
	}

	COMMON_FREE(sc);
}

struct secman_config * secman_config_create()
{
	struct secman_config * result;

	result = COMMON_ALLOC(sizeof(struct secman_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct secman_config));
	INIT_LIST_HEAD(&result->specific_profiles);

	return result;
}
COMMON_EXPORT(secman_config_create);

int routing_config_serlen(const struct routing_config * rc)
{
	if (!rc) return 0;

	return policy_serlen(rc->ps);
}

void serialize_routing_config(void **pptr, const struct routing_config *rc)
{
	if (!rc) return;

	serialize_policy(pptr, rc->ps);
}

int deserialize_routing_config(const void **pptr, struct routing_config *rc)
{
	rc->ps = policy_create();
	if (!rc->ps) {
		return -1;
	}

	return deserialize_policy(pptr, rc->ps);
}

void routing_config_free(struct routing_config * rc)
{
	if (!rc)
		return;

	if (rc->ps) {
		policy_free(rc->ps);
		rc->ps = 0;
	}

	COMMON_FREE(rc);
}

struct routing_config * routing_config_create()
{
	struct routing_config * result;

	result = COMMON_ALLOC(sizeof(struct routing_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct routing_config));

	return result;
}

int ipcp_config_entry_serlen(const struct ipcp_config_entry * ice)
{
	if (!ice) return 0;

	return 2*sizeof(uint16_t) + string_prlen(ice->name)
				  + string_prlen(ice->value);
}

void serialize_ipcp_config_entry(void **pptr, const struct ipcp_config_entry *ice)
{
	if (!ice) return;

	serialize_string(pptr, ice->name);
	serialize_string(pptr, ice->value);
}

int deserialize_ipcp_config_entry(const void **pptr, struct ipcp_config_entry *ice)
{
	int ret;

	ret = deserialize_string(pptr, &ice->name);
	if (ret)
		return ret;

	return deserialize_string(pptr, &ice->value);
}

void ipcp_config_entry_free(struct ipcp_config_entry * ice)
{
	if (!ice)
		return;

	if (ice->name) {
		COMMON_FREE(ice->name);
		ice->name = 0;
	}

	if (ice->value) {
		COMMON_FREE(ice->value);
		ice->value = 0;
	}
	COMMON_FREE(ice);
}

struct ipcp_config_entry * ipcp_config_entry_create()
{
	struct ipcp_config_entry * result;

	result = COMMON_ALLOC(sizeof(struct ipcp_config_entry), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct ipcp_config_entry));

	return result;
}

int dif_config_serlen(const struct dif_config * dif_config)
{
	int ret = 0;
	struct ipcp_config * pos;

	if (!dif_config) return 0;

	ret = sizeof(address_t) + sizeof(uint16_t);

        list_for_each_entry(pos, &(dif_config->ipcp_config_entries), next) {
                ret = ret + ipcp_config_entry_serlen(pos->entry);
        }

	ret = ret + efcp_config_serlen(dif_config->efcp_config)
		  + rmt_config_serlen(dif_config->rmt_config)
		  + fa_config_serlen(dif_config->fa_config)
		  + et_config_serlen(dif_config->et_config)
		  + nsm_config_serlen(dif_config->nsm_config)
		  + routing_config_serlen(dif_config->routing_config)
		  + resall_config_serlen(dif_config->resall_config)
		  + secman_config_serlen(dif_config->secman_config);

        return ret;
}

void serialize_dif_config(void **pptr, const struct dif_config *dif_config)
{
	struct ipcp_config * pos;
	uint16_t size = 0;

	if (!dif_config) return;

	serialize_obj(*pptr, address_t, dif_config->address);

	list_for_each_entry(pos, &(dif_config->ipcp_config_entries), next) {
		size ++;
	}

	serialize_obj(*pptr, uint16_t, size);

	list_for_each_entry(pos, &(dif_config->ipcp_config_entries), next) {
		serialize_ipcp_config_entry(pptr, pos->entry);
	}

	serialize_efcp_config(pptr, dif_config->efcp_config);
	serialize_rmt_config(pptr, dif_config->rmt_config);
	serialize_fa_config(pptr, dif_config->fa_config);
	serialize_et_config(pptr, dif_config->et_config);
	serialize_nsm_config(pptr, dif_config->nsm_config);
	serialize_routing_config(pptr, dif_config->routing_config);
	serialize_resall_config(pptr, dif_config->resall_config);
	serialize_secman_config(pptr, dif_config->secman_config);
}

int deserialize_dif_config(const void **pptr, struct dif_config ** dif_config)
{
	int ret;
	struct ipcp_config * pos;
	uint16_t size;
	int i;

	*dif_config = dif_config_create();
	if (!*dif_config)
		return -1;

	deserialize_obj(*pptr, address_t, &(*dif_config)->address);
	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = COMMON_ALLOC(sizeof(struct ipcp_config), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		pos->entry = ipcp_config_entry_create();
		if (!pos->entry) {
			return -1;
		}

		ret = deserialize_ipcp_config_entry(pptr, pos->entry);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &(*dif_config)->ipcp_config_entries);
	}

	(*dif_config)->efcp_config = efcp_config_create();
	if (!(*dif_config)->efcp_config)
		return -1;
	deserialize_efcp_config(pptr, (*dif_config)->efcp_config);

	(*dif_config)->rmt_config = rmt_config_create();
	if (!(*dif_config)->rmt_config)
		return -1;
	deserialize_rmt_config(pptr, (*dif_config)->rmt_config);

	(*dif_config)->fa_config = fa_config_create();
	if (!(*dif_config)->fa_config)
		return -1;
	deserialize_fa_config(pptr, (*dif_config)->fa_config);

	(*dif_config)->et_config = et_config_create();
	if (!(*dif_config)->et_config)
		return -1;
	deserialize_et_config(pptr, (*dif_config)->et_config);

	(*dif_config)->nsm_config = nsm_config_create();
	if (!(*dif_config)->nsm_config)
		return -1;
	deserialize_nsm_config(pptr, (*dif_config)->nsm_config);

	(*dif_config)->routing_config = routing_config_create();
	if (!(*dif_config)->routing_config)
		return -1;
	deserialize_routing_config(pptr, (*dif_config)->routing_config);

	(*dif_config)->resall_config = resall_config_create();
	if (!(*dif_config)->resall_config)
		return -1;
	deserialize_resall_config(pptr, (*dif_config)->resall_config);

	(*dif_config)->secman_config = secman_config_create();
	if (!(*dif_config)->secman_config)
		return -1;
	deserialize_secman_config(pptr, (*dif_config)->secman_config);

	return ret;
}

void dif_config_free(struct dif_config * dif_config)
{
	struct ipcp_config * pos, * npos;

	if (!dif_config)
		return;

	if (dif_config->efcp_config) {
		efcp_config_free(dif_config->efcp_config);
		dif_config->efcp_config = 0;
	}

	if (dif_config->rmt_config) {
		rmt_config_free(dif_config->rmt_config);
		dif_config->rmt_config = 0;
	}

	if (dif_config->fa_config) {
		fa_config_free(dif_config->fa_config);
		dif_config->fa_config = 0;
	}

	if (dif_config->et_config) {
		et_config_free(dif_config->et_config);
		dif_config->et_config = 0;
	}

	if (dif_config->nsm_config) {
		nsm_config_free(dif_config->nsm_config);
		dif_config->nsm_config = 0;
	}

	if (dif_config->routing_config) {
		routing_config_free(dif_config->routing_config);
		dif_config->routing_config = 0;
	}

	if (dif_config->resall_config) {
		resall_config_free(dif_config->resall_config);
		dif_config->resall_config = 0;
	}

	if (dif_config->secman_config) {
		secman_config_free(dif_config->secman_config);
		dif_config->secman_config = 0;
	}

	list_for_each_entry_safe(pos, npos, &dif_config->ipcp_config_entries, next) {
		list_del(&pos->next);
		if (pos->entry) {
			ipcp_config_entry_free(pos->entry);
			pos->entry = 0;
		}

		COMMON_FREE(pos);
	}

	COMMON_FREE(dif_config);
}

struct dif_config * dif_config_create()
{
	struct dif_config * result;

	result = COMMON_ALLOC(sizeof(struct dif_config), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct dif_config));
	INIT_LIST_HEAD(&result->ipcp_config_entries);

	return result;
}

int rib_object_data_serlen(const struct rib_object_data * rod)
{
	if (!rod) return 0;

	return 3 * sizeof(uint16_t) + string_prlen(rod->name)
			+ string_prlen(rod->clazz)
			+ string_prlen(rod->disp_value)
			+ sizeof(uint64_t);
}

void serialize_rib_object_data(void **pptr, const struct rib_object_data *rod)
{
	if (!rod) return;

	serialize_obj(*pptr, uint64_t, rod->instance);
	serialize_string(pptr, rod->name);
	serialize_string(pptr, rod->clazz);
	serialize_string(pptr, rod->disp_value);
}

int deserialize_rib_object_data(const void **pptr, struct rib_object_data *rod)
{
	int ret;

	deserialize_obj(*pptr, uint64_t, &rod->instance);

	ret = deserialize_string(pptr, &rod->name);
	if (ret)
		return ret;

	ret = deserialize_string(pptr, &rod->clazz);
	if (ret)
		return ret;

	return deserialize_string(pptr, &rod->disp_value);
}

void rib_object_data_free(struct rib_object_data * rod)
{
	if (!rod)
		return;

	if (rod->name) {
		COMMON_FREE(rod->name);
		rod->name = 0;
	}

	if (rod->clazz) {
		COMMON_FREE(rod->clazz);
		rod->clazz = 0;
	}

	if (rod->disp_value) {
		COMMON_FREE(rod->disp_value);
		rod->disp_value = 0;
	}

	COMMON_FREE(rod);
}

struct rib_object_data * rib_object_data_create()
{
	struct rib_object_data * result;

	result = COMMON_ALLOC(sizeof(struct rib_object_data), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct rib_object_data));
	INIT_LIST_HEAD(&result->next);

	return result;
}

int query_rib_resp_serlen(const struct query_rib_resp * qrr)
{
	int ret;
	struct rib_object_data * pos;

	if (!qrr) return 0;

	ret = sizeof(uint16_t);

        list_for_each_entry(pos, &(qrr->rib_object_data_entries), next) {
                ret = ret + rib_object_data_serlen(pos);
        }

        return ret;
}

void serialize_query_rib_resp(void **pptr, const struct query_rib_resp *qrr)
{
	struct rib_object_data * pos;
	uint16_t size = 0;

	if (!qrr) return;

        list_for_each_entry(pos, &(qrr->rib_object_data_entries), next) {
                size++;
        }

        serialize_obj(*pptr, uint16_t, size);

        list_for_each_entry(pos, &(qrr->rib_object_data_entries), next) {
                serialize_rib_object_data(pptr, pos);
        }
}

int deserialize_query_rib_resp(const void **pptr, struct query_rib_resp **qrr)
{
	int ret = 0;
	struct rib_object_data * pos;
	uint16_t size;
	int i;

	*qrr = query_rib_resp_create();
	if (!*qrr)
		return -1;

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = rib_object_data_create();
		if (!pos) {
			return -1;
		}

		ret = deserialize_rib_object_data(pptr, pos);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &(*qrr)->rib_object_data_entries);
	}

	return ret;
}

void query_rib_resp_free(struct query_rib_resp * qrr)
{
	struct rib_object_data * pos, * npos;

	if (!qrr)
		return;

	list_for_each_entry_safe(pos, npos, &qrr->rib_object_data_entries, next) {
		list_del(&pos->next);
		rib_object_data_free(pos);
	}

	COMMON_FREE(qrr);
}

struct query_rib_resp * query_rib_resp_create()
{
	struct query_rib_resp * result;

	result = COMMON_ALLOC(sizeof(struct query_rib_resp), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct query_rib_resp));
	INIT_LIST_HEAD(&result->rib_object_data_entries);

	return result;
}

int port_id_altlist_serlen(const struct port_id_altlist * pia)
{
	if (!pia) return 0;

	return sizeof(uint16_t) + pia->num_ports * sizeof(port_id_t);
}

void serialize_port_id_altlist(void **pptr, const struct port_id_altlist *pia)
{
	int plength;

	if (!pia) return;

	serialize_obj(*pptr, uint16_t, pia->num_ports);
	plength = pia->num_ports * sizeof(port_id_t);

	if (plength > 0) {
		memcpy(*pptr, pia->ports, plength);
		*pptr += plength;
	}
}

int deserialize_port_id_altlist(const void **pptr, struct port_id_altlist *pia)
{
	int plength;

	deserialize_obj(*pptr, uint16_t, &pia->num_ports);
	plength = pia->num_ports * sizeof(port_id_t);

	if (plength > 0) {
		pia->ports = COMMON_ALLOC(plength, 1);
		memcpy(pia->ports, *pptr, plength);
		*pptr += plength;
	}

	return 0;
}

void port_id_altlist_free(struct port_id_altlist * pia)
{
	if (!pia)
		return;

	if (pia->num_ports > 0) {
		COMMON_FREE(pia->ports);
		pia->ports = 0;
	}

	COMMON_FREE(pia);
}

struct port_id_altlist * port_id_altlist_create()
{
	struct port_id_altlist * result;

	result = COMMON_ALLOC(sizeof(struct port_id_altlist), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct port_id_altlist));
	INIT_LIST_HEAD(&result->next);

	return result;
}

int mod_pff_entry_serlen(const struct mod_pff_entry * pffe)
{
	int ret;
	struct port_id_altlist * pos;

	if (!pffe) return 0;

	ret = sizeof(address_t) + sizeof(qos_id_t) + sizeof(uint16_t)
			+ sizeof(uint32_t);

        list_for_each_entry(pos, &(pffe->port_id_altlists), next) {
                ret = ret + port_id_altlist_serlen(pos);
        }

	return ret;
}

void serialize_mod_pff_entry(void **pptr, const struct mod_pff_entry *pffe)
{
	uint16_t num_alts = 0;
	struct port_id_altlist * pos;

	if (!pffe) return;

	serialize_obj(*pptr, address_t, pffe->fwd_info);
	serialize_obj(*pptr, qos_id_t, pffe->qos_id);
	serialize_obj(*pptr, uint32_t, pffe->cost);

        list_for_each_entry(pos, &(pffe->port_id_altlists), next) {
               num_alts++;
        }

        serialize_obj(*pptr, uint16_t, num_alts);
        list_for_each_entry(pos, &(pffe->port_id_altlists), next) {
               serialize_port_id_altlist(pptr, pos);
        }
}

int deserialize_mod_pff_entry(const void **pptr, struct mod_pff_entry *pffe)
{
	int ret;
	uint16_t num_alts;
	struct port_id_altlist * pos;
	int i;

	deserialize_obj(*pptr, address_t, &pffe->fwd_info);
	deserialize_obj(*pptr, qos_id_t, &pffe->qos_id);
	deserialize_obj(*pptr, uint32_t, &pffe->cost);
	deserialize_obj(*pptr, uint16_t, &num_alts);

	for(i = 0; i < num_alts; i++) {
		pos = port_id_altlist_create();
		if (!pos) {
			return -1;
		}

		ret = deserialize_port_id_altlist(pptr, pos);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &pffe->port_id_altlists);
	}

	return 0;
}

void mod_pff_entry_free(struct mod_pff_entry * pffe)
{
	struct port_id_altlist * pos, * npos;

	if (!pffe)
		return;

	list_for_each_entry_safe(pos, npos, &pffe->port_id_altlists, next) {
		list_del(&pos->next);
		port_id_altlist_free(pos);
	}

	COMMON_FREE(pffe);
}

struct mod_pff_entry * mod_pff_entry_create()
{
	struct mod_pff_entry * result;

	result = COMMON_ALLOC(sizeof(struct mod_pff_entry), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct mod_pff_entry));
	INIT_LIST_HEAD(&result->next);
	INIT_LIST_HEAD(&result->port_id_altlists);

	return result;
}

int pff_entry_list_serlen(const struct pff_entry_list * pel)
{
	int ret;
	struct mod_pff_entry * pos;

	if (!pel) return 0;

	ret = sizeof(uint16_t);

        list_for_each_entry(pos, &(pel->pff_entries), next) {
                ret = ret + mod_pff_entry_serlen(pos);
        }

        return ret;
}

void serialize_pff_entry_list(void **pptr, const struct pff_entry_list *pel)
{
	struct mod_pff_entry * pos;
	uint16_t size = 0;

	if (!pel) return;

        list_for_each_entry(pos, &(pel->pff_entries), next) {
                size++;
        }

        serialize_obj(*pptr, uint16_t, size);

        list_for_each_entry(pos, &(pel->pff_entries), next) {
                serialize_mod_pff_entry(pptr, pos);
        }
}

int deserialize_pff_entry_list(const void **pptr, struct pff_entry_list **pel)
{
	int ret;
	struct mod_pff_entry * pos;
	uint16_t size;
	int i;

	*pel = pff_entry_list_create();
	if (!*pel)
		return -1;

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = mod_pff_entry_create();
		if (!pos) {
			return -1;
		}

		ret = deserialize_mod_pff_entry(pptr, pos);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &(*pel)->pff_entries);
	}

	return 0;
}

void pff_entry_list_free(struct pff_entry_list * pel)
{
	struct mod_pff_entry * pos, * npos;

	if (!pel)
		return;

	list_for_each_entry_safe(pos, npos, &pel->pff_entries, next) {
		list_del(&pos->next);
		mod_pff_entry_free(pos);
	}

	COMMON_FREE(pel);
}

struct pff_entry_list * pff_entry_list_create()
{
	struct pff_entry_list * result;

	result = COMMON_ALLOC(sizeof(struct pff_entry_list), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct pff_entry_list));
	INIT_LIST_HEAD(&result->pff_entries);

	return result;
}

int sdup_crypto_state_serlen(const struct sdup_crypto_state * scs)
{
	if (!scs) return 0;

	return 2 * sizeof(bool) + 3 * sizeof(uint16_t) + sizeof(port_id_t)
	       + string_prlen(scs->compress_alg)
	       + string_prlen(scs->enc_alg)
	       + string_prlen(scs->mac_alg)
	       + 6 * sizeof(uint32_t)
	       + buffer_prlen(scs->encrypt_key_rx)
	       + buffer_prlen(scs->encrypt_key_tx)
	       + buffer_prlen(scs->iv_rx)
	       + buffer_prlen(scs->iv_tx)
	       + buffer_prlen(scs->mac_key_rx)
	       + buffer_prlen(scs->mac_key_tx);
}

void serialize_sdup_crypto_state(void **pptr, const struct sdup_crypto_state *scs)
{
	if (!scs) return;

	serialize_obj(*pptr, bool, scs->enable_crypto_rx);
	serialize_obj(*pptr, bool, scs->enable_crypto_tx);
	serialize_obj(*pptr, port_id_t, scs->port_id);
	serialize_string(pptr, scs->compress_alg);
	serialize_string(pptr, scs->enc_alg);
	serialize_string(pptr, scs->mac_alg);
	serialize_buffer(pptr, scs->encrypt_key_rx);
	serialize_buffer(pptr, scs->encrypt_key_tx);
	serialize_buffer(pptr, scs->iv_rx);
	serialize_buffer(pptr, scs->iv_tx);
	serialize_buffer(pptr, scs->mac_key_rx);
	serialize_buffer(pptr, scs->mac_key_tx);
}

int deserialize_sdup_crypto_state(const void **pptr, struct sdup_crypto_state **scs)
{
	int ret;

	*scs = sdup_crypto_state_create();
	if (!*scs)
		return -1;

	deserialize_obj(*pptr, bool, &(*scs)->enable_crypto_rx);
	deserialize_obj(*pptr, bool, &(*scs)->enable_crypto_tx);
	deserialize_obj(*pptr, port_id_t, &(*scs)->port_id);

	ret = deserialize_string(pptr, &(*scs)->compress_alg);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &(*scs)->enc_alg);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &(*scs)->mac_alg);
	if (ret) {
		return ret;
	}

	ret = deserialize_buffer(pptr, &(*scs)->encrypt_key_rx);
	if (ret) {
		return ret;
	}

	ret = deserialize_buffer(pptr, &(*scs)->encrypt_key_tx);
	if (ret) {
		return ret;
	}

	ret = deserialize_buffer(pptr, &(*scs)->iv_rx);
	if (ret) {
		return ret;
	}

	ret = deserialize_buffer(pptr, &(*scs)->iv_tx);
	if (ret) {
		return ret;
	}

	ret = deserialize_buffer(pptr, &(*scs)->mac_key_rx);
	if (ret) {
		return ret;
	}

	return deserialize_buffer(pptr, &(*scs)->mac_key_tx);
}

void sdup_crypto_state_free(struct sdup_crypto_state * scs)
{
	if (!scs)
		return;

	if (scs->compress_alg) {
		COMMON_FREE(scs->compress_alg);
		scs->compress_alg = 0;
	}

	if (scs->enc_alg) {
		COMMON_FREE(scs->enc_alg);
		scs->enc_alg = 0;
	}

	if (scs->mac_alg) {
		COMMON_FREE(scs->mac_alg);
		scs->mac_alg = 0;
	}

	if (scs->encrypt_key_rx) {
		buffer_destroy(scs->encrypt_key_rx);
		scs->encrypt_key_rx = 0;
	}

	if (scs->encrypt_key_tx) {
		buffer_destroy(scs->encrypt_key_tx);
		scs->encrypt_key_rx = 0;
	}

	if (scs->iv_rx) {
		buffer_destroy(scs->iv_rx);
		scs->iv_rx = 0;
	}

	if (scs->iv_tx) {
		buffer_destroy(scs->iv_tx);
		scs->iv_tx = 0;
	}

	if (scs->mac_key_rx) {
		buffer_destroy(scs->mac_key_rx);
		scs->mac_key_rx = 0;
	}

	if (scs->mac_key_tx) {
		buffer_destroy(scs->mac_key_tx);
		scs->mac_key_tx = 0;
	}

	COMMON_FREE(scs);
}

struct sdup_crypto_state * sdup_crypto_state_create()
{
	struct sdup_crypto_state * result;

	result = COMMON_ALLOC(sizeof(struct sdup_crypto_state), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct sdup_crypto_state));

	return result;
}

int dif_properties_entry_serlen(const struct dif_properties_entry * dpe)
{
	if (!dpe) return 0;

	return sizeof(uint16_t) + rina_name_serlen(dpe->dif_name);
}

void serialize_dif_properties_entry(void **pptr, const struct dif_properties_entry *dpe)
{
	if (!dpe) return;

	serialize_obj(*pptr, uint16_t, dpe->max_sdu_size);
	serialize_rina_name(pptr, dpe->dif_name);
}

int deserialize_dif_properties_entry(const void **pptr, struct dif_properties_entry *dpe)
{
	deserialize_obj(*pptr, uint16_t, &dpe->max_sdu_size);
	return deserialize_rina_name(pptr, &dpe->dif_name);
}

void dif_properties_entry_free(struct dif_properties_entry * dpe)
{
	if (!dpe)
		return;

	if (dpe->dif_name) {
		rina_name_free(dpe->dif_name);
		dpe->dif_name = 0;
	}

	COMMON_FREE(dpe);
}

struct dif_properties_entry * dif_properties_entry_create()
{
	struct dif_properties_entry * result;

	result = COMMON_ALLOC(sizeof(struct dif_properties_entry), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct dif_properties_entry));
	INIT_LIST_HEAD(&result->next);

	return result;
}

int get_dif_prop_resp_serlen(const struct get_dif_prop_resp * gdp)
{
	int ret;
	struct dif_properties_entry * pos;

	if (!gdp) return 0;

	ret = sizeof(uint16_t);

        list_for_each_entry(pos, &(gdp->dif_propery_entries), next) {
                ret = ret + dif_properties_entry_serlen(pos);
        }

        return ret;
}

void serialize_get_dif_prop_resp(void **pptr, const struct get_dif_prop_resp *gdp)
{
	struct dif_properties_entry * pos;
	uint16_t size = 0;

	if (!gdp) return;

        list_for_each_entry(pos, &(gdp->dif_propery_entries), next) {
                size++;
        }

        serialize_obj(*pptr, uint16_t, size);

        list_for_each_entry(pos, &(gdp->dif_propery_entries), next) {
        	serialize_dif_properties_entry(pptr, pos);
        }
}

int deserialize_get_dif_prop_resp(const void **pptr, struct get_dif_prop_resp **gdp)
{
	int ret = 0;
	struct dif_properties_entry * pos;
	uint16_t size;
	int i;

	*gdp = get_dif_prop_resp_create();
	if (!*gdp)
		return -1;

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = dif_properties_entry_create();
		if (!pos) {
			return -1;
		}

		ret = deserialize_dif_properties_entry(pptr, pos);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &(*gdp)->dif_propery_entries);
	}

	return ret;
}

void get_dif_prop_resp_free(struct get_dif_prop_resp * gdp)
{
	struct dif_properties_entry * pos, * npos;

	if (!gdp)
		return;

	list_for_each_entry_safe(pos, npos, &gdp->dif_propery_entries, next) {
		list_del(&pos->next);
		dif_properties_entry_free(pos);
	}

	COMMON_FREE(gdp);
}

struct get_dif_prop_resp * get_dif_prop_resp_create()
{
	struct get_dif_prop_resp * result;

	result = COMMON_ALLOC(sizeof(struct get_dif_prop_resp), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct get_dif_prop_resp));
	INIT_LIST_HEAD(&result->dif_propery_entries);

	return result;
}

int ipcp_neighbor_serlen(const struct ipcp_neighbor * nei)
{
	int ret;
	struct name_entry * pos;

	if (!nei) return 0;

	ret = 4 * sizeof(uint32_t) + sizeof(bool) + 3 * sizeof(int32_t)
		+ sizeof(uint16_t) + rina_name_serlen(nei->ipcp_name)
		+ rina_name_serlen(nei->sup_dif_name);

        list_for_each_entry(pos, &(nei->supporting_difs), next) {
                ret = ret + rina_name_serlen(pos->entry);
        }

        return ret;
}

void serialize_ipcp_neighbor(void **pptr, const struct ipcp_neighbor *nei)
{
	struct name_entry * pos;
	uint16_t size = 0;

	if (!nei) return;

	serialize_obj(*pptr, uint32_t, nei->address);
	serialize_obj(*pptr, uint32_t, nei->old_address);
	serialize_obj(*pptr, uint32_t, nei->average_rtt_in_ms);
	serialize_obj(*pptr, uint32_t, nei->num_enroll_attempts);
	serialize_obj(*pptr, bool, nei->enrolled);
	serialize_obj(*pptr, int32_t, nei->under_port_id);
	serialize_obj(*pptr, int32_t, nei->intern_port_id);
	serialize_obj(*pptr, int32_t, nei->last_heard_time_ms);
	serialize_rina_name(pptr, nei->ipcp_name);
	serialize_rina_name(pptr, nei->sup_dif_name);

        list_for_each_entry(pos, &(nei->supporting_difs), next) {
                size++;
        }

        serialize_obj(*pptr, uint16_t, size);

        list_for_each_entry(pos, &(nei->supporting_difs), next) {
        	serialize_rina_name(pptr, pos->entry);
        }
}

int deserialize_ipcp_neighbor(const void **pptr, struct ipcp_neighbor *nei)
{
	int ret = 0;
	struct name_entry * pos;
	uint16_t size;
	int i;

	deserialize_obj(*pptr, uint32_t, &nei->address);
	deserialize_obj(*pptr, uint32_t, &nei->old_address);
	deserialize_obj(*pptr, uint32_t, &nei->average_rtt_in_ms);
	deserialize_obj(*pptr, uint32_t, &nei->num_enroll_attempts);
	deserialize_obj(*pptr, bool, &nei->enrolled);
	deserialize_obj(*pptr, int32_t, &nei->under_port_id);
	deserialize_obj(*pptr, int32_t, &nei->intern_port_id);
	deserialize_obj(*pptr, int32_t, &nei->last_heard_time_ms);
	deserialize_rina_name(pptr, &nei->ipcp_name);
	deserialize_rina_name(pptr, &nei->sup_dif_name);

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = COMMON_ALLOC(sizeof(struct name_entry), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		ret = deserialize_rina_name(pptr, &pos->entry);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &nei->supporting_difs);
	}

	return ret;
}

void ipcp_neighbor_free(struct ipcp_neighbor * nei)
{
	struct name_entry * pos, * npos;

	if (!nei)
		return;

	if (nei->ipcp_name) {
		rina_name_free(nei->ipcp_name);
		nei->ipcp_name = 0;
	}

	if (nei->sup_dif_name) {
		rina_name_free(nei->sup_dif_name);
		nei->sup_dif_name = 0;
	}

	list_for_each_entry_safe(pos, npos, &nei->supporting_difs, next) {
		list_del(&pos->next);

		if (pos->entry) {
			rina_name_free(pos->entry);
			pos->entry = 0;
		}

		COMMON_FREE(pos);
	}

	COMMON_FREE(nei);
}

struct ipcp_neighbor * ipcp_neighbor_create()
{
	struct ipcp_neighbor * result;

	result = COMMON_ALLOC(sizeof(struct ipcp_neighbor), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct ipcp_neighbor));
	INIT_LIST_HEAD(&result->supporting_difs);

	return result;
}

int ipcp_neigh_list_serlen(const struct ipcp_neigh_list * nei)
{
	int ret;
	struct ipcp_neighbor_entry * pos;

	if (!nei) return 0;

	ret = sizeof(uint16_t);

        list_for_each_entry(pos, &(nei->ipcp_neighbors), next) {
                ret = ret + ipcp_neighbor_serlen(pos->entry);
        }

        return ret;
}

void serialize_ipcp_neigh_list(void **pptr, const struct ipcp_neigh_list *nei)
{
	struct ipcp_neighbor_entry * pos;
	uint16_t size = 0;

	if (!nei) return;

        list_for_each_entry(pos, &(nei->ipcp_neighbors), next) {
                size++;
        }

        serialize_obj(*pptr, uint16_t, size);

        list_for_each_entry(pos, &(nei->ipcp_neighbors), next) {
        	serialize_ipcp_neighbor(pptr, pos->entry);
        }
}

int deserialize_ipcp_neigh_list(const void **pptr, struct ipcp_neigh_list **nei)
{
	int ret = 0;
	struct ipcp_neighbor_entry * pos;
	uint16_t size;
	int i;

	*nei = ipcp_neigh_list_create();
	if (!*nei)
		return -1;

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = COMMON_ALLOC(sizeof(struct ipcp_neighbor_entry), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		pos->entry = ipcp_neighbor_create();
		if (!pos->entry)
			return -1;

		ret = deserialize_ipcp_neighbor(pptr, pos->entry);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &(*nei)->ipcp_neighbors);
	}

	return ret;
}

void ipcp_neigh_list_free(struct ipcp_neigh_list * nei)
{
	struct ipcp_neighbor_entry * pos, * npos;

	if (!nei)
		return;

	list_for_each_entry_safe(pos, npos, &nei->ipcp_neighbors, next) {
		list_del(&pos->next);

		if (pos->entry) {
			ipcp_neighbor_free(pos->entry);
			pos->entry = 0;
		}

		COMMON_FREE(pos);
	}

	COMMON_FREE(nei);
}

struct ipcp_neigh_list * ipcp_neigh_list_create()
{
	struct ipcp_neigh_list * result;

	result = COMMON_ALLOC(sizeof(struct ipcp_neigh_list), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct ipcp_neigh_list));
	INIT_LIST_HEAD(&result->ipcp_neighbors);

	return result;
}

int bs_info_entry_serlen(const struct bs_info_entry * bie)
{
	if (!bie) return 0;

	return sizeof(int32_t) + sizeof(uint16_t)
			+ string_prlen(bie->ipcp_addr);
}

void serialize_bs_info_entry(void **pptr, const struct bs_info_entry *bie)
{
	if (!bie) return;

	serialize_obj(*pptr, int32_t, bie->signal_strength);
	serialize_string(pptr, bie->ipcp_addr);
}

int deserialize_bs_info_entry(const void **pptr, struct bs_info_entry *bie)
{
	deserialize_obj(*pptr, int32_t, &bie->signal_strength);

	return deserialize_string(pptr, &bie->ipcp_addr);
}

void bs_info_entry_free(struct bs_info_entry * bie)
{
	if (!bie)
		return;

	if (bie->ipcp_addr) {
		COMMON_FREE(bie->ipcp_addr);
		bie->ipcp_addr = 0;
	}

	COMMON_FREE(bie);
}

struct bs_info_entry * bs_info_entry_create()
{
	struct bs_info_entry * result;

	result = COMMON_ALLOC(sizeof(struct bs_info_entry), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct bs_info_entry));
	INIT_LIST_HEAD(&result->next);

	return result;
}

int media_dif_info_serlen(const struct media_dif_info * mdi)
{
	int ret;
	struct bs_info_entry * pos;

	if (!mdi) return 0;

	ret = 3 * sizeof(uint16_t) + string_prlen(mdi->dif_name)
			+ string_prlen(mdi->sec_policies);

        list_for_each_entry(pos, &(mdi->available_bs_ipcps), next) {
                ret = ret + bs_info_entry_serlen(pos);
        }

        return ret;
}

void serialize_media_dif_info(void **pptr, const struct media_dif_info *mdi)
{
	uint16_t size = 0;
	struct bs_info_entry * pos;

	if (!mdi) return;

	serialize_string(pptr, mdi->dif_name);
	serialize_string(pptr, mdi->sec_policies);

        list_for_each_entry(pos, &(mdi->available_bs_ipcps), next) {
                size++;
        }

        serialize_obj(*pptr, uint16_t, size);

        list_for_each_entry(pos, &(mdi->available_bs_ipcps), next) {
        	serialize_bs_info_entry(pptr, pos);
        }
}

int deserialize_media_dif_info(const void **pptr, struct media_dif_info *mdi)
{
	int ret;
	struct bs_info_entry * pos;
	uint16_t size;
	int i;

	ret = deserialize_string(pptr, &mdi->dif_name);
	if (ret)
		return ret;

	ret = deserialize_string(pptr, &mdi->sec_policies);
	if (ret)
		return ret;

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = bs_info_entry_create();
		if (!pos) {
			return -1;
		}

		ret = deserialize_bs_info_entry(pptr, pos);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &mdi->available_bs_ipcps);
	}

	return ret;
}

void media_dif_info_free(struct media_dif_info * mdi)
{
	struct bs_info_entry * pos, * npos;

	if (!mdi)
		return;

	if (mdi->dif_name) {
		COMMON_FREE(mdi->dif_name);
		mdi->dif_name = 0;
	}

	if (mdi->sec_policies) {
		COMMON_FREE(mdi->sec_policies);
		mdi->sec_policies = 0;
	}

	list_for_each_entry_safe(pos, npos, &mdi->available_bs_ipcps, next) {
		list_del(&pos->next);
		bs_info_entry_free(pos);
	}

	COMMON_FREE(mdi);
}

struct media_dif_info * media_dif_info_create()
{
	struct media_dif_info * result;

	result = COMMON_ALLOC(sizeof(struct media_dif_info), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct media_dif_info));
	INIT_LIST_HEAD(&result->available_bs_ipcps);

	return result;
}

int media_report_serlen(const struct media_report * mre)
{
	int ret;
	struct media_info_entry * pos;

	if (!mre) return 0;

	ret = 3 * sizeof(uint16_t) + string_prlen(mre->dif_name)
		+ string_prlen(mre->bs_ipcp_addr) + sizeof(ipc_process_id_t);

	list_for_each_entry(pos, &(mre->available_difs), next) {
		ret = ret + sizeof(uint16_t) + string_prlen(pos->dif_name)
				+ media_dif_info_serlen(pos->entry);
	}

	return ret;
}

void serialize_media_report(void **pptr, const struct media_report *mre)
{
	uint16_t size = 0;
	struct media_info_entry * pos;

	if (!mre) return;

	serialize_obj(*pptr, ipc_process_id_t, mre->ipcp_id);
	serialize_string(pptr, mre->dif_name);
	serialize_string(pptr, mre->bs_ipcp_addr);

        list_for_each_entry(pos, &(mre->available_difs), next) {
                size++;
        }

        serialize_obj(*pptr, uint16_t, size);

        list_for_each_entry(pos, &(mre->available_difs), next) {
        	serialize_string(pptr, pos->dif_name);
        	serialize_media_dif_info(pptr, pos->entry);
        }
}

int deserialize_media_report(const void **pptr, struct media_report **mre)
{
	int ret;
	struct media_info_entry * pos;
	uint16_t size;
	int i;

	*mre = media_report_create();
	if (!*mre)
		return -1;

	deserialize_obj(*pptr, ipc_process_id_t, &(*mre)->ipcp_id);

	ret = deserialize_string(pptr, &(*mre)->dif_name);
	if (ret)
		return ret;

	ret = deserialize_string(pptr, &(*mre)->bs_ipcp_addr);
	if (ret)
		return ret;

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = COMMON_ALLOC(sizeof(struct media_info_entry), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		ret = deserialize_string(pptr, &pos->dif_name);
		if (ret)
			return ret;

		pos->entry = media_dif_info_create();
		if (!pos->entry) {
			return -1;
		}

		ret = deserialize_media_dif_info(pptr, pos->entry);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &(*mre)->available_difs);
	}

	return ret;
}

void media_report_free(struct media_report * mre)
{
	struct media_info_entry * pos, * npos;

	if (!mre)
		return;

	if (mre->dif_name) {
		COMMON_FREE(mre->dif_name);
		mre->dif_name = 0;
	}

	if (mre->bs_ipcp_addr) {
		COMMON_FREE(mre->bs_ipcp_addr);
		mre->bs_ipcp_addr = 0;
	}

	list_for_each_entry_safe(pos, npos, &mre->available_difs, next) {
		list_del(&pos->next);

		if (pos->dif_name) {
			COMMON_FREE(pos->dif_name);
			pos->dif_name = 0;
		}

		if (pos->entry) {
			media_dif_info_free(pos->entry);
			pos->entry = 0;
		}

		COMMON_FREE(pos);
	}

	COMMON_FREE(mre);
}

struct media_report * media_report_create()
{
	struct media_report * result;

	result = COMMON_ALLOC(sizeof(struct media_report), 1);
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct media_report));
	INIT_LIST_HEAD(&result->available_difs);

	return result;
}

int serialize_irati_msg(struct irati_msg_layout *numtables,
		        size_t num_entries,
			void *serbuf,
			const struct irati_msg_base *msg)
{
	void *serptr = serbuf;
	unsigned int serlen;
	unsigned int copylen;
	struct name ** name;
	string_t ** str;
	const struct buffer ** bf;
	struct flow_spec ** fspec;
	struct dif_config ** dif_config;
	struct dtp_config ** dtp_config;
	struct dtcp_config ** dtcp_config;
	struct query_rib_resp ** qrr;
	struct pff_entry_list ** pel;
	struct sdup_crypto_state ** scs;
	struct get_dif_prop_resp ** gdp;
	struct ipcp_neigh_list ** inl;
	struct media_report ** mre;
	int i;

	if (msg->msg_type >= num_entries) {
		return -1;
	}

	copylen = numtables[msg->msg_type].copylen;
	memcpy(serbuf, msg, copylen);

	serptr = serbuf + copylen;
	name = (struct name **) (((void *)msg) + copylen);
	for (i = 0; i < numtables[msg->msg_type].names; i++, name++) {
		serialize_rina_name(&serptr, *name);
	}

	str = (string_t **)(name);
	for (i = 0; i < numtables[msg->msg_type].strings; i++, str++) {
		serialize_string(&serptr, *str);
	}

	fspec = (struct flow_spec **)str;
	for (i = 0; i < numtables[msg->msg_type].flow_specs; i++, fspec++) {
		serialize_flow_spec(&serptr, *fspec);
	}

	dif_config = (struct dif_config **)fspec;
	for (i = 0; i < numtables[msg->msg_type].dif_configs; i++, dif_config++) {
		serialize_dif_config(&serptr, *dif_config);
	}

	dtp_config = (struct dtp_config **)dif_config;
	for (i = 0; i < numtables[msg->msg_type].dtp_configs; i++, dtp_config++) {
		serialize_dtp_config(&serptr, *dtp_config);
	}

	dtcp_config = (struct dtcp_config **)dtp_config;
	for (i = 0; i < numtables[msg->msg_type].dtcp_configs; i++, dtcp_config++) {
		serialize_dtcp_config(&serptr, *dtcp_config);
	}

	qrr = (struct query_rib_resp **)dtcp_config;
	for (i = 0; i < numtables[msg->msg_type].query_rib_resps; i++, qrr++) {
		serialize_query_rib_resp(&serptr, *qrr);
	}

	pel = (struct pff_entry_list **)qrr;
	for (i = 0; i < numtables[msg->msg_type].pff_entry_lists; i++, pel++) {
		serialize_pff_entry_list(&serptr, *pel);
	}

	scs = (struct sdup_crypto_state **)pel;
	for (i = 0; i < numtables[msg->msg_type].sdup_crypto_states; i++, scs++) {
		serialize_sdup_crypto_state(&serptr, *scs);
	}

	gdp = (struct get_dif_prop_resp **)scs;
	for (i = 0; i < numtables[msg->msg_type].dif_properties; i++, gdp++) {
		serialize_get_dif_prop_resp(&serptr, *gdp);
	}

	inl = (struct ipcp_neigh_list **)gdp;
	for (i = 0; i < numtables[msg->msg_type].ipcp_neigh_lists; i++, inl++) {
		serialize_ipcp_neigh_list(&serptr, *inl);
	}

	mre = (struct media_report **)inl;
	for (i = 0; i < numtables[msg->msg_type].media_reports; i++, mre++) {
		serialize_media_report(&serptr, *mre);
	}

	bf = (const struct buffer **)mre;
	for (i = 0; i < numtables[msg->msg_type].buffers; i++, bf++) {
		serialize_buffer(&serptr, *bf);
	}

	serlen = serptr - serbuf;

	return serlen;
}
COMMON_EXPORT(serialize_irati_msg);

static void * allocate_irati_msg(irati_msg_t msg_t)
{
	switch(msg_t) {
	case RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST: {
		struct irati_kmsg_ipcm_assign_to_dif * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcm_assign_to_dif), 1);
		return result;
	}
	case RINA_C_IPCM_PLUGIN_LOAD_RESPONSE:
	case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE:
	case RINA_C_IPCM_DESTROY_IPCP_RESPONSE:
	case RINA_C_IPCM_CREATE_IPCP_RESPONSE:
	case RINA_C_IPCP_MANAGEMENT_SDU_WRITE_RESPONSE:
	case RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE:
	case RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE:
	case RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE:
	case RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE:
	case RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE:
	case RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE: {
		struct irati_msg_base_resp * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_base_resp), 1);
		return result;
	}
	case RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST: {
		struct irati_kmsg_ipcm_update_config * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcm_update_config), 1);
		return result;
	}
	case RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION:
	case RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION: {
		struct irati_kmsg_ipcp_dif_reg_not * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcp_dif_reg_not), 1);
		return result;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST:
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED:
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED:
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST: {
		struct irati_kmsg_ipcm_allocate_flow * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcm_allocate_flow), 1);
		return result;
	}
	case RINA_C_IPCP_CONN_UPDATE_RESULT:
	case RINA_C_IPCP_CONN_DESTROY_REQUEST:
	case RINA_C_IPCP_CONN_DESTROY_RESULT:
	case RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE:
	case RINA_C_IPCP_ALLOCATE_PORT_RESPONSE:
	case RINA_C_IPCP_DEALLOCATE_PORT_REQUEST:
	case RINA_C_IPCP_DEALLOCATE_PORT_RESPONSE:
	case RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST:
	case RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION:
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT: {
		struct irati_kmsg_multi_msg * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_multi_msg), 1);
		return result;
	}
	case RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE: {
		struct irati_kmsg_ipcm_allocate_flow_resp * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcm_allocate_flow_resp), 1);
		return result;
	}
	case RINA_C_IPCM_REGISTER_APPLICATION_REQUEST: {
		struct irati_kmsg_ipcm_reg_app * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcm_reg_app), 1);
		return result;
	}
	case RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST: {
		struct irati_kmsg_ipcm_unreg_app * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcm_unreg_app), 1);
		return result;
	}
	case RINA_C_IPCM_QUERY_RIB_REQUEST: {
		struct irati_kmsg_ipcm_query_rib * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcm_query_rib), 1);
		return result;
	}
	case RINA_C_IPCM_QUERY_RIB_RESPONSE: {
		struct irati_kmsg_ipcm_query_rib_resp * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcm_query_rib_resp), 1);
		return result;
	}
	case RINA_C_IPCM_SCAN_MEDIA_REQUEST:
	case RINA_C_IPCM_FINALIZE_REQUEST:
	case RINA_C_RMT_DUMP_FT_REQUEST: {
		struct irati_msg_base * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_base), 1);
		return result;
	}
	case RINA_C_RMT_MODIFY_FTE_REQUEST:
	case RINA_C_RMT_DUMP_FT_REPLY: {
		struct irati_kmsg_rmt_dump_ft * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_rmt_dump_ft), 1);
		return result;
	}
	case RINA_C_IPCP_CONN_CREATE_REQUEST:
	case RINA_C_IPCP_CONN_CREATE_ARRIVED: {
		struct irati_kmsg_ipcp_conn_create_arrived * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcp_conn_create_arrived), 1);
		return result;
	}
	case RINA_C_IPCP_CONN_CREATE_RESPONSE:
	case RINA_C_IPCP_CONN_CREATE_RESULT:
	case RINA_C_IPCP_CONN_MODIFY_REQUEST:
	case RINA_C_IPCP_CONN_UPDATE_REQUEST: {
		struct irati_kmsg_ipcp_conn_update * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcp_conn_update), 1);
		return result;
	}
	case RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST: {
		struct irati_kmsg_ipcp_select_ps_param * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcp_select_ps_param), 1);
		return result;
	}
	case RINA_C_IPCP_SELECT_POLICY_SET_REQUEST: {
		struct irati_kmsg_ipcp_select_ps * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcp_select_ps), 1);
		return result;
	}
	case RINA_C_IPCP_UPDATE_CRYPTO_STATE_REQUEST: {
		struct irati_kmsg_ipcp_update_crypto_state * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcp_update_crypto_state), 1);
		return result;
	}
	case RINA_C_IPCP_ADDRESS_CHANGE_REQUEST: {
		struct irati_kmsg_ipcp_address_change * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcp_address_change), 1);
		return result;
	}
	case RINA_C_IPCP_ALLOCATE_PORT_REQUEST: {
		struct irati_kmsg_ipcp_allocate_port * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcp_allocate_port), 1);
		return result;
	}
	case RINA_C_IPCP_MANAGEMENT_SDU_WRITE_REQUEST:
	case RINA_C_IPCP_MANAGEMENT_SDU_READ_NOTIF: {
		struct irati_kmsg_ipcp_mgmt_sdu * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcp_mgmt_sdu), 1);
		return result;
	}
	case RINA_C_IPCM_CREATE_IPCP_REQUEST: {
		struct irati_kmsg_ipcm_create_ipcp * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcm_create_ipcp), 1);
		return result;
	}
	case RINA_C_IPCM_DESTROY_IPCP_REQUEST: {
		struct irati_kmsg_ipcm_destroy_ipcp * result;
		result = COMMON_ALLOC(sizeof(struct irati_kmsg_ipcm_destroy_ipcp), 1);
		return result;
	}
	case RINA_C_IPCM_ENROLL_TO_DIF_REQUEST: {
		struct irati_msg_ipcm_enroll_to_dif * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_ipcm_enroll_to_dif), 1);
		return result;
	}
	case RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE: {
		struct irati_msg_ipcm_enroll_to_dif_resp * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_ipcm_enroll_to_dif_resp), 1);
		return result;
	}
	case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST:
	case RINA_C_IPCM_IPC_PROCESS_INITIALIZED: {
		struct irati_msg_with_name * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_with_name), 1);
		return result;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT: {
		struct irati_msg_app_alloc_flow_result * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_app_alloc_flow_result), 1);
		return result;
	}
	case RINA_C_APP_ALLOCATE_FLOW_RESPONSE: {
		struct irati_msg_app_alloc_flow_response * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_app_alloc_flow_response), 1);
		return result;
	}
	case RINA_C_APP_DEALLOCATE_FLOW_REQUEST:
	case RINA_C_APP_REGISTER_APPLICATION_REQUEST: {
		struct irati_msg_app_reg_app * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_app_reg_app), 1);
		return result;
	}
	case RINA_C_APP_UNREGISTER_APPLICATION_REQUEST:
	case RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE:
	case RINA_C_APP_GET_DIF_PROPERTIES_REQUEST:
	case RINA_C_APP_REGISTER_APPLICATION_RESPONSE: {
		struct irati_msg_app_reg_app_resp * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_app_reg_app_resp), 1);
		return result;
	}
	case RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION: {
		struct irati_msg_app_reg_cancel * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_app_reg_cancel), 1);
		return result;
	}
	case RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE: {
		struct irati_msg_get_dif_prop * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_get_dif_prop), 1);
		return result;
	}
	case RINA_C_IPCM_PLUGIN_LOAD_REQUEST: {
		struct irati_msg_ipcm_plugin_load * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_ipcm_plugin_load), 1);
		return result;
	}
	case RINA_C_IPCM_FWD_CDAP_MSG_REQUEST:
	case RINA_C_IPCM_FWD_CDAP_MSG_RESPONSE: {
		struct irati_msg_ipcm_fwd_cdap_msg * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_ipcm_fwd_cdap_msg), 1);
		return result;
	}
	case RINA_C_IPCM_MEDIA_REPORT: {
		struct irati_msg_ipcm_media_report * result;
		result = COMMON_ALLOC(sizeof(struct irati_msg_ipcm_media_report), 1);
		return result;
	}
	}

	return 0;
}

void * deserialize_irati_msg(struct irati_msg_layout *numtables,
			     size_t num_entries,
			     const void *serbuf,
			     unsigned int serbuf_len)
{
	struct irati_msg_base * bmsg = IRATI_MB(serbuf);
	void * msgbuf;
	struct name ** name;
	string_t ** str;
	struct buffer ** bf;
	struct flow_spec ** fspec;
	struct dif_config ** dif_config;
	struct dtp_config ** dtp_config;
	struct dtcp_config ** dtcp_config;
	struct query_rib_resp ** qrr;
	struct pff_entry_list ** pel;
	struct sdup_crypto_state ** scs;
	struct get_dif_prop_resp ** gdp;
	struct ipcp_neigh_list ** inl;
	struct media_report ** mre;
	unsigned int copylen;
	const void *desptr;
	int i;

	if (bmsg->msg_type >= num_entries) {
		return 0;
	}

	msgbuf = allocate_irati_msg(bmsg->msg_type);
	if (!msgbuf) {
		return 0;
	}

	copylen = numtables[bmsg->msg_type].copylen;
	memcpy(msgbuf, serbuf, copylen);

	desptr = serbuf + copylen;
	name = (struct name **)(msgbuf + copylen);
	for (i = 0; i < numtables[bmsg->msg_type].names; i++, name++) {
		if (deserialize_rina_name(&desptr, name)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	str = (string_t **) name;
	for (i = 0; i < numtables[bmsg->msg_type].strings; i++, str++) {
		if (deserialize_string(&desptr, str)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	fspec = (struct flow_spec **) str;
	for (i = 0; i < numtables[bmsg->msg_type].flow_specs; i++, fspec++) {
		if (deserialize_flow_spec(&desptr, fspec)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	dif_config = (struct dif_config **) fspec;
	for (i = 0; i < numtables[bmsg->msg_type].dif_configs; i++, dif_config++) {
		if (deserialize_dif_config(&desptr, dif_config)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	dtp_config = (struct dtp_config **) dif_config;
	for (i = 0; i < numtables[bmsg->msg_type].dtp_configs; i++, dtp_config++) {
		if (deserialize_dtp_config(&desptr, dtp_config)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	dtcp_config = (struct dtcp_config **) dtp_config;
	for (i = 0; i < numtables[bmsg->msg_type].dtcp_configs; i++, dtcp_config++) {
		if (deserialize_dtcp_config(&desptr, dtcp_config)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	qrr = (struct query_rib_resp **) dtcp_config;
	for (i = 0; i < numtables[bmsg->msg_type].query_rib_resps; i++, qrr++) {
		if (deserialize_query_rib_resp(&desptr, qrr)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	pel = (struct pff_entry_list **) qrr;
	for (i = 0; i < numtables[bmsg->msg_type].pff_entry_lists; i++, pel++) {
		if (deserialize_pff_entry_list(&desptr, pel)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	scs = (struct sdup_crypto_state **) pel;
	for (i = 0; i < numtables[bmsg->msg_type].sdup_crypto_states; i++, scs++) {
		if (deserialize_sdup_crypto_state(&desptr, scs)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	gdp = (struct get_dif_prop_resp **) scs;
	for (i = 0; i < numtables[bmsg->msg_type].dif_properties; i++, gdp++) {
		if (deserialize_get_dif_prop_resp(&desptr, gdp)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	inl = (struct ipcp_neigh_list **) gdp;
	for (i = 0; i < numtables[bmsg->msg_type].ipcp_neigh_lists; i++, inl++) {
		if (deserialize_ipcp_neigh_list(&desptr, inl)) {
			irati_msg_free(numtables, num_entries,
					(struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	mre = (struct media_report **) inl;
	for (i = 0; i < numtables[bmsg->msg_type].media_reports; i++, mre++) {
		if (deserialize_media_report(&desptr, mre)) {
			irati_msg_free(numtables, num_entries,
				       (struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	bf = (struct buffer **) mre;
	for (i = 0; i < numtables[bmsg->msg_type].buffers; i++, bf++) {
		if (deserialize_buffer(&desptr, bf)) {
			irati_msg_free(numtables, num_entries,
				       (struct irati_msg_base *) msgbuf);
			return 0;
		}
	}

	if ((desptr - serbuf) != serbuf_len) {
		irati_msg_free(numtables, num_entries,
			       (struct irati_msg_base *) msgbuf);
		return 0;
	}

	return msgbuf;
}
COMMON_EXPORT(deserialize_irati_msg);

unsigned int irati_msg_serlen(struct irati_msg_layout *numtables,
			      size_t num_entries,
			      const struct irati_msg_base *msg)
{
	unsigned int ret;
	struct name ** name;
	string_t ** str;
	struct flow_spec ** fspec;
	struct dif_config ** dif_config;
	struct dtp_config ** dtp_config;
	struct dtcp_config ** dtcp_config;
	struct query_rib_resp ** qrr;
	struct pff_entry_list ** pel;
	struct sdup_crypto_state ** scs;
	struct get_dif_prop_resp ** gdp;
	struct ipcp_neigh_list ** inl;
	struct media_report ** mre;
	const struct buffer ** bf;
	int i;

	if (msg->msg_type >= num_entries) {
		return -1;
	}

	ret = numtables[msg->msg_type].copylen;

	name = (struct name **)(((void *)msg) + ret);
	for (i = 0; i < numtables[msg->msg_type].names; i++, name++) {
		ret += rina_name_serlen(*name);
	}

	str = (string_t **)name;
	for (i = 0; i < numtables[msg->msg_type].strings; i++, str++) {
		ret += sizeof(uint16_t) + string_prlen(*str);
	}

	fspec = (struct flow_spec **)str;
	for (i = 0; i < numtables[msg->msg_type].flow_specs; i++, fspec++) {
		ret += flow_spec_serlen(*fspec);
	}

	dif_config = (struct dif_config **)fspec;
	for (i = 0; i < numtables[msg->msg_type].dif_configs; i++, dif_config++) {
		ret += dif_config_serlen(*dif_config);
	}

	dtp_config = (struct dtp_config **)dif_config;
	for (i = 0; i < numtables[msg->msg_type].dtp_configs; i++, dtp_config++) {
		ret += dtp_config_serlen(*dtp_config);
	}

	dtcp_config = (struct dtcp_config **)dtp_config;
	for (i = 0; i < numtables[msg->msg_type].dtcp_configs; i++, dtcp_config++) {
		ret += dtcp_config_serlen(*dtcp_config);
	}

	qrr = (struct query_rib_resp **)dtcp_config;
	for (i = 0; i < numtables[msg->msg_type].query_rib_resps; i++, qrr++) {
		ret += query_rib_resp_serlen(*qrr);
	}

	pel = (struct pff_entry_list **)qrr;
	for (i = 0; i < numtables[msg->msg_type].pff_entry_lists; i++, pel++) {
		ret += pff_entry_list_serlen(*pel);
	}

	scs = (struct sdup_crypto_state **)pel;
	for (i = 0; i < numtables[msg->msg_type].sdup_crypto_states; i++, scs++) {
		ret += sdup_crypto_state_serlen(*scs);
	}

	gdp = (struct get_dif_prop_resp **)scs;
	for (i = 0; i < numtables[msg->msg_type].dif_properties; i++, gdp++) {
		ret += get_dif_prop_resp_serlen(*gdp);
	}

	inl = (struct ipcp_neigh_list **)gdp;
	for (i = 0; i < numtables[msg->msg_type].ipcp_neigh_lists; i++, inl++) {
		ret += ipcp_neigh_list_serlen(*inl);
	}

	mre = (struct media_report **)inl;
	for (i = 0; i < numtables[msg->msg_type].media_reports; i++, mre++) {
		ret += media_report_serlen(*mre);
	}

	bf = (const struct buffer **)mre;
	for (i = 0; i < numtables[msg->msg_type].buffers; i++, bf++) {
		ret += sizeof((*bf)->size) + (*bf)->size;
	}

	return ret;
}
COMMON_EXPORT(irati_msg_serlen);

void irati_msg_free(struct irati_msg_layout *numtables, size_t num_entries,
                    struct irati_msg_base *msg)
{
	unsigned int copylen = numtables[msg->msg_type].copylen;
	struct name ** name;
	string_t ** str;
	struct flow_spec ** fspec;
	struct dif_config ** dif_config;
	struct dtp_config ** dtp_config;
	struct dtcp_config ** dtcp_config;
	struct query_rib_resp ** qrr;
	struct pff_entry_list ** pel;
	struct sdup_crypto_state ** scs;
	struct get_dif_prop_resp ** gdp;
	struct ipcp_neigh_list ** inl;
	struct media_report ** mre;
	struct buffer ** bf;
	int i;

	if (msg->msg_type >= num_entries) {
		return;
	}

	/* Skip the copiable part and scan all the names contained in
	 * the message. */
	name = (struct name **)(((void *)msg) + copylen);
	for (i = 0; i < numtables[msg->msg_type].names; i++, name++) {
		rina_name_free(*name);
	}

	str = (string_t **)(name);
	for (i = 0; i < numtables[msg->msg_type].strings; i++, str++) {
		if (str) {
			COMMON_FREE(*str);
		}
	}

	fspec = (struct flow_spec **)(str);
	for (i = 0; i < numtables[msg->msg_type].flow_specs; i++, fspec++) {
		flow_spec_free(*fspec);
	}

	dif_config = (struct dif_config **)(fspec);
	for (i = 0; i < numtables[msg->msg_type].dif_configs; i++, dif_config++) {
		dif_config_free(*dif_config);
	}

	dtp_config = (struct dtp_config **)(dif_config);
	for (i = 0; i < numtables[msg->msg_type].dtp_configs; i++, dtp_config++) {
		dtp_config_free(*dtp_config);
	}

	dtcp_config = (struct dtcp_config **)(dtp_config);
	for (i = 0; i < numtables[msg->msg_type].dtcp_configs; i++, dtcp_config++) {
		dtcp_config_free(*dtcp_config);
	}

	qrr = (struct query_rib_resp **)(dtcp_config);
	for (i = 0; i < numtables[msg->msg_type].query_rib_resps; i++, qrr++) {
		query_rib_resp_free(*qrr);
	}

	pel = (struct pff_entry_list **)(qrr);
	for (i = 0; i < numtables[msg->msg_type].pff_entry_lists; i++, pel++) {
		pff_entry_list_free(*pel);
	}

	scs = (struct sdup_crypto_state **)(pel);
	for (i = 0; i < numtables[msg->msg_type].sdup_crypto_states; i++, scs++) {
		sdup_crypto_state_free(*scs);
	}

	gdp = (struct get_dif_prop_resp **)(scs);
	for (i = 0; i < numtables[msg->msg_type].dif_properties; i++, gdp++) {
		get_dif_prop_resp_free(*gdp);
	}

	inl = (struct ipcp_neigh_list **)(gdp);
	for (i = 0; i < numtables[msg->msg_type].ipcp_neigh_lists; i++, inl++) {
		ipcp_neigh_list_free(*inl);
	}

	mre = (struct media_report **)(inl);
	for (i = 0; i < numtables[msg->msg_type].media_reports; i++, mre++) {
		media_report_free(*mre);
	}

	bf = (struct buffer **)(mre);
	for (i = 0; i < numtables[msg->msg_type].buffers; i++, bf++) {
		buffer_destroy(*bf);
	}
}
COMMON_EXPORT(irati_msg_free);

unsigned int irati_numtables_max_size(struct irati_msg_layout *numtables,
				      unsigned int n)
{
	unsigned int max = 0;
	int i = 0;

	for (i = 0; i < n; i++) {
		unsigned int cur = numtables[i].copylen +
				numtables[i].names * sizeof(struct name) +
				numtables[i].strings * sizeof(char *) +
				numtables[i].flow_specs * sizeof(struct flow_spec) +
				numtables[i].dif_configs * sizeof(struct dif_config) +
				numtables[i].dtp_configs * sizeof(struct dtp_config) +
				numtables[i].dtcp_configs * sizeof(struct dtcp_config) +
				numtables[i].query_rib_resps * sizeof(struct query_rib_resp) +
				numtables[i].pff_entry_lists * sizeof(struct pff_entry_list) +
				numtables[i].sdup_crypto_states * sizeof(struct sdup_crypto_state) +
				numtables[i].dif_properties * sizeof(struct get_dif_prop_resp) +
				numtables[i].ipcp_neigh_lists * sizeof(struct ipcp_neigh_list) +
				numtables[i].media_reports * sizeof(struct media_report);

		if (cur > max) {
			max = cur;
		}
	}

	return max;
}
