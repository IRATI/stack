/*
 * Shim related utilities
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
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

#ifndef RINA_SHIM_UTILS_H
#define RINA_SHIM_UTILS_H

#include "common.h"

/* FIXME: Old API, to be removed */
int name_kmalloc(struct name ** dst);
int name_dup(struct name ** dst, const struct name * src);
int name_cpy(struct name ** dst, const struct name * src);
int name_kfree(struct name ** dst);

#if 0
/* FIXME: New API, to be added */

/*
 * Allocates a new name, returning the allocated object. In case of an error, a
 * NULL is returned.
 */
struct name * name_alloc(void);

/*
 * Initializes a previously allocated name. Returns the passed object pointer
 * in case everything is ok, a NULL otherwise. In case of error a call to
 * name_free() is allowed in order to release the associated resources.
 */
struct name * name_init(struct name *    name,
                        const string_t * process_name,
                        const string_t * process_instance,
                        const string_t * entity_name,
                        const string_t * entity_instance);

/* This function performs as name_alloc() and name_init() */
struct name * name_alloc_and_init(const string_t * process_name,
                                  const string_t * process_instance,
                                  const string_t * entity_name,
                                  const string_t * entity_instance);

/* Releases all the associated resources bound to a name object */
void          name_free(struct name * ptr);

/* Duplicates a name object, returning the pointer to the new object */
struct name * name_dup(const struct name * src);

/*
 * Copies the source object contents into the destination object, both must
 * be previously allocated. Returns 
 */
int           name_cpy(const struct name * src, const struct name * dst);
#endif

#endif
