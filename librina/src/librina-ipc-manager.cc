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

#define RINA_PREFIX "ipc-manager"

#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "logs.h"
#include "librina-ipc-manager.h"
#include "core.h"
#include "concurrency.h"
#include "rina-syscalls.h"

namespace rina{

std::string _installationPath;
std::string _libraryPath;

void initializeIPCManager(unsigned int localPort,
                const std::string& installationPath,
                const std::string& libraryPath)
	throw (IPCManagerInitializationException){
	initialize(localPort);

	_installationPath = installationPath;
	_libraryPath = libraryPath;

	IpcmIPCManagerPresentMessage message;
	message.setDestPortId(0);
	message.setNotificationMessage(true);

	try{
		rinaManager->sendResponseOrNotficationMessage(&message);
	}catch(NetlinkException &e){
		throw IPCManagerInitializationException(e.what());
	}
}

/* CLASS IPC PROCESS*/
const std::string IPCProcess::error_assigning_to_dif =
		"Error assigning IPC Process to DIF";
const std::string IPCProcess::error_update_dif_config =
                "Error updating DIF Configuration";
const std::string IPCProcess::error_registering_app =
		"Error registering application";
const std::string IPCProcess::error_unregistering_app =
		"Error unregistering application";
const std::string IPCProcess::error_not_a_dif_member =
		"Error: the IPC Process is not member of a DIF";
const std::string IPCProcess::error_allocating_flow =
		"Error allocating flow";
const std::string IPCProcess::error_deallocating_flow =
		"Error deallocating flow";
const std::string IPCProcess::error_querying_rib =
		"Error querying rib";

IPCProcess::IPCProcess() {
	id = 0;
	portId = 0;
	pid = 0;
	difMember = false;
}

IPCProcess::IPCProcess(unsigned short id, unsigned int portId,
		pid_t pid, const std::string& type,
		const ApplicationProcessNamingInformation& name) {
	this->id = id;
	this->portId = portId;
	this->pid = pid;
	this->type = type;
	this->name = name;
	difMember = false;
}

bool IPCProcess::isDIFMember() const{
	return difMember;
}

void IPCProcess::setDIFMember(bool difMember){
	this->difMember = difMember;
}

unsigned int IPCProcess::getId() const {
	return id;
}

const std::string& IPCProcess::getType() const {
	return type;
}

const ApplicationProcessNamingInformation& IPCProcess::getName() const {
	return name;
}

unsigned int IPCProcess::getPortId() const{
	return portId;
}

void IPCProcess::setPortId(unsigned int portId){
	this->portId = portId;
}

pid_t IPCProcess::getPid() const{
	return pid;
}

void IPCProcess::setPid(pid_t pid){
	this->pid = pid;
}

const DIFInformation& IPCProcess::getDIFInformation() const{
	return difInformation;
}

void IPCProcess::setDIFInformation(const DIFInformation& difInformation){
	this->difInformation = difInformation;
}

void IPCProcess::assignToDIF(
                const DIFInformation& difInformation)
throw (AssignToDIFException) {

        std::string currentDIFName =
                        this->difInformation.getDifName().getProcessName();
        LOG_DBG("Current DIF name is %s", currentDIFName.c_str());

        if(difMember) {
                std::string message;
                message =  message + "This IPC Process is already assigned "+
                                "to the DIF " + currentDIFName;
                LOG_ERR("%s", message.c_str());
                throw AssignToDIFException(message);
        }

#if STUB_API
	//Do nothing
#else
	IpcmAssignToDIFRequestMessage message;
	message.setDIFInformation(difInformation);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);

	IpcmAssignToDIFResponseMessage * assignToDIFResponse;
	try{
		assignToDIFResponse =
				dynamic_cast<IpcmAssignToDIFResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_assigning_to_dif));
	}catch(NetlinkException &e){
		throw AssignToDIFException(e.what());
	}

	if (assignToDIFResponse->getResult() < 0){
		delete assignToDIFResponse;
		throw AssignToDIFException(
				IPCProcess::error_assigning_to_dif);
	}

	LOG_DBG("Assigned IPC Process %d to DIF %s", id,
			difInformation.getDifName().getProcessName().c_str());
	delete assignToDIFResponse;
#endif

	this->difInformation = difInformation;
	this->difMember = true;
}

void IPCProcess::updateDIFConfiguration(
                        const DIFConfiguration& difConfiguration)
        throw (UpdateDIFConfigurationException)
{
        std::string currentDIFName =
                        this->difInformation.getDifName().getProcessName();
        LOG_DBG("Current DIF name is %s", currentDIFName.c_str());

        if(!difMember) {
                std::string message;
                message =  message + "This IPC Process is not yet assigned "+
                                "to any DIF.";
                LOG_ERR("%s", message.c_str());
                throw UpdateDIFConfigurationException(message);
        }

#if STUB_API
        //Do nothing
#else
        IpcmUpdateDIFConfigurationRequestMessage message;
        message.setDIFConfiguration(difConfiguration);
        message.setDestIpcProcessId(id);
        message.setDestPortId(portId);
        message.setRequestMessage(true);

        IpcmUpdateDIFConfigurationResponseMessage * updateConfigResponse;
        try{
                updateConfigResponse =
                                dynamic_cast<IpcmUpdateDIFConfigurationResponseMessage *>(
                                                rinaManager->sendRequestAndWaitForResponse(&message,
                                                                IPCProcess::error_update_dif_config));
        }catch(NetlinkException &e){
                throw UpdateDIFConfigurationException(e.what());
        }

        if (updateConfigResponse->getResult() < 0){
                delete updateConfigResponse;
                throw UpdateDIFConfigurationException(
                                IPCProcess::error_update_dif_config);
        }

        LOG_DBG("Updated configuratin of DIF %s",
                        difInformation.getDifName().getProcessName().c_str());
        delete updateConfigResponse;

#endif
        this->difInformation.setDifConfiguration(difConfiguration);
}

void IPCProcess::notifyRegistrationToSupportingDIF(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const ApplicationProcessNamingInformation& difName)
throw (NotifyRegistrationToDIFException) {
	LOG_DBG("IPCProcess::notify registration to supporting DIF called");
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

	try{
		rinaManager->sendResponseOrNotficationMessage(&message);
	}catch(NetlinkException &e){
		throw NotifyRegistrationToDIFException(e.what());
	}
#endif
}

void IPCProcess::notifyUnregistrationFromSupportingDIF(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const ApplicationProcessNamingInformation& difName)
throw (NotifyUnregistrationFromDIFException) {
	LOG_DBG("IPCProcess::notify unregistration from supporting DIF called");
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

	try{
		rinaManager->sendResponseOrNotficationMessage(&message);
	}catch(NetlinkException &e){
		throw NotifyUnregistrationFromDIFException(e.what());
	}
#endif
}

void IPCProcess::enroll(const ApplicationProcessNamingInformation& difName,
		const ApplicationProcessNamingInformation& supportinDifName)
throw (EnrollException) {
	LOG_DBG("IPCProcess::enroll called");
	throw IPCException(IPCException::operation_not_implemented_error);
}

void IPCProcess::disconnectFromNeighbor(
		const ApplicationProcessNamingInformation& neighbor)
throw (DisconnectFromNeighborException) {
	LOG_DBG("IPCProcess::disconnect from neighbour called");
	throw IPCException(IPCException::operation_not_implemented_error);
}

void IPCProcess::registerApplication(
		const ApplicationProcessNamingInformation& applicationName)
throw (IpcmRegisterApplicationException) {
	LOG_DBG("IPCProcess::register application called");
	if (!difMember){
		throw IPCException(IPCProcess::error_not_a_dif_member);
	}
#if STUB_API
	//Do nothing
#else
	IpcmRegisterApplicationRequestMessage message;
	message.setApplicationName(applicationName);
	message.setDifName(difInformation.getDifName());
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);

	IpcmRegisterApplicationResponseMessage * registerAppResponse;
	try{
		registerAppResponse =
				dynamic_cast<IpcmRegisterApplicationResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_registering_app));
	}catch(NetlinkException &e){
		throw IpcmRegisterApplicationException(e.what());
	}

	if (registerAppResponse->getResult() < 0){
		delete registerAppResponse;
		throw IpcmRegisterApplicationException(
				IPCProcess::error_registering_app);
	}

	LOG_DBG("Registered app %s to DIF %s",
			applicationName.getProcessName().c_str(),
			difInformation.getDifName().getProcessName().c_str());
	delete registerAppResponse;
#endif
}

void IPCProcess::unregisterApplication(
		const ApplicationProcessNamingInformation& applicationName)
throw (IpcmUnregisterApplicationException) {
	LOG_DBG("IPCProcess::unregister application called");
#if STUB_API
	//Do nothing
#else
	IpcmUnregisterApplicationRequestMessage message;
	message.setApplicationName(applicationName);
	message.setDifName(difInformation.getDifName());
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);

	IpcmUnregisterApplicationResponseMessage * unregisterAppResponse;
	try{
		unregisterAppResponse =
				dynamic_cast<IpcmUnregisterApplicationResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_unregistering_app));
	}catch(NetlinkException &e){
		throw IpcmUnregisterApplicationException(e.what());
	}

	if (unregisterAppResponse->getResult() < 0){
		delete unregisterAppResponse;
		throw IpcmUnregisterApplicationException(
				IPCProcess::error_unregistering_app);
	}
	LOG_DBG("Unregistered app %s from DIF %s",
			applicationName.getProcessName().c_str(),
			difInformation.getDifName().getProcessName().c_str());
	delete unregisterAppResponse;
#endif
}

void IPCProcess::allocateFlow(const FlowRequestEvent& flowRequest)
throw (AllocateFlowException) {
	LOG_DBG("IPCProcess::allocate flow called");
	if (!difMember){
		throw IPCException(IPCProcess::error_not_a_dif_member);
	}
#if STUB_API
	//Do nothing
#else
	IpcmAllocateFlowRequestMessage message;
	message.setSourceAppName(flowRequest.getLocalApplicationName());
	message.setDestAppName(flowRequest.getRemoteApplicationName());
	message.setFlowSpec(flowRequest.getFlowSpecification());
	message.setDifName(flowRequest.getDIFName());
	message.setPortId(flowRequest.getPortId());
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);

	IpcmAllocateFlowRequestResultMessage * allocateFlowResponse;
	try{
		allocateFlowResponse =
				dynamic_cast<IpcmAllocateFlowRequestResultMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_allocating_flow));
	}catch(NetlinkException &e){
		throw AllocateFlowException(e.what());
	}

	if (allocateFlowResponse->getResult() < 0){
		delete allocateFlowResponse;
		throw AllocateFlowException(
				IPCProcess::error_allocating_flow);
	}

	LOG_DBG("Allocated flow from %s to %s with portId %d",
			message.getSourceAppName().getProcessName().c_str(),
			message.getDestAppName().getProcessName().c_str(),
			message.getPortId());
	delete allocateFlowResponse;

#endif
}

void IPCProcess::allocateFlowResponse(const FlowRequestEvent& flowRequest,
		int result)
		throw(AllocateFlowException){
#if STUB_API
	//Do nothing
#else
	IpcmAllocateFlowResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setPortId(flowRequest.getPortId());
	//FIXME add parameter to API
	responseMessage.setNotifySource(true);
	responseMessage.setDestIpcProcessId(id);
	responseMessage.setDestPortId(portId);
	responseMessage.setSequenceNumber(flowRequest.getSequenceNumber());
	responseMessage.setResponseMessage(true);

	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw AllocateFlowException(e.what());
	}
#endif
}

void IPCProcess::deallocateFlow(int flowPortId)
	throw (IpcmDeallocateFlowException){
	LOG_DBG("IPCProcess::deallocate flow called");
#if STUB_API
	//Do nothing
#else
	IpcmDeallocateFlowRequestMessage message;
	message.setPortId(flowPortId);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);

	IpcmDeallocateFlowResponseMessage * deallocateFlowResponse;
	try{
		deallocateFlowResponse =
				dynamic_cast<IpcmDeallocateFlowResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_deallocating_flow));
	}catch(NetlinkException &e){
		throw IpcmDeallocateFlowException(e.what());
	}

	if (deallocateFlowResponse->getResult() < 0){
		delete deallocateFlowResponse;
		throw IpcmDeallocateFlowException(
				IPCProcess::error_deallocating_flow);
	}

	LOG_DBG("Deallocated flow from to with portId %d",
			message.getPortId());
	delete deallocateFlowResponse;

#endif
}

const std::list<RIBObject> IPCProcess::queryRIB(const std::string& objectClass,
		const std::string& objectName, unsigned long objectInstance,
		unsigned int scope, const std::string& filter)
			throw (QueryRIBException){
	LOG_DBG("IPCProcess::query RIB called");
#if STUB_API
	std::list<RIBObject> ribObjects;
	return ribObjects;
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

	IpcmDIFQueryRIBResponseMessage * queryRIBResponse;
	try{
		queryRIBResponse =
				dynamic_cast<IpcmDIFQueryRIBResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_querying_rib));
	}catch(NetlinkException &e){
		throw QueryRIBException(e.what());
	}

	if (queryRIBResponse->getResult() < 0){
		delete queryRIBResponse;
		throw QueryRIBException(
				IPCProcess::error_querying_rib);
	}

	LOG_DBG("Queried RIB of IPC Process %d; got %d objects",
			id, queryRIBResponse->getRIBObjects().size());
	std::list<RIBObject> ribObjects = queryRIBResponse->getRIBObjects();
	delete queryRIBResponse;
	return ribObjects;
#endif
}

/** CLASS IPC PROCESS FACTORY */
const std::string IPCProcessFactory::unknown_ipc_process_error =
		"Could not find an IPC Process with the provided id";
const std::string IPCProcessFactory::path_to_ipc_process_types =
		"/sys/rina/personalities/default/ipcp-factories/";
const std::string IPCProcessFactory::normal_ipc_process_type =
		"normal";

IPCProcessFactory::IPCProcessFactory(): Lockable(){
}

IPCProcessFactory::~IPCProcessFactory() throw(){
}

std::list<std::string> IPCProcessFactory::getSupportedIPCProcessTypes(){
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
		if (name.compare(".") != 0 && name.compare("..") != 0){
			result.push_back(name);
		}
	}

	closedir(dp);

	return result;
}

IPCProcess * IPCProcessFactory::create(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const std::string& difType) throw (CreateIPCProcessException) {
	lock();
	int ipcProcessId = 1;
	pid_t pid=0;
	for (int i = 1; i < 1000; i++) {
		if (ipcProcesses.find(i) == ipcProcesses.end()) {
			ipcProcessId = i;
			break;
		}
	}

#if STUB_API
	//Do nothing
#else
	int result = syscallCreateIPCProcess(
	                ipcProcessName, ipcProcessId, difType);
	if (result != 0)
	{
	        unlock();
	        throw CreateIPCProcessException();
	}

	if (difType.compare(NORMAL_IPC_PROCESS) == 0){
		pid = fork();
		if (pid == 0){
			//This is the OS process that has to execute the IPC Process
			//program and then exit
			LOG_DBG("New OS Process created, executing IPC Process ...");

			char * argv[] =
			{
                                /* FIXME: These hardwired things must disappear */
					stringToCharArray("/usr/bin/java"),
					stringToCharArray("-jar"),
					stringToCharArray(_installationPath +
					                "/ipcprocess/rina.ipcprocess.impl-1.0.0-irati-SNAPSHOT.jar"),
					stringToCharArray(ipcProcessName.getProcessName()),
					stringToCharArray(ipcProcessName.getProcessInstance()),
					intToCharArray(ipcProcessId),
					0
			};

			char * envp[] =
			{
                                /* FIXME: These hardwired things must disappear */
					stringToCharArray("PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"),
					stringToCharArray("LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"
					                +_libraryPath),
					(char*) 0
			};

			execve(argv[0], &argv[0], envp);

			LOG_ERR("Problems loading IPC Process program, finalizing OS Process");

			//Destroy the IPC Process in the kernel
			syscallDestroyIPCProcess(ipcProcessId);

			exit(-1);
		}else if (pid < 0){
			//This is the IPC Manager, and fork failed
		        //Try to destroy the IPC Process in the kernel and return error
		        syscallDestroyIPCProcess(ipcProcessId);

			unlock();
			throw CreateIPCProcessException();
		}else{
			//This is the IPC Manager, and fork was successful
			LOG_DBG("Craeted a new IPC Process with pid = %d", pid);
		}
	}
#endif

	IPCProcess * ipcProcess = new IPCProcess(ipcProcessId, 0, pid, difType,
			ipcProcessName);
	ipcProcesses[ipcProcessId] = ipcProcess;
	unlock();

	return ipcProcess;
}

void IPCProcessFactory::destroy(unsigned int ipcProcessId)
throw (DestroyIPCProcessException) {
	lock();

	int resultUserSpace = 0;
	int resultKernel = 0;
	std::map<int, IPCProcess*>::iterator iterator;
	iterator = ipcProcesses.find(ipcProcessId);
	if (iterator == ipcProcesses.end())
	{
		unlock();
		throw DestroyIPCProcessException(
		                IPCProcessFactory::unknown_ipc_process_error);
	}

#if STUB_API
	//Do nothing
#else
	IPCProcess * ipcProcess = iterator->second;

	resultKernel = syscallDestroyIPCProcess(ipcProcessId);

	if (ipcProcess->getType().compare(NORMAL_IPC_PROCESS) == 0)
		resultUserSpace = kill(ipcProcess->getPid(), SIGKILL);
#endif

	delete iterator->second;
	ipcProcesses.erase(ipcProcessId);

	unlock();

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

std::vector<IPCProcess *> IPCProcessFactory::listIPCProcesses() {
	std::vector<IPCProcess *> response;

	lock();
	for (std::map<int, IPCProcess*>::iterator it = ipcProcesses.begin();
			it != ipcProcesses.end(); ++it) {
		response.push_back(it->second);
	}
	unlock();

	return response;
}

IPCProcess * IPCProcessFactory::getIPCProcess(unsigned int ipcProcessId)
        throw (GetIPCProcessException)
{
        std::map<int, IPCProcess*>::iterator iterator;

        lock();
        iterator = ipcProcesses.find(ipcProcessId);
        unlock();

        if (iterator == ipcProcesses.end())
                throw GetIPCProcessException();

        return iterator->second;
}

Singleton<IPCProcessFactory> ipcProcessFactory;

/** CLASS APPLICATION MANAGER */

void ApplicationManager::applicationRegistered(
		const ApplicationRegistrationRequestEvent& event,
		const ApplicationProcessNamingInformation& difName, int result)
			throw (NotifyApplicationRegisteredException) {
	LOG_DBG("ApplicationManager::applicationRegistered called");

#if STUB_API
	//Do nothing
#else
	AppRegisterApplicationResponseMessage responseMessage;
	responseMessage.setApplicationName(event.getApplicationName());
	responseMessage.setDifName(difName);
	responseMessage.setResult(result);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw NotifyApplicationRegisteredException(e.what());
	}
#endif
}

void ApplicationManager::applicationUnregistered(
		const ApplicationUnregistrationRequestEvent& event,
		int result)
			throw (NotifyApplicationUnregisteredException) {
	LOG_DBG("ApplicationManager::applicationUnregistered called");

#if STUB_API
	//Do nothing
#else
	AppUnregisterApplicationResponseMessage responseMessage;
	responseMessage.setApplicationName(event.getApplicationName());
	responseMessage.setResult(result);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw NotifyApplicationUnregisteredException(e.what());
	}
#endif
}

void ApplicationManager::flowAllocated(const FlowRequestEvent& flowRequestEvent)
throw (NotifyFlowAllocatedException) {
	LOG_DBG("ApplicationManager::flowAllocated called");

#if STUB_API
	//Do nothing
#else
	AppAllocateFlowRequestResultMessage responseMessage;
	responseMessage.setPortId(flowRequestEvent.getPortId());
	responseMessage.setSourceAppName(flowRequestEvent.getLocalApplicationName());
	responseMessage.setDifName(flowRequestEvent.getDIFName());
	responseMessage.setSequenceNumber(flowRequestEvent.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw NotifyFlowAllocatedException(e.what());
	}
#endif
}

void ApplicationManager::flowRequestArrived(
			const ApplicationProcessNamingInformation& localAppName,
			const ApplicationProcessNamingInformation& remoteAppName,
			const FlowSpecification& flowSpec,
			const ApplicationProcessNamingInformation& difName,
			int portId) throw (AppFlowArrivedException){
	LOG_DBG("ApplicationManager::flowRequestArrived called");

#if STUB_API
	//Do nothing
#else
	AppAllocateFlowRequestArrivedMessage message;
	message.setSourceAppName(remoteAppName);
	message.setDestAppName(localAppName);
	message.setFlowSpecification(flowSpec);
	message.setDifName(difName);
	message.setPortId(portId);
	message.setRequestMessage(true);

	AppAllocateFlowResponseMessage * allocateFlowResponse;
	try{
		allocateFlowResponse =
				dynamic_cast<AppAllocateFlowResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_allocating_flow));
	}catch(NetlinkException &e){
		throw AppFlowArrivedException(e.what());
	}

	if (!(allocateFlowResponse->isAccept())){
		std::string reason = IPCProcess::error_allocating_flow + " " +
				allocateFlowResponse->getDenyReason();
		delete allocateFlowResponse;
		throw AppFlowArrivedException(reason);
	}

	LOG_DBG("Application %s accepted flow with portId %d",
			localAppName.getProcessName().c_str(), portId);
	delete allocateFlowResponse;
#endif
}

void ApplicationManager::flowDeallocated(
		const FlowDeallocateRequestEvent& event, int result)
		throw (NotifyFlowDeallocatedException){
	LOG_DBG("ApplicationManager::flowdeallocated called");

#if STUB_API
	//Do nothing
#else
	AppDeallocateFlowResponseMessage responseMessage;
	responseMessage.setApplicationName(event.getApplicationName());
	responseMessage.setResult(result);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw NotifyFlowDeallocatedException(e.what());
	}
#endif
}

void ApplicationManager::flowDeallocatedRemotely(
		int portId, int code,
		const ApplicationProcessNamingInformation& appName)
	throw (NotifyFlowDeallocatedException){
	LOG_DBG("ApplicationManager::flowDeallocatedRemotely called");
#if STUB_API
	//Do nothing
#else
	AppFlowDeallocatedNotificationMessage message;
	message.setPortId(portId);
	message.setCode(code);
	message.setApplicationName(appName);
	message.setNotificationMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&message);
	}catch(NetlinkException &e){
		throw NotifyFlowDeallocatedException(e.what());
	}
#endif
}

void ApplicationManager::getDIFPropertiesResponse(
		const GetDIFPropertiesRequestEvent &event,
			int result, const std::list<DIFProperties>& difProperties)
			throw (GetDIFPropertiesResponseException){
#if STUB_API
	//Do nothing
#else
	AppGetDIFPropertiesResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setApplicationName(event.getApplicationName());
	responseMessage.setDIFProperties(difProperties);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
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
				IPCEvent(GET_DIF_PROPERTIES, sequenceNumber){
	this->applicationName = appName;
	this->DIFName = DIFName;
}

const ApplicationProcessNamingInformation&
	GetDIFPropertiesRequestEvent::getApplicationName() const{
	return applicationName;
}
const ApplicationProcessNamingInformation&
	GetDIFPropertiesRequestEvent::getDIFName() const{
	return DIFName;
}

}
