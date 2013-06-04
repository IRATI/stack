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

#include "librina-application.h"

#define RINA_PREFIX "application"

#include "logs.h"
#include <stdexcept>

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
	return this->flowState;
}

int Flow::getPortId() const {
	return this->portId;
}

const ApplicationProcessNamingInformation& Flow::getDIFName() const {
	return this->DIFName;
}

const ApplicationProcessNamingInformation& Flow::getSourceApplicationName() const {
	return this->sourceApplicationName;
}

const ApplicationProcessNamingInformation& Flow::getDestinationApplcationName() const {
	return this->destinationApplicationName;
}

const FlowSpecification Flow::getFlowSpecification() const {
	return this->flowSpecification;
}

int Flow::readSDU(unsigned char * sdu) throw (IPCException) {
	LOG_DBG("Flow.readSDU called");

	if (this->flowState != FlowState::FLOW_ALLOCATED) {
		throw IPCException(Flow::flow_not_allocated_error);
	}

	unsigned char buffer[] = { 0, 23, 43, 32, 45, 23, 78 };
	*sdu = buffer;
	return 7;
}

void Flow::writeSDU(unsigned char * sdu, int size) throw (IPCException) {
	LOG_DBG("Flow.writeSDU called");

	if (this->flowState != FlowState::FLOW_ALLOCATED) {
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
	return this->applicationName;
}

const std::list<ApplicationProcessNamingInformation>& ApplicationRegistration::getDIFNames() const {
	return this->DIFNames;
}

void ApplicationRegistration::addDIFName(
		const ApplicationProcessNamingInformation& DIFName) {
	this->DIFNames.push_back(DIFName);
}

void ApplicationRegistration::removeDIFName(
		const ApplicationProcessNamingInformation& DIFName) {
	this->DIFNames.remove(DIFName);
}

/* CLASS IPC MANAGER */

bool IPCManager::instanceFlag = false;

IPCManager* IPCManager::instance = NULL;

IPCManager* IPCManager::getInstance() {
	if (!instanceFlag) {
		instance = new IPCManager();
		instanceFlag = true;
		return instance;
	} else {
		return instance;
	}
}

const std::string IPCManager::application_registered_error =
		"The application is already registered in this DIF";
const std::string IPCManager::application_registered_error =
		"The application is not registered in this DIF";
const std::string IPCManager::unknown_flow_error =
		"There is no flow at the specified portId";

std::vector<DIFProperties> IPCManager::getDIFProperties(
		const ApplicationProcessNamingInformation& DIFName) {
	LOG_DBG("IPCManager.getDIFProperties called");
	std::vector<DIFProperties> result;
	DIFProperties DIFProperties;
	ApplicationProcessNamingInformation name;

	if (DIFName != NULL && DIFName.getProcessName().compare("") != 0) {
		DIFProperties = new DIFProperties(DIFName, 2000);
		result.push_back(DIFProperties);
		return result;
	}

	name = new ApplicationProcessNamingInformation("test.DIF", "");
	DIFProperties = new DIFProperties(name, 5000);
	result.push_back(DIFProperties);
	name = new ApplicationProcessNamingInformation("Public-Internet.DIF", "");
	DIFProperties = new DIFProperties(name, 10000);
	result.push_back(DIFProperties);
	return result;
}

void IPCManager::registerApplication(
		const ApplicationProcessNamingInformation& applicationName,
		const ApplicationProcessNamingInformation& DIFName) throw (IPCException) {
	LOG_DBG("IPCManager.registerApplication called");
	ApplicationRegistration applicationRegistration;
	ApplicationProcessNamingInformation name;
	bool wasRegistered = false;

	try {
		applicationRegistration = this->applicationRegistrations.at(
				applicationName);
		wasRegistered = true;
	} catch (std::out_of_range& oor) {
	}

	if (wasRegistered) {
		std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
		for (iterator = applicationRegistration.DIFNames.begin();
				iterator != applicationRegistration.DIFNames.end();
				++iterator) {
			name = *iterator;
			if (name == DIFName) {
				throw IPCException(IPCManager::application_registered_error);
			}
		}
	} else {
		//The application was not registered, create a new ApplicationRegistration object and add it to the map
		applicationRegistration = new ApplicationRegistration(applicationName);
		this->applicationRegistrations[applicationName] = applicationRegistration;
	}

	applicationRegistration.addDIFName(DIFName);
}

void IPCManager::unregisterApplication(
		ApplicationProcessNamingInformation applicationName,
		ApplicationProcessNamingInformation DIFName) throw (IPCException) {
	LOG_DBG("IPCManager.unregisterApplication called");

	ApplicationRegistration applicationRegistration;
	ApplicationProcessNamingInformation name;
	bool wasRegistered = false;

	try {
		applicationRegistration = this->applicationRegistrations.at(
				applicationName);
	} catch (std::out_of_range& oor) {
		throw IPCException(IPCManager::application_not_registered_error);
	}

	std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
	for (iterator = applicationRegistration.DIFNames.begin();
			iterator != applicationRegistration.DIFNames.end();
			++iterator) {
		name = *iterator;
		if (name == DIFName) {
			wasRegistered = true;
			break;
		}
	}

	if (wasRegistered){
		applicationRegistration.removeDIFName(DIFName);
		if (applicationRegistration.getDIFNames().size() == 0){
			this->applicationRegistrations.erase(applicationName);
		}
	}else{
		throw IPCException(IPCManager::application_not_registered_error);
	}
}

const Flow& IPCManager::allocateFlowRequest(const ApplicationProcessNamingInformation& sourceAppName,
			const ApplicationProcessNamingInformation& destAppName,
			const FlowSpecification& flowSpec) const throw(IPCException){
	LOG_DBG("IPCManager.allocateFlowRequest called");

	int portId;
	int i;
	for(i=0; i<1000; i++){
		if (this->allocatedFlows.find(i) == this->allocatedFlows.end()){
			portId = i;
			break;
		}
	}

	ApplicationProcessNamingInformation DIFName = new ApplicationProcessNamingInformation("test.DIF", "");
	Flow flow = new Flow(sourceAppName, destAppName, flowSpec, DIFName, FlowState::FLOW_ALLOCATED, portId);
	this->allocatedFlows[portId] = flow;

	return flow;
}

const Flow& IPCManager::allocateFlowResponse(int portId, bool accept,
			const std::string& reason) const throw(IPCException){
	LOG_DBG("IPCManager.allocateFlowResponse called");

	if (!accept){
		return NULL;
	}

	ApplicationProcessNamingInformation sourceAppName = new ApplicationProcessNamingInformation("/test/app/source", "1");
	ApplicationProcessNamingInformation destAppName = new ApplicationProcessNamingInformation("/test/app/dest", "1");
	ApplicationProcessNamingInformation DIFName = new ApplicationProcessNamingInformation("test.DIF", "");
	FlowSpecification flowSpec = new FlowSpecification();
	Flow flow = new Flow(sourceAppName, destAppName, flowSpec, DIFName, FlowState::FLOW_ALLOCATED, portId);
	this->allocatedFlows[portId] = flow;
	return flow;
}

void IPCManager::deallocateFlow(const Flow& flow) throw(IPCException){
	LOG_DBG("IPCManager.deallocateFlow called");

	std::map<int,Flow>::iterator iterator;
	iterator = this->allocatedFlows.find(flow.getPortId());
	if (iterator == this->allocatedFlows.end()){
		throw new IPCException(IPCManager::unknown_flow_error);
	}

	flow.flowState = FlowState::FLOW_DEALLOCATED;
	this->allocatedFlows.erase(flow.getPortId());
}
