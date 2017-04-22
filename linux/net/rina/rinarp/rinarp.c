/*
 * RINARP
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#include <linux/module.h>
#include <linux/if_arp.h>
#include <linux/netdevice.h>

#define RINA_PREFIX "rinarp"

#include "common.h"
#include "logs.h"
#include "debug.h"
#include "utils.h"

#include "rinarp.h"
#include "arp826.h"
#include "arp826-utils.h"

/* This is an observer (a single one) */
struct rinarp_handle {
        struct gpa *        pa;
        struct gha *        ha;
        struct net_device * dev;
};

static void handle_destroy(struct rinarp_handle * handle)
{
        ASSERT(handle);

        /* This function should work in (almost) all cases */

        if (handle->pa) gpa_destroy(handle->pa);
        if (handle->ha) gha_destroy(handle->ha);

        rkfree(handle);
}

static struct rinarp_handle * handle_create(struct net_device * dev,
                                            const struct gpa *  pa,
                                            const struct gha *  ha)
{
        struct rinarp_handle * handle;

        ASSERT(dev);

        if (!gpa_is_ok(pa) || !gha_is_ok(ha)) {
                LOG_ERR("Bad input parameters, cannot create a handle");
                return NULL;
        }

        handle = rkzalloc(sizeof(*handle), GFP_KERNEL);
        if (!handle)
                return NULL;

        handle->pa  = gpa_dup(pa);
        handle->ha  = gha_dup(ha);
        handle->dev = dev;
        if (!handle->pa || !handle->ha || !handle->dev) {
                handle_destroy(handle);
                return NULL;
        }

        return handle;
}

static bool handle_is_ok(const struct rinarp_handle * handle)
{
        return (handle                &&
                gpa_is_ok(handle->pa) &&
                gha_is_ok(handle->ha) &&
                handle->dev);
}

struct rinarp_handle * rinarp_add(struct net_device * dev,
                                  const struct gpa *  pa,
                                  const struct gha *  ha)
{
        struct rinarp_handle * handle;

        handle = handle_create(dev, pa, ha);
        if (!handle)
                return NULL;

        if (arp826_add(dev, ETH_P_RINA, handle->pa, handle->ha)) {
                handle_destroy(handle);
                return NULL;
        }

        ASSERT(handle_is_ok(handle));

        return handle;
}
EXPORT_SYMBOL(rinarp_add);

int rinarp_remove(struct rinarp_handle * handle)
{
        if (!handle_is_ok(handle)) {
                LOG_ERR("Bogus input parameter, cannot unregister");
                return -1;
        }

        arp826_remove(handle->dev, ETH_P_RINA, handle->pa, handle->ha);
        handle_destroy(handle);

        return 0;
}
EXPORT_SYMBOL(rinarp_remove);

int rinarp_resolve_gpa(struct rinarp_handle * handle,
                       const struct gpa *     tpa,
                       rinarp_notification_t  notify,
                       void *                 opaque)
{
        if (!handle_is_ok(handle) ||
            !gpa_is_ok(tpa)       ||
            !notify
            /* opaque can be NULL */) {
                LOG_ERR("Bogus input parameters, won't resolve GPA");
                return -1;
        }

        return arp826_resolve_gpa(handle->dev, ETH_P_RINA,
                                  handle->pa, handle->ha, tpa,
                                  (arp826_notify_t) notify, opaque);
}
EXPORT_SYMBOL(rinarp_resolve_gpa);

const struct gpa * rinarp_find_gpa(struct rinarp_handle * handle,
                                   const struct gha *     ha)
{
        if (!handle_is_ok(handle) || !gha_is_ok(ha)) {
                LOG_ERR("Cannot find GPA, bad input parameters");
                return NULL;
        }

        return arp826_find_gpa(handle->dev, ETH_P_RINA, ha);
}
EXPORT_SYMBOL(rinarp_find_gpa);

static int __init mod_init(void)
{ return 0; }

static void __exit mod_exit(void)
{ }

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA ARP wrapper");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
