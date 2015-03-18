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
#include "ipcp-handlers.h"

using namespace std;


namespace rinad {

void IPCManager_::ipc_process_daemon_initialized_event_handler(
				rina::IPCProcessDaemonInitializedEvent * e)
{
	ostringstream ss;
	IPCMIPCProcess* ipcp;

	//Recover the syscall transaction state and finalize the
	TransactionState* trans = new TransactionState(NULL, e->ipcProcessId);
	trans->ret = 0;

	if(add_syscall_transaction_state(e->ipcProcessId, trans) < 0){
		delete trans;
		//Notify
		trans = get_syscall_transaction_state(e->ipcProcessId);
		trans->completed(0);
	}else{
		//Do nothing (we are the first ones)
	};

	//FIXME TODO XXX Rwlock

	ipcp = lookup_ipcp_by_id(e->ipcProcessId);

	//If the ipcp is not there, there is some corruption
	if(!ipcp){
		ss << ": Warning: IPCP daemon initialized, "
				"but no pending normal IPC process initialization"
				<< endl;
		FLUSH_LOG(WARN, ss);
		assert(0);
		return;
	}

	assert(ipcp->get_type() == rina::NORMAL_IPC_PROCESS);

	//Initialize
	ipcp->setInitialized();
	ss << "IPC process daemon initialized [id = " <<
		e->ipcProcessId<< "]" << endl;
	FLUSH_LOG(INFO, ss);
}

int IPCManager_::ipcm_register_response_ipcp(
        rina::IpcmRegisterApplicationResponseEvent *e)
{
        ostringstream ss;
        bool success;
        int ret = -1;

	IPCPregTransState* trans = get_transaction_state<IPCPregTransState>(e->sequenceNumber);

	if(!trans){
		assert(0);
		return -1;
	}

	rinad::IPCMIPCProcess *ipcp = lookup_ipcp_by_id(trans->ipcp_id);
        rinad::IPCMIPCProcess *slave_ipcp = lookup_ipcp_by_id(trans->slave_ipcp_id);
        const rina::ApplicationProcessNamingInformation&
                slave_dif_name = slave_ipcp->dif_name_;

        success = ipcm_register_response_common(e, ipcp->get_name(),
                                        slave_ipcp, slave_dif_name);

	if(!ipcp || !slave_ipcp){
		assert(0);
		return -1;
	}


        if (success) {
                // Notify the registered IPC process.
                try {
                        ipcp->notifyRegistrationToSupportingDIF(
                                        slave_ipcp->get_name(),
                                        slave_dif_name
                                        );
                        ss << "IPC process " << ipcp->get_name().toString() <<
                                " informed about its registration "
                                "to N-1 DIF " <<
                                slave_dif_name.toString() << endl;
                        FLUSH_LOG(INFO, ss);
                        ret = 0;
                } catch (rina::NotifyRegistrationToDIFException) {
                        ss  << ": Error while notifying "
                                "IPC process " <<
                                ipcp->get_name().toString() <<
                                " about registration to N-1 DIF"
                                << slave_dif_name.toString() << endl;
                        FLUSH_LOG(ERR, ss);
                }
        } else {
                ss  << "Cannot register IPC process "
                        << ipcp->get_name().toString() <<
                        " to DIF " << slave_dif_name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        }

        return ret;
}

int IPCManager_::ipcm_unregister_response_ipcp(
                        rina::IpcmUnregisterApplicationResponseEvent *e)
{
	ostringstream ss;
        bool success;
        int ret = -1;


	IPCPregTransState* trans = get_transaction_state<IPCPregTransState>(e->sequenceNumber);

	if(!trans){
		assert(0);
		return -1;
	}

	rinad::IPCMIPCProcess *ipcp = lookup_ipcp_by_id(trans->ipcp_id);
        rinad::IPCMIPCProcess *slave_ipcp = lookup_ipcp_by_id(trans->slave_ipcp_id);
        const rina::ApplicationProcessNamingInformation&
                slave_dif_name = slave_ipcp->dif_name_;

	// Inform the supporting IPC process
        success = ipcm_unregister_response_common(e, slave_ipcp,
                                                  ipcp->get_name());

        try {
                if (success) {
                        // Notify the IPCP process that it has been unregistered
                        // from the DIF
                        ipcp->notifyUnregistrationFromSupportingDIF(
                                                        slave_ipcp->get_name(),
                                                        slave_dif_name);
                        ss << "IPC process " << ipcp->get_name().toString() <<
                                " informed about its unregistration from DIF "
                                << slave_dif_name.toString() << endl;
                        FLUSH_LOG(INFO, ss);
                        ret = 0;
                } else {
                        ss  << ": Cannot unregister IPC Process "
                                << ipcp->get_name().toString() << " from DIF " <<
                                slave_dif_name.toString() << endl;
                        FLUSH_LOG(ERR, ss);
                }
        } catch (rina::NotifyRegistrationToDIFException) {
                ss  << ": Error while reporing "
                        "unregistration result for IPC process "
                        << ipcp->get_name().toString() << endl;
                FLUSH_LOG(ERR, ss);
        }

        //pending_ipcp_unregistrations.erase(mit);

        return ret;
}

void
IPCManager_::application_unregistered_event_handler(rina::IPCEvent * event)
{
	(void) event;  // Stop compiler barfs
}

void
IPCManager_::assign_to_dif_request_event_handler(rina::IPCEvent * event)
{
	(void) event;  // Stop compiler barfs
}


#if 0
void
IPCManager_::assign_to_dif_response_event_handler(rina::IPCEvent * e)
{
        if (!ipcp)
                return -1;

        rinad::DIFProperties   dif_props;
        rina::DIFInformation   dif_info;
        rina::DIFConfiguration dif_config;
        ostringstream          ss;
        unsigned int           seqnum;
        bool                   arrived = true;
        bool                   found;
        int                    ret = -1;

        concurrency.lock();

        try {

                // Try to extract the DIF properties from the
                // configuration.
                found = config.lookup_dif_properties(dif_name,
                                dif_props);
                if (!found) {
                        ss << "Cannot find properties for DIF "
                                << dif_name.toString();
                        throw Exception();
                }

                // Fill in the DIFConfiguration object.
                if (ipcp->type == rina::NORMAL_IPC_PROCESS) {
                        rina::EFCPConfiguration efcp_config;
                        rina::NamespaceManagerConfiguration nsm_config;
                        rina::AddressingConfiguration address_config;
                        unsigned int address;

                        // FIll in the EFCPConfiguration object.
                        efcp_config.set_data_transfer_constants(
                                        dif_props.dataTransferConstants);
                        rina::QoSCube * qosCube = 0;
                        for (list<rina::QoSCube>::iterator
                                        qit = dif_props.qosCubes.begin();
                                        qit != dif_props.qosCubes.end();
                                        qit++) {
                                qosCube = new rina::QoSCube(*qit);
                                efcp_config.add_qos_cube(qosCube);
                        }

                        for (list<AddressPrefixConfiguration>::iterator
                                        ait = dif_props.addressPrefixes.begin();
                                        ait != dif_props.addressPrefixes.end();
                                        ait ++) {
                                rina::AddressPrefixConfiguration prefix;
                                prefix.address_prefix_ = ait->addressPrefix;
                                prefix.organization_ = ait->organization;
                                address_config.address_prefixes_.push_back(prefix);
                        }

                        for (list<rinad::KnownIPCProcessAddress>::iterator
                                        kit = dif_props.knownIPCProcessAddresses.begin();
                                        kit != dif_props.knownIPCProcessAddresses.end();
                                        kit ++) {
                                rina::StaticIPCProcessAddress static_address;
                                static_address.ap_name_ = kit->name.processName;
                                static_address.ap_instance_ = kit->name.processInstance;
                                static_address.address_ = kit->address;
                                address_config.static_address_.push_back(static_address);
                        }
                        nsm_config.addressing_configuration_ = address_config;

                        found = dif_props.
                                lookup_ipcp_address(ipcp->get_name(),
                                                address);
                        if (!found) {
                                ss << "No address for IPC process " <<
                                        ipcp->get_name().toString() <<
                                        " in DIF " << dif_name.toString() <<
                                        endl;
                                throw Exception();
                        }
                        dif_config.set_efcp_configuration(efcp_config);
                        dif_config.nsm_configuration_ = nsm_config;
                        dif_config.pduft_generator_configuration_ =
                                dif_props.pdufTableGeneratorConfiguration;
                        dif_config.rmt_configuration_ = dif_props.rmtConfiguration;
                        dif_config.et_configuration_ = dif_props.etConfiguration;
                        dif_config.set_address(address);
                }

                for (map<string, string>::const_iterator
                                pit = dif_props.configParameters.begin();
                                pit != dif_props.configParameters.end();
                                pit++) {
                        dif_config.add_parameter
                                (rina::Parameter(pit->first, pit->second));
                }

                // Fill in the DIFInformation object.
                dif_info.set_dif_name(dif_name);
                dif_info.set_dif_type(ipcp->type);
                dif_info.set_dif_configuration(dif_config);

                // Validate the parameters
                DIFConfigValidator validator(dif_config, dif_info,
                                ipcp->type);
                if(!validator.validateConfigs())
                        throw rina::BadConfigurationException("DIF configuration validator failed");

                // Invoke librina to assign the IPC process to the
                // DIF specified by dif_info.
                seqnum = ipcp->assignToDIF(dif_info);

                pending_ipcp_dif_assignments[seqnum] = ipcp;
                ss << "Requested DIF assignment of IPC process " <<
                        ipcp->get_name().toString() << " to DIF " <<
                        dif_name.toString() << endl;
                FLUSH_LOG(INFO, ss);
                arrived = concurrency.wait_for_event(rina::ASSIGN_TO_DIF_RESPONSE_EVENT,
                                                     seqnum, ret);
        } catch (rina::AssignToDIFException) {
                ss << "Error while assigning " <<
                        ipcp->get_name().toString() <<
                        " to DIF " << dif_name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        } catch (rina::BadConfigurationException &e) {
                LOG_ERR("DIF %s configuration failed", dif_name.toString().c_str());
                throw e;
        }
        catch (Exception) {
                FLUSH_LOG(ERR, ss);
        }

        concurrency.unlock();

        if (!arrived) {
                ss  << ": Timed out" << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        return ret;
}
#endif


void
IPCManager_::assign_to_dif_response_event_handler(rina::AssignToDIFResponseEvent* e)
{
	ostringstream ss;
	bool success = (e->result == 0);
        IPCMIPCProcess* ipcp;
	int ret = -1;

	IPCPTransState* trans = get_transaction_state<IPCPTransState>(e->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown assign to DIF response received: "<<e->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	// Inform the IPC process about the result of the
	// DIF assignment operation
	try {
		ipcp = lookup_ipcp_by_id(trans->ipcp_id);
		if(!ipcp){
			ss << ": Warning: Could not complete assign to dif action: "<<e->sequenceNumber<<
			"IPCP with id: "<<trans->ipcp_id<<"does not exist! Perhaps deleted?" << endl;
			FLUSH_LOG(WARN, ss);
			throw rina::AssignToDIFException();
		}
		ipcp->assignToDIFResult(success);

		ss << "DIF assignment operation completed for IPC "
			<< "process " << ipcp->get_name().toString() <<
			" [success=" << success << "]" << endl;
		FLUSH_LOG(INFO, ss);
		ret = 0;
	} catch (rina::AssignToDIFException) {
		ss << ": Error while reporting DIF "
			"assignment result for IPC process "
			<< ipcp->get_name().toString() << endl;
		FLUSH_LOG(ERR, ss);
	}

	//Mark as completed
	trans->completed(ret);

	//If there was a calle, invoke the callback and remove it. Otherwise
	//transaction is fully complete and originator will clean up the state
	if(trans->callee){
		//Invoke callback
		//FIXME

		//Remove the transaction
		remove_transaction_state(trans->tid);
		delete trans;
	}
}

void
IPCManager_::update_dif_config_request_event_handler(rina::IPCEvent *event)
{
	(void)event;
}

void
IPCManager_::update_dif_config_response_event_handler(rina::UpdateDIFConfigurationResponseEvent* e)
{
	ostringstream ss;
	bool success = (e->result == 0);
        IPCMIPCProcess* ipcp;
	int ret = -1;

	IPCPTransState* trans = get_transaction_state<IPCPTransState>(e->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown DIF config response received: "<<e->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	try {
		ipcp = lookup_ipcp_by_id(trans->ipcp_id);
		if(!ipcp){
			ss << ": Warning: Could not complete config to dif action: "<<e->sequenceNumber<<
			"IPCP with id: "<<trans->ipcp_id<<"does not exist! Perhaps deleted?" << endl;
			FLUSH_LOG(WARN, ss);
			throw rina::UpdateDIFConfigurationException();
		}

		// Inform the requesting IPC process about the result of
		// the configuration update operation
		ss << "Configuration update operation completed for IPC "
			<< "process " << ipcp->get_name().toString() <<
			" [success=" << success << "]" << endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::UpdateDIFConfigurationException) {
		ss  << ": Error while reporting DIF "
			"configuration update for process " <<
			ipcp->get_name().toString() << endl;
		FLUSH_LOG(ERR, ss);
	}

	//Mark as completed
	trans->completed(ret);

	//If there was a calle, invoke the callback and remove it. Otherwise
	//transaction is fully complete and originator will clean up the state
	if(trans->callee){
		//Invoke callback
		//FIXME

		//Remove the transaction
		remove_transaction_state(trans->tid);
		delete trans;
	}
}

void
IPCManager_::enroll_to_dif_request_event_handler(rina::IPCEvent *event)
{
	(void) event; // Stop compiler barfs
}

void
IPCManager_::enroll_to_dif_response_event_handler(rina::EnrollToDIFResponseEvent *event)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp;
	bool success = (event->result == 0);
	int ret = -1;

	IPCPTransState* trans = get_transaction_state<IPCPTransState>(event->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown enrollment to DIF response received: "<<event->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	ipcp = lookup_ipcp_by_id(trans->ipcp_id);
	if(!ipcp){
		ss << ": Warning: Could not complete enroll to dif action: "<<event->sequenceNumber<<
		"IPCP with id: "<<trans->ipcp_id<<" does not exist! Perhaps deleted?" << endl;
		FLUSH_LOG(WARN, ss);
	}else{
		if (success) {
			ss << "Enrollment operation completed for IPC "
				<< "process " << ipcp->get_name().toString() << endl;
			FLUSH_LOG(INFO, ss);
			ret = 0;
		} else {
			ss  << ": Error: Enrollment operation of "
				"process " << ipcp->get_name().toString() << " failed"
				<< endl;
			FLUSH_LOG(ERR, ss);
		}
	}

	//Mark as completed
	trans->completed(ret);

	//If there was a calle, invoke the callback and remove it. Otherwise
	//transaction is fully complete and originator will clean up the state
	if(trans->callee){
		//Invoke callback
		//FIXME

		//Remove the transaction
		remove_transaction_state(trans->tid);
		delete trans;
	}
}

void IPCManager_::neighbors_modified_notification_event_handler(rina::NeighborsModifiedNotificationEvent* event)
{
	ostringstream ss;

	if (!event->neighbors.size()) {
		ss  << ": Warning: Empty neighbors-modified "
			"notification received" << endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	IPCPTransState* trans = get_transaction_state<IPCPTransState>(event->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown enrollment to DIF response received: "<<event->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	IPCMIPCProcess* ipcp = lookup_ipcp_by_id(trans->ipcp_id);
	if(!ipcp){
		ss  << ": Error: IPC process unexpectedly "
			"went away" << endl;
		FLUSH_LOG(ERR, ss);
		return;
	}

	ss << "Neighbors update [" << (event->added ? "+" : "-") <<
		"#" << event->neighbors.size() << "]for IPC process " <<
		ipcp->get_name().toString() <<  endl;
	FLUSH_LOG(INFO, ss);

}

void IPCManager_::ipc_process_dif_registration_notification_handler(rina::IPCEvent *event)
{
	(void) event;  // Stop compiler barfs
}

void IPCManager_::ipc_process_query_rib_handler(rina::IPCEvent *event)
{
	(void) event;  // Stop compiler barfs
}

void IPCManager_::get_dif_properties_handler(rina::IPCEvent *event)
{
	(void) event;  // Stop compiler barfs
}

void IPCManager_::get_dif_properties_response_event_handler(rina::IPCEvent *event)
{
	(void) event;  // Stop compiler barfs
}

void IPCManager_::ipc_process_create_connection_response_handler(rina::IPCEvent * event)
{
	(void) event;  // Stop compiler barfs
}

void IPCManager_::ipc_process_update_connection_response_handler(rina::IPCEvent * event)
{
	(void) event;  // Stop compiler barfs
}

void IPCManager_::ipc_process_create_connection_result_handler(rina::IPCEvent * event)
{
	(void) event;  // Stop compiler barfs
}

void IPCManager_::ipc_process_destroy_connection_result_handler(rina::IPCEvent * event)
{
	(void) event;  // Stop compiler barfs
}

void IPCManager_::ipc_process_dump_ft_response_handler(rina::IPCEvent * event)
{
	(void) event;  // Stop compiler barfs
}

} //namespace rinad
