//
// Implementation of the IPC Process
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <cstdlib>
#include <sstream>

#define IPCP_MODULE "core"
#include "ipcp-logging.h"

#include "ipcp/enrollment-task.h"
#include "ipcp/flow-allocator.h"
#include "ipcp/ipc-process.h"
#include "ipcp/namespace-manager.h"
#include "ipcp/resource-allocator.h"
#include "ipcp/rib-daemon.h"
#include "ipcp/components.h"

namespace rinad {

#define IPCP_LOG_IPCP_FILE_PREFIX "/tmp/ipcp-log-file"

//Class IPCProcessImpl
IPCProcessImpl::IPCProcessImpl(const rina::ApplicationProcessNamingInformation& nm,
		unsigned short id, unsigned int ipc_manager_port,
		std::string log_level, std::string log_file) : IPCProcess(nm.processName, nm.processInstance)
{
        try {
                std::stringstream ss;
                ss << IPCP_LOG_IPCP_FILE_PREFIX << "-" << id;
                rina::initialize(log_level, log_file);
                rina::extendedIPCManager->ipcManagerPort = ipc_manager_port;
                rina::extendedIPCManager->ipcProcessId = id;
                rina::kernelIPCProcess->ipcProcessId = id;
                LOG_IPCP_INFO("Librina initialized");
        } catch (rina::Exception &e) {
                std::cerr << "Cannot initialize librina" << std::endl;
                exit(EXIT_FAILURE);
        }

        state = NOT_INITIALIZED;
        lock_ = new rina::Lockable();

        // Load the default pluggable components
        if (plugin_load(PLUGINSDIR, "default")) {
        		throw rina::Exception("Failed to load default plugin");
        }

        // Initialize application entities
        init_cdap_session_manager();
        init_encoder();

        delimiter_ = 0; //TODO initialize Delimiter once it is implemented
        internal_event_manager_ = new rina::SimpleInternalEventManager();
        enrollment_task_ = new EnrollmentTask();
        flow_allocator_ = new FlowAllocator();
        namespace_manager_ = new NamespaceManager();
        resource_allocator_ = new ResourceAllocator();
        security_manager_ = new IPCPSecurityManager();
        routing_component_ = new RoutingComponent();
        rib_daemon_ = new IPCPRIBDaemonImpl();

        add_entity(internal_event_manager_);
        add_entity(rib_daemon_);
        add_entity(enrollment_task_);
        add_entity(resource_allocator_->get_n_minus_one_flow_manager());
        add_entity(resource_allocator_);
        add_entity(namespace_manager_);
        add_entity(flow_allocator_);
        add_entity(security_manager_);
        add_entity(routing_component_);

        // Select the default policy sets
        security_manager_->select_policy_set(std::string(), rina::IPolicySet::DEFAULT_PS_SET_NAME);
        if (!security_manager_->ps) {
                throw rina::Exception("Cannot create security manager policy-set");
        }
        security_manager_->add_auth_policy_set(rina::IAuthPolicySet::AUTH_NONE);

        flow_allocator_->select_policy_set(std::string(), rina::IPolicySet::DEFAULT_PS_SET_NAME);
        if (!flow_allocator_->ps) {
                throw rina::Exception("Cannot create flow allocator policy-set");
        }

        namespace_manager_->select_policy_set(std::string(), rina::IPolicySet::DEFAULT_PS_SET_NAME);
        if (!namespace_manager_->ps) {
                throw rina::Exception("Cannot create namespace manager policy-set");
        }

        resource_allocator_->select_policy_set(std::string(), rina::IPolicySet::DEFAULT_PS_SET_NAME);
        if (!resource_allocator_->ps) {
                throw rina::Exception("Cannot create resource allocator policy-set");
        }

        enrollment_task_->select_policy_set(std::string(), rina::IPolicySet::DEFAULT_PS_SET_NAME);
        if (!enrollment_task_->ps) {
                throw rina::Exception("Cannot create enrollment task policy-set");
        }

        routing_component_->select_policy_set(std::string(), "link-state");
        if (!routing_component_->ps) {
                throw rina::Exception("Cannot create routing component policy-set");
        }

        try {
                rina::ApplicationProcessNamingInformation naming_info(name_, instance_);
                rina::extendedIPCManager->notifyIPCProcessInitialized(naming_info);
        } catch (rina::Exception &e) {
        		LOG_IPCP_ERR("Problems communicating with IPC Manager: %s. Exiting... ", e.what());
        		exit(EXIT_FAILURE);
        }

        state = INITIALIZED;

        LOG_IPCP_INFO("Initialized IPC Process with name: %s, instance %s, id %hu ",
        		name_.c_str(), instance_.c_str(), id);
}

IPCProcessImpl::~IPCProcessImpl() {
	if (lock_) {
		delete lock_;
	}

	if (delimiter_) {
		delete delimiter_;
	}

	if (encoder_) {
		delete encoder_;
	}

	if (internal_event_manager_) {
		delete internal_event_manager_;
	}

	if (cdap_session_manager_) {
		delete cdap_session_manager_;
	}

	if (enrollment_task_) {
		psDestroy(rina::ApplicationEntity::ENROLLMENT_TASK_AE_NAME,
                   enrollment_task_->selected_ps_name,
                   enrollment_task_->ps);
		delete enrollment_task_;
	}

	if (flow_allocator_) {
		psDestroy(IFlowAllocator::FLOW_ALLOCATOR_AE_NAME,
                   flow_allocator_->selected_ps_name,
                   flow_allocator_->ps);
		delete flow_allocator_;
	}

	if (namespace_manager_) {
		psDestroy(INamespaceManager::NAMESPACE_MANAGER_AE_NAME,
                   namespace_manager_->selected_ps_name,
                   namespace_manager_->ps);
		delete namespace_manager_;
	}

	if (resource_allocator_) {
		psDestroy(IResourceAllocator::RESOURCE_ALLOCATOR_AE_NAME,
					resource_allocator_->selected_ps_name,
					resource_allocator_->ps);
		delete resource_allocator_;
	}

	if (security_manager_) {
		psDestroy(rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME,
                   security_manager_->selected_ps_name,
                   security_manager_->ps);
        delete security_manager_;
	}

	if (routing_component_) {
		psDestroy(IRoutingComponent::ROUTING_COMPONENT_AE_NAME,
				routing_component_->selected_ps_name,
				routing_component_->ps);
        delete routing_component_;
	}

	if (rib_daemon_) {
		delete rib_daemon_;
	}
}

void IPCProcessImpl::init_cdap_session_manager() {
	rina::WireMessageProviderFactory wire_factory_;
	rina::CDAPSessionManagerFactory cdap_manager_factory_;
	long timeout = 180000;
	cdap_session_manager_ = cdap_manager_factory_.createCDAPSessionManager(
			&wire_factory_, timeout);
}

void IPCProcessImpl::init_encoder() {
	encoder_ = new rinad::Encoder();
	encoder_->addEncoder(EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS,
			new DataTransferConstantsEncoder());
	encoder_->addEncoder(EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS,
			new DirectoryForwardingTableEntryEncoder());
	encoder_->addEncoder(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
			new DirectoryForwardingTableEntryListEncoder());
	encoder_->addEncoder(EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
			new EnrollmentInformationRequestEncoder());
	encoder_->addEncoder(EncoderConstants::FLOW_RIB_OBJECT_CLASS,
			new FlowEncoder());
	encoder_->addEncoder(rina::NeighborSetRIBObject::NEIGHBOR_RIB_OBJECT_CLASS,
			new NeighborEncoder());
	encoder_->addEncoder(rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS,
			new NeighborListEncoder());
	encoder_->addEncoder(EncoderConstants::QOS_CUBE_RIB_OBJECT_CLASS,
			new QoSCubeEncoder());
	encoder_->addEncoder(EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS,
			new QoSCubeListEncoder());
	encoder_->addEncoder(EncoderConstants::WHATEVERCAST_NAME_RIB_OBJECT_CLASS,
			new WhatevercastNameEncoder());
	encoder_->addEncoder(EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS,
			new WhatevercastNameListEncoder());
	encoder_->addEncoder(EncoderConstants::WATCHDOG_RIB_OBJECT_CLASS,
			new WatchdogEncoder());
	encoder_->addEncoder(rina::ADataObject::A_DATA_OBJECT_CLASS,
			new ADataObjectEncoder());
}

unsigned short IPCProcessImpl::get_id() {
	return rina::extendedIPCManager->ipcProcessId;
}

const std::list<rina::Neighbor*> IPCProcessImpl::get_neighbors() const {
	return enrollment_task_->get_neighbors();
}

const IPCProcessOperationalState& IPCProcessImpl::get_operational_state() const {
	rina::ScopedLock g(*lock_);
	return state;
}

void IPCProcessImpl::set_operational_state(const IPCProcessOperationalState& operational_state) {
	rina::ScopedLock g(*lock_);
	state = operational_state;
}

const rina::DIFInformation& IPCProcessImpl::get_dif_information() const {
	rina::ScopedLock g(*lock_);
	return dif_information_;
}

void IPCProcessImpl::set_dif_information(const rina::DIFInformation& dif_information) {
	rina::ScopedLock g(*lock_);
	dif_information_ = dif_information;
}

unsigned int IPCProcessImpl::get_address() const {
	rina::ScopedLock g(*lock_);
	if (state != ASSIGNED_TO_DIF) {
		return 0;
	}

	return dif_information_.dif_configuration_.address_;
}

void IPCProcessImpl::set_address(unsigned int address) {
	rina::ScopedLock g(*lock_);
	dif_information_.dif_configuration_.address_ = address;
}

void IPCProcessImpl::processAssignToDIFRequestEvent(const rina::AssignToDIFRequestEvent& event) {
	rina::ScopedLock g(*lock_);

	if (state != INITIALIZED) {
		//The IPC Process can only be assigned to a DIF once, reply with error message
		LOG_IPCP_ERR("Got a DIF assignment request while not in INITIALIZED state. Current state is: %d",
				state);
		rina::extendedIPCManager->assignToDIFResponse(event, -1);
		return;
	}

	try {
		unsigned int handle = rina::kernelIPCProcess->assignToDIF(event.difInformation);
		pending_events_.insert(std::pair<unsigned int,
				rina::AssignToDIFRequestEvent>(handle, event));
		state = ASSIGN_TO_DIF_IN_PROCESS;
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending DIF Assignment request to the kernel: %s", e.what());
		rina::extendedIPCManager->assignToDIFResponse(event, -1);
	}
}

void IPCProcessImpl::processAssignToDIFResponseEvent(const rina::AssignToDIFResponseEvent& event) {
	rina::ScopedLock g(*lock_);

	if (state == ASSIGNED_TO_DIF ) {
		LOG_IPCP_INFO("Got reply from the Kernel components regarding DIF assignment: %d",
				event.result);
		return;
	}

	if (state != ASSIGN_TO_DIF_IN_PROCESS) {
		LOG_IPCP_ERR("Got a DIF assignment response while not in ASSIGN_TO_DIF_IN_PROCESS state. State is %d ",
				state);
		return;
	}

	std::map<unsigned int, rina::AssignToDIFRequestEvent>::iterator it;
	it = pending_events_.find(event.sequenceNumber);
	if (it == pending_events_.end()) {
		LOG_IPCP_ERR("Couldn't find an Assign to DIF request event associated to the handle %u",
				event.sequenceNumber);
		return;
	}

	rina::AssignToDIFRequestEvent requestEvent = it->second;
	dif_information_ = requestEvent.difInformation;
	pending_events_.erase(it);
	if (event.result != 0) {
		LOG_IPCP_ERR("The kernel couldn't successfully process the Assign to DIF Request: %d",
				event.result);
		LOG_IPCP_ERR("Could not assign IPC Process to DIF %s",
				it->second.difInformation.dif_name_.processName.c_str());
		state = INITIALIZED;

		try {
			rina::extendedIPCManager->assignToDIFResponse(requestEvent, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	//TODO do stuff
	LOG_IPCP_DBG("The kernel processed successfully the Assign to DIF request");

        // Select the policy-sets specified in the DIF configuration, for
        // userspace IPCP components
        std::list<rina::Parameter>& policy_sets_config =
                                dif_information_.dif_configuration_.policy_sets;
        for (std::list<rina::Parameter>::iterator
                        it = policy_sets_config.begin();
                                it != policy_sets_config.end(); it++) {
                std::string path = it->name;
                std::string name = it->value;
                bool got_in_userspace;
                int result;

                result = dispatchSelectPolicySet(path, name, got_in_userspace);
                if (result) {
                        LOG_IPCP_ERR("Failed to select policy set %s for component %s",
                                name.c_str(), path.c_str());
                } else if (!got_in_userspace) {
                        LOG_IPCP_ERR("Component %s is not an userspace IPCP component",
                                path.c_str());
                }
        }

	try{
		rib_daemon_->set_dif_configuration(dif_information_.dif_configuration_);
		resource_allocator_->set_dif_configuration(dif_information_.dif_configuration_);
		routing_component_->set_dif_configuration(dif_information_.dif_configuration_);
		namespace_manager_->set_dif_configuration(dif_information_.dif_configuration_);
		security_manager_->set_dif_configuration(dif_information_.dif_configuration_);
		flow_allocator_->set_dif_configuration(dif_information_.dif_configuration_);
		enrollment_task_->set_dif_configuration(dif_information_.dif_configuration_);
	}
	catch(rina::Exception &e){
		state = INITIALIZED;
		LOG_IPCP_ERR("Bad configuration error: %s", e.what());
		rina::extendedIPCManager->assignToDIFResponse(requestEvent, -1);
	}

	state = ASSIGNED_TO_DIF;

	try {
		rina::extendedIPCManager->assignToDIFResponse(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

void IPCProcessImpl::requestPDUFTEDump() {
	try{
		rina::kernelIPCProcess->dumptPDUFT();
	} catch (rina::Exception &e) {
		LOG_IPCP_WARN("Error requesting IPC Process to Dump PDU Forwarding Table: %s", e.what());
	}
}

void IPCProcessImpl::logPDUFTE(const rina::DumpFTResponseEvent& event) {
	if (event.result != 0) {
		LOG_IPCP_WARN("Dump PDU FT operation returned error, error code: %d",
				event.getResult());
		return;
	}

	std::list<rina::PDUForwardingTableEntry>::const_iterator it;
	std::list<unsigned int>::const_iterator it2;
	std::stringstream ss;
	ss << "Contents of the PDU Forwarding Table: " << std::endl;
	for (it = event.entries.begin(); it != event.entries.end(); ++it) {
		ss << "Address: " << it->address << "; QoS-id: ";
		ss << it->qosId << "; Port-ids: ";
		for (it2 = it->portIds.begin(); it2 != it->portIds.end(); ++it2) {
			ss << (*it2) << "; ";
		}
		ss << std::endl;
	}

	LOG_IPCP_INFO("%s", ss.str().c_str());
}

static void parse_path(const std::string& path, std::string& component,
                       std::string& remainder)
{
        size_t dotpos = path.find_first_of('.');

        component = path.substr(0, dotpos);
        if (dotpos == std::string::npos || dotpos + 1 == path.size()) {
                remainder = std::string();
        } else {
                remainder = path.substr(dotpos + 1);
        }
}

void IPCProcessImpl::processSetPolicySetParamRequestEvent(
                        const rina::SetPolicySetParamRequestEvent& event) {
	rina::ScopedLock g(*lock_);
        std::string component, remainder;
        bool got_in_userspace = true;
        int result = -1;

        parse_path(event.path, component, remainder);

        // First check if the request should be served by this daemon
        // or should be forwarded to kernelspace
        if (component == "security-manager") {
                result = security_manager_->set_policy_set_param(remainder,
                                                                event.name,
                                                                event.value);
        } else if (component == "enrollment") {
                result = enrollment_task_->set_policy_set_param(remainder,
                                                               event.name,
                                                               event.value);
        } else if (component == "flow-allocator") {
                result = flow_allocator_->set_policy_set_param(remainder,
                                                              event.name,
                                                              event.value);
        } else if (component == "namespace-manager") {
                result = namespace_manager_->set_policy_set_param(remainder,
                                                                 event.name,
                                                                 event.value);
        } else if (component == "resource-allocator") {
                result = resource_allocator_->set_policy_set_param(remainder,
                                                                  event.name,
                                                                  event.value);
        } else if (component == "rib-daemon") {
                result = rib_daemon_->set_policy_set_param(remainder,
                                                          event.name,
                                                          event.value);
        } else if (component == "routing") {
                result = routing_component_->set_policy_set_param(remainder,
                                                                  event.name,
                                                                  event.value);
        } else {
                got_in_userspace = false;
        }

        if (got_in_userspace) {
                // Event managed without going through kernelspace. Notify
                // the IPC Manager about the result
		rina::extendedIPCManager->setPolicySetParamResponse(event,
                                                                    result);
                return;
        }

	try {
		unsigned int handle =
                        rina::kernelIPCProcess->setPolicySetParam(event.path,
                                                event.name, event.value);
		pending_set_policy_set_param_events.insert(
                        std::pair<unsigned int,
			rina::SetPolicySetParamRequestEvent>(handle, event));
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending set-policy-set-param request "
                        "to the kernel: %s", e.what());
		rina::extendedIPCManager->setPolicySetParamResponse(event, -1);
	}
}

void IPCProcessImpl::processSetPolicySetParamResponseEvent(
                        const rina::SetPolicySetParamResponseEvent& event) {
	rina::ScopedLock g(*lock_);
	std::map<unsigned int,
                 rina::SetPolicySetParamRequestEvent>::iterator it;

	it = pending_set_policy_set_param_events.find(event.sequenceNumber);
	if (it == pending_set_policy_set_param_events.end()) {
		LOG_IPCP_ERR("Couldn't find a set-policy-set-param request event "
                        "associated to the handle %u", event.sequenceNumber);
		return;
	}

	rina::SetPolicySetParamRequestEvent requestEvent = it->second;

	pending_set_policy_set_param_events.erase(it);
	if (event.result != 0) {
		LOG_IPCP_ERR("The kernel couldn't successfully process the "
                        "set-policy-set-param Request: %d", event.result);

		try {
			rina::extendedIPCManager->setPolicySetParamResponse(it->second, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	LOG_IPCP_DBG("The kernel processed successfully the "
                "set-policy-set-param request");

	try {
		rina::extendedIPCManager->setPolicySetParamResponse(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

int IPCProcessImpl::dispatchSelectPolicySet(const std::string& path,
                                            const std::string& name,
                                            bool& got_in_userspace)
{
        std::string component, remainder;
        int result = 0;

        got_in_userspace = true;

        parse_path(path, component, remainder);

        // First check if the request should be served by this daemon
        // or should be forwarded to kernelspace
        if (component == "security-manager") {
                result = security_manager_->select_policy_set(remainder,
                                                              name);
        } else if (component == "enrollment") {
                result = enrollment_task_->select_policy_set(remainder,
                                                             name);
        } else if (component == "flow-allocator") {
                result = flow_allocator_->select_policy_set(remainder,
                                                            name);
        } else if (component == "namespace-manager") {
                result = namespace_manager_->select_policy_set(remainder,
                                                               name);
        } else if (component == "resource-allocator") {
                result = resource_allocator_->select_policy_set(remainder,
                                                                name);
        } else if (component == "rib-daemon") {
                result = rib_daemon_->select_policy_set(remainder,
                                                        name);
        } else if (component == "routing") {
                result = routing_component_->select_policy_set(remainder,
                                                               name);
        } else {
                got_in_userspace = false;
        }

        return result;
}

void IPCProcessImpl::processSelectPolicySetRequestEvent(
                        const rina::SelectPolicySetRequestEvent& event) {
	rina::ScopedLock g(*lock_);
        bool got_in_userspace;
        int result = dispatchSelectPolicySet(event.path, event.name, got_in_userspace);

        if (got_in_userspace) {
                // Event managed without going through kernelspace. Notify
                // the IPC Manager about the result
		rina::extendedIPCManager->selectPolicySetResponse(event,
                                                                  result);
                return;
        }

        // Forward the request to the kernel
	try {
		unsigned int handle =
                        rina::kernelIPCProcess->selectPolicySet(event.path,
                                                                event.name);
		pending_select_policy_set_events.insert(
                        std::pair<unsigned int,
			rina::SelectPolicySetRequestEvent>(handle, event));
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending select-policy-set request "
                        "to the kernel: %s", e.what());
		rina::extendedIPCManager->selectPolicySetResponse(event, -1);
	}
}

void IPCProcessImpl::processSelectPolicySetResponseEvent(
                        const rina::SelectPolicySetResponseEvent& event) {
	rina::ScopedLock g(*lock_);
	std::map<unsigned int,
                 rina::SelectPolicySetRequestEvent>::iterator it;

	it = pending_select_policy_set_events.find(event.sequenceNumber);
	if (it == pending_select_policy_set_events.end()) {
		LOG_IPCP_ERR("Couldn't find a select-policy-set request event "
                        "associated to the handle %u", event.sequenceNumber);
		return;
	}

	rina::SelectPolicySetRequestEvent requestEvent = it->second;

	pending_select_policy_set_events.erase(it);
	if (event.result != 0) {
		LOG_IPCP_ERR("The kernel couldn't successfully process the "
                        "select-policy-set Request: %d", event.result);

		try {
			rina::extendedIPCManager->selectPolicySetResponse(it->second, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	LOG_IPCP_DBG("The kernel processed successfully the "
                "set-policy-set-param request");

	try {
		rina::extendedIPCManager->selectPolicySetResponse(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

void IPCProcessImpl::processPluginLoadRequestEvent(
                        const rina::PluginLoadRequestEvent& event) {
		rina::ScopedLock g(*lock_);
        int result;

        if (event.load) {
                result = plugin_load(PLUGINSDIR, event.name);
        } else {
                result = plugin_unload(event.name);
        }
        rina::extendedIPCManager->pluginLoadResponse(event, result);

        return;
}

//Event loop handlers
static void
ipc_process_dif_registration_notification_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::IPCProcessDIFRegistrationEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->resource_allocator_->get_n_minus_one_flow_manager()->
			processRegistrationNotification(*event);
}

static void
assign_to_dif_request_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::AssignToDIFRequestEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->processAssignToDIFRequestEvent(*event);
}

static void
assign_to_dif_response_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::AssignToDIFResponseEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->processAssignToDIFResponseEvent(*event);
}

static void
allocate_flow_request_result_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::AllocateFlowRequestResultEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->resource_allocator_->get_n_minus_one_flow_manager()->
			allocateRequestResult(*event);
}

static void
flow_allocation_requested_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::FlowRequestEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	if (event->localRequest) {
		//A local application is requesting this IPC Process to allocate a flow
		ipcp->flow_allocator_->submitAllocateRequest(*event);
	} else {
		//A remote IPC process is requesting a flow to this IPC Process
		ipcp->resource_allocator_->get_n_minus_one_flow_manager()->
				flowAllocationRequested(*event);
	}
}

static void
deallocate_flow_response_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::DeallocateFlowResponseEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->resource_allocator_->get_n_minus_one_flow_manager()->
					deallocateFlowResponse(*event);
}

static void
flow_deallocated_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::FlowDeallocatedEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->resource_allocator_->get_n_minus_one_flow_manager()->
					flowDeallocatedRemotely(*event);
}

static void
enroll_to_dif_request_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::EnrollToDAFRequestEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->enrollment_task_->processEnrollmentRequestEvent(event);
}

static void
ipc_process_query_rib_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::QueryRIBRequestEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->rib_daemon_->processQueryRIBRequestEvent(*event);
	ipcp->requestPDUFTEDump();
}

static void
application_registration_request_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::ApplicationRegistrationRequestEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->namespace_manager_->processApplicationRegistrationRequestEvent(*event);
}

static void
application_unregistration_request_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::ApplicationUnregistrationRequestEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->namespace_manager_->processApplicationUnregistrationRequestEvent(*event);
}

static void
ipc_process_create_connection_response_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::CreateConnectionResponseEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->flow_allocator_->processCreateConnectionResponseEvent(*event);
}

static void
allocate_flow_response_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::AllocateFlowResponseEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->flow_allocator_->submitAllocateResponse(*event);
}

static void
ipc_process_create_connection_result_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::CreateConnectionResultEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->flow_allocator_->processCreateConnectionResultEvent(*event);
}

static void
ipc_process_update_connection_response_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::UpdateConnectionResponseEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->flow_allocator_->processUpdateConnectionResponseEvent(*event);
}

static void
flow_deallocation_requested_event_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::FlowDeallocateRequestEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->flow_allocator_->submitDeallocate(*event);
}

static void
ipc_process_destroy_connection_result_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::DestroyConnectionResultEvent, event);
	(void) opaque;

	if (event->result != 0){
		LOG_IPCP_WARN("Problems destroying connection with associated to port-id %d",
				event->portId);
	}
}

static void
ipc_process_dump_ft_response_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	DOWNCAST_DECL(e, rina::DumpFTResponseEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->logPDUFTE(*event);
}

static void
ipc_process_set_policy_set_param_handler(rina::IPCEvent *e,
		                         EventLoopData *opaque)

{
	DOWNCAST_DECL(e, rina::SetPolicySetParamRequestEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->processSetPolicySetParamRequestEvent(*event);
}

static void
ipc_process_set_policy_set_param_response_handler(rina::IPCEvent *e,
		                                  EventLoopData *opaque)

{
	DOWNCAST_DECL(e, rina::SetPolicySetParamResponseEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->processSetPolicySetParamResponseEvent(*event);
}

static void
ipc_process_select_policy_set_handler(rina::IPCEvent *e,
		                      EventLoopData *opaque)

{
	DOWNCAST_DECL(e, rina::SelectPolicySetRequestEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->processSelectPolicySetRequestEvent(*event);
}

static void
ipc_process_select_policy_set_response_handler(rina::IPCEvent *e,
		                               EventLoopData *opaque)

{
	DOWNCAST_DECL(e, rina::SelectPolicySetResponseEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->processSelectPolicySetResponseEvent(*event);
}

static void
ipc_process_plugin_load_handler(rina::IPCEvent *e,
		                EventLoopData *opaque)

{
	DOWNCAST_DECL(e, rina::PluginLoadRequestEvent, event);
	DOWNCAST_DECL(opaque, IPCProcessImpl, ipcp);

	ipcp->processPluginLoadRequestEvent(*event);
}

static void
ipc_process_default_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	(void) opaque;

	LOG_IPCP_WARN("Received unsupported event: %d", e->eventType);
}

void register_handlers_all(EventLoop& loop) {
	loop.register_event(rina::IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION,
			ipc_process_dif_registration_notification_handler);
	loop.register_event(rina::ASSIGN_TO_DIF_REQUEST_EVENT,
			assign_to_dif_request_event_handler);
	loop.register_event(rina::ASSIGN_TO_DIF_RESPONSE_EVENT,
			assign_to_dif_response_event_handler);
	loop.register_event(rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT,
			allocate_flow_request_result_event_handler);
	loop.register_event(rina::FLOW_ALLOCATION_REQUESTED_EVENT,
			flow_allocation_requested_event_handler);
	loop.register_event(rina::DEALLOCATE_FLOW_RESPONSE_EVENT,
			deallocate_flow_response_event_handler);
	loop.register_event(rina::FLOW_DEALLOCATED_EVENT,
			flow_deallocated_event_handler);
	loop.register_event(rina::FLOW_DEALLOCATION_REQUESTED_EVENT,
			flow_deallocation_requested_event_handler);
	loop.register_event(rina::ALLOCATE_FLOW_RESPONSE_EVENT,
			allocate_flow_response_event_handler);
	loop.register_event(rina::APPLICATION_REGISTRATION_REQUEST_EVENT,
			application_registration_request_event_handler);
	loop.register_event(rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT,
			application_unregistration_request_event_handler);
	loop.register_event(rina::ENROLL_TO_DIF_REQUEST_EVENT,
			enroll_to_dif_request_event_handler);
	loop.register_event(rina::IPC_PROCESS_QUERY_RIB,
			ipc_process_query_rib_handler);
	loop.register_event(rina::IPC_PROCESS_CREATE_CONNECTION_RESPONSE,
			ipc_process_create_connection_response_handler);
	loop.register_event(rina::IPC_PROCESS_CREATE_CONNECTION_RESULT,
			ipc_process_create_connection_result_handler);
	loop.register_event(rina::IPC_PROCESS_UPDATE_CONNECTION_RESPONSE,
			ipc_process_update_connection_response_handler);
	loop.register_event(rina::IPC_PROCESS_DESTROY_CONNECTION_RESULT,
			ipc_process_destroy_connection_result_handler);
	loop.register_event(rina::IPC_PROCESS_DUMP_FT_RESPONSE,
			ipc_process_dump_ft_response_handler);
        loop.register_event(rina::IPC_PROCESS_SET_POLICY_SET_PARAM,
                        ipc_process_set_policy_set_param_handler);
        loop.register_event(rina::IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE,
                        ipc_process_set_policy_set_param_response_handler);
        loop.register_event(rina::IPC_PROCESS_SELECT_POLICY_SET,
                        ipc_process_select_policy_set_handler);
        loop.register_event(rina::IPC_PROCESS_SELECT_POLICY_SET_RESPONSE,
                        ipc_process_select_policy_set_response_handler);
        loop.register_event(rina::IPC_PROCESS_PLUGIN_LOAD,
                        ipc_process_plugin_load_handler);

	//Unsupported events
	loop.register_event(rina::APPLICATION_UNREGISTERED_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::REGISTER_APPLICATION_RESPONSE_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::UNREGISTER_APPLICATION_RESPONSE_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::APPLICATION_REGISTRATION_CANCELED_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::UPDATE_DIF_CONFIG_REQUEST_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::ENROLL_TO_DIF_RESPONSE_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::GET_DIF_PROPERTIES,
			ipc_process_default_handler);
	loop.register_event(rina::GET_DIF_PROPERTIES_RESPONSE_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::OS_PROCESS_FINALIZED,
			ipc_process_default_handler);
	loop.register_event(rina::IPCM_REGISTER_APP_RESPONSE_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
			ipc_process_default_handler);
	loop.register_event(rina::QUERY_RIB_RESPONSE_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT,
			ipc_process_default_handler);
	loop.register_event(rina::TIMER_EXPIRED_EVENT,
			ipc_process_default_handler);
        loop.register_event(rina::IPC_PROCESS_PLUGIN_LOAD_RESPONSE,
                        ipc_process_default_handler);
}

}
