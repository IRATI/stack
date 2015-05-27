/*
 * Default policy set for PFT
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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
#include <linux/module.h>
#include <linux/string.h>
#include <linux/random.h>

#define RINA_PREFIX "pft-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rds/rtimer.h"
#include "pft-ps-common.h"
#include "debug.h"

static int default_next_hop(struct pft_ps * ps,
                            struct pci *    pci,
                            port_id_t **    ports,
                            size_t *        count)
{ return common_next_hop(ps, pci, ports, count); }

static int pft_ps_set_policy_set_param(struct ps_base * bps,
                                       const char *     name,
                                       const char *     value)
{ return pft_ps_common_set_policy_set_param(bps, name, value); }

static struct ps_base *
pft_ps_default_create(struct rina_component * component)
{
        struct pft * dtp = pft_from_component(component);
        struct pft_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = pft_ps_set_policy_set_param;
        ps->dm              = dtp;
        ps->priv            = NULL;

        ps->next_hop = default_next_hop;

        return &ps->base;
}

static void pft_ps_default_destroy(struct ps_base * bps)
{
        struct pft_ps *ps = container_of(bps, struct pft_ps, base);

        if (bps) {
                rkfree(ps);
        }
}

struct ps_factory pft_factory = {
        .owner          = THIS_MODULE,
        .create  = pft_ps_default_create,
        .destroy = pft_ps_default_destroy,
};
