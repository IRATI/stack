/*
 * Robjects and sysfs utils
 *
 * Robjects wrap those kobjects API that are interesting for IRATI.
 * sysfs utils set of macros speed up the creation and assigment of ktyes,
 * attributes and so on to the kobjects.
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#define RINA_PREFIX "robjects"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "robjects.h"

#define to_ktype(rtype) ((struct kobj_type *) rtype)
#define to_kobj(robj) ((struct kobject *) robj)
#define to_kset(rset) (rset->kset)

#define __PREPARE_ROBJECT_WRAPPER					\
	va_list args;							\
	int ret;							\
	char * s;							\
	va_start(args, fmt);						\
	s = kvasprintf(GFP_ATOMIC, fmt, args);				\
	va_end(args);

#define __CLEAN_ROBJECT_WRAPPER						\
	kfree(s);							\
	return ret;

const char * robject_attr_name(struct robj_attribute * attr)
{ return attr->kattr.attr.name; }
EXPORT_SYMBOL(robject_attr_name);

const char * robject_name(struct robject * robj)
{ return kobject_name(&robj->kobj); }
EXPORT_SYMBOL(robject_name);

ssize_t robject_attr_show_redirect(struct kobject *	   kobj,
                         	   struct kobj_attribute * attr,
                                   char *		   buf)
{
	struct robject * robj;
	struct robj_attribute * rattr;
	robj = container_of(kobj, struct robject, kobj);
	rattr = container_of(attr, struct robj_attribute, kattr);
	return rattr->show(robj, rattr, buf);
}
EXPORT_SYMBOL(robject_attr_show_redirect);

void
robject_init(struct robject *robj, struct robj_type *rtype)
{
	memset(to_kobj(robj), 0x00, sizeof(robj->kobj));
	kobject_init(to_kobj(robj), to_ktype(rtype));
}
EXPORT_SYMBOL(robject_init);

int
robject_add(struct robject *robj, struct robject *parent, const char *fmt, ...)
{
	__PREPARE_ROBJECT_WRAPPER
	ret = kobject_add(to_kobj(robj), parent ? &parent->kobj : NULL, s);
	__CLEAN_ROBJECT_WRAPPER
}
EXPORT_SYMBOL(robject_add);

int
robject_init_and_add(struct robject *robj, struct robj_type *rtype,
		     struct robject *parent, const char *fmt, ...)
{
	__PREPARE_ROBJECT_WRAPPER
	memset(to_kobj(robj), 0x00, sizeof(robj->kobj));
	ret = kobject_init_and_add(to_kobj(robj), to_ktype(rtype),
				    parent ? &parent->kobj : NULL, s);
	__CLEAN_ROBJECT_WRAPPER
}
EXPORT_SYMBOL(robject_init_and_add);


void robject_del(struct robject *robj)
{
	kobject_del(to_kobj(robj));
	kobject_put(to_kobj(robj));
}
EXPORT_SYMBOL(robject_del);

int
robject_rset_add(struct robject *robj, struct rset *parentset,
		 const char *fmt, ...)
{
	struct kobject * kobj;
	__PREPARE_ROBJECT_WRAPPER
	kobj = to_kobj(robj);
	kobj->kset = to_kset(parentset);
	ret = kobject_add(kobj, NULL, s);
	__CLEAN_ROBJECT_WRAPPER
}
EXPORT_SYMBOL(robject_rset_add);

int
robject_rset_init_and_add(struct robject *robj, struct robj_type *rtype,
			  struct rset *parentset, const char *fmt, ...)
{
	struct kobject * kobj;
	__PREPARE_ROBJECT_WRAPPER
	kobj = to_kobj(robj);
	kobj->kset = to_kset(parentset);
	ret = kobject_init_and_add(kobj, to_ktype(rtype), NULL, s);
	__CLEAN_ROBJECT_WRAPPER
}
EXPORT_SYMBOL(robject_rset_init_and_add);

struct rset *
rset_create_and_add(const char *name, struct robject *parent)
{
	struct rset * rset;
	rset = rkzalloc(sizeof(*rset), GFP_ATOMIC);
	to_kset(rset) = kset_create_and_add(name, NULL, to_kobj(parent));
	return rset;
}
EXPORT_SYMBOL(rset_create_and_add);

void rset_unregister(struct rset * set)
{
	kset_unregister(to_kset(set));
	rkfree(set);
}
EXPORT_SYMBOL(rset_unregister);
