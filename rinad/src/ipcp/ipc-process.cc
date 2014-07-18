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

#define RINA_PREFIX "ipc-process"

#include <librina/logs.h>
#include "ipcp/enrollment-task.h"
#include "ipcp/flow-allocator.h"
#include "ipcp/ipc-process.h"
#include "ipcp/namespace-manager.h"
#include "ipcp/resource-allocator.h"
#include "ipcp/rib-daemon.h"
#include "ipcp/security-manager.h"

namespace rinad {

#define IPCP_LOG_FILE_PREFIX "/tmp/ipcp-log-file"

//Class IPCProcessImpl
IPCProcessImpl::IPCProcessImpl(const rina::ApplicationProcessNamingInformation& name,
		unsigned short id, unsigned int ipc_manager_port) {
	try {
		std::stringstream ss;
		ss << IPCP_LOG_FILE_PREFIX << "-" << id;
		rina::initialize(LOG_LEVEL_DBG, ss.str());
		rina::extendedIPCManager->ipcManagerPort = ipc_manager_port;
		rina::extendedIPCManager->ipcProcessId = id;
		rina::kernelIPCProcess->ipcProcessId = id;
		LOG_INFO("Librina initialized");
	} catch (Exception &e) {
        std::cerr << "Cannot initialize librina" << std::endl;
        exit(EXIT_FAILURE);
	}

	name_ = name;
	lock_ = new rina::Lockable();

	init_delimiter();
	init_cdap_session_manager();
	init_encoder();
	init_enrollment_task();
	init_flow_allocator();
	init_namespace_manager();
	init_resource_allocator();
	init_security_manager();
	init_rib_daemon();

	rib_daemon_->set_ipc_process(this);
	enrollment_task_->set_ipc_process(this);
	resource_allocator_->set_ipc_process(this);
	namespace_manager_->set_ipc_process(this);
	flow_allocator_->set_ipc_process(this);
	security_manager_->set_ipc_process(this);

	populate_rib();

	try {
		rina::extendedIPCManager->notifyIPCProcessInitialized(name_);
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with IPC Manager: %s. Exiting... ", e.what());
		exit(EXIT_FAILURE);
	}

	state_ = INITIALIZED;

	LOG_INFO("Initialized IPC Process with name: %s, instance %s, id %hu ",
			name_.processName.c_str(), name_.processInstance.c_str(), id);
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

	if (cdap_session_manager_) {
		delete cdap_session_manager_;
	}

	if (enrollment_task_) {
		delete enrollment_task_;
	}

	if (flow_allocator_) {
		delete flow_allocator_;
	}

	if (namespace_manager_) {
		delete namespace_manager_;
	}

	if (resource_allocator_) {
		delete resource_allocator_;
	}

	if (security_manager_) {
		delete security_manager_;
	}

	if (rib_daemon_) {
		delete rib_daemon_;
	}
}

void IPCProcessImpl::init_delimiter() {
	//TODO initialize Delimiter once it is implemented
	delimiter_ = 0;
}

void IPCProcessImpl::init_cdap_session_manager() {
	rina::WireMessageProviderFactory wire_factory_;
	rina::CDAPSessionManagerFactory cdap_manager_factory_;
	long timeout = 10000;
	cdap_session_manager_ = cdap_manager_factory_.createCDAPSessionManager(
			&wire_factory_, timeout);
}

void IPCProcessImpl::init_encoder() {
	encoder_ = new rinad::Encoder();
	//TODO add backend encoders as they become available
	//encoder_->addEncoder(rinad::EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS,
	//		new FakeEncoder());
}

void IPCProcessImpl::init_enrollment_task() {
	enrollment_task_ = new EnrollmentTask();
}

void IPCProcessImpl::init_flow_allocator() {
	flow_allocator_ = new FlowAllocator();
}

void IPCProcessImpl::init_namespace_manager() {
	namespace_manager_ = new NamespaceManager();
}

void IPCProcessImpl::init_resource_allocator() {
	resource_allocator_ = new ResourceAllocator();
}

void IPCProcessImpl::init_security_manager() {
	security_manager_ = new SecurityManager();
}

void IPCProcessImpl::init_rib_daemon() {
	rib_daemon_ = new RIBDaemon();
}

void IPCProcessImpl::populate_rib() {
	//TODO Create RIB objects not created by IPC Process components
}

unsigned short IPCProcessImpl::get_id() {
	return rina::extendedIPCManager->ipcProcessId;
}

const rina::ApplicationProcessNamingInformation& IPCProcessImpl::get_name() const {
	return name_;
}

IDelimiter * IPCProcessImpl::get_delimiter() {
	return delimiter_;
}

Encoder * IPCProcessImpl::get_encoder() {
	return encoder_;
}

rina::CDAPSessionManagerInterface* IPCProcessImpl::get_cdap_session_manager() {
	return cdap_session_manager_;
}

IEnrollmentTask * IPCProcessImpl::get_enrollment_task() {
	return enrollment_task_;
}

IFlowAllocator * IPCProcessImpl::get_flow_allocator() {
	return flow_allocator_;
}

INamespaceManager * IPCProcessImpl::get_namespace_manager() {
	return namespace_manager_;
}

IResourceAllocator * IPCProcessImpl::get_resource_allocator() {
	return resource_allocator_;
}

ISecurityManager * IPCProcessImpl::get_security_manager() {
	return security_manager_;
}

IRIBDaemon * IPCProcessImpl::get_rib_daemon() {
	return rib_daemon_;
}

const std::list<rina::Neighbor*> IPCProcessImpl::get_neighbors() const {
	return enrollment_task_->get_neighbors();
}

const IPCProcessOperationalState& IPCProcessImpl::get_operational_state() const {
	rina::AccessGuard g(*lock_);
	return state_;
}

void IPCProcessImpl::set_operational_state(const IPCProcessOperationalState& operational_state) {
	rina::AccessGuard g(*lock_);
	state_ = operational_state;
}

const rina::DIFInformation& IPCProcessImpl::get_dif_information() const {
	rina::AccessGuard g(*lock_);
	return dif_information_;
}

void IPCProcessImpl::set_dif_information(const rina::DIFInformation& dif_information) {
	rina::AccessGuard g(*lock_);
	dif_information_ = dif_information;
}

unsigned int IPCProcessImpl::get_address() const {
	rina::AccessGuard g(*lock_);
	if (state_ != ASSIGNED_TO_DIF) {
		return 0;
	}

	return dif_information_.dif_configuration_.address_;
}

void IPCProcessImpl::set_address(unsigned int address) {
	rina::AccessGuard g(*lock_);
	dif_information_.dif_configuration_.address_ = address;
}

void IPCProcessImpl::processAssignToDIFRequestEvent(const rina::AssignToDIFRequestEvent& event) {
	rina::AccessGuard g(*lock_);

	if (state_ != INITIALIZED) {
		//The IPC Process can only be assigned to a DIF once, reply with error message
		LOG_ERR("Got a DIF assignment request while not in INITIALIZED state. Current state is: %d",
				state_);
		rina::extendedIPCManager->assignToDIFResponse(event, -1);
		return;
	}

	try {
		unsigned int handle = rina::kernelIPCProcess->assignToDIF(event.difInformation);
		pending_events_[handle] = event;
		state_ = ASSIGN_TO_DIF_IN_PROCESS;
	} catch (Exception &e) {
		LOG_ERR("Problems sending DIF Assignment request to the kernel: %s", e.what());
		rina::extendedIPCManager->assignToDIFResponse(event, -1);
	}
}

void IPCProcessImpl::processAssignToDIFResponseEvent(const rina::AssignToDIFResponseEvent& event) {
	//TODO
	(void) event;
}

void IPCProcessImpl::requestPDUFTEDump() {
	try{
		rina::kernelIPCProcess->dumptPDUFT();
	} catch (Exception &e) {
		LOG_WARN("Error requesting IPC Process to Dump PDU Forwarding Table: %s", e.what());
	}
}

void IPCProcessImpl::logPDUFTE(const rina::DumpFTResponseEvent& event) {
	if (event.result != 0) {
		LOG_WARN("Dump PDU FT operation returned error, error code: %d",
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

	LOG_INFO("%s", ss.str().c_str());
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

	if (event->isLocalRequest()) {
		//A local application is requesting this IPC Process to allocate a flow
		ipcp->flow_allocator_->submitAllocateRequest(event);
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
	DOWNCAST_DECL(e, rina::EnrollToDIFRequestEvent, event);
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
		LOG_WARN("Problems destroying connection with associated to port-id %d",
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
ipc_process_default_handler(rina::IPCEvent *e,
		EventLoopData *opaque)
{
	(void) opaque;

	LOG_WARN("Received unsupported event: %d", e->eventType);
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
	loop.register_event(rina::NEIGHBORS_MODIFIED_NOTIFICATION_EVENT,
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

}

}
