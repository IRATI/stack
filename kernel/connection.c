/*
 * Connection
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
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

#include <linux/export.h>
#include <linux/types.h>

#define RINA_PREFIX "connection"

#include "connection.h"
#include "rds/rmem.h"

struct connection * connection_create(void)
{
        struct connection * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}
EXPORT_SYMBOL(connection_create);

struct connection *
connection_dup_from_user(const struct connection __user * conn)
{
        struct connection * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        if (copy_from_user(tmp, conn, sizeof(*tmp)))
                return NULL;

        return tmp;
}

int connection_destroy(struct connection * conn)
{
        if (!conn)
                return -1;
        rkfree(conn);
        return 0;
}
EXPORT_SYMBOL(connection_destroy);
