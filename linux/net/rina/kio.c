/*
 * KIO (Kernel I/O)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

/* Egress Port Mapping */
struct epm {
        int temp;
};

struct epm * epm_create(void)
{
        struct epm * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}

int epm_destroy(struct epm * instance)
{
        if (!instance)
                return -1;

        rkfree(instance);
        return 0;
}

struct efcp_container * epm_egress_get(struct epm * instance,
                                       port_id_t    id)
{
        LOG_MISSING;

        return NULL;
}

int epm_egress_set(struct epm *            instance,
                   port_id_t               id,
                   struct efcp_container * container)
{
        LOG_MISSING;

        return -1;
}

/* Ingress Port Mapping */
struct ipm {
        int temp;
};

struct ipm * ipm_create(void)
{
        struct ipm * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}

int ipm_destroy(struct ipm * instance)
{
        if (!instance)
                return -1;

        rkfree(instance);
        return 0;
}

struct rmt * ipm_ingress_get(struct ipm * instance,
                             port_id_t     id)
{
        LOG_MISSING;

        return NULL;
}

int ipm_ingress_set(struct ipm * instance,
                    port_id_t    id,
                    struct rmt * rmt)
{
        LOG_MISSING;

        return -1;
}
