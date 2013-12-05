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
        address_t        destination;
        qos_id_t         qos_id;
        struct list_head ports;
        struct list_head next;
};

static struct pft_entry * pfte_create_gfp(gfp_t flags)
{
        LOG_MISSING;

        return NULL;
}

static struct pft_entry * pfte_create_ni(void)
{ return pfte_create_gfp(GFP_ATOMIC); }

static struct pft_entry * pfte_create(void)
{ return pfte_create_gfp(GFP_KERNEL); }

static int pfte_destroy(struct pft_entry * entry)
{
        LOG_MISSING;

        return -1;
}

/* FIXME: This PDU-FWD-T representation is crappy and has to be rearranged */
struct pft {
        struct list_head entries;
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

static int __pft_flush(struct pft * instance)
{
        struct pft_entry * pos, * nxt;
        int                ret;

        ASSERT(pft_is_ok(instance));

        list_for_each_entry_safe(pos, nxt, &instance->entries, next) {
                list_del(&pos->next);
                ret = pfte_destroy(pos);
                if (!ret) {
                        LOG_WARN("Could not destroy PDU-FWD-T entry %pK", pos);
                        return ret;
                }
        }

        return 0;
}

int pft_flush(struct pft * instance)
{
        if (!pft_is_ok(instance))
                return -1;

        return __pft_flush(instance);
}

int pft_destroy(struct pft * instance)
{
        int ret;

        if (!pft_is_ok(instance))
                return -1;

        ret = __pft_flush(instance);
        if (ret)
                return ret;

        rkfree(instance);

        return 0;
}
