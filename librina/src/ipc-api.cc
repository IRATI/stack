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
#include <errno.h>
#include <stdexcept>

#define RINA_PREFIX "librina.ipc-api"

#include "librina/logs.h"
#include "core.h"
#include "rina-syscalls.h"
#include "librina/ipc-api.h"

namespace rina {

/* CLASS APPLICATION REGISTRATION */
ApplicationRegistration::ApplicationRegistration(
		const ApplicationProcessNamingInformation& applicationName)
{
	this->applicationName = applicationName;
}

void ApplicationRegistration::addDIFName(
		const ApplicationProcessNamingInformation& DIFName)
{
	DIFNames.push_back(DIFName);
}

void ApplicationRegistration::removeDIFName(
		const ApplicationProcessNamingInformation& DIFName)
{
	DIFNames.remove(DIFName);
}

/* CLASS IPC MANAGER */
IPCManager::IPCManager()
{
}

IPCManager::~IPCManager() throw()
{
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

FlowInformation * IPCManager::getPendingFlow(unsigned int seqNumber)
{
        std::map<unsigned int, FlowInformation*>::iterator iterator;

        iterator = pendingFlows.find(seqNumber);
        if (iterator == pendingFlows.end()) {
                return 0;
        }

        return iterator->second;
}

FlowInformation * IPCManager::getAllocatedFlow(int portId)
{
        std::map<int, FlowInformation*>::iterator iterator;

        iterator = allocatedFlows.find(portId);
        if (iterator == allocatedFlows.end()) {
                return 0;
        }

        return iterator->second;
}

FlowInformation IPCManager::getFlowInformation(int portId)
{
        std::map<int, FlowInformation*>::iterator iterator;
        FlowInformation * flowInformation;

        ReadScopedLock readLock(flows_rw_lock);

        iterator = allocatedFlows.find(portId);
        if (iterator == allocatedFlows.end()) {
                throw UnknownFlowException();
        }

        flowInformation = iterator->second;
        return *flowInformation;
}

int IPCManager::getPortIdToRemoteApp(const ApplicationProcessNamingInformation& remoteAppName)
{
        std::map<int, FlowInformation*>::iterator iterator;

        ReadScopedLock readLock(flows_rw_lock);

        for(iterator = allocatedFlows.begin();
	    iterator != allocatedFlows.end();
	    ++iterator) {
                if (iterator->second->remoteAppName == remoteAppName) {
                        return iterator->second->portId;
                }
        }

        throw UnknownFlowException();
}

ApplicationRegistrationInformation IPCManager::getRegistrationInfo(unsigned int seqNumber)
{
        std::map<unsigned int, ApplicationRegistrationInformation>::iterator iterator;

        iterator = registrationInformation.find(seqNumber);
        if (iterator == registrationInformation.end()) {
                throw IPCException("Registration not found");
        }

        return iterator->second;
}

ApplicationRegistration * IPCManager::getApplicationRegistration(
                const ApplicationProcessNamingInformation& appName)
{
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
                        ApplicationRegistration * value)
{
        applicationRegistrations[key] = value;
}

void IPCManager::removeApplicationRegistration(
                        const ApplicationProcessNamingInformation& key)
{
        applicationRegistrations.erase(key);
}

unsigned int IPCManager::internalRequestFlowAllocation(
                const ApplicationProcessNamingInformation& localAppName,
                const ApplicationProcessNamingInformation& remoteAppName,
                const FlowSpecification& flowSpec,
                unsigned short sourceIPCProcessId)
{
        FlowInformation * flow;
        unsigned int result = 0;

        WriteScopedLock writeLock(flows_rw_lock);

#if STUB_API
#else
        AppAllocateFlowRequestMessage message;
        message.setSourceAppName(localAppName);
        message.setDestAppName(remoteAppName);
        message.setSourceIpcProcessId(sourceIPCProcessId);
        message.setFlowSpecification(flowSpec);
        message.setRequestMessage(true);

        try{
                rinaManager->sendMessage(&message, true);
        }catch(NetlinkException &e){
                throw FlowAllocationException(e.what());
        }

        result = message.getSequenceNumber();
#endif

        flow = new FlowInformation();
        flow->localAppName = localAppName;
        flow->remoteAppName = remoteAppName;
        flow->flowSpecification = flowSpec;
        flow->state = FlowInformation::FLOW_ALLOCATION_REQUESTED;

        pendingFlows[result] = flow;

        return result;
}

unsigned int IPCManager::internalRequestFlowAllocationInDIF(
                const ApplicationProcessNamingInformation& localAppName,
                const ApplicationProcessNamingInformation& remoteAppName,
                const ApplicationProcessNamingInformation& difName,
                unsigned short sourceIPCProcessId,
                const FlowSpecification& flowSpec)
{
	unsigned int result = 0;
	FlowInformation * flow = 0;

	WriteScopedLock writeLock(flows_rw_lock);

#if STUB_API
#else
        AppAllocateFlowRequestMessage message;
        message.setSourceAppName(localAppName);
        message.setDestAppName(remoteAppName);
        message.setSourceIpcProcessId(sourceIPCProcessId);
        message.setFlowSpecification(flowSpec);
        message.setDifName(difName);
        message.setRequestMessage(true);

        try{
                rinaManager->sendMessage(&message, true);
        }catch(NetlinkException &e){
                throw FlowAllocationException(e.what());
        }

        result = message.getSequenceNumber();
#endif

        flow = new FlowInformation();
        flow->localAppName = localAppName;
        flow->remoteAppName = remoteAppName;
        flow->flowSpecification = flowSpec;
        flow->state = FlowInformation::FLOW_ALLOCATION_REQUESTED;

        pendingFlows[result] = flow;

        return result;
}

FlowInformation IPCManager::internalAllocateFlowResponse(
                const FlowRequestEvent& flowRequestEvent,
                int result,
		bool notifySource,
		unsigned short ipcProcessId,
		bool blocking)
{
	FlowInformation * flow = 0;

	WriteScopedLock writeLock(flows_rw_lock);

#if STUB_API
#else
        AppAllocateFlowResponseMessage responseMessage;
        responseMessage.setResult(result);
        responseMessage.setNotifySource(notifySource);
        responseMessage.setSourceIpcProcessId(ipcProcessId);
        responseMessage.setSequenceNumber(flowRequestEvent.sequenceNumber);
        responseMessage.setResponseMessage(true);
        try{
                rinaManager->sendMessage(&responseMessage, false);
        }catch(NetlinkException &e){
                throw FlowAllocationException(e.what());
        }
#endif
        if (result != 0) {
                LOG_WARN("Flow was not accepted, error code: %d", result);
                FlowInformation flowInformation;
                return flowInformation;
        }

        flow = new FlowInformation();
        flow->localAppName = flowRequestEvent.localApplicationName;
        flow->remoteAppName = flowRequestEvent.remoteApplicationName;
        flow->flowSpecification = flowRequestEvent.flowSpecification;
        flow->state = FlowInformation::FLOW_ALLOCATED;
        flow->difName = flowRequestEvent.DIFName;
        flow->portId = flowRequestEvent.portId;

	setFlowOptsBlocking(flow->portId, blocking);
        allocatedFlows[flowRequestEvent.portId] = flow;

        return *flow;
}

unsigned int IPCManager::getDIFProperties(
		const ApplicationProcessNamingInformation& applicationName,
		const ApplicationProcessNamingInformation& DIFName)
{

#if STUB_API
	return 0;
#else
	AppGetDIFPropertiesRequestMessage message;
	message.setApplicationName(applicationName);
	message.setDifName(DIFName);
	message.setRequestMessage(true);

	try{
		rinaManager->sendMessage(&message, true);
	}catch(NetlinkException &e){
		throw GetDIFPropertiesException(e.what());
	}

	return message.getSequenceNumber();
#endif
}

unsigned int IPCManager::requestApplicationRegistration(
                const ApplicationRegistrationInformation& appRegistrationInfo)
{
	WriteScopedLock writeLock(regs_rw_lock);

#if STUB_API
        registrationInformation[0] = appRegistrationInfo;
	return 0;
#else
	AppRegisterApplicationRequestMessage message;
	message.setApplicationRegistrationInformation(appRegistrationInfo);
	message.setRequestMessage(true);

	try{
	        rinaManager->sendMessage(&message, true);
	}catch(NetlinkException &e){
	        throw ApplicationRegistrationException(e.what());
	}

	registrationInformation[message.getSequenceNumber()] = appRegistrationInfo;

	return message.getSequenceNumber();
#endif
}

ApplicationRegistration * IPCManager::commitPendingRegistration(
                        unsigned int seqNumber,
                        const ApplicationProcessNamingInformation& DIFName)
{
        ApplicationRegistrationInformation appRegInfo;
        ApplicationRegistration * applicationRegistration;

        WriteScopedLock writeLock(regs_rw_lock);

        try {
        	appRegInfo = getRegistrationInfo(seqNumber);
        } catch(IPCException &e){
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

        return applicationRegistration;
}

void IPCManager::withdrawPendingRegistration(unsigned int seqNumber)
{
        ApplicationRegistrationInformation appRegInfo;

        WriteScopedLock writeLock(regs_rw_lock);

        try {
        	appRegInfo = getRegistrationInfo(seqNumber);
        } catch(IPCException &e){
        	throw ApplicationRegistrationException("Unknown registration");
        }

        registrationInformation.erase(seqNumber);
}

unsigned int IPCManager::requestApplicationUnregistration(
		const ApplicationProcessNamingInformation& applicationName,
		const ApplicationProcessNamingInformation& DIFName)
{
        ApplicationRegistration * applicationRegistration;
        bool found = false;

        WriteScopedLock writeLock(regs_rw_lock);

        applicationRegistration = getApplicationRegistration(applicationName);
        if (!applicationRegistration){
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
                throw ApplicationUnregistrationException(
                                IPCManager::application_not_registered_error);
        }

        ApplicationRegistrationInformation appRegInfo;
        appRegInfo.appName = applicationName;
        appRegInfo.difName = DIFName;

#if STUB_API
        registrationInformation[0] = appRegInfo;
	return 0;
#else
	AppUnregisterApplicationRequestMessage message;
	message.setApplicationName(applicationName);
	message.setDifName(DIFName);
	message.setRequestMessage(true);

	try{
	        rinaManager->sendMessage(&message, true);
	}catch(NetlinkException &e){
	        throw ApplicationUnregistrationException(e.what());
	}

	registrationInformation[message.getSequenceNumber()] =
	                appRegInfo;

	return message.getSequenceNumber();
#endif
}

void IPCManager::appUnregistrationResult(unsigned int seqNumber, bool success)
{
        ApplicationRegistrationInformation appRegInfo;

        WriteScopedLock writeLock(regs_rw_lock);

        try {
                appRegInfo = getRegistrationInfo(seqNumber);
        } catch (IPCException &e){
                throw ApplicationUnregistrationException(
                                "Pending unregistration not found");
        }

       registrationInformation.erase(seqNumber);
       ApplicationRegistration * applicationRegistration;

       applicationRegistration = getApplicationRegistration(
                       appRegInfo.appName);
       if (!applicationRegistration){
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
}

unsigned int IPCManager::requestFlowAllocation(
		const ApplicationProcessNamingInformation& localAppName,
		const ApplicationProcessNamingInformation& remoteAppName,
		const FlowSpecification& flowSpec)
{
        return internalRequestFlowAllocation(
                        localAppName, remoteAppName, flowSpec, 0);
}

unsigned int IPCManager::requestFlowAllocationInDIF(
                const ApplicationProcessNamingInformation& localAppName,
                const ApplicationProcessNamingInformation& remoteAppName,
                const ApplicationProcessNamingInformation& difName,
                const FlowSpecification& flowSpec)
{
        return internalRequestFlowAllocationInDIF(localAppName,
                        remoteAppName, difName, 0, flowSpec);
}

FlowInformation IPCManager::commitPendingFlow(unsigned int sequenceNumber,
				   	      int portId,
				              const ApplicationProcessNamingInformation& DIFName)
{
        FlowInformation * flow;

        WriteScopedLock writeLock(flows_rw_lock);

        flow = getPendingFlow(sequenceNumber);
        if (flow == 0) {
                throw FlowDeallocationException(IPCManager::unknown_flow_error);
        }

        pendingFlows.erase(sequenceNumber);

        flow->portId = portId;
        flow->difName = DIFName;
        flow->state = FlowInformation::FLOW_ALLOCATED;
        allocatedFlows[portId] = flow;

        return *flow;
}

FlowInformation IPCManager::withdrawPendingFlow(unsigned int sequenceNumber)
{
        std::map<int, FlowInformation*>::iterator iterator;
        FlowInformation* flow;
        FlowInformation result;

        WriteScopedLock writeLock(flows_rw_lock);

        flow = getPendingFlow(sequenceNumber);
        if (flow == 0) {
                throw FlowDeallocationException(IPCManager::unknown_flow_error);
        }

        pendingFlows.erase(sequenceNumber);
        result = *flow;
        delete flow;

        return result;
}

/* FIXME: bool blocking should be replaced by flow_opts_t */
FlowInformation IPCManager::allocateFlowResponse(
	const FlowRequestEvent& flowRequestEvent,
	int result,
	bool notifySource,
	bool blocking /* = true */)
{
        return internalAllocateFlowResponse(
                        flowRequestEvent,
			result,
			notifySource,
			0,
			blocking);
}

/* returns 0 if nonblocking, > 0 if blocking, < 0 on error */
int IPCManager::flowOptsBlocking(int portId)
{
#if STUB_API
	return 0;
#else

	FlowInformation *flow;
	uint             flags;

	flow = getAllocatedFlow(portId);
	if (flow == 0) {
		return -1;
        }

        if (flow->state != FlowInformation::FLOW_ALLOCATED) {
                return -1;
	}

        flags = syscallFlowIOCtl(portId, F_GETFL, 0 /* ignored */);
	return !(flags & O_NONBLOCK);
#endif
}

int IPCManager::setFlowOptsBlocking(int portId, bool blocking)
{
#if STUB_API
        return 0;
#else

	FlowInformation * flow;
	uint              flags;

	flow = getAllocatedFlow(portId);
	if (flow == 0) {
		return -1;
        }

        if (flow->state != FlowInformation::FLOW_ALLOCATED) {
                return -1;
	}

	/* this mimics the fcntl approach to setting flags */
	flags = syscallFlowIOCtl(portId, F_GETFL,0);
	if (!blocking)
		flags |= O_NONBLOCK; /* set nonblocking */
	else flags &= ~O_NONBLOCK; /* clear nonblocking */

	return syscallFlowIOCtl(portId, F_SETFL,flags);
#endif
}

unsigned int IPCManager::requestFlowDeallocation(int portId)
{
        FlowInformation * flow;

        WriteScopedLock writeLock(flows_rw_lock);

        flow = getAllocatedFlow(portId);
        if (flow == 0) {
                throw FlowDeallocationException(IPCManager::unknown_flow_error);
        }

        if (flow->state != FlowInformation::FLOW_ALLOCATED) {
                throw FlowDeallocationException(IPCManager::wrong_flow_state);
        }

#if STUB_API
        flow->state = FlowInformation::FLOW_DEALLOCATION_REQUESTED;
	return 0;
#else

	LOG_DBG("Application %s requested to deallocate flow with port-id %d",
		flow->localAppName.processName.c_str(),
		flow->portId);
	AppDeallocateFlowRequestMessage message;
	message.setApplicationName(flow->localAppName);
	message.setPortId(flow->portId);
	message.setRequestMessage(true);

	try{
	        rinaManager->sendMessage(&message, true);
	}catch(NetlinkException &e){
	        throw FlowDeallocationException(e.what());
	}

	flow->state = FlowInformation::FLOW_DEALLOCATION_REQUESTED;

	return message.getSequenceNumber();
#endif
}

void IPCManager::flowDeallocationResult(int portId, bool success)
{
        FlowInformation * flow;

        WriteScopedLock writeLock(flows_rw_lock);

        flow = getAllocatedFlow(portId);
        if (flow == 0) {
                throw FlowDeallocationException(
                                IPCManager::unknown_flow_error);
        }

        if (flow->state != FlowInformation::FLOW_DEALLOCATION_REQUESTED) {
                throw FlowDeallocationException(
                                IPCManager::wrong_flow_state);
        }

        if (success) {
                allocatedFlows.erase(portId);
                delete flow;
        } else {
                flow->state = FlowInformation::FLOW_ALLOCATED;
        }
}

void IPCManager::flowDeallocated(int portId)
{
        FlowInformation * flow;

        WriteScopedLock writeLock(flows_rw_lock);

	flow = getAllocatedFlow(portId);
	if (flow == 0) {
		throw FlowDeallocationException("Unknown flow");
	}

	allocatedFlows.erase(portId);
	delete flow;
}

std::vector<FlowInformation> IPCManager::getAllocatedFlows()
{
	std::vector<FlowInformation> response;

	ReadScopedLock readLock(flows_rw_lock);

	for (std::map<int, FlowInformation*>::iterator it = allocatedFlows.begin();
			it != allocatedFlows.end(); ++it) {
		response.push_back(*(it->second));
	}

	return response;
}

std::vector<ApplicationRegistration *> IPCManager::getRegisteredApplications()
{
	LOG_DBG("IPCManager.getRegisteredApplications called");
	std::vector<ApplicationRegistration *> response;

	ReadScopedLock readLock(regs_rw_lock);

	for (std::map<ApplicationProcessNamingInformation,
			ApplicationRegistration*>::iterator it = applicationRegistrations
			.begin(); it != applicationRegistrations.end(); ++it) {
		response.push_back(it->second);
	}

	return response;
}

int IPCManager::readSDU(int portId, void * sdu, int maxBytes)
{
#if STUB_API
        memset(sdu, 'v', maxBytes);
	return maxBytes;
#else
	int result = syscallReadSDU(portId, sdu, maxBytes);

	if (result >=0)
		return result;

	switch(result) {
	case -EINVAL:
		throw InvalidArgumentsException();
		break;
	case -EBADF:
		throw UnknownFlowException();
		break;
	case -ESHUTDOWN:
		throw FlowNotAllocatedException();
		break;
	case -EIO:
		throw ReadSDUException();
		break;
	case -EAGAIN:
		return 0;
	default:
		throw IPCException("Unknown error");
	}

	return result;
#endif
}

int IPCManager::writeSDU(int portId, void * sdu, int size)
{
#if STUB_API
	/* Do nothing. */

        return size;
#else
	int result = syscallWriteSDU(portId, sdu, size);

	if (result >= 0)
		return result;

	switch(result) {
	case -EINVAL:
		throw InvalidArgumentsException();
		break;
	case -EBADF:
		throw UnknownFlowException();
		break;
	case -ESHUTDOWN:
		throw FlowNotAllocatedException();
		break;
	case -EIO:
		throw WriteSDUException();
		break;
	case -EAGAIN:
		return 0;
	default:
		throw IPCException("Unknown error");
	}

	return result;
#endif
}

Singleton<IPCManager> ipcManager;

/* CLASS APPLICATION UNREGISTERED EVENT */

ApplicationUnregisteredEvent::ApplicationUnregisteredEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber) :
				IPCEvent(APPLICATION_UNREGISTERED_EVENT,
						sequenceNumber)
{
	this->applicationName = appName;
	this->DIFName = DIFName;
}

/* CLASS APPLICATION REGISTRATION CANCELED EVENT*/
AppRegistrationCanceledEvent::AppRegistrationCanceledEvent(int code,
                const std::string& reason,
                const ApplicationProcessNamingInformation& difName,
                unsigned int sequenceNumber):
			IPCEvent(APPLICATION_REGISTRATION_CANCELED_EVENT,
				 sequenceNumber)
{
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
                                IPCEvent(ALLOCATE_FLOW_REQUEST_RESULT_EVENT,
                                         sequenceNumber)
{
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
                                         sequenceNumber)
{
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
                                         sequenceNumber)
{
        this->applicationName = appName;
        this->difProperties = difProperties;
}

}
