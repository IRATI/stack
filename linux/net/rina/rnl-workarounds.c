/*
 * RNL workarounds
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

/* FIXME: This is another workaround we would like to avoid ... */
#include "../net/netlink/af_netlink.h"

#define RINA_PREFIX "rnl-workarounds"

#include "logs.h"
#include "rnl-workarounds.h"

/* FIXME: Yet another workaround ... */
void netlink_table_grab(void);
void netlink_table_ungrab(void);

void set_netlink_non_root_send_flag(void)
{
        LOG_DBG("Setting NL_CFG_F_NONROOT_SEND flag for NL_GENERIC sockets");

        netlink_table_grab();

        /*
         * FIXME: this is the most pigsty workaround here, it would be good to
         * have a fine granularity on these flags to avoid patching the
         * situation as we're doing now ...
         */
        nl_table[NETLINK_GENERIC].flags |= NL_CFG_F_NONROOT_SEND;

        netlink_table_ungrab();

        LOG_DBG("NL_CFG_F_NONROOT_SEND flag set");
}
