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

#define RINA_PREFIX "rinarp"

#include "logs.h"
#include "debug.h"
#include "utils.h"

#include "rinarp.h"
#include "arp826-utils.h"

/* This is an observer (a single one) */
struct rinarp_handle {
        struct gpa *     subject;

        rinarp_handler_t handler;
        void *           opaque;
};

struct rinarp_handle * rinarp_register(const struct net_device * device,
                                       const struct gpa *        address)
{
        struct rinarp_handle * handle;

        if (!device || !gpa_is_ok(address)) {
                LOG_ERR("Bogus input parameters, cannot register");
                return NULL;
        }

        handle = rkmalloc(sizeof(*handle), GFP_KERNEL);
        if (!handle)
                return NULL;

        handle->handler = NULL;
        handle->opaque  = NULL; /* Useless ... */
        handle->subject = gpa_dup(address);
        if (!handle->subject) {
                rkfree(handle);
                return NULL;
        }

        return handle;
}
EXPORT_SYMBOL(rinarp_register);

int rinarp_unregister(struct rinarp_handle * handle)
{
        if (!handle) {
                LOG_ERR("Bogus input parameters, cannot unregister");
                return -1;
        }

        ASSERT(handle->subject);

        gpa_destroy(handle->subject);
        rkfree(handle);

        return 0;
}
EXPORT_SYMBOL(rinarp_unregister);

int rinarp_resolve(struct rinarp_handle * handle, 
                   rinarp_handler_t       handler,
                   void *                 opaque)
{
        if (!handle || !handler /* the opaque can be NULL */) {
                LOG_ERR("Bogus input parameter, won't resolve");
                return -1;
        }

        ASSERT(handle->subject);

        if (handle->handler) {
                LOG_ERR("A resolution is in progress, try it later ...");
                return -1;
        }

        ASSERT(handle->handler == NULL);

        handle->handler = handler;
        handle->opaque  = opaque;

        LOG_MISSING;

        return -1;
}
EXPORT_SYMBOL(rinarp_resolve);
