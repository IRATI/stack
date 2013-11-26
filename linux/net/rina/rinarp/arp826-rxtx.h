/*
 * ARP 826 (wonnabe) RX/TX functions
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

#ifndef ARP826_RXTX_H
#define ARP826_RXTX_H

#include <linux/skbuff.h>
#include <linux/netdevice.h>

#include "arp826-utils.h"

int arp_send_reply(struct net_device * dev,
                   uint16_t            ptype,
                   const struct gpa *  spa,
                   const struct gha *  sha,
                   const struct gpa *  tpa,
                   const struct gha *  tha);

int arp_send_request(struct net_device * dev,
                     uint16_t            ptype,
                     const struct gpa *  spa,
                     const struct gha *  sha,
                     const struct gpa *  tpa);

/* NOTE: The following function uses a different mapping for return values */
int arp_receive(struct sk_buff *     skb,
                struct net_device *  dev,
                struct packet_type * pkt,
                struct net_device *  orig_dev);

#endif
