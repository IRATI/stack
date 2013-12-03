/*
 * Port-2-IPCP instance mapping
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

#define RINA_PREFIX "pim"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "pim.h"

struct pim {
        int temp;
};

struct pim * pim_create(void)
{
        struct pim * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}

int pim_destroy(struct pim * pim)
{
        if (!pim)
                return -1;

        rkfree(pim);
        return 0;
}

struct ipcp_instance_data * pim_ingress_get(struct pim * pim,
                                            port_id_t    id)
{
        struct ipcp_instance_data * tmp;

        if (!pim)
                return NULL;
        if (!is_port_id_ok(id))
                return NULL;

        tmp = NULL;

        return tmp;
}

int pim_ingress_set(struct pim *                pim,
                    port_id_t                   id,
                    struct ipcp_instance_data * ipcp)
{
        if (!pim)
                return -1;
        if (!is_port_id_ok(id))
                return -1;
        
        LOG_MISSING;

        return -1;
}
