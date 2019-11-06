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
	IPVPNManager();
	~IPVPNManager();
	int add_registered_ip_vpn(const std::string& ip_vpn);
	int remove_registered_ip_vpn(const std::string& ip_vpn);
	bool ip_vpn_registered(const std::string& ip_vpn);
	int iporina_flow_allocated(const rina::FlowRequestEvent& event);
	void ip_vpn_flow_allocation_requested(const rina::FlowRequestEvent& event);
	int get_ip_vpn_flow_info(int port_id, rina::FlowRequestEvent& event);
	int ip_vpn_flow_deallocated(int port_id, const int ipcp_id);
	int map_ip_prefix_to_flow(const std::string& prefix, int port_id);

private:
	bool __ip_vpn_registered(const std::string& ip_vpn);
	int add_flow(const rina::FlowRequestEvent& event);
	int remove_flow(rina::FlowRequestEvent& event);
	int add_or_remove_ip_route(const std::string ip_prefix,
			           const int ipcp_id, int port_id, bool add);
	int activate_device(const int ipcp_id, int port_id, bool activate);
	std::string exec_shell_command(std::string result);
	std::string get_rina_dev_name(const int ipcp_id, int port_id);
	std::string get_ip_prefix_string(const std::string& input);

	std::list<std::string> reg_ip_vpns;
	rina::Lockable lock;
	std::map<int, rina::FlowRequestEvent> iporina_flows;
};

} //namespace rinad

#endif  /* __IP_VPN_MANAGER_H__ */
