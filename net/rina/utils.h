/*
 * Utilities
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

#ifndef RINA_UTILS_H
#define RINA_UTILS_H

#include <linux/bug.h>
#include <linux/slab.h>
/* #include <linux/gfp.h> */

/* Embed assertions in the code upon user-choice */
#ifdef CONFIG_RINA_ASSERTIONS
#define ASSERT(COND) BUG_ON(!(COND))
#else
#define ASSERT(COND)
#endif

#include <linux/kobject.h>
#define RINA_ATTR_RO(NAME)                              \
        static struct kobj_attribute NAME##_attr =      \
		 __ATTR_RO(NAME)

#define RINA_ATTR_RW(NAME)                                      \
        static struct kobj_attribute NAME##_attr =              \
		__ATTR(NAME, 0644, NAME##show, NAME##store)

#include <linux/string.h>

#define bzero(DEST, LEN) do { (void) memset(DEST, 0, LEN); } while (0)

void * rkmalloc(size_t size, gfp_t flags);
void * rkzalloc(size_t size, gfp_t flags);

#endif
