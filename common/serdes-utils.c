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

unsigned int irati_msg_serlen(struct irati_msg_layout *numtables,
			      size_t num_entries,
			      const struct irati_msg_base *msg)
{
	unsigned int ret;
	struct name *name;
	string_t *str;
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
		ret += sizeof(uint16_t) + string_prlen(*str);
	}

	bf = (const struct rl_buf_field *)str;
	for (i = 0; i < numtables[msg->msg_type].buffers; i++, bf++) {
		ret += sizeof(bf->len) + bf->len;
	}

	return ret;
}
COMMON_EXPORT(irati_msg_serlen);

unsigned int irati_numtables_max_size(struct irati_msg_layout *numtables,
				      unsigned int n)
{
	unsigned int max = 0;
	int i = 0;

	for (i = 0; i < n; i++) {
		unsigned int cur = numtables[i].copylen +
				numtables[i].names * sizeof(struct name) +
				numtables[i].strings * sizeof(char *);

		if (cur > max) {
			max = cur;
		}
	}

	return max;
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
		PE("FAILED\n");
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

unsigned int serialize_irati_msg(struct irati_msg_layout *numtables,
				 size_t num_entries,
				 void *serbuf,
				 const struct irati_msg_base *msg)
{
	void *serptr = serbuf;
	unsigned int serlen;
	unsigned int copylen;
	struct rina_name *name;
	string_t *str;
	const struct buffer *bf;
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
		serialize_string(&serptr, *str);
	}

	bf = (const struct buffer *)str;
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
		ret = deserialize_string(&desptr, str);
		if (ret) {
			return ret;
		}
	}

	bf = (struct buffer *)str;
	for (i = 0; i < numtables[bmsg->msg_type].buffers; i++, bf++) {
		ret = deserialize_buffer(&desptr, bf);
	}

	if ((desptr - serbuf) != serbuf_len) {
		return -1;
	}

	return 0;
}
COMMON_EXPORT(deserialize_irati_msg);

void irati_msg_free(struct irati_msg_layout *numtables, size_t num_entries,
                    struct irati_msg_base *msg)
{
	unsigned int copylen = numtables[msg->msg_type].copylen;
	struct name *name;
	string_t *str;
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
		if (*str) {
			COMMON_FREE(*str);
		}
	}
}
COMMON_EXPORT(irati_msg_free);
