/*
 * Utilities for shim IPCPs over Ethernet
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef SHIM_ETH_UTILS_H
#define SHIM_ETH_UTILS_H

#include <linux/netdevice.h>
#include <linux/types.h>

/* Enables all N-flows whose shim-eth-vlan IPCP is associated to the device */
void enable_write_all(struct net_device * dev);

/* Restores the qdiscs of a net_device to the default ones */
void restore_qdisc(struct net_device * dev);

/* Replaces the qdisc(s) if a net_device by the shim-eth-qdisc(s) */
int  update_qdisc(struct net_device * dev,
		  uint16_t qdisc_max_size,
		  uint16_t qdisc_enable_size);

#endif
