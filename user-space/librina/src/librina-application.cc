/*
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

#include <cstring>
#include <stdexcept>

#define RINA_PREFIX "application"

#include "logs.h"
#include "librina-application.h"
#include "core.h"

namespace rina {

/* CLASS FLOW */

Flow::Flow(const ApplicationProcessNamingInformation& sourceApplicationName,
		const ApplicationProcessNamingInformation& destinationApplicationName,
		const FlowSpecification& flowSpecification, FlowState flowState,
		const ApplicationProcessNamingInformation& DIFName, int portId) {
	this->sourceApplicationName = sourceApplicationName;
	this->destinationApplicationName = destinationApplicationName;
	this->flowSpecification = flowSpecification;
	this->DIFName = DIFName;
	this->flowState = flowState;
	this->portId = portId;
}

const std::string Flow::flow_not_allocated_error =
		"The flow is not in ALLOCATED state";

const FlowState& Flow::getState() const {
	return flowState;
}

int Flow::getPortId() const {
	return portId;
}

const ApplicationProcessNamingInformation& Flow::getDIFName() const {
	return DIFName;
}

const ApplicationProcessNamingInformation& Flow::getSourceApplicationName() const {
	return sourceApplicationName;
}

const ApplicationProcessNamingInformation& Flow::getDestinationApplcationName() const {
	return destinationApplicationName;
}

const FlowSpecification Flow::getFlowSpecification() const {
	return flowSpecification;
}

int Flow::readSDU(unsigned char * sdu) throw (IPCException) {
	LOG_DBG("Flow.readSDU called");

	if (flowState != FLOW_ALLOCATED) {
		throw IPCException(Flow::flow_not_allocated_error);
	}

#if STUB_API
	unsigned char buffer[] = { 0, 23, 43, 32, 45, 23, 78 };
	sdu = buffer;
	return 7;
#else
	//TODO add real implementation here
	return 0;
#endif
}

void Flow::writeSDU(unsigned char * sdu, int size) throw (IPCException) {
	LOG_DBG("Flow.writeSDU called");

	if (flowState != FLOW_ALLOCATED) {
		throw IPCException(Flow::flow_not_allocated_error);
	}

#if STUB_API
	int i;
	for (i = 0; i < size; i++) {
		LOG_DBG("SDU[%d] = %d", i, sdu[i]);
	}
#else
	//TODO add real implementation here
#endif
}

/* CLASS APPLICATION REGISTRATION */
ApplicationRegistration::ApplicationRegistration(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const ApplicationProcessNamingInformation& ApplicationRegistration::getApplicationName() const {
	return applicationName;
}

const std::list<ApplicationProcessNamingInformation>& ApplicationRegistration::getDIFNames() const {
	return DIFNames;
}

void ApplicationRegistration::addDIFName(
		const ApplicationProcessNamingInformation& DIFName) {
	DIFNames.push_back(DIFName);
}

void ApplicationRegistration::removeDIFName(
		const ApplicationProcessNamingInformation& DIFName) {
	DIFNames.remove(DIFName);
}

/* CLASS IPC MANAGER */
IPCManager::IPCManager() {
}

IPCManager::~IPCManager() {
}

const std::string IPCManager::application_registered_error =
		"The application is already registered in this DIF";
const std::string IPCManager::application_not_registered_error =
		"The application is not registered in this DIF";
const std::string IPCManager::unknown_flow_error =
		"There is no flow at the specified portId";
const std::string IPCManager::error_registering_application =
		"Error registering application";
const std::string IPCManager::error_unregistering_application =
		"Error unregistering application";
const std::string IPCManager::error_requesting_flow_allocation =
		"Error requesting flow allocation";
const std::string IPCManager::error_requesting_flow_deallocation =
		"Error requesting flow deallocation";

/* Auxiliar function called in case of using the stubbed version of the API */
std::vector<DIFProperties> getFakeDIFProperties(
		const ApplicationProcessNamingInformation& DIFName) {
	std::vector<DIFProperties> result;
	DIFProperties * properties;
	ApplicationProcessNamingInformation * name;

	if (DIFName.getProcessName().compare("") != 0) {
		properties = new DIFProperties(DIFName, 2000);
		result.push_back(*properties);
		return result;
	}

	name = new ApplicationProcessNamingInformation("test.DIF", "");
	properties = new DIFProperties(*name, 5000);
	result.push_back(*properties);
	name = new ApplicationProcessNamingInformation("Public-Internet.DIF", "");
	properties = new DIFProperties(*name, 10000);
	result.push_back(*properties);
	return result;
}

std::vector<DIFProperties> IPCManager::getDIFProperties(
		const ApplicationProcessNamingInformation& DIFName) {
	LOG_DBG("IPCManager.getDIFProperties called");

#if STUB_API
	return getFakeDIFProperties(DIFName);
#else
	std::vector<DIFProperties> result;
	//TODO add the real implementation
	return result;
#endif
}

void IPCManager::registerApplication(
		const ApplicationProcessNamingInformation& applicationName,
		const ApplicationProcessNamingInformation& DIFName)
throw (IPCException) {
	LOG_DBG("IPCManager.registerApplication called");
	ApplicationRegistration * applicationRegistration = 0;

	std::map<ApplicationProcessNamingInformation,
	ApplicationRegistration*>::iterator it =
			applicationRegistrations.find(applicationName);

	if (it != applicationRegistrations.end()){
		applicationRegistration = it->second;
		std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
		for (iterator = applicationRegistration->getDIFNames().begin();
				iterator != applicationRegistration->getDIFNames().end();
				++iterator) {
			if (*iterator == DIFName) {
				throw IPCException(IPCManager::application_registered_error);
			}
		}
	}

#if STUB_API
	//Do nothing
#else
	AppRegisterApplicationRequestMessage message;
	message.setApplicationName(applicationName);
	message.setDifName(DIFName);
	message.setRequestMessage(true);

	AppRegisterApplicationResponseMessage * registerResponseMessage =
			dynamic_cast<AppRegisterApplicationResponseMessage *>(
					rinaManager->sendRequestAndWaitForResponse(&message,
							IPCManager::error_registering_application));

	if (registerResponseMessage->getResult() < 0){
		std::string reason = IPCManager::error_registering_application +
				registerResponseMessage->getErrorDescription();
		delete registerResponseMessage;
		throw IPCException(reason);
	}

	LOG_DBG("Application %s registered successfully to DIF %s",
			applicationName.getProcessName().c_str(),
			DIFName.getProcessName().c_str());
	delete registerResponseMessage;
#endif

	if (!applicationRegistration){
		applicationRegistration = new ApplicationRegistration(applicationName);
		applicationRegistrations[applicationName] = applicationRegistration;
	}

	applicationRegistration->addDIFName(DIFName);
}

void IPCManager::unregisterApplication(
		ApplicationProcessNamingInformation applicationName,
		ApplicationProcessNamingInformation DIFName) throw (IPCException) {
	LOG_DBG("IPCManager.unregisterApplication called");

#if STUB_API
	//Do nothing
#else
	AppUnregisterApplicationRequestMessage * message =
			new AppUnregisterApplicationRequestMessage();
	message->setApplicationName(applicationName);
	message->setDifName(DIFName);
	message->setRequestMessage(true);

	AppUnregisterApplicationResponseMessage * unregisterResponseMessage =
			dynamic_cast<AppUnregisterApplicationResponseMessage *>(
					rinaManager->sendRequestAndWaitForResponse(message,
							IPCManager::error_unregistering_application));

	if (unregisterResponseMessage->getResult() < 0){
		std::string reason = IPCManager::error_unregistering_application +
				unregisterResponseMessage->getErrorDescription();
		delete unregisterResponseMessage;
		throw IPCException(reason);
	}

	LOG_DBG("Application %s unregistered successfully to DIF %s",
			applicationName.getProcessName().c_str(),
			DIFName.getProcessName().c_str());
	delete unregisterResponseMessage;
#endif

	ApplicationRegistration * applicationRegistration = 0;

	std::map<ApplicationProcessNamingInformation,
	ApplicationRegistration*>::iterator it =
			applicationRegistrations.find(applicationName);

	if (it == applicationRegistrations.end()){
		throw IPCException(IPCManager::application_not_registered_error);
	}else{
		applicationRegistration = it->second;
	}

	std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
	for (iterator = applicationRegistration->getDIFNames().begin();
			iterator != applicationRegistration->getDIFNames().end();
			++iterator) {
		if (*iterator == DIFName) {
			applicationRegistration->removeDIFName(DIFName);
			if (applicationRegistration->getDIFNames().size() == 0) {
				applicationRegistrations.erase(applicationName);
			}

			return;
		}
	}

	throw IPCException(IPCManager::application_not_registered_error);
}

int getFakePortId(std::map<int, Flow*> allocatedFlows){
	int portId = 0;
	while(allocatedFlows.find(portId) != allocatedFlows.end()){
		portId++;
	}

	return portId;
}

Flow * IPCManager::allocateFlowRequest(
		const ApplicationProcessNamingInformation& sourceAppName,
		const ApplicationProcessNamingInformation& destAppName,
		const FlowSpecification& flowSpec) throw (IPCException) {
	LOG_DBG("IPCManager.allocateFlowRequest called");

	int portId = 0;
	Flow * flow = 0;

#if STUB_API
	ApplicationProcessNamingInformation DIFName =
			ApplicationProcessNamingInformation("test.DIF", "");
	portId = getFakePortId(allocatedFlows);
	flow = new Flow(sourceAppName, destAppName, flowSpec, FLOW_ALLOCATED,
			DIFName, portId);
#else
	AppAllocateFlowRequestMessage message;
	message.setSourceAppName(sourceAppName);
	message.setDestAppName(destAppName);
	message.setFlowSpecification(flowSpec);
	message.setRequestMessage(true);

	AppAllocateFlowRequestResultMessage * flowRequestResponse =
			dynamic_cast<AppAllocateFlowRequestResultMessage *>(
					rinaManager->sendRequestAndWaitForResponse(&message,
							IPCManager::error_requesting_flow_allocation));

	if (flowRequestResponse->getPortId() < 0){
		std::string reason = IPCManager::error_requesting_flow_allocation +
				flowRequestResponse->getErrorDescription();
		delete flowRequestResponse;
		throw IPCException(reason);
	}

	LOG_DBG("Flow from %s to %s allocated successfully! Port-id: %d",
			sourceAppName.getProcessName().c_str(),
			destAppName.getProcessName().c_str(),
			flowRequestResponse->getPortId());
	portId = flowRequestResponse->getPortId();
	flow = new Flow(sourceAppName, destAppName, flowSpec, FLOW_ALLOCATED,
			flowRequestResponse->getDifName(), portId);
	delete flowRequestResponse;
#endif

	allocatedFlows[portId] = flow;
	return flow;
}

Flow * IPCManager::allocateFlowResponse(
		const FlowRequestEvent& flowRequestEvent, bool accept,
		const std::string& reason) throw (IPCException) {
	LOG_DBG("IPCManager.allocateFlowResponse called");

	if (!accept) {
		LOG_WARN("Flow was not accepted because: %s", reason.c_str());
		return 0;
	}

#if STUB_API
	//Do nothing
#else
	AppAllocateFlowResponseMessage message;
	message.setAccept(accept);
	message.setDenyReason(reason);
	message.setNotifySource(true);
	message.setSequenceNumber(flowRequestEvent.getSequenceNumber());
#endif

	Flow * flow = new Flow(flowRequestEvent.getSourceApplicationName(),
			flowRequestEvent.getDestApplicationName(),
			flowRequestEvent.getFlowSpecification(), FLOW_ALLOCATED,
			flowRequestEvent.getDIFName(), flowRequestEvent.getPortId());
	allocatedFlows[flowRequestEvent.getPortId()] = flow;
	return flow;
}

void IPCManager::deallocateFlow(
		int portId, const ApplicationProcessNamingInformation& applicationName)
throw (IPCException) {
	LOG_DBG("IPCManager.deallocateFlow called");

	Flow * flow = 0;
	std::map<int, Flow*>::iterator iterator;
	iterator = allocatedFlows.find(portId);
	if (iterator == allocatedFlows.end()) {
		throw IPCException(IPCManager::unknown_flow_error);
	}
	flow = iterator->second;

#if STUB_API
	//Do nothing
#else
	AppDeallocateFlowRequestMessage message;
	message.setApplicationName(applicationName);
	message.setPortId(portId);
	message.setDifName(flow->getDIFName());
	message.setRequestMessage(true);

	AppDeallocateFlowResponseMessage * deallocateResponse =
			dynamic_cast<AppDeallocateFlowResponseMessage *>(
					rinaManager->sendRequestAndWaitForResponse(&message,
							IPCManager::error_requesting_flow_deallocation));

	if (deallocateResponse->getResult() < 0){
		std::string reason = IPCManager::error_requesting_flow_deallocation +
				deallocateResponse->getErrorDescription();
		delete deallocateResponse;
		throw IPCException(reason);
	}

	LOG_DBG("Flow deallocated successfully! Port-id: %d", portId);
	delete deallocateResponse;

#endif

	allocatedFlows.erase(portId);
	delete flow;
}

std::vector<Flow *> IPCManager::getAllocatedFlows() {
	LOG_DBG("IPCManager.getAllocatedFlows called");
	std::vector<Flow *> response;

	for (std::map<int, Flow*>::iterator it = allocatedFlows.begin();
			it != allocatedFlows.end(); ++it) {
		response.push_back(it->second);
	}

	return response;
}

std::vector<ApplicationRegistration *> IPCManager::getRegisteredApplications() {
	LOG_DBG("IPCManager.getRegisteredApplications called");
	std::vector<ApplicationRegistration *> response;

	for (std::map<ApplicationProcessNamingInformation,
			ApplicationRegistration*>::iterator it = applicationRegistrations
			.begin(); it != applicationRegistrations.end(); ++it) {
		response.push_back(it->second);
	}

	return response;
}

Singleton<IPCManager> ipcManager;

/* CLASS FLOW DEALLOCATED EVENT */

FlowDeallocatedEvent::FlowDeallocatedEvent(
		int portId, int code, const std::string& reason,
		const ApplicationProcessNamingInformation& difName) :
				IPCEvent(FLOW_DEALLOCATED_EVENT, 0) {
	this->portId = portId;
	this->code = code;
	this->reason = reason;
	this->difName = difName;
}

int FlowDeallocatedEvent::getPortId() const {
	return portId;
}

int FlowDeallocatedEvent::getCode() const{
	return code;
}

const std::string FlowDeallocatedEvent::getReason() const{
	return reason;
}

const ApplicationProcessNamingInformation
FlowDeallocatedEvent::getDIFName() const{
	return difName;
}


/* CLASS APPLICATION UNREGISTERED EVENT */

ApplicationUnregisteredEvent::ApplicationUnregisteredEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber) :
				IPCEvent(APPLICATION_UNREGISTERED_EVENT,
						sequenceNumber) {
	this->applicationName = appName;
	this->DIFName = DIFName;
}

const ApplicationProcessNamingInformation&
ApplicationUnregisteredEvent::getApplicationName() const {
	return applicationName;
}

const ApplicationProcessNamingInformation&
ApplicationUnregisteredEvent::getDIFName() const {
	return DIFName;
}

/* CLASS APPLICATION REGISTRATION CANCELED EVENT*/
AppRegistrationCanceledEvent::AppRegistrationCanceledEvent(int code,
		const std::string& reason,
		const ApplicationProcessNamingInformation& difName,
		unsigned int sequenceNumber):
					IPCEvent(
							APPLICATION_REGISTRATION_CANCELED_EVENT, sequenceNumber){
	this->code = code;
	this->reason = reason;
	this->difName = difName;
}

int AppRegistrationCanceledEvent::getCode() const{
	return code;
}
const std::string AppRegistrationCanceledEvent::getReason() const{
	return reason;
}
const ApplicationProcessNamingInformation&
AppRegistrationCanceledEvent::getApplicationName() const{
	return applicationName;
}
const ApplicationProcessNamingInformation
AppRegistrationCanceledEvent::getDIFName() const{
	return difName;
}

}
