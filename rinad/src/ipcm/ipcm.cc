/*
 * IPC Manager
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <iterator>

#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#include <google/protobuf/stubs/common.h>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/plugin-info.h>
#include <librina/concurrency.h>

#define RINA_PREFIX "ipcm"
#include <librina/logs.h>
#include <debug.h>

#include "rina-configuration.h"
#include "ipcm.h"
#include "dif-validator.h"
#include "app-handlers.h"

//Addons
#include "addons/console.h"
#include "addons/scripting.h"
#include "addons/ma/agent.h"
//[+] Add more here...

//IPCM IPCP
#include "ipcp.h"

//Handler specifics (transaction states)
#include "ipcp-handlers.h"
#include "flow-alloc-handlers.h"
#include "misc-handlers.h"

//Timeouts for timed wait
#define IPCM_EVENT_TIMEOUT_S 0
#define IPCM_EVENT_TIMEOUT_NS 100000000 //0.1 sec
#define IPCM_TRANS_TIMEOUT_S 7

//Downcast MACRO
#ifndef DOWNCAST_DECL
// Useful MACRO to perform downcasts in declarations.
#define DOWNCAST_DECL(_var,_class,_name)\
		_class *_name = dynamic_cast<_class*>(_var);
#endif //DOWNCAST_DECL

namespace rinad {

//
//IPCManager_
//

//Singleton instance
Singleton<IPCManager_> IPCManager;

IPCManager_::IPCManager_()
        : req_to_stop(false),
          io_thread(NULL),
          dif_template_manager(NULL),
          dif_allocator(NULL),
	  osp_monitor(NULL),
	  ip_vpn_manager(NULL)
{
	rina::removedir_all("/tmp/rina");
	rina::createdir("/tmp/rina");
	rina::createdir("/tmp/rina/ipcps");
}

IPCManager_::~IPCManager_()
{
	if (dif_template_manager) {
		delete dif_template_manager;
	}

	if (dif_allocator) {
		delete dif_allocator;
	}

	if (ip_vpn_manager) {
	        delete ip_vpn_manager;
	}

	forwarded_calls.clear();

	for (std::map<int, TransactionState*>::iterator
			it = pend_transactions.begin(); it != pend_transactions.end(); ++it) {
		delete it->second;
	}

	rina::removedir_all("/tmp/rina");
}

void IPCManager_::init(const std::string& loglevel, std::string& config_file)
{
    // Initialize the IPC manager infrastructure in librina.

    try
    {
        rina::initializeIPCManager(IPCM_CTRLDEV_PORT, config.local.installationPath,
                                   config.local.libraryPath, loglevel,
                                   config.local.logPath);
        LOG_DBG("IPC Manager daemon initialized");
        LOG_DBG("       installation path: %s",
                config.local.installationPath.c_str());
        LOG_DBG("       library path: %s", config.local.libraryPath.c_str());
        LOG_DBG("       log folder: %s", config.local.logPath.c_str());

        // Load the plugins catalog
        catalog.import();
        //catalog.print();

        // Initialize the I/O thread
        io_thread = new rina::Thread(io_loop_trampoline, NULL,
                                     std::string("ipcm-io-thread"), false);
        io_thread->start();

        // Initialize DIF Allocator
        dif_allocator = DIFAllocator::create_instance(config, this);

        // Initialize DIF Templates Manager (with its monitor thread)
        dif_template_manager = new DIFTemplateManager(config_file,
                                                      dif_allocator);

        // Initialize OS Process Monitor
        osp_monitor = new OSProcessMonitor();
        osp_monitor->start();

        // Initialize IP VPN Manager
        ip_vpn_manager = new IPVPNManager();
    } catch (rina::InitializationException& e)
    {
        LOG_ERR("Error while initializing librina-ipc-manager");
        exit (EXIT_FAILURE);
    }
}

void IPCManager_::request_finalization(void)
{
	try {
		rina::request_ipcm_finalization(IPCM_CTRLDEV_PORT);
	} catch (rina::IPCException &e) {
	        LOG_ERR("Error while requesting IPCM finalization");
	        rina::librina_finalize();
	        exit (EXIT_FAILURE);
	}
}

void IPCManager_::load_addons(const std::string& addon_list)
{

    std::string al = addon_list;

    //Convert the list of addons to lowercase
    std::transform(al.begin(), al.end(), al.begin(), ::tolower);

    //Split comma based, and remove extra chars (spaces)
    std::stringstream ss(al);
    std::string t;
    while (std::getline(ss, t, ','))
    {
        //Remove whitespaces
        t.erase(std::remove_if(t.begin(), t.end(), ::isspace), t.end());

        //Call the factory
        Addon::factory(config, t);
    }
}

/*
 *
 * Public API
 *
 */

ipcm_res_t IPCManager_::create_ipcp(Addon* callee, CreateIPCPPromise* promise,
				    rina::ApplicationProcessNamingInformation& name,
				    const std::string& type, const std::string& dif_name)
{
	IPCMIPCProcess *ipcp;
	std::ostringstream ss;
	rina::IPCProcessFactory fact;
	std::list < std::string > ipcp_types;
	bool difCorrect = false;
	std::string s;
	SyscallTransState* trans;
	int ipcp_id = -1;

	// Let the DIF Allocator generate the IPCP name
	// If sysname is not set we assume the old configuration
	// method is used (IPCP name is explicitly set)
	dif_allocator->generate_ipcp_name(name, dif_name);

	try
	{
		// Check that the AP name is not empty
		if (name.processName == std::string(""))
		{
			ss << "Cannot create IPC process with an "
					"empty AP name" << std::endl;
			FLUSH_LOG(ERR, ss);
			throw rina::CreateIPCProcessException();
		}

		//Check if dif type exists
		list_ipcp_types (ipcp_types);
		if (std::find(ipcp_types.begin(), ipcp_types.end(), type)
		== ipcp_types.end())
		{
			const char* const separator = ", ";
			ss << "IPCP type parameter " << type.c_str()
                		    << " is wrong, options are: [" << s;
			// actually list the optons
			std::copy(ipcp_types.begin(),
					ipcp_types.end(),
					std::ostream_iterator<std::string>(ss, separator));
			ss << "]";
			FLUSH_LOG(ERR, ss);
			throw rina::CreateIPCProcessException();
		}

		rina::ScopedLock g(req_lock);

		//Call the factory
		ipcp = ipcp_factory_.create(name, type);
		ipcp_id = ipcp->get_id();
		pending_cipcp_req[ipcp->proxy_->seq_num] = ipcp_id;

		//Set the promise
		if (promise)
			promise->ipcp_id = ipcp_id;

		//TODO: this should be moved to the factory
		//Moreover the API should be homgenized such that the
		if (type != rina::NORMAL_IPC_PROCESS &&
				type != rina::SHIM_WIFI_IPC_PROCESS_AP &&
				type != rina::SHIM_WIFI_IPC_PROCESS_STA)
		{
			// Shim IPC processes are set as initialized
			// immediately.
			ipcp->setInitialized();
		}

		//Release the lock asap
		ipcp->rwlock.unlock();

		// Normal IPC processes can be set as
		// initialized only when the corresponding
		// IPC process daemon is initialized, so we
		// defer the operation.

		//Add transaction state
		trans = new SyscallTransState(callee, promise, ipcp_id);
		if (!trans)
		{
			assert(0);
			ss << "Failed to create IPC process '" << name.toString()
                        				<< "' of type '" << type << "'. Out of memory!"
							<< std::endl;
			FLUSH_LOG(ERR, ss);
			FLUSH_LOG(ERR, ss);
			throw rina::CreateIPCProcessException();
		}

		if (add_syscall_transaction_state(trans) < 0)
		{
			assert(0);
			throw rina::CreateIPCProcessException();
		}
		//Show a nice trace
		ss << "IPC process " << name.toString()
                				    << " created and waiting for initialization"
						    "[id = " << ipcp_id << "]" << std::endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::ConcurrentException& e)
	{
		ss << "Failed to create IPC process '" << name.toString()
                		<< "' of type '" << type << "'. Transaction timed out"
				<< std::endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (...)
	{
		ss << "Failed to create IPC process '" << name.toString()
                		<< "' of type '" << type << "'" << std::endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t IPCManager_::destroy_ipcp(Addon* callee, unsigned short ipcp_id)
{
    std::ostringstream ss;
    unsigned int seq_num = 0;

    //Distribute the event to the addons
    IPCMEvent addon_e(callee, IPCM_IPCP_TO_BE_DESTROYED, ipcp_id);
    Addon::distribute_ipcm_event(addon_e);

    rina::ScopedLock g(req_lock);

    try
    {
        seq_num = ipcp_factory_.destroy(ipcp_id);
        pending_dipcp_req[seq_num] = ipcp_id;
        ss << "IPC process destroyed [id = " << ipcp_id << "]" << std::endl;
        FLUSH_LOG(INFO, ss);
    } catch (rina::DestroyIPCProcessException& e)
    {
        ss << ": Error while destroying IPC "
                "process with id " << ipcp_id << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    }

    // Synchronize the catalog state, so that
    // the ipcp_id of the IPCP just destroyed can
    // be reused without inconsistencies in the catalog
    catalog.ipcp_destroyed(ipcp_id);

    return IPCM_SUCCESS;
}

void IPCManager_::list_ipcps(std::ostream& os)
{
    if (mad::ManagementAgent::inst != NULL) {
	    std::list<std::string> args;
	    os << mad::ManagementAgent::inst->console_command(mad::LIST_MAD_STATE, args);
    } else {
	    os << "Management Agent not started" << std::endl;
	    os << std::endl;
    }

    //Prevent any insertion/deletion to happen
    rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

    std::vector<IPCMIPCProcess *> ipcps;
    ipcp_factory_.listIPCProcesses(ipcps);

    os
            << "Current IPC processes (id | name | type | state | Registered applications | Port-ids of flows provided)"
            << std::endl;
    for (unsigned int i = 0; i < ipcps.size(); i++)
    {
        ipcps[i]->get_description(os);
    }
}

void IPCManager_::list_da_mappings(std::ostream& os)
{
	dif_allocator->list_da_mappings(os);
}

std::string IPCManager_::query_ma_rib()
{
	std::stringstream ss;

	if (mad::ManagementAgent::inst != NULL) {
		std::list<std::string> args;
		ss << mad::ManagementAgent::inst->console_command(mad::QUERY_MAD_RIB, args);
	} else {
		ss << "Management Agent not started" << std::endl;
		ss << std::endl;
	}

	return ss.str();
}

//NOTE: this assumes an empty name is invalid as a return value for
//could not complete the operation
std::string IPCManager_::get_ipcp_name(int ipcp_id)
{
    //Prevent any insertion/deletion to happen
    rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

    IPCMIPCProcess* ipcp = lookup_ipcp_by_id(ipcp_id, false);

    if (!ipcp)
        return "";

    //Prevent any insertion/deletion to happen
    rina::ReadScopedLock rreadlock(ipcp->rwlock, false);

    if (!ipcp)
        return std::string("");

    return ipcp->get_name().processName;
}

void IPCManager_::list_ipcps(std::list<int>& list)
{
    //Prevent any insertion/deletion to happen
    rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

    //Call the factory
    std::vector<IPCMIPCProcess*> ipcps;
    ipcp_factory_.listIPCProcesses(ipcps);

    //Compose the list
    std::vector<IPCMIPCProcess*>::const_iterator it;
    for (it = ipcps.begin(); it != ipcps.end(); ++it)
        list.push_back((*it)->get_id());
}

bool IPCManager_::ipcp_exists(const unsigned short ipcp_id)
{
	return ipcp_factory_.exists(ipcp_id);
}

unsigned short IPCManager_::ipcp_exists_by_pid(pid_t pid)
{
	return ipcp_factory_.exists_by_pid(pid);
}

void IPCManager_::list_ipcp_types(std::list<std::string>& types)
{
	types = ipcp_factory_.getSupportedIPCProcessTypes();
}

//TODO this assumes single IPCP per DIF
int IPCManager_::get_ipcp_by_dif_name(std::string& difName)
{

    IPCMIPCProcess* ipcp;
    int ret;
    rina::ApplicationProcessNamingInformation dif(difName, std::string());

    ipcp = select_ipcp_by_dif(dif);
    if (!ipcp)
        ret = -1;
    else
        ret = ipcp->get_id();

    return ret;
}

void IPCManager_::pre_assign_to_dif(
        Addon* callee,
        const rina::ApplicationProcessNamingInformation& dif_name,
        const unsigned short ipcp_id, IPCMIPCProcess*& ipcp)
{
    std::stringstream ss;
    if (is_any_ipcp_assigned_to_dif(dif_name))
    {
        ss << "There is already an IPCP assigned to DIF " << dif_name.toString()
                << " in this system.";
        FLUSH_LOG(ERR, ss);
        throw rina::AssignToDIFException();
    }

    ipcp = lookup_ipcp_by_id(ipcp_id, true);
    if (!ipcp)
    {
        ss << "Invalid IPCP id " << ipcp_id;
        FLUSH_LOG(ERR, ss);
        throw rina::AssignToDIFException();
    }

    //Auto release the write lock
    rina::WriteScopedLock writelock(ipcp->rwlock, false);
}

void IPCManager_::assign_to_dif(Addon* callee, Promise* promise,
                                rina::DIFInformation dif_info,
                                IPCMIPCProcess* ipcp)
{
    std::stringstream ss;
    IPCPTransState* trans;
    // Validate the parameters
    DIFConfigValidator validator(dif_info, ipcp->get_type());
    if (!validator.validateConfigs())
        throw rina::BadConfigurationException(
                "DIF configuration validator failed");

    //Create a transaction
    trans = new IPCPTransState(callee, promise, ipcp->get_id());
    if (!trans)
    {
        ss
                << "Unable to allocate memory for the transaction object. Out of memory! "
                << dif_info.dif_name_.toString();
        FLUSH_LOG(ERR, ss);
        throw rina::AssignToDIFException();
    }

    //Store transaction
    if (add_transaction_state(trans) < 0)
    {
        ss << "Unable to add transaction; out of memory? "
                << dif_info.dif_name_.toString();
        FLUSH_LOG(ERR, ss);
        throw rina::AssignToDIFException();
    }

    ipcp->assignToDIF(dif_info, trans->tid);

    ss << "Requested DIF assignment of IPC process "
            << ipcp->get_name().toString() << " to DIF "
            << dif_info.dif_name_.toString() << std::endl;
    FLUSH_LOG(INFO, ss);
}

ipcm_res_t IPCManager_::assign_to_dif(
        Addon* callee, Promise* promise, const unsigned short ipcp_id,
        rina::DIFInformation& dif_info,
        const rina::ApplicationProcessNamingInformation &dif_name)
{
    std::ostringstream ss;
    IPCMIPCProcess* ipcp = NULL;

    try
    {
	pre_assign_to_dif(callee, dif_name, ipcp_id, ipcp);
        // Load plugin catalog
        catalog.load(callee, ipcp_id, dif_info.dif_configuration_);
        // assign to diff
        assign_to_dif(callee, promise, dif_info, ipcp);

    } catch (rina::ConcurrentException& e)
    {
	if (ipcp){
		ss << "Error while assigning " << ipcp->get_name().toString()
			<< " to DIF " << dif_name.toString()
			<< ". Operation timedout" << std::endl;
	}else{
		ss << "Error while assigning ipcp to DIF" << std::endl;
	}
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::AssignToDIFException& e)
    {
	if (ipcp){
		ss << "Error while assigning " << ipcp->get_name().toString()
			<< " to DIF " << dif_name.toString() << std::endl;
	}else{
		ss << "Error while assigning ipcp to DIF" << std::endl;
	}
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::BadConfigurationException& e)
    {
        LOG_ERR("Asssign IPCP %d to dif %s failed. Bad configuration.", ipcp_id,
                dif_name.toString().c_str());
        return IPCM_FAILURE;
    } catch (rina::Exception &e)
    {
        LOG_ERR("Asssign IPCP %d to dif %s failed. Unknown error catched: %s:%d",
                ipcp_id, dif_name.toString().c_str(), __FILE__, __LINE__);
        return IPCM_FAILURE;
    }
    return IPCM_PENDING;
}

ipcm_res_t IPCManager_::assign_to_dif(Addon* callee,
				      Promise* promise,
				      const unsigned short ipcp_id,
				      DIFTemplate& dif_template,
				      const rina::ApplicationProcessNamingInformation& dif_name)
{
	rina::DIFInformation dif_info;
	rina::DIFConfiguration dif_config;
	std::ostringstream ss;
	IPCMIPCProcess* ipcp = NULL;

	try
	{
		pre_assign_to_dif(callee, dif_name, ipcp_id, ipcp);

		// Fill in the DIFConfiguration object.
		if (ipcp->get_type() == rina::NORMAL_IPC_PROCESS)
		{
			rina::EFCPConfiguration efcp_config;
			rina::NamespaceManagerConfiguration nsm_config;
			rina::AddressingConfiguration address_config;
			unsigned int address;

			//catalog.load_by_template(callee, ipcp_id, dif_template);
			// FIll in the EFCPConfiguration object.
			efcp_config.set_data_transfer_constants(
					dif_template.dataTransferConstants);
			rina::QoSCube * qosCube = 0;
			for (std::list<rina::QoSCube>::iterator qit = dif_template.qosCubes
					.begin(); qit != dif_template.qosCubes.end(); qit++)
			{
				qosCube = new rina::QoSCube(*qit);
				if (!qosCube)
				{
					ss
					<< "Unable to allocate memory for the QoSCube object. Out of memory! "
					<< dif_name.toString();
					FLUSH_LOG(ERR, ss);
					throw rina::Exception();
				}
				efcp_config.add_qos_cube(qosCube);
			}

			for (std::list<AddressPrefixConfiguration>::iterator ait =
					dif_template.addressPrefixes.begin();
					ait != dif_template.addressPrefixes.end(); ait++)
			{
				rina::AddressPrefixConfiguration prefix;
				prefix.address_prefix_ = ait->addressPrefix;
				prefix.organization_ = ait->organization;
				address_config.address_prefixes_.push_back(prefix);
			}

			for (std::list<rinad::KnownIPCProcessAddress>::iterator kit =
					dif_template.knownIPCProcessAddresses.begin();
					kit != dif_template.knownIPCProcessAddresses.end(); kit++)
			{
				rina::StaticIPCProcessAddress static_address;
				static_address.ap_name_ = kit->name.processName;
				static_address.ap_instance_ = kit->name.processInstance;
				static_address.address_ = kit->address;
				address_config.static_address_.push_back(static_address);
			}
			nsm_config.addressing_configuration_ = address_config;
			nsm_config.policy_set_ = dif_template.nsmConfiguration.policy_set_;

			bool found = dif_template.lookup_ipcp_address(ipcp->get_name(),
					address);
			if (!found)
			{
				ss << "No address for IPC process "
						<< ipcp->get_name().toString() << " in DIF "
						<< dif_name.toString() << std::endl;
				FLUSH_LOG(ERR, ss);
				throw rina::Exception();
			}
			dif_config.efcp_configuration_ = efcp_config;
			dif_config.nsm_configuration_ = nsm_config;
			dif_config.rmt_configuration_ = dif_template.rmtConfiguration;
			dif_config.fa_configuration_ = dif_template.faConfiguration;
			dif_config.ra_configuration_ = dif_template.raConfiguration;
			dif_config.routing_configuration_ = dif_template.routingConfiguration;
			dif_config.sm_configuration_ = dif_template.secManConfiguration;
			dif_config.et_configuration_ = dif_template.etConfiguration;
			dif_config.set_address(address);

			dif_config.sm_configuration_ = dif_template.secManConfiguration;

			// Load plugin catalog
			catalog.load(callee, ipcp_id, dif_config);
		}

		for (std::map<std::string, std::string>::const_iterator pit =
				dif_template.configParameters.begin();
				pit != dif_template.configParameters.end(); pit++)
		{
			dif_config.add_parameter(
					rina::PolicyParameter(pit->first, pit->second));
		}

		// Fill in the DIFInformation object.
		dif_info.set_dif_name(dif_name);
		dif_info.set_dif_type(ipcp->get_type());
		dif_info.set_dif_configuration(dif_config);

		// assign to diff
		assign_to_dif(callee, promise, dif_info, ipcp);

	} catch (rina::ConcurrentException& e)
	{
		if (ipcp){
			ss << "Error while assigning " << ipcp->get_name().toString()
					<< " to DIF " << dif_name.toString()
					<< ". Operation timedout" << std::endl;
		}
		else {
			ss << "Error while assigning ipcp to DIF" << std::endl;
		}
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::AssignToDIFException& e)
	{
		if (ipcp){
			ss << "Error while assigning " << ipcp->get_name().toString()
					<< " to DIF " << dif_name.toString() << std::endl;
		} else {
			ss << "Error while assigning ipcp to DIF" << std::endl;
		}
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::BadConfigurationException& e)
	{
		LOG_ERR("Asssign IPCP %d to dif %s failed. Bad configuration.", ipcp_id,
				dif_name.toString().c_str());
		return IPCM_FAILURE;
	} catch (rina::Exception &e)
	{
		LOG_ERR("Asssign IPCP %d to dif %s failed. Unknown error catched: %s:%d",
				ipcp_id, dif_name.toString().c_str(), __FILE__, __LINE__);
		return IPCM_FAILURE;
	}
	return IPCM_PENDING;
}

ipcm_res_t IPCManager_::register_at_dif(Addon* callee,
					Promise* promise,
					const unsigned short ipcp_id,
					const rina::ApplicationProcessNamingInformation& dif_name)
{
    // Select a slave (N-1) IPC process.
    IPCMIPCProcess *ipcp, *slave_ipcp;
    std::ostringstream ss;
    IPCPregTransState* trans;

    // Try to register @ipcp to the slave IPC process.
    try
    {
        ipcp = lookup_ipcp_by_id(ipcp_id, true);

        if (!ipcp)
        {
            ss << "Invalid IPCP id " << ipcp_id;
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        rina::WriteScopedLock writelock(ipcp->rwlock, false);
        slave_ipcp = select_ipcp_by_dif(dif_name, true);

        if (!slave_ipcp)
        {
            ss << "Cannot find any IPC process belonging " << "to DIF "
                    << dif_name.toString() << std::endl;
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        rina::WriteScopedLock swritelock(slave_ipcp->rwlock, false);

        //Create a transaction
        trans = new IPCPregTransState(callee, promise, ipcp->get_id(),
                                      slave_ipcp->get_id());
        if (!trans)
        {
            ss
                    << "Unable to allocate memory for the transaction object. Out of memory! "
                    << dif_name.toString();
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Store transaction
        if (add_transaction_state(trans) < 0)
        {
            ss << "Unable to add transaction; out of memory? "
                    << dif_name.toString();
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Register
        rina::ApplicationRegistrationInformation ari;
        ari.appName = ipcp->get_name();
        ari.dafName = ipcp->dif_name_;
        ari.difName = slave_ipcp->dif_name_;
        ari.ipcProcessId = ipcp->get_id();
        ari.pid = ipcp->proxy_->pid;
        ari.ctrl_port = ipcp->proxy_->portId;
        slave_ipcp->registerApplication(ari, trans->tid);

        ss << "Requested DIF registration of IPC process "
                << ipcp->get_name().toString() << " at DIF "
                << dif_name.toString() << " through IPC process "
                << slave_ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(INFO, ss);
    } catch (rina::ConcurrentException& e)
    {
        ss << ": Error while requesting registration. Operation timedout"
                << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::Exception& e)
    {
        ss << ": Error while requesting registration: " << e.what()
                << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    }

    return IPCM_PENDING;
}

ipcm_res_t IPCManager_::unregister_ipcp_from_ipcp(
        Addon* callee, Promise* promise, const unsigned short ipcp_id,
        const rina::ApplicationProcessNamingInformation& dif_name)
{
    std::ostringstream ss;
    IPCMIPCProcess *ipcp, *slave_ipcp;
    IPCPregTransState* trans;

    try
    {

        ipcp = lookup_ipcp_by_id(ipcp_id, true);

        if (!ipcp)
        {
            ss << "Invalid IPCP id " << ipcp_id;
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        rina::WriteScopedLock writelock(ipcp->rwlock, false);
        slave_ipcp = select_ipcp_by_dif(dif_name, true);

        if (!slave_ipcp)
        {
            ss << "Cannot find any IPC process belonging " << "to DIF "
                    << dif_name.toString() << std::endl;
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        rina::WriteScopedLock swritelock(slave_ipcp->rwlock, false);

        //Create a transaction
        trans = new IPCPregTransState(callee, promise, ipcp->get_id(),
                                      slave_ipcp->get_id());

        if (!trans)
        {
            ss
                    << "Unable to allocate memory for the transaction object. Out of memory! ";
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Store transaction
        if (add_transaction_state(trans) < 0)
        {
            ss << "Unable to add transaction; out of memory? ";
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        // Forward the unregistration request to the IPC process
        // that the client IPC process is registered to
        slave_ipcp->unregisterApplication(ipcp->get_name(), trans->tid);

        ss << "Requested unregistration of IPC process "
                << ipcp->get_name().toString() << " from IPC "
                        "process " << slave_ipcp->get_name().toString()
                << std::endl;
        FLUSH_LOG(INFO, ss);
    } catch (rina::ConcurrentException& e)
    {
        ss << ": Error while unregistering IPC process "
                << ipcp->get_name().toString() << " from IPC "
                        "process " << slave_ipcp->get_name().toString()
                << "The operation timedout" << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::IpcmUnregisterApplicationException& e)
    {
        ss << ": Error while unregistering IPC process "
                << ipcp->get_name().toString() << " from IPC "
                        "process " << slave_ipcp->get_name().toString()
                << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::Exception& e)
    {
        ss << ": Unknown error while requesting unregistering IPCP"
                << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    }

    return IPCM_PENDING;
}

ipcm_res_t IPCManager_::enroll_to_dif(Addon* callee, Promise* promise,
                                      const unsigned short ipcp_id,
                                      const rinad::NeighborData& neighbor,
				      bool prepare_hand,
				      const rina::ApplicationProcessNamingInformation& disc_neigh)
{
    std::ostringstream ss;
    IPCMIPCProcess *ipcp;
    IPCPTransState* trans;

    try
    {
        ipcp = lookup_ipcp_by_id(ipcp_id);

        if (!ipcp)
        {
            ss << "Invalid IPCP id " << ipcp_id;
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Auto release the write lock
        rina::ReadScopedLock readlock(ipcp->rwlock, false);

        //Create a transaction
        trans = new IPCPTransState(callee, promise, ipcp->get_id());

        if (!trans)
        {
            ss
                    << "Unable to allocate memory for the transaction object. Out of memory! "
                    << neighbor.difName.toString();
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Store transaction
        if (add_transaction_state(trans) < 0)
        {
            ss << "Unable to add transaction; out of memory? "
                    << neighbor.difName.toString();
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        if (prepare_hand) {
        	ipcp->enroll_prepare_handover(neighbor.difName, neighbor.supportingDifName,
        				      neighbor.apName, disc_neigh, trans->tid);
        } else {
        	ipcp->enroll(neighbor.difName, neighbor.supportingDifName,
        			neighbor.apName, trans->tid);
        }

        ss << "Requested enrollment of IPC process "
                << ipcp->get_name().toString() << " to DIF "
                << neighbor.difName.toString() << " through DIF "
                << neighbor.supportingDifName.toString()
                << " and neighbor IPC process " << neighbor.apName.toString()
                << std::endl;
        FLUSH_LOG(INFO, ss);
    } catch (rina::ConcurrentException& e)
    {
        ss << ": Error while enrolling " << "to DIF "
                << neighbor.difName.toString() << ". Operation timedout"
                << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::EnrollException& e)
    {
        ss << ": Error while enrolling " << "to DIF "
                << neighbor.difName.toString() << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::Exception& e)
    {
        ss << ": Unknown error while enrolling IPCP" << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    }

    return IPCM_PENDING;
}

ipcm_res_t IPCManager_::disconnect_neighbor(Addon* callee,
			       	            Promise* promise,
					    const unsigned short ipcp_id,
					    const rina::ApplicationProcessNamingInformation& neighbor)
{
	std::ostringstream ss;
	IPCMIPCProcess *ipcp;
	IPCPTransState* trans;

	try
	{
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if (!ipcp)
		{
			ss << "Invalid IPCP id " << ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Auto release the write lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		//Create a transaction
		trans = new IPCPTransState(callee, promise, ipcp->get_id());

		if (!trans)
		{
			ss
			<< "Unable to allocate memory for the transaction object. Out of memory! "
			<< neighbor.toString();
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if (add_transaction_state(trans) < 0)
		{
			ss << "Unable to add transaction; out of memory? "
					<< neighbor.toString();
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		ipcp->disconnectFromNeighbor(neighbor, trans->tid);

		ss << "Requested IPC Process "
				<< ipcp->get_name().toString() << " to disconnect from neighbor "
				<< neighbor.toString()
				<< std::endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::ConcurrentException& e)
	{
		ss << ": Error while disconnecting " << "from neighbor "
				<< neighbor.toString() << ". Operation timedout"
				<< std::endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::EnrollException& e)
	{
		ss << ": Error while disconnecting " << "from neighbor "
				<< neighbor.toString() << std::endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::Exception& e)
	{
		ss << ": Unknown error while disconnecting from neighbor" << std::endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

void IPCManager_::check_peer_discovery_config(rinad::DIFTemplate& dif_template,
					      const rinad::IPCProcessToCreate& ipcp_to_create)
{
	std::list<std::string>::const_iterator it;
	rina::PolicyParameter param;

	if (ipcp_to_create.n1difsPeerDiscovery.size() == 0) {
		return;
	}

	param.name_ = "peerDiscoveryPeriodMs";
	param.value_ = "10000";
	dif_template.etConfiguration.policy_set_.add_parameter(param);

	param.name_ = "n1difsPeerDiscovery";
	for (it = ipcp_to_create.n1difsPeerDiscovery.begin();
			it != ipcp_to_create.n1difsPeerDiscovery.end(); it++) {
		param.value_ = *it;
		dif_template.etConfiguration.policy_set_.add_parameter(param);
	}
}

ipcm_res_t IPCManager_::apply_configuration()
{
    std::ostringstream ss;
    std::list<int> ipcps;
    std::list<rinad::IPCProcessToCreate>::iterator cit;
    std::list<int>::iterator pit;

    //TODO: move this to a write_lock over the IPCP

    try
    {
        //FIXME TODO XXX this method needs to be heavily refactored
        //It is not clear which exceptions can be thrown and what to do
        //if this happens. Just protecting to prevent dead-locks.

	//Set system name
	dif_allocator->sys_name = config.local.system_name;

        // Examine all the IPCProcesses that are going to be created
        // according to the configuration file.
        ipcm_res_t result;
        CreateIPCPPromise c_promise;
        Promise promise;
        bool found;
        int rv;
        rinad::DIFTemplateMapping template_mapping;
        rinad::DIFTemplate dif_template;
        for (cit = config.ipcProcessesToCreate.begin();
                cit != config.ipcProcessesToCreate.end(); cit++)
        {
            std::ostringstream ss;

            found = config.lookup_dif_template_mappings(cit->difName,
                                                        template_mapping);
            if (!found)
            {
                ss << "Could not find DIF template for dif name "
                        << cit->difName.processName << std::endl;
                FLUSH_LOG(ERR, ss);
                continue;
            }

            rv = dif_template_manager->get_dif_template(template_mapping.template_name,
        		    	    	    	        dif_template);
            if (rv != 0)
            {
                ss << "Cannot find template called "
                        << template_mapping.template_name;
                FLUSH_LOG(ERR, ss);
                continue;
            }

            try
            {
                if (create_ipcp(NULL, &c_promise, cit->name,
                                dif_template.difType, cit->difName.processName) == IPCM_FAILURE
                        || c_promise.wait() != IPCM_SUCCESS)
                {
                    continue;
                }
                ipcps.push_back(c_promise.ipcp_id);

                check_peer_discovery_config(dif_template, *cit);

                if (assign_to_dif(NULL, &promise, c_promise.ipcp_id,
                                  dif_template, cit->difName) == IPCM_FAILURE
                        || promise.wait() != IPCM_SUCCESS)
                {
                    ss << "Problems assigning IPCP " << c_promise.ipcp_id
                            << " to DIF " << cit->difName.processName
                            << std::endl;
                    FLUSH_LOG(ERR, ss);
                }

                for (std::list<rina::ApplicationProcessNamingInformation>::const_iterator nit =
                        cit->difsToRegisterAt.begin();
                        nit != cit->difsToRegisterAt.end(); nit++)
                {
                    if (register_at_dif(NULL,
                		    	&promise,
					c_promise.ipcp_id,
					*nit)
                            == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS)
                    {
                        ss << "Problems registering IPCP " << c_promise.ipcp_id
                                << " to DIF " << nit->processName << std::endl;
                        FLUSH_LOG(ERR, ss);
                    }
                }
            } catch (rina::Exception &e)
            {
                LOG_ERR("Exception while applying configuration: %s", e.what());
                continue;
            }
        }

        // Perform all the enrollments specified by the configuration file.
        for (pit = ipcps.begin(), cit = config.ipcProcessesToCreate.begin();
                pit != ipcps.end(); pit++, cit++)
        {
            Promise promise;
            try
            {
                for (std::list<rinad::NeighborData>::const_iterator nit = cit
                        ->neighbors.begin(); nit != cit->neighbors.end(); nit++)
                {
                    if (enroll_to_dif(NULL, &promise, *pit, *nit)
                            == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS)
                    {
                        ss << ": Unknown error while enrolling IPCP " << *pit
                                << " to neighbour "
                                << nit->apName.getEncodedString() << std::endl;
                        FLUSH_LOG(ERR, ss);
                        continue;
                    }
                }
            } catch (rina::Exception& e)
            {
                ss << ": Unknown error while enrolling IPCP " << *pit
                        << " to neighbours." << std::endl;
                FLUSH_LOG(ERR, ss);
                continue;
            }
        }
    } catch (rina::Exception &e)
    {
        LOG_ERR("Exception while applying configuration: %s", e.what());
        return IPCM_FAILURE;
    }

    return IPCM_SUCCESS;
}

ipcm_res_t IPCManager_::update_dif_configuration(
        Addon* callee, Promise* promise, const unsigned short ipcp_id,
        const rina::DIFConfiguration & dif_config)
{
    std::ostringstream ss;
    IPCMIPCProcess *ipcp;
    IPCPTransState* trans;

    try
    {
        ipcp = lookup_ipcp_by_id(ipcp_id, true);

        if (!ipcp)
        {
            ss << "Invalid IPCP id " << ipcp_id;
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Auto release the write lock
        rina::WriteScopedLock writelock(ipcp->rwlock, false);

        // Request a configuration update for the IPC process
        /* FIXME The meaning of this operation is unclear: what
         * configuration is modified? The configuration of the
         * IPC process only or the configuration of the whole DIF
         * (which possibly contains more IPC process, both on the same
         * processing systems and on different processing systems) ?
         */
        trans = new IPCPTransState(callee, promise, ipcp->get_id());

        if (!trans)
        {
            ss
                    << "Unable to allocate memory for the transaction object. Out of memory! ";
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Store transaction
        if (add_transaction_state(trans) < 0)
        {
            ss << "Unable to add transaction; out of memory? ";
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }
        ipcp->updateDIFConfiguration(dif_config, trans->tid);

        ss << "Requested configuration update for IPC process "
                << ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(INFO, ss);
    } catch (rina::ConcurrentException& e)
    {
        ss << ": Error while updating DIF configuration "
                " for IPC process " << ipcp->get_name().toString()
                << "Operation timedout." << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::UpdateDIFConfigurationException& e)
    {
        ss << ": Error while updating DIF configuration "
                " for IPC process " << ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::Exception& e)
    {
        ss << ": Unknown error while update configuration" << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    }

    return IPCM_PENDING;
}

ipcm_res_t IPCManager_::query_rib(Addon* callee, QueryRIBPromise* promise,
                                  const unsigned short ipcp_id,
                                  const std::string& objectClass,
                                  const std::string& objectName)
{
    std::ostringstream ss;
    IPCMIPCProcess *ipcp;
    RIBqTransState* trans;

    try
    {
        ipcp = lookup_ipcp_by_id(ipcp_id);

        if (!ipcp)
        {
            ss << "Invalid IPCP id " << ipcp_id;
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Auto release the read lock
        rina::ReadScopedLock readlock(ipcp->rwlock, false);

        trans = new RIBqTransState(callee, promise, ipcp->get_id());
        if (!trans)
        {
            ss
                    << "Unable to allocate memory for the transaction object. Out of memory! ";
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Store transaction
        if (add_transaction_state(trans) < 0)
        {
            ss << "Unable to add transaction; out of memory? ";
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        ipcp->queryRIB(objectClass, objectName, 0, 0, "", trans->tid);

        ss << "Requested query RIB of IPC process "
                << ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(DBG, ss);
    } catch (rina::ConcurrentException& e)
    {
        ss << "Error while querying RIB of IPC Process "
                << ipcp->get_name().toString() << ". Operation timedout"
                << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::QueryRIBException& e)
    {
        ss << "Error while querying RIB of IPC Process "
                << ipcp->get_name().toString() << std::endl;
        return IPCM_FAILURE;
    } catch (rina::Exception& e)
    {
        ss << ": Unknown error while query RIB" << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    }

    return IPCM_PENDING;
}

std::string IPCManager_::get_log_level() const
{
    return log_level_;
}

IPCMIPCProcessFactory * IPCManager_::get_ipcp_factory()
{
	return &ipcp_factory_;
}

ipcm_res_t IPCManager_::set_policy_set_param(Addon* callee, Promise* promise,
                                             const unsigned short ipcp_id,
                                             const std::string& component_path,
                                             const std::string& param_name,
                                             const std::string& param_value)
{
    std::ostringstream ss;
    IPCPTransState* trans = NULL;
    IPCMIPCProcess *ipcp;

    try
    {
        ipcp = lookup_ipcp_by_id(ipcp_id);

        if (!ipcp)
        {
            ss << "Invalid IPCP id " << ipcp_id;
            FLUSH_LOG(ERR, ss);
            throw rina::SetPolicySetParamException();
        }

        //Auto release the read lock
        rina::ReadScopedLock readlock(ipcp->rwlock, false);

        trans = new IPCPTransState(callee, promise, ipcp->get_id());
        if (!trans)
        {
            ss
                    << "Unable to allocate memory for the transaction object. Out of memory! ";
            FLUSH_LOG(ERR, ss);
            throw rina::SetPolicySetParamException();
        }

        //Store transaction
        if (add_transaction_state(trans) < 0)
        {
            ss << "Unable to add transaction; out of memory? ";
            FLUSH_LOG(ERR, ss);
            throw rina::SetPolicySetParamException();
        }

        ipcp->setPolicySetParam(component_path, param_name, param_value,
                                trans->tid);

        ss << "Issued set-policy-set-param to IPC process "
                << ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(INFO, ss);

    } catch (rina::ConcurrentException& e)
    {
        ss << "Error while issuing set-policy-set-param request "
                "to IPC Process " << ipcp->get_name().toString()
                << ". Operation timedout" << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::SetPolicySetParamException& e)
    {
        ss << "Error while issuing set-policy-set-param request "
                "to IPC Process " << ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::Exception& e)
    {
        ss << ": Unknown error while issuing set-policy-set-param request"
                << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    }

    return IPCM_PENDING;
}

int IPCManager_::store_delegated_obj(int port, int invoke_id,
		rina::rib::DelegationObj* obj)
{
       rina::ScopedLock scope(forwarded_calls_lock);
       int key = 0;
       do
       {
               key++;
       }while(forwarded_calls.find(key) != forwarded_calls.end());
       delegated_stored_t *del_sto = new delegated_stored_t;
       del_sto->obj = obj;
       del_sto->invoke_id = invoke_id;
       del_sto->port = port;
       forwarded_calls[key] = del_sto;
       return key;
}


static std::string extract_subcomponent_name(const std::string& cpath)
{
    size_t l = cpath.rfind(".");

    if (l == std::string::npos)
    {
        return cpath;
    }

    return cpath.substr(l + 1);
}

/* If cpath is in the form "efcp.NN.xxx", extract NN,
 * otherwise just return ipcp_id. */
unsigned int extract_resource_id(const unsigned short ipcp_id,
                                 std::string cpath)
{
    std::stringstream ss;
    std::string prefix("efcp.");
    unsigned int port_id;
    size_t dot;

    if (cpath.find(prefix) != 0)
    {
        return ipcp_id;
    }

    cpath = cpath.substr(prefix.size());
    dot = cpath.find(".");
    if (dot == std::string::npos)
    {
        return ipcp_id;
    }

    cpath = cpath.substr(0, dot);

    ss << cpath;
    ss >> port_id;
    if (ss.fail())
    {
        return ipcp_id;
    }

    return port_id;
}

ipcm_res_t IPCManager_::select_policy_set(Addon* callee, Promise* promise,
                                          const unsigned short ipcp_id,
                                          const std::string& component_path,
                                          const std::string& ps_name)
{
    std::ostringstream ss;
    IPCMIPCProcess *ipcp;
    IPCPSelectPsTransState* trans;

    try
    {
        /* Load the policy set in the catalog. */
        rina::PsInfo ps_info;
        unsigned int rsrc_id;
        int ret;

        ps_info.name = ps_name;
        ps_info.app_entity = extract_subcomponent_name(component_path);
        rsrc_id = extract_resource_id(ipcp_id, component_path);
        ret = catalog.load_policy_set(callee, ipcp_id, ps_info);
        if (ret)
        {
            throw rina::Exception();
        }

        /* Select the policy set. */
        ipcp = lookup_ipcp_by_id(ipcp_id);

        if (!ipcp)
        {
            ss << "Invalid IPCP id " << ipcp_id;
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Auto release the read lock
        rina::ReadScopedLock readlock(ipcp->rwlock, false);

        trans = new IPCPSelectPsTransState(callee, promise, ipcp_id, ps_info,
                                           rsrc_id);
        if (!trans)
        {
            ss
                    << "Unable to allocate memory for the transaction object. Out of memory! ";
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Store transaction
        if (add_transaction_state(trans) < 0)
        {
            ss << "Unable to add transaction; out of memory? ";
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        ipcp->selectPolicySet(component_path, ps_name, trans->tid);

        ss << "Issued select-policy-set to IPC process "
                << ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(INFO, ss);
    } catch (rina::ConcurrentException& e)
    {
        ss << "Error while issuing select-policy-set request "
                "to IPC Process " << ipcp->get_name().toString()
                << ". Operation timedout." << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::SelectPolicySetException& e)
    {
        ss << "Error while issuing select-policy-set request "
                "to IPC Process " << ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::Exception& e)
    {
        ss << ": Unknown error while issuing select-policy-set request"
                << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    }

    return IPCM_PENDING;
}

// Returns IPCM_SUCCESS if a kernel plugin was successfully loaded/unloaded,
// IPCM_FAILURE otherwise
ipcm_res_t IPCManager_::plugin_load_kernel(const std::string& plugin_name,
                                           bool load)
{
    std::ostringstream ss;
    ipcm_res_t result = IPCM_FAILURE;
    pid_t pid;

    pid = fork();
    if (pid < 0)
    {
        // parent, fork() failed
        ss << "Kernel plugin (un)loading: fork() failed";
        FLUSH_LOG(ERR, ss);
        result = IPCM_FAILURE;

    } else if (pid == 0)
    {
        // child
	int nfd;

	// redirect stdout to /dev/null
	nfd = open("/dev/null", O_WRONLY);
	dup2(nfd, STDOUT_FILENO);
	// redirect stderr to stdout
	dup2(STDOUT_FILENO, STDERR_FILENO);

        if (load) {
            execlp("modprobe", "modprobe", plugin_name.c_str(), NULL);

        } else {
            execlp("modprobe", "modprobe", "-r", plugin_name.c_str(), NULL);
        }

        ss << "Kernel plugin (un)loading: exec() failed";
        FLUSH_LOG(ERR, ss);

        exit (EXIT_FAILURE);

    } else
    {
        // parent, fork() successful
        int child_status = 0;

        waitpid(pid, &child_status, 0);

        result = child_status ? IPCM_FAILURE : IPCM_SUCCESS;
    }

    return result;
}

ipcm_res_t IPCManager_::plugin_load(Addon* callee, Promise* promise,
                                    const unsigned short ipcp_id,
                                    const std::string& plugin_name, bool load)
{
    std::ostringstream ss;
    IPCMIPCProcess *ipcp;
    IPCPpluginTransState* trans;

    try
    {
        //First try to see if its a kernel module
        if (plugin_load_kernel(plugin_name, load) == IPCM_SUCCESS)
        {
            promise->ret = IPCM_SUCCESS;
            catalog.plugin_loaded(plugin_name, ipcp_id, load);

            return IPCM_SUCCESS;
        }

        ipcp = lookup_ipcp_by_id(ipcp_id);

        if (!ipcp)
        {
            ss << "Invalid IPCP id " << ipcp_id;
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Auto release the read lock
        rina::ReadScopedLock readlock(ipcp->rwlock, false);

        trans = new IPCPpluginTransState(callee, promise, ipcp->get_id(),
                                         plugin_name, load);
        if (!trans)
        {
            ss
                    << "Unable to allocate memory for the transaction object. Out of memory! ";
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }

        //Store transaction
        if (add_transaction_state(trans) < 0)
        {
            ss << "Unable to add transaction; out of memory? ";
            FLUSH_LOG(ERR, ss);
            throw rina::Exception();
        }
        ipcp->pluginLoad(plugin_name, load, trans->tid);

        ss << "Issued plugin-load to IPC process "
                << ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(INFO, ss);
    } catch (rina::ConcurrentException& e)
    {
        ss << "Error while issuing plugin-load request "
                "to IPC Process " << ipcp->get_name().toString()
                << ". Operation timedout" << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::PluginLoadException& e)
    {
        ss << "Error while issuing plugin-load request "
                "to IPC Process " << ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    } catch (rina::Exception& e)
    {
        ss << ": Unknown error while issuing plugin-load request " << std::endl;
        FLUSH_LOG(ERR, ss);
        return IPCM_FAILURE;
    }

    return IPCM_PENDING;
}

ipcm_res_t IPCManager_::plugin_get_info(const std::string& plugin_name,
                                        std::list<rina::PsInfo>& result)
{
    int ret = rina::plugin_get_info(plugin_name, IPCPPLUGINSDIR, result);

    return ret ? IPCM_FAILURE : IPCM_SUCCESS;
}

ipcm_res_t IPCManager_::update_catalog(Addon* callee)
{
    catalog.import();

    return IPCM_SUCCESS;
}

ipcm_res_t IPCManager_::delegate_ipcp_ribobj(rina::rib::DelegationObj* obj,
                                             const unsigned short ipcp_id,
					     rina::cdap::cdap_m_t::Opcode op_code,
					     const std::string& object_class,
					     const std::string& object_name,
					     const rina::ser_obj_t &object_value,
					     int scope,
					     int invoke_id,
					     int port)
{
    IPCMIPCProcess * ipcp;
   // TransactionState* trans;
    std::ostringstream ss;

    try
    {
        ipcp = lookup_ipcp_by_id(ipcp_id);

        if (!ipcp)
        {
            LOG_ERR("Invalid IPCP id %hu", ipcp_id);
            return IPCM_FAILURE;
        }
        //Auto release the read lock
        rina::ReadScopedLock readlock(ipcp->rwlock, false);
        if (ipcp->get_type() != rina::NORMAL_IPC_PROCESS)
        {
        	LOG_ERR("Trying to delegate to a shim IPCP, operation not allowed");
        	return IPCM_FAILURE;
        }

        rina::cdap::CDAPMessage msg;
        msg.op_code_ = op_code;
        msg.obj_class_ = object_class;
        msg.obj_name_ = object_name;
        msg.obj_value_ = object_value;
        msg.scope_ = scope;

        // Generate a unique id to recover the delegated object
        msg.invoke_id_ = store_delegated_obj(port, invoke_id, obj);

        ipcp->forwardCDAPMessage(msg, 0);

        ss << "Forwarded CDAPMessage to IPC process "
                << ipcp->get_name().toString() << std::endl;
        FLUSH_LOG(INFO, ss);

    } catch (rina::Exception &e)
    {
        LOG_ERR("Problems: %s", e.what());
        return IPCM_FAILURE;
    }

    return IPCM_PENDING;
}

ipcm_res_t IPCManager_::unregister_app_from_ipcp(Addon* callee,
						 Promise* promise,
						 const rina::ApplicationUnregistrationRequestEvent& req_event,
						 const unsigned short slave_ipcp_id)
{
	std::ostringstream ss;
	IPCMIPCProcess *slave_ipcp;
	APPUnregTransState* trans;

	try
	{
		slave_ipcp = lookup_ipcp_by_id(slave_ipcp_id, true);

		if (!slave_ipcp)
		{
			ss << "Cannot find any IPC process belonging " << std::endl;
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Auto release the write lock
		rina::WriteScopedLock writelock(slave_ipcp->rwlock, false);

		// Forward the unregistration request to the IPC process
		// that the application is registered to
		trans = new APPUnregTransState(callee, promise,
					       slave_ipcp->get_id(), req_event);
		if (!trans)
		{
			ss
			<< "Unable to allocate memory for the transaction object. Out of memory! ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if (add_transaction_state(trans) < 0)
		{
			ss << "Unable to add transaction; out of memory? ";
			FLUSH_LOG(ERR, ss);
			delete trans;
			throw rina::Exception();
		}

		slave_ipcp->unregisterApplication(req_event.applicationName,
				trans->tid);
		ss << "Requested unregistration of application "
				<< req_event.applicationName.toString() << " from IPC "
				"process " << slave_ipcp->get_name().toString()
				<< std::endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::ConcurrentException& e)
	{
		ss << ": Error while unregistering application "
				<< req_event.applicationName.toString() << " from IPC "
				"process " << slave_ipcp->get_name().toString()
				<< ". Operation timedout." << std::endl;
		FLUSH_LOG(ERR, ss);
		remove_transaction_state(trans->tid);
		return IPCM_FAILURE;
	} catch (rina::IpcmUnregisterApplicationException& e)
	{
		ss << ": Error while unregistering application "
				<< req_event.applicationName.toString() << " from IPC "
				"process " << slave_ipcp->get_name().toString()
				<< std::endl;
		remove_transaction_state(trans->tid);
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::Exception& e)
	{
		ss << ": Unknown error while unregistering application " << std::endl;
		FLUSH_LOG(ERR, ss);
		remove_transaction_state(trans->tid);
		return IPCM_FAILURE;
	}
	return IPCM_PENDING;
}

//
// Promises
//
ipcm_res_t Promise::wait(void)
{
    unsigned int i;
    // Due to the async nature of the API, notifications (signal)
    // the transaction can well end before the thread is waiting
    // in the condition variable. As apposed to sempahores
    // pthread_cond don't keep the "credit"
    for (i = 0;
            i < PROMISE_TIMEOUT_S * (_PROMISE_1_SEC_NSEC / PROMISE_RETRY_NSEC);
            ++i)
    {
        try
        {
            if (ret != IPCM_PENDING)
                return ret;
            wait_cond.timedwait(0, PROMISE_RETRY_NSEC);
        } catch (...)
        {
        };
    }

    //hard timeout expired
    if (!trans->abort())
        //The transaction ended at the very last second
        return ret;
    ret = IPCM_FAILURE;
    return ret;
}

ipcm_res_t Promise::timed_wait(const unsigned int seconds)
{

    if (ret != IPCM_PENDING)
        return ret;
    try
    {
        wait_cond.timedwait(seconds, 0);
    } catch (rina::ConcurrentException& e)
    {
        if (ret != IPCM_PENDING)
            return ret;
        return IPCM_PENDING;
    };
    return ret;
}

//
// Transactions
//

TransactionState::TransactionState(Addon* callee_, Promise* _promise)
        : promise(_promise),
          tid(IPCManager->__tid_gen.next()),
          callee(callee_),
          finalised(false)
{
    if (promise)
    {
        promise->ret = IPCM_PENDING;
        promise->trans = this;
    }
}
;

//State management routines
int IPCManager_::add_transaction_state(TransactionState* t)
{

    //Lock
    rina::WriteScopedLock writelock(trans_rwlock);

    //Check if exists already
    if (pend_transactions.find(t->tid) != pend_transactions.end())
    {
        assert(0);  //Transaction id repeated
        return -1;
    }

    //Just add and return
    try
    {
        pend_transactions[t->tid] = t;
    } catch (...)
    {
        LOG_DBG("Could not add transaction %u. Out of memory?", t->tid);
        assert(0);
        return -1;
    }

    return 0;
}

int IPCManager_::remove_transaction_state(int tid)
{

    TransactionState* t;
    //Lock
    rina::WriteScopedLock writelock(trans_rwlock);

    //Check if it really exists
    if (pend_transactions.find(tid) == pend_transactions.end())
        return -1;

    t = pend_transactions[tid];
    pend_transactions.erase(tid);
    delete t;

    return 0;
}

//Syscall state management routines
int IPCManager_::add_syscall_transaction_state(SyscallTransState* t)
{

    //Lock
    rina::WriteScopedLock writelock(trans_rwlock);

    //Check if exists already
    if (pend_sys_trans.find(t->tid) != pend_sys_trans.end())
    {
        assert(0);  //Transaction id repeated
        return -1;
    }

    //Just add and return
    try
    {
        pend_sys_trans[t->tid] = t;
    } catch (...)
    {
        LOG_DBG("Could not add syscall transaction %u. Out of memory?", t->tid);
        assert(0);
        return -1;
    }

    return 0;
}

int IPCManager_::remove_syscall_transaction_state(int tid)
{

    SyscallTransState* t;

    //Lock
    rina::WriteScopedLock writelock(trans_rwlock);

    //Check if it really exists
    if (pend_sys_trans.find(tid) == pend_sys_trans.end())
        return -1;

    t = pend_sys_trans[tid];
    pend_sys_trans.erase(tid);
    delete t;

    return 0;
}

//Main I/O loop
void IPCManager_::run()
{

    void* status;

    //Wait for the request to stop
    stop_cond.doWait();

    //Cleanup
    LOG_DBG("Cleaning the house...");

    //Destroy all IPCPs
    std::vector<IPCMIPCProcess *> ipcps;
    ipcp_factory_.listIPCProcesses(ipcps);
    std::vector<IPCMIPCProcess *>::const_iterator it;

    //Rwlock: write
    for (it = ipcps.begin(); it != ipcps.end(); ++it)
    {
        if (destroy_ipcp(NULL, (*it)->get_id()) < 0)
        {
            LOG_WARN("Warning could not destroy IPCP id: %d\n",
                     (*it)->get_id());
        }
    }

    //Destroy all addons (stop them)
    Addon::destroy_all();

    //Join the I/O loop thread
    io_thread->join(&status);

    //I/O thread
    delete io_thread;

    // Destroy RINAManager
    rina::destroyIPCManager();

    // Shutdown Protobuf Library
    google::protobuf::ShutdownProtobufLibrary();
}

delegated_stored_t* IPCManager_::get_forwarded_object(int invoke_id,
                                                            bool remove)
{
        rina::ScopedLock scope(forwarded_calls_lock);
        delegated_stored_t* obj;
        std::map<int, delegated_stored_t*>::iterator it =
                        forwarded_calls.find(invoke_id);
        if (it == forwarded_calls.end())
                return NULL;
        else
        {
                obj = it->second;
                if (remove)
                {
                        forwarded_calls.erase(it);
                }
                return obj;
        }
}

//static
void* IPCManager_::io_loop_trampoline(void* param)
{
    IPCManager->io_loop();
    return NULL;
}

void IPCManager_::io_loop()
{
    rina::IPCEvent *event;

    LOG_DBG("Starting main I/O loop...");

    while (true)
    {
        event = rina::ipcEventProducer->eventWait();
        if (!event) {
        	LOG_WARN("Event is NULL");
        	rina::librina_finalize();
        	stop_cond.signal();
        	break;
        }

        if (event->eventType == rina::IPCM_FINALIZATION_REQUEST_EVENT && req_to_stop)
        {
        	//Signal the main thread to start
        	//the stop procedure
        	LOG_INFO("IPCM event loop requested to stop");

        	void * status;
        	if (osp_monitor) {
        		osp_monitor->do_stop();
        		osp_monitor->join(&status);

        		delete osp_monitor;
        	}

        	stop_cond.signal();
        	break;
        }

        LOG_DBG("Got event of type %s and sequence number %u",
                rina::IPCEvent::eventTypeToString(event->eventType).c_str(),
                event->sequenceNumber);

        try
        {
            switch (event->eventType) {
                case rina::FLOW_ALLOCATION_REQUESTED_EVENT: {
                    DOWNCAST_DECL(event, rina::FlowRequestEvent, e);
                    flow_allocation_requested_event_handler(NULL, e);
                }
                    break;

                case rina::ALLOCATE_FLOW_RESPONSE_EVENT: {
                    DOWNCAST_DECL(event, rina::AllocateFlowResponseEvent, e);
                    allocate_flow_response_event_handler(e);
                }
                    break;

                case rina::FLOW_DEALLOCATION_REQUESTED_EVENT: {
                    DOWNCAST_DECL(event, rina::FlowDeallocateRequestEvent, e);
                    flow_deallocation_requested_event_handler(NULL, e);
                }
                    break;

                case rina::FLOW_DEALLOCATED_EVENT: {
                    DOWNCAST_DECL(event, rina::FlowDeallocatedEvent, e);
                    IPCManager->flow_deallocated_event_handler(e);
                }
                    break;
                case rina::APPLICATION_REGISTRATION_REQUEST_EVENT: {
                    DOWNCAST_DECL(event,
                                  rina::ApplicationRegistrationRequestEvent, e);
                    app_reg_req_handler(e);
                }
                    break;

                case rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT: {
                    DOWNCAST_DECL(event,
                                  rina::ApplicationUnregistrationRequestEvent,
                                  e);
                    application_unregistration_request_event_handler(e);
                }
                    break;

                case rina::ASSIGN_TO_DIF_RESPONSE_EVENT: {
                    DOWNCAST_DECL(event, rina::AssignToDIFResponseEvent, e);
                    assign_to_dif_response_event_handler(e);
                }
                    break;

                case rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT: {
                    DOWNCAST_DECL(event,
                                  rina::UpdateDIFConfigurationResponseEvent, e);
                    update_dif_config_response_event_handler(e);
                }
                    break;

                case rina::ENROLL_TO_DIF_RESPONSE_EVENT: {
                    DOWNCAST_DECL(event, rina::EnrollToDIFResponseEvent, e);
                    enroll_to_dif_response_event_handler(e);
                }
                    break;

                case rina::DISCONNECT_NEIGHBOR_RESPONSE_EVENT: {
                    DOWNCAST_DECL(event, rina::DisconnectNeighborResponseEvent, e);
                    disconnect_neighbor_response_event_handler(e);
                }
                    break;

                case rina::IPCM_REGISTER_APP_RESPONSE_EVENT: {
                    DOWNCAST_DECL(event,
                                  rina::IpcmRegisterApplicationResponseEvent, e);
                    app_reg_response_handler(e);
                }
                    break;

                case rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT: {
                    DOWNCAST_DECL(event,
                                  rina::IpcmUnregisterApplicationResponseEvent,
                                  e);
                    unreg_app_response_handler(e);
                }
                    break;

                case rina::IPCM_ALLOCATE_FLOW_REQUEST_RESULT: {
                    DOWNCAST_DECL(event,
                                  rina::IpcmAllocateFlowRequestResultEvent, e);
                    ipcm_allocate_flow_request_result_handler(e);
                }
                    break;

                case rina::QUERY_RIB_RESPONSE_EVENT: {
                    DOWNCAST_DECL(event, rina::QueryRIBResponseEvent, e);
                    query_rib_response_event_handler(e);
                }
                    break;

                case rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT: {
                    DOWNCAST_DECL(event,
                		  rina::IPCProcessDaemonInitializedEvent, e);
                    ipc_process_daemon_initialized_event_handler(e);
                }
                    break;

                    //Policies
                case rina::IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE: {
                    DOWNCAST_DECL(event, rina::SetPolicySetParamResponseEvent,
                                  e);
                    ipc_process_set_policy_set_param_response_handler(e);
                }
                    break;
                case rina::IPC_PROCESS_SELECT_POLICY_SET_RESPONSE: {
                    DOWNCAST_DECL(event, rina::SelectPolicySetResponseEvent, e);
                    ipc_process_select_policy_set_response_handler(e);
                }
                    break;
                case rina::IPC_PROCESS_PLUGIN_LOAD_RESPONSE: {
                    DOWNCAST_DECL(event, rina::PluginLoadResponseEvent, e);
                    ipc_process_plugin_load_response_handler(e);
                }
                    break;

                case rina::IPCM_CREATE_IPCP_RESPONSE: {
                    DOWNCAST_DECL(event, rina::CreateIPCPResponseEvent, e);
                    ipc_process_create_response_event_handler(e);
                }
                    break;

                case rina::IPCM_DESTROY_IPCP_RESPONSE: {
                    DOWNCAST_DECL(event, rina::DestroyIPCPResponseEvent, e);
                    ipc_process_destroy_response_event_handler(e);
                }
                    break;

                    //Addon specific events
                default:
                {
                    TransactionState* trans = get_transaction_state<
                            TransactionState>(event->sequenceNumber);

                    Addon::distribute_flow_event(event);

                    if (trans)
                    {
                        //Mark as completed
                        trans->completed(IPCM_SUCCESS);
                        remove_transaction_state(trans->tid);
                    }

                    continue;
                    break;
                }
            }

        } catch (rina::Exception &e)
        {
            LOG_ERR("ERROR while processing event %d: %s",event->eventType,
            		e.what());
            //TODO: move locking to a smaller scope
        }

        delete event;
    }

    //TODO: probably move this to a private method if it starts to grow
    LOG_DBG("Stopping I/O loop...");
}

}  //rinad namespace
