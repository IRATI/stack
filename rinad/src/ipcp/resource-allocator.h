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

#ifdef __cplusplus

#include "ipcp/components.h"

namespace rinad {

class NMinusOneFlowManager: public INMinusOneFlowManager {
public:
	NMinusOneFlowManager();
	void set_ipc_process(IPCProcess * ipc_process);
	unsigned int allocateNMinus1Flow(const rina::FlowInformation& flowInformation);
	void allocateRequestResult(const rina::AllocateFlowRequestResultEvent& event);
	void flowAllocationRequested(const rina::FlowRequestEvent& event);
	void deallocateNMinus1Flow(int portId);
	void deallocateFlowResponse(const rina::DeallocateFlowResponseEvent& event);
	void flowDeallocatedRemotely(const rina::FlowDeallocatedEvent& event);
	const rina::FlowInformation& getNMinus1FlowInformation(int portId) const;
	void processRegistrationNotification(const rina::IPCProcessDIFRegistrationEvent& event);
	bool isSupportingDIF(const rina::ApplicationProcessNamingInformation& difName);
	std::list<rina::FlowInformation> getAllNMinusOneFlowInformation() const;

private:
	IPCProcess * ipc_process_;
	IRIBDaemon * rib_daemon_;
	rina::CDAPSessionManagerInterface * cdap_session_manager_;

	///Populate the IPC Process RIB with the objects related to N-1 Flow Management
	void populateRIB();

	///Remove the N-1 flow object from the RIB and send an internal notification
	void cleanFlowAndNotify(int portId);
};

class ResourceAllocator: public IResourceAllocator {
public:
	ResourceAllocator();
	~ResourceAllocator();
	void set_ipc_process(IPCProcess * ipc_process);
	INMinusOneFlowManager * get_n_minus_one_flow_manager() const;
	IPDUForwardingTableGenerator * get_pdu_forwarding_table_generator() const;

private:
	IPCProcess * ipc_process_;
	INMinusOneFlowManager * n_minus_one_flow_manager_;
	IPDUForwardingTableGenerator * pdu_forwarding_table_generator_;
};

}

#endif

#endif
