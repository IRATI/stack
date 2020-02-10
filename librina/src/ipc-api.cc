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
#include <fcntl.h>
#include <sys/ioctl.h>

#define RINA_PREFIX "librina.ipc-api"

#include "librina/logs.h"
#include "core.h"
#include "ctrl.h"
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

unsigned int
IPCManager::internalRequestFlowAllocation(const ApplicationProcessNamingInformation& localAppName,
					  const ApplicationProcessNamingInformation& remoteAppName,
					  const FlowSpecification& flowSpec,
					  unsigned short sourceIPCProcessId)
{
        FlowInformation * flow;
        unsigned int result = 0;
        ApplicationProcessNamingInformation dif_name;

        WriteScopedLock writeLock(flows_rw_lock);

#if STUB_API
#else
        struct irati_kmsg_ipcm_allocate_flow * msg;

        msg = new irati_kmsg_ipcm_allocate_flow();
        msg->msg_type = RINA_C_APP_ALLOCATE_FLOW_REQUEST;
        msg->src_ipcp_id = sourceIPCProcessId;
        msg->dest_ipcp_id = 0;
        msg->dest_port = IPCM_CTRLDEV_PORT;
        msg->local = localAppName.to_c_name();
        msg->remote = remoteAppName.to_c_name();
        msg->dif_name = dif_name.to_c_name();
        msg->fspec = flowSpec.to_c_flowspec();
        msg->pid = getpid();

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw FlowAllocationException("Problems sending CTRL message");
        }

        result = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
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
        struct irati_kmsg_ipcm_allocate_flow * msg;

        msg = new irati_kmsg_ipcm_allocate_flow();
        msg->msg_type = RINA_C_APP_ALLOCATE_FLOW_REQUEST;
        msg->src_ipcp_id = sourceIPCProcessId;
        msg->dest_ipcp_id = 0;
        msg->dest_port = IPCM_CTRLDEV_PORT;
        msg->local = localAppName.to_c_name();
        msg->remote = remoteAppName.to_c_name();
        msg->dif_name = difName.to_c_name();
        msg->fspec = flowSpec.to_c_flowspec();
        msg->pid = getpid();

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw FlowAllocationException("Problems sending CTRL message");
        }

        result = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif

        flow = new FlowInformation();
        flow->localAppName = localAppName;
        flow->remoteAppName = remoteAppName;
        flow->flowSpecification = flowSpec;
        flow->state = FlowInformation::FLOW_ALLOCATION_REQUESTED;
        flow->user_ipcp_id = sourceIPCProcessId;

        pendingFlows[result] = flow;

        return result;
}

FlowInformation IPCManager::internalAllocateFlowResponse(const FlowRequestEvent& flowRequestEvent,
                					 int result,
							 bool notifySource,
							 unsigned short ipcProcessId,
							 bool blocking)
{
	FlowInformation * flow = 0;

	WriteScopedLock writeLock(flows_rw_lock);

#if STUB_API
#else
        struct irati_msg_app_alloc_flow_response * msg;
        int ret = 0;

        msg = new irati_msg_app_alloc_flow_response();
        msg->msg_type = RINA_C_APP_ALLOCATE_FLOW_RESPONSE;
        msg->dest_ipcp_id = 0;
        msg->dest_port = IPCM_CTRLDEV_PORT;
        msg->not_source = notifySource;
        msg->result = result;
        msg->src_ipcp_id = ipcProcessId;
        msg->event_id = flowRequestEvent.sequenceNumber;
        msg->pid = getpid();

        ret = irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false);

        irati_ctrl_msg_free((struct irati_msg_base *) msg);

        if (ret)
        	throw FlowAllocationException("Problems sending CTRL message");
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
        flow->user_ipcp_id = ipcProcessId;

        // If the user of the flow is an application, init the I/O dev so that
        // data can be read and written to the flow via read/write calls
#if STUB_API
#else
        if (ipcProcessId == 0) {
        	initIodev(flow, flowRequestEvent.portId);
        	if (fcntl(flow->fd, F_SETFL, blocking ? 0 : O_NONBLOCK)) {
        		LOG_WARN("Failed to set blocking mode on fd %d", flow->fd);
        	}
        }
#endif

        allocatedFlows[flowRequestEvent.portId] = flow;

        return *flow;
}

unsigned int
IPCManager::getDIFProperties(const ApplicationProcessNamingInformation& applicationName,
			     const ApplicationProcessNamingInformation& DIFName)
{
	unsigned int seq_num = 0;

#if STUB_API
#else
        struct irati_msg_app_reg_app_resp * msg;
        int ret;

        msg = new irati_msg_app_reg_app_resp();
        msg->msg_type = RINA_C_APP_GET_DIF_PROPERTIES_REQUEST;
        msg->dest_ipcp_id = 0;
        msg->dest_port = IPCM_CTRLDEV_PORT;
        msg->app_name = applicationName.to_c_name();
        msg->dif_name = DIFName.to_c_name();

        ret = irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true);
        seq_num = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
        if (ret) {
        	throw GetDIFPropertiesException("Problems sending CTRL message");
        }
#endif
        return seq_num;
}

unsigned int
IPCManager::requestApplicationRegistration(const ApplicationRegistrationInformation& appRegistrationInfo)
{
	unsigned int seq_num = 0;

	WriteScopedLock writeLock(regs_rw_lock);

#if STUB_API
#else
        struct irati_msg_app_reg_app * msg;
        int ret;

        msg = new irati_msg_app_reg_app();
        msg->msg_type = RINA_C_APP_REGISTER_APPLICATION_REQUEST;
        msg->src_ipcp_id = appRegistrationInfo.ipcProcessId;
        msg->dest_ipcp_id = 0;
        msg->dest_port = IPCM_CTRLDEV_PORT;
        msg->app_name = appRegistrationInfo.appName.to_c_name();
        msg->dif_name = appRegistrationInfo.difName.to_c_name();
        msg->daf_name = appRegistrationInfo.dafName.to_c_name();
        msg->ipcp_id = appRegistrationInfo.ipcProcessId;
        msg->reg_type = appRegistrationInfo.applicationRegistrationType;
        msg->pid = getpid();
        msg->fa_ctrl_port = irati_ctrl_mgr->get_irati_ctrl_port();

        ret = irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true);
        seq_num = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
        if (ret) {
        	throw ApplicationRegistrationException("Problems sending CTRL message");
        }

#endif
        registrationInformation[seq_num] = appRegistrationInfo;
	return seq_num;
}

ApplicationRegistration *
IPCManager::commitPendingRegistration(unsigned int seqNumber,
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

unsigned int
IPCManager::requestApplicationUnregistration(const ApplicationProcessNamingInformation& applicationName,
					     const ApplicationProcessNamingInformation& DIFName)
{
        ApplicationRegistration * applicationRegistration;
        bool found = false;
        unsigned int seq_num = 0;

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
#else
        struct irati_msg_app_reg_app_resp * msg;
        int ret = 0;

        msg = new irati_msg_app_reg_app_resp();
        msg->msg_type = RINA_C_APP_UNREGISTER_APPLICATION_REQUEST;
        msg->dest_ipcp_id = 0;
        msg->dest_port = IPCM_CTRLDEV_PORT;
        msg->app_name = applicationName.to_c_name();
        msg->dif_name = DIFName.to_c_name();

        ret = irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true);
        seq_num = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
        if (ret) {
        	throw ApplicationUnregistrationException("Problems sending CTRL message");
        }

	registrationInformation[seq_num] = appRegInfo;
#endif
	return seq_num;
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

void IPCManager::initIodev(FlowInformation *flow, int portId)
{
        flow->fd = irati_open_io_port(portId);
        if (flow->fd < 0) {
                std::ostringstream oss;
                oss << "Cannot open /dev/irati [" << strerror(errno) << "]";
                throw FlowAllocationException(oss.str());
        }
}

FlowInformation IPCManager::commitPendingFlow(unsigned int sequenceNumber,
				   	      int portId,
				              const ApplicationProcessNamingInformation& DIFName)
{
        FlowInformation * flow;
        WriteScopedLock writeLock(flows_rw_lock);

        flow = getPendingFlow(sequenceNumber);
        if (flow == 0) {
                throw FlowAllocationException(IPCManager::unknown_flow_error);
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

FlowInformation IPCManager::allocateFlowResponse(
	const FlowRequestEvent& flowRequestEvent,
	int result,
	bool notifySource,
	bool blocking)
{
        return internalAllocateFlowResponse(
                        flowRequestEvent,
			result,
			notifySource,
			0,
			blocking);
}

void IPCManager::deallocate_flow(int portId)
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
#else

	LOG_DBG("Application %s requested to deallocate flow with port-id %d and fd %d",
		flow->localAppName.processName.c_str(),
		flow->portId, flow->fd);

	if (flow->fd > 0) {
		close(flow->fd);
	} else {
		//IPCP
		struct irati_msg_app_dealloc_flow msg;

	        msg.msg_type = RINA_C_APP_DEALLOCATE_FLOW_REQUEST;
	        msg.port_id = portId;
	        msg.dest_port = IPCM_CTRLDEV_PORT;
	        msg.src_ipcp_id = flow->user_ipcp_id;
	        msg.dest_ipcp_id = 0;
	        msg.event_id = 0;
	        irati_ctrl_mgr->send_msg((struct irati_msg_base *) &msg, true);
	}
#endif

	allocatedFlows.erase(portId);
	delete flow;
}

void IPCManager::flowDeallocated(int portId)
{
        FlowInformation * flow;

        WriteScopedLock writeLock(flows_rw_lock);

	flow = getAllocatedFlow(portId);
	if (flow == 0) {
		throw FlowDeallocationException("Unknown flow");
	}

	if (flow->fd > 0) {
		close(flow->fd);
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

int IPCManager::getControlFd()
{
        return irati_ctrl_mgr->get_ctrl_fd();
}

Singleton<IPCManager> ipcManager;

/* CLASS APPLICATION UNREGISTERED EVENT */

ApplicationUnregisteredEvent::ApplicationUnregisteredEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id) :
				IPCEvent(APPLICATION_UNREGISTERED_EVENT,
					 sequenceNumber, ctrl_port, ipcp_id)
{
	this->applicationName = appName;
	this->DIFName = DIFName;
}

/* CLASS APPLICATION REGISTRATION CANCELED EVENT*/
AppRegistrationCanceledEvent::AppRegistrationCanceledEvent(int code,
                const std::string& reason,
                const ApplicationProcessNamingInformation& difName,
                unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id):
			IPCEvent(APPLICATION_REGISTRATION_CANCELED_EVENT,
				 sequenceNumber, ctrl_port, ipcp_id)
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
                        unsigned int sequenceNumber,
			unsigned int ctrl_port, unsigned short ipcp_id):
                                IPCEvent(ALLOCATE_FLOW_REQUEST_RESULT_EVENT,
                                         sequenceNumber, ctrl_port, ipcp_id)
{
        this->sourceAppName = appName;
        this->difName = difName;
        this->portId = portId;
}

/* CLASS Get DIF Properties Response EVENT*/
GetDIFPropertiesResponseEvent::GetDIFPropertiesResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const std::list<DIFProperties>& difProperties,
                        int result,
                        unsigned int sequenceNumber,
			unsigned int ctrl_port, unsigned short ipcp_id):
                                BaseResponseEvent(result,
                                         GET_DIF_PROPERTIES_RESPONSE_EVENT,
                                         sequenceNumber, ctrl_port, ipcp_id)
{
        this->applicationName = appName;
        this->difProperties = difProperties;
}

}
