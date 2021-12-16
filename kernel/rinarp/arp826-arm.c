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
#include <linux/netdevice.h>

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
#include "arp826-arm.h"

struct resolution {
        struct resolve_data * data;

        arp826_notify_t       notify;
        void *                opaque;

        struct timer_list     timer;

        struct list_head      next;
};

static DEFINE_SPINLOCK(resolutions_lock);
static struct list_head resolutions_ongoing;

struct resolve_data {
        struct net_device * dev;
        uint16_t            ptype;
        struct gpa *        spa;
        struct gha *        sha;
        struct gpa *        tpa;
        struct gha *        tha;

        // If this is true it means the request as timed out and that
        // the 'tha' data is not going to be filled.
        bool                 timed_out;
};

static struct workqueue_struct *arm_wq = NULL;

static bool is_resolve_data_matching(struct resolve_data * a,
                                     struct resolve_data * b)
{
        if (a == b)
                return true;

        if ((a && !b) || (!a && b))
                return false;

        ASSERT(a);
        ASSERT(b);

        gha_log_dbg("A Source HW Addr (a->sha)", a->sha);
        gha_log_dbg("B Target HW Addr (b->tha)", b->tha);
        gpa_log_dbg("A Target Proto Addr (a->tpa)", b->tpa);
        gpa_log_dbg("B Source Proto Addr (b->spa)", b->spa);
        gpa_log_dbg("A Source Proto Addr (a->spa)", a->spa);
        gpa_log_dbg("B Target Proto Addr (b->tpa)", b->tpa);

        if (a->dev != b->dev)
                return false;

        if (!(a->ptype == b->ptype)       ||
            !gha_is_equal(a->sha, b->tha) ||
            !gpa_is_equal(a->tpa, b->spa) ||
            !gpa_is_equal(a->spa, b->tpa))
                return false;

        return true;
}

static bool is_resolve_data_complete(const struct resolve_data * data)
{
        return (!data                    ||
                !data->spa || !data->sha ||
                !data->tpa || !data->tha) ? false : true;
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

static struct resolve_data * resolve_data_create_gfp(gfp_t               flags,
                                                     struct net_device * dev,
                                                     uint16_t            ptype,
                                                     struct gpa *        spa,
                                                     struct gha *        sha,
                                                     struct gpa *        tpa,
                                                     struct gha *        tha)
{
        struct resolve_data * tmp;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->dev   = dev;
        tmp->ptype = ptype;
        tmp->spa   = spa;
        tmp->sha   = sha;
        tmp->tpa   = tpa;
        tmp->tha   = tha;

        return tmp;
}

static struct resolve_data * resolve_data_create(struct net_device * dev,
                                                 uint16_t            ptype,
                                                 struct gpa *        spa,
                                                 struct gha *        sha,
                                                 struct gpa *        tpa,
                                                 struct gha *        tha)
{ return resolve_data_create_gfp(GFP_KERNEL, dev, ptype, spa, sha, tpa, tha); }

static void resolution_destroy(struct resolution *res) {
        /* Remove the resolution in the list of pending
         * resolutions. */
        spin_lock(&resolutions_lock);
        list_del(&res->next);
        spin_unlock(&resolutions_lock);

        /* Destroy the pending resolution object data and stop the
           timeout timer. */
        resolve_data_destroy(res->data);
        rtimer_stop(&res->timer);
        rkfree(res);
}

static int timeout_resolver(void *o) {
        struct resolution *res;

        LOG_DBG("In the ARP timeout resolver, calling the timed out notifier");

        res = (struct resolution *)o;
        if (!res) return -1;

        /* If the request timed out, we have to use the target address
         * in the resolve_data object since this is what is used by
         * the notifier object to find the ARP request corresponding
         * to an ARP reply. */
        res->notify(res->opaque,
                    res->data->timed_out,
                    res->data->tpa,
                    res->data->tha);

        resolution_destroy(res);

        return 0;
}

/*
 * ARP reply resolver. Make sure this function is passed a
 * "resolve_data" struct that it can free.
 */
static int reply_resolver(void *o)
{
        struct resolve_data *tmp;
        struct resolution *pos, *nxt;

        LOG_DBG("In the ARP resolver, looking for the right handler");

        tmp = (struct resolve_data *)o;
        if (!tmp) return -1;

        if (tmp->timed_out) {
                LOG_CRIT("ARP resolver called on timed out request");
                resolve_data_destroy(tmp);
                return -1;
        }

        if (!is_resolve_data_complete(tmp)) {
                LOG_ERR("Wrong data passed to resolver ...");
                resolve_data_destroy(tmp);
                return -1;
        }

        spin_lock(&resolutions_lock);

        /* Find the resolution that matches this reply. */
        list_for_each_entry_safe(pos, nxt, &resolutions_ongoing, next) {
                if (is_resolve_data_matching(pos->data, tmp)) {
                        spin_unlock(&resolutions_lock);

                        pos->notify(pos->opaque,
                                    tmp->timed_out,
                                    tmp->spa,
                                    tmp->sha);

                        resolution_destroy(pos);
                }
        }

        spin_unlock(&resolutions_lock);

        /* Finally destroy the data */
        resolve_data_destroy(tmp);

        return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
static void resolution_timeout(void *data)
#else
static void resolution_timeout(struct timer_list *tl)
#endif
{
        struct resolution *res;
        struct rwq_work_item *r;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        res = (struct resolution *)data;
#else
        res = from_timer(res, tl, timer);
#endif

        LOG_DBG("Entering rinarp timeout handler.");

        ASSERT(arm_wq);

        res->data->timed_out = 1;

        r = rwq_work_create_ni(timeout_resolver, res);
        if (!r) {
                LOG_CRIT("Cannot create work item for ARP timeout");
                return;
        }

        /* Takes the ownership ... and disposes everything */
        rwq_work_post(arm_wq, r);
}

/* FIXME: We should have a wq-alike job posting approach ... */
int arm_resolve(struct net_device * dev,
                uint16_t            ptype,
                struct gpa *        spa,
                struct gha *        sha,
                struct gpa *        tpa,
                struct gha *        tha)
{
        struct resolve_data *  tmp;
        struct rwq_work_item * r;

        if (!gpa_is_ok(spa) || !gha_is_ok(sha) ||
            !gpa_is_ok(tpa) || !gha_is_ok(tha))
                return -1;

        tmp = resolve_data_create_gfp(GFP_ATOMIC,
                                      dev, ptype,
                                      spa, sha, tpa, tha);
        if (!tmp)
                return -1;

        ASSERT(arm_wq);

        r = rwq_work_create_ni(reply_resolver, tmp);
        if (!r) {
                resolve_data_destroy(tmp);
                return -1;
        }

        /* Takes the ownership ... and disposes everything */
        return rwq_work_post(arm_wq, r);
}

/* FIXME: Use dev */
int arp826_resolve_gpa(struct net_device * dev,
                       uint16_t            ptype,
                       const struct gpa *  spa,
                       const struct gha *  sha,
                       const struct gpa *  tpa,
                       arp826_notify_t     notify,
                       void *              opaque)
{
        struct resolution * resolution;

        struct gpa * tmp_spa;
        struct gpa * tmp_tpa;
        struct gha * tmp_sha;

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
                LOG_ERR("Cannot resolve, bad input parameters pt (TPA)");
                return -1;
        }

        tmp_spa = gpa_dup(spa);
        if (!tmp_spa)
                return -1;
        tmp_tpa = gpa_dup(tpa);
        if (!tmp_tpa) {
                gpa_destroy(tmp_spa);
                return -1;
        }
        tmp_sha = gha_dup(sha);
        if (!tmp_sha) {
                gpa_destroy(tmp_spa);
                gpa_destroy(tmp_tpa);
                return -1;
        }

        resolution = rkzalloc(sizeof(*resolution), GFP_KERNEL);
        if (!resolution) {
                gpa_destroy(tmp_spa);
                gpa_destroy(tmp_tpa);
                gha_destroy(tmp_sha);
                return -1;
        }

        resolution->data = resolve_data_create(dev, ptype,
                                               tmp_spa, tmp_sha,
                                               tmp_tpa, NULL);
        if (!resolution->data) {
                gpa_destroy(tmp_spa);
                gpa_destroy(tmp_tpa);
                gha_destroy(tmp_sha);
                rkfree(resolution);
                return -1;
        }

        resolution->notify = notify;
        resolution->opaque = opaque;
        rtimer_init(resolution_timeout, &resolution->timer, resolution);

        INIT_LIST_HEAD(&resolution->next);

        LOG_DBG("Adding new resolution to the ongoing list");
        spin_lock(&resolutions_lock);
        list_add(&resolution->next, &resolutions_ongoing);
        spin_unlock(&resolutions_lock);

        rtimer_start(&resolution->timer, 2000);

        if (arp_send_request(dev, ptype, spa, sha, tpa)) {
                LOG_ERR("Cannot send request, cannot resolve GPA");
                resolution_destroy(resolution);
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(arp826_resolve_gpa);

int arm_init(void)
{
        arm_wq = rwq_create("arp826-wq");
        if (!arm_wq)
                return -1;

        spin_lock_init(&resolutions_lock);
        INIT_LIST_HEAD(&resolutions_ongoing);

        LOG_INFO("ARM initialized successfully");

        return 0;
}

int arm_fini(void)
{
        struct resolution * pos, * nxt;
        int                 ret;

        list_for_each_entry_safe(pos, nxt, &resolutions_ongoing, next) {
                resolution_destroy(pos);
        }

        ret = rwq_destroy(arm_wq);

        LOG_INFO("ARM finalized successfully");

        return ret;
}
