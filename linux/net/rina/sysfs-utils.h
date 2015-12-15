/*
 * SYSFS utils
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

#ifndef RINA_SYSFSUTILS_H
#define RINA_SYSFSUTILS_H

/* Supports maximum 24 arguments, aka sysfs attributes, to be defined at once */
#define _GET_NTH_ARG(				\
	_1, _2, _3, _4, _5, _6, _7, _8, _9, _10,\
	_11, _12, _13, _14, _15, _16, _17, _18,	\
	_19, _20, _21, _22, _23, _24, N, ...) N

/* Utility functions to map the different number of variable arguments
 * _call: Function name
 * x	: Component name
 * y	: Attribute name
 */
#define _func_0(_call, ...)
#define _func_1(_call, x , y) _call(x, y)
#define _func_2(_call, x, y, ...) _call(x, y) _func_1(_call, x, __VA_ARGS__)
#define _func_3(_call, x, y, ...) _call(x, y) _func_2(_call, x, __VA_ARGS__)
#define _func_4(_call, x, y, ...) _call(x, y) _func_3(_call, x, __VA_ARGS__)
#define _func_5(_call, x, y, ...) _call(x, y) _func_4(_call, x, __VA_ARGS__)
#define _func_6(_call, x, y, ...) _call(x, y) _func_5(_call, x, __VA_ARGS__)
#define _func_7(_call, x, y, ...) _call(x, y) _func_6(_call, x, __VA_ARGS__)
#define _func_8(_call, x, y, ...) _call(x, y) _func_7(_call, x, __VA_ARGS__)
#define _func_9(_call, x, y, ...) _call(x, y) _func_8(_call, x, __VA_ARGS__)
#define _func_10(_call, x, y, ...) _call(x, y) _func_9(_call, x, __VA_ARGS__)
#define _func_11(_call, x, y, ...) _call(x, y) _func_10(_call, x, __VA_ARGS__)
#define _func_12(_call, x, y, ...) _call(x, y) _func_11(_call, x, __VA_ARGS__)
#define _func_13(_call, x, y, ...) _call(x, y) _func_12(_call, x, __VA_ARGS__)
#define _func_14(_call, x, y, ...) _call(x, y) _func_13(_call, x, __VA_ARGS__)
#define _func_15(_call, x, y, ...) _call(x, y) _func_14(_call, x, __VA_ARGS__)
#define _func_16(_call, x, y, ...) _call(x, y) _func_15(_call, x, __VA_ARGS__)
#define _func_17(_call, x, y, ...) _call(x, y) _func_16(_call, x, __VA_ARGS__)
#define _func_18(_call, x, y, ...) _call(x, y) _func_17(_call, x, __VA_ARGS__)
#define _func_19(_call, x, y, ...) _call(x, y) _func_18(_call, x, __VA_ARGS__)
#define _func_20(_call, x, y, ...) _call(x, y) _func_19(_call, x, __VA_ARGS__)
#define _func_21(_call, x, y, ...) _call(x, y) _func_20(_call, x, __VA_ARGS__)
#define _func_22(_call, x, y, ...) _call(x, y) _func_21(_call, x, __VA_ARGS__)
#define _func_23(_call, x, y, ...) _call(x, y) _func_22(_call, x, __VA_ARGS__)
#define _func_24(_call, x, y, ...) _call(x, y) _func_23(_call, x, __VA_ARGS__)

#define __CAT(A, B) A ## B
#define CAT(A, B) __CAT(A, B)

/* Loops on __VA_ARGS__ (attribute names) at the end you get one call to
 * __func_1(x,y,attribute_name) per attribute
 * x	: Function/macro name
 * y	: Component name
 */
#define CALL_FOR_EACH(x, y,  ...) _GET_NTH_ARG(__VA_ARGS__, 		\
        _func_24, _func_23, _func_22, _func_21, _func_20, _func_19,	\
        _func_18, _func_17, _func_16, _func_15, _func_14, _func_13,	\
        _func_12, _func_11, _func_10, _func_9, _func_8, _func_7,	\
        _func_6, _func_5, _func_4, _func_3, _func_2, _func_1, _func_0)	\
	(x, y, ##__VA_ARGS__)

/* Declares a kobj_attribute object */
#define DECLARE_ATTR(COMP_NAME, ATTR_NAME)				  \
        static struct kobj_attribute ATTR_NAME##_attr = {		  \
		.attr = {.name = __stringify(ATTR_NAME), .mode = S_IRUGO},\
        	.show = CAT(COMP_NAME, _attr_show),			  \
	};

/* Declares a sysfs_ops object with is show function*/
#define DECLARE_SYSFS_OPS(COMP_NAME)				\
static ssize_t COMP_NAME##_sysfs_show(struct kobject *   kobj,	\
				      struct attribute * attr,	\
				      char *             buf)	\
{								\
	struct kobj_attribute * kattr;				\
	kattr = container_of(attr, struct kobj_attribute, attr);\
	return kattr->show(kobj, kattr, buf);			\
}								\
static const struct sysfs_ops COMP_NAME##_sysfs_ops = {		\
        .show = COMP_NAME##_sysfs_show				\
};

#define ADD_KTYPE_ATTR(COMP_NAME, ATTR_NAME) 			\
	&ATTR_NAME##_attr.attr,

/* Declares all the attributes for the kobject to be exported by sysfs
 * Requires the .show function to be previously declared as
 * <comp_name>_attr_show
 */
#define DECLARE_SYSFS_ATTRS(COMP_NAME, ...)			\
	CALL_FOR_EACH(DECLARE_ATTR, COMP_NAME, ##__VA_ARGS__)	\
	static struct attribute * COMP_NAME##_attrs[] = {	\
	CALL_FOR_EACH(ADD_KTYPE_ATTR, COMP_NAME, ##__VA_ARGS__)	\
	NULL, };

/* Declares the kobj_type object for the component */
#define DECLARE_SYSFS_KTYPE(COMP_NAME)			\
	static struct kobj_type COMP_NAME##_ktype = {		\
	        .sysfs_ops     = &COMP_NAME##_sysfs_ops,	\
	        .default_attrs = COMP_NAME##_attrs,		\
	        .release       = NULL,				\
	};

/* Declares an empty kobj_type object for the component. No sysfs_ops nor
 * default_attrs initialized*/
#define DECLARE_SYSFS_EMTPY_KTYPE(COMP_NAME)			\
	static struct kobj_type COMP_NAME##_ktype = {		\
	        .release       = NULL,				\
	};

/* Adds an attribute to an existing kobject */
#define ADD_ATTR_TO_KOBJ(KOBJ, ATTR_NAME)			\
	sysfs_create_file(KOBJ, &ATTR_NAME##_attr.attr);

/* Declares a new attribute and adds it to an existing kobject */
#define DECLARE_AND_ADD_SYSFS_ATTRS(kobj, COMP_NAME, ...)	\
	CALL_FOR_EACH(DECLARE_ATTR, COMP_NAME, ##__VA_ARGS__)	\
	CALL_FOR_EACH(ADD_ATTR_TO_KOBJ, kobj, ##__VA_ARGS__)
#endif
