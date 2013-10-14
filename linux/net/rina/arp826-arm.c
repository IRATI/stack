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

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-arm"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826.h"
#include "arp826-cache.h"

extern struct workqueue_struct * arp826_armq;

int arp826_resolve_gpa(uint16_t           ptype,
                       const struct gpa * spa,
                       const struct gha * sha,
                       const struct gpa * tpa,
                       arp826_notify_t    notify,
                       void *             opaque)
{
        if (!gpa_is_ok(spa) || !gha_is_ok(sha) || !gpa_is_ok(tpa) || notify) {
                LOG_ERR("Cannot resolve, bad input parameters");
                return -1;
        }

        LOG_MISSING;
        
        return 0;
}
EXPORT_SYMBOL(arp826_resolve_gpa);

int arp826_resolve_gha(uint16_t           ptype,
                       const struct gpa * spa,
                       const struct gha * sha,
                       const struct gha * tha,
                       arp826_notify_t    notify,
                       void *             opaque)
{
        if (!gpa_is_ok(spa) || !gha_is_ok(sha) || !gha_is_ok(tha) || notify) {
                LOG_ERR("Cannot resolve, bad input parameters");
                return -1;
        }

        LOG_MISSING;

        return 0;
}
EXPORT_SYMBOL(arp826_resolve_gha);
