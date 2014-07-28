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

#define RINA_PREFIX "resource-allocator"

#include <librina/logs.h>

#include "events.h"
#include "resource-allocator.h"
#include "pduft-generator.h"

namespace rinad {

//Class NMinusOneFlowManager
NMinusOneFlowManager::NMinusOneFlowManager() {
	rib_daemon_ = 0;
	ipc_process_ = 0;
	cdap_session_manager_ = 0;
}

void NMinusOneFlowManager::set_ipc_process(IPCProcess * ipc_process) {
	ipc_process_ = ipc_process;
	rib_daemon_ = ipc_process->get_rib_daemon();
	cdap_session_manager_ = ipc_process->get_cdap_session_manager();
}

void NMinusOneFlowManager::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	LOG_DBG("DIF configuration set %u", dif_configuration.get_address());
}

void NMinusOneFlowManager::populateRIB(){
	try {
		BaseRIBObject * object = new SimpleSetRIBObject(ipc_process_,
				EncoderConstants::DIF_REGISTRATION_SET_RIB_OBJECT_CLASS,
				EncoderConstants::DIF_REGISTRATION_RIB_OBJECT_CLASS,
				EncoderConstants::DIF_REGISTRATION_SET_RIB_OBJECT_NAME);
		rib_daemon_->addRIBObject(object);
		object = new SimpleSetRIBObject(ipc_process_,
				EncoderConstants::N_MINUS_ONE_FLOW_SET_RIB_OBJECT_CLASS,
				EncoderConstants::N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS,
				EncoderConstants::N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME);
		rib_daemon_->addRIBObject(object);
	} catch (Exception &e) {
		LOG_ERR("Problems adding object to the RIB : %s", e.what());
	}
}

const rina::FlowInformation& NMinusOneFlowManager::getNMinus1FlowInformation(int portId)
const {
	rina::Flow * flow = rina::extendedIPCManager->getAllocatedFlow(portId);
	if (flow == 0) {
		throw Exception("Unknown N-1 flow");
	}

	return flow->getFlowInformation();
}

unsigned int NMinusOneFlowManager::allocateNMinus1Flow(const rina::FlowInformation& flowInformation) {
	unsigned int handle = 0;

	try {
		handle = rina::extendedIPCManager->requestFlowAllocationInDIF(flowInformation.localAppName,
				flowInformation.remoteAppName, flowInformation.difName,
				flowInformation.flowSpecification);
	} catch(rina::FlowAllocationException &e) {
		throw Exception(e.what());
	}

	LOG_INFO("Requested the allocation of N-1 flow to application %s-%s through DIF %s",
			flowInformation.remoteAppName.processName.c_str(),
			flowInformation.remoteAppName.processInstance.c_str(),
			flowInformation.difName.processName.c_str());

	return handle;
}

void NMinusOneFlowManager::allocateRequestResult(const rina::AllocateFlowRequestResultEvent& event) {
	if (event.portId <= 0) {
		LOG_ERR("Allocation of N-1 flow denied. Error code: %d", event.portId);
		rina::FlowInformation flowInformation = rina::extendedIPCManager->withdrawPendingFlow(event.sequenceNumber);
		Event * flowFailedEvent = new NMinusOneFlowAllocationFailedEvent(event.sequenceNumber,
				flowInformation, ""+event.portId);
		rib_daemon_->deliverEvent(flowFailedEvent);
		return;
	}

	rina::Flow * flow = rina::extendedIPCManager->commitPendingFlow(event.sequenceNumber,
				event.portId, event.difName);
	try {
		std::stringstream ss;
		ss<<EncoderConstants::N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME;
		ss<<EncoderConstants::SEPARATOR<<event.portId;
		rib_daemon_->createObject(EncoderConstants::N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS,
				ss.str(), &(flow->getFlowInformation()), 0);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}

	Event * flowAllocatedEvent = new NMinusOneFlowAllocatedEvent(event.sequenceNumber,
			flow->getFlowInformation());
	rib_daemon_->deliverEvent(flowAllocatedEvent);
}

void NMinusOneFlowManager::flowAllocationRequested(const rina::FlowRequestEvent& event) {
	if (event.localApplicationName.processName.compare(
			ipc_process_->get_name().processName) != 0 ||
			event.localApplicationName.processInstance.compare(
					ipc_process_->get_name().processInstance) != 0) {
		LOG_ERR("Rejected flow request from %s-%s since this IPC Process is not the intended target of this flow",
				event.remoteApplicationName.processName.c_str(),
				event.remoteApplicationName.processInstance.c_str());
		try {
			rina::extendedIPCManager->allocateFlowResponse(event, -1, true);
		} catch (Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}
		return;
	}

	//In this implementation we cannot accept flows if the IPC Process is not assigned to a DIF
	if (ipc_process_->get_operational_state() != ASSIGNED_TO_DIF) {
		LOG_ERR("Rejecting flow request since the IPC Process is not ASSIGNED to a DIF");
		try {
			rina::extendedIPCManager->allocateFlowResponse(event, -1, true);
		} catch (Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}
		return;
	}

	//TODO deal with the different AEs (Management vs. Data transfer), right now assuming the flow
	//is both used for data transfer and management purposes

	if (rina::extendedIPCManager->getFlowToRemoteApp(event.remoteApplicationName) != 0) {
		LOG_INFO("Rejecting flow request since we already have a flow to the remote IPC Process: %s-%s",
				event.remoteApplicationName.processName.c_str(),
				event.remoteApplicationName.processInstance.c_str());
		try {
			rina::extendedIPCManager->allocateFlowResponse(event, -1, true);
		} catch (Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}
		return;
	}

	rina::Flow * flow = 0;
	try {
		flow = rina::extendedIPCManager->allocateFlowResponse(event, 0, true);
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		if (!flow) {
			return;
		}
	}

	LOG_INFO("Accepted new flow from IPC Process %s-%s",
			event.remoteApplicationName.processName.c_str(),
			event.remoteApplicationName.processInstance.c_str());
	try {
		std::stringstream ss;
		ss<<EncoderConstants::N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME;
		ss<<EncoderConstants::SEPARATOR<<event.portId;
		rib_daemon_->createObject(EncoderConstants::N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS,
				ss.str(), &(flow->getFlowInformation()), 0);
	} catch (Exception &e){
		LOG_ERR("Error creating RIB object: %s", e.what());
	}

	Event * flowAllocatedEvent = new NMinusOneFlowAllocatedEvent(event.sequenceNumber,
			flow->getFlowInformation());
	rib_daemon_->deliverEvent(flowAllocatedEvent);
	return;
}

void NMinusOneFlowManager::deallocateNMinus1Flow(int portId) {
	rina::extendedIPCManager->requestFlowDeallocation(portId);
}

void NMinusOneFlowManager::deallocateFlowResponse(
		const rina::DeallocateFlowResponseEvent& event) {
	bool success = false;

	if (event.result == 0){
		success = true;
	}

	try {
		rina::extendedIPCManager->flowDeallocationResult(event.portId, success);
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}

	cleanFlowAndNotify(event.portId);
}

void NMinusOneFlowManager::flowDeallocatedRemotely(const rina::FlowDeallocatedEvent& event) {
	try {
		rina::extendedIPCManager->flowDeallocated(event.portId);
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}

	cleanFlowAndNotify(event.portId);
}

void NMinusOneFlowManager::cleanFlowAndNotify(int portId) {
	try{
		std::stringstream ss;
		ss<<EncoderConstants::N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME;
		ss<<EncoderConstants::SEPARATOR<<portId;
		rib_daemon_->deleteObject(EncoderConstants::N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS, ss.str(), 0, 0);
	}catch(Exception &e) {
		LOG_ERR("Problems deleting object from the RIB: %s", e.what());
	}

	//Notify about the event
	const rina::CDAPSessionInterface * cdapSession = cdap_session_manager_->get_cdap_session(portId);
	rina::CDAPSessionDescriptor * cdapSessionDescriptor = 0;
	if (cdapSession) {
		cdapSessionDescriptor = cdapSession->get_session_descriptor();
	}

	NMinusOneFlowDeallocatedEvent * flowDeEvent =
			new NMinusOneFlowDeallocatedEvent(portId, cdapSessionDescriptor);
	rib_daemon_->deliverEvent(flowDeEvent);
}

void NMinusOneFlowManager::processRegistrationNotification(const rina::IPCProcessDIFRegistrationEvent& event) {
	if (event.isRegistered()) {
		try {
			rina::extendedIPCManager->appRegistered(event.getIPCProcessName(), event.getDIFName());
		} catch (Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		LOG_INFO("IPC Process registered to N-1 DIF %s",
					event.getDIFName().processName.c_str());
		try{
			std::stringstream ss;
			ss<<EncoderConstants::DIF_REGISTRATION_SET_RIB_OBJECT_NAME;
			ss<<EncoderConstants::SEPARATOR<<event.getDIFName().processName;
			rib_daemon_->createObject(EncoderConstants::DIF_REGISTRATION_RIB_OBJECT_CLASS, ss.str(),
					&(event.getDIFName().processName), 0);
		}catch(Exception &e){
			LOG_ERR("Problems creating RIB object: %s", e.what());;
		}

		return;
	}

	try {
		rina::extendedIPCManager->appUnregistered(event.getIPCProcessName(), event.getDIFName());
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}

	LOG_INFO("IPC Process unregistered from N-1 DIF %s",
			event.getDIFName().processName.c_str());

	try {
		std::stringstream ss;
		ss<<EncoderConstants::DIF_REGISTRATION_SET_RIB_OBJECT_NAME;
		ss<<EncoderConstants::SEPARATOR<<event.getDIFName().processName;
		rib_daemon_->deleteObject(EncoderConstants::DIF_REGISTRATION_RIB_OBJECT_CLASS, ss.str(), 0, 0);
	}catch (Exception &e) {
		LOG_ERR("Problems deleting object from RIB: %s", e.what());
	}
}

bool NMinusOneFlowManager::isSupportingDIF(const rina::ApplicationProcessNamingInformation& difName) {
	std::vector<rina::ApplicationRegistration*> registrations = rina::extendedIPCManager->getRegisteredApplications();
	std::list<rina::ApplicationProcessNamingInformation> namesList;
	for(unsigned int i=0; i<registrations.size(); i++) {
		namesList = registrations[i]->DIFNames;
		for (std::list<rina::ApplicationProcessNamingInformation>::iterator it = namesList.begin();
				it != namesList.end(); ++it){
			if (it->processName.compare(difName.processName) == 0) {
				return true;
			}
		}
	}

	return false;
}

std::list<rina::FlowInformation> NMinusOneFlowManager::getAllNMinusOneFlowInformation() const {
	std::vector<rina::Flow *> flows = rina::extendedIPCManager->getAllocatedFlows();
	std::list<rina::FlowInformation> result;
	for (unsigned int i=0; i<flows.size(); i++) {
		result.push_back(flows[i]->getFlowInformation());
	}

	return result;
}

//CLASS Resource Allocator
ResourceAllocator::ResourceAllocator() {
	n_minus_one_flow_manager_ = new NMinusOneFlowManager();
	pdu_forwarding_table_generator_ = new PDUForwardingTableGenerator();
	ipc_process_ = 0;
}

ResourceAllocator::~ResourceAllocator() {
	if (n_minus_one_flow_manager_) {
		delete n_minus_one_flow_manager_;
	}

	if (pdu_forwarding_table_generator_) {
		delete pdu_forwarding_table_generator_;
	}
}

void ResourceAllocator::set_ipc_process(IPCProcess * ipc_process) {
	if (!ipc_process) {
		return;
	}

	ipc_process_ = ipc_process;
	if (n_minus_one_flow_manager_) {
		n_minus_one_flow_manager_->set_ipc_process(ipc_process);
	}

	if (pdu_forwarding_table_generator_) {
		pdu_forwarding_table_generator_->set_ipc_process(ipc_process);
	}
}

void ResourceAllocator::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	if (n_minus_one_flow_manager_) {
		n_minus_one_flow_manager_->set_dif_configuration(dif_configuration);
	}

	if (pdu_forwarding_table_generator_) {
		pdu_forwarding_table_generator_->set_dif_configuration(dif_configuration);
	}
}

INMinusOneFlowManager * ResourceAllocator::get_n_minus_one_flow_manager() const {
	return n_minus_one_flow_manager_;
}

IPDUForwardingTableGenerator * ResourceAllocator::get_pdu_forwarding_table_generator() const {
	return pdu_forwarding_table_generator_;
}

}
