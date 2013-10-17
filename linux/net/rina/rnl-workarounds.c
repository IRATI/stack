/*
 * RNL workarounds (we should find a better solution if possible)
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

#include "../net/netlink/af_netlink.h"
#include <linux/rwlock.h>

#define RINA_PREFIX "rnl-workarounds"

#include "logs.h"
#include "rnl-workarounds.h"

void set_netlink_non_root_send_flag(void)
{
        LOG_DBG("Setting NL_CFG_F_NONROOT_SEND flag for NL_GENERIC sockets");

        write_lock(&nl_table_lock);
        nl_table[NETLINK_GENERIC].flags |= NL_CFG_F_NONROOT_SEND;
        write_unlock(&nl_table_lock);

        LOG_DBG("NL_CFG_F_NONROOT_SEND flag set");
}
EXPORT_SYMBOL(set_netlink_non_root_send_flag);
