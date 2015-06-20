/*
 * RINA Strings
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#include <linux/uaccess.h>

#define RINA_PREFIX "rstr"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "rstr.h"
#include "rmem.h"

char * rkstrdup_gfp(const char * s, gfp_t flags)
{
        size_t len;
        char * buf;

        if (!s)
                return NULL;

        len = strlen(s) + 1;
        buf = rkmalloc(len, flags);
        if (buf)
                memcpy(buf, s, len);

        return buf;
}
EXPORT_SYMBOL(rkstrdup_gfp);

char * rkstrdup(const char * s)
{ return rkstrdup_gfp(s, GFP_KERNEL); }
EXPORT_SYMBOL(rkstrdup);

char * rkstrdup_ni(const char * s)
{ return rkstrdup_gfp(s, GFP_ATOMIC); }
EXPORT_SYMBOL(rkstrdup_ni);

string_t * string_from_user(const char __user * src)
{ return strdup_from_user(src); }
EXPORT_SYMBOL(string_from_user);

int string_dup_gfp(gfp_t            flags,
                   const string_t * src,
                   string_t **      dst)
{
        if (!dst) {
                LOG_ERR("Destination string is NULL, cannot copy");
                return -1;
        }

        /*
         * An empty source is allowed (ref. the chain of calls) and it must
         * provoke no consequeunces
         */
        if (src) {
                *dst = rkstrdup_gfp(src, flags);
                if (!*dst) {
                        LOG_ERR("Cannot duplicate source string "
                                "in kernel-space");
                        return -1;
                }
        } else {
                LOG_DBG("Duplicating a NULL source string ...");
                *dst = NULL;
        }

        return 0;
}

int string_dup(const string_t * src, string_t ** dst)
{ return string_dup_gfp(GFP_KERNEL, src, dst); }
EXPORT_SYMBOL(string_dup);

int string_cmp(const string_t * a, const string_t * b)
{ return strcmp(a, b); }
EXPORT_SYMBOL(string_cmp);

/* FIXME: Should we assert here ? */
int string_len(const string_t * s)
{ return strlen(s); }

char * get_zero_length_string(void)
{
	char * buf;
	buf = rkmalloc(1, GFP_KERNEL);
	if (buf)
		buf[0] = '\0';

	return buf;
}
EXPORT_SYMBOL(get_zero_length_string);
