/*
 * IP VPN Manager. Manages creation and deletion of IP over RINA flows
 * and VPNs. Manages registration of IP prefixes as application names
 * in RINA DIFs, as well as allocation of flows between IP prefixes,
 * the creation of VPNs and the reconfiguration of the IP forwarding table.
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <stdio.h>

#define RINA_PREFIX     "ipcm.ip-vpn-manager"
#include <librina/logs.h>

#include "app-handlers.h"
#include "ip-vpn-manager.h"
#include "ipcm.h"

namespace rinad {

//Class IPVPNManager
IPVPNManager::IPVPNManager()
{
}

IPVPNManager::~IPVPNManager()
{
	std::map<int, rina::FlowRequestEvent>::iterator it;
	rina::FlowRequestEvent event;

	//Remove all IP over RINA routes and set RINA devs down
	for (it = iporina_flows.begin(); it != iporina_flows.end(); ++it) {
		event = it->second;
		add_or_remove_ip_route(event.remoteApplicationName.processName,
				       event.ipcProcessId, event.portId, false);
	}
}

int IPVPNManager::add_registered_ip_prefix(const std::string& ip_prefix)
{
	rina::ScopedLock g(lock);

	if (__ip_prefix_registered(ip_prefix)) {
		LOG_ERR("IP prefix %s is already registered",
			ip_prefix.c_str());
		return -1;
	}

	reg_ip_prefixes.push_back(ip_prefix);

	return 0;
}

int IPVPNManager::remove_registered_ip_prefix(const std::string& ip_prefix)
{
	std::list<std::string>::iterator it;

	rina::ScopedLock g(lock);

	for (it = reg_ip_prefixes.begin(); it != reg_ip_prefixes.end(); ++it) {
		if ((*it) == ip_prefix) {
			reg_ip_prefixes.erase(it);
			return 0;
		}
	}

	LOG_ERR("IP prefix %s is not registered", ip_prefix.c_str());

	return -1;
}

bool IPVPNManager::ip_prefix_registered(const std::string& ip_prefix)
{
	rina::ScopedLock g(lock);

	return __ip_prefix_registered(ip_prefix);
}

bool IPVPNManager::__ip_prefix_registered(const std::string& ip_prefix)
{
	std::list<std::string>::iterator it;

	for (it = reg_ip_prefixes.begin(); it != reg_ip_prefixes.end(); ++it) {
		if ((*it) == ip_prefix) {
			return true;
		}
	}

	return false;
}

int IPVPNManager::iporina_flow_allocated(const rina::FlowRequestEvent& event)
{
	int res = 0;

	rina::ScopedLock g(lock);

	res = add_flow(event);
	if (res != 0) {
		return res;
	}

	//Add entry to the IP forwarding table
	res = add_or_remove_ip_route(event.remoteApplicationName.processName,
			             event.ipcProcessId, event.portId, true);
	if (res != 0) {
		LOG_ERR("Problems adding route to IP routing table");
	}

	LOG_INFO("IP over RINA flow between %s and %s allocated, port-id: %d",
		  event.localApplicationName.processName.c_str(),
		  event.remoteApplicationName.processName.c_str(),
		  event.portId);

	return 0;
}

void IPVPNManager::
iporina_flow_allocation_requested(const rina::FlowRequestEvent& event)
{
	int res = 0;

	rina::ScopedLock g(lock);

	if (!__ip_prefix_registered(event.localApplicationName.processName)) {
		LOG_INFO("Rejected IP over RINA flow between %s and %s",
			  event.localApplicationName.processName.c_str(),
			  event.remoteApplicationName.processName.c_str());
		IPCManager->allocate_iporina_flow_response(event, -1, true);
		return;
	}

	res = add_flow(event);
	if (res != 0) {
		LOG_ERR("Flow with port-id %d already exists", event.portId);
		IPCManager->allocate_iporina_flow_response(event, -1, true);
		return;
	}

	//Add entry to the IP forwarding table
	res = add_or_remove_ip_route(event.remoteApplicationName.processName,
			             event.ipcProcessId, event.portId, true);
	if (res != 0) {
		LOG_ERR("Problems adding route to IP routing table");
	}

	LOG_INFO("Accepted IP over RINA flow between %s and %s; port-id: %d",
			event.localApplicationName.processName.c_str(),
			event.remoteApplicationName.processName.c_str(),
			event.portId);

	IPCManager->allocate_iporina_flow_response(event, 0, true);
}

int IPVPNManager::get_iporina_flow_info(int port_id,
					rina::FlowRequestEvent& event)
{
	int res = 0;
	std::map<int, rina::FlowRequestEvent>::iterator it;

	rina::ScopedLock g(lock);

	it = iporina_flows.find(port_id);
	if (it == iporina_flows.end()) {
		LOG_ERR("Could not find IP over RINA flow with port-id %d", port_id);
		return -1;
	}

	event = it->second;

	return 0;
}

int IPVPNManager::iporina_flow_deallocated(int port_id, const int ipcp_id)
{
	int res = 0;
	rina::FlowRequestEvent event;

	rina::ScopedLock g(lock);

	event.portId = port_id;
	res = remove_flow(event);
	if (res != 0) {
		return res;
	}

	//Remove entry from the IP forwarding table
	res = add_or_remove_ip_route(event.remoteApplicationName.processName,
			             ipcp_id, event.portId, false);
	if (res != 0) {
		LOG_ERR("Problems removing entry from IP routing table");
	}

	LOG_INFO("IP over RINA flow between %s and %s deallocated, port-id: %d",
		  event.localApplicationName.processName.c_str(),
		  event.remoteApplicationName.processName.c_str(),
		  event.portId);

	return 0;
}

int IPVPNManager::add_flow(const rina::FlowRequestEvent& event)
{
	std::map<int, rina::FlowRequestEvent>::iterator it;

	it = iporina_flows.find(event.portId);
	if (it != iporina_flows.end()) {
		LOG_ERR("Cannot add IP over RINA flow with port-id %d, already exists",
			 event.portId);
		return -1;
	}

	iporina_flows[event.portId] = event;

	return 0;
}

int IPVPNManager::remove_flow(rina::FlowRequestEvent& event)
{
	std::map<int, rina::FlowRequestEvent>::iterator it;

	it = iporina_flows.find(event.portId);
	if (it == iporina_flows.end()) {
		LOG_ERR("Cannot remove IP over RINA flow with port-id %d, not found",
			 event.portId);
		return -1;
	}

	event = it->second;
	iporina_flows.erase(it);

	return 0;
}

std::string IPVPNManager::exec_shell_command(std::string command)
{
	char buffer[128];
	std::string result = "";
	FILE * pipe = 0;

	LOG_DBG("Executing command '%s' ...", command.c_str());

	pipe = popen(command.c_str(), "r");
	if (!pipe) throw std::runtime_error("popen() failed!");
	try {
		while (!feof(pipe)) {
			if (fgets(buffer, 128, pipe) != NULL)
				result += buffer;
		}
	} catch (...) {
		pclose(pipe);
		throw;
	}
	pclose(pipe);

	return result;
}

std::string IPVPNManager::get_rina_dev_name(const int ipcp_id,
					    const int port_id)
{
	std::stringstream ss;

	ss << "rina." << ipcp_id << "." << port_id;
	return ss.str();
}

std::string IPVPNManager::get_ip_prefix_string(const std::string& input)
{
	std::string result = input;
	std::replace(result.begin(), result.end(), '|', '/'); // replace all '|' to '/'

	return result;
}

int IPVPNManager::add_or_remove_ip_route(const std::string ip_prefix,
					 const int ipcp_id,
					 const int port_id,
					 bool add)
{
	std::stringstream ss;
	std::string prefix;
	std::string suffix;
	std::string result;
	std::string dev_name;

	dev_name = get_rina_dev_name(ipcp_id, port_id);

	if (add) {
		prefix = "add ";
		suffix = " up";
	} else {
		prefix = "delete ";
		suffix = " down";
	}

	ss << "ifconfig " << dev_name << suffix;
	result = exec_shell_command(ss.str());
	//TODO parse result

	ss.str("");
	ss << "ip route " << prefix
	   << get_ip_prefix_string(ip_prefix) << " dev "
	   << dev_name;

	result = exec_shell_command(ss.str());
	//TODO parse result

	return 0;
}

// Class IPCManager
ipcm_res_t IPCManager_::register_ip_prefix_to_dif(Promise* promise,
						  const std::string& ip_range,
						  const rina::ApplicationProcessNamingInformation& dif_name)
{
	IPCMIPCProcess *slave_ipcp = NULL;
	APPregTransState* trans = NULL;
	rina::ApplicationRegistrationRequestEvent event;
	rina::ApplicationProcessNamingInformation daf_name;

	// Select an IPC process from the DIF specified in the request
	slave_ipcp = select_ipcp_by_dif(dif_name);
	if (!slave_ipcp) {
		LOG_ERR("Cannot find a suitable DIF to "
			"register IP range %s ", ip_range.c_str());
		return IPCM_FAILURE;
	}

	//Populate application name
	event.applicationRegistrationInformation.appName.processName = ip_range;
	event.applicationRegistrationInformation.appName.entityName = RINA_IP_FLOW_ENT_NAME;

	//Auto release the read lock
	rina::ReadScopedLock readlock(slave_ipcp->rwlock, false);

	//Perform the registration
	try {
		//Create a transaction
		trans = new APPregTransState(NULL, promise, slave_ipcp->get_id(), event);
		if(!trans){
			LOG_ERR("Unable to allocate memory for the transaction object. Out of memory! ");
			return IPCM_FAILURE;
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			LOG_ERR("Unable to add transaction; out of memory? ");
			return IPCM_FAILURE;
		}

		slave_ipcp->registerApplication(event.applicationRegistrationInformation.appName,
						daf_name,
						0,
						trans->tid);

		LOG_INFO("Requested registration of IP prefix %s"
				" to IPC process %s ",
				ip_range.c_str(),
				slave_ipcp->get_name().toString().c_str());
	} catch (rina::IpcmRegisterApplicationException& e) {
		//Remove the transaction and return
		remove_transaction_state(trans->tid);

		LOG_ERR("Error while registering IP prefix %s",
			event.applicationRegistrationInformation.appName.toString().c_str());
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t IPCManager_::unregister_ip_prefix_from_dif(Promise* promise,
						      const std::string& ip_range,
						      const rina::ApplicationProcessNamingInformation& dif_name)
{
	IPCMIPCProcess *slave_ipcp;
	APPUnregTransState* trans;
	rina::ApplicationUnregistrationRequestEvent req_event;

	try
	{
		slave_ipcp = select_ipcp_by_dif(dif_name, true);

		if (!slave_ipcp)
		{
			LOG_ERR("Cannot find any IPC process belonging to DIF %s",
				dif_name.toString().c_str());
			return IPCM_FAILURE;
		}

		rina::WriteScopedLock swritelock(slave_ipcp->rwlock, false);

		//Create a transaction
		req_event.applicationName.processName = ip_range;
		req_event.applicationName.entityName = RINA_IP_FLOW_ENT_NAME;
		trans = new APPUnregTransState(NULL, promise, slave_ipcp->get_id(), req_event);
		if (!trans)
		{
			LOG_ERR("Unable to allocate memory for the transaction object. Out of memory! ");
			return IPCM_FAILURE;
		}

		//Store transaction
		if (add_transaction_state(trans) < 0)
		{
			LOG_ERR("Unable to add transaction; out of memory?");
			return IPCM_FAILURE;
		}

		// Forward the unregistration request to the IPC process
		// that the client IPC process is registered to
		slave_ipcp->unregisterApplication(req_event.applicationName, trans->tid);

		LOG_INFO("Requested unregistration of IP prefix %s"
				" from IPC process %s",
				ip_range.c_str(),
				slave_ipcp->get_name().toString().c_str());
	} catch (rina::ConcurrentException& e)
	{
		LOG_ERR("Error while unregistering IP prefix %s"
				" from IPC process %s, The operation timed out.",
				ip_range.c_str(),
				slave_ipcp->get_name().toString().c_str());
		remove_transaction_state(trans->tid);
		return IPCM_FAILURE;
	} catch (rina::IpcmUnregisterApplicationException& e)
	{
		LOG_ERR("Error while unregistering IP prefix %s"
				" from IPC process %s",
				ip_range.c_str(),
				slave_ipcp->get_name().toString().c_str());
		remove_transaction_state(trans->tid);
		return IPCM_FAILURE;
	} catch (rina::Exception& e)
	{
		LOG_ERR("Unknown error while requesting unregistering IPCP");
		remove_transaction_state(trans->tid);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t IPCManager_::allocate_iporina_flow(Promise* promise,
				 	      const std::string& src_ip_range,
					      const std::string& dst_ip_range,
					      const std::string& dif_name,
					      const rina::FlowSpecification flow_spec)
{
	rina::FlowRequestEvent event;

	event.localRequest = true;
	event.localApplicationName.processName = src_ip_range;
	event.localApplicationName.entityName = RINA_IP_FLOW_ENT_NAME;
	event.remoteApplicationName.processName = dst_ip_range;
	event.remoteApplicationName.entityName = RINA_IP_FLOW_ENT_NAME;
	event.flowSpecification = flow_spec;
	event.DIFName.processName = dif_name;
	event.DIFName.processInstance = "";

	return flow_allocation_requested_event_handler(promise, &event);
}

void IPCManager_::allocate_iporina_flow_response(const rina::FlowRequestEvent& event,
				    	    	 int result,
						 bool notify_source)
{
	IPCMIPCProcess * ipcp;

	ipcp = lookup_ipcp_by_id(event.ipcProcessId);
	if (!ipcp) {
		LOG_ERR("Error: Could not retrieve IPC process with id %d "
			", to serve remote flow allocation request", event.ipcProcessId);
		return;
	}

	//Auto release the read lock
	rina::ReadScopedLock readlock(ipcp->rwlock, false);

	try {
		// Inform the IPC process about the response of the flow
		// allocation procedure
		ipcp->allocateFlowResponse(event, result, notify_source, 0);

		LOG_INFO("Informing IPC process %s about flow allocation from application %s"
			" to application %s in DIF %s [success = %d, port-id = %d]",
			 ipcp->proxy_->name.getProcessNamePlusInstance().c_str(),
			 event.localApplicationName.toString().c_str(),
			 event.remoteApplicationName.toString().c_str(),
			 ipcp->dif_name_.processName.c_str(), result, event.portId);
	} catch (rina::AllocateFlowException& e) {
		LOG_ERR("Error while informing IPC process %d"
			" about flow allocation", ipcp->proxy_->id);
	}
}

ipcm_res_t IPCManager_::deallocate_iporina_flow(Promise* promise,
				   	   	int port_id)
{
	rina::FlowRequestEvent req_event;
	rina::FlowDeallocateRequestEvent event;

	if (ip_vpn_manager->get_iporina_flow_info(port_id, req_event)) {
		return IPCM_FAILURE;
	}

	event.sequenceNumber = 0;
	event.portId = port_id;
	event.applicationName = req_event.localApplicationName;

	return flow_deallocation_requested_event_handler(promise, &event);
}

} //namespace rinad
