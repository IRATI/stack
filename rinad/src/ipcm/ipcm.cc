/*
 * IPC Manager
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

#include <librina/common.h>
#include <librina/ipc-manager.h>

#define RINA_PREFIX "ipcm"
#include <librina/logs.h>

#include "rina-configuration.h"
#include "ipcm.h"
#include "dif-validator.h"

//Addons
#include "addons/console.h"
#include "addons/scripting.h"
//[+] Add more here...

//Event transaction states
#include "ipcp.h"
#include "flow-alloc.h"
#include "misc.h"

//Timeouts for timed wait
#define IPCMANAGER_TIMEOUT_S 0
#define IPCMANAGER_TIMEOUT_NS 100000000 //0.1 sec

using namespace std;

namespace rinad {

#if 0
//
//IPCMConcurrency
//
bool
IPCMConcurrency::wait_for_event(rina::IPCEventType ty, unsigned int seqnum,
				int &result)
{
	bool arrived = false;

	event_waiting = true;
	event_ty      = ty;
	event_sn      = seqnum;
	event_result  = -1;

	try {
		timedwait(wait_time, 0);
		arrived = true;
	} catch (rina::ConcurrentException) {
		event_waiting = false;
		event_result = -1;
	}

	result = event_result;

	return arrived;
}

void
IPCMConcurrency::notify_event(rina::IPCEvent *event)
{
	if (event_waiting && event_ty == event->eventType
			&& (event_sn == 0 ||
			    event_sn == event->sequenceNumber)) {
		signal();
		event_waiting = false;
	}
}
#endif


//
//IPCManager_
//

//Singleton instance
Singleton<IPCManager_> IPCManager;

IPCManager_::IPCManager_() : script(NULL), console(NULL){ }

IPCManager_::~IPCManager_()
{
	if (console) {
		delete console;
	}

	if (script) {
		// Maybe we should join here
		delete script;
	}
}

void IPCManager_::init(unsigned int wait_time, const std::string& loglevel)
{
	// Initialize the IPC manager infrastructure in librina.

	try {
		rina::initializeIPCManager(1,
					   config.local.installationPath,
					   config.local.libraryPath,
					   loglevel,
					   config.local.logPath);
		LOG_DBG("IPC Manager daemon initialized");
		LOG_DBG("       installation path: %s",
			config.local.installationPath.c_str());
		LOG_DBG("       library path: %s",
			config.local.libraryPath.c_str());
		LOG_DBG("       log folder: %s", config.local.logPath.c_str());
	} catch (rina::InitializationException) {
		LOG_ERR("Error while initializing librina-ipc-manager");
		exit(EXIT_FAILURE);
	}
}

int
IPCManager_::start_script_worker()
{
	if (script)
		return -1;

	rina::ThreadAttributes ta;
	script = new rina::Thread(&ta, script_function, this);

	return 0;
}

int
IPCManager_::start_console_worker()
{
	if (console) {
		return -1;
	}

	rina::ThreadAttributes ta;
	console = new IPCMConsole(ta, config.local.consolePort);

	return 0;
}

/*
*
* Public API
*
*/

int
IPCManager_::create_ipcp(const Addon* callee,
			const rina::ApplicationProcessNamingInformation& name,
			const std::string& type)
{
	rina::IPCProcess *ipcp;
	ostringstream ss;
	rina::IPCProcessFactory fact;
	std::list<std::string> ipcp_types;
	bool difCorrect = false;
	std::string s;
	int ret;
	TransactionState* state;

	//TODO: check if rwlock is really necessary here

	try {
		// Check that the AP name is not empty
		if (name.processName == std::string("")) {
			ss << "Cannot create IPC process with an "
				"empty AP name" << endl;
			FLUSH_LOG(ERR, ss);
			throw rina::CreateIPCProcessException();
		}

		//Check if dif type exists
		list_ipcp_types(ipcp_types);
		if(std::find(ipcp_types.begin(), ipcp_types.end(), type)
							== ipcp_types.end()){
			ss << "IPCP type parameter "
				   << name.toString()
				   << " is wrong, options are: "
				   << s;
				FLUSH_LOG(ERR, ss);
				throw rina::CreateIPCProcessException();
		}

		ipcp = rina::ipcProcessFactory->create(name, type);

		//TODO: this should be moved to the factory
		//Moreover the API should be homgenized such that the
		if (type != rina::NORMAL_IPC_PROCESS) {
			// Shim IPC processes are set as initialized
			// immediately.
			ipcp->setInitialized();
		} else {
			// Normal IPC processes can be set as
			// initialized only when the corresponding
			// IPC process daemon is initialized, so we
			// defer the operation.

			//Add transaction state
			state = new TransactionState(callee, ipcp->id);

			//TODO: this is a botch that we have to do due to the
			//way ipcmanager in librina works. Since we cannot make
			//sure we add the transaction *before* the response is
			//received (race condition), we simply try to add it
			//and if the notification thread is first, just deduce
			//that has been already notified.
			//
			//The only proper solution is combine the code of the
			//IPCFactory here, since the state is basically the
			//same and is the only way to prevent these nasty race
			//conditions
			if(add_syscall_transaction_state(ipcp->id, state) < 0){
				delete state;
				state = NULL;
			}
		}

		//TODO: fix this accordingly (part of the botch)
		if(state){
			//We have to wait for the notification
			if(callee){
				//callback will be called
				return ipcp->id;
			}else{
				//We have to synchronously wait
				state->wait(); //FIXME: timed wait
				if(!state->ret < 0){
					ss << "Failed to create IPC process '" <<
						name.toString() << "' of type '" <<
						type << "'" << endl;
					FLUSH_LOG(ERR, ss);
					return -1;
				}
			}
		}else{
			//The notification already arrived
			state = get_syscall_transaction_state(ipcp->id);
			if(!state){
				assert(0);
				ss << "Corrupted ipc create operation"<<endl;
				FLUSH_LOG(ERR, ss);
			}
		}

		//Show a nice trace
		ss << "IPC process " << name.toString() << " created "
			"[id = " << ipcp->id << "]" << endl;
		FLUSH_LOG(INFO, ss);


	} catch (rina::CreateIPCProcessException) {
		ss << "Failed to create IPC process '" <<
			name.toString() << "' of type '" <<
			type << "'" << endl;
		FLUSH_LOG(ERR, ss);
		return -1;
	}


	return ret;
}

int
IPCManager_::destroy_ipcp(const Addon* callee, unsigned int ipcp_id)
{
	ostringstream ss;
	int ret = 0;

	//TODO: check if rwlock is really necessary here
	try {
		rina::ipcProcessFactory->destroy(ipcp_id);
		ss << "IPC process destroyed [id = " << ipcp_id
			<< "]" << endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::DestroyIPCProcessException) {
		ss  << ": Error while destroying IPC "
			"process with id " << ipcp_id << endl;
		FLUSH_LOG(ERR, ss);
		ret = -1;
	}

	return ret;
}

int
IPCManager_::list_ipcps(std::ostream& os)
{
	const vector<rina::IPCProcess *>& ipcps =
		rina::ipcProcessFactory->listIPCProcesses();

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcps_rwlock);

	os << "Current IPC processes:" << endl;
	for (unsigned int i = 0; i < ipcps.size(); i++) {
		os << "    " << ipcps[i]->id << ": " <<
			ipcps[i]->name.toString() << "\n";
	}

	return 0;
}

bool
IPCManager_::ipcp_exists(const int ipcp_id){
	return (lookup_ipcp_by_id(ipcp_id) != NULL);
}

int
IPCManager_::list_ipcp_types(std::list<std::string>& types)
{
	types = rina::ipcProcessFactory->getSupportedIPCProcessTypes();
	return 0;
}

//TODO this assumes single IPCP per DIF
int IPCManager_::get_ipcp_by_dif_name(std::string& difName){

	rina::IPCProcess* ipcp;
	int ret;
	rina::ApplicationProcessNamingInformation dif(difName, string());

	ipcp = select_ipcp_by_dif(dif);
	if(!ipcp)
		ret = -1;
	else
		ret = ipcp->id;

	return ret;
}



int
IPCManager_::assign_to_dif(const Addon* callee, const int ipcp_id,
			  const rina::ApplicationProcessNamingInformation &
			  dif_name)
{
	rinad::DIFProperties   dif_props;
	rina::DIFInformation   dif_info;
	rina::DIFConfiguration dif_config;
	ostringstream	  ss;
	unsigned int	   seqnum;
	bool		   found;
	int		    ret = -1;
	rina::IPCProcess* ipcp;
	IPCPTransState* trans;

	//TODO: move this to a write_lock over the IPCP

	try {

		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			throw Exception();
		}

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
				if(!qosCube){
					ss << "Unable to allocate memory for the QoSCube object. Out of memory! "
					<< dif_name.toString();
					throw Exception();
				}
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
		seqnum = opaque_generator_.next();

		//Create a transaction
		trans = new IPCPTransState(callee, seqnum,
							ipcp->id);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! "
				<< dif_name.toString();
			throw Exception();
		}

		//Store transaction
		if(add_transaction_state(seqnum, trans) < 0){
			ss << "Unable to add transaction; out of memory? "
				<< dif_name.toString();
			throw Exception();
		}

		ipcp->assignToDIF(dif_info, seqnum);

		ss << "Requested DIF assignment of IPC process " <<
			ipcp->name.toString() << " to DIF " <<
			dif_name.toString() << endl;
		FLUSH_LOG(INFO, ss);

		//If it is a synchronous call, wait until it has been finished
		if(!callee){
			//Just wait in the condition variable
			//TODO: move this to a timedwait
			trans->wait();

			//Callback
			//TODO
		}
	} catch (rina::AssignToDIFException) {
		ss << "Error while assigning " <<
			ipcp->name.toString() <<
			" to DIF " << dif_name.toString() << endl;
		FLUSH_LOG(ERR, ss);
		ret = -1;
	} catch (rina::BadConfigurationException &e) {
		FLUSH_LOG(ERR, ss);
		LOG_ERR("DIF %s configuration failed", dif_name.toString().c_str());
		ret = -1;
	}catch (Exception) {
		FLUSH_LOG(ERR, ss);
		ret = -1;
	}


	return ret;
}

int
IPCManager_::register_at_dif(const Addon* callee, const int ipcp_id,
			    const rina::ApplicationProcessNamingInformation&
			    dif_name)
{
	// Select a slave (N-1) IPC process.
	rina::IPCProcess *ipcp, *slave_ipcp;
	ostringstream ss;
	unsigned int seqnum;
	int ret = -1;
	IPCPregTransState* trans;

	//TODO: move this to a write_lock over the IPCP

	// Try to register @ipcp to the slave IPC process.
	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw Exception();
		}

		slave_ipcp = select_ipcp_by_dif(dif_name);

		if (!slave_ipcp) {
			ss << "Cannot find any IPC process belonging "
			   << "to DIF " << dif_name.toString()
			   << endl;
			FLUSH_LOG(ERR, ss);
			throw Exception();
		}

		//Create a transaction
		seqnum = opaque_generator_.next();
		trans = new IPCPregTransState(callee, seqnum, ipcp->id,
							slave_ipcp->id);

		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! "
				<< dif_name.toString();
			throw Exception();
		}

		//Store transaction
		if(add_transaction_state(seqnum, trans) < 0){
			ss << "Unable to add transaction; out of memory? "
				<< dif_name.toString();
			throw Exception();
		}

		//Register
		slave_ipcp->registerApplication(
				ipcp->name, ipcp->id, seqnum);

		ss << "Requested DIF registration of IPC process " <<
			ipcp->name.toString() << " at DIF " <<
			dif_name.toString() << " through IPC process "
		   << slave_ipcp->name.toString()
		   << endl;
		FLUSH_LOG(INFO, ss);

		//If it is a synchronous call, wait until it has been finished
		if(!callee){
			//Just wait in the condition variable
			//TODO: move this to a timedwait
			trans->wait();

			//Callback
			//TODO
		}
	} catch (Exception) {
		ss  << ": Error while requesting registration"
		    << endl;
		FLUSH_LOG(ERR, ss);
		ret = -1;
	}

	return ret;
}

int IPCManager_::register_at_difs(const Addon* callee, const int ipcp_id,
		const list<rina::ApplicationProcessNamingInformation>& difs)
{

	rina::IPCProcess *ipcp;
	ostringstream ss;
	int ret = 0;

	//TODO: move this to a write_lock over the IPCP

	try{
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
                	FLUSH_LOG(ERR, ss);
			throw Exception();
		}

		for (list<rina::ApplicationProcessNamingInformation>::const_iterator
				nit = difs.begin(); nit != difs.end(); nit++) {
			register_at_dif(callee, ipcp_id, *nit);
		}
	} catch (Exception) {
		ss  << ": Unknown error while requesting registration at dif"
		    << endl;
		FLUSH_LOG(ERR, ss);
		ret = -1;
	}

	return ret;
}

int
IPCManager_::unregister_ipcp_from_ipcp(const Addon* callee, int ipcp_id,
                                      int slave_ipcp_id)
{
        unsigned int seqnum;
        bool arrived = true;
        ostringstream ss;
        rina::IPCProcess *ipcp, *slave_ipcp;
	IPCPregTransState* trans;
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

		//Create a transaction
		seqnum = opaque_generator_.next();
		trans = new IPCPregTransState(callee, seqnum, ipcp->id,
							slave_ipcp->id);

		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			throw Exception();
		}

		//Store transaction
		if(add_transaction_state(seqnum, trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			throw Exception();
		}


                // Forward the unregistration request to the IPC process
                // that the client IPC process is registered to
                slave_ipcp->unregisterApplication(ipcp->name, seqnum);

                ss << "Requested unregistration of IPC process " <<
                        ipcp->name.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
                FLUSH_LOG(INFO, ss);

		//If it is a synchronous call, wait until it has been finished
		if(!callee){
			//Just wait in the condition variable
			//TODO: move this to a timedwait
			trans->wait();

			//Callback
			//TODO
		}
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

int
IPCManager_::enroll_to_dif(const Addon* callee, const int ipcp_id,
			  const rinad::NeighborData& neighbor,
			  bool sync)
{
	ostringstream ss;
	rina::IPCProcess *ipcp;
	int ret = 0;
	unsigned int seqnum;
	IPCPTransState* trans;

	//TODO: move this to a write_lock over the IPCP

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
                	FLUSH_LOG(ERR, ss);
			throw Exception();
		}


		//Create a transaction
		seqnum = opaque_generator_.next();
		trans = new IPCPTransState(callee, seqnum, ipcp->id);

		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! "
				<< neighbor.difName.toString(); 
			throw Exception();
		}

		//Store transaction
		if(add_transaction_state(seqnum, trans) < 0){
			ss << "Unable to add transaction; out of memory? "
				<< neighbor.difName.toString();
			throw Exception();
		}

		ipcp->enroll(neighbor.difName,
				neighbor.supportingDifName,
				neighbor.apName, seqnum);

		ss << "Requested enrollment of IPC process " <<
			ipcp->name.toString() << " to DIF " <<
			neighbor.difName.toString() << " through DIF "
			<< neighbor.supportingDifName.toString() <<
			" and neighbor IPC process " <<
			neighbor.apName.toString() << endl;
		FLUSH_LOG(INFO, ss);

		//If it is a synchronous call, wait until it has been finished
		if(!callee){
			//Just wait in the condition variable
			//TODO: move this to a timedwait
			trans->wait();

			//Callback
			//TODO
		}
	} catch (rina::EnrollException) {
		ss  << ": Error while enrolling "
			<< "to DIF " << neighbor.difName.toString()
			<< endl;
		FLUSH_LOG(ERR, ss);
		ret = -1;
	}

	return ret;
}

int IPCManager_::enroll_to_difs(const Addon* callee, const int ipcp_id,
			       const list<rinad::NeighborData>& neighbors)
{
	ostringstream ss;
	rina::IPCProcess *ipcp;
	int ret = -1;

	//TODO: move this to a write_lock over the IPCP

	try{
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
                	FLUSH_LOG(ERR, ss);
			throw Exception();
		}

		for (list<rinad::NeighborData>::const_iterator
				nit = neighbors.begin();
					nit != neighbors.end(); nit++) {
			enroll_to_dif(callee, ipcp_id, *nit, false);
		}
	} catch (Exception) {
		ss  << ": Unknown error while enrolling to difs"
		    << endl;
		FLUSH_LOG(ERR, ss);
		ret = -1;
	}

	return ret;
}

bool IPCManager_::lookup_dif_by_application(
	const rina::ApplicationProcessNamingInformation& apName,
	rina::ApplicationProcessNamingInformation& difName){
	return config.lookup_dif_by_application(apName, difName);
}

int
IPCManager_::apply_configuration()
{
	ostringstream ss;
	list<int> ipcps;
	list<rinad::IPCProcessToCreate>::iterator cit;
	list<int>::iterator pit;

	//TODO: move this to a write_lock over the IPCP

	try{
		//FIXME TODO XXX this method needs to be heavily refactored
		//It is not clear which exceptions can be thrown and what to do
		//if this happens. Just protecting to prevent dead-locks.

		// Examine all the IPCProcesses that are going to be created
		// according to the configuration file.
		for (cit = config.ipcProcessesToCreate.begin();
		     cit != config.ipcProcessesToCreate.end(); cit++) {
			std::string	type;
			ostringstream      ss;
			int ipcp_id;

			if (!config.lookup_type_by_dif(cit->difName, type)) {
				ss << "Failed to retrieve DIF type for "
				   << cit->name.toString() << endl;
				FLUSH_LOG(ERR, ss);
				continue;
			}

			try {
				ipcp_id = create_ipcp(NULL, cit->name, type);
				if (ipcp_id < 0) {
					continue;
				}
				assign_to_dif(NULL, ipcp_id, cit->difName);
				register_at_difs(NULL, ipcp_id, cit->difsToRegisterAt);
			} catch (Exception &e) {
				LOG_ERR("Exception while applying configuration: %s",
					e.what());
				return -1;
			}

			ipcps.push_back(ipcp_id);
		}

		// Perform all the enrollments specified by the configuration file.
		for (pit = ipcps.begin(), cit = config.ipcProcessesToCreate.begin();
		     pit != ipcps.end();
		     pit++, cit++) {
			enroll_to_difs(NULL, *pit, cit->neighbors);
		}
	} catch (Exception &e) {
		LOG_ERR("Exception while applying configuration: %s",
								e.what());
		return -1;
	}

	return 0;
}

int
IPCManager_::update_dif_configuration(const Addon* callee, int ipcp_id,
				     const rina::DIFConfiguration & dif_config)
{
	ostringstream ss;
	int ret = 0;
	unsigned int seqnum;
	rina::IPCProcess *ipcp;
	IPCPTransState* trans;

	//TODO: move this to a write_lock over the IPCP

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
                	FLUSH_LOG(ERR, ss);
			throw Exception();
		}


		// Request a configuration update for the IPC process
		/* FIXME The meaning of this operation is unclear: what
		 * configuration is modified? The configuration of the
		 * IPC process only or the configuration of the whole DIF
		 * (which possibly contains more IPC process, both on the same
		 * processing systems and on different processing systems) ?
		 */
		seqnum = opaque_generator_.next();
		trans = new IPCPTransState(callee, seqnum, ipcp->id);

		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			throw Exception();
		}

		//Store transaction
		if(add_transaction_state(seqnum, trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			throw Exception();
		}
		ipcp->updateDIFConfiguration(dif_config, seqnum);

		ss << "Requested configuration update for IPC process " <<
			ipcp->name.toString() << endl;
		FLUSH_LOG(INFO, ss);

		//If it is a synchronous call, wait until it has been finished
		if(!callee){
			//Just wait in the condition variable
			//TODO: move this to a timedwait
			trans->wait();

			//Callback
			//TODO
		}
	} catch (rina::UpdateDIFConfigurationException) {
		ss  << ": Error while updating DIF configuration "
			" for IPC process " << ipcp->name.toString() << endl;
		FLUSH_LOG(ERR, ss);
	}

	return ret;
}

std::string
IPCManager_::query_rib(const Addon* callee, const int ipcp_id)
{
	std::string retstr = "Query RIB operation was not successful";
	ostringstream ss;
	int ret = -1;
	rina::IPCProcess *ipcp;
	unsigned int seqnum;
	RIBqTransState* trans;

	//TODO: move this to a read_lock over the IPCP

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
                	FLUSH_LOG(ERR, ss);
			throw Exception();
		}

		seqnum = opaque_generator_.next();
		trans = new RIBqTransState(callee, seqnum, ipcp->id);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			throw Exception();
		}

		//Store transaction
		if(add_transaction_state(seqnum, trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			throw Exception();
		}

		ipcp->queryRIB("", "", 0, 0, "", seqnum);

		ss << "Requested query RIB of IPC process " <<
			ipcp->name.toString() << endl;
		FLUSH_LOG(INFO, ss);

		//If it is a synchronous call, wait until it has been finished
		if(!callee){
			//Just wait in the condition variable
			//TODO: move this to a timedwait
			trans->wait();

			//Callback
			//TODO

			//Copy value
			retstr = trans->result;

			//Remove and destroy
			remove_transaction_state(trans->tid);
			delete trans;
			return 	retstr;
		}else{
			return std::string("");
		}
	} catch (rina::QueryRIBException) {
		ss << "Error while querying RIB of IPC Process " <<
			ipcp->name.toString() << endl;
		FLUSH_LOG(ERR, ss);
	}

	return retstr;
}

std::string IPCManager_::get_log_level() const
{
	return log_level_;
}


int
IPCManager_::set_policy_set_param(const Addon* callee, const int ipcp_id,
                                 const std::string& component_path,
                                 const std::string& param_name,
                                 const std::string& param_value)
{
        ostringstream ss;
        unsigned int seqnum;
        int ret = 0;
	rina::IPCProcess *ipcp;
	IPCPTransState* trans;

	//TODO: move this to a write_lock over the IPCP

        try {
        	ipcp = lookup_ipcp_by_id(ipcp_id);

        	if(!ipcp){
        		ss << "Invalid IPCP id "<< ipcp_id;
        		FLUSH_LOG(ERR, ss);
        		throw Exception();
        	}

		seqnum = opaque_generator_.next();
		trans = new IPCPTransState(callee, seqnum, ipcp->id);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			throw Exception();
		}

		//Store transaction
		if(add_transaction_state(seqnum, trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			throw Exception();
		}

        	ipcp->setPolicySetParam(component_path,
        			param_name, param_value, seqnum);

        	ss << "Issued set-policy-set-param to IPC process " <<
        			ipcp->name.toString() << endl;
        	FLUSH_LOG(INFO, ss);

		//If it is a synchronous call, wait until it has been finished
		if(!callee){
			//Just wait in the condition variable
			//TODO: move this to a timedwait
			trans->wait();

			//Callback
			//TODO
		}
        } catch (rina::SetPolicySetParamException) {
                ss << "Error while issuing set-policy-set-param request "
                        "to IPC Process " << ipcp->name.toString() << endl;
                FLUSH_LOG(ERR, ss);
		ret = -1;
        }

        return ret;
}

int
IPCManager_::select_policy_set(const Addon* callee, const int ipcp_id,
                              const std::string& component_path,
                              const std::string& ps_name)
{
        ostringstream ss;
        unsigned int seqnum;
        int ret = 0;
	rina::IPCProcess *ipcp;
	IPCPTransState* trans;

        try {
        	ipcp = lookup_ipcp_by_id(ipcp_id);

        	if(!ipcp){
        		ss << "Invalid IPCP id "<< ipcp_id;
        		FLUSH_LOG(ERR, ss);
        		throw Exception();
        	}

        	seqnum = opaque_generator_.next();
		trans = new IPCPTransState(callee, seqnum, ipcp->id);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			throw Exception();
		}

		//Store transaction
		if(add_transaction_state(seqnum, trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			throw Exception();
		}

        	ipcp->selectPolicySet(component_path, ps_name, seqnum);

        	//pending_select_policy_set_ops[seqnum] = ipcp;
        	ss << "Issued select-policy-set to IPC process " <<
        			ipcp->name.toString() << endl;
        	FLUSH_LOG(INFO, ss);

		if(!callee){
			//Just wait in the condition variable
			//TODO: move this to a timedwait
			trans->wait();

			//Callback
			//TODO
		}
        } catch (rina::SelectPolicySetException) {
                ss << "Error while issuing select-policy-set request "
                        "to IPC Process " << ipcp->name.toString() << endl;
                FLUSH_LOG(ERR, ss);
		ret = -1;
        }

        return ret;
}

int
IPCManager_::plugin_load(const Addon* callee, const int ipcp_id,
                        const std::string& plugin_name, bool load)
{
        ostringstream ss;
        unsigned int seqnum;
        int ret = 0;
	rina::IPCProcess *ipcp;
	IPCPTransState* trans;

	//TODO: move this to a write_lock over the IPCP

        try {
        	ipcp = lookup_ipcp_by_id(ipcp_id);

        	if(!ipcp){
        		ss << "Invalid IPCP id "<< ipcp_id;
        		FLUSH_LOG(ERR, ss);
        		throw Exception();
        	}

        	seqnum = opaque_generator_.next();
		trans = new IPCPTransState(callee, seqnum, ipcp->id);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			throw Exception();
		}
		//Store transaction
		if(add_transaction_state(seqnum, trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			throw Exception();
		}
        	ipcp->pluginLoad(plugin_name, load, seqnum);

        	ss << "Issued plugin-load to IPC process " <<
        			ipcp->name.toString() << endl;
        	FLUSH_LOG(INFO, ss);

		if(!callee){
			//Just wait in the condition variable
			//TODO: move this to a timedwait
			trans->wait();

			//Callback
			//TODO
		}
        } catch (rina::PluginLoadException) {
                ss << "Error while issuing plugin-load request "
                        "to IPC Process " << ipcp->name.toString() << endl;
                FLUSH_LOG(ERR, ss);
		ret = -1;
        }

        return ret;
}

int
IPCManager_::unregister_app_from_ipcp(const Addon* callee,
                const rina::ApplicationUnregistrationRequestEvent& req_event,
                int slave_ipcp_id)
{
        ostringstream ss;
        unsigned int seqnum;
        rina::IPCProcess *slave_ipcp;
	IPCPTransState* trans;

        try {
		slave_ipcp = lookup_ipcp_by_id(slave_ipcp_id);

		if (!slave_ipcp) {
			ss << "Cannot find any IPC process belonging "<<endl;
			FLUSH_LOG(ERR, ss);
			throw Exception();
		}

                // Forward the unregistration request to the IPC process
                // that the application is registered to
		seqnum = opaque_generator_.next();

        	seqnum = opaque_generator_.next();
		trans = new IPCPTransState(callee, seqnum, slave_ipcp->id);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			throw Exception();
		}

		//Store transaction
		if(add_transaction_state(seqnum, trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			throw Exception();
		}
                slave_ipcp->unregisterApplication(req_event.applicationName,
                                                  seqnum);
                ss << "Requested unregistration of application " <<
                        req_event.applicationName.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
                FLUSH_LOG(INFO, ss);

		if(!callee){
			//Just wait in the condition variable
			//TODO: move this to a timedwait
			trans->wait();

			//Callback
			//TODO
		}
        } catch (rina::IpcmUnregisterApplicationException) {
                ss  << ": Error while unregistering application "
                        << req_event.applicationName.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        return 0;
}

//
// Transactions
//

//State management routines
int IPCManager_::add_transaction_state(int tid, TransactionState* t){
	//Rwlock: write

	//Check if exists already
	if ( pend_transactions.find(tid) != pend_transactions.end() ){
		assert(0); //Transaction id repeated
		return -1;
	}

	//Just add and return
	try{
		pend_transactions[tid] = t;
	}catch(...){
		LOG_DBG("Could not add transaction %u. Out of memory?", tid);
		assert(0);
		return -1;
	}
	return 0;
}

int IPCManager_::remove_transaction_state(int tid){
	//Rwlock: write

	//Check if it really exists
	if ( pend_transactions.find(tid) == pend_transactions.end() )
		return -1;

	pend_transactions.erase(tid);

	return 0;
}

//Syscall state management routines
int IPCManager_::add_syscall_transaction_state(int tid, TransactionState* t){
	//Rwlock: write

	//Check if exists already
	if ( pend_sys_trans.find(tid) != pend_sys_trans.end() ){
		assert(0); //Transaction id repeated
		return -1;
	}

	//Just add and return
	try{
		pend_sys_trans[tid] = t;
	}catch(...){
		LOG_DBG("Could not add syscall transaction %u. Out of memory?",
									 tid);
		assert(0);
		return -1;
	}
	return 0;
}

int IPCManager_::remove_syscall_transaction_state(int tid){
	//Rwlock: write

	//Check if it really exists
	if ( pend_sys_trans.find(tid) == pend_sys_trans.end() )
		return -1;

	pend_sys_trans.erase(tid);

	return 0;
}


//Main I/O loop
void IPCManager_::run(){

	rina::IPCEvent *event;

	keep_running = true;

	LOG_DBG("Starting main I/O loop...");

	while(keep_running) {
		event = rina::ipcEventProducer->eventTimedWait(
						IPCMANAGER_TIMEOUT_S,
						IPCMANAGER_TIMEOUT_NS);
		if(!event)
			continue;

		LOG_DBG("Got event of type %s and sequence number %u",
		rina::IPCEvent::eventTypeToString(event->eventType).c_str(),
							event->sequenceNumber);

		if (!event) {
			std::cerr << "Null event received" << std::endl;
			continue;
		}

		try {
			//TODO: Locking?

			switch(event->eventType){
				case rina::FLOW_ALLOCATION_REQUESTED_EVENT:
						flow_allocation_requested_event_handler(event);
						break;

				case rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT:
						allocate_flow_request_result_event_handler(event);
						break;

				case rina::ALLOCATE_FLOW_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::AllocateFlowResponseEvent, e);
						allocate_flow_response_event_handler(e);
						}
						break;

				case rina::FLOW_DEALLOCATION_REQUESTED_EVENT:
						flow_deallocation_requested_event_handler(event);
						break;

				case rina::DEALLOCATE_FLOW_RESPONSE_EVENT:
						deallocate_flow_response_event_handler(event);
						break;

				case rina::APPLICATION_UNREGISTERED_EVENT:
						application_unregistered_event_handler(event);
						break;

				case rina::FLOW_DEALLOCATED_EVENT:
						IPCManager->flow_deallocated_event_handler(event);
						break;

				case rina::APPLICATION_REGISTRATION_REQUEST_EVENT:
						{
        					DOWNCAST_DECL(event, rina::ApplicationRegistrationRequestEvent, e);
						app_reg_req_handler(e);
						}
						break;

				case rina::REGISTER_APPLICATION_RESPONSE_EVENT:
						register_application_response_event_handler(event);
						break;

				case rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT:
						application_unregistration_request_event_handler(event);
						break;

				case rina::UNREGISTER_APPLICATION_RESPONSE_EVENT:
						unregister_application_response_event_handler(event);
						break;

				case rina::APPLICATION_REGISTRATION_CANCELED_EVENT:
						application_registration_canceled_event_handler(event);
						break;

				case rina::ASSIGN_TO_DIF_REQUEST_EVENT:
						assign_to_dif_request_event_handler(event);
						break;

				case rina::ASSIGN_TO_DIF_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::AssignToDIFResponseEvent, e);
						assign_to_dif_response_event_handler(e);
						}
						break;

				case rina::UPDATE_DIF_CONFIG_REQUEST_EVENT:
						update_dif_config_request_event_handler(event);
						break;

				case rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::UpdateDIFConfigurationResponseEvent, e);
						update_dif_config_response_event_handler(e);
						}
						break;

				case rina::ENROLL_TO_DIF_REQUEST_EVENT:
						enroll_to_dif_request_event_handler(event);
						break;

				case rina::ENROLL_TO_DIF_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::EnrollToDIFResponseEvent, e);
						enroll_to_dif_response_event_handler(e);
						}
						break;

				case rina::NEIGHBORS_MODIFIED_NOTIFICATION_EVENT:
						{
        					DOWNCAST_DECL(event, rina::NeighborsModifiedNotificationEvent, e);
						neighbors_modified_notification_event_handler(e);
						}
						break;

				case rina::IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION:
						ipc_process_dif_registration_notification_handler(event);
						break;

				case rina::IPC_PROCESS_QUERY_RIB:
						{
						DOWNCAST_DECL(event, rina::QueryRIBResponseEvent, e);
						ipc_process_query_rib_handler(e);
						}
						break;

				case rina::GET_DIF_PROPERTIES:
						get_dif_properties_handler(event);
						break;

				case rina::GET_DIF_PROPERTIES_RESPONSE_EVENT:
						get_dif_properties_response_event_handler(event);
						break;

				case rina::OS_PROCESS_FINALIZED:
						os_process_finalized_handler(event);
						break;

				case rina::IPCM_REGISTER_APP_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::IpcmRegisterApplicationResponseEvent, e);
						app_reg_response_handler(e);
						}
						break;

				case rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::IpcmUnregisterApplicationResponseEvent, e);
						unreg_app_response_handler(e);
						}
						break;

				case rina::IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::IpcmDeallocateFlowResponseEvent, e);
						ipcm_deallocate_flow_response_event_handler(e);
						}
						break;

				case rina::IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
						{
        					DOWNCAST_DECL(event, rina::IpcmAllocateFlowRequestResultEvent, e);
						ipcm_allocate_flow_request_result_handler(e);
						}
						break;

				case rina::QUERY_RIB_RESPONSE_EVENT:
						{
						DOWNCAST_DECL(event, rina::QueryRIBResponseEvent, e);
						query_rib_response_event_handler(e);
						}
						break;

				case rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT:
						{
						DOWNCAST_DECL(event, rina::IPCProcessDaemonInitializedEvent, e);
						ipc_process_daemon_initialized_event_handler(e);
						}
						break;

				case rina::TIMER_EXPIRED_EVENT:
						timer_expired_event_handler(event);
						break;

				case rina::IPC_PROCESS_CREATE_CONNECTION_RESPONSE:
						ipc_process_create_connection_response_handler(event);
						break;

				case rina::IPC_PROCESS_UPDATE_CONNECTION_RESPONSE:
						ipc_process_update_connection_response_handler(event);
						break;

				case rina::IPC_PROCESS_CREATE_CONNECTION_RESULT:
						ipc_process_create_connection_result_handler(event);
						break;

				case rina::IPC_PROCESS_DESTROY_CONNECTION_RESULT:
						ipc_process_destroy_connection_result_handler(event);
						break;

				case rina::IPC_PROCESS_DUMP_FT_RESPONSE:
						ipc_process_dump_ft_response_handler(event);
						break;

				//Policies
				case rina::IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE:
					ipc_process_set_policy_set_param_response_handler(event);
					break;
				case rina::IPC_PROCESS_SELECT_POLICY_SET_RESPONSE:
					ipc_process_select_policy_set_response_handler(event);
					break;
				case rina::IPC_PROCESS_PLUGIN_LOAD_RESPONSE:
					ipc_process_plugin_load_response_handler(event);
					break;
			}

		} catch (Exception &e) {
			LOG_ERR("ERROR while processing event: %s", e.what());
			//TODO: move locking to a smaller scope
		}

		delete event;
	}

	//TODO: probably move this to a private method if it starts to grow
	LOG_DBG("Stopping I/O loop and cleaning the house...");

	//Destroy all IPCPs
	const std::vector<rina::IPCProcess *>& ipcps =
		rina::ipcProcessFactory->listIPCProcesses();
	std::vector<rina::IPCProcess *>::const_iterator it;

	//Rwlock: write
	for(it = ipcps.begin(); it != ipcps.end(); ++it){
		if(destroy_ipcp(NULL, (*it)->id) < 0 ){
			LOG_WARN("Warning could not destroy IPCP id: %d\n",
								(*it)->id);
		}
	}
}

} //rinad namespace
