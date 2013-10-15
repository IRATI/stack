/*
 * ARP 826 (wonnabe) core
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

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-arm"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826.h"
#include "arp826-utils.h"
#include "arp826-tables.h"

struct resolve_data {
        uint16_t     ptype;
        struct gpa * spa;
        struct gha * sha;
        struct gpa * tpa;
        struct gha * tha;
};

bool is_resolve_data_complete(const struct resolve_data * data)
{
        return (!data                    ||
                !data->spa || !data->sha ||
                !data->tpa || !data->tha) ? 0 : 1;
}

static void resolve_data_destroy(struct resolve_data * data)
{
        ASSERT(data);

        if (data->spa) gpa_destroy(data->spa);
        if (data->sha) gha_destroy(data->sha);
        if (data->tpa) gpa_destroy(data->tpa);
        if (data->tha) gha_destroy(data->tha);
        rkfree(data);
}

static struct resolve_data * resolve_data_create(uint16_t     ptype,
                                                 struct gpa * spa,
                                                 struct gha * sha,
                                                 struct gpa * tpa,
                                                 struct gha * tha)
{
        struct resolve_data * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->ptype = ptype;
        tmp->spa   = spa;
        tmp->sha   = sha;
        tmp->tpa   = tpa;
        tmp->tha   = tha;

        return tmp;
}

struct resolution {
        struct resolve_data resolution;

        arp826_notify_t     notify;
        void *              opaque;

        struct list_head    next;
};

struct resolution * ongoing = NULL;

static int resolver(void * o)
{
        struct resolve_data * tmp;

        tmp = (struct resolve_data *) o;
        if (!tmp)
                return -1;

        if (!is_resolve_data_complete(tmp)) {
                LOG_ERR("Wrong data passed to resolver ...");
                resolve_data_destroy(tmp);
                return -1;
        }

        /* FIXME: Find the entry */
        /* FIXME: Update the tables */
        /* FIXME: Call the callback */
        /* FIXME: Finally, update the ARP cache */

        /* Finally destroy the data */
        resolve_data_destroy(tmp);

        return 0;
}

static struct workqueue_struct * arm_wq = NULL;

int arm_resolve(uint16_t     ptype,
                struct gpa * spa,
                struct gha * sha,
                struct gpa * tpa,
                struct gha * tha)
{
        struct resolve_data * tmp;

        if (!gpa_is_ok(spa) || !gha_is_ok(sha) ||
            !gpa_is_ok(tpa) || !gha_is_ok(tha))
                return -1;

        tmp = resolve_data_create(ptype, spa, sha, tpa, tha);
        if (!tmp)
                return -1;

        /* Takes the ownership ... and disposes everything */
        return rwq_post(arm_wq, resolver, tmp);
}

int arm_init(void)
{
        arm_wq = rwq_create("arp826-wq");
        if (!arm_wq)
                return -1;
        return 0;
}

int arm_fini(void)
{ return rwq_destroy(arm_wq); }

int arp826_resolve_gpa(uint16_t           ptype,
                       const struct gpa * spa,
                       const struct gha * sha,
                       const struct gpa * tpa,
                       arp826_notify_t    notify,
                       void *             opaque)
{
#if 0
        struct arp826_arm_resolve * tmp;

        if (!gpa_is_ok(spa) || !gha_is_ok(sha) || !gpa_is_ok(tpa) || notify) {
                LOG_ERR("Cannot resolve, bad input parameters");
                return -1;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->ptype  = ptype;
        tmp->spa    = gpa_dup(spa);
        tmp->sha    = gha_dup(sha);
        tmp->tpa    = gpa_dup(tpa);
        tmp->tha    = NULL;

        tmp->notify = notify;
        tmp->opaque = opaque;

        /* Do we really need to resolve it or ... do we have it already ??? */

        LOG_MISSING;
#else
        LOG_MISSING;
#endif
        
        return 0;
}
EXPORT_SYMBOL(arp826_resolve_gpa);
