/*
 * IP VPN Manager. Manages creation and deletion of IP over RINA flows
 * and VPNs. Manages registration of IP prefixes as application names
 * in RINA DIFs, as well as allocation of flows between IP prefixes,
 * the creation of VPNs and the reconfiguration of the IP forwarding table.
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef __IP_VPN_MANAGER_H__
#define __IP_VPN_MANAGER_H__

#include <librina/concurrency.h>

#define RINA_IP_FLOW_ENT_NAME "RINA_IP"

namespace rinad {

class IPVPNManager {
public:
	IPVPNManager(){};
	~IPVPNManager(){};
	int add_registered_ip_prefix(const std::string& ip_prefix);
	int remove_registered_ip_prefix(const std::string& ip_prefix);
	bool ip_prefix_registered(const std::string& ip_prefix);
	int iporina_flow_allocated(const rina::FlowRequestEvent& event);
	void iporina_flow_allocation_requested(const rina::FlowRequestEvent& event);
	int iporina_flow_deallocated(int port_id);

private:
	bool __ip_prefix_registered(const std::string& ip_prefix);
	int add_flow(const rina::FlowRequestEvent& event);
	int remove_flow(rina::FlowRequestEvent& event);

	std::list<std::string> reg_ip_prefixes;
	rina::Lockable lock;
	std::map<int, rina::FlowRequestEvent> iporina_flows;
};

} //namespace rinad

#endif  /* __IP_VPN_MANAGER_H__ */
