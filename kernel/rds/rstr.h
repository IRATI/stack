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

#ifndef RINA_RSTR_H
#define RINA_RSTR_H

#include <linux/uaccess.h>

#include <irati/kucommon.h>

/* FIXME: This file and all associated definitions must disappear */

string_t * string_from_user(const char __user * src);
int        string_dup(const string_t * src, string_t ** dst);
int        string_cmp(const string_t * a, const string_t * b);
int        string_len(const string_t * s);

char *     rkstrdup_gfp(const char * s, gfp_t flags);
char *     rkstrdup(const char * s);
char *     rkstrdup_ni(const char * s);
char *     get_zero_length_string(void);

#endif
