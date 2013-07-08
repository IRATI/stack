/*
 * Utilities
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

#include <linux/export.h>

#define RINA_PREFIX "shim-utils"

#include "logs.h"
#include "common.h"
#include "shim-utils.h"
#include "utils.h"

int name_kmalloc(struct name ** dst) 
{
	*dst = rkzalloc(sizeof(**dst), GFP_KERNEL);
	if (!*dst) {
		return -1;
	}
	(*dst)->process_name = 
		rkzalloc(sizeof(*((*dst)->process_name)), GFP_KERNEL);
	(*dst)->process_instance = 
		rkzalloc(sizeof(*((*dst)->process_instance)), GFP_KERNEL);
	(*dst)->entity_name = 
		rkzalloc(sizeof(*((*dst)->entity_name)), GFP_KERNEL);
	(*dst)->entity_instance = 
		rkzalloc(sizeof(*((*dst)->entity_instance)), GFP_KERNEL);
	if (!(*dst)->process_name || 
		(*dst)->process_instance || 
		(*dst)->entity_name || 
		(*dst)->entity_instance) {
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(name_kmalloc);

int name_dup(struct name ** dst, const struct name * src)
{
	if (!name_kmalloc(dst))
                return -1;
	if (!name_cpy(dst,src))
		return -1;

	return 0;
}
EXPORT_SYMBOL(name_dup);

int name_cpy(struct name ** dst, const struct name * src) 
{
	if(!strcpy((*dst)->process_name,     src->process_name)     ||
           !strcpy((*dst)->process_instance, src->process_instance) ||
           !strcpy((*dst)->entity_name,      src->entity_name)      ||
           !strcpy((*dst)->entity_instance,  src->entity_instance)) {
                LOG_ERR("Cannot perform strcpy");
                return -1;
        }

	return 0;
}
EXPORT_SYMBOL(name_cpy);

int name_kfree(struct name ** dst) 
{
	rkfree((*dst)->process_name);
	rkfree((*dst)->process_instance);
	rkfree((*dst)->entity_name); 
	rkfree((*dst)->entity_instance);
	rkfree(*dst);

	return 0;
}
EXPORT_SYMBOL(name_kfree);

#if 0
struct name * name_alloc(void)
{ return rkzalloc(sizeof(*tmp), GFP_KERNEL); }
EXPORT_SYMBOL(name_alloc);

#define INIT_RECORD(T, NAME, SRC)                               \
	do {                                                    \
	        if (T -> NAME) rkfree(T -> NAME);               \
	        T ->NAME = rkmalloc(strlen(SRC), GFP_KERNEL);   \
	} while (0);

struct name * name_init(struct name *    dst,
                        const string_t * process_name,
                        const string_t * process_instance,
                        const string_t * entity_name,
                        const string_t * entity_instance)
{
        ASSERT(name);

        INIT_RECORD(dst, process_name,     process_name);
        INIT_RECORD(dst, process_instance, process_instance);
        INIT_RECORD(dst, enitity_name,     enitity_name);
        INIT_RECORD(dst, enitity_instance, enitity_instance);

        return dst;
}
EXPORT_SYMBOL(name_init);

struct name * name_alloc_and_init(const string_t * process_name,
                                  const string_t * process_instance,
                                  const string_t * entity_name,
                                  const string_t * entity_instance)
{ return NULL; }
EXPORT_SYMBOL(name_alloc_and_init);

void name_free(struct name * ptr)
{ return NULL; }
EXPORT_SYMBOL(name_free);

struct name * name_dup(const struct name * src)
{ return NULL; }
EXPORT_SYMBOL(name_dup);

int name_cpy(const struct name * src, const struct name * dst)
{ return -1; }
EXPORT_SYMBOL(name_cpy);

#endif
