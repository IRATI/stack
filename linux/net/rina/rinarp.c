/*
 * RINARP
 *
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#include <linux/if_arp.h>

#define RINA_PREFIX "rinarp"

#include "logs.h"
#include "debug.h"
#include "utils.h"

#include "rinarp.h"
#include "arp826.h"
#include "arp826-utils.h"

/* This is an observer (a single one) */
struct rinarp_handle {
        struct gpa * pa;
        struct gha * ha;
};

struct rinarp_handle * rinarp_add(const struct net_device * device,
                                  const struct gpa *        address)
{
        struct rinarp_handle * handle;

        if (!device                      ||
            !gpa_is_ok(address)          ||
            !device->dev_addr            ||
            device->type != ARPHRD_ETHER ||
            device->addr_len != 6) {
                LOG_ERR("Bogus input parameters, cannot register");
                return NULL;
        }

        handle = rkmalloc(sizeof(*handle), GFP_KERNEL);
        if (!handle)
                return NULL;

        handle->pa = gpa_dup(address);
        if (!handle->pa) {
                rkfree(handle);
                return NULL;
        }

        ASSERT(device->type     == ARPHRD_ETHER);
        ASSERT(device->addr_len == 6);
        handle->ha = gha_create(MAC_ADDR_802_3, device->dev_addr);
        if (!handle->ha) {
                gpa_destroy(handle->pa);
                rkfree(handle);
                return NULL;
        }

        if (arp826_add(ETH_P_RINA, handle->pa, handle->ha, -1)) {
                gpa_destroy(handle->pa);
                gha_destroy(handle->ha);
                rkfree(handle);
                return NULL;
        }

        return handle;
}
EXPORT_SYMBOL(rinarp_add);

int rinarp_remove(struct rinarp_handle * handle)
{
        if (!handle) {
                LOG_ERR("Bogus input parameters, cannot unregister");
                return -1;
        }

        ASSERT(handle->pa);
        ASSERT(handle->ha);

        arp826_remove(ETH_P_RINA, handle->pa, handle->ha);

        gpa_destroy(handle->pa);
        gha_destroy(handle->ha);
        rkfree(handle);

        return 0;
}
EXPORT_SYMBOL(rinarp_remove);

int rinarp_resolve_gpa(struct rinarp_handle * handle,
                       const struct gpa *     tpa,
                       rinarp_notification_t  notify,
                       void *                 opaque)
{
        if (!handle || !gpa_is_ok(tpa) || !notify /* opaque can be NULL */) {
                LOG_ERR("Bogus input parameters, won't resolve GPA");
                return -1;
        }

        return arp826_resolve_gpa(ETH_P_RINA,
                                  handle->pa, handle->ha, tpa,
                                  (arp826_notify_t) notify, opaque);
}
EXPORT_SYMBOL(rinarp_resolve_gpa);

const struct gpa * rinarp_resolve_gha(struct rinarp_handle * handle,
                                      const struct gha *     tha)
{
        rinarp_notification_t notify = NULL;
        void *                opaque = NULL;

        if (!handle || !gha_is_ok(tha)) {
                LOG_ERR("Bogus input parameters, won't resolve GHA");
        }

        if (arp826_resolve_gha(ETH_P_RINA,
                               handle->pa, handle->ha, tha,
                               (arp826_notify_t) notify, opaque))
                return NULL;

        LOG_MISSING;

        return NULL;
}
EXPORT_SYMBOL(rinarp_resolve_gha);
