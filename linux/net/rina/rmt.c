/*
 * RMT (Relaying and Multiplexing Task)
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

#include <linux/kobject.h>

#define RINA_PREFIX "rmt"

#include "logs.h"
#include "utils.h"
#include "rmt.h"
#include "debug.h"

struct rmt * rmt_init(struct kobject * parent)
{
        struct rmt * e = NULL;

        LOG_DBG("Initializing instance");

        e = rkzalloc(sizeof(*e), GFP_KERNEL);
        if (!e)
                return NULL;

        return e;
}

int rmt_fini(struct rmt * instance)
{
        LOG_DBG("Finalizing instance %pK", instance);

        ASSERT(instance);

        rkfree(instance);

        return 0;
}
