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

#define RINA_PREFIX "serdes-utils"

#ifdef __KERNEL__

#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "logs.h"
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

#include "librina/logs.h"

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
	if (slen) {
		*s = COMMON_ALLOC(slen + 1, 1);
		if (!(*s)) {
			return -1;
		}

		memcpy(*s, *pptr, slen);
		(*s)[slen] = '\0';
		*pptr += slen;
	} else {
		*s = NULL;
	}

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
		serialize_obj(*pptr, size_t, 0);
		return;
	}

	serialize_obj(*pptr, size_t, b->size);
	memcpy(*pptr, b->data, b->size);
	*pptr += b->size;
}

int deserialize_buffer(const void **pptr, struct buffer **b)
{
	size_t blen;

	deserialize_obj(*pptr, size_t, &blen);
	if (blen) {
		*b = COMMON_ALLOC(blen, 1);
		if (!(*b)) {
			return -1;
		}

		memcpy((*b)->data, *pptr, blen);
		(*b)->size = blen;
		*pptr += blen;
	} else {
		*b = NULL;
	}

	return 0;
}

/* Serialize a RINA name. */
void serialize_rina_name(void **pptr, const struct name *name)
{
	serialize_string(pptr, name->process_name);
	serialize_string(pptr, name->process_instance);
	serialize_string(pptr, name->entity_name);
	serialize_string(pptr, name->entity_instance);
}

/* Deserialize a RINA name. */
int deserialize_rina_name(const void **pptr, struct name *name)
{
	int ret;

	memset(name, 0, sizeof(*name));

	ret = deserialize_string(pptr, &name->process_name);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &name->process_instance);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &name->entity_name);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &name->entity_instance);

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

	*cur = ':';
	cur++;

	memcpy(cur, name->process_instance, api_len);
	cur += api_len;

	*cur = ':';
	cur++;

	memcpy(cur, name->entity_name, aen_len);
	cur += aen_len;

	*cur = ':';
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
		LOG_ERR("FAILED\n");
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

	apn = strsep(strp, ":");
	api = strsep(strp, ":");
	aen = strsep(strp, ":");
	aei = strsep(strp, ":");

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

int flow_spec_serlen(const struct flow_spec * fspec)
{
	return 8 * sizeof(uint32_t) + sizeof(int32_t) + 2* sizeof(bool);
}

void serialize_flow_spec(void **pptr, const struct flow_spec *fspec)
{
	serialize_obj(*pptr, uint32_t, fspec->average_bandwidth);
	serialize_obj(*pptr, uint32_t, fspec->average_sdu_bandwidth);
	serialize_obj(*pptr, uint32_t, fspec->delay);
	serialize_obj(*pptr, uint32_t, fspec->jitter);
	serialize_obj(*pptr, int32_t, fspec->max_allowable_gap);
	serialize_obj(*pptr, uint32_t, fspec->max_sdu_size);
	serialize_obj(*pptr, bool, fspec->ordered_delivery);
	serialize_obj(*pptr, bool, fspec->partial_delivery);
	serialize_obj(*pptr, uint32_t, fspec->peak_bandwidth_duration);
	serialize_obj(*pptr, uint32_t, fspec->peak_sdu_bandwidth_duration);
	serialize_obj(*pptr, int32_t, fspec->undetected_bit_error_rate);
}

int deserialize_flow_spec(const void **pptr, struct flow_spec *fspec)
{
	memset(fspec, 0, sizeof(*fspec));

	deserialize_obj(*pptr, uint32_t, &fspec->average_bandwidth);
	deserialize_obj(*pptr, uint32_t, &fspec->average_sdu_bandwidth);
	deserialize_obj(*pptr, uint32_t, &fspec->delay);
	deserialize_obj(*pptr, uint32_t, &fspec->jitter);
	deserialize_obj(*pptr, int32_t, &fspec->max_allowable_gap);
	deserialize_obj(*pptr, uint32_t, &fspec->max_sdu_size);
	deserialize_obj(*pptr, bool, &fspec->ordered_delivery);
	deserialize_obj(*pptr, bool, &fspec->partial_delivery);
	deserialize_obj(*pptr, uint32_t, &fspec->peak_bandwidth_duration);
	deserialize_obj(*pptr, uint32_t, &fspec->peak_sdu_bandwidth_duration);
	deserialize_obj(*pptr, int32_t, &fspec->undetected_bit_error_rate);

	return 0;
}

void flow_spec_free(struct flow_spec * fspec)
{
	if (!fspec)
		return;

	COMMON_FREE(fspec);
}

int policy_parm_serlen(const struct policy_parm * prm)
{
	unsigned int ret = 2 * sizeof(uint16_t);

	if (!prm) {
		return ret;
	}

	return ret + string_prlen(prm->name) + string_prlen(prm->value);
}

void serialize_policy_parm(void **pptr, const struct policy_parm *prm)
{
	serialize_string(pptr, prm->name);
	serialize_string(pptr, prm->value);
}

int deserialize_policy_parm(const void **pptr, struct policy_parm *prm)
{
	int ret;

	memset(prm, 0, sizeof(*prm));

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

int policy_serlen(const struct policy * policy)
{
	struct policy_parm * pos;

	unsigned int ret = 2 * sizeof(uint16_t);

	if (!policy) {
		return ret;
	}

	ret = ret + string_prlen(policy->name)
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

	memset(policy, 0, sizeof(*policy));

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
		pos = COMMON_ALLOC(sizeof(struct policy_parm), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
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

int dtp_config_serlen(const struct dtp_config * dtp_config)
{
	return 4 * sizeof(bool) + sizeof(int) + sizeof(timeout_t)
	         + sizeof(seq_num_t) + policy_serlen(dtp_config->dtp_ps);
}

void serialize_dtp_config(void **pptr, const struct dtp_config *dtp_config)
{
	serialize_obj(*pptr, bool, dtp_config->dtcp_present);
	serialize_obj(*pptr, int, dtp_config->seq_num_ro_th);
	serialize_obj(*pptr, timeout_t, dtp_config->initial_a_timer);
	serialize_obj(*pptr, bool, dtp_config->partial_delivery);
	serialize_obj(*pptr, bool, dtp_config->incomplete_delivery);
	serialize_obj(*pptr, bool, dtp_config->in_order_delivery);
	serialize_obj(*pptr, seq_num_t, dtp_config->max_sdu_gap);
	serialize_policy(pptr, dtp_config->dtp_ps);
}

int deserialize_dtp_config(const void **pptr, struct dtp_config *dtp_config)
{
	memset(dtp_config, 0, sizeof(*dtp_config));

	deserialize_obj(*pptr, bool, &dtp_config->dtcp_present);
	deserialize_obj(*pptr, int, &dtp_config->seq_num_ro_th);
	deserialize_obj(*pptr, timeout_t, &dtp_config->initial_a_timer);
	deserialize_obj(*pptr, bool, &dtp_config->partial_delivery);
	deserialize_obj(*pptr, bool, &dtp_config->incomplete_delivery);
	deserialize_obj(*pptr, bool, &dtp_config->in_order_delivery);
	deserialize_obj(*pptr, seq_num_t, &dtp_config->max_sdu_gap);

	dtp_config->dtp_ps = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!dtp_config->dtp_ps) {
		return -1;
	}

	INIT_LIST_HEAD(&dtp_config->dtp_ps->params);
	return deserialize_policy(pptr, dtp_config->dtp_ps);
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

int window_fctrl_config_serlen(const struct window_fctrl_config * wfc)
{
	return 2 * sizeof(uint32_t) + policy_serlen(wfc->rcvr_flow_control)
				    + policy_serlen(wfc->tx_control);
}

void serialize_window_fctrl_config(void **pptr,
				   const struct window_fctrl_config *wfc)
{
	serialize_obj(*pptr, uint32_t, wfc->initial_credit);
	serialize_obj(*pptr, uint32_t, wfc->max_closed_winq_length);
	serialize_policy(pptr, wfc->rcvr_flow_control);
	serialize_policy(pptr, wfc->tx_control);
}

int deserialize_window_fctrl_config(const void **pptr,
				    struct window_fctrl_config *wfc)
{
	int ret;

	memset(wfc, 0, sizeof(*wfc));

	deserialize_obj(*pptr, uint32_t, &wfc->initial_credit);
	deserialize_obj(*pptr, uint32_t, &wfc->max_closed_winq_length);

	wfc->rcvr_flow_control = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!wfc->rcvr_flow_control) {
		return -1;
	}

	INIT_LIST_HEAD(&wfc->rcvr_flow_control->params);
	ret = deserialize_policy(pptr, wfc->rcvr_flow_control);
	if (ret) {
		return ret;
	}

	wfc->tx_control = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!wfc->tx_control) {
		return -1;
	}

	INIT_LIST_HEAD(&wfc->tx_control->params);
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

int rate_fctrl_config_serlen(const struct rate_fctrl_config * rfc)
{
	return 2 * sizeof(uint32_t) + policy_serlen(rfc->rate_reduction)
                 + policy_serlen(rfc->no_rate_slow_down)
		 + policy_serlen(rfc->no_override_default_peak);
}

void serialize_rate_fctrl_config(void **pptr, const struct rate_fctrl_config *rfc)
{
	serialize_obj(*pptr, uint32_t, rfc->sending_rate);
	serialize_obj(*pptr, uint32_t, rfc->time_period);
	serialize_policy(pptr, rfc->no_override_default_peak);
	serialize_policy(pptr, rfc->no_rate_slow_down);
	serialize_policy(pptr, rfc->rate_reduction);
}

int deserialize_rate_fctrl_config(const void **pptr, struct rate_fctrl_config *rfc)
{
	int ret;

	memset(rfc, 0, sizeof(*rfc));

	deserialize_obj(*pptr, uint32_t, &rfc->sending_rate);
	deserialize_obj(*pptr, uint32_t, &rfc->time_period);

	rfc->no_override_default_peak = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!rfc->no_override_default_peak) {
		return -1;
	}

	INIT_LIST_HEAD(&rfc->no_override_default_peak->params);
	ret = deserialize_policy(pptr, rfc->no_override_default_peak);
	if (ret) {
		return ret;
	}

	rfc->no_rate_slow_down = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!rfc->no_rate_slow_down) {
		return -1;
	}

	INIT_LIST_HEAD(&rfc->no_rate_slow_down->params);
	ret = deserialize_policy(pptr, rfc->no_rate_slow_down);
	if (ret) {
		return ret;
	}

	rfc->rate_reduction = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!rfc->rate_reduction) {
		return -1;
	}

	INIT_LIST_HEAD(&rfc->rate_reduction->params);
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

int dtcp_fctrl_config_serlen(const struct dtcp_fctrl_config * dfc)
{
	int ret;

	ret = 6 * sizeof(uint32_t) + 2 * sizeof(bool) +
		 + policy_serlen(dfc->closed_window)
                 + policy_serlen(dfc->receiving_flow_control)
		 + policy_serlen(dfc->reconcile_flow_conflict);

	if (dfc->window_based_fctrl)
		ret = ret + window_fctrl_config_serlen(dfc->wfctrl_cfg);

	if (dfc->rate_based_fctrl)
		ret = ret + rate_fctrl_config_serlen(dfc->rfctrl_cfg);

	return ret;
}

void serialize_dtcp_fctrl_config(void **pptr, const struct dtcp_fctrl_config *dfc)
{
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

	if (dfc->window_based_fctrl)
		serialize_window_fctrl_config(pptr, dfc->wfctrl_cfg);

	if (dfc->rate_based_fctrl)
		serialize_rate_fctrl_config(pptr, dfc->rfctrl_cfg);
}

int deserialize_dtcp_fctrl_config(const void **pptr, struct dtcp_fctrl_config *dfc)
{
	int ret;

	memset(dfc, 0, sizeof(*dfc));

	deserialize_obj(*pptr, uint32_t, &dfc->rcvd_buffers_th);
	deserialize_obj(*pptr, uint32_t, &dfc->rcvd_bytes_percent_th);
	deserialize_obj(*pptr, uint32_t, &dfc->rcvd_bytes_th);
	deserialize_obj(*pptr, uint32_t, &dfc->sent_buffers_th);
	deserialize_obj(*pptr, uint32_t, &dfc->sent_bytes_percent_th);
	deserialize_obj(*pptr, uint32_t, &dfc->sent_bytes_th);
	deserialize_obj(*pptr, bool, &dfc->window_based_fctrl);
	deserialize_obj(*pptr, bool, &dfc->rate_based_fctrl);

	dfc->closed_window = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!dfc->closed_window) {
		return -1;
	}

	INIT_LIST_HEAD(&dfc->closed_window->params);
	ret = deserialize_policy(pptr, dfc->closed_window);
	if (ret) {
		return ret;
	}

	dfc->receiving_flow_control = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!dfc->receiving_flow_control) {
		return -1;
	}

	INIT_LIST_HEAD(&dfc->receiving_flow_control->params);
	ret = deserialize_policy(pptr, dfc->receiving_flow_control);
	if (ret) {
		return ret;
	}

	dfc->reconcile_flow_conflict = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!dfc->reconcile_flow_conflict) {
		return -1;
	}

	INIT_LIST_HEAD(&dfc->reconcile_flow_conflict->params);
	ret = deserialize_policy(pptr, dfc->reconcile_flow_conflict);
	if (ret) {
		return ret;
	}

	if (dfc->window_based_fctrl) {
		dfc->wfctrl_cfg = COMMON_ALLOC(sizeof(struct window_fctrl_config), 1);
		if (!dfc->wfctrl_cfg) {
			return -1;
		}

		ret = deserialize_window_fctrl_config(pptr, dfc->wfctrl_cfg);
		if (ret) {
			return ret;
		}
	}

	if (dfc->rate_based_fctrl) {
		dfc->rfctrl_cfg = COMMON_ALLOC(sizeof(struct rate_fctrl_config), 1);
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

int dtcp_rxctrl_config_serlen(const struct dtcp_rxctrl_config * rxfc)
{
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

	memset(rxfc, 0, sizeof(*rxfc));

	deserialize_obj(*pptr, uint32_t, &rxfc->data_retransmit_max);
	deserialize_obj(*pptr, uint32_t, &rxfc->initial_tr);
	deserialize_obj(*pptr, uint32_t, &rxfc->max_time_retry);

	rxfc->rcvr_ack = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!rxfc->rcvr_ack) {
		return -1;
	}

	INIT_LIST_HEAD(&rxfc->rcvr_ack->params);
	ret = deserialize_policy(pptr, rxfc->rcvr_ack);
	if (ret) {
		return ret;
	}

	rxfc->rcvr_control_ack = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!rxfc->rcvr_control_ack) {
		return -1;
	}

	INIT_LIST_HEAD(&rxfc->rcvr_control_ack->params);
	ret = deserialize_policy(pptr, rxfc->rcvr_control_ack);
	if (ret) {
		return ret;
	}

	rxfc->receiving_ack_list = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!rxfc->receiving_ack_list) {
		return -1;
	}

	INIT_LIST_HEAD(&rxfc->receiving_ack_list->params);
	ret = deserialize_policy(pptr, rxfc->receiving_ack_list);
	if (ret) {
		return ret;
	}

	rxfc->retransmission_timer_expiry = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!rxfc->retransmission_timer_expiry) {
		return -1;
	}

	INIT_LIST_HEAD(&rxfc->retransmission_timer_expiry->params);
	ret = deserialize_policy(pptr, rxfc->retransmission_timer_expiry);
	if (ret) {
		return ret;
	}

	rxfc->sender_ack = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!rxfc->sender_ack) {
		return -1;
	}

	INIT_LIST_HEAD(&rxfc->sender_ack->params);
	ret = deserialize_policy(pptr, rxfc->sender_ack);
	if (ret) {
		return ret;
	}

	rxfc->sending_ack = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!rxfc->sending_ack) {
		return -1;
	}

	INIT_LIST_HEAD(&rxfc->sending_ack->params);
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

int dtcp_config_serlen(const struct dtcp_config * dtcp_config)
{
	int ret;

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

int deserialize_dtcp_config(const void **pptr, struct dtcp_config *dtcp_config)
{
	int ret;

	memset(dtcp_config, 0, sizeof(*dtcp_config));

	deserialize_obj(*pptr, bool, &dtcp_config->flow_ctrl);
	deserialize_obj(*pptr, bool, &dtcp_config->rtx_ctrl);

	dtcp_config->dtcp_ps = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!dtcp_config->dtcp_ps) {
		return -1;
	}

	INIT_LIST_HEAD(&dtcp_config->dtcp_ps->params);
	ret = deserialize_policy(pptr, dtcp_config->dtcp_ps);
	if (ret) {
		return ret;
	}

	dtcp_config->lost_control_pdu = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!dtcp_config->lost_control_pdu) {
		return -1;
	}

	INIT_LIST_HEAD(&dtcp_config->lost_control_pdu->params);
	ret = deserialize_policy(pptr, dtcp_config->lost_control_pdu);
	if (ret) {
		return ret;
	}

	dtcp_config->rtt_estimator = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!dtcp_config->rtt_estimator) {
		return -1;
	}

	INIT_LIST_HEAD(&dtcp_config->rtt_estimator->params);
	ret = deserialize_policy(pptr, dtcp_config->rtt_estimator);
	if (ret) {
		return ret;
	}

	if (dtcp_config->flow_ctrl) {
		dtcp_config->fctrl_cfg = COMMON_ALLOC(sizeof(struct dtcp_fctrl_config), 1);
		if (!dtcp_config->fctrl_cfg)
			return -1;

		ret = deserialize_dtcp_fctrl_config(pptr, dtcp_config->fctrl_cfg);
		if (ret)
			return ret;
	}

	if (dtcp_config->rtx_ctrl) {
		dtcp_config->rxctrl_cfg = COMMON_ALLOC(sizeof(struct dtcp_rxctrl_config), 1);
		if (!dtcp_config->rxctrl_cfg)
			return -1;

		ret = deserialize_dtcp_rxctrl_config(pptr, dtcp_config->rxctrl_cfg);
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

int pff_config_serlen(const struct pff_config * pff)
{
	return policy_serlen(pff->policy_set);
}

void serialize_pff_config(void **pptr, const struct pff_config *pff)
{
	serialize_policy(pptr, pff->policy_set);
}

int deserialize_pff_config(const void **pptr, struct pff_config *pff)
{
	memset(pff, 0, sizeof(*pff));

	pff->policy_set = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!pff->policy_set) {
		return -1;
	}

	INIT_LIST_HEAD(&pff->policy_set->params);
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

int rmt_config_serlen(const struct rmt_config * rmt)
{
	return policy_serlen(rmt->policy_set)
		+ pff_config_serlen(rmt->pff_conf);
}

void serialize_rmt_config(void **pptr, const struct rmt_config *rmt)
{
	serialize_policy(pptr, rmt->policy_set);
	serialize_pff_config(pptr, rmt->pff_conf);
}

int deserialize_rmt_config(const void **pptr, struct rmt_config *rmt)
{
	int ret;

	memset(rmt, 0, sizeof(*rmt));

	rmt->policy_set = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!rmt->policy_set) {
		return -1;
	}

	INIT_LIST_HEAD(&rmt->policy_set->params);
	ret = deserialize_policy(pptr, rmt->policy_set);
	if (ret)
		return ret;

	rmt->pff_conf = COMMON_ALLOC(sizeof(struct pff_config), 1);
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

int dup_config_entry_serlen(const struct dup_config_entry * dce)
{
	return sizeof(uint16_t) + string_prlen(dce->n_1_dif_name)
			+ policy_serlen(dce->crypto_policy)
			+ policy_serlen(dce->error_check_policy)
			+ policy_serlen(dce->ttl_policy);
}

void serialize_dup_config_entry(void **pptr, const struct dup_config_entry *dce)
{
	serialize_string(pptr, dce->n_1_dif_name);
	serialize_policy(pptr, dce->crypto_policy);
	serialize_policy(pptr, dce->error_check_policy);
	serialize_policy(pptr, dce->ttl_policy);
}

int deserialize_dup_config_entry(const void **pptr, struct dup_config_entry *dce)
{
	int ret;

	memset(dce, 0, sizeof(*dce));

	ret = deserialize_string(pptr, &dce->n_1_dif_name);
	if (ret) {
		return ret;
	}

	dce->crypto_policy = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!dce->crypto_policy) {
		return -1;
	}

	INIT_LIST_HEAD(&dce->crypto_policy->params);
	ret = deserialize_policy(pptr, dce->crypto_policy);
	if (ret)
		return ret;

	dce->error_check_policy = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!dce->error_check_policy) {
		return -1;
	}

	INIT_LIST_HEAD(&dce->error_check_policy->params);
	ret = deserialize_policy(pptr, dce->error_check_policy);
	if (ret)
		return ret;

	dce->ttl_policy = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!dce->ttl_policy) {
		return -1;
	}

	INIT_LIST_HEAD(&dce->ttl_policy->params);
	return deserialize_policy(pptr, dce->ttl_policy);
}

void dup_config_entry_free(struct dup_config_entry * dce)
{
	if (!dce)
		return;

	if (dce->crypto_policy) {
		policy_free(dce->crypto_policy);
		dce->crypto_policy = 0;
	}

	if (dce->error_check_policy) {
		policy_free(dce->error_check_policy);
		dce->error_check_policy = 0;
	}

	if (dce->ttl_policy) {
		policy_free(dce->ttl_policy);
		dce->ttl_policy = 0;
	}

	if (dce->n_1_dif_name) {
		COMMON_FREE(dce->n_1_dif_name);
		dce->n_1_dif_name = 0;
	}

	COMMON_FREE(dce);
}

int sdup_config_serlen(const struct sdup_config * sdc)
{
	int ret;
	struct dup_config * pos;

	ret = dup_config_entry_serlen(sdc->default_dup_conf)
		  + sizeof(uint16_t);

        list_for_each_entry(pos, &(sdc->specific_dup_confs), next) {
                ret = ret + dup_config_entry_serlen(pos->entry);
        }

        return ret;
}

void serialize_sdup_config(void **pptr, const struct sdup_config *sdc)
{
	struct dup_config * pos;
	uint16_t num_parms;

	serialize_dup_config_entry(pptr, sdc->default_dup_conf);

	num_parms = 0;
	list_for_each_entry(pos, &(sdc->specific_dup_confs), next) {
		num_parms ++;
	}

	serialize_obj(*pptr, uint16_t, num_parms);

	list_for_each_entry(pos, &(sdc->specific_dup_confs), next) {
		serialize_dup_config_entry(pptr, pos->entry);
	}
}

int deserialize_sdup_config(const void **pptr, struct sdup_config *sdc)
{
	int ret;
	struct dup_config * pos;
	uint16_t num_attrs;
	int i;

	memset(sdc, 0, sizeof(*sdc));

	sdc->default_dup_conf = COMMON_ALLOC(sizeof(struct dup_config_entry), 1);
	if (!sdc->default_dup_conf) {
		return -1;
	}

	ret = deserialize_dup_config_entry(pptr, sdc->default_dup_conf);
	if (ret)
		return ret;

	deserialize_obj(*pptr, uint16_t, &num_attrs);
	for(i = 0; i < num_attrs; i++) {
		pos = COMMON_ALLOC(sizeof(struct dup_config), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		pos->entry = COMMON_ALLOC(sizeof(struct dup_config_entry), 1);
		if (!pos->entry) {
			return -1;
		}

		ret = deserialize_dup_config_entry(pptr, pos->entry);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &sdc->specific_dup_confs);
	}

	return ret;
}

void sdup_config_free(struct sdup_config * sdc)
{
	struct dup_config * pos, * npos;

	if (!sdc)
		return;

	if (sdc->default_dup_conf) {
		dup_config_entry_free(sdc->default_dup_conf);
		sdc->default_dup_conf = 0;
	}

	list_for_each_entry_safe(pos, npos, &sdc->specific_dup_confs, next) {
		list_del(&pos->next);
		if (pos->entry) {
			dup_config_entry_free(pos->entry);
			pos->entry = 0;
		}

		COMMON_FREE(pos);
	}

	COMMON_FREE(sdc);
}

int dt_cons_serlen(const struct dt_cons * dtc)
{
	return 9 * sizeof(uint16_t) + 2 * sizeof(uint32_t) + sizeof(bool);
}

void serialize_dt_cons(void **pptr, const struct dt_cons *dtc)
{
	serialize_obj(*pptr, uint16_t, dtc->address_length);
	serialize_obj(*pptr, uint16_t, dtc->cep_id_length);
	serialize_obj(*pptr, uint16_t, dtc->ctrl_seq_num_length);
	serialize_obj(*pptr, bool, dtc->dif_integrity);
	serialize_obj(*pptr, uint16_t, dtc->frame_length);
	serialize_obj(*pptr, uint16_t, dtc->length_length);
	serialize_obj(*pptr, uint32_t, dtc->max_pdu_life);
	serialize_obj(*pptr, uint32_t, dtc->max_pdu_size);
	serialize_obj(*pptr, uint16_t, dtc->port_id_length);
	serialize_obj(*pptr, uint16_t, dtc->qos_id_length);
	serialize_obj(*pptr, uint16_t, dtc->rate_length);
	serialize_obj(*pptr, uint16_t, dtc->seq_num_length);
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
	deserialize_obj(*pptr, uint16_t, &dtc->port_id_length);
	deserialize_obj(*pptr, uint16_t, &dtc->qos_id_length);
	deserialize_obj(*pptr, uint16_t, &dtc->rate_length);
	deserialize_obj(*pptr, uint16_t, &dtc->seq_num_length);

	return 0;
}

void dt_cons_free(struct dt_cons * dtc)
{
	if (!dtc)
		return;

	COMMON_FREE(dtc);
}

int efcp_config_serlen(const struct efcp_config * efc)
{
	return dt_cons_serlen(efc->dt_cons) + policy_serlen(efc->unknown_flow)
			+ sizeof(uint8_t) + sizeof(ssize_t);
}

void serialize_efcp_config(void **pptr, const struct efcp_config *efc)
{
	uint8_t size = 0;

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
}

int deserialize_efcp_config(const void **pptr, struct efcp_config *efc)
{
	int ret;
	uint8_t size;

	memset(efc, 0, sizeof(*efc));

	efc->dt_cons = COMMON_ALLOC(sizeof(struct dt_cons), 1);
	if (!efc->dt_cons) {
		return -1;
	}

	ret = deserialize_dt_cons(pptr, efc->dt_cons);
	if (ret)
		return ret;

	efc->unknown_flow = COMMON_ALLOC(sizeof(struct policy), 1);
	if (!efc->unknown_flow) {
		return -1;
	}

	INIT_LIST_HEAD(&efc->unknown_flow->params);
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

	return ret;
}

void efcp_config_free(struct efcp_config * efc)
{
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

	COMMON_FREE(efc);
}

int ipcp_config_entry_serlen(const struct ipcp_config_entry * ice)
{
	return 2*sizeof(uint16_t) + string_prlen(ice->name)
				  + string_prlen(ice->value);
}

void serialize_ipcp_config_entry(void **pptr, const struct ipcp_config_entry *ice)
{
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

int dif_config_serlen(const struct dif_config * dif_config)
{
	int ret;
	struct ipcp_config * pos;

	ret = sizeof(address_t) + sizeof(uint16_t);

        list_for_each_entry(pos, &(dif_config->ipcp_config_entries), next) {
                ret = ret + ipcp_config_entry_serlen(pos->entry);
        }

        if (dif_config->efcp_config)
        	ret = ret + efcp_config_serlen(dif_config->efcp_config);

        if (dif_config->rmt_config)
        	ret = ret + rmt_config_serlen(dif_config->rmt_config);

        if (dif_config->sdup_config)
        	ret = ret + sdup_config_serlen(dif_config->sdup_config);

        return ret;
}

void serialize_dif_config(void **pptr, const struct dif_config *dif_config)
{
	struct ipcp_config * pos;
	uint16_t size = 0;

	serialize_obj(*pptr, address_t, dif_config->address);

	list_for_each_entry(pos, &(dif_config->ipcp_config_entries), next) {
		size ++;
	}

	serialize_obj(*pptr, uint16_t, size);

	list_for_each_entry(pos, &(dif_config->ipcp_config_entries), next) {
		serialize_ipcp_config_entry(pptr, pos->entry);
	}

	if (dif_config->efcp_config)
		serialize_efcp_config(pptr, dif_config->efcp_config);
	if (dif_config->rmt_config)
		serialize_rmt_config(pptr, dif_config->rmt_config);
	if (dif_config->sdup_config)
		serialize_sdup_config(pptr, dif_config->sdup_config);
}

int deserialize_dif_config(const void **pptr, struct dif_config *dif_config)
{
	int ret;
	struct ipcp_config * pos;
	uint16_t size;
	int i;

	memset(dif_config, 0, sizeof(*dif_config));

	deserialize_obj(*pptr, address_t, &dif_config->address);
	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = COMMON_ALLOC(sizeof(struct ipcp_config), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		pos->entry = COMMON_ALLOC(sizeof(struct ipcp_config_entry), 1);
		if (!pos->entry) {
			return -1;
		}

		ret = deserialize_ipcp_config_entry(pptr, pos->entry);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &dif_config->ipcp_config_entries);
	}

	deserialize_efcp_config(pptr, dif_config->efcp_config);
	deserialize_rmt_config(pptr, dif_config->rmt_config);
	deserialize_sdup_config(pptr, dif_config->sdup_config);

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

	if (dif_config->sdup_config) {
		sdup_config_free(dif_config->sdup_config);
		dif_config->sdup_config = 0;
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

int rib_object_data_serlen(const struct rib_object_data * rod)
{
	return 3 * sizeof(uint16_t) + string_prlen(rod->name)
			+ string_prlen(rod->class)
			+ string_prlen(rod->disp_value)
			+ sizeof(uint64_t);
}

void serialize_rib_object_data(void **pptr, const struct rib_object_data *rod)
{
	serialize_obj(*pptr, uint64_t, rod->instance);
	serialize_string(pptr, rod->name);
	serialize_string(pptr, rod->class);
	serialize_string(pptr, rod->disp_value);
}

int deserialize_rib_object_data(const void **pptr, struct rib_object_data *rod)
{
	int ret;

	memset(rod, 0, sizeof(*rod));

	deserialize_obj(*pptr, uint64_t, &rod->instance);

	ret = deserialize_string(pptr, &rod->name);
	if (ret)
		return ret;

	ret = deserialize_string(pptr, &rod->class);
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

	if (rod->class) {
		COMMON_FREE(rod->class);
		rod->class = 0;
	}

	if (rod->disp_value) {
		COMMON_FREE(rod->disp_value);
		rod->disp_value = 0;
	}

	COMMON_FREE(rod);
}

int query_rib_resp_serlen(const struct query_rib_resp * qrr)
{
	int ret;
	struct rib_object_data * pos;

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

        list_for_each_entry(pos, &(qrr->rib_object_data_entries), next) {
                size++;
        }

        serialize_obj(*pptr, uint16_t, size);

        list_for_each_entry(pos, &(qrr->rib_object_data_entries), next) {
                serialize_rib_object_data(pptr, pos);
        }
}

int deserialize_query_rib_resp(const void **pptr, struct query_rib_resp *qrr)
{
	int ret;
	struct rib_object_data * pos;
	uint16_t size;
	int i;

	memset(qrr, 0, sizeof(*qrr));

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = COMMON_ALLOC(sizeof(struct rib_object_data), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		ret = deserialize_rib_object_data(pptr, pos);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &qrr->rib_object_data_entries);
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

int port_id_altlist_serlen(const struct port_id_altlist * pia)
{
	return sizeof(uint16_t) + pia->num_ports * sizeof(port_id_t);
}

void serialize_port_id_altlist(void **pptr, const struct port_id_altlist *pia)
{
	int plength;

	serialize_obj(*pptr, uint16_t, pia->num_ports);
	plength = pia->num_ports * sizeof(port_id_t);

	if (pia->ports > 0) {
		memcpy(*pptr, pia->ports, plength);
		*pptr += plength;
	}
}

int deserialize_port_id_altlist(const void **pptr, struct port_id_altlist *pia)
{
	int plength;

	memset(pia, 0, sizeof(*pia));

	deserialize_obj(*pptr, uint16_t, &pia->num_ports);
	plength = pia->num_ports * sizeof(port_id_t);

	if (pia->ports > 0) {
		memcpy(pia->ports, *pptr, plength);
		*pptr += plength;
	}

	return 0;
}

void port_id_altlist_free(struct port_id_altlist * pia)
{
	if (!pia)
		return;

	if (pia->ports) {
		COMMON_FREE(pia->ports);
		pia->ports = 0;
	}

	COMMON_FREE(pia);
}

int mod_pff_entry_serlen(const struct mod_pff_entry * pffe)
{
	int ret;
	struct port_id_altlist * pos;

	ret = sizeof(address_t) + sizeof(qos_id_t) + sizeof(uint16_t);

        list_for_each_entry(pos, &(pffe->next), next) {
                ret = ret + port_id_altlist_serlen(pos);
        }

	return ret;
}

void serialize_mod_pff_entry(void **pptr, const struct mod_pff_entry *pffe)
{
	uint16_t num_alts = 0;
	struct port_id_altlist * pos;

	serialize_obj(*pptr, address_t, pffe->fwd_info);
	serialize_obj(*pptr, qos_id_t, pffe->qos_id);

        list_for_each_entry(pos, &(pffe->next), next) {
               num_alts++;
        }

        serialize_obj(*pptr, uint16_t, num_alts);

        list_for_each_entry(pos, &(pffe->next), next) {
               serialize_port_id_altlist(pptr, pos);
        }
}

int deserialize_mod_pff_entry(const void **pptr, struct mod_pff_entry *pffe)
{
	int ret;
	uint16_t num_alts;
	struct port_id_altlist * pos;
	int i;

	memset(pffe, 0, sizeof(*pffe));

	deserialize_obj(*pptr, address_t, &pffe->fwd_info);
	deserialize_obj(*pptr, qos_id_t, &pffe->qos_id);
	deserialize_obj(*pptr, uint16_t, &num_alts);

	for(i = 0; i < num_alts; i++) {
		pos = COMMON_ALLOC(sizeof(struct port_id_altlist), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		ret = deserialize_port_id_altlist(pptr, pos);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &pffe->port_id_altlists);
	}

	return ret;
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

int pff_entry_list_serlen(const struct pff_entry_list * pel)
{
	int ret;
	struct mod_pff_entry * pos;

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

        list_for_each_entry(pos, &(pel->pff_entries), next) {
                size++;
        }

        serialize_obj(*pptr, uint16_t, size);

        list_for_each_entry(pos, &(pel->pff_entries), next) {
                serialize_mod_pff_entry(pptr, pos);
        }
}

int deserialize_pff_entry_list(const void **pptr, struct pff_entry_list *pel)
{
	int ret;
	struct mod_pff_entry * pos;
	uint16_t size;
	int i;

	memset(pel, 0, sizeof(*pel));

	deserialize_obj(*pptr, uint16_t, &size);

	for(i = 0; i < size; i++) {
		pos = COMMON_ALLOC(sizeof(struct mod_pff_entry), 1);
		if (!pos) {
			return -1;
		}

		INIT_LIST_HEAD(&pos->next);
		ret = deserialize_mod_pff_entry(pptr, pos);
		if (ret) {
			return ret;
		}

		list_add_tail(&pos->next, &pel->pff_entries);
	}

	return ret;
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

int sdup_crypto_state_serlen(const struct sdup_crypto_state * scs)
{
	return 2 * sizeof(bool) + 3 * sizeof(uint16_t)
	       + string_prlen(scs->compress_alg)
	       + string_prlen(scs->enc_alg)
	       + string_prlen(scs->mac_alg)
	       + 6 * sizeof(size_t)
	       + buffer_prlen(scs->encrypt_key_rx)
	       + buffer_prlen(scs->encrypt_key_tx)
	       + buffer_prlen(scs->iv_rx)
	       + buffer_prlen(scs->iv_tx)
	       + buffer_prlen(scs->mac_key_rx)
	       + buffer_prlen(scs->mac_key_tx);
}

void serialize_sdup_crypto_state(void **pptr, const struct sdup_crypto_state *scs)
{
	serialize_obj(*pptr, bool, scs->enable_crypto_rx);
	serialize_obj(*pptr, bool, scs->enable_crypto_tx);
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

int deserialize_sdup_crypto_state(const void **pptr, struct sdup_crypto_state *scs)
{
	int ret;

	memset(scs, 0, sizeof(*scs));

	deserialize_obj(*pptr, bool, &scs->enable_crypto_rx);
	deserialize_obj(*pptr, bool, &scs->enable_crypto_tx);

	ret = deserialize_string(pptr, &scs->compress_alg);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &scs->enc_alg);
	if (ret) {
		return ret;
	}

	ret = deserialize_string(pptr, &scs->mac_alg);
	if (ret) {
		return ret;
	}

	ret = deserialize_buffer(pptr, &scs->encrypt_key_rx);
	if (ret) {
		return ret;
	}

	ret = deserialize_buffer(pptr, &scs->encrypt_key_tx);
	if (ret) {
		return ret;
	}

	ret = deserialize_buffer(pptr, &scs->iv_rx);
	if (ret) {
		return ret;
	}

	ret = deserialize_buffer(pptr, &scs->iv_tx);
	if (ret) {
		return ret;
	}

	ret = deserialize_buffer(pptr, &scs->mac_key_rx);
	if (ret) {
		return ret;
	}

	return deserialize_buffer(pptr, &scs->mac_key_tx);
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

unsigned int serialize_irati_msg(struct irati_msg_layout *numtables,
				 size_t num_entries,
				 void *serbuf,
				 const struct irati_msg_base *msg)
{
	void *serptr = serbuf;
	unsigned int serlen;
	unsigned int copylen;
	struct name *name;
	string_t *str;
	const struct buffer *bf;
	struct flow_spec *fspec;
	struct dif_config *dif_config;
	struct dtp_config *dtp_config;
	struct dtcp_config *dtcp_config;
	struct query_rib_resp * qrr;
	struct pff_entry_list * pel;
	struct sdup_crypto_state * scs;
	int i;

	if (msg->msg_type >= num_entries) {
		LOG_ERR("Invalid numtables access [msg_type=%u]\n", msg->msg_type);
		return -1;
	}

	copylen = numtables[msg->msg_type].copylen;
	memcpy(serbuf, msg, copylen);

	serptr = serbuf + copylen;
	name = (struct name *)(((void *)msg) + copylen);
	for (i = 0; i < numtables[msg->msg_type].names; i++, name++) {
		serialize_rina_name(&serptr, name);
	}

	str = (string_t *)(name);
	for (i = 0; i < numtables[msg->msg_type].strings; i++, str++) {
		serialize_string(&serptr, str);
	}

	fspec = (struct flow_spec *)str;
	for (i = 0; i < numtables[msg->msg_type].flow_specs; i++, fspec++) {
		serialize_flow_spec(&serptr, fspec);
	}

	dif_config = (struct dif_config *)fspec;
	for (i = 0; i < numtables[msg->msg_type].dif_configs; i++, dif_config++) {
		serialize_dif_config(&serptr, dif_config);
	}

	dtp_config = (struct dtp_config*)dif_config;
	for (i = 0; i < numtables[msg->msg_type].dtp_configs; i++, dtp_config++) {
		serialize_dtp_config(&serptr, dtp_config);
	}

	dtcp_config = (struct dtcp_config*)dtp_config;
	for (i = 0; i < numtables[msg->msg_type].dtcp_configs; i++, dtcp_config++) {
		serialize_dtcp_config(&serptr, dtcp_config);
	}

	qrr = (struct query_rib_resp *)dtcp_config;
	for (i = 0; i < numtables[msg->msg_type].query_rib_resps; i++, qrr++) {
		serialize_query_rib_resp(&serptr, qrr);
	}

	pel = (struct pff_entry_list *)qrr;
	for (i = 0; i < numtables[msg->msg_type].pff_entry_lists; i++, pel++) {
		serialize_pff_entry_list(&serptr, pel);
	}

	scs = (struct sdup_crypto_state *)pel;
	for (i = 0; i < numtables[msg->msg_type].sdup_crypto_states; i++, scs++) {
		serialize_sdup_crypto_state(&serptr, scs);
	}

	bf = (const struct buffer *)scs;
	for (i = 0; i < numtables[msg->msg_type].buffers; i++, bf++) {
		serialize_buffer(&serptr, bf);
	}

	serlen = serptr - serbuf;

	return serlen;
}
COMMON_EXPORT(serialize_irati_msg);

int deserialize_irati_msg(struct irati_msg_layout *numtables, size_t num_entries,
                          const void *serbuf, unsigned int serbuf_len,
                          void *msgbuf, unsigned int msgbuf_len)
{
	struct irati_msg_base *bmsg = IRATI_MB(serbuf);
	struct name *name;
	string_t *str;
	struct buffer *bf;
	struct flow_spec *fspec;
	struct dif_config *dif_config;
	struct dtp_config *dtp_config;
	struct dtcp_config *dtcp_config;
	struct query_rib_resp * qrr;
	struct pff_entry_list * pel;
	struct sdup_crypto_state * scs;
	unsigned int copylen;
	const void *desptr;
	int ret;
	int i;

	if (bmsg->msg_type >= num_entries) {
		LOG_ERR("Invalid numtables access [msg_type=%u]\n",
			bmsg->msg_type);
		return -1;
	}

	copylen = numtables[bmsg->msg_type].copylen;
	memcpy(msgbuf, serbuf, copylen);

	desptr = serbuf + copylen;
	name = (struct name *)(msgbuf + copylen);
	for (i = 0; i < numtables[bmsg->msg_type].names; i++, name++) {
		ret = deserialize_rina_name(&desptr, name);
		if (ret) {
			return ret;
		}
	}

	str = (string_t *)name;
	for (i = 0; i < numtables[bmsg->msg_type].strings; i++, str++) {
		ret = deserialize_string(&desptr, &str);
		if (ret) {
			return ret;
		}
	}

	fspec = (struct flow_spec *)str;
	for (i = 0; i < numtables[bmsg->msg_type].flow_specs; i++, fspec++) {
		ret = deserialize_flow_spec(&desptr, fspec);
	}

	dif_config = (struct dif_config *)fspec;
	for (i = 0; i < numtables[bmsg->msg_type].dif_configs; i++, dif_config++) {
		ret = deserialize_dif_config(&desptr, dif_config);
	}

	dtp_config = (struct dtp_config *)dif_config;
	for (i = 0; i < numtables[bmsg->msg_type].dtp_configs; i++, dtp_config++) {
		ret = deserialize_dtp_config(&desptr, dtp_config);
	}

	dtcp_config = (struct dtcp_config *)dtp_config;
	for (i = 0; i < numtables[bmsg->msg_type].dtcp_configs; i++, dtcp_config++) {
		ret = deserialize_dtcp_config(&desptr, dtcp_config);
	}

	qrr = (struct query_rib_resp *)dtcp_config;
	for (i = 0; i < numtables[bmsg->msg_type].query_rib_resps; i++, qrr++) {
		ret = deserialize_query_rib_resp(&desptr, qrr);
	}

	pel = (struct pff_entry_list *)qrr;
	for (i = 0; i < numtables[bmsg->msg_type].pff_entry_lists; i++, pel++) {
		ret = deserialize_pff_entry_list(&desptr, pel);
	}

	scs = (struct sdup_crypto_state *)pel;
	for (i = 0; i < numtables[bmsg->msg_type].sdup_crypto_states; i++, scs++) {
		ret = deserialize_sdup_crypto_state(&desptr, scs);
	}

	bf = (struct buffer *)scs;
	for (i = 0; i < numtables[bmsg->msg_type].buffers; i++, bf++) {
		ret = deserialize_buffer(&desptr, &bf);
	}

	if ((desptr - serbuf) != serbuf_len) {
		return -1;
	}

	return 0;
}
COMMON_EXPORT(deserialize_irati_msg);

unsigned int irati_msg_serlen(struct irati_msg_layout *numtables,
			      size_t num_entries,
			      const struct irati_msg_base *msg)
{
	unsigned int ret;
	struct name *name;
	string_t *str;
	struct flow_spec *fspec;
	struct dif_config *dif_config;
	struct dtp_config *dtp_config;
	struct dtcp_config *dtcp_config;
	struct query_rib_resp *qrr;
	struct pff_entry_list * pel;
	struct sdup_crypto_state * scs;
	const struct buffer *bf;
	int i;

	if (msg->msg_type >= num_entries) {
		LOG_ERR("Invalid numtables access [msg_type=%u]\n", msg->msg_type);
		return -1;
	}

	ret = numtables[msg->msg_type].copylen;

	name = (struct name *)(((void *)msg) + ret);
	for (i = 0; i < numtables[msg->msg_type].names; i++, name++) {
		ret += rina_name_serlen(name);
	}

	str = (string_t *)name;
	for (i = 0; i < numtables[msg->msg_type].strings; i++, str++) {
		ret += sizeof(uint16_t) + string_prlen(str);
	}

	fspec = (struct flow_spec *)str;
	for (i = 0; i < numtables[msg->msg_type].flow_specs; i++, fspec++) {
		ret += flow_spec_serlen(fspec);
	}

	dif_config = (struct dif_config *)fspec;
	for (i = 0; i < numtables[msg->msg_type].dif_configs; i++, dif_config++) {
		ret += dif_config_serlen(dif_config);
	}

	dtp_config = (struct dtp_config *)dif_config;
	for (i = 0; i < numtables[msg->msg_type].dtp_configs; i++, dtp_config++) {
		ret += dtp_config_serlen(dtp_config);
	}

	dtcp_config = (struct dtcp_config *)dtp_config;
	for (i = 0; i < numtables[msg->msg_type].dtcp_configs; i++, dtcp_config++) {
		ret += dtcp_config_serlen(dtcp_config);
	}

	qrr = (struct query_rib_resp *)dtcp_config;
	for (i = 0; i < numtables[msg->msg_type].query_rib_resps; i++, qrr++) {
		ret += query_rib_resp_serlen(qrr);
	}

	pel = (struct pff_entry_list *)qrr;
	for (i = 0; i < numtables[msg->msg_type].pff_entry_lists; i++, pel++) {
		ret += pff_entry_list_serlen(pel);
	}

	scs = (struct sdup_crypto_state *)pel;
	for (i = 0; i < numtables[msg->msg_type].sdup_crypto_states; i++, scs++) {
		ret += sdup_crypto_state_serlen(scs);
	}

	bf = (const struct buffer *)scs;
	for (i = 0; i < numtables[msg->msg_type].buffers; i++, bf++) {
		ret += sizeof(bf->size) + bf->size;
	}

	return ret;
}
COMMON_EXPORT(irati_msg_serlen);

void irati_msg_free(struct irati_msg_layout *numtables, size_t num_entries,
                    struct irati_msg_base *msg)
{
	unsigned int copylen = numtables[msg->msg_type].copylen;
	struct name *name;
	string_t *str;
	struct flow_spec *fspec;
	struct dif_config * dif_config;
	struct dtp_config * dtp_config;
	struct dtcp_config * dtcp_config;
	struct query_rib_resp * qrr;
	struct pff_entry_list * pel;
	struct sdup_crypto_state * scs;
	int i;

	if (msg->msg_type >= num_entries) {
		LOG_ERR("Invalid numtables access [msg_type=%u]\n",
			msg->msg_type);
		return;
	}

	/* Skip the copiable part and scan all the names contained in
	 * the message. */
	name = (struct name *)(((void *)msg) + copylen);
	for (i = 0; i < numtables[msg->msg_type].names; i++, name++) {
		rina_name_free(name);
	}

	str = (string_t *)(name);
	for (i = 0; i < numtables[msg->msg_type].strings; i++, str++) {
		if (str) {
			COMMON_FREE(str);
		}
	}

	fspec = (struct flow_spec *)(str);
	for (i = 0; i < numtables[msg->msg_type].flow_specs; i++, fspec++) {
		flow_spec_free(fspec);
	}

	dif_config = (struct dif_config *)(fspec);
	for (i = 0; i < numtables[msg->msg_type].dif_configs; i++, dif_config++) {
		dif_config_free(dif_config);
	}

	dtp_config = (struct dtp_config *)(dif_config);
	for (i = 0; i < numtables[msg->msg_type].dtp_configs; i++, dtp_config++) {
		dtp_config_free(dtp_config);
	}

	dtcp_config = (struct dtcp_config *)(dtp_config);
	for (i = 0; i < numtables[msg->msg_type].dtcp_configs; i++, dtcp_config++) {
		dtcp_config_free(dtcp_config);
	}

	qrr = (struct query_rib_resp *)(dtcp_config);
	for (i = 0; i < numtables[msg->msg_type].query_rib_resps; i++, qrr++) {
		query_rib_resp_free(qrr);
	}

	pel = (struct pff_entry_list *)(qrr);
	for (i = 0; i < numtables[msg->msg_type].pff_entry_lists; i++, pel++) {
		pff_entry_list_free(pel);
	}

	scs = (struct sdup_crypto_state *)(pel);
	for (i = 0; i < numtables[msg->msg_type].sdup_crypto_states; i++, scs++) {
		sdup_crypto_state_free(scs);
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
				numtables[i].sdup_crypto_states * sizeof(struct sdup_crypto_state);

		if (cur > max) {
			max = cur;
		}
	}

	return max;
}
