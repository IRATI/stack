//
// Resource Allocator
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <sstream>

#include <librina/internal-events.h>

#define IPCP_MODULE "resource-allocator"

#include "ipcp-logging.h"
#include "resource-allocator.h"

namespace rinad {

//Class NMinusOneFlowManager
NMinusOneFlowManager::NMinusOneFlowManager()
{
	rib_daemon_ = 0;
	ipc_process_ = 0;
	cdap_session_manager_ = 0;
	flow_acceptor_ = 0;
}

NMinusOneFlowManager::~NMinusOneFlowManager()
{
	if (flow_acceptor_) {
		delete flow_acceptor_;
	}
}

void NMinusOneFlowManager::set_ipc_process(IPCProcess * ipc_process)
{
	app = ipc_process;
	ipc_process_ = ipc_process;
	rib_daemon_ = ipc_process->rib_daemon_;
	cdap_session_manager_ = ipc_process->cdap_session_manager_;
	event_manager_ = ipc_process->internal_event_manager_;
	flow_acceptor_ = new IPCPFlowAcceptor(ipc_process_);
	set_flow_acceptor(flow_acceptor_);
	populateRIB();
}

void NMinusOneFlowManager::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	LOG_IPCP_DBG("DIF configuration set %u", dif_configuration.address_);
}

void NMinusOneFlowManager::processRegistrationNotification(const rina::IPCProcessDIFRegistrationEvent& event) {
	if (event.isRegistered()) {
		try {
			rina::extendedIPCManager->appRegistered(event.getIPCProcessName(), event.getDIFName());
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		LOG_IPCP_INFO("IPC Process registered to N-1 DIF %s",
				event.getDIFName().processName.c_str());
		try{
			std::stringstream ss;
			ss<<rina::DIFRegistrationSetRIBObject::DIF_REGISTRATION_SET_RIB_OBJECT_NAME;
			ss<<rina::RIBNamingConstants::SEPARATOR<<event.getDIFName().processName;
			rib_daemon_->createObject(rina::DIFRegistrationSetRIBObject::DIF_REGISTRATION_RIB_OBJECT_CLASS,
					ss.str(), &(event.getDIFName().processName), 0);
		}catch(rina::Exception &e){
			LOG_IPCP_ERR("Problems creating RIB object: %s", e.what());;
		}

		return;
	}

	try {
		rina::extendedIPCManager->appUnregistered(event.getIPCProcessName(), event.getDIFName());
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}

	LOG_IPCP_INFO("IPC Process unregistered from N-1 DIF %s",
			event.getDIFName().processName.c_str());

	try {
		std::stringstream ss;
		ss<<rina::DIFRegistrationSetRIBObject::DIF_REGISTRATION_SET_RIB_OBJECT_NAME;
		ss<<rina::RIBNamingConstants::SEPARATOR<<event.getDIFName().processName;
		rib_daemon_->deleteObject(rina::DIFRegistrationSetRIBObject::DIF_REGISTRATION_RIB_OBJECT_CLASS,
								  ss.str(), 0, 0);
	}catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems deleting object from RIB: %s", e.what());
	}
}

std::list<int> NMinusOneFlowManager::getNMinusOneFlowsToNeighbour(unsigned int address) {
	std::vector<rina::FlowInformation> flows = rina::extendedIPCManager->getAllocatedFlows();
	std::list<int> result;
	unsigned int target_address = 0;
	for (unsigned int i=0; i<flows.size(); i++) {
		target_address = ipc_process_->namespace_manager_->getAdressByname(
				flows[i].remoteAppName);
		if (target_address == address) {
			result.push_back(flows[i].portId);
		}
	}

	return result;
}

int NMinusOneFlowManager::getManagementFlowToNeighbour(unsigned int address) {
	const std::list<rina::Neighbor*> neighbors = ipc_process_->get_neighbors();
	for (std::list<rina::Neighbor*>::const_iterator it = neighbors.begin();
			it != neighbors.end(); ++it) {
		if ((*it)->address_ == address) {
			return (*it)->underlying_port_id_;
		}
	}

	return -1;
}

unsigned int NMinusOneFlowManager::numberOfFlowsToNeighbour(const std::string& apn,
		const std::string& api) {
	std::vector<rina::FlowInformation> flows = rina::extendedIPCManager->getAllocatedFlows();
	unsigned int result = 0;
	for (unsigned int i=0; i<flows.size(); i++) {
		if (flows[i].remoteAppName.processName == apn &&
				flows[i].remoteAppName.processInstance == api) {
			result ++;
		}
	}

	return result;
}

//Class IPCP Flow Acceptor
bool IPCPFlowAcceptor::accept_flow(const rina::FlowRequestEvent& event)
{
	if (ipcp_->get_operational_state() != ASSIGNED_TO_DIF) {
		return false;
	}

	//TODO deal with the different AEs (Management vs. Data transfer), right now assuming the flow
	//is both used for data transfer and management purposes
	try {
		rina::extendedIPCManager->getPortIdToRemoteApp(event.remoteApplicationName);
		LOG_IPCP_INFO("Rejecting flow request since we already have a flow to the remote IPC Process: %s-%s",
				event.remoteApplicationName.processName.c_str(),
				event.remoteApplicationName.processInstance.c_str());
		return false;
	} catch(rina::Exception & ex) {
		return true;
	}
}

//CLASS Resource Allocator
ResourceAllocator::ResourceAllocator() : IResourceAllocator() {
	n_minus_one_flow_manager_ = new NMinusOneFlowManager();
	ipcp = 0;
}

ResourceAllocator::~ResourceAllocator() {
	if (n_minus_one_flow_manager_) {
		delete n_minus_one_flow_manager_;
	}
}

void ResourceAllocator::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
		return;

	app = ap;
	ipcp = dynamic_cast<IPCProcess*>(app);
	if (!ipcp) {
		LOG_IPCP_ERR("Bogus instance of IPCP passed, return");
		return;
	}

	if (n_minus_one_flow_manager_) {
		n_minus_one_flow_manager_->set_ipc_process(ipcp);
	}
}

void ResourceAllocator::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	std::string ps_name = dif_configuration.ra_configuration_.pduftg_conf_.policy_set_.name_;
	if (set_pduft_gen_policy_set(ps_name) != 0) {
		throw rina::Exception("Cannot create PDU Forwarding Table Generator policy-set");
	}

	if (n_minus_one_flow_manager_) {
		n_minus_one_flow_manager_->set_dif_configuration(dif_configuration);
	}
}

INMinusOneFlowManager * ResourceAllocator::get_n_minus_one_flow_manager() const {
	return n_minus_one_flow_manager_;
}

} //namespace rinad
