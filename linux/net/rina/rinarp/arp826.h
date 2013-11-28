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

#ifndef ARP826_H
#define ARP826_H

#include <linux/if_ether.h>

#include "rinarp/arp826-utils.h"

#define RINARP_ETH_PROTO ETH_P_BATMAN

/* FIXME: This API should get the htype also ... */

int                arp826_add(struct net_device * dev,
                              uint16_t            ptype,
                              const struct gpa *  pa,
                              const struct gha *  ha);
int                arp826_remove(struct net_device * dev,
                                 uint16_t            ptype,
                                 const struct gpa *  pa,
                                 const struct gha *  ha);

typedef void (* arp826_notify_t)(void *             opaque,
                                 const struct gpa * tpa,
                                 const struct gha * tha);

int                arp826_resolve_gpa(struct net_device * dev,
                                      uint16_t            ptype,
                                      const struct gpa *  spa,
                                      const struct gha *  sha,
                                      const struct gpa *  tpa,
                                      arp826_notify_t     notify,
                                      void *              opaque);
const struct gpa * arp826_find_gpa(struct net_device * dev,
                                   uint16_t            ptype,
                                   const struct gha *  ha);

#endif
