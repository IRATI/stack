//
// Implementation of the IPC Process
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <cstdlib>
#include <sstream>
#include <iostream>

#define IPCP_MODULE "normal-ipcp"
#include "ipcp-logging.h"

#include "ipcp/enrollment-task.h"
#include "ipcp/flow-allocator.h"
#include "ipcp/ipc-process.h"
#include "ipcp/namespace-manager.h"
#include "ipcp/resource-allocator.h"
#include "ipcp/rib-daemon.h"
#include "ipcp/components.h"
#include "ipcp/utils.h"

namespace rinad {

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

//Class ExpireOldIPCPAddressTimerTask
class ExpireOldIPCPAddressTimerTask: public rina::TimerTask {
public:
	ExpireOldIPCPAddressTimerTask(IPCProcessImpl * ipcp);
	~ExpireOldIPCPAddressTimerTask() throw() {};
	void run();
	std::string name() const {
		return "expire-old-ipcp-address";
	}

private:
	IPCProcessImpl * ipcp;
};

ExpireOldIPCPAddressTimerTask::ExpireOldIPCPAddressTimerTask(IPCProcessImpl * ipc_process)
{
	ipcp = ipc_process;
}

void ExpireOldIPCPAddressTimerTask::run()
{
	ipcp->expire_old_address();
}

//Class UseNewIPCPAddressTimerTask
class UseNewIPCPAddressTimerTask: public rina::TimerTask {
public:
	UseNewIPCPAddressTimerTask(IPCProcessImpl * ipcp);
	~UseNewIPCPAddressTimerTask() throw() {};
	void run();
	std::string name() const {
		return "use-new-ipcp-address";
	}

private:
	IPCProcessImpl * ipcp;
};

UseNewIPCPAddressTimerTask::UseNewIPCPAddressTimerTask(IPCProcessImpl * ipc_process)
{
	ipcp = ipc_process;
}

void UseNewIPCPAddressTimerTask::run()
{
	ipcp->activate_new_address();
}

//Class IPCProcessImpl
IPCProcessImpl::IPCProcessImpl(const rina::ApplicationProcessNamingInformation& nm,
			       unsigned short id,
			       unsigned int ipc_manager_port,
			       std::string log_level,
			       std::string log_file) : IPCProcess(nm.processName, nm.processInstance),
					       	       LazyIPCProcessImpl(nm, id, ipc_manager_port, log_level, log_file)
{
	old_address = 0;
	address_change_period = false;
	use_new_address = false;
	kernel_sync = NULL;

        // Initialize application entities
        delimiter_ = 0; //TODO initialize Delimiter once it is implemented
        internal_event_manager_ = new rina::SimpleInternalEventManager();
        enrollment_task_ = new EnrollmentTask();
        flow_allocator_ = new FlowAllocator();
        namespace_manager_ = new NamespaceManager();
        resource_allocator_ = new ResourceAllocator();
        security_manager_ = new IPCPSecurityManager();
        routing_component_ = new RoutingComponent();
        rib_daemon_ = new IPCPRIBDaemonImpl(enrollment_task_);

        add_entity(internal_event_manager_);
        add_entity(rib_daemon_);
        add_entity(enrollment_task_);
        add_entity(routing_component_);
        add_entity(resource_allocator_->get_n_minus_one_flow_manager());
        add_entity(resource_allocator_);
        add_entity(namespace_manager_);
        add_entity(flow_allocator_);
        add_entity(security_manager_);

        subscribeToEvents();

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

IPCProcessImpl::~IPCProcessImpl()
{
	if (kernel_sync) {
		kernel_sync->finish();
		kernel_sync->join(NULL);
		delete kernel_sync;
	}

	if (delimiter_) {
		delete delimiter_;
	}

	delete dynamic_cast<FlowAllocator*>(flow_allocator_);
	delete dynamic_cast<IPCPSecurityManager*>(security_manager_);
	delete dynamic_cast<NamespaceManager*>(namespace_manager_);
	delete dynamic_cast<RoutingComponent*>(routing_component_);
	delete dynamic_cast<ResourceAllocator*>(resource_allocator_);
	delete dynamic_cast<IPCPRIBDaemonImpl*>(rib_daemon_);
	delete dynamic_cast<EnrollmentTask*>(enrollment_task_);
}

void IPCProcessImpl::subscribeToEvents()
{
	internal_event_manager_->subscribeToEvent(rina::InternalEvent::ADDRESS_CHANGE,
					 	  this);
}

void IPCProcessImpl::eventHappened(rina::InternalEvent * event)
{
	if (event->type == rina::InternalEvent::ADDRESS_CHANGE){
		rina::AddressChangeEvent * addrEvent =
				(rina::AddressChangeEvent *) event;
		addressChange(addrEvent);
	}
}

void IPCProcessImpl::addressChange(rina::AddressChangeEvent * event)
{
	rina::TimerTask * task = 0;

	rina::ScopedLock g(*lock_);
	old_address = event->old_address;
	dif_information_.dif_configuration_.address_ = event->new_address;
	address_change_period = true;
	use_new_address = false;

	task = new UseNewIPCPAddressTimerTask(this);
	timer.scheduleTask(task, event->use_new_timeout);

	task = new ExpireOldIPCPAddressTimerTask(this);
	timer.scheduleTask(task, event->deprecate_old_timeout);

	rina::kernelIPCProcess->changeAddress(event->new_address,
					      event->old_address,
					      event->use_new_timeout,
					      event->deprecate_old_timeout);
}

unsigned short IPCProcessImpl::get_id()
{
	return rina::extendedIPCManager->ipcProcessId;
}

const IPCProcessOperationalState& IPCProcessImpl::get_operational_state() const
{
	rina::ScopedLock g(*lock_);
	return state;
}

void IPCProcessImpl::set_operational_state(const IPCProcessOperationalState& operational_state)
{
	rina::ScopedLock g(*lock_);
	state = operational_state;
}

rina::DIFInformation& IPCProcessImpl::get_dif_information()
{
	rina::ScopedLock g(*lock_);
	return dif_information_;
}

void IPCProcessImpl::set_dif_information(const rina::DIFInformation& dif_information)
{
	rina::ScopedLock g(*lock_);
	dif_information_ = dif_information;
}

const std::list<rina::Neighbor> IPCProcessImpl::get_neighbors() const
{
	return enrollment_task_->get_neighbors();
}

unsigned int IPCProcessImpl::get_address() const
{
	rina::ScopedLock g(*lock_);
	if (state != ASSIGNED_TO_DIF) {
		return 0;
	}

	return dif_information_.dif_configuration_.address_;
}

unsigned int IPCProcessImpl::get_active_address()
{
	rina::ScopedLock g(*lock_);
	if (state != ASSIGNED_TO_DIF) {
		return 0;
	}

	if (address_change_period && !use_new_address)
		return old_address;

	return dif_information_.dif_configuration_.address_;
}

void IPCProcessImpl::set_address(unsigned int address)
{
	rina::ScopedLock g(*lock_);
	dif_information_.dif_configuration_.address_ = address;
}

void IPCProcessImpl::expire_old_address()
{
	rina::ScopedLock g(*lock_);
	address_change_period = false;
	use_new_address = false;
	old_address = 0;
}

void IPCProcessImpl::activate_new_address(void)
{
	rina::ScopedLock g(*lock_);
	use_new_address = true;
}

bool IPCProcessImpl::check_address_is_mine(unsigned int address)
{
	rina::ScopedLock g(*lock_);
	if (address == 0)
		return false;

	if (address == old_address ||
			address == dif_information_.dif_configuration_.address_)
		return true;

	return false;
}

unsigned int IPCProcessImpl::get_old_address()
{
	rina::ScopedLock g(*lock_);
	return old_address;
}

void IPCProcessImpl::requestPDUFTEDump()
{
	try{
		rina::kernelIPCProcess->dumptPDUFT();
	} catch (rina::Exception &e) {
		LOG_IPCP_WARN("Error requesting IPC Process to Dump PDU Forwarding Table: %s", e.what());
	}
}

void IPCProcessImpl::sync_with_kernel()
{
	flow_allocator_->sync_with_kernel();
	resource_allocator_->sync_with_kernel();
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

void IPCProcessImpl::dif_registration_notification_handler(const rina::IPCProcessDIFRegistrationEvent& event)
{
	resource_allocator_->get_n_minus_one_flow_manager()->processRegistrationNotification(event);
}

void IPCProcessImpl::assign_to_dif_request_handler(const rina::AssignToDIFRequestEvent& event)
{
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

void IPCProcessImpl::assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event)
{
	rina::ApplicationRegistrationInformation ari;

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
		pending_events_.erase(it);
		state = INITIALIZED;

		try {
			rina::extendedIPCManager->assignToDIFResponse(requestEvent, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	try{
		rib_daemon_->set_dif_configuration(dif_information_);
		resource_allocator_->set_dif_configuration(dif_information_);
		routing_component_->set_dif_configuration(dif_information_);
		namespace_manager_->set_dif_configuration(dif_information_);
		security_manager_->set_dif_configuration(dif_information_);
		flow_allocator_->set_dif_configuration(dif_information_);
		enrollment_task_->set_dif_configuration(dif_information_);
	} catch(rina::Exception &e) {
		state = INITIALIZED;
		LOG_IPCP_ERR("Bad configuration error: %s", e.what());
		rina::extendedIPCManager->assignToDIFResponse(requestEvent, -1);
	}

	try {
		rina::extendedIPCManager->assignToDIFResponse(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}

	state = ASSIGNED_TO_DIF;

	//TODO Avoid starting kernel sync trigger thread for the moment
	//kernel_sync = new KernelSyncTrigger(this, 4000);
	//kernel_sync->start();
}

void IPCProcessImpl::allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event)
{
	resource_allocator_->get_n_minus_one_flow_manager()->allocateRequestResult(event);
}

void IPCProcessImpl::flow_allocation_requested_handler(const rina::FlowRequestEvent& event)
{
	if (event.localRequest)
		//A local application is requesting this IPC Process to allocate a flow
		flow_allocator_->submitAllocateRequest(event);
	else
		//A remote IPC process is requesting a flow to this IPC Process
		resource_allocator_->get_n_minus_one_flow_manager()->flowAllocationRequested(event);
}

void IPCProcessImpl::flow_deallocated_handler(const rina::FlowDeallocatedEvent& event)
{
	resource_allocator_->get_n_minus_one_flow_manager()->flowDeallocatedRemotely(event);
}

void IPCProcessImpl::flow_deallocation_requested_handler(const rina::FlowDeallocateRequestEvent& event)
{
	flow_allocator_->submitDeallocate(event);
}

void IPCProcessImpl::allocate_flow_response_handler(const rina::AllocateFlowResponseEvent& event)
{
	flow_allocator_->submitAllocateResponse(event);
}

void IPCProcessImpl::application_registration_request_handler(const rina::ApplicationRegistrationRequestEvent& event)
{
	namespace_manager_->processApplicationRegistrationRequestEvent(event);
}

void IPCProcessImpl::application_unregistration_handler(const rina::ApplicationUnregistrationRequestEvent& event)
{
	namespace_manager_->processApplicationUnregistrationRequestEvent(event);
}

void IPCProcessImpl::enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event)
{
	enrollment_task_->processEnrollmentRequestEvent(event);
}

void IPCProcessImpl::disconnet_neighbor_handler(const rina::DisconnectNeighborRequestEvent& event)
{
	enrollment_task_->processDisconnectNeighborRequestEvent(event);
}

void IPCProcessImpl::query_rib_handler(const rina::QueryRIBRequestEvent& event)
{
	rib_daemon_->processQueryRIBRequestEvent(event);
	requestPDUFTEDump();
}

void IPCProcessImpl::create_efcp_connection_response_handler(const rina::CreateConnectionResponseEvent& event)
{
	flow_allocator_->processCreateConnectionResponseEvent(event);
}

void IPCProcessImpl::create_efcp_connection_result_handler(const rina::CreateConnectionResultEvent& event)
{
	flow_allocator_->processCreateConnectionResultEvent(event);
}

void IPCProcessImpl::update_efcp_connection_response_handler(const rina::UpdateConnectionResponseEvent& event)
{
	flow_allocator_->processUpdateConnectionResponseEvent(event);
}

void IPCProcessImpl::destroy_efcp_connection_result_handler(const rina::DestroyConnectionResultEvent& event)
{
	if (event.result != 0) {
		LOG_IPCP_WARN("Problems destroying connection with associated to port-id %d",
			      event.portId);
	}
}

void IPCProcessImpl::dump_ft_response_handler(const rina::DumpFTResponseEvent& event)
{
	if (event.result != 0) {
		LOG_IPCP_WARN("Dump PDU FT operation returned error, error code: %d",
				event.getResult());
		return;
	}

	std::list<rina::PDUForwardingTableEntry>::const_iterator it;
	std::list<rina::PortIdAltlist>::const_iterator it2;
	std::list<unsigned int>::const_iterator it3;
	std::stringstream ss;
	ss << "Contents of the PDU Forwarding Table: " << std::endl;
	for (it = event.entries.begin(); it != event.entries.end(); ++it) {
		ss << "Address: " << it->address << "; QoS-id: ";
		ss << it->qosId << "; Port-ids: ";
		for (it2 = it->portIdAltlists.begin(); it2 != it->portIdAltlists.end(); ++it2) {
			for (it3 = it2->alts.begin(); it3 != it2->alts.end(); it3++) {
				ss << (*it3) << ", ";
			}
			ss << ";";
		}
		ss << std::endl;
	}

	LOG_IPCP_DBG("%s", ss.str().c_str());
}

void IPCProcessImpl::set_policy_set_param_handler(const rina::SetPolicySetParamRequestEvent& event)
{
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
        } else if (component == "enrollment-task") {
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

void IPCProcessImpl::set_policy_set_param_response_handler(const rina::SetPolicySetParamResponseEvent& event)
{
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

void IPCProcessImpl::select_policy_set_handler(const rina::SelectPolicySetRequestEvent& event)
{
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

void IPCProcessImpl::select_policy_set_response_handler(const rina::SelectPolicySetResponseEvent& event)
{
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
                     "select-policy-set request");

	try {
		rina::extendedIPCManager->selectPolicySetResponse(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

void IPCProcessImpl::plugin_load_handler(const rina::PluginLoadRequestEvent& event)
{
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

void IPCProcessImpl::update_crypto_state_response_handler(const rina::UpdateCryptoStateResponseEvent& event)
{
	security_manager_->process_update_crypto_state_response(event);
}

void IPCProcessImpl::fwd_cdap_msg_handler(rina::FwdCDAPMsgRequestEvent& event)
{
	if (!event.sermsg.message_) {
		LOG_IPCP_ERR("No CDAP message to be forwarded");
		return;
	}

	rina::cdap::getProvider()->process_message(event.sermsg,
						   event.sequenceNumber,
						   rina::cdap_rib::CDAP_DEST_IPCM);

        return;
}

void IPCProcessImpl::ipcp_allocate_port_response_event_handler(const rina::AllocatePortResponseEvent& event)
{
	flow_allocator_->processAllocatePortResponse(event);
}

void IPCProcessImpl::ipcp_deallocate_port_response_event_handler(const rina::DeallocatePortResponseEvent& event)
{
	flow_allocator_->processDeallocatePortResponse(event);
}

void IPCProcessImpl::ipcp_write_mgmt_sdu_response_event_handler(const rina::WriteMgmtSDUResponseEvent& event)
{
	//Ignore for now, assuming all works well
	//TODO add error handler in the RIB Daemon
}

void IPCProcessImpl::ipcp_read_mgmt_sdu_notif_event_handler(rina::ReadMgmtSDUResponseEvent& event)
{
	rib_daemon_->processReadManagementSDUEvent(event);
}


} //namespace rinad
