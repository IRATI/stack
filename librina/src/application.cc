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
        this->localApplicationName = localApplicationName;
        this->remoteApplicationName = remoteApplicationName;
        this->flowSpecification = flowSpecification;
        this->flowState = flowState;
        this->portId = 0;
}

Flow::Flow(const ApplicationProcessNamingInformation& localApplicationName,
           const ApplicationProcessNamingInformation& remoteApplicationName,
	   const FlowSpecification& flowSpecification, FlowState flowState,
	   const ApplicationProcessNamingInformation& DIFName, int portId) {
	this->localApplicationName = localApplicationName;
	this->remoteApplicationName = remoteApplicationName;
	this->flowSpecification = flowSpecification;
	this->DIFName = DIFName;
	this->flowState = flowState;
	this->portId = portId;
}

void Flow::setPortId(int portId){
        this->portId = portId;
}

void Flow::setDIFName(const ApplicationProcessNamingInformation& DIFName) {
        this->DIFName = DIFName;
}

void Flow::setState(FlowState flowState) {
        this->flowState = flowState;
}

const FlowState& Flow::getState() const {
	return flowState;
}

int Flow::getPortId() const {
	return portId;
}

const ApplicationProcessNamingInformation&
Flow::getDIFName() const {
	return DIFName;
}

const ApplicationProcessNamingInformation&
Flow::getLocalApplicationName() const {
	return localApplicationName;
}

const ApplicationProcessNamingInformation&
Flow::getRemoteApplcationName() const {
	return remoteApplicationName;
}

const FlowSpecification& Flow::getFlowSpecification() const {
	return flowSpecification;
}

bool Flow::isAllocated() const{
	return flowState == FLOW_ALLOCATED;
}

FlowInformation Flow::getFlowInformation() const {
   FlowInformation flowInformation;
   flowInformation.setDifName(DIFName);
   flowInformation.setFlowSpecification(flowSpecification);
   flowInformation.setLocalAppName(localApplicationName);
   flowInformation.setRemoteAppName(remoteApplicationName);
   flowInformation.setPortId(portId);

   return flowInformation;
}

int Flow::readSDU(void * sdu, int maxBytes)
		throw (FlowNotAllocatedException, ReadSDUException) {
	if (flowState != FLOW_ALLOCATED) {
		throw FlowNotAllocatedException();
	}

#if STUB_API
        memset(sdu, 'v', maxBytes);

	return maxBytes;
#else
	int result = syscallReadSDU(portId, sdu, maxBytes);
	if (result < 0){
		throw ReadSDUException();
	}

	return result;
#endif
}

void Flow::writeSDU(void * sdu, int size)
		throw (FlowNotAllocatedException, WriteSDUException) {
	if (flowState != FLOW_ALLOCATED) {
		throw FlowNotAllocatedException();
	}

#if STUB_API
	/* Do nothing. */
        (void)sdu;
        (void)size;
#else
	int result = syscallWriteSDU(portId, sdu, size);
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
                        unsigned int seqNumber) throw (IPCException) {
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
                unsigned short sourceIPCProcessId) throw (FlowAllocationException) {
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
                const FlowSpecification& flowSpec)
throw (FlowAllocationException) {
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
                int result, bool notifySource, unsigned short ipcProcessId)
throw (FlowAllocationException) {
#if STUB_API
        //Do nothing
        (void)notifySource;
        (void)ipcProcessId;
#else
        AppAllocateFlowResponseMessage responseMessage;
        responseMessage.setResult(result);
        responseMessage.setNotifySource(notifySource);
        responseMessage.setSourceIpcProcessId(ipcProcessId);
        responseMessage.setSequenceNumber(flowRequestEvent.getSequenceNumber());
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

        Flow * flow = new Flow(flowRequestEvent.getLocalApplicationName(),
                        flowRequestEvent.getRemoteApplicationName(),
                        flowRequestEvent.getFlowSpecification(), FLOW_ALLOCATED,
                        flowRequestEvent.getDIFName(), flowRequestEvent.getPortId());
        lock();
        allocatedFlows[flowRequestEvent.getPortId()] = flow;
        unlock();
        return flow;
}

unsigned int IPCManager::getDIFProperties(
		const ApplicationProcessNamingInformation& applicationName,
		const ApplicationProcessNamingInformation& DIFName)
throw (GetDIFPropertiesException) {

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
                const ApplicationRegistrationInformation& appRegistrationInfo)
throw (ApplicationRegistrationException) {
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

ApplicationRegistration * IPCManager::commitPendingResitration(
                        unsigned int seqNumber,
                        const ApplicationProcessNamingInformation& DIFName)
throw (ApplicationRegistrationException) {
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
                        appRegInfo.getApplicationName());

        if (!applicationRegistration){
                applicationRegistration = new ApplicationRegistration(
                                appRegInfo.getApplicationName());
                applicationRegistrations[appRegInfo.getApplicationName()] =
                                applicationRegistration;
        }

        applicationRegistration->addDIFName(DIFName);
        unlock();

        return applicationRegistration;
}

void IPCManager::withdrawPendingRegistration(unsigned int seqNumber)
throw (ApplicationRegistrationException) {
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
		const ApplicationProcessNamingInformation& DIFName)
		throw (ApplicationUnregistrationException) {
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
        for (iterator = applicationRegistration->getDIFNames().begin();
                        iterator != applicationRegistration->getDIFNames().end();
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
        appRegInfo.setApplicationName(applicationName);
        appRegInfo.setDIFName(DIFName);

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

void IPCManager::appUnregistrationResult(unsigned int seqNumber, bool success)
                                throw (ApplicationUnregistrationException) {
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
                       appRegInfo.getApplicationName());
       if (!applicationRegistration){
               unlock();
               throw ApplicationUnregistrationException(
                       IPCManager::application_not_registered_error);
       }

       if (!success) {
    	   return;
       }

       std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
       for (iterator = applicationRegistration->getDIFNames().begin();
                       iterator != applicationRegistration->getDIFNames().end();
                       ++iterator) {
               if (*iterator == appRegInfo.getDIFName()) {
                       applicationRegistration->removeDIFName(
                                       appRegInfo.getDIFName());
                       if (applicationRegistration->getDIFNames().size() == 0) {
                               applicationRegistrations.erase(
                                       appRegInfo.getApplicationName());
                       }

                       break;
               }
       }

       unlock();
}

unsigned int IPCManager::requestFlowAllocation(
		const ApplicationProcessNamingInformation& localAppName,
		const ApplicationProcessNamingInformation& remoteAppName,
		const FlowSpecification& flowSpec)
throw (FlowAllocationException) {
        return internalRequestFlowAllocation(
                        localAppName, remoteAppName, flowSpec, 0);
}

unsigned int IPCManager::requestFlowAllocationInDIF(
                const ApplicationProcessNamingInformation& localAppName,
                const ApplicationProcessNamingInformation& remoteAppName,
                const ApplicationProcessNamingInformation& difName,
                const FlowSpecification& flowSpec)
throw (FlowAllocationException) {
        return internalRequestFlowAllocationInDIF(localAppName,
                        remoteAppName, difName, 0, flowSpec);
}

Flow * IPCManager::commitPendingFlow(unsigned int sequenceNumber, int portId,
                        const ApplicationProcessNamingInformation& DIFName)
                throw (FlowAllocationException) {
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

FlowInformation IPCManager::withdrawPendingFlow(unsigned int sequenceNumber)
                throw (FlowAllocationException) {
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
		bool notifySource) throw (FlowAllocationException) {
        return internalAllocateFlowResponse(
                        flowRequestEvent, result, notifySource, 0);
}

unsigned int IPCManager::requestFlowDeallocation(int portId)
        throw (FlowDeallocationException) {
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
		flow->getLocalApplicationName().getProcessName().c_str(),
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

void IPCManager::flowDeallocationResult(int portId, bool success)
                        throw (FlowDeallocationException) {
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

void IPCManager::flowDeallocated(int portId)
throw (FlowDeallocationException) {
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
	LOG_DBG("IPCManager.getAllocatedFlows called");
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
				APPLICATION_REGISTRATION_CANCELED_EVENT,
				sequenceNumber){
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

const ApplicationProcessNamingInformation&
AllocateFlowRequestResultEvent::getAppName() const{
        return sourceAppName;
}

const ApplicationProcessNamingInformation&
        AllocateFlowRequestResultEvent::getDIFName() const{
        return difName;
}

int AllocateFlowRequestResultEvent::getPortId() const{
        return portId;
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

const ApplicationProcessNamingInformation&
DeallocateFlowResponseEvent::getAppName() const{
        return appName;
}

int DeallocateFlowResponseEvent::getPortId() const{
        return portId;
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

const ApplicationProcessNamingInformation&
GetDIFPropertiesResponseEvent::getAppName() const {
        return applicationName;
}

const std::list<DIFProperties>& GetDIFPropertiesResponseEvent::
        getDIFProperties() const {
        return difProperties;
}

}
