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
#include "netlink-manager.h"

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

	unsigned char buffer[] = { 0, 23, 43, 32, 45, 23, 78 };
	sdu = buffer;
	return 7;
}

void Flow::writeSDU(unsigned char * sdu, int size) throw (IPCException) {
	LOG_DBG("Flow.writeSDU called");

	if (flowState != FLOW_ALLOCATED) {
		throw IPCException(Flow::flow_not_allocated_error);
	}

	int i;
	for (i = 0; i < size; i++) {
		LOG_DBG("SDU[%d] = %d", i, sdu[i]);
	}
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
	//TODO Probably clear all Netlink related machineries?
}

const std::string IPCManager::application_registered_error =
		"The application is already registered in this DIF";
const std::string IPCManager::application_not_registered_error =
		"The application is not registered in this DIF";
const std::string IPCManager::unknown_flow_error =
		"There is no flow at the specified portId";

std::vector<DIFProperties> IPCManager::getDIFProperties(
		const ApplicationProcessNamingInformation& DIFName) {
	LOG_DBG("IPCManager.getDIFProperties called");
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

void IPCManager::registerApplication(
		const ApplicationProcessNamingInformation& applicationName,
		const ApplicationProcessNamingInformation& DIFName)
				throw (IPCException) {
	LOG_DBG("IPCManager.registerApplication called");
	ApplicationRegistration * applicationRegistration;

	try {
		ApplicationRegistration * foundRegistration = applicationRegistrations
				.at(applicationName);
		std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
		for (iterator = foundRegistration->getDIFNames().begin();
				iterator != foundRegistration->getDIFNames().end();
				++iterator) {
			if (*iterator == DIFName) {
				throw IPCException(IPCManager::application_registered_error);
			}
		}

		foundRegistration->addDIFName(DIFName);
		return;
	} catch (std::out_of_range& oor) {
	}

	//The application was not registered, create a new ApplicationRegistration object and add it to the map
	applicationRegistration = new ApplicationRegistration(applicationName);
	applicationRegistrations[applicationName] = applicationRegistration;
	applicationRegistration->addDIFName(DIFName);
}

void IPCManager::unregisterApplication(
		ApplicationProcessNamingInformation applicationName,
		ApplicationProcessNamingInformation DIFName) throw (IPCException) {
	LOG_DBG("IPCManager.unregisterApplication called");

	try {
		ApplicationRegistration * foundRegistration = applicationRegistrations
				.at(applicationName);
		std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
		for (iterator = foundRegistration->getDIFNames().begin();
				iterator != foundRegistration->getDIFNames().end();
				++iterator) {
			if (*iterator == DIFName) {
				foundRegistration->removeDIFName(DIFName);
				if (foundRegistration->getDIFNames().size() == 0) {
					applicationRegistrations.erase(applicationName);
				}

				return;
			}
		}

		throw IPCException(IPCManager::application_not_registered_error);
	} catch (std::out_of_range& oor) {
		throw IPCException(IPCManager::application_not_registered_error);
	}
}

Flow * IPCManager::allocateFlowRequest(
		const ApplicationProcessNamingInformation& sourceAppName,
		const ApplicationProcessNamingInformation& destAppName,
		const FlowSpecification& flowSpec) throw (IPCException) {
	LOG_DBG("IPCManager.allocateFlowRequest called");

	int portId;
	int i;
	for (i = 1; i < 1000; i++) {
		if (allocatedFlows.find(i) == allocatedFlows.end()) {
			portId = i;
			break;
		}
	}

	ApplicationProcessNamingInformation * DIFName =
			new ApplicationProcessNamingInformation("test.DIF", "");
	Flow * flow = new Flow(sourceAppName, destAppName, flowSpec,
			FLOW_ALLOCATED, *DIFName, portId);
	allocatedFlows[portId] = flow;

	return flow;
}

Flow * IPCManager::allocateFlowResponse(int portId, bool accept,
		const std::string& reason) throw (IPCException) {
	LOG_DBG("IPCManager.allocateFlowResponse called");

	if (!accept) {
		LOG_DBG("Flow was not accepted because: %s", reason.c_str());
		return NULL;
	}

	ApplicationProcessNamingInformation * sourceAppName =
			new ApplicationProcessNamingInformation("/test/app/source", "1");
	ApplicationProcessNamingInformation * destAppName =
			new ApplicationProcessNamingInformation("/test/app/dest", "1");
	ApplicationProcessNamingInformation * DIFName =
			new ApplicationProcessNamingInformation("test.DIF", "");
	FlowSpecification * flowSpec = new FlowSpecification();
	Flow * flow = new Flow(*sourceAppName, *destAppName, *flowSpec,
			FLOW_ALLOCATED, *DIFName, portId);
	allocatedFlows[portId] = flow;
	return flow;
}

void IPCManager::deallocateFlow(int portId) throw (IPCException) {
	LOG_DBG("IPCManager.deallocateFlow called");

	std::map<int, Flow*>::iterator iterator;
	iterator = allocatedFlows.find(portId);
	if (iterator == allocatedFlows.end()) {
		throw IPCException(IPCManager::unknown_flow_error);
	}

	delete iterator->second;
	allocatedFlows.erase(portId);
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
		int portId, unsigned int sequenceNumber) :
		IPCEvent(FLOW_DEALLOCATED_EVENT, sequenceNumber) {
	this->portId = portId;
}

int FlowDeallocatedEvent::getPortId() const {
	return portId;
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

/* CLASS INCOMING FLOW REQUEST EVENT */
IncomingFlowRequestEvent::IncomingFlowRequestEvent(
		const FlowSpecification& flowSpecification,
		const ApplicationProcessNamingInformation& sourceApplicationName,
		const ApplicationProcessNamingInformation& destApplicationName,
		unsigned int sequenceNumber):
				IPCEvent(FLOW_ALLOCATION_REQUESTED_EVENT,
						sequenceNumber) {
	this->flowSpecification = flowSpecification;
	this->sourceApplicationName = sourceApplicationName;
	this->destinationApplicationName = destApplicationName;
	this->portId = 0;
}

IncomingFlowRequestEvent::IncomingFlowRequestEvent(int portId,
		const FlowSpecification& flowSpecification,
		const ApplicationProcessNamingInformation& sourceApplicationName,
		const ApplicationProcessNamingInformation& destApplicationName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber) :
		IPCEvent(FLOW_ALLOCATION_REQUESTED_EVENT,
				sequenceNumber) {
	this->flowSpecification = flowSpecification;
	this->sourceApplicationName = sourceApplicationName;
	this->destinationApplicationName = destApplicationName;
	this->DIFName = DIFName;
	this->portId = portId;
}

int IncomingFlowRequestEvent::getPortId() const {
	return portId;
}

const FlowSpecification& IncomingFlowRequestEvent::getFlowSpecification() const {
	return flowSpecification;
}

const ApplicationProcessNamingInformation& IncomingFlowRequestEvent::getDIFName() const {
	return DIFName;
}

const ApplicationProcessNamingInformation& IncomingFlowRequestEvent::getSourceApplicationName() const {
	return sourceApplicationName;
}

const ApplicationProcessNamingInformation& IncomingFlowRequestEvent::getDestApplicationName() const {
	return destinationApplicationName;
}

}
