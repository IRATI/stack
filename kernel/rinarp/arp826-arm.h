/*
 * An ARP RFC-826 (wonnabe) compliant implementation
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

#ifndef ARP826_ARM_H
#define ARP826_ARM_H

#include <linux/netdevice.h>

#include "arp826-utils.h"

int arm_init(void);
int arm_fini(void);

/* Marks a resolution, takes the ownership of all the passed data */
int arm_resolve(struct net_device * device,
                uint16_t            ptype,
                struct gpa *        spa,
                struct gha *        sha,
                struct gpa *        tpa,
                struct gha *        tha);

#endif
