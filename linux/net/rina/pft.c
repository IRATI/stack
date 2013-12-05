/*
 * PDU-FWD-T (PDU Forwarding Table)
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

#include <linux/list.h>

#define RINA_PREFIX "pft"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "pft.h"

/* FIXME: This PDU-FWD-T entry representation has to be rearranged */
struct pft_entry {
        address_t          destination;
        qos_id_t           qos_id;
        struct list_head * ports;
};

/* FIXME: This PDU-FWD-T representation is crappy and has to be rearranged */
struct pft {
        struct list_head * entries;
};

struct pft * pft_create_gfp(gfp_t flags)
{
        struct pft * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        return tmp;
}

struct pft * pft_create(void)
{ return pft_create_gfp(GFP_KERNEL); }

struct pft * pft_create_ni(void)
{ return pft_create_gfp(GFP_ATOMIC); }

static bool pft_is_ok(struct pft * instance)
{ return instance ? true : false; }

bool pft_is_empty(struct pft * instance)
{
        if (!pft_is_ok(instance))
                return false;
        return list_empty(&instance->entries);
}

int pft_destroy(struct pft * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        rkfree(instance);

        return 0;
}
