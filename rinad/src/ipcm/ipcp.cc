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

void IPCManager_::ipc_process_daemon_initialized_event_handler(
				rina::IPCProcessDaemonInitializedEvent * e)
{
	ostringstream ss;
	rina::IPCProcess* ipcp;

	//Recover the syscall transaction state and finalize the
	TransactionState* trans = new TransactionState(NULL, e->ipcProcessId);
	trans->ret = 0;

	if(add_syscall_transaction_state(e->ipcProcessId, trans) < 0){
		delete trans;
		//Notify
		trans = get_syscall_transaction_state(e->ipcProcessId);
		trans->wait_cond.signal();
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

	assert(ipcp->getType() == rina::NORMAL_IPC_PROCESS);

	//Initialize
	ipcp->setInitialized();
	ss << "IPC process daemon initialized [id = " <<
		e->ipcProcessId<< "]" << endl;
	FLUSH_LOG(INFO, ss);
}

int IPCManager_::ipcm_register_response_ipcp(
        rina::IpcmRegisterApplicationResponseEvent *event,
        map<unsigned int,
            std::pair<rina::IPCProcess *, rina::IPCProcess *>
           >::iterator mit)
{
        rina::IPCProcess *ipcp = mit->second.first;
        rina::IPCProcess *slave_ipcp = mit->second.second;
        const rina::ApplicationProcessNamingInformation&
                slave_dif_name = slave_ipcp->
                getDIFInformation().dif_name_;
        ostringstream ss;
        bool success;
        int ret = -1;

        success = ipcm_register_response_common(event, ipcp->name,
                                        slave_ipcp, slave_dif_name);
        if (success) {
                // Notify the registered IPC process.
                try {
                        ipcp->notifyRegistrationToSupportingDIF(
                                        slave_ipcp->name,
                                        slave_dif_name
                                        );
                        ss << "IPC process " << ipcp->name.toString() <<
                                " informed about its registration "
                                "to N-1 DIF " <<
                                slave_dif_name.toString() << endl;
                        FLUSH_LOG(INFO, ss);
                        ret = 0;
                } catch (rina::NotifyRegistrationToDIFException) {
                        ss  << ": Error while notifying "
                                "IPC process " <<
                                ipcp->name.toString() <<
                                " about registration to N-1 DIF"
                                << slave_dif_name.toString() << endl;
                        FLUSH_LOG(ERR, ss);
                }
        } else {
                ss  << "Cannot register IPC process "
                        << ipcp->name.toString() <<
                        " to DIF " << slave_dif_name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        }

        pending_ipcp_registrations.erase(mit);

        return ret;
}

int
IPCManager_::unregister_ipcp_from_ipcp(int ipcp_id,
                                      int slave_ipcp_id)
{
        unsigned int seqnum;
        bool arrived = true;
        ostringstream ss;
        rina::IPCProcess *ipcp, *slave_ipcp;
	int ret;

        try {

		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw Exception();
		}
		slave_ipcp = lookup_ipcp_by_id(slave_ipcp_id);

		if (!slave_ipcp) {
			ss << "Invalid IPCP id "<< slave_ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw Exception();
		}

                // Forward the unregistration request to the IPC process
                // that the client IPC process is registered to
				seqnum = opaque_generator_.next();
                slave_ipcp->unregisterApplication(ipcp->name, seqnum);
                pending_ipcp_unregistrations[seqnum] =
                                make_pair(ipcp, slave_ipcp);

                ss << "Requested unregistration of IPC process " <<
                        ipcp->name.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
                FLUSH_LOG(INFO, ss);

                /*arrived = concurrency.wait_for_event(
                                rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT,
                                seqnum, ret);*/
        } catch (rina::IpcmUnregisterApplicationException) {
                ss  << ": Error while unregistering IPC process "
                        << ipcp->name.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        if (!arrived) {
                ss  << ": Timed out" << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        return 0;
}

int IPCManager_::ipcm_unregister_response_ipcp(
                        rina::IpcmUnregisterApplicationResponseEvent *event,
                        map<unsigned int,
                            pair<rina::IPCProcess *, rina::IPCProcess *>
                           >::iterator mit)
{
        rina::IPCProcess *ipcp = mit->second.first;
        rina::IPCProcess *slave_ipcp = mit->second.second;
        rina::ApplicationProcessNamingInformation slave_dif_name = slave_ipcp->
                                                getDIFInformation().dif_name_;
        ostringstream ss;
        bool success;
        int ret = -1;

        // Inform the supporting IPC process
        success = ipcm_unregister_response_common(event, slave_ipcp,
                                                  ipcp->name);

        try {
                if (success) {
                        // Notify the IPCP process that it has been unregistered
                        // from the DIF
                        ipcp->notifyUnregistrationFromSupportingDIF(
                                                        slave_ipcp->name,
                                                        slave_dif_name);
                        ss << "IPC process " << ipcp->name.toString() <<
                                " informed about its unregistration from DIF "
                                << slave_dif_name.toString() << endl;
                        FLUSH_LOG(INFO, ss);
                        ret = 0;
                } else {
                        ss  << ": Cannot unregister IPC Process "
                                << ipcp->name.toString() << " from DIF " <<
                                slave_dif_name.toString() << endl;
                        FLUSH_LOG(ERR, ss);
                }
        } catch (rina::NotifyRegistrationToDIFException) {
                ss  << ": Error while reporing "
                        "unregistration result for IPC process "
                        << ipcp->name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        }

        pending_ipcp_unregistrations.erase(mit);

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
                                lookup_ipcp_address(ipcp->name,
                                                address);
                        if (!found) {
                                ss << "No address for IPC process " <<
                                        ipcp->name.toString() <<
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
                        ipcp->name.toString() << " to DIF " <<
                        dif_name.toString() << endl;
                FLUSH_LOG(INFO, ss);
                arrived = concurrency.wait_for_event(rina::ASSIGN_TO_DIF_RESPONSE_EVENT,
                                                     seqnum, ret);
        } catch (rina::AssignToDIFException) {
                ss << "Error while assigning " <<
                        ipcp->name.toString() <<
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
IPCManager_::assign_to_dif_response_event_handler(rina::IPCEvent * e)
{
	DOWNCAST_DECL(e, rina::AssignToDIFResponseEvent, event);
	map<unsigned int, rina::IPCProcess*>::iterator mit;
	ostringstream ss;
	bool success = (event->result == 0);
	int ret = -1;

	mit = IPCManager->pending_ipcp_dif_assignments.find(
					event->sequenceNumber);
	if (mit != IPCManager->pending_ipcp_dif_assignments.end()) {
		rina::IPCProcess *ipcp = mit->second;

		// Inform the IPC process about the result of the
		// DIF assignment operation
		try {
			ipcp->assignToDIFResult(success);
			ss << "DIF assignment operation completed for IPC "
				<< "process " << ipcp->name.toString() <<
				" [success=" << success << "]" << endl;
			FLUSH_LOG(INFO, ss);
			ret = 0;
		} catch (rina::AssignToDIFException) {
			ss << ": Error while reporting DIF "
				"assignment result for IPC process "
				<< ipcp->name.toString() << endl;
			FLUSH_LOG(ERR, ss);
		}
		IPCManager->pending_ipcp_dif_assignments.erase(mit);
	} else {
		ss << ": Warning: DIF assignment response "
			"received, but no pending DIF assignment" << endl;
		FLUSH_LOG(WARN, ss);
	}

	//IPCManager->concurrency.set_event_result(ret);
}

void
IPCManager_::update_dif_config_request_event_handler(rina::IPCEvent *event)
{
	(void)event;
}

void
IPCManager_::update_dif_config_response_event_handler(rina::IPCEvent *e)
{
	DOWNCAST_DECL(e, rina::UpdateDIFConfigurationResponseEvent, event);
	map<unsigned int, rina::IPCProcess*>::iterator mit;
	bool success = (event->result == 0);
	rina::IPCProcess *ipcp = NULL;
	ostringstream ss;

	mit = IPCManager->pending_dif_config_updates.find(event->sequenceNumber);
	if (mit == IPCManager->pending_dif_config_updates.end()) {
		ss  << ": Warning: DIF configuration update "
			"response received, but no corresponding pending "
			"request" << endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	ipcp = mit->second;
	try {

		// Inform the requesting IPC process about the result of
		// the configuration update operation
		ipcp->updateDIFConfigurationResult(success);
		ss << "Configuration update operation completed for IPC "
			<< "process " << ipcp->name.toString() <<
			" [success=" << success << "]" << endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::UpdateDIFConfigurationException) {
		ss  << ": Error while reporting DIF "
			"configuration update for process " <<
			ipcp->name.toString() << endl;
		FLUSH_LOG(ERR, ss);
	}

	IPCManager->pending_dif_config_updates.erase(mit);

	//IPCManager->concurrency.set_event_result(event->result);
}

void
IPCManager_::enroll_to_dif_request_event_handler(rina::IPCEvent *event)
{
	(void) event; // Stop compiler barfs
}

void
IPCManager_::enroll_to_dif_response_event_handler(rina::IPCEvent *e)
{
	DOWNCAST_DECL(e, rina::EnrollToDIFResponseEvent, event);
	map<unsigned int, rina::IPCProcess *>::iterator mit;
	rina::IPCProcess *ipcp = NULL;
	bool success = (event->result == 0);
	ostringstream ss;
	int ret = -1;

	mit = IPCManager->pending_ipcp_enrollments.find(event->sequenceNumber);
	if (mit == IPCManager->pending_ipcp_enrollments.end()) {
		ss  << ": Warning: IPC process enrollment "
			"response received, but no corresponding pending "
			"request" << endl;
		FLUSH_LOG(WARN, ss);
	} else {
		ipcp = mit->second;
		if (success) {
			ipcp->addNeighbors(event->neighbors);
			ipcp->setDIFInformation(event->difInformation);
			ss << "Enrollment operation completed for IPC "
				<< "process " << ipcp->name.toString() << endl;
			FLUSH_LOG(INFO, ss);
			ret = 0;
		} else {
			ss  << ": Error: Enrollment operation of "
				"process " << ipcp->name.toString() << " failed"
				<< endl;
			FLUSH_LOG(ERR, ss);
		}

		IPCManager->pending_ipcp_enrollments.erase(mit);
	}

	//IPCManager->concurrency.set_event_result(ret);
}

void IPCManager_::neighbors_modified_notification_event_handler(rina::IPCEvent * e)
{
	DOWNCAST_DECL(e, rina::NeighborsModifiedNotificationEvent, event);

	rina::IPCProcess *ipcp =
		rina::ipcProcessFactory->
		getIPCProcess(event->ipcProcessId);
	ostringstream ss;

	if (!event->neighbors.size()) {
		ss  << ": Warning: Empty neighbors-modified "
			"notification received" << endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	if (!ipcp) {
		ss  << ": Error: IPC process unexpectedly "
			"went away" << endl;
		FLUSH_LOG(ERR, ss);
		return;
	}

	if (event->added) {
		// We have new neighbors
		ipcp->addNeighbors(event->neighbors);
	} else {
		// We have lost some neighbors
		ipcp->removeNeighbors(event->neighbors);
	}
	ss << "Neighbors update [" << (event->added ? "+" : "-") <<
		"#" << event->neighbors.size() << "]for IPC process " <<
		ipcp->name.toString() <<  endl;
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
