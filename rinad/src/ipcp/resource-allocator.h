/*
 * Resource Allocator
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
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

#ifndef IPCP_RESOURCE_ALLOCATOR_HH
#define IPCP_RESOURCE_ALLOCATOR_HH

#include "ipcp/components.h"

namespace rinad {

class NMinusOneFlowManager: public INMinusOneFlowManager {
public:
	NMinusOneFlowManager();
	~NMinusOneFlowManager();
	void set_ipc_process(IPCProcess * ipc_process);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	void processRegistrationNotification(const rina::IPCProcessDIFRegistrationEvent& event);;
	std::list<int> getNMinusOneFlowsToNeighbour(unsigned int address);
	int getManagementFlowToNeighbour(unsigned int address);
	unsigned int numberOfFlowsToNeighbour(const std::string& apn,
			const std::string& api);

private:
	IPCProcess * ipc_process_;
	rina::FlowAcceptor * flow_acceptor_;
};

class IPCPFlowAcceptor : public rina::FlowAcceptor {
public:
		IPCPFlowAcceptor(IPCProcess * ipcp) : ipcp_(ipcp) { };
		~IPCPFlowAcceptor() { };
		bool accept_flow(const rina::FlowRequestEvent& event);

		IPCProcess * ipcp_;
};

class ResourceAllocator: public IResourceAllocator {
public:
	ResourceAllocator();
	~ResourceAllocator();
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	INMinusOneFlowManager * get_n_minus_one_flow_manager() const;

private:
	INMinusOneFlowManager * n_minus_one_flow_manager_;
};

} //namespace rinad

#endif //IPCP_RESOURCE_ALLOCATOR_HH
