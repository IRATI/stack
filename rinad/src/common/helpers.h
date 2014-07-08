/*
 * Helpers for the RINA daemons
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#ifndef __RINAD_HELPERS_H__
#define __RINAD_HELPERS_H__

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <list>


namespace rinad {

rina::IPCProcess *select_ipcp_by_dif(const
                rina::ApplicationProcessNamingInformation& dif_name);

rina::IPCProcess *select_ipcp();

bool application_is_registered_to_ipcp(
                const rina::ApplicationProcessNamingInformation&,
                rina::IPCProcess *slave_ipcp);

rina::IPCProcess *lookup_ipcp_by_port(unsigned int port_id);

void collect_flows_by_application(
                const rina::ApplicationProcessNamingInformation& app_name,
                std::list<rina::FlowInformation>& result);
}
#endif  /* __RINAD_HELPERS_H__ */
