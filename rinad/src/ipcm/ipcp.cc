/*
 * IPC Manager - IPCP related routine handlers
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Marc Sune             <marc.sune (at) bisdn.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
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
#include "common/debug.h"
#include "ipcp.h"

using namespace std;


namespace rinad {

//
// IPCM IPCP process class
//

IPCMIPCProcess::IPCMIPCProcess() {
	proxy_ = NULL;
	state_ = IPCM_IPCP_CREATED;
	kernel_ready = false;
}

IPCMIPCProcess::~IPCMIPCProcess() throw(){

}

IPCMIPCProcess::IPCMIPCProcess(rina::IPCProcessProxy* ipcp_proxy)
{
	state_ = IPCM_IPCP_CREATED;
	proxy_ = ipcp_proxy;
	kernel_ready = false;
}

/** Return the information of a registration request */
rina::ApplicationRegistrationInformation
IPCMIPCProcess::getPendingRegistration(unsigned int seqNumber)
{
        std::map<unsigned int,
                 rina::ApplicationRegistrationInformation>::iterator iterator;

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

rina::ApplicationProcessNamingInformation
IPCMIPCProcess::getPendingDisconnection(unsigned int seqNumber)
{
        std::map<unsigned int,
                 rina::ApplicationProcessNamingInformation>::iterator iterator;

        iterator = pendingDisconnections.find(seqNumber);
        if (iterator == pendingDisconnections.end()) {
                throw rina::IPCException("Could not find pending disconnection");
        }

        return iterator->second;
}

const rina::ApplicationProcessNamingInformation& IPCMIPCProcess::getDIFName() const
{ return dif_name_; }

std::list<rina::ApplicationProcessNamingInformation>
IPCMIPCProcess::get_neighbors_with_n1dif(const rina::ApplicationProcessNamingInformation& dif_name)
{
	std::list<rina::Neighbor>::iterator it;
	std::list<rina::ApplicationProcessNamingInformation> result;

	rina::ReadScopedLock readlock(rwlock);

	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if (it->supporting_dif_name_.processName == dif_name.processName)
			result.push_back(it->name_);
	}

	return result;
}

void IPCMIPCProcess::get_description(std::ostream& os) {
	os << "    " << get_id() << " | " <<
				get_name().toString() << " | " <<
				get_type() << " | ";
	rina::ReadScopedLock readlock(rwlock);
	switch (state_) {
	case IPCM_IPCP_CREATED:
		os << "CREATED";
		break;
	case IPCM_IPCP_INITIALIZED:
		os << "INITIALIZED";
		break;
	case IPCM_IPCP_ASSIGN_TO_DIF_IN_PROGRESS:
		os << "ASSIGN TO DIF IN PROGRESS";
		break;
	case IPCM_IPCP_ASSIGNED_TO_DIF:
		os << "ASSIGNED TO DIF " << dif_name_.processName;
		break;
	default:
		os << "UNKNOWN STATE";
	}

	os << " | ";
	if (registeredApplications.size() > 0) {
		std::list<rina::ApplicationRegistrationInformation>::const_iterator it;
		for (it = registeredApplications.begin();
				it != registeredApplications.end(); ++it) {
			if (it != registeredApplications.begin()) {
				os << ", ";
			}
			os << it->appName.getEncodedString();
		}
	} else {
		os << "-";
	}

	os << " | ";
	if (allocatedFlows.size () > 0) {
		std::list<rina::FlowInformation>::const_iterator it;
		for (it = allocatedFlows.begin();
				it != allocatedFlows.end(); ++it) {
			if (it != allocatedFlows.begin()) {
				os << ", ";
			}
			os << it->portId;
		}
	} else {
		os << "-";
	}

	os << "\n";
}

void IPCMIPCProcess::setInitialized()
{
	state_ = IPCM_IPCP_INITIALIZED;
}

unsigned int IPCMIPCProcess::get_fa_ctrl_port(const rina::ApplicationProcessNamingInformation& reg_app)
{
	std::list<rina::ApplicationRegistrationInformation>::iterator it;

	for(it = registeredApplications.begin();
			it != registeredApplications.end(); ++it) {
		if (it->appName == reg_app) {
			return it->ctrl_port;
		}
	}

	return 0;
}

void IPCMIPCProcess::assignToDIF(const rina::DIFInformation& difInformation,
		unsigned int opaque)
{
	if (state_ != IPCM_IPCP_INITIALIZED) {
		throw rina::AssignToDIFException(
				"IPC Process not yet initialized");
	}

	state_ = IPCM_IPCP_ASSIGN_TO_DIF_IN_PROGRESS;
	dif_name_ = difInformation.dif_name_;

	try {
        	proxy_->assignToDIF(difInformation, opaque);
	}catch (rina::Exception &e){
		rina::WriteScopedLock writelock(rwlock);
		state_ = IPCM_IPCP_INITIALIZED;
		throw e;
	}
}

void IPCMIPCProcess::assignToDIFResult(bool success)
{
	if (state_ != IPCM_IPCP_ASSIGN_TO_DIF_IN_PROGRESS)
		throw rina::AssignToDIFException("IPC Process in bad state");

	if (!success)
		state_ = IPCM_IPCP_INITIALIZED;
	else
		state_ = IPCM_IPCP_ASSIGNED_TO_DIF;
}

void
IPCMIPCProcess::updateDIFConfiguration(const rina::DIFConfiguration& difConfiguration,
		unsigned int opaque)
{
	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF) {
		std::string message;
		message =  message + "This IPC Process is not yet assigned "+
				"to any DIF, or a DIF configuration";
		LOG_ERR("%s", message.c_str());
		throw rina::UpdateDIFConfigurationException(message);
	}

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

void IPCMIPCProcess::enroll_prepare_handover(const rina::ApplicationProcessNamingInformation& difName,
			     	  	     const rina::ApplicationProcessNamingInformation& supportingDifName,
					     const rina::ApplicationProcessNamingInformation& neighborName,
					     const rina::ApplicationProcessNamingInformation& disc_neigh_name,
					     unsigned int opaque)
{
	proxy_->enroll_prepare_hand(difName, supportingDifName, neighborName,
				    disc_neigh_name, opaque);
}

void IPCMIPCProcess::disconnectFromNeighbor(const rina::ApplicationProcessNamingInformation& neighbor,
					    unsigned int opaque)
{
	proxy_->disconnectFromNeighbor(neighbor, opaque);
	pendingDisconnections[opaque] = neighbor;
}

void IPCMIPCProcess::registerApplication(const rina::ApplicationRegistrationInformation & ari,
					 unsigned int opaque)
{
	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF)
		throw rina::IpcmRegisterApplicationException(
				rina::IPCProcessProxy::error_not_a_dif_member);

	try {
		proxy_->registerApplication(ari.appName,
					    ari.dafName,
					    ari.ipcProcessId,
					    dif_name_,
					    opaque);
	}catch (rina::Exception &e) {
		throw e;
	}

	pendingRegistrations[opaque] = ari;
}

void IPCMIPCProcess::registerApplicationResult(unsigned int sequenceNumber,
					       bool success)
{
	rina::ApplicationRegistrationInformation app_reg;

	try {
		app_reg = getPendingRegistration(sequenceNumber);
	} catch(rina::IPCException &e) {
		throw rina::IpcmRegisterApplicationException(e.what());
	}

	pendingRegistrations.erase(sequenceNumber);
	if (success)
		registeredApplications.push_back(app_reg);
}

void IPCMIPCProcess::disconnectFromNeighborResult(unsigned int sequenceNumber,
					          bool success)
{
	rina::ApplicationProcessNamingInformation neigh_name;
	std::list<rina::Neighbor>::iterator it;

	neigh_name = getPendingDisconnection(sequenceNumber);

	pendingDisconnections.erase(sequenceNumber);
	if (success) {
		for (it = neighbors.begin(); it != neighbors.end(); ++it) {
			LOG_DBG("Neighbor name: %s", it->name_.processName.c_str());
			if (it->name_.processName == neigh_name.processName) {
				neighbors.erase(it);
				return;
			}
		}

	}
}

void IPCMIPCProcess::add_neighbors(const std::list<rina::Neighbor> & new_neighs)
{
	std::list<rina::Neighbor>::const_iterator it;
	std::list<rina::Neighbor>::iterator jt;
	bool add = true;

	for (it = new_neighs.begin(); it != new_neighs.end(); ++it) {
		add = true;
		for (jt = neighbors.begin(); jt != neighbors.end(); ++jt) {
			if (it->name_.processName == jt->name_.processName) {
				add = false;
				break;
			}
		}

		if (add) {
			LOG_DBG("Adding neighbor %s", it->name_.processName.c_str());
			neighbors.push_back(*it);
		}
	}
}

void IPCMIPCProcess::unregisterApplication(const rina::ApplicationProcessNamingInformation & app_name,
					   unsigned int opaque)
{
	rina::ApplicationRegistrationInformation ari;
	ari.appName = app_name;

	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF)
		throw rina::IpcmRegisterApplicationException(
				rina::IPCProcessProxy::error_not_a_dif_member);

	try {
		proxy_->unregisterApplication(app_name, dif_name_, opaque);
	}catch (rina::Exception &e) {
		throw e;
	}

	pendingRegistrations[opaque] = ari;
}

void IPCMIPCProcess::unregisterApplicationResult(unsigned int sequenceNumber,
						 bool success)
{
	rina::ApplicationRegistrationInformation app_reg;
	std::list<rina::ApplicationRegistrationInformation>::iterator it;

	try {
		app_reg = getPendingRegistration(sequenceNumber);
	} catch(rina::IPCException &e) {
		throw rina::IpcmRegisterApplicationException(e.what());
	}

	pendingRegistrations.erase(sequenceNumber);

	if (!success)
		return;
	for (it = registeredApplications.begin(); it != registeredApplications.end(); ++it) {
		if (it->appName == app_reg.appName) {
			registeredApplications.erase(it);
			return;
		}
	}
}

void IPCMIPCProcess::allocateFlow(const rina::FlowRequestEvent& flowRequest,
		unsigned int opaque)
{
	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF)
		throw rina::IpcmRegisterApplicationException(
				rina::IPCProcessProxy::error_not_a_dif_member);

	try {
		proxy_->allocateFlow(flowRequest, opaque);
	} catch (rina::Exception &e) {
		throw e;
	}

	rina::FlowInformation flowInformation;
	flowInformation.localAppName = flowRequest.localApplicationName;
	flowInformation.remoteAppName = flowRequest.remoteApplicationName;
	flowInformation.difName = dif_name_;
	flowInformation.flowSpecification = flowRequest.flowSpecification;
	flowInformation.portId = flowRequest.portId;
	flowInformation.pid = flowRequest.pid;
	pendingFlowOperations[opaque] = flowInformation;
}

void IPCMIPCProcess::allocateFlowResult(unsigned int sequenceNumber,
					bool success, int portId)
{
	rina::FlowInformation flowInformation;
	try {
		flowInformation = getPendingFlowOperation(sequenceNumber);
		flowInformation.portId = portId;
	} catch(rina::IPCException &e) {
		throw rina::AllocateFlowException(e.what());
	}

	pendingFlowOperations.erase(sequenceNumber);
	if (success)
		allocatedFlows.push_back(flowInformation);
}

void IPCMIPCProcess::allocateFlowResponse(const rina::FlowRequestEvent& flowRequest,
					  int result, pid_t pid,
					  bool notifySource,
					  int flowAcceptorIpcProcessId)
{
	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF)
		throw rina::IpcmRegisterApplicationException(
				rina::IPCProcessProxy::error_not_a_dif_member);

	try{
		proxy_->allocateFlowResponse(flowRequest,
					     result,
					     notifySource,
					     flowAcceptorIpcProcessId);
	} catch(rina::Exception &e){
		throw e;
	}

	if (result == 0) {
		rina::FlowInformation flowInformation;
		flowInformation.localAppName = flowRequest.localApplicationName;
		flowInformation.remoteAppName = flowRequest.remoteApplicationName;
		flowInformation.difName = dif_name_;
		flowInformation.flowSpecification = flowRequest.flowSpecification;
		flowInformation.portId = flowRequest.portId;
		flowInformation.pid = pid;

		allocatedFlows.push_back(flowInformation);
	}
}

bool IPCMIPCProcess::getFlowInformation(int flowPortId, rina::FlowInformation& result) {

	std::list<rina::FlowInformation>::const_iterator iterator;

	for (iterator = allocatedFlows.begin();
			iterator != allocatedFlows.end(); ++iterator) {
                if (iterator->portId == flowPortId) {
                        result = *iterator;
                        return true;
                }
	}
	return false;
}

void IPCMIPCProcess::deallocateFlow(int flowPortId, unsigned int opaque)
{
	rina::FlowInformation flowInformation;

	if (state_ != IPCM_IPCP_ASSIGNED_TO_DIF)
		throw rina::IPCException(rina::IPCProcessProxy::error_not_a_dif_member);

	if (!getFlowInformation(flowPortId, flowInformation)) {
		throw rina::IPCException("No flow for such port-id");
	}

	try {
		proxy_->deallocateFlow(flowPortId, opaque);
	} catch (rina::Exception &e) {
		throw e;
	}

	allocatedFlows.remove(flowInformation);
}

rina::FlowInformation IPCMIPCProcess::flowDeallocated(int flowPortId)
{
	rina::FlowInformation flowInformation;

	if (!getFlowInformation(flowPortId, flowInformation))
		throw rina::IpcmDeallocateFlowException(
						"No flow for such port-id");
	allocatedFlows.remove(flowInformation);

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

void IPCMIPCProcess::forwardCDAPMessage(const rina::cdap::CDAPMessage& msg,
					unsigned int opaque)
{
	rina::ser_obj_t encoded_msg;
	rina::cdap_rib::concrete_syntax_t syntax;
	rina::cdap::CDAPMessageEncoder encoder(syntax);

	encoder.encode(msg, encoded_msg);

	proxy_->forwardCDAPRequestMessage(encoded_msg,
				   opaque);
}

//
// IPCM IPC process factory
//

std::list<std::string> IPCMIPCProcessFactory::getSupportedIPCProcessTypes() {
	return proxy_factory_.getSupportedIPCProcessTypes();
}

IPCMIPCProcess * IPCMIPCProcessFactory::create(
		const rina::ApplicationProcessNamingInformation& ipcProcessName,
		const std::string& difType) {
	rina::IPCProcessProxy * ipcp_proxy = 0;
	IPCMIPCProcess * ipcp = 0;
	unsigned short id;

	rina::WriteScopedLock writelock(rwlock);

	//Try to maximize reusage
	for (id = 1; id < 0xFFFF; ++id) {
		if (ipcProcesses.find(id) == ipcProcesses.end())
			break;
	}
	try {
		ipcp_proxy = proxy_factory_.create(ipcProcessName, difType,
									  id);
	} catch (rina::Exception & e){
		throw e;
	}

	ipcp = new IPCMIPCProcess(ipcp_proxy);

	//Acquire lock to prevent any race condition
	ipcp->rwlock.writelock();

	//Now add to the list of IPCP processes
	ipcProcesses[id] = ipcp;

	return ipcp;
}

unsigned int IPCMIPCProcessFactory::destroy(unsigned short ipcProcessId) {

	std::map<unsigned short, IPCMIPCProcess*>::iterator iterator;
	IPCMIPCProcess * ipcp;
	rina::IPCProcessProxy * ipcp_proxy;
	unsigned int result = 0;

	rina::WriteScopedLock writelock(rwlock);

	iterator = ipcProcesses.find(ipcProcessId);
	if (iterator == ipcProcesses.end())
	{
		throw rina::DestroyIPCProcessException(
				rina::IPCProcessFactory::unknown_ipc_process_error);
	}

	//Recover IPCP
	ipcp = iterator->second;

	try {
		if(ipcp->proxy_)
			result = proxy_factory_.destroy(ipcp->proxy_);
	}catch (rina::Exception &e) {
		assert(0);
	}
	delete ipcp;
	ipcProcesses.erase(ipcProcessId);

	return result;
}

bool IPCMIPCProcessFactory::exists(const unsigned short id)
{
	std::map<unsigned short, IPCMIPCProcess*>::iterator iterator;
	rina::ReadScopedLock readlock(rwlock);

	iterator = ipcProcesses.find(id);
	if (iterator == ipcProcesses.end())
		return false;

	return true;
}

unsigned short IPCMIPCProcessFactory::exists_by_pid(pid_t pid)
{
	std::map<unsigned short, IPCMIPCProcess*>::iterator it;
	rina::ReadScopedLock readlock(rwlock);

	for (it = ipcProcesses.begin(); it != ipcProcesses.end(); ++it) {
		if (it->second->proxy_->pid == pid) {
			return it->first;
		}
	}

	return 0;
}

void IPCMIPCProcessFactory::listIPCProcesses(std::vector<IPCMIPCProcess *>& result)
{
	rina::ReadScopedLock readlock(rwlock);

	for (std::map<unsigned short, IPCMIPCProcess*>::iterator it = ipcProcesses.begin();
			it != ipcProcesses.end(); ++it) {
		result.push_back(it->second);
	};
}

IPCMIPCProcess * IPCMIPCProcessFactory::getIPCProcess(unsigned short ipcProcessId)
{
        std::map<unsigned short, IPCMIPCProcess*>::iterator iterator;

	rina::ReadScopedLock readlock(rwlock);

        iterator = ipcProcesses.find(ipcProcessId);

        if (iterator == ipcProcesses.end())
                return NULL;

        return iterator->second;
}

IPCMIPCProcess * IPCMIPCProcessFactory::getIPCProcessByName(const rina::ApplicationProcessNamingInformation& ipcp_name)
{
	std::map<unsigned short, IPCMIPCProcess*>::iterator iterator;

	rina::ReadScopedLock readlock(rwlock);

	for (iterator = ipcProcesses.begin();
			iterator != ipcProcesses.end(); ++iterator) {
		if (iterator->second->get_name().processName == ipcp_name.processName) {
			return iterator->second;
		}
	}

	return NULL;
}

void IPCMIPCProcessFactory::get_local_dif_names(std::list<std::string>& result)
{
	std::map<unsigned short, IPCMIPCProcess*>::iterator it;
	rina::ReadScopedLock readlock(rwlock);

	for (it = ipcProcesses.begin();
			it != ipcProcesses.end(); ++it){
		result.push_back(it->second->dif_name_.processName);
	}

}


} //namespace rinad
