/*
 * KIO (Kernel I/O)
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

#include <linux/export.h>

#define RINA_PREFIX "kio"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "kio.h"

struct kio {
        int temp;
};

struct kio * kio_create(void)
{
        struct kio * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}

int kio_destroy(struct kio * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        rkfree(instance);

        return 0;
}

struct efcp_container * kio_egress_get(struct kio * instance,
                                       port_id_t    id)
{ return NULL; }

int kio_egress_set(struct kio *            instance,
                   port_id_t               id,
                   struct efcp_container * container)
{ return -1; }

struct rmt * kio_ingress_get(struct kio * instance,
                             port_id_t     id)
{ return NULL; }

int kio_ingress_set(struct kio * instance,
                    port_id_t    id,
                    struct rmt * rmt)
{ return -1; }
