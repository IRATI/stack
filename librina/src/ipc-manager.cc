//
// IPC Manager
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#include <algorithm>
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>

#define RINA_PREFIX "librina.ipc-manager"

#include "librina/logs.h"

#include "librina/ipc-manager.h"
#include "librina/concurrency.h"
#include "rina-syscalls.h"

#include "utils.h"
#include "core.h"

namespace rina {

std::string _installation_path;
std::string _library_path;
std::string _log_path;
std::string _log_level;

void initializeIPCManager(unsigned int        localPort,
                          const std::string & installationPath,
                          const std::string & libraryPath,
                          const std::string & logLevel,
                          const std::string & pathToLogFolder)
{
	std::string ipcmPathToLogFile = pathToLogFolder + "/ipcm.log";
	initialize(localPort, logLevel, ipcmPathToLogFile);

	_installation_path = installationPath;
	_library_path = libraryPath;
	_log_path = pathToLogFolder;
	_log_level = logLevel;

	IpcmIPCManagerPresentMessage message;
	message.setDestPortId(0);
	message.setNotificationMessage(true);

	try {
		rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
		throw InitializationException(e.what());
	}
}

void destroyIPCManager()
{
	librina_finalize();
}

/* CLASS IPC PROCESS*/
const std::string IPCProcessProxy::error_assigning_to_dif =
		"Error assigning IPC Process to DIF";
const std::string IPCProcessProxy::error_update_dif_config =
                "Error updating DIF Configuration";
const std::string IPCProcessProxy::error_registering_app =
		"Error registering application";
const std::string IPCProcessProxy::error_unregistering_app =
		"Error unregistering application";
const std::string IPCProcessProxy::error_not_a_dif_member =
		"Error: the IPC Process is not member of a DIF";
const std::string IPCProcessProxy::error_allocating_flow =
		"Error allocating flow";
const std::string IPCProcessProxy::error_deallocating_flow =
		"Error deallocating flow";
const std::string IPCProcessProxy::error_querying_rib =
		"Error querying rib";


IPCProcessProxy::IPCProcessProxy() {
	id = 0;
	portId = 0;
	pid = 0;
}

IPCProcessProxy::IPCProcessProxy(unsigned short id, unsigned int portId,
                       pid_t pid, const std::string& type,
                       const ApplicationProcessNamingInformation& name)
{
	this->id = id;
	this->portId = portId;
	this->pid = pid;
	this->type = type;
	this->name = name;
}

unsigned short IPCProcessProxy::getId() const
{ return id; }

const std::string& IPCProcessProxy::getType() const
{ return type; }

const ApplicationProcessNamingInformation& IPCProcessProxy::getName() const
{ return name; }

unsigned int IPCProcessProxy::getPortId() const
{ return portId; }

void IPCProcessProxy::setPortId(unsigned int portId)
{ this->portId = portId; }

pid_t IPCProcessProxy::getPid() const
{ return pid; }

void IPCProcessProxy::setPid(pid_t pid)
{ this->pid = pid; }

void IPCProcessProxy::assignToDIF(const DIFInformation& difInformation,
		unsigned int opaque)
{
#if STUB_API
        //Do nothing
#else
        IpcmAssignToDIFRequestMessage message;
        message.setDIFInformation(difInformation);
        message.setDestIpcProcessId(id);
        message.setDestPortId(portId);
        message.setRequestMessage(true);
        message.setSequenceNumber(opaque);

        try {
                //FIXME, compute maximum message size dynamically
                rinaManager->sendMessageOfMaxSize(&message,
                                                  5 * get_page_size(),
                                                  false);
        } catch (NetlinkException &e) {
                throw AssignToDIFException(e.what());
        }
#endif
}

void
IPCProcessProxy::updateDIFConfiguration(const DIFConfiguration& difConfiguration,
		unsigned int opaque)
{
#if STUB_API
        //Do nothing
#else
        IpcmUpdateDIFConfigurationRequestMessage message;
        message.setDIFConfiguration(difConfiguration);
        message.setDestIpcProcessId(id);
        message.setDestPortId(portId);
        message.setRequestMessage(true);
        message.setSequenceNumber(opaque);

        try {
                rinaManager->sendMessage(&message, false);
        } catch (NetlinkException &e) {
                throw UpdateDIFConfigurationException(e.what());
        }

#endif
}

void IPCProcessProxy::notifyRegistrationToSupportingDIF(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const ApplicationProcessNamingInformation& difName)
{
#if STUB_API
	//Do nothing
#else
	IpcmDIFRegistrationNotification message;
	message.setIpcProcessName(ipcProcessName);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setDifName(difName);
	message.setRegistered(true);
	message.setNotificationMessage(true);

	try {
		rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
		throw NotifyRegistrationToDIFException(e.what());
	}
#endif
}

void IPCProcessProxy::notifyUnregistrationFromSupportingDIF(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const ApplicationProcessNamingInformation& difName)
{
#if STUB_API
	//Do nothing
#else
	IpcmDIFRegistrationNotification message;
	message.setIpcProcessName(ipcProcessName);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setDifName(difName);
	message.setRegistered(false);
	message.setNotificationMessage(true);

	try {
		rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
		throw NotifyUnregistrationFromDIFException(e.what());
	}
#endif
}

void IPCProcessProxy::enroll(
        const ApplicationProcessNamingInformation& difName,
        const ApplicationProcessNamingInformation& supportingDifName,
        const ApplicationProcessNamingInformation& neighborName,
        unsigned int opaque)
{
#if STUB_API
        //Do nothing
#else
        IpcmEnrollToDIFRequestMessage message;
        message.setDifName(difName);
        message.setSupportingDifName(supportingDifName);
        message.setNeighborName(neighborName);
        message.setDestIpcProcessId(id);
        message.setDestPortId(portId);
        message.setRequestMessage(true);
        message.setSequenceNumber(opaque);

        try {
                rinaManager->sendMessage(&message, false);
        } catch(NetlinkException &e) {
                throw EnrollException(e.what());
        }
#endif
}

void IPCProcessProxy::disconnectFromNeighbor(
		const ApplicationProcessNamingInformation& neighbor)
{
	LOG_DBG("IPCProcess::disconnect from neighbour called");
	/* TODO: IMPLEMENT FUNCTIONALITY */
	ApplicationProcessNamingInformation a = neighbor;
	throw IPCException(IPCException::operation_not_implemented_error);
}

void IPCProcessProxy::registerApplication(
		const ApplicationProcessNamingInformation& applicationName,
		unsigned short regIpcProcessId,
		const ApplicationProcessNamingInformation& dif_name,
		unsigned int opaque)
{
#if STUB_API
	//Do nothing
#else
	IpcmRegisterApplicationRequestMessage message;
	message.setApplicationName(applicationName);
	message.setDifName(dif_name);
	message.setRegIpcProcessId(regIpcProcessId);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);
	message.setSequenceNumber(opaque);

	try {
	        rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
	        throw IpcmRegisterApplicationException(e.what());
	}

#endif
}

void IPCProcessProxy::unregisterApplication(
		const ApplicationProcessNamingInformation& applicationName,
		const ApplicationProcessNamingInformation& dif_name,
		unsigned int opaque)
{

#if STUB_API
	//Do nothing
#else
        IpcmUnregisterApplicationRequestMessage message;
        message.setApplicationName(applicationName);
        message.setDifName(dif_name);
        message.setDestIpcProcessId(id);
        message.setDestPortId(portId);
        message.setRequestMessage(true);
        message.setSequenceNumber(opaque);

        try {
        	rinaManager->sendMessage(&message, false);
        } catch (NetlinkException &e) {
        	LOG_DBG("Error %s", e.what());
        	throw IpcmUnregisterApplicationException(e.what());
        }
#endif
}

void IPCProcessProxy::allocateFlow(const FlowRequestEvent& flowRequest,
		unsigned int opaque)
{
#if STUB_API
	//Do nothing
#else
	IpcmAllocateFlowRequestMessage message;
	message.setSourceAppName(flowRequest.localApplicationName);
	message.setDestAppName(flowRequest.remoteApplicationName);
	message.setFlowSpec(flowRequest.flowSpecification);
	message.setDifName(flowRequest.DIFName);
	message.setSourceIpcProcessId(
	                flowRequest.flowRequestorIpcProcessId);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);
	message.setSequenceNumber(opaque);

	try {
	        rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
	        throw AllocateFlowException(e.what());
	}

#endif
}

void IPCProcessProxy::allocateFlowResponse(const FlowRequestEvent& flowRequest,
					   int result,
					   bool notifySource,
					   int flowAcceptorIpcProcessId)
{
#if STUB_API
	//Do nothing
#else
	IpcmAllocateFlowResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setNotifySource(notifySource);
	responseMessage.setSourceIpcProcessId(flowAcceptorIpcProcessId);
	responseMessage.setDestIpcProcessId(id);
	responseMessage.setDestPortId(portId);
	responseMessage.setSequenceNumber(flowRequest.sequenceNumber);
	responseMessage.setResponseMessage(true);

	try {
		rinaManager->sendMessage(&responseMessage, false);
	} catch (NetlinkException &e) {
		throw AllocateFlowException(e.what());
	}
#endif
}

void IPCProcessProxy::deallocateFlow(int flowPortId, unsigned int opaque)
{
#if STUB_API
	//Do nothing
#else
	IpcmDeallocateFlowRequestMessage message;
	message.setPortId(flowPortId);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);
	message.setSequenceNumber(opaque);

	try {
	        rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
	        throw IpcmDeallocateFlowException(e.what());
	}
#endif
}

void IPCProcessProxy::queryRIB(const std::string& objectClass,
		const std::string& objectName, unsigned long objectInstance,
		unsigned int scope, const std::string& filter,
		unsigned int opaque)
{
#if STUB_API
#else
	IpcmDIFQueryRIBRequestMessage message;
	message.setObjectClass(objectClass);
	message.setObjectName(objectName);
	message.setObjectInstance(objectInstance);
	message.setScope(scope);
	message.setFilter(filter);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);
	message.setSequenceNumber(opaque);

	try {
	        rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
	        throw QueryRIBException(e.what());
	}
#endif
}

void IPCProcessProxy::setPolicySetParam(const std::string& path,
                        const std::string& name, const std::string& value,
                        unsigned int opaque)
{
#if STUB_API
#else
	IpcmSetPolicySetParamRequestMessage message;
        message.path = path;
        message.name = name;
        message.value = value;
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);
	message.setSequenceNumber(opaque);

	try {
	        rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
	        throw SetPolicySetParamException(e.what());
	}
#endif
}

void IPCProcessProxy::selectPolicySet(const std::string& path,
                                 const std::string& name,
                                 unsigned int opaque)
{
#if STUB_API
#else
	IpcmSelectPolicySetRequestMessage message;
        message.path = path;
        message.name = name;
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);
	message.setSequenceNumber(opaque);

	try {
	        rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
	        throw SelectPolicySetException(e.what());
	}
#endif
}

void IPCProcessProxy::pluginLoad(const std::string& name, bool load,
		unsigned int opaque)
{
#if STUB_API
#else
	IpcmPluginLoadRequestMessage message;
        message.name = name;
        message.load = load;
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);
	message.setSequenceNumber(opaque);

	try {
	        rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
	        throw PluginLoadException(e.what());
	}
#endif
}

void IPCProcessProxy::forwardCDAPRequestMessage(const ser_obj_t& sermsg,
					 unsigned int opaque)
{
#if STUB_API
#else
	IpcmFwdCDAPRequestMessage message;
        message.sermsg = sermsg;
	message.result = 0;  // unused when IPC Manager sends
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);
	message.setSequenceNumber(opaque);

	try {
	        rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
	        throw FwdCDAPMsgException(e.what());
	}
#endif
}

void IPCProcessProxy::forwardCDAPResponseMessage(const ser_obj_t& sermsg,
                                         unsigned int opaque)
{
#if STUB_API
#else
        IpcmFwdCDAPResponseMessage message;
        message.sermsg = sermsg;
        message.result = 0;  // unused when IPC Manager sends
        message.setDestIpcProcessId(id);
        message.setDestPortId(portId);
        message.setRequestMessage(true);
        message.setSequenceNumber(opaque);

        try {
                rinaManager->sendMessage(&message, false);
        } catch (NetlinkException &e) {
                throw FwdCDAPMsgException(e.what());
        }
#endif
}


/** CLASS IPC PROCESS FACTORY */
const std::string IPCProcessFactory::unknown_ipc_process_error =
		"Could not find an IPC Process with the provided id";
const std::string IPCProcessFactory::path_to_ipc_process_types =
		"/sys/rina/ipcp-factories/";
const std::string IPCProcessFactory::normal_ipc_process_type =
		"normal";

IPCProcessFactory::IPCProcessFactory() {
}

IPCProcessFactory::~IPCProcessFactory() throw() {
}

std::list<std::string> IPCProcessFactory::getSupportedIPCProcessTypes() {
	std::list<std::string> result;

	DIR *dp;
	struct dirent *dirp;
	if((dp = opendir(path_to_ipc_process_types.c_str())) == 0) {
		LOG_ERR("Error %d opening %s", errno,
				path_to_ipc_process_types.c_str());
		return result;
	}

	std::string name;
	while ((dirp = readdir(dp)) != 0) {
		name = std::string(dirp->d_name);
		if (name.compare(".") != 0 && name.compare("..") != 0) {
			result.push_back(name);
		}
	}

	closedir(dp);

	return result;
}

IPCProcessProxy * IPCProcessFactory::create(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const std::string& difType,
		unsigned short ipcProcessId)
{
	unsigned int portId = 0;
	pid_t pid=0;

#if STUB_API
	//Do nothing
#else
	int result = syscallCreateIPCProcess(ipcProcessName,
                                             ipcProcessId,
                                             difType);
	if (result != 0) {
	        throw CreateIPCProcessException();
	}

	if (difType.compare(NORMAL_IPC_PROCESS) == 0) {
		pid = fork();
		if (pid == 0) {
			//This is the OS process that has to execute the IPC Process
			//program and then exit
			LOG_DBG("New OS Process created, executing IPC Process ...");

			char * argv[] =
			{
                                /* FIXME: These hardwired things must disappear */
				stringToCharArray(_installation_path +"/ipcp"),
				stringToCharArray(ipcProcessName.processName),
				stringToCharArray(ipcProcessName.processInstance),
				intToCharArray(ipcProcessId),
				intToCharArray(getNelinkPortId()),
				stringToCharArray(_log_level),
				stringToCharArray(_log_path + "/" + ipcProcessName.processName
						+ "-" + ipcProcessName.processInstance + ".log"),
				0
			};
			char * envp[] =
			{
                                /* FIXME: These hardwired things must disappear */
				stringToCharArray("PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"),
				stringToCharArray("LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"
					          +_library_path),
				(char*) 0
			};

			execve(argv[0], &argv[0], envp);

			LOG_ERR("Problems loading IPC Process program, finalizing OS Process with error %s", strerror(errno));

			//Destroy the IPC Process in the kernel
			syscallDestroyIPCProcess(ipcProcessId);

			exit(-1);
		}else if (pid < 0) {
			//This is the IPC Manager, and fork failed
		        //Try to destroy the IPC Process in the kernel and return error
		        syscallDestroyIPCProcess(ipcProcessId);
			throw CreateIPCProcessException();
		}else{
			//This is the IPC Manager, and fork was successful
		        portId = pid;
			LOG_DBG("Craeted a new IPC Process with pid = %d", pid);
		}
	}
#endif

	IPCProcessProxy * ipcProcess = new IPCProcessProxy(ipcProcessId, portId, pid, difType,
			ipcProcessName);
	return ipcProcess;
}

void IPCProcessFactory::destroy(IPCProcessProxy* ipcp)
{
	int resultUserSpace = 0;
	int resultKernel = 0;

#if STUB_API
	//Do nothing
#else
	resultKernel = syscallDestroyIPCProcess(ipcp->id);

	if (ipcp->getType().compare(NORMAL_IPC_PROCESS) == 0)
	{
		resultUserSpace = kill(ipcp->getPid(), SIGINT);
		// TODO: Change magic number
		usleep(1000000);
		resultUserSpace = kill(ipcp->getPid(), SIGKILL);
	}
#endif

	delete ipcp;

	if (resultKernel || resultUserSpace)
	{
	        std::string error = "Problems destroying IPCP.";
	        error = error + "Result in the kernel: " +
	                        intToCharArray(resultKernel);
	        error = error + "Result in user space:  " +
	                        intToCharArray(resultUserSpace);
	        LOG_ERR("%s", error.c_str());
	        throw DestroyIPCProcessException(error);
	}
}

/** CLASS APPLICATION MANAGER */
void ApplicationManager::applicationRegistered(
		const ApplicationRegistrationRequestEvent& event,
		const ApplicationProcessNamingInformation& difName, int result)
{
	LOG_DBG("ApplicationManager::applicationRegistered called");

#if STUB_API
	//Do nothing
#else
	AppRegisterApplicationResponseMessage responseMessage;
	responseMessage.setApplicationName(event.
			applicationRegistrationInformation.appName);
	responseMessage.setDifName(difName);
	responseMessage.setResult(result);
	responseMessage.setSequenceNumber(event.sequenceNumber);
	responseMessage.setResponseMessage(true);
	try {
		rinaManager->sendMessage(&responseMessage, false);
	} catch (NetlinkException &e) {
		throw NotifyApplicationRegisteredException(e.what());
	}
#endif
}

void ApplicationManager::applicationUnregistered(
		const ApplicationUnregistrationRequestEvent& event,
		int result)
{
	LOG_DBG("ApplicationManager::applicationUnregistered called");

#if STUB_API
	//Do nothing
#else
	AppUnregisterApplicationResponseMessage responseMessage;
	responseMessage.setApplicationName(event.applicationName);
	responseMessage.setResult(result);
	responseMessage.setSequenceNumber(event.sequenceNumber);
	responseMessage.setResponseMessage(true);
	try {
		rinaManager->sendMessage(&responseMessage, false);
	} catch (NetlinkException &e) {
		throw NotifyApplicationUnregisteredException(e.what());
	}
#endif
}

void ApplicationManager::flowAllocated(const FlowRequestEvent& flowRequestEvent)
{
	LOG_DBG("ApplicationManager::flowAllocated called");

#if STUB_API
	//Do nothing
#else
	AppAllocateFlowRequestResultMessage responseMessage;
	responseMessage.setPortId(flowRequestEvent.portId);
	responseMessage.setSourceAppName(flowRequestEvent.localApplicationName);
	responseMessage.setDifName(flowRequestEvent.DIFName);
	responseMessage.setSequenceNumber(flowRequestEvent.sequenceNumber);
	responseMessage.setResponseMessage(true);
	try {
		rinaManager->sendMessage(&responseMessage, false);
	} catch (NetlinkException &e) {
		throw NotifyFlowAllocatedException(e.what());
	}
#endif
}

void ApplicationManager::flowRequestArrived(
			const ApplicationProcessNamingInformation& localAppName,
			const ApplicationProcessNamingInformation& remoteAppName,
			const FlowSpecification& flowSpec,
			const ApplicationProcessNamingInformation& difName,
			int portId,
			unsigned int opaque)
{
#if STUB_API
#else
	AppAllocateFlowRequestArrivedMessage message;
	message.setSourceAppName(remoteAppName);
	message.setDestAppName(localAppName);
	message.setFlowSpecification(flowSpec);
	message.setDifName(difName);
	message.setPortId(portId);
	message.setRequestMessage(true);
	message.setSequenceNumber(opaque);

	try {
	        rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
	        throw AppFlowArrivedException(e.what());
	}
#endif
}

void ApplicationManager::flowDeallocated(
		const FlowDeallocateRequestEvent& event, int result)
{
	LOG_DBG("ApplicationManager::flowdeallocated called");

#if STUB_API
	//Do nothing
#else
	AppDeallocateFlowResponseMessage responseMessage;
	responseMessage.setApplicationName(event.applicationName);
	responseMessage.setPortId(event.portId);
	responseMessage.setResult(result);
	responseMessage.setSequenceNumber(event.sequenceNumber);
	responseMessage.setResponseMessage(true);
	try {
		rinaManager->sendMessage(&responseMessage, false);
	} catch (NetlinkException &e) {
		throw NotifyFlowDeallocatedException(e.what());
	}
#endif
}

void ApplicationManager::flowDeallocatedRemotely(
		int portId, int code,
		const ApplicationProcessNamingInformation& appName)
{
	LOG_DBG("ApplicationManager::flowDeallocatedRemotely called");
#if STUB_API
	//Do nothing
#else
	AppFlowDeallocatedNotificationMessage message;
	message.setPortId(portId);
	message.setCode(code);
	message.setApplicationName(appName);
	message.setNotificationMessage(true);
	try {
		rinaManager->sendMessage(&message, false);
	} catch (NetlinkException &e) {
		throw NotifyFlowDeallocatedException(e.what());
	}
#endif
}

void ApplicationManager::getDIFPropertiesResponse(
		const GetDIFPropertiesRequestEvent &event,
			int result, const std::list<DIFProperties>& difProperties)
{
#if STUB_API
	//Do nothing
#else
	AppGetDIFPropertiesResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setApplicationName(event.getApplicationName());
	responseMessage.setDIFProperties(difProperties);
	responseMessage.setSequenceNumber(event.sequenceNumber);
	responseMessage.setResponseMessage(true);
	try {
		rinaManager->sendMessage(&responseMessage, false);
	} catch (NetlinkException &e) {
		throw GetDIFPropertiesResponseException(e.what());
	}
#endif
}

Singleton<ApplicationManager> applicationManager;

/* CLASS GET DIF PROPERTIES REQUEST EVENT */
GetDIFPropertiesRequestEvent::GetDIFPropertiesRequestEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber):
        IPCEvent(GET_DIF_PROPERTIES, sequenceNumber)
{
	this->applicationName = appName;
	this->DIFName = DIFName;
}

const ApplicationProcessNamingInformation&
	GetDIFPropertiesRequestEvent::getApplicationName() const
{ return applicationName; }

const ApplicationProcessNamingInformation&
	GetDIFPropertiesRequestEvent::getDIFName() const
{ return DIFName; }

/* CLASS IPCM REGISTER APPLICATION RESPONSE EVENT */
IpcmRegisterApplicationResponseEvent::IpcmRegisterApplicationResponseEvent(
                int result, unsigned int sequenceNumber):
                        BaseResponseEvent(result,
                                        IPCM_REGISTER_APP_RESPONSE_EVENT,
                                        sequenceNumber)
{ }

/* CLASS IPCM UNREGISTER APPLICATION RESPONSE EVENT */
IpcmUnregisterApplicationResponseEvent::IpcmUnregisterApplicationResponseEvent(
                int result, unsigned int sequenceNumber):
                        BaseResponseEvent(result,
                                        IPCM_UNREGISTER_APP_RESPONSE_EVENT,
                                        sequenceNumber)
{ }

/* CLASS IPCM DEALLOCATE FLOW RESPONSE EVENT */
IpcmDeallocateFlowResponseEvent::IpcmDeallocateFlowResponseEvent(
                int result, unsigned int sequenceNumber):
                        BaseResponseEvent(result,
                                        IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT,
                                        sequenceNumber)
{ }

/* CLASS IPCM ALLOCATE FLOW REQUEST RESULT EVENT */
IpcmAllocateFlowRequestResultEvent::IpcmAllocateFlowRequestResultEvent(
                int result, int portId, unsigned int sequenceNumber):
                        BaseResponseEvent(result,
                                        IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
                                        sequenceNumber)
{ this->portId = portId; }

int IpcmAllocateFlowRequestResultEvent::getPortId() const
{ return portId; }

/* CLASS QUERY RIB RESPONSE EVENT */
QueryRIBResponseEvent::QueryRIBResponseEvent(
                const std::list<rib::RIBObjectData>& ribObjects,
                int result,
                unsigned int sequenceNumber) :
        BaseResponseEvent(result,
                          QUERY_RIB_RESPONSE_EVENT,
                          sequenceNumber)
{ this->ribObjects = ribObjects; }

const std::list<rib::RIBObjectData>& QueryRIBResponseEvent::getRIBObject() const
{ return ribObjects; }

/* CLASS UPDATE DIF CONFIGURATION RESPONSE EVENT */
UpdateDIFConfigurationResponseEvent::UpdateDIFConfigurationResponseEvent(
                int result, unsigned int sequenceNumber):
                        BaseResponseEvent(result,
                                        UPDATE_DIF_CONFIG_RESPONSE_EVENT,
                                        sequenceNumber)
{ }

/* CLASS ENROLL TO DIF RESPONSE EVENT */
EnrollToDIFResponseEvent::EnrollToDIFResponseEvent(
                const std::list<Neighbor>& neighbors,
                const DIFInformation& difInformation,
                int result, unsigned int sequenceNumber):
                        BaseResponseEvent(result,
                                        ENROLL_TO_DIF_RESPONSE_EVENT,
                                        sequenceNumber)
{
        this->neighbors = neighbors;
        this->difInformation = difInformation;
}

const std::list<Neighbor>&
EnrollToDIFResponseEvent::getNeighbors() const
{ return neighbors; }

const DIFInformation&
EnrollToDIFResponseEvent::getDIFInformation() const
{ return difInformation; }

/* CLASS IPC PROCESS DAEMON INITIALIZED EVENT */
IPCProcessDaemonInitializedEvent::IPCProcessDaemonInitializedEvent(
                unsigned short ipcProcessId,
                const ApplicationProcessNamingInformation&  name,
                unsigned int sequenceNumber):
                        IPCEvent(IPC_PROCESS_DAEMON_INITIALIZED_EVENT,
                                        sequenceNumber)
{
        this->ipcProcessId = ipcProcessId;
        this->name = name;
}

unsigned short IPCProcessDaemonInitializedEvent::getIPCProcessId() const
{ return ipcProcessId; }

const ApplicationProcessNamingInformation&
IPCProcessDaemonInitializedEvent::getName() const
{ return name; }

/* CLASS TIMER EXPIRED EVENT */
TimerExpiredEvent::TimerExpiredEvent(unsigned int sequenceNumber) :
                IPCEvent(TIMER_EXPIRED_EVENT, sequenceNumber)
{ }

}
