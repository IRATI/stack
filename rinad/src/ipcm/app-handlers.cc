/*
 * IPC Manager - Registration and unregistration
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#define RINA_PREFIX "ipcm.app"
#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>
#include <debug.h>

#include "rina-configuration.h"
#include "app-handlers.h"

using namespace std;

namespace rinad {

void IPCManager_::os_process_finalized_handler(pid_t pid)
{
	list<rina::FlowInformation> involved_flows;
	vector<IPCMIPCProcess *> ipcps;
	std::list<rina::ApplicationProcessNamingInformation> reg_names;
	std::list<rina::ApplicationProcessNamingInformation>::iterator regn_iter;
	unsigned short ipcp_id = 0;
	ostringstream ss;
	std::list<int>::iterator pid_it;

	{
		//Prevent any insertion/deletion to happen
		rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

		// Look if the terminating application had registrations or flows
		// provided by some N-1 IPC processes
		ipcp_factory_.listIPCProcesses(ipcps);
		for (unsigned int i = 0; i < ipcps.size(); i++) {
			std::list<int> port_ids;
			reg_names.clear();

			if (application_is_registered_to_ipcp(reg_names, pid, ipcps[i])) {
				for (regn_iter = reg_names.begin(); regn_iter != reg_names.end();
						++regn_iter) {
					// Build a structure that will be used during
					// the unregistration process. The last argument
					// is the request sequence number: 0 means that
					// the unregistration response does not match
					// an application request - this is indeed an
					// unregistration forced by the IPCM.
					rina::ApplicationUnregistrationRequestEvent req_event(*regn_iter,
							ipcps[i]->dif_name_,
							0, 0, 0);
					IPCManager->unregister_app_from_ipcp(NULL, NULL,
							req_event,
							ipcps[i]->get_id());
					LOG_INFO("OS process %d terminated, unregistered name %s from DIF %s",
							pid, (*regn_iter).toString().c_str(),
							ipcps[i]->dif_name_.processName.c_str());
				}
			}

			application_has_flow_by_ipcp(pid, ipcps[i], port_ids);
			for(pid_it = port_ids.begin(); pid_it != port_ids.end(); ++pid_it) {
				rina::FlowDeallocateRequestEvent fevent;
				fevent.portId = *pid_it;
				IPCManager->deallocate_flow(NULL, ipcps[i]->proxy_->id, fevent);
				LOG_INFO("OS process %d terminated, deallocated flow with port-id %d",
					  pid, fevent.portId);
			}
		}
	}

	ipcp_id = IPCManager->ipcp_exists_by_pid(pid);
	if (ipcp_id != 0) {
		LOG_INFO("OS process %d terminated, it was IPCP with id %u",
				pid, ipcp_id);
		//An IPCP OS process has crashed, we need to clean up state
		//TODO if the IPCP was supporting flows or had
		//registered applications, notify them

		//Distribute the event to the addons
		IPCMEvent addon_e(NULL, IPCM_IPCP_CRASHED, ipcp_id);
		Addon::distribute_ipcm_event(addon_e);

		// Cleanup IPC Process state in the kernel
		if(IPCManager->destroy_ipcp(NULL, ipcp_id) < 0 ){
			LOG_WARN("Problems cleaning up state of IPCP with id: %d\n",
				 ipcp_id);
		}
	}
}

void IPCManager_::notify_app_reg(
        const rina::ApplicationRegistrationRequestEvent& req_event,
        const rina::ApplicationProcessNamingInformation& app_name,
        const rina::ApplicationProcessNamingInformation& slave_dif_name,
        bool success)
{
        ostringstream ss;

        try {
                rina::applicationManager->applicationRegistered(req_event,
                                slave_dif_name, success ? 0 : -1);
                ss << "Application " << app_name.toString() <<
                        " informed about its registration "
                        "to N-1 DIF " << slave_dif_name.toString() <<
                        " [success = " << success << "]" << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::NotifyApplicationRegisteredException& e) {
                ss  << "Error while notifying application "
                        << app_name.toString() << " about registration "
                        "to DIF " << slave_dif_name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        }
}

void IPCManager_::app_reg_req_handler(rina::ApplicationRegistrationRequestEvent *event)
{
	const rina::ApplicationRegistrationInformation& info = event->
			applicationRegistrationInformation;
	const rina::ApplicationProcessNamingInformation app_name =
			info.appName;
	IPCMIPCProcess *slave_ipcp = NULL;
	ostringstream ss;
	rina::ApplicationProcessNamingInformation dif_name;
	rina::ApplicationProcessNamingInformation daf_name;
	APPregTransState* trans;
	da_res_t dif_specified;
	std::list<std::string> dif_names;

	//Prepare the registration information
	if (info.applicationRegistrationType ==
			rina::APPLICATION_REGISTRATION_ANY_DIF) {
		// See if the configuration specifies a mapping between
		// the registering application and a DIF.
		// Ask the DIF allocator
		ipcp_factory_.get_local_dif_names(dif_names);
		dif_specified =  dif_allocator->lookup_dif_by_application(app_name,
								dif_name,
								dif_names);
		if (dif_specified == DA_SUCCESS){
			// If a mapping exists, select an IPC process
			// from the specified DIF.
			slave_ipcp = select_ipcp_by_dif(dif_name);
		} else {
			// Otherwise select any IPC process.
			slave_ipcp = select_ipcp();
		}
	} else if (info.applicationRegistrationType ==
			rina::APPLICATION_REGISTRATION_SINGLE_DIF) {
		// Select an IPC process from the DIF specified in the
		// registration request.
		slave_ipcp = select_ipcp_by_dif(info.difName);
	} else {
		ss  << ": Unsupported registration type: "
				<< info.applicationRegistrationType << endl;
		FLUSH_LOG(ERR, ss);

		// Notify the application about the unsuccessful registration.
		notify_app_reg(*event, app_name,
				rina::ApplicationProcessNamingInformation(), false);
		return;
	}

	if (!slave_ipcp) {
		ss  << ": Cannot find a suitable DIF to "
				"register application " << app_name.toString() << endl;
		FLUSH_LOG(ERR, ss);
		// Notify the application about the unsuccessful registration.
		notify_app_reg(*event, app_name,
				rina::ApplicationProcessNamingInformation(), false);
		return;
	}

	//Auto release the read lock
	rina::ReadScopedLock readlock(slave_ipcp->rwlock, false);

	//Perform the registration
	try {
		//Create a transaction
		trans = new APPregTransState(NULL, NULL, slave_ipcp->get_id(), *event);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! "
					<< dif_name.toString();
			throw rina::IpcmRegisterApplicationException();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? "
					<< dif_name.toString();
			throw rina::IpcmRegisterApplicationException();
		}

		slave_ipcp->registerApplication(info,
						trans->tid);

		ss << "Requested registration of application " <<
				app_name.toString() << " to IPC process " <<
				slave_ipcp->get_name().toString() << endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::IpcmRegisterApplicationException& e) {
		//Remove the transaction and return
		remove_transaction_state(trans->tid);

		ss  << ": Error while registering application "
				<< app_name.toString() << endl;
		FLUSH_LOG(ERR, ss);

		// Notify the application about the unsuccessful registration.
		notify_app_reg(*event, app_name,
				slave_ipcp->get_name(), false);
	}
}

bool
IPCManager_::ipcm_register_response_common(rina::IpcmRegisterApplicationResponseEvent *event,
					   const rina::ApplicationProcessNamingInformation& app_name,
					   IPCMIPCProcess *slave_ipcp,
					   const rina::ApplicationProcessNamingInformation& slave_dif_name)
{
        bool success = (event->result == 0);
        ostringstream ss;

        try {
                // Notify the N-1 IPC process.
                slave_ipcp->registerApplicationResult(
                                        event->sequenceNumber, success);
        } catch (rina::IpcmRegisterApplicationException& e) {
                ss << ": Error while reporting "
                        "registration result of application "
                        << app_name.toString() <<
                        " to N-1 DIF " << slave_dif_name.toString()
                        << endl;
                FLUSH_LOG(ERR, ss);
        }

        if (success)

        return success;
}

int
IPCManager_::ipcm_register_response_app(rina::IpcmRegisterApplicationResponseEvent *event,
					IPCMIPCProcess * slave_ipcp,
					const rina::ApplicationRegistrationRequestEvent& req_event)
{
	const rina::ApplicationProcessNamingInformation& app_name =
			req_event.applicationRegistrationInformation.appName;
	const rina::ApplicationProcessNamingInformation&
	slave_dif_name = slave_ipcp->dif_name_;

	bool success;

	success = ipcm_register_response_common(event, app_name, slave_ipcp,
			slave_dif_name);

        //Inform DIF allocator
        if (event->result == 0)
        	dif_allocator->app_registered(req_event.applicationRegistrationInformation.appName,
        			              slave_ipcp->dif_name_.processName);

        if  (app_name.entityName == RINA_IP_FLOW_ENT_NAME) {
        	ip_vpn_manager->add_registered_ip_vpn(app_name.processName);

        	LOG_INFO("IP VPN %s registered to DIF %s",
        			app_name.processName.c_str(),
				slave_dif_name.processName.c_str());
        } else {
        	// Notify the application about the (un)successful registration.
        	notify_app_reg(req_event, app_name, slave_dif_name, success);
        }

	return success;
}

void IPCManager_::app_reg_response_handler(rina::IpcmRegisterApplicationResponseEvent* e)
{
        IPCMIPCProcess *ipcp;
        ostringstream ss;
	APPregTransState* t1;
	IPCPregTransState* t2;
	ipcm_res_t ret = IPCM_FAILURE;

	if (e->result == 0) {
		ret = IPCM_SUCCESS;
	}
	IPCPTransState* trans = get_transaction_state<IPCPTransState>(e->sequenceNumber);

	if(!trans){
		//Transacion was not found
		ss << ": Warning: application registration response " <<
		      "received, but no pending transaction with id " <<
			   e->sequenceNumber << endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	try{
		//Recover IPCP
		ipcp = lookup_ipcp_by_id(trans->ipcp_id);
		if(!ipcp){
			ss << ": Warning: Could not complete application registration: "<<e->sequenceNumber<<
			"IPCP with id: "<<trans->ipcp_id<<"does not exist! Perhaps deleted?" << endl;
			FLUSH_LOG(WARN, ss);
			throw rina::Exception();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		//TODO  solve this mess. We have to do it this way
		//because code is different from IPCP to an APP, but
		//there is a single event
		t1 = get_transaction_state<APPregTransState>(trans->tid);
		if(t1){
			ipcm_register_response_app(e, ipcp, t1->req);
		}else{
			//IPCP registration
			ipcm_register_response_ipcp(ipcp, e);
		}
	}catch(...){
		ret = IPCM_FAILURE;
	}

	trans->completed(ret);
	remove_transaction_state(trans->tid);
}

void IPCManager_::application_manager_app_unregistered(
                rina::ApplicationUnregistrationRequestEvent event,
                int result)
{
        ostringstream ss;

        // Inform the application about the unregistration result
	if (event.sequenceNumber == 0)
		return;
	try {
		rina::applicationManager->applicationUnregistered(event,
								  result);
		ss << "Application " << event.applicationName.
		toString() << " informed about its "
		"unregistration [success = " << (!result) <<
		"]" << endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::NotifyApplicationUnregisteredException& e) {
		ss  << ": Error while notifying application "
		<< event.applicationName.toString() << " about "
		"failed unregistration" << endl;
		FLUSH_LOG(ERR, ss);
	}
}

void IPCManager_::application_unregistration_request_event_handler(
			rina::ApplicationUnregistrationRequestEvent* event)
{
	IPCMIPCProcess * slave_ipcp = NULL;

	if (event->DIFName.processName == "") {
		slave_ipcp = select_ipcp_by_reg_app(event->applicationName);
	} else {
	        // Select any IPC process in the DIF from which the application
	        // wants to unregister from
		slave_ipcp = select_ipcp_by_dif(event->DIFName);
	}

        ostringstream ss;
        int err;
        unsigned short ipcp_id;

        if (!slave_ipcp) {
                ss  << ": Error: Application " <<
                        event->applicationName.toString() <<
                        " wants to unregister from DIF " <<
                        event->DIFName.toString() << " but couldn't find "
                        "any IPC process belonging to it" << endl;
                FLUSH_LOG(ERR, ss);

                application_manager_app_unregistered(*event, -1);
                return;
        }

        {
        	//Auto release the read lock
        	rina::ReadScopedLock readlock(slave_ipcp->rwlock, false);
        	ipcp_id = slave_ipcp->get_id();
        }

        err = unregister_app_from_ipcp(NULL, NULL, *event, ipcp_id);
        if (err < 0) {
                // Inform the unregistering application that the unregistration
                // operation failed
                application_manager_app_unregistered(*event, -1);
        }
}

bool IPCManager_::ipcm_unregister_response_common(
                        rina::IpcmUnregisterApplicationResponseEvent *event,
                        IPCMIPCProcess *slave_ipcp,
                        const rina::ApplicationProcessNamingInformation&
                        app_name)
{
        bool success = (event->result == 0);
        ostringstream ss;

        try {
                // Inform the IPC process about the application unregistration
                slave_ipcp->unregisterApplicationResult(event->
                                sequenceNumber, success);
                ss << "IPC process " << slave_ipcp->get_name().toString() <<
                        " informed about unregistration of application "
                        << app_name.toString() << " [success = " << success
                        << "]" << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::IpcmRegisterApplicationException& e) {
                ss  << ": Error while reporing "
                        "unregistration result for application "
                        << app_name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        }

        return success;
}

int IPCManager_::ipcm_unregister_response_app(
                        rina::IpcmUnregisterApplicationResponseEvent *event,
                        IPCMIPCProcess * ipcp,
                        rina::ApplicationUnregistrationRequestEvent& req)
{
	// Inform the supporting IPC process
	ipcm_unregister_response_common(event, ipcp,
			req.applicationName);

	//Inform DIF allocator
	if (event->result == 0)
		dif_allocator->app_unregistered(req.applicationName,
				ipcp->dif_name_.processName);

	if (req.applicationName.entityName == RINA_IP_FLOW_ENT_NAME) {
		ip_vpn_manager->remove_registered_ip_vpn(req.applicationName.processName);

		LOG_INFO("IP VPN %s unregistered from DIF %s",
				req.applicationName.processName.c_str(),
				ipcp->dif_name_.processName.c_str());

	} else {
		// Inform the application
		application_manager_app_unregistered(req,
				event->result);
	}


	return 0;
}

void IPCManager_::unreg_app_response_handler(rina::IpcmUnregisterApplicationResponseEvent *e)
{
	ostringstream ss;
	IPCMIPCProcess* ipcp;
	ipcm_res_t ret = IPCM_FAILURE;
	if (e->result == 0) {
		ret = IPCM_SUCCESS;
	}

	//First check if this de-reg was a pending
	APPUnregTransState* t1;
	IPCPregTransState* t2;
	IPCPTransState* trans = get_transaction_state<IPCPTransState>(e->sequenceNumber);

	if(!trans){
		//Transacion was not found
                ss << ": Warning: Application unregistration response "
                        "received but unknown. Perhaps the IPCP was deleted? " << endl;
                FLUSH_LOG(WARN, ss);
		return;
	}

	try{
		//Recover IPCP
		ipcp = lookup_ipcp_by_id(trans->ipcp_id);
		if(!ipcp){
			ss << ": Warning: Could not complete application unregistration: "<<e->sequenceNumber<<
			"IPCP with id: "<<trans->ipcp_id<<"does not exist! Perhaps deleted?" << endl;
			FLUSH_LOG(WARN, ss);
			throw rina::Exception();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		//TODO  solve this mess. We have to do it this way
		//because code is different from IPCP to an APP, but
		//there is a single event
		t1 = get_transaction_state<APPUnregTransState>(trans->tid);

		//Application registration
		if(t1) {
			ipcm_unregister_response_app(e, ipcp, t1->req);
		} else {
			t2 = get_transaction_state<IPCPregTransState>(e->sequenceNumber);
			if (t2){
				ipcm_unregister_response_ipcp(ipcp, e, t2);
			}
			else {
				//This is the case when the app unreg has been requested by the IPCM
				bool success = (e->result == 0);
				ipcp->unregisterApplicationResult(e->sequenceNumber, success);
			}
		}
	}catch(...){
		ret = IPCM_FAILURE;
	}

	trans->completed(ret);
	remove_transaction_state(trans->tid);
}

}//namespace rinad
