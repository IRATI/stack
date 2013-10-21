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
#include "arp826-rxtx.h"

struct resolution {
        struct resolve_data * data;

        arp826_notify_t       notify;
        void *                opaque;

        struct list_head      next;
};

spinlock_t       resolutions_lock;
struct list_head resolutions_ongoing;

struct resolve_data {
        uint16_t     ptype;
        struct gpa * spa;
        struct gha * sha;
        struct gpa * tpa;
        struct gha * tha;
};

static bool is_resolve_data_equal(struct resolve_data * a,
                                  struct resolve_data * b)
{
        if (a == b)
                return 1;

        if (!(a->ptype == b->ptype)       ||
            !gha_is_equal(a->sha, b->sha) ||
            !gpa_is_equal(a->tpa, b->tpa) ||
            !gha_is_equal(a->tha, b->tha))
                return 0;

        return 1;
}

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

static int resolver(void * o)
{
        struct resolve_data * tmp;
        struct resolution *   pos, * nxt;

        tmp = (struct resolve_data *) o;
        if (!tmp)
                return -1;

        if (!is_resolve_data_complete(tmp)) {
                LOG_ERR("Wrong data passed to resolver ...");
                resolve_data_destroy(tmp);
                return -1;
        }

        spin_lock(&resolutions_lock);

        /* FIXME: Find the entry in the ongoing resolutions */
        list_for_each_entry_safe(pos, nxt, &resolutions_ongoing, next) {
                if (is_resolve_data_equal(pos->data, tmp)) {

                        ASSERT(pos->notify);

                        pos->notify(pos->opaque,
                                    pos->data->tpa,
                                    pos->data->tha);

                        /* Get rid of the (now useless) data */
                        list_del(&pos->next);
                        resolve_data_destroy(pos->data);
                        rkfree(pos);
                }
        }

        spin_unlock(&resolutions_lock);

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

        ASSERT(arm_wq);

        /* Takes the ownership ... and disposes everything */
        return rwq_post(arm_wq, resolver, tmp);
}

int arp826_resolve_gpa(uint16_t           ptype,
                       const struct gpa * spa,
                       const struct gha * sha,
                       const struct gpa * tpa,
                       arp826_notify_t    notify,
                       void *             opaque)
{
        struct resolution * resolution;

        if (!notify) {
                LOG_ERR("No notify callback passed, this call is useless");
                return -1;
        }
        if (!gpa_is_ok(spa)) {
                LOG_ERR("Cannot resolve, bad input parameters (GPA)");
                return -1;
        }
        if (!gha_is_ok(sha)) {
                LOG_ERR("Cannot resolve, bad input parameters (SHA)");
                return -1;
        }
        if (!gpa_is_ok(tpa)) {
                LOG_ERR("Cannot resolve, bad input parameters (TPA)");
                return -1;
        }

        if (!arp_send_request(ARP_REQUEST, spa, sha, tpa)) {
                LOG_ERR("Cannot send request, cannot resolve GPA");
                return -1;
        }

        resolution = rkzalloc(sizeof(*resolution), GFP_KERNEL);
        if (!resolution)
                return -1;

        resolution->data = resolve_data_create(ptype,
                                               gpa_dup(spa), gha_dup(sha),
                                               gpa_dup(tpa), NULL);
        if (!resolution->data) {
                rkfree(resolution);
                return -1;
        }

        resolution->notify = notify;
        resolution->opaque = opaque;
        INIT_LIST_HEAD(&resolution->next);

        spin_lock(&resolutions_lock);
        list_add(&resolutions_ongoing, &resolution->next);
        spin_unlock(&resolutions_lock);

        return 0;
}
EXPORT_SYMBOL(arp826_resolve_gpa);

int arm_init(void)
{
        LOG_DBG("Initializing");

        arm_wq = rwq_create("arp826-wq");
        if (!arm_wq)
                return -1;

        spin_lock_init(&resolutions_lock);
        INIT_LIST_HEAD(&resolutions_ongoing);

        LOG_DBG("Initialized successfully");

        return 0;
}

int arm_fini(void)
{
        struct resolution * pos, * nxt;
        int                 ret;

        LOG_DBG("Finalizing");

        list_for_each_entry_safe(pos, nxt, &resolutions_ongoing, next) {
                resolve_data_destroy(pos->data);
                rkfree(pos);
        }

        ret = rwq_destroy(arm_wq);

        LOG_DBG("Finalized successfully");

        return ret;
}
