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
		handle = rina::extendedIPCManager->requestFlowAllocationInDIF(flowInformation.getLocalAppName(),
				flowInformation.getRemoteAppName(), flowInformation.getDifName(),
				flowInformation.getFlowSpecification());
	} catch(rina::FlowAllocationException &e) {
		throw Exception(e.what());
	}

	LOG_INFO("Requested the allocation of N-1 flow to application %s-%s through DIF %s",
			flowInformation.getRemoteAppName().getProcessName().c_str(),
			flowInformation.getRemoteAppName().getProcessInstance().c_str(),
			flowInformation.getDifName().getProcessName().c_str());

	return handle;
}

void NMinusOneFlowManager::allocateRequestResult(const rina::AllocateFlowRequestResultEvent& event) {
	if (event.getPortId() <= 0) {
		LOG_ERR("Allocation of N-1 flow denied. Error code: %d", event.getPortId());
		rina::FlowInformation flowInformation = rina::extendedIPCManager->withdrawPendingFlow(event.getSequenceNumber());
		Event * flowFailedEvent = new NMinusOneFlowAllocationFailedEvent(event.getSequenceNumber(),
				flowInformation, ""+event.getPortId());
		rib_daemon_->deliverEvent(flowFailedEvent);
		return;
	}

	rina::Flow * flow = rina::extendedIPCManager->commitPendingFlow(event.getSequenceNumber(),
				event.getPortId(), event.getDIFName());
	try {
		std::stringstream ss;
		ss<<EncoderConstants::N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME;
		ss<<EncoderConstants::SEPARATOR<<event.getPortId();
		rib_daemon_->createObject(EncoderConstants::N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS,
				ss.str(), &(flow->getFlowInformation()), 0);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}

	Event * flowAllocatedEvent = new NMinusOneFlowAllocatedEvent(event.getSequenceNumber(),
			flow->getFlowInformation());
	rib_daemon_->deliverEvent(flowAllocatedEvent);
}

void NMinusOneFlowManager::flowAllocationRequested(const rina::FlowRequestEvent& event) {
	if (event.getLocalApplicationName().getProcessName().compare(
			ipc_process_->get_name().getProcessName()) == 0 &&
			event.getLocalApplicationName().getProcessInstance().compare(
					ipc_process_->get_name().getProcessInstance()) == 0) {
		//TODO deal with the different AEs (Management vs. Data transfer), right now assuming the flow
		//is both used for data transfer and management purposes

		if (rina::extendedIPCManager->getFlowToRemoteApp(event.getRemoteApplicationName()) != 0) {
			LOG_INFO("Rejecting flow since we already have a flow to the remote IPC Process: %s-%s",
					event.getRemoteApplicationName().getProcessName().c_str(),
					event.getRemoteApplicationName().getProcessInstance().c_str());
			rina::extendedIPCManager->allocateFlowResponse(event, -1, true);
			return;
		}

		rina::Flow * flow = rina::extendedIPCManager->allocateFlowResponse(event, 0, true);
		LOG_INFO("Accepted new flow from IPC Process %s-%s",
				event.getRemoteApplicationName().getProcessName().c_str(),
				event.getRemoteApplicationName().getProcessInstance().c_str());
		try {
			std::stringstream ss;
			ss<<EncoderConstants::N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME;
			ss<<EncoderConstants::SEPARATOR<<event.getPortId();
			rib_daemon_->createObject(EncoderConstants::N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS,
					ss.str(), &(flow->getFlowInformation()), 0);
		} catch (Exception &e){
			LOG_ERR("Error creating RIB object: %s", e.what());
		}

		Event * flowAllocatedEvent = new NMinusOneFlowAllocatedEvent(event.getSequenceNumber(),
				flow->getFlowInformation());
		rib_daemon_->deliverEvent(flowAllocatedEvent);
		return;
	}

	LOG_ERR("Rejected flow from %s-%s since this IPC Process is not the intended target of this flow",
			event.getRemoteApplicationName().getProcessName().c_str(),
			event.getRemoteApplicationName().getProcessInstance().c_str());
	rina::extendedIPCManager->allocateFlowResponse(event, -1, true);
}

void NMinusOneFlowManager::deallocateNMinus1Flow(int portId) {
	rina::extendedIPCManager->requestFlowDeallocation(portId);
}

void NMinusOneFlowManager::deallocateFlowResponse(
		const rina::DeallocateFlowResponseEvent& event) {
	bool success = false;

	if (event.getResult() == 0){
		success = true;
	}

	rina::extendedIPCManager->flowDeallocationResult(event.getPortId(), success);
	cleanFlowAndNotify(event.getPortId());
}

void NMinusOneFlowManager::flowDeallocatedRemotely(const rina::FlowDeallocatedEvent& event) {
	rina::extendedIPCManager->flowDeallocated(event.getPortId());
	cleanFlowAndNotify(event.getPortId());
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
		rina::extendedIPCManager->appRegistered(event.getIPCProcessName(), event.getDIFName());
		LOG_INFO("IPC Process registered to N-1 DIF %s",
					event.getDIFName().getProcessName().c_str());
		try{
			std::stringstream ss;
			ss<<EncoderConstants::DIF_REGISTRATION_SET_RIB_OBJECT_NAME;
			ss<<EncoderConstants::SEPARATOR<<event.getDIFName().getProcessName();
			rib_daemon_->createObject(EncoderConstants::DIF_REGISTRATION_RIB_OBJECT_CLASS, ss.str(),
					&(event.getDIFName().getProcessName()), 0);
		}catch(Exception &e){
			LOG_ERR("Problems creating RIB object: %s", e.what());;
		}

		return;
	}

	rina::extendedIPCManager->appUnregistered(event.getIPCProcessName(), event.getDIFName());
	LOG_INFO("IPC Process unregistered from N-1 DIF %s",
			event.getDIFName().getProcessName().c_str());

	try {
		std::stringstream ss;
		ss<<EncoderConstants::DIF_REGISTRATION_SET_RIB_OBJECT_NAME;
		ss<<EncoderConstants::SEPARATOR<<event.getDIFName().getProcessName();
		rib_daemon_->deleteObject(EncoderConstants::DIF_REGISTRATION_RIB_OBJECT_CLASS, ss.str(), 0, 0);
	}catch (Exception &e) {
		LOG_ERR("Problems deleting object from RIB: %s", e.what());
	}
}

bool NMinusOneFlowManager::isSupportingDIF(const rina::ApplicationProcessNamingInformation& difName) {
	std::vector<rina::ApplicationRegistration*> registrations = rina::extendedIPCManager->getRegisteredApplications();
	std::list<rina::ApplicationProcessNamingInformation> namesList;
	for(unsigned int i=0; i<registrations.size(); i++) {
		namesList = registrations[i]->getDIFNames();
		for (std::list<rina::ApplicationProcessNamingInformation>::iterator it = namesList.begin();
				it != namesList.end(); ++it){
			if (it->getProcessName().compare(difName.getProcessName()) == 0) {
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

INMinusOneFlowManager * ResourceAllocator::get_n_minus_one_flow_manager() const {
	return n_minus_one_flow_manager_;
}

IPDUForwardingTableGenerator * ResourceAllocator::get_pdu_forwarding_table_generator() const {
	return pdu_forwarding_table_generator_;
}

}
