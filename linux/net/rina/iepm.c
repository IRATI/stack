/*
 * Ingress/Egress port-2-IPCP instance mapping
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

#define RINA_PREFIX "iepim"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "iepm.h"

/* Egress Port/IPCP-Instance Mapping */
struct epim {
        int temp;
};

struct epim * epim_create(void)
{
        struct epim * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}

int epim_destroy(struct epim * instance)
{
        if (!instance)
                return -1;

        rkfree(instance);
        return 0;
}

struct efcp_container * epim_egress_get(struct epim * instance,
                                        port_id_t     id)
{
        struct efcp_container * tmp;

        if (!instance)
                return NULL;
        if (!is_port_id_ok(id))
                return NULL;

        tmp = NULL;

        return tmp;
}

int epim_egress_set(struct epim *           instance,
                    port_id_t               id,
                    struct efcp_container * container)
{
        if (!instance)
                return -1;
        if (!is_port_id_ok(id))
                return -1;

        LOG_MISSING;

        return -1;
}

/* Ingress Port/IPCP-Instance Mapping */
struct ipim {
        int temp;
};

struct ipim * ipim_create(void)
{
        struct ipim * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}

int ipim_destroy(struct ipim * instance)
{
        if (!instance)
                return -1;

        rkfree(instance);
        return 0;
}

struct rmt * ipim_ingress_get(struct ipim * instance,
                              port_id_t     id)
{
        struct rmt * tmp;

        if (!instance)
                return NULL;
        if (!is_port_id_ok(id))
                return NULL;

        tmp = NULL;

        return tmp;
}

int ipim_ingress_set(struct ipim * instance,
                     port_id_t     id,
                     struct rmt *  rmt)
{
        if (!instance)
                return -1;
        if (!is_port_id_ok(id))
                return -1;
        
        LOG_MISSING;

        return -1;
}
