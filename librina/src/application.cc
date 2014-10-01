//
// Application
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <cstring>
#include <stdexcept>

#define RINA_PREFIX "application"

#include "librina/logs.h"
#include "core.h"
#include "rina-syscalls.h"
#include "librina/application.h"

namespace rina {

/* CLASS FLOW */
Flow::Flow(const ApplicationProcessNamingInformation& localApplicationName,
		const ApplicationProcessNamingInformation& remoteApplicationName,
		const FlowSpecification& flowSpecification, FlowState flowState){
	flowInformation.localAppName = localApplicationName;
	flowInformation.remoteAppName = remoteApplicationName;
	flowInformation.flowSpecification = flowSpecification;
	flowInformation.portId = 0;
	this->flowState = flowState;
}

Flow::Flow(const ApplicationProcessNamingInformation& localApplicationName,
		const ApplicationProcessNamingInformation& remoteApplicationName,
		const FlowSpecification& flowSpecification, FlowState flowState,
		const ApplicationProcessNamingInformation& DIFName, int portId) {
	flowInformation.localAppName = localApplicationName;
	flowInformation.remoteAppName = remoteApplicationName;
	flowInformation.flowSpecification = flowSpecification;
	flowInformation.portId = portId;
	flowInformation.difName = DIFName;
	this->flowState = flowState;
}

void Flow::setPortId(int portId){
	flowInformation.portId = portId;
}

void Flow::setDIFName(const ApplicationProcessNamingInformation& DIFName) {
	flowInformation.difName = DIFName;
}

void Flow::setState(FlowState flowState) {
	this->flowState = flowState;
}

const FlowState& Flow::getState() const {
	return flowState;
}

int Flow::getPortId() const {
	return flowInformation.portId;
}

const ApplicationProcessNamingInformation&
Flow::getDIFName() const {
	return flowInformation.difName;
}

const ApplicationProcessNamingInformation&
Flow::getLocalApplicationName() const {
	return flowInformation.localAppName;
}

const ApplicationProcessNamingInformation&
Flow::getRemoteApplcationName() const {
	return flowInformation.remoteAppName;
}

const FlowSpecification& Flow::getFlowSpecification() const {
	return flowInformation.flowSpecification;
}

bool Flow::isAllocated() const{
	return flowState == FLOW_ALLOCATED;
}

const FlowInformation& Flow::getFlowInformation() const {
   return flowInformation;
}

int Flow::readSDU(void * sdu, int maxBytes) throw(Exception){
	if (flowState != FLOW_ALLOCATED) {
		throw FlowNotAllocatedException();
	}

#if STUB_API
        memset(sdu, 'v', maxBytes);

	return maxBytes;
#else
	int result = syscallReadSDU(flowInformation.portId, sdu, maxBytes);
	if (result < 0){
		throw ReadSDUException();
	}

	return result;
#endif
}

void Flow::writeSDU(void * sdu, int size) throw(Exception) {
	if (flowState != FLOW_ALLOCATED) {
		throw FlowNotAllocatedException();
	}

#if STUB_API
	/* Do nothing. */
        (void)sdu;
        (void)size;
#else
	int result = syscallWriteSDU(flowInformation.portId, sdu, size);
	if (result < 0){
		throw WriteSDUException();
	}
#endif
}

/* CLASS APPLICATION REGISTRATION */
ApplicationRegistration::ApplicationRegistration(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
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
IPCManager::IPCManager() : Lockable() {
}

IPCManager::~IPCManager() throw(){
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
const std::string IPCManager::error_getting_dif_properties =
		"Error getting DIF properties";
const std::string IPCManager::wrong_flow_state  =
                "Wrong flow state";

Flow * IPCManager::getPendingFlow(unsigned int seqNumber) {
        std::map<unsigned int, Flow*>::iterator iterator;

        iterator = pendingFlows.find(seqNumber);
        if (iterator == pendingFlows.end()) {
                return 0;
        }

        return iterator->second;
}

Flow * IPCManager::getAllocatedFlow(int portId) {
        std::map<int, Flow*>::iterator iterator;

        iterator = allocatedFlows.find(portId);
        if (iterator == allocatedFlows.end()) {
                return 0;
        }

        return iterator->second;
}

Flow * IPCManager::getFlowToRemoteApp(
                ApplicationProcessNamingInformation remoteAppName) {
        std::map<int, Flow*>::iterator iterator;

        for(iterator = allocatedFlows.begin(); iterator != allocatedFlows.end();
                        ++iterator) {
                if (iterator->second->getRemoteApplcationName() ==
                                remoteAppName) {
                        return iterator->second;
                }
        }

        return NULL;
}

ApplicationRegistrationInformation IPCManager::getRegistrationInfo(
                        unsigned int seqNumber) {
        std::map<unsigned int, ApplicationRegistrationInformation>::iterator iterator;

        iterator = registrationInformation.find(seqNumber);
        if (iterator == registrationInformation.end()) {
                throw IPCException("Registration not found");
        }

        return iterator->second;
}

ApplicationRegistration * IPCManager::getApplicationRegistration(
                const ApplicationProcessNamingInformation& appName) {
        std::map<ApplicationProcessNamingInformation,
        ApplicationRegistration*>::iterator iterator =
                        applicationRegistrations.find(appName);

        if (iterator == applicationRegistrations.end()){
                return 0;
        }

        return iterator->second;
}

void IPCManager::putApplicationRegistration(
                        const ApplicationProcessNamingInformation& key,
                        ApplicationRegistration * value) {
        applicationRegistrations[key] = value;
}

void IPCManager::removeApplicationRegistration(
                        const ApplicationProcessNamingInformation& key) {
        applicationRegistrations.erase(key);
}

unsigned int IPCManager::internalRequestFlowAllocation(
                const ApplicationProcessNamingInformation& localAppName,
                const ApplicationProcessNamingInformation& remoteAppName,
                const FlowSpecification& flowSpec,
                unsigned short sourceIPCProcessId) {
        Flow * flow;

#if STUB_API
        flow = new Flow(localAppName, remoteAppName, flowSpec, FLOW_ALLOCATED);
        pendingFlows[0] = flow;
        (void)sourceIPCProcessId;
        return 0;
#else
        AppAllocateFlowRequestMessage message;
        message.setSourceAppName(localAppName);
        message.setDestAppName(remoteAppName);
        message.setSourceIpcProcessId(sourceIPCProcessId);
        message.setFlowSpecification(flowSpec);
        message.setRequestMessage(true);

        try{
                rinaManager->sendMessage(&message);
        }catch(NetlinkException &e){
                throw FlowAllocationException(e.what());
        }

        flow = new Flow(localAppName, remoteAppName, flowSpec, FLOW_ALLOCATED);
        lock();
        pendingFlows[message.getSequenceNumber()] = flow;
        unlock();

        return message.getSequenceNumber();
#endif
}

unsigned int IPCManager::internalRequestFlowAllocationInDIF(
                const ApplicationProcessNamingInformation& localAppName,
                const ApplicationProcessNamingInformation& remoteAppName,
                const ApplicationProcessNamingInformation& difName,
                unsigned short sourceIPCProcessId,
                const FlowSpecification& flowSpec) {
        Flow * flow;

#if STUB_API
        flow = new Flow(localAppName, remoteAppName, flowSpec, FLOW_ALLOCATED);
        pendingFlows[0] = flow;
        (void)difName;
        (void)sourceIPCProcessId;
        return 0;
#else
        AppAllocateFlowRequestMessage message;
        message.setSourceAppName(localAppName);
        message.setDestAppName(remoteAppName);
        message.setSourceIpcProcessId(sourceIPCProcessId);
        message.setFlowSpecification(flowSpec);
        message.setDifName(difName);
        message.setRequestMessage(true);

        try{
                rinaManager->sendMessage(&message);
        }catch(NetlinkException &e){
                throw FlowAllocationException(e.what());
        }

        flow = new Flow(localAppName, remoteAppName, flowSpec, FLOW_ALLOCATED);
        lock();
        pendingFlows[message.getSequenceNumber()] = flow;
        unlock();

        return message.getSequenceNumber();
#endif
}

Flow * IPCManager::internalAllocateFlowResponse(
                const FlowRequestEvent& flowRequestEvent,
                int result, bool notifySource, unsigned short ipcProcessId) {
#if STUB_API
        //Do nothing
        (void)notifySource;
        (void)ipcProcessId;
#else
        AppAllocateFlowResponseMessage responseMessage;
        responseMessage.setResult(result);
        responseMessage.setNotifySource(notifySource);
        responseMessage.setSourceIpcProcessId(ipcProcessId);
        responseMessage.setSequenceNumber(flowRequestEvent.sequenceNumber);
        responseMessage.setResponseMessage(true);
        try{
                rinaManager->sendMessage(&responseMessage);
        }catch(NetlinkException &e){
                throw FlowAllocationException(e.what());
        }
#endif
        if (result != 0) {
                LOG_WARN("Flow was not accepted, error code: %d", result);
                return 0;
        }

        Flow * flow = new Flow(flowRequestEvent.localApplicationName,
                        flowRequestEvent.remoteApplicationName,
                        flowRequestEvent.flowSpecification, FLOW_ALLOCATED,
                        flowRequestEvent.DIFName, flowRequestEvent.portId);
        lock();
        allocatedFlows[flowRequestEvent.portId] = flow;
        unlock();
        return flow;
}

unsigned int IPCManager::getDIFProperties(
		const ApplicationProcessNamingInformation& applicationName,
		const ApplicationProcessNamingInformation& DIFName) {

#if STUB_API
        (void)applicationName;
        (void)DIFName;
	return 0;
#else
	AppGetDIFPropertiesRequestMessage message;
	message.setApplicationName(applicationName);
	message.setDifName(DIFName);
	message.setRequestMessage(true);

	try{
		rinaManager->sendMessage(&message);
	}catch(NetlinkException &e){
		throw GetDIFPropertiesException(e.what());
	}

	return message.getSequenceNumber();
#endif
}

unsigned int IPCManager::requestApplicationRegistration(
                const ApplicationRegistrationInformation& appRegistrationInfo) {
#if STUB_API
        registrationInformation[0] = appRegistrationInfo;
	return 0;
#else
	AppRegisterApplicationRequestMessage message;
	message.setApplicationRegistrationInformation(appRegistrationInfo);
	message.setRequestMessage(true);

	try{
	        rinaManager->sendMessage(&message);
	}catch(NetlinkException &e){
	        throw ApplicationRegistrationException(e.what());
	}

	lock();
	registrationInformation[message.getSequenceNumber()] = appRegistrationInfo;
	unlock();

	return message.getSequenceNumber();
#endif
}

ApplicationRegistration * IPCManager::commitPendingRegistration(
                        unsigned int seqNumber,
                        const ApplicationProcessNamingInformation& DIFName) {
        ApplicationRegistrationInformation appRegInfo;
        ApplicationRegistration * applicationRegistration;

        lock();
        try {
        	appRegInfo = getRegistrationInfo(seqNumber);
        } catch(IPCException &e){
                unlock();
                throw ApplicationRegistrationException("Unknown registration");
        }

        registrationInformation.erase(seqNumber);

        applicationRegistration = getApplicationRegistration(
                        appRegInfo.appName);

        if (!applicationRegistration){
                applicationRegistration = new ApplicationRegistration(
                                appRegInfo.appName);
                applicationRegistrations[appRegInfo.appName] =
                                applicationRegistration;
        }

        applicationRegistration->addDIFName(DIFName);
        unlock();

        return applicationRegistration;
}

void IPCManager::withdrawPendingRegistration(unsigned int seqNumber) {
        ApplicationRegistrationInformation appRegInfo;

        lock();
        try {
        	appRegInfo = getRegistrationInfo(seqNumber);
        } catch(IPCException &e){
        	unlock();
        	throw ApplicationRegistrationException("Unknown registration");
        }

        unlock();
        registrationInformation.erase(seqNumber);
}

unsigned int IPCManager::requestApplicationUnregistration(
		const ApplicationProcessNamingInformation& applicationName,
		const ApplicationProcessNamingInformation& DIFName) {
        ApplicationRegistration * applicationRegistration;
        bool found = false;

        lock();
        applicationRegistration = getApplicationRegistration(applicationName);
        if (!applicationRegistration){
                unlock();
                throw ApplicationUnregistrationException(
                                IPCManager::application_not_registered_error);
        }

        std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
        for (iterator = applicationRegistration->DIFNames.begin();
                        iterator != applicationRegistration->DIFNames.end();
                        ++iterator) {
                if (*iterator == DIFName) {
                        found = true;
                }
        }

        if (!found) {
                unlock();
                throw ApplicationUnregistrationException(
                                IPCManager::application_not_registered_error);
        }

        ApplicationRegistrationInformation appRegInfo;
        appRegInfo.appName = applicationName;
        appRegInfo.difName = DIFName;

#if STUB_API
        registrationInformation[0] = appRegInfo;
        unlock();
	return 0;
#else
	AppUnregisterApplicationRequestMessage message;
	message.setApplicationName(applicationName);
	message.setDifName(DIFName);
	message.setRequestMessage(true);

	try{
	        rinaManager->sendMessage(&message);
	}catch(NetlinkException &e){
	        unlock();
	        throw ApplicationUnregistrationException(e.what());
	}

	registrationInformation[message.getSequenceNumber()] =
	                appRegInfo;
	unlock();
	return message.getSequenceNumber();
#endif

}

void IPCManager::appUnregistrationResult(unsigned int seqNumber, bool success) {
        ApplicationRegistrationInformation appRegInfo;

        lock();

        try {
                appRegInfo = getRegistrationInfo(seqNumber);
        } catch (IPCException &e){
                unlock();
                throw ApplicationUnregistrationException(
                                "Pending unregistration not found");
        }

       registrationInformation.erase(seqNumber);
       ApplicationRegistration * applicationRegistration;

       applicationRegistration = getApplicationRegistration(
                       appRegInfo.appName);
       if (!applicationRegistration){
               unlock();
               throw ApplicationUnregistrationException(
                       IPCManager::application_not_registered_error);
       }

       if (!success) {
    	   return;
       }

       std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
       for (iterator = applicationRegistration->DIFNames.begin();
                       iterator != applicationRegistration->DIFNames.end();
                       ++iterator) {
               if (*iterator == appRegInfo.difName) {
                       applicationRegistration->removeDIFName(
                                       appRegInfo.difName);
                       if (applicationRegistration->DIFNames.size() == 0) {
                               applicationRegistrations.erase(
                                       appRegInfo.appName);
                       }

                       break;
               }
       }

       unlock();
}

unsigned int IPCManager::requestFlowAllocation(
		const ApplicationProcessNamingInformation& localAppName,
		const ApplicationProcessNamingInformation& remoteAppName,
		const FlowSpecification& flowSpec) {
        return internalRequestFlowAllocation(
                        localAppName, remoteAppName, flowSpec, 0);
}

unsigned int IPCManager::requestFlowAllocationInDIF(
                const ApplicationProcessNamingInformation& localAppName,
                const ApplicationProcessNamingInformation& remoteAppName,
                const ApplicationProcessNamingInformation& difName,
                const FlowSpecification& flowSpec) {
        return internalRequestFlowAllocationInDIF(localAppName,
                        remoteAppName, difName, 0, flowSpec);
}

Flow * IPCManager::commitPendingFlow(unsigned int sequenceNumber, int portId,
                        const ApplicationProcessNamingInformation& DIFName) {
        Flow * flow;

        lock();
        flow = getPendingFlow(sequenceNumber);
        if (flow == 0) {
                unlock();
                throw FlowDeallocationException(IPCManager::unknown_flow_error);
        }

        pendingFlows.erase(sequenceNumber);

        flow->setPortId(portId);
        flow->setDIFName(DIFName);
        allocatedFlows[portId] = flow;
        unlock();

        return flow;
}

FlowInformation IPCManager::withdrawPendingFlow(unsigned int sequenceNumber) {
        std::map<int, Flow*>::iterator iterator;
        Flow * flow;
        FlowInformation flowInformation;

        lock();
        flow = getPendingFlow(sequenceNumber);
        if (flow == 0) {
                unlock();
                throw FlowDeallocationException(IPCManager::unknown_flow_error);
        }

        pendingFlows.erase(sequenceNumber);
        flowInformation = flow->getFlowInformation();
        delete flow;
        unlock();

        return flowInformation;
}

Flow * IPCManager::allocateFlowResponse(
		const FlowRequestEvent& flowRequestEvent, int result,
		bool notifySource) {
        return internalAllocateFlowResponse(
                        flowRequestEvent, result, notifySource, 0);
}

unsigned int IPCManager::requestFlowDeallocation(int portId) {
        Flow * flow;

        lock();
        flow = getAllocatedFlow(portId);
        if (flow == 0) {
                unlock();
                throw FlowDeallocationException(
                                IPCManager::unknown_flow_error);
        }

        if (flow->getState() != FLOW_ALLOCATED) {
                unlock();
                throw FlowDeallocationException(
                                IPCManager::wrong_flow_state);
        }

#if STUB_API
        flow->setState(FLOW_DEALLOCATION_REQUESTED);
        unlock();
	return 0;
#else

	LOG_DBG("Application %s requested to deallocate flow with port-id %d",
		flow->getLocalApplicationName().processName.c_str(),
		flow->getPortId());
	AppDeallocateFlowRequestMessage message;
	message.setApplicationName(flow->getLocalApplicationName());
	message.setPortId(flow->getPortId());
	message.setRequestMessage(true);

	try{
	        rinaManager->sendMessage(&message);
	}catch(NetlinkException &e){
	        unlock();
	        throw FlowDeallocationException(e.what());
	}

	flow->setState(FLOW_DEALLOCATION_REQUESTED);
	unlock();

	return message.getSequenceNumber();
#endif
}

void IPCManager::flowDeallocationResult(int portId, bool success) {
        Flow * flow;

        lock();

        flow = getAllocatedFlow(portId);
        if (flow == 0) {
                unlock();
                throw FlowDeallocationException(
                                IPCManager::unknown_flow_error);
        }

        if (flow->getState() != FLOW_DEALLOCATION_REQUESTED) {
                unlock();
                throw FlowDeallocationException(
                                IPCManager::wrong_flow_state);
        }

        if (success) {
                allocatedFlows.erase(portId);
                delete flow;
        } else {
                flow->setState(FLOW_ALLOCATED);
        }

        unlock();
}

void IPCManager::flowDeallocated(int portId) {
	lock();

	Flow * flow = getAllocatedFlow(portId);

	if (flow == 0) {
		unlock();
		throw FlowDeallocationException("Unknown flow");
	}

	allocatedFlows.erase(portId);
	delete flow;

	unlock();
}

std::vector<Flow *> IPCManager::getAllocatedFlows() {
	std::vector<Flow *> response;

	lock();
	for (std::map<int, Flow*>::iterator it = allocatedFlows.begin();
			it != allocatedFlows.end(); ++it) {
		response.push_back(it->second);
	}
	unlock();

	return response;
}

std::vector<ApplicationRegistration *> IPCManager::getRegisteredApplications() {
	LOG_DBG("IPCManager.getRegisteredApplications called");
	std::vector<ApplicationRegistration *> response;

	lock();
	for (std::map<ApplicationProcessNamingInformation,
			ApplicationRegistration*>::iterator it = applicationRegistrations
			.begin(); it != applicationRegistrations.end(); ++it) {
		response.push_back(it->second);
	}
	unlock();

	return response;
}

Singleton<IPCManager> ipcManager;

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

/* CLASS APPLICATION REGISTRATION CANCELED EVENT*/
AppRegistrationCanceledEvent::AppRegistrationCanceledEvent(int code,
                const std::string& reason,
                const ApplicationProcessNamingInformation& difName,
                unsigned int sequenceNumber):
			IPCEvent(
				APPLICATION_REGISTRATION_CANCELED_EVENT,
				sequenceNumber){
        this->code = code;
        this->reason = reason;
        this->difName = difName;
}

/* CLASS AllocateFlowRequestResultEvent EVENT*/
AllocateFlowRequestResultEvent::AllocateFlowRequestResultEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& difName,
                        int portId,
                        unsigned int sequenceNumber):
                                IPCEvent(
                                         ALLOCATE_FLOW_REQUEST_RESULT_EVENT,
                                         sequenceNumber){
        this->sourceAppName = appName;
        this->difName = difName;
        this->portId = portId;
}

/* CLASS Deallocate Flow Response EVENT*/
DeallocateFlowResponseEvent::DeallocateFlowResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        int portId, int result,
                        unsigned int sequenceNumber):
                                BaseResponseEvent(result,
                                         DEALLOCATE_FLOW_RESPONSE_EVENT,
                                         sequenceNumber){
        this->appName = appName;
        this->portId = portId;
}

/* CLASS Get DIF Properties Response EVENT*/
GetDIFPropertiesResponseEvent::GetDIFPropertiesResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const std::list<DIFProperties>& difProperties,
                        int result,
                        unsigned int sequenceNumber):
                                BaseResponseEvent(result,
                                         GET_DIF_PROPERTIES_RESPONSE_EVENT,
                                         sequenceNumber){
        this->applicationName = appName;
        this->difProperties = difProperties;
}

}
