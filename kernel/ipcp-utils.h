/*
 * IPC Processes related utilities
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#ifndef RINA_IPCP_UTILS_H
#define RINA_IPCP_UTILS_H

#include <linux/uaccess.h>

#include "common.h"
#include "ipcp-instances.h"
#include "efcp.h"
#include "rds/rstr.h"

/*
 * Allocates a new name, returning the allocated object. In case of an error, a
 * NULL is returned.
 */
struct name * name_create(void);
struct name * name_create_ni(void);

/*
 * Initializes a previously dynamically allocated name (i.e. name_create())
 * or a statically one (e.g. declared into a struct not as a pointer).
 * Returns the passed object pointer in case everything is ok, a NULL
 * otherwise.
 *
 * A call to name_destroy() is allowed in case of error, in order to
 * release the associated resources.
 *
 * It is allowed to call name_init() over an already initialized object
 */
struct name * name_init_from(struct name *    dst,
                             const string_t * process_name,
                             const string_t * process_instance,
                             const string_t * entity_name,
                             const string_t * entity_instance);

struct name * name_init_from_ni(struct name *    dst,
                                const string_t * process_name,
                                const string_t * process_instance,
                                const string_t * entity_name,
                                const string_t * entity_instance);

/* Takes ownership of the passed parameters */
struct name * name_init_with(struct name * dst,
                             string_t *    process_name,
                             string_t *    process_instance,
                             string_t *    entity_name,
                             string_t *    entity_instance);

/*
 * Finalize a name object, releasing all the embedded resources (without
 * releasing the object itself). After name_fini() execution the passed
 * object will be in the same states as at the end of name_init().
 */
void          name_fini(struct name * dst);

/* Releases all the associated resources bound to a name object */
void          name_destroy(struct name * ptr);

/* Duplicates a name object, returning the pointer to the new object */
struct name * name_dup(const struct name * src);

/* Duplicates the name from user space */
struct name * name_dup_from_user(const struct name __user * src);

/*
 * Copies the source object contents into the destination object, both must
 * be previously allocated
 */
int           name_cpy(const struct name * src, struct name * dst);

/* Copies the name from user space */
int           name_cpy_from_user(const struct name __user * src,
                                 struct name *              dst);

bool          name_is_equal(const struct name * a, const struct name * b);
bool          name_is_ok(const struct name * n);

#define NAME_CMP_APN 0x01
#define NAME_CMP_API 0x02
#define NAME_CMP_AEN 0x04
#define NAME_CMP_AEI 0x08
#define NAME_CMP_ALL (NAME_CMP_APN | NAME_CMP_API | NAME_CMP_AEN | NAME_CMP_AEI)

bool          name_cmp(uint8_t             flags,
                       const struct name * a,
                       const struct name * b);

/* Returns a name as a (newly allocated) string */
char *        name_tostring(const struct name * n);
char *        name_tostring_ni(const struct name * n);

/* Inverse of name_tostring() */
struct name * string_toname(const string_t * s);
struct name * string_toname_ni(const string_t * s);

struct dif_info *    dif_info_create(void);
int                  dif_info_destroy(struct dif_info * dif_info);

struct flow_spec *   flow_spec_dup(const struct flow_spec * fspec);

int                 	  dup_config_entry_cpy(const struct auth_sdup_profile * src,
                                               struct auth_sdup_profile * dst);
struct auth_sdup_profile * dup_config_entry_dup(const struct auth_sdup_profile * src);

#endif
