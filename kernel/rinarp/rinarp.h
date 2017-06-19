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

#ifndef RINARP_H
#define RINARP_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>

#include "arp826-utils.h"

struct rinarp_handle;

struct rinarp_handle * rinarp_add(struct net_device * dev,
                                  const struct gpa *  pa,
                                  const struct gha *  ha);
int                    rinarp_remove(struct rinarp_handle * handle);

typedef void (* rinarp_notification_t)(void *             opaque,
                                       const struct gpa * tpa,
                                       const struct gha * tha);

int                    rinarp_resolve_gpa(struct rinarp_handle * handle,
                                          const struct gpa *     tpa,
                                          rinarp_notification_t  notify,
                                          void *                 opaque);

/* FIXME: Should return a copy */
const struct gpa *     rinarp_find_gpa(struct rinarp_handle * handle,
                                       const struct gha *     tha);

#endif
