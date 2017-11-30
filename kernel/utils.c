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

#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/export.h>
#include <linux/uaccess.h>

/* For RWQ */
#include <linux/workqueue.h>

#define RINA_PREFIX "utils"

#include "logs.h"
#include "utils.h"
#include "debug.h"

#define USTR_MAX_SIZE 10000

bool is_value_in_range(int value, int min_value, int max_value)
{ return ((value >= min_value || value <= max_value) ? true : false); }

char * strdup_from_user(const char __user * src)
{
        size_t size;
        char * tmp;

        if (!src) {
                LOG_ERR("Source buffer from user is NULL, "
                        "cannot-strdup-from-user");
                return NULL;
        }

        size = strnlen_user(src, USTR_MAX_SIZE); /* Includes the terminating NUL */
        if (!size) {
                LOG_ERR("Got exception while detecting string length");
                return NULL;
        }

        tmp = rkmalloc(size, GFP_KERNEL);
        if (!tmp)
                return NULL;

        /*
         * strncpy_from_user() copies the terminating NUL. On success, returns
         * the length of the string (not including the trailing NUL). We care
         * on having the complete-copy, parts of it are a no-go
         */
        if (strncpy_from_user(tmp, src, size) != (size - 1)) {
                LOG_ERR("Cannot strncpy-from-user!");
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}
