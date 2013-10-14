/*
 * An ARP RFC-826 (wonnabe) compliant implementation
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

#ifndef ARP_826_H
#define ARP_826_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>

#include "arp826-utils.h"

typedef int arp826_timeout_t; /* In seconds, < 0 mean infinite */

int arp826_add(const struct gpa * pa,
               const struct gha * ha,
               arp826_timeout_t   timeout);
int arp826_remove(const struct gpa * pa,
                  const struct gha * ha);

typedef void (* arp826_notify_t)(void *             opaque,
                                 const struct gpa * tpa,
                                 const struct gha * tha);

int arp826_resolve(const struct gpa * spa,
                   const struct gha * sha,
                   const struct gpa * tpa,
                   arp826_notify_t    notify,
                   void *             opaque);

#endif
