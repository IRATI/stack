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

struct rinarp_handle * rinarp_register(const struct net_device * device,
                                       const struct gpa *        address);
int                    rinarp_unregister(struct rinarp_handle * handle);

typedef void (* rinarp_handler_t)(void *              opaque,
                                  const struct gpa ** dest_pa,
                                  const struct gha ** dest_ha);

int                    rinarp_resolve(struct rinarp_handle * handle, 
                                      rinarp_handler_t       handler,
                                      void *                 opaque);

#endif
