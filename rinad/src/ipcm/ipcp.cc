/*
 * IPC Manager - IPCP related routine handlers
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *
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

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/ipc-manager.h>

#define RINA_PREFIX "ipcm.ipcp"
#include <librina/logs.h>

#include "rina-configuration.h"
#include "ipcp.h"

using namespace std;


namespace rinad {


//
// IPCM IPCP process class
//

IPCMIPCProcess::IPCMIPCProcess() {
	proxy_ = NULL;
	state_ = IPCM_IPCP_CREATED;
}

IPCMIPCProcess::~IPCMIPCProcess() throw(){

}

IPCMIPCProcess::IPCMIPCProcess(rina::IPCProcessProxy* ipcp_proxy)
{
	state_ = IPCM_IPCP_CREATED;
	proxy_ = ipcp_proxy;
}

/** Return the information of a registration request */
rina::ApplicationProcessNamingInformation
IPCMIPCProcess::getPendingRegistration(unsigned int seqNumber)
{
        std::map<unsigned int,
                 rina::ApplicationProcessNamingInformation>::iterator iterator;

        iterator = pendingRegistrations.find(seqNumber);
        if (iterator == pendingRegistrations.end()) {
                throw rina::IPCException("Could not find pending registration");
        }

        return iterator->second;
}

rina::FlowInformation IPCMIPCProcess::getPendingFlowOperation(unsigned int seqNumber)
{
	std::map<unsigned int, rina::FlowInformation>::iterator iterator;

	iterator = pendingFlowOperations.find(seqNumber);
	if (iterator == pendingFlowOperations.end()) {
		throw rina::IPCException("Could not find pending flow operation");
	}

	return iterator->second;
}

const rina::ApplicationProcessNamingInformation& IPCMIPCProcess::getDIFName() const
{ return dif_name_; }

void IPCMIPCProcess::setInitialized()
{
	lock();
	state_ = IPCM_IPCP_INITIALIZED;
	unlock();
}

void IPCMIPCProcess::assignToDIF(const rina::DIFInformation& difInformation,
		unsigned int opaque)
{
	lock();
	if (state_ != IPCM_IPCP_INITIALIZED) {
		unlock();
		throw rina::AssignToDIFException("IPC Process not yet initialized");
	}

	state_ = IPCM_IPCP_ASSIGN_TO_DIF_IN_PROGRESS;
	dif_name_ = difInformation.dif_name_;
	unlock();

	try {
        proxy_->assignToDIF(difInformation, opaque);
	}catch (Exception &e){
		lock();
		state_ = IPCM_IPCP_INITIALIZED;
		unlock();
		throw e;
	}
}

void IPCMIPCProcess::assignToDIFResult(bool success)
{
	lock();
	if (state_ != IPCM_IPCP_ASSIGN_TO_DIF_IN_PROGRESS) {
		unlock();
		throw rina::AssignToDIFException("IPC Process in bad state");
	}

	if (!success) {
		state_ = IPCM_IPCP_INITIALIZED;
	}else {
		state_ = IPCM_IPCP_ASSIGNED_TO_DIF;
	}
	unlock();
}

void
IPCMIPCProcess::updateDIFConfiguration(const rina::DIFConfiguration& difConfiguration,
		unsigned int opaque)
{
	lock();
	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF) {
		unlock();
		std::string message;
		message =  message + "This IPC Process is not yet assigned "+
				"to any DIF, or a DIF configuration";
		LOG_ERR("%s", message.c_str());
		throw rina::UpdateDIFConfigurationException(message);
	}

	unlock();
    proxy_->updateDIFConfiguration(difConfiguration, opaque);
}

void IPCMIPCProcess::notifyRegistrationToSupportingDIF(
		const rina::ApplicationProcessNamingInformation& ipcProcessName,
		const rina::ApplicationProcessNamingInformation& difName)
{
	proxy_->notifyRegistrationToSupportingDIF(ipcProcessName, difName);
}

void IPCMIPCProcess::notifyUnregistrationFromSupportingDIF(
		const rina::ApplicationProcessNamingInformation& ipcProcessName,
		const rina::ApplicationProcessNamingInformation& difName)
{
	proxy_->notifyUnregistrationFromSupportingDIF(ipcProcessName, difName);
}

void IPCMIPCProcess::enroll(
        const rina::ApplicationProcessNamingInformation& difName,
        const rina::ApplicationProcessNamingInformation& supportingDifName,
        const rina::ApplicationProcessNamingInformation& neighborName,
        unsigned int opaque)
{
	proxy_->enroll(difName, supportingDifName, neighborName, opaque);
}

void IPCMIPCProcess::disconnectFromNeighbor(
		const rina::ApplicationProcessNamingInformation& neighbor)
{
	proxy_->disconnectFromNeighbor(neighbor);
}

void IPCMIPCProcess::registerApplication(
		const rina::ApplicationProcessNamingInformation& applicationName,
		unsigned short regIpcProcessId,
		unsigned int opaque)
{
	lock();
	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF) {
		unlock();
		throw rina::IpcmRegisterApplicationException(
				rina::IPCProcessProxy::error_not_a_dif_member);
	}

	try {
		proxy_->registerApplication(applicationName, regIpcProcessId,
				dif_name_, opaque);
	}catch (Exception &e) {
		unlock();
		throw e;
	}

	pendingRegistrations[opaque] = applicationName;
	unlock();
}

void IPCMIPCProcess::registerApplicationResult(unsigned int sequenceNumber,
		bool success) {
	lock();
	rina::ApplicationProcessNamingInformation appName;
	try {
		appName = getPendingRegistration(sequenceNumber);
	} catch(rina::IPCException &e) {
		unlock();
		throw rina::IpcmRegisterApplicationException(e.what());
	}

	pendingRegistrations.erase(sequenceNumber);
	if (success) {
		registeredApplications.push_back(appName);
	}

	unlock();
}

void IPCMIPCProcess::unregisterApplication(
		const rina::ApplicationProcessNamingInformation& applicationName,
		unsigned int opaque) {
	lock();
	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF) {
		unlock();
		throw rina::IpcmRegisterApplicationException(
				rina::IPCProcessProxy::error_not_a_dif_member);
	}

	try {
		proxy_->unregisterApplication(applicationName, dif_name_, opaque);
	}catch (Exception &e) {
		unlock();
		throw e;
	}

	pendingRegistrations[opaque] = applicationName;
	unlock();
}

void IPCMIPCProcess::unregisterApplicationResult(unsigned int sequenceNumber,
		bool success)
{
	lock();

	rina::ApplicationProcessNamingInformation appName;
	try {
		appName = getPendingRegistration(sequenceNumber);
	} catch(rina::IPCException &e) {
		unlock();
		throw rina::IpcmRegisterApplicationException(e.what());
	}

	pendingRegistrations.erase(sequenceNumber);
	if (success) {
		registeredApplications.remove(appName);
	}

	unlock();
}

void IPCMIPCProcess::allocateFlow(const rina::FlowRequestEvent& flowRequest,
		unsigned int opaque)
{
	lock();
	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF) {
		unlock();
		throw rina::IpcmRegisterApplicationException(
				rina::IPCProcessProxy::error_not_a_dif_member);
	}

	try {
		proxy_->allocateFlow(flowRequest, opaque);
	} catch (Exception &e) {
		unlock();
		throw e;
	}

	rina::FlowInformation flowInformation;
	flowInformation.localAppName = flowRequest.localApplicationName;
	flowInformation.remoteAppName = flowRequest.remoteApplicationName;
	flowInformation.difName = dif_name_;
	flowInformation.flowSpecification = flowRequest.flowSpecification;
	flowInformation.portId = flowRequest.portId;
	pendingFlowOperations[opaque] = flowInformation;

	unlock();
}

void IPCMIPCProcess::allocateFlowResult(
                unsigned int sequenceNumber, bool success, int portId)
{
	lock();

	rina::FlowInformation flowInformation;
	try {
		flowInformation = getPendingFlowOperation(sequenceNumber);
		flowInformation.portId = portId;
	} catch(rina::IPCException &e) {
		unlock();
		throw rina::AllocateFlowException(e.what());
	}

	pendingFlowOperations.erase(sequenceNumber);
	if (success) {
		allocatedFlows.push_back(flowInformation);
	}

	unlock();
}

void IPCMIPCProcess::allocateFlowResponse(const rina::FlowRequestEvent& flowRequest,
		int result, bool notifySource, int flowAcceptorIpcProcessId)
{
	lock();
	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF) {
		unlock();
		throw rina::IpcmRegisterApplicationException(
				rina::IPCProcessProxy::error_not_a_dif_member);
	}

	try{
		proxy_->allocateFlowResponse(flowRequest, result,
				notifySource, flowAcceptorIpcProcessId);
	} catch(Exception &e){
		unlock();
		throw e;
	}

	if (result == 0) {
		rina::FlowInformation flowInformation;
		flowInformation.localAppName = flowRequest.localApplicationName;
		flowInformation.remoteAppName = flowRequest.remoteApplicationName;
		flowInformation.difName = dif_name_;
		flowInformation.flowSpecification = flowRequest.flowSpecification;
		flowInformation.portId = flowRequest.portId;

		allocatedFlows.push_back(flowInformation);
	}

	unlock();
}

bool IPCMIPCProcess::getFlowInformation(int flowPortId, rina::FlowInformation& result) {
	std::list<rina::FlowInformation>::const_iterator iterator;

	lock();
	for (iterator = allocatedFlows.begin();
			iterator != allocatedFlows.end(); ++iterator) {
                if (iterator->portId == flowPortId) {
                        result = *iterator;
                        unlock();
                        return true;
                }
	}

	unlock();
    return false;
}

void IPCMIPCProcess::deallocateFlow(int flowPortId, unsigned int opaque)
{
	rina::FlowInformation flowInformation;
	bool success;

	lock();
	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF) {
		unlock();
		throw rina::IpcmRegisterApplicationException(
				rina::IPCProcessProxy::error_not_a_dif_member);
	}

	try {
		proxy_->deallocateFlow(flowPortId, opaque);
	} catch (Exception &e) {
		unlock();
		throw e;
	}

	success = getFlowInformation(flowPortId, flowInformation);
	if (success) {
		pendingFlowOperations[opaque] = flowInformation;
	}

	unlock();
}

void IPCMIPCProcess::deallocateFlowResult(unsigned int sequenceNumber, bool success)
{
	rina::FlowInformation flowInformation;
	lock();

	try {
		flowInformation = getPendingFlowOperation(sequenceNumber);
	} catch(rina::IPCException &e) {
		unlock();
		throw rina::IpcmDeallocateFlowException(e.what());
	}

	pendingFlowOperations.erase(sequenceNumber);
	if (success) {
		allocatedFlows.remove(flowInformation);
	}

	unlock();
}

rina::FlowInformation IPCMIPCProcess::flowDeallocated(int flowPortId)
{
	rina::FlowInformation flowInformation;
	bool success;

	lock();

	success = getFlowInformation(flowPortId, flowInformation);
	if (!success) {
		unlock();
		throw rina::IpcmDeallocateFlowException("No flow for such port-id");
	}
	allocatedFlows.remove(flowInformation);

	unlock();

	return flowInformation;
}

void IPCMIPCProcess::queryRIB(const std::string& objectClass,
		const std::string& objectName, unsigned long objectInstance,
		unsigned int scope, const std::string& filter,
		unsigned int opaque)
{
	proxy_->queryRIB(objectClass, objectName, objectInstance, scope, filter, opaque);
}

void IPCMIPCProcess::setPolicySetParam(const std::string& path,
                        const std::string& name, const std::string& value,
                        unsigned int opaque)
{
	proxy_->setPolicySetParam(path, name, value, opaque);
}

void IPCMIPCProcess::selectPolicySet(const std::string& path,
                                 const std::string& name,
                                 unsigned int opaque)
{
	proxy_->selectPolicySet(path, name, opaque);
}

void IPCMIPCProcess::pluginLoad(const std::string& name, bool load,
		unsigned int opaque)
{
	proxy_->pluginLoad(name, load, opaque);
}


//
// IPCM IPC process factory
//

IPCMIPCProcessFactory::IPCMIPCProcessFactory(){
}

IPCMIPCProcessFactory::~IPCMIPCProcessFactory() throw(){
}

std::list<std::string> IPCMIPCProcessFactory::getSupportedIPCProcessTypes() {
	return proxy_factory_.getSupportedIPCProcessTypes();
}

IPCMIPCProcess * IPCMIPCProcessFactory::create(
		const rina::ApplicationProcessNamingInformation& ipcProcessName,
		const std::string& difType) {
	rina::IPCProcessProxy * ipcp_proxy = 0;
	IPCMIPCProcess * ipcp = 0;
	unsigned short ipcProcessId = 1;

	lock();

	for (unsigned short i = 1; i < 1000; i++) {
		if (ipcProcesses.find(i) == ipcProcesses.end()) {
			ipcProcessId = i;
			break;
		}
	}

	try {
		ipcp_proxy = proxy_factory_.create(ipcProcessName,
										  difType,
										  ipcProcessId);
	} catch (Exception & e){
		unlock();
		throw e;
	}

	ipcp = new IPCMIPCProcess(ipcp_proxy);
	ipcProcesses[ipcProcessId] = ipcp;
	unlock();

	return ipcp;
}

void IPCMIPCProcessFactory::destroy(unsigned short ipcProcessId) {
	std::map<unsigned short, IPCMIPCProcess*>::iterator iterator;
	IPCMIPCProcess * ipcp = 0;
	rina::IPCProcessProxy * ipcp_proxy = 0;

	lock();
	iterator = ipcProcesses.find(ipcProcessId);
	if (iterator == ipcProcesses.end())
	{
		unlock();
		throw rina::DestroyIPCProcessException(
				rina::IPCProcessFactory::unknown_ipc_process_error);
	}

	ipcp = iterator->second;

	try {
		if(ipcp->proxy_)
			proxy_factory_.destroy(ipcp->proxy_);
	}catch (Exception &e) {
		assert(0);
	}
	delete ipcp;
	ipcProcesses.erase(ipcProcessId);


	unlock();
}

std::vector<IPCMIPCProcess *> IPCMIPCProcessFactory::listIPCProcesses()
{
	std::vector<IPCMIPCProcess *> response;

	lock();
	for (std::map<unsigned short, IPCMIPCProcess*>::iterator it = ipcProcesses.begin();
			it != ipcProcesses.end(); ++it) {
		response.push_back(it->second);
	}
	unlock();

	return response;
}

IPCMIPCProcess * IPCMIPCProcessFactory::getIPCProcess(unsigned short ipcProcessId)
{
        std::map<unsigned short, IPCMIPCProcess*>::iterator iterator;

        lock();
        iterator = ipcProcesses.find(ipcProcessId);
        unlock();

        if (iterator == ipcProcesses.end())
                return NULL;

        return iterator->second;
}

} //namespace rinad
