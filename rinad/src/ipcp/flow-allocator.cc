//
// Flow Allocator
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
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

#include <climits>
#include <sstream>
#include <vector>

#define IPCP_MODULE "flow-allocator"
#include "ipcp-logging.h"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "ipcp/ipc-process.h"
#include "flow-allocator.h"
#include "common/encoder.h"
#include "common/configuration.h"
#include "utils.h"

namespace rinad {

//Class Flow RIB Object
const std::string FlowRIBObject::class_name = "Flow";
const std::string FlowRIBObject::object_name_prefix = "/fa/flows/key=";

FlowRIBObject::FlowRIBObject(IPCProcess * ipc_process,
			     IFlowAllocatorInstance * flow_allocator_instance)
	: IPCPRIBObj(ipc_process, class_name)
{
	flow_allocator_instance_ = flow_allocator_instance;
}

void FlowRIBObject::read(const rina::cdap_rib::con_handle_t &con,
			 const std::string& fqn,
			 const std::string& class_,
			 const rina::cdap_rib::filt_info_t &filt,
			 const int invoke_id,
			 rina::cdap_rib::obj_info_t &obj_reply,
			 rina::cdap_rib::res_info_t& res)
{
	configs::Flow * flow = flow_allocator_instance_->get_flow();
	if (flow) {
		encoders::FlowEncoder encoder;
		encoder.encode(*flow, obj_reply.value_);
	}

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

void FlowRIBObject::write(const rina::cdap_rib::con_handle_t &con,
			  const std::string& fqn,
			  const std::string& class_,
			  const rina::cdap_rib::filt_info_t &filt,
			  const int invoke_id,
			  const rina::ser_obj_t &obj_req,
			  rina::ser_obj_t &obj_reply,
			  rina::cdap_rib::res_info_t& res)
{
	encoders::FlowEncoder encoder;
	configs::Flow flow;
	encoder.decode(obj_req, flow);

	flow_allocator_instance_->modify_flow_request(flow);

	//Does not require a response message, return CDAP_PENDING
	res.code_ = rina::cdap_rib::CDAP_PENDING;
}

bool FlowRIBObject::delete_(const rina::cdap_rib::con_handle_t &con,
			    const std::string& fqn,
			    const std::string& class_,
			    const rina::cdap_rib::filt_info_t &filt,
			    const int invoke_id,
			    rina::cdap_rib::res_info_t& res)
{
	flow_allocator_instance_->deleteFlowRequestMessageReceived();
	res.code_ = rina::cdap_rib::CDAP_PENDING;
	return false;
}

void FlowRIBObject::create_cb(const rina::rib::rib_handle_t rib,
			      const rina::cdap_rib::con_handle_t &con,
			      const std::string& fqn,
			      const std::string& class_,
			      const rina::cdap_rib::filt_info_t &filt,
			      const int invoke_id,
			      const rina::ser_obj_t &obj_req,
			      rina::cdap_rib::obj_info_t &obj_reply,
			      rina::cdap_rib::res_info_t& res)
{
	encoders::FlowEncoder encoder;
	configs::Flow rcv_flow;
	encoder.decode(obj_req, rcv_flow);

	configs::Flow * flow = new configs::Flow(rcv_flow);
	IPCPFactory::getIPCP()->flow_allocator_->createFlowRequestMessageReceived(flow,
							  	  	  	  fqn,
							  	  	  	  invoke_id);
	res.code_ = rina::cdap_rib::CDAP_PENDING;
}

const std::string FlowRIBObject::get_displayable_value() const
{
	return flow_allocator_instance_->get_flow()->toString();
}

//Class Flow Set RIB Object
const std::string FlowsRIBObject::class_name = "Flows";
const std::string FlowsRIBObject::object_name = "/fa/flows";

FlowsRIBObject::FlowsRIBObject(IPCProcess * ipc_process,
			       IFlowAllocator * flow_allocator)
	: IPCPRIBObj(ipc_process, class_name)
{
	flow_allocator_ = flow_allocator;
}

//Class Connections RIB Object
const std::string ConnectionsRIBObj::class_name = "Connections";
const std::string ConnectionsRIBObj::object_name = "/dt/conns";

ConnectionsRIBObj::ConnectionsRIBObj(IPCProcess * ipc_process,
			       	     IFlowAllocator * flow_allocator)
	: IPCPRIBObj(ipc_process, class_name)
{
	flow_allocator_ = flow_allocator;
}

//Class Connection RIB Object
const std::string ConnectionRIBObject::class_name = "Connection";
const std::string ConnectionRIBObject::object_name_prefix = "/dt/conns/scepid=";

ConnectionRIBObject::ConnectionRIBObject(IPCProcess * ipc_process,
					 IFlowAllocatorInstance * fai_)
	: IPCPRIBObj(ipc_process, class_name)
{
	fai = fai_;
}

void ConnectionRIBObject::read(const rina::cdap_rib::con_handle_t &con,
			       const std::string& fqn,
			       const std::string& class_,
			       const rina::cdap_rib::filt_info_t &filt,
			       const int invoke_id,
			       rina::cdap_rib::obj_info_t &obj_reply,
			       rina::cdap_rib::res_info_t& res)
{
	encoders::DTPInformationEncoder encoder;
	rina::DTPInformation dtp_info(fai->get_flow()->getActiveConnection());
	encoder.encode(dtp_info, obj_reply.value_);

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

const std::string ConnectionRIBObject::get_displayable_value() const
{
	std::stringstream ss;

	if (!fai || !fai->get_flow())
		return "";

	rina::Connection * con = fai->get_flow()->getActiveConnection();
	if (!con)
		return "";

	ss << "CEP-ids: src = " << con->sourceCepId << ", dest = " << con->destCepId
	   << "; Addresses: src = " << con->sourceAddress << ", dest = " << con->destAddress
	   << "; Qos-id: " << con->qosId << "; Port-id: " << con->portId << std::endl;
	ss << "DTP config: " << con->dtpConfig.toString();
	ss << "Tx: pdus = " << con->stats.tx_pdus << ", Bytes = " << con->stats.tx_bytes
	   << "; RX: pdus = " << con->stats.rx_pdus << ", Bytes = " << con->stats.rx_bytes << std::endl;

	return ss.str();
}

//Class DTCP RIB Object
const std::string DTCPRIBObject::class_name = "DTCP";
const std::string DTCPRIBObject::object_name_suffix = "/dtcp";

DTCPRIBObject::DTCPRIBObject(IPCProcess * ipc_process,
			     IFlowAllocatorInstance * fai_)
	: IPCPRIBObj(ipc_process, class_name)
{
	fai = fai_;
}

void DTCPRIBObject::read(const rina::cdap_rib::con_handle_t &con,
			 const std::string& fqn,
			 const std::string& class_,
			 const rina::cdap_rib::filt_info_t &filt,
			 const int invoke_id,
			 rina::cdap_rib::obj_info_t &obj_reply,
			 rina::cdap_rib::res_info_t& res)
{
	encoders::DTCPInformationEncoder encoder;
	encoder.encode(fai->get_flow()->getActiveConnection()->dtcpConfig,
		       obj_reply.value_);

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

const std::string DTCPRIBObject::get_displayable_value() const
{
	rina::Connection * con = fai->get_flow()->getActiveConnection();
	return con->dtcpConfig.toString();
}

//Class Flow Allocator
const int FlowAllocator::DEALLOCATE_PORT_DELAY = 0;
const int FlowAllocator::TEARDOWN_FLOW_DELAY = 0;

FlowAllocator::FlowAllocator() : IFlowAllocator()
{
	ipcp = 0;
	rib_daemon_ = 0;
	namespace_manager_ = 0;
}

FlowAllocator::~FlowAllocator()
{
	delete ps;
}

IFlowAllocatorInstance * FlowAllocator::getFAI(int portId)
{
	std::map<int, FlowAllocatorInstance *>::iterator it;

	rina::ScopedLock g(fai_lock);

	it = fa_instances.find(portId);
	if (it != fa_instances.end())
		return it->second;

	return NULL;
}

void FlowAllocator::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
		return;

	app = ap;
	ipcp = dynamic_cast<IPCProcess*>(app);
	if (!ipcp) {
		LOG_IPCP_ERR("Bogus instance of IPCP passed, return");
		return;
	}
	rib_daemon_ = ipcp->rib_daemon_;
	namespace_manager_ = ipcp->namespace_manager_;
	populateRIB();
	subscribeToEvents();
}

void FlowAllocator::subscribeToEvents()
{
	ipcp->internal_event_manager_->subscribeToEvent(rina::InternalEvent::ADDRESS_CHANGE,
					 	 	this);
}

void FlowAllocator::eventHappened(rina::InternalEvent * event)
{
	if (event->type == rina::InternalEvent::ADDRESS_CHANGE){
		rina::AddressChangeEvent * addrEvent =
				(rina::AddressChangeEvent *) event;
		FAAddressChangeTimerTask * task = new FAAddressChangeTimerTask(this,
				addrEvent->new_address,
				addrEvent->old_address);
		timer.scheduleTask(task, addrEvent->use_new_timeout);
	}
}

void FlowAllocator::address_changed(unsigned int new_address,
				    unsigned int old_address)
{
	std::map<int, FlowAllocatorInstance *>::iterator it;

	LOG_IPCP_DBG("Local address has changed from %u to &u, notifying EFCP peers",
			new_address, old_address);
	rina::ScopedLock g(fai_lock);

	//Tell all FAIs src address has changed
	for (it = fa_instances.begin(); it != fa_instances.end(); ++it) {
		it->second->address_changed(new_address, old_address);
	}
}

FAAddressChangeTimerTask::FAAddressChangeTimerTask(FlowAllocator * fa,
		       	       	       	       	   unsigned int naddr,
						   unsigned int oaddr)
{
	fall = fa;
	new_address = naddr;
	old_address = oaddr;
}

void FAAddressChangeTimerTask::run()
{
	fall->address_changed(new_address, old_address);
}

void FlowAllocator::set_dif_configuration(const rina::DIFInformation& dif_information)
{
	std::string ps_name = dif_information.dif_configuration_.fa_configuration_.policy_set_.name_;
	if (select_policy_set(std::string(), ps_name) != 0) {
		throw rina::Exception("Cannot create Flow Allocator policy-set");
	}
}

void FlowAllocator::populateRIB()
{
	rina::rib::RIBObj* tmp;
	rina::cdap_rib::vers_info_t vers;
	vers.version_ = 0x1ULL;

	try {
		tmp = new rina::rib::RIBObj("FlowAllocator");
		rib_daemon_->addObjRIB("/fa", &tmp);

		tmp = new FlowsRIBObject(ipcp, this);
		rib_daemon_->addObjRIB(FlowsRIBObject::object_name, &tmp);
		rib_daemon_->getProxy()->addCreateCallbackSchema(vers,
								 FlowRIBObject::class_name,
								 FlowsRIBObject::object_name,
								 FlowRIBObject::create_cb);

		tmp = new DataTransferRIBObj(ipcp);
		rib_daemon_->addObjRIB(DataTransferRIBObj::object_name, &tmp);

		tmp = new ConnectionsRIBObj(ipcp, this);
		rib_daemon_->addObjRIB(ConnectionsRIBObj::object_name, &tmp);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems adding object to the RIB : %s", e.what());
	}
}

void FlowAllocator::createFlowRequestMessageReceived(configs::Flow * flow,
						     const std::string& object_name,
						     int invoke_id)
{
	unsigned int address = 0;
	int portId = 0;
	bool process_flow_request = false;
	rina::ApplicationProcessNamingInformation remote_info, app_info;
	OngoingFlowAllocState flow_state;
	unsigned int seq_num;

	//Check if the flow is to the layer management tasks of this IPCP
	if (flow->remote_naming_info.processName == ipcp->get_name() &&
			flow->remote_naming_info.processInstance == ipcp->get_instance() &&
			flow->remote_naming_info.entityName == IPCProcess::MANAGEMENT_AE) {
		process_flow_request = true;
		flow->internal = true;
	} else {
		address = namespace_manager_->getDFTNextHop(flow->remote_naming_info);
		if (address == 0) {
			LOG_IPCP_ERR("The directory forwarding table returned no entries when looking up %s",
					flow->remote_naming_info.toString().c_str());
			return;
		}

		app_info = flow->remote_naming_info;

		if (ipcp->check_address_is_mine(address)) {
			process_flow_request = true;
			flow->internal = false;
		}
	}

	if (process_flow_request) {
		rina::ScopedLock g(port_alloc_lock);

		LOG_IPCP_DBG("The destination AP is reachable through me");

		//Reverse flow information
		remote_info = flow->remote_naming_info;
		flow->remote_naming_info = flow->local_naming_info;
		flow->local_naming_info = remote_info;
		flow->remote_port_id = flow->local_port_id;
		flow->remote_address = flow->local_address;
		flow->local_address = ipcp->get_active_address();
		rina::Connection * connection = flow->getActiveConnection();
		address = connection->getSourceAddress();
		connection->setSourceAddress(connection->getDestAddress());
		connection->setDestAddress(address);
		connection->setDestCepId(connection->getSourceCepId());

		try {
			seq_num = rina::extendedIPCManager->allocatePortId(app_info,
									   flow->flow_specification);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems requesting an available port-id to the Kernel IPC Manager: %s",
					e.what());
			return;
		}

		flow_state.local_request = false;
		flow_state.flow = flow;
		flow_state.object_name = object_name;
		flow_state.invoke_id = invoke_id;

		pending_port_allocs[seq_num] = flow_state;
		return;
	}

	//The address is not this IPC process, forward the CDAP message to that address increment the hop
	//count of the Flow object extract the flow object from the CDAP message
	flow->hop_count = flow->hop_count - 1;
	if (flow->hop_count <= 0) {
		//TODO send negative create Flow response CDAP message to the source IPC process,
		//saying that the application process could not be found before the hop count expired
		LOG_IPCP_ERR("Missing code");
	}

	LOG_IPCP_ERR("Missing code");
}

void FlowAllocator::__createFlowRequestMessageReceived(configs::Flow * flow,
	             	     	     		       const std::string& object_name,
						       int invoke_id,
						       int port_id)
{
	FlowAllocatorInstance * fai;
	std::stringstream ss;

	rina::ScopedLock g(fai_lock);

	ss << port_id;
	fai = new FlowAllocatorInstance(ipcp,
					this,
					port_id,
					false,
					ss.str(),
					&timer);
	fa_instances[port_id] = fai;

	fai->createFlowRequestMessageReceived(flow,
					      object_name,
					      invoke_id);
	return;
}

void FlowAllocator::replyToIPCManager(const rina::FlowRequestEvent& event,
				      int result)
{
	try {
		rina::extendedIPCManager->allocateFlowRequestResult(event,
								    result);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager Daemon: %s",
			     e.what());
	}
}

void FlowAllocator::submitAllocateRequest(const rina::FlowRequestEvent& event,
					  unsigned int address)
{
	unsigned int seq_num = 0;
	OngoingFlowAllocState flow_state;
	rina::ApplicationProcessNamingInformation app_info;

	LOG_IPCP_DBG("Trying to allocate flow between %s and %s",
			event.localApplicationName.toString().c_str(),
			event.remoteApplicationName.toString().c_str());

	if (!event.internal) {
		app_info = event.localApplicationName;
	}

	rina::ScopedLock g(port_alloc_lock);

	try {
		seq_num = rina::extendedIPCManager->allocatePortId(app_info,
								   event.flowSpecification);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems requesting an available port-id to the Kernel IPC Manager: %s",
				e.what());
		if (!event.internal) {
			replyToIPCManager(event, -1);
		} else {
			throw e;
		}
	}

	flow_state.local_request = true;
	flow_state.flow_event = event;
	flow_state.address = address;
	pending_port_allocs[seq_num] = flow_state;
}

void FlowAllocator::processAllocatePortResponse(const rina::AllocatePortResponseEvent& event)
{
	int portId = 0;
	OngoingFlowAllocState flow_state;
	IFlowAllocatorInstance * fai;
	std::map<unsigned int, OngoingFlowAllocState>::iterator it;

	port_alloc_lock.lock();
	it = pending_port_allocs.find(event.sequenceNumber);
	if (it == pending_port_allocs.end()) {
		port_alloc_lock.unlock();
		LOG_IPCP_WARN("Got an allocate port response event with seqnum %d, "
				"but found not associated Flow requests",
				event.sequenceNumber);
		return;
	}

	flow_state = it->second;
	pending_port_allocs.erase(it);
	port_alloc_lock.unlock();

	if (event.result != 0) {
		LOG_IPCP_ERR("Port-id allocation failed: %d", event.result);
		if (flow_state.local_request && !flow_state.flow_event.internal) {
			replyToIPCManager(flow_state.flow_event, -1);
		}
		return;
	}

	LOG_IPCP_DBG("Got assigned port_id %d", event.port_id);
	if (flow_state.local_request) {
		__submitAllocateRequest(flow_state.flow_event,
					event.port_id,
					flow_state.address);
	} else {
		__createFlowRequestMessageReceived(flow_state.flow,
						   flow_state.object_name,
						   flow_state.invoke_id,
						   event.port_id);
	}
}

void FlowAllocator::__submitAllocateRequest(const rina::FlowRequestEvent& event,
					    int port_id,
					    unsigned int address)
{
	FlowAllocatorInstance * fai;
	std::stringstream ss;

	rina::ScopedLock g(fai_lock);

	ss << port_id;
	fai = new FlowAllocatorInstance(ipcp,
					this,
					port_id,
					true,
					ss.str(),
					&timer);
	fa_instances[port_id] = fai;

	try {
		fai->submitAllocateRequest(event, address);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems allocating flow: %s",
				e.what());
		fa_instances.erase(port_id);
		delete fai;

		try {
			rina::extendedIPCManager->deallocatePortId(port_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems releasing port-id %d: %s",
					port_id,
					e.what());
		}

		if (!event.internal) {
			replyToIPCManager(event, -1);
		} else {
			throw e;
		}
	}
}

void FlowAllocator::processDeallocatePortResponse(const rina::DeallocatePortResponseEvent& event)
{
	if (event.result == 0) {
		LOG_IPCP_DBG("Port id %d successfully deallocated", event.port_id);
	} else {
		LOG_IPCP_ERR("Problems deallocating port-id %d: %d",
			      event.port_id, event.result);
	}
}

void FlowAllocator::processCreateConnectionResponseEvent(const rina::CreateConnectionResponseEvent& event)
{
	std::map<int, FlowAllocatorInstance *>::iterator it;

	rina::ScopedLock g(fai_lock);

	it = fa_instances.find(event.portId);
	if (it == fa_instances.end()) {
		LOG_IPCP_ERR("Received create connection response event associated to unknown port-id %d",
					     event.portId);
	} else {
		it->second->processCreateConnectionResponseEvent(event);;
	}
}

void FlowAllocator::submitAllocateResponse(const rina::AllocateFlowResponseEvent& event)
{
	std::map<int, FlowAllocatorInstance *>::iterator it;

	LOG_IPCP_DBG("Local application invoked allocate response with seq num %ud and result %d, ",
		     event.sequenceNumber,
		     event.result);

	fai_lock.lock();
	for (it = fa_instances.begin(); it != fa_instances.end(); ++it) {
		if (it->second->get_allocate_response_message_handle() == event.sequenceNumber) {
			fai_lock.unlock();
			it->second->submitAllocateResponse(event);
			return;
		}
	}
	fai_lock.unlock();

	LOG_IPCP_ERR("Could not find FAI with handle %ud", event.sequenceNumber);
}

void FlowAllocator::processCreateConnectionResultEvent(const rina::CreateConnectionResultEvent& event)
{
	std::map<int, FlowAllocatorInstance *>::iterator it;

	rina::ScopedLock g(fai_lock);

	it = fa_instances.find(event.portId);
	if (it == fa_instances.end()) {
		LOG_IPCP_ERR("Problems looking for FAI at portId %d", event.portId);
		try {
			rina::extendedIPCManager->deallocatePortId(event.getPortId());
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems requesting IPC Manager to deallocate port-id %d: %s",
					event.portId, e.what());
		}
	} else {
		it->second->processCreateConnectionResultEvent(event);
	}
}

void FlowAllocator::processUpdateConnectionResponseEvent(const rina::UpdateConnectionResponseEvent& event)
{
	std::map<int, FlowAllocatorInstance *>::iterator it;

	rina::ScopedLock g(fai_lock);

	it = fa_instances.find(event.portId);
	if (it == fa_instances.end()) {
		LOG_IPCP_ERR("Problems looking for FAI at portId %d", event.portId);
		try {
			rina::extendedIPCManager->deallocatePortId(event.portId);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems requesting IPC Manager to deallocate port-id %d: %s",
					event.portId, e.what());
		}
	} else {
		it->second->processUpdateConnectionResponseEvent(event);
	}
}

void FlowAllocator::submitDeallocate(const rina::FlowDeallocateRequestEvent& event)
{
	std::map<int, FlowAllocatorInstance *>::iterator it;

	LOG_IPCP_DBG("Requested deallocation of flow %d", event.portId);

	fai_lock.lock();
	it = fa_instances.find(event.portId);
	fai_lock.unlock();
	if (it == fa_instances.end()) {
		LOG_IPCP_ERR("Problems looking for FAI at portId %d", event.portId);
		try {
			rina::extendedIPCManager->deallocatePortId(event.portId);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems requesting IPC Manager to deallocate port-id %d: %s",
				      event.portId, e.what());
		}
	} else {
		it->second->submitDeallocate(event);
	}
}

void FlowAllocator::removeFlowAllocatorInstance(int portId)
{
	std::map<int, FlowAllocatorInstance *>::iterator it;
	FlowAllocatorInstance * fai = 0;

	LOG_IPCP_DBG("Removing FAI of port-id %d from Flow Allocator", portId);

	fai_lock.lock();
	it = fa_instances.find(portId);
	if (it != fa_instances.end()) {
		fai = it->second;
		fa_instances.erase(it);
	}
	fai_lock.unlock();

	if (fai) {
		delete fai;
	}

	DeallocatePortTimerTask * timerTask = new DeallocatePortTimerTask(portId);
	timer.scheduleTask(timerTask, DEALLOCATE_PORT_DELAY);
}

void FlowAllocator::sync_with_kernel()
{
	std::map<int, FlowAllocatorInstance *>::iterator it;

	rina::ScopedLock g(fai_lock);

	for (it = fa_instances.begin(); it != fa_instances.end(); ++it) {
		it->second->sync_with_kernel();
	}
}

//Class Flow Allocator Instance
FlowAllocatorInstance::FlowAllocatorInstance(IPCProcess * ipc_process,
					     IFlowAllocator * flow_allocator,
					     int port_id,
					     bool loc,
					     const std::string& instance_id,
					     rina::Timer * timer_)
	: IFlowAllocatorInstance(instance_id)
{
	initialize(ipc_process, flow_allocator, port_id, loc, timer_);
	LOG_IPCP_DBG("Created flow allocator instance to manage the flow identified by portId %d ",
		     port_id);
}

FlowAllocatorInstance::~FlowAllocatorInstance()
{
	if (flow_) {
		delete flow_;
	}
}

void FlowAllocatorInstance::initialize(IPCProcess * ipc_process,
				       IFlowAllocator * flow_allocator,
				       int port_id, bool loc,
				       rina::Timer * timer_)
{
	timer = timer_;
	flow_allocator_ = flow_allocator;
	ipc_process_ = ipc_process;
	port_id_ = port_id;
	rib_daemon_ = ipc_process->rib_daemon_;
	namespace_manager_ = ipc_process->namespace_manager_;
	security_manager_ = ipc_process->security_manager_;
	state = NO_STATE;
	allocate_response_message_handle_ = 0;
	flow_ = 0;
	local = loc;
}

void FlowAllocatorInstance::set_application_entity(rina::ApplicationEntity * app_entity)
{
	ae = app_entity;
}

int FlowAllocatorInstance::get_port_id() const
{
	return port_id_;
}

configs::Flow * FlowAllocatorInstance::get_flow() const
{
	return flow_;
}

bool FlowAllocatorInstance::isFinished() const
{
	return state == FINISHED;
}

unsigned int FlowAllocatorInstance::get_allocate_response_message_handle() const
{
	unsigned int t;

	{
		rina::ScopedLock g(rina::Lockable lock);
		t = allocate_response_message_handle_;
	}

	return t;
}

void FlowAllocatorInstance::set_allocate_response_message_handle(
		unsigned int allocate_response_message_handle)
{
	rina::ScopedLock g(lock_);
	allocate_response_message_handle_ =
		allocate_response_message_handle;
}

void FlowAllocatorInstance::submitAllocateRequest(const rina::FlowRequestEvent& event,
						  unsigned int address)
{
	IFlowAllocatorPs * faps;

	faps = dynamic_cast<IFlowAllocatorPs *>(flow_allocator_->ps);
	if (!faps) {
		std::stringstream ss;
		ss << "Flow allocator policy is NULL ";
		throw rina::Exception(ss.str().c_str());
	}

	flow_request_event_ = event;
	flow_request_event_.portId = port_id_;
	flow_ = faps->newFlowRequest(ipc_process_, flow_request_event_);
	flow_->internal = event.internal;

	//1 Check directory to see to what IPC process the CDAP M_CREATE request has to be delivered
	if (address == 0) {
		flow_->remote_address = namespace_manager_->getDFTNextHop(flow_->remote_naming_info);
	} else {
		flow_->remote_address = address;
	}

	LOG_IPCP_DBG("The directory forwarding table returned address %u",
			flow_->remote_address);

	if (flow_->remote_address == 0) {
		std::stringstream ss;
		ss << "Could not find entry in DFT for application ";
		ss << event.remoteApplicationName.toString();
		throw rina::Exception(ss.str().c_str());
	}

	flow_->getActiveConnection()->destAddress = flow_->remote_address;

	flow_->local_address= ipc_process_->get_active_address();
	flow_->local_port_id = port_id_;
	std::stringstream ss;
	ss << FlowRIBObject::object_name_prefix
	   << flow_->getKey();
	object_name_ = ss.str();

	//3 Request the creation of the connection(s) in the Kernel
	lock_.lock();
	state = CONNECTION_CREATE_REQUESTED;
	lock_.unlock();
	rina::kernelIPCProcess->createConnection(*(flow_->getActiveConnection()));
	LOG_IPCP_DBG("Requested the creation of a connection to the kernel, for flow with port-id %d",
		     port_id_);
}

void FlowAllocatorInstance::replyToIPCManager(int result)
{
	rina::InternalEvent * event = 0;
	int fd = 0;
	std::string dif_name;

	if (flow_->internal) {
		dif_name = ipc_process_->get_dif_information().dif_name_.processName;

		if (result == 0)  {
			try {
				fd = rina::extendedIPCManager->internal_flow_allocated(flow_->to_flow_information(dif_name, port_id_));
			} catch (rina::Exception &e) {
				LOG_ERR("Problems opening file descriptor associated to flow: %s", e.what());
			}

			event  = new rina::IPCPInternalFlowAllocatedEvent(port_id_,
									  fd,
									  flow_->to_flow_information(dif_name, port_id_));
		} else {
			event = new rina::IPCPInternalFlowAllocationFailedEvent(result,
									        flow_->to_flow_information(dif_name, port_id_),
										"");
		}

		ipc_process_->internal_event_manager_->deliverEvent(event);
		LOG_IPCP_DBG("After deliver event");
		return;
	}

	try {
		rina::extendedIPCManager->allocateFlowRequestResult(flow_request_event_,
								    result);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager Daemon: %s",
				e.what());
	}
}

void DeallocatePortTimerTask::run()
{
	try {
		rina::extendedIPCManager->deallocatePortId(port_id);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems releasing port-id %d", port_id);
	}
}

void FlowAllocatorInstance::release_remove()
{
	flow_allocator_->removeFlowAllocatorInstance(port_id_);
}

void FlowAllocatorInstance::processCreateConnectionResponseEvent(const rina::CreateConnectionResponseEvent& event)
{
	rina::cdap_rib::con_handle_t con_handle;
	int rv;

	lock_.lock();

	if (state != CONNECTION_CREATE_REQUESTED) {
		LOG_IPCP_ERR("Received a process Create Connection Response Event while in %d state. Ignoring it",
			     state);
		lock_.unlock();
		return;
	}

	if (event.getCepId() < 0) {
		LOG_IPCP_ERR("The EFCP component of the IPC Process could not create a connection instance: %d",
			     event.getCepId());
		lock_.unlock();
		replyToIPCManager(-1);
		return;
	}

	LOG_IPCP_DBG("Created connection with cep-id %d", event.getCepId());
	flow_->getActiveConnection()->setSourceCepId(event.getCepId());
	state = MESSAGE_TO_PEER_FAI_SENT;
	lock_.unlock();

	if (flow_->remote_address != flow_->local_address) {
		rv = ipc_process_->enrollment_task_->get_con_handle_to_ipcp(flow_->remote_address,
									    con_handle);
		if (rv != 0) {
			LOG_IPCP_ERR("Could not find con_handle to next hop for destination address %u",
				     flow_->remote_address);
			replyToIPCManager(-1);
			return;
		}

		con_handle.address = flow_->remote_address;
		con_handle.cdap_dest = rina::cdap_rib::CDAP_DEST_ADATA;

		try {
			//5 Send to destination address
			rina::cdap_rib::flags_t flags;
			rina::cdap_rib::filt_info_t filt;
			rina::cdap_rib::obj_info_t obj;
			encoders::FlowEncoder encoder;
			obj.class_ = FlowRIBObject::class_name;
			obj.name_ = object_name_;
			encoder.encode(*flow_, obj.value_);

			rib_daemon_->getProxy()->remote_create(con_handle,
							       obj,
							       flags,
							       filt,
							       this);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending M_CREATE <Flow> CDAP message to neighbor: %s",
					e.what());
			replyToIPCManager(-1);
			release_remove();
			return;
		}

	} else {
		//Destination application is registered at this IPC Process
		//Bypass RIB Daemon and call Flow Allocator directly
		configs::Flow * dest_flow = new configs::Flow(*flow_);
		flow_allocator_->createFlowRequestMessageReceived(dest_flow, object_name_, 0);
	}
}

void FlowAllocatorInstance::createFlowRequestMessageReceived(configs::Flow * flow,
							     const std::string& object_name,
							     int invoke_id)
{
	rina::cdap_rib::con_handle_t con_handle;
	IPCPSecurityManagerPs *smps = 0;
	int rv;
	unsigned int aux;

	smps = dynamic_cast<IPCPSecurityManagerPs *>(security_manager_->ps);
	assert(smps);

	lock_.lock();

	LOG_IPCP_DBG("Create flow request received: %s",
			flow->toString().c_str());
	flow_ = flow;
	invoke_id_ = invoke_id;
	object_name_ = object_name;
	flow_->local_port_id = port_id_;
	rina::Connection * connection = flow_->getActiveConnection();
	connection->setPortId(port_id_);
	if (flow->internal) {
		connection->setFlowUserIpcProcessId(0);
	} else {
		connection->setFlowUserIpcProcessId(
				namespace_manager_->getRegIPCProcessId(flow_->local_naming_info));
	}
	LOG_IPCP_DBG("Target application IPC Process id is %d",
			connection->getFlowUserIpcProcessId());

	lock_.unlock();

	//2 Check if the source application process has access to the destination application process.
	// If not send negative M_CREATE_R back to the sender IPC process, and do housekeeping.
	if (!smps->acceptFlow(*flow_)) {
		LOG_IPCP_WARN("Security Manager denied incoming flow request from application %s",
				flow_->remote_naming_info.getEncodedString().c_str());

		if (flow_->local_address != flow_->remote_address) {
			rv = ipc_process_->enrollment_task_->get_con_handle_to_ipcp(flow_->remote_address,
										    con_handle);
			if (rv != 0) {
				LOG_IPCP_ERR("Could not find con_handle to next hop for destination address %u",
					     flow_->remote_address);
				release_remove();
				return;
			}
			con_handle.address = flow_->remote_address;
			con_handle.cdap_dest = rina::cdap_rib::CDAP_DEST_ADATA;

			try {
				rina::cdap_rib::flags_t flags;
				rina::cdap_rib::filt_info_t filt;
				rina::cdap_rib::obj_info_t obj;
				encoders::FlowEncoder encoder;
				obj.class_ = FlowRIBObject::class_name;
				obj.name_ = object_name_;
				encoder.encode(*flow_, obj.value_);
				rina::cdap_rib::res_info_t res;
				res.code_ = rina::cdap_rib::CDAP_ERROR;

				rina::cdap::getProvider()->send_create_result(con_handle,
									      obj,
									      flags,
									      res,
									      invoke_id_);
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Problems sending CDAP message: %s",
					     e.what());
			}
		} else {
			//TODO notify source FAI
		}

		release_remove();
		return;
	}

	//3 TODO If it has, determine if the proposed policies for the flow are acceptable (invoke NewFlowREquestPolicy)
	//Not done in this version, it is assumed that the proposed policies for the flow are acceptable.

	//4 Request creation of connection
	try {
		lock_.lock();
		state = CONNECTION_CREATE_REQUESTED;
		lock_.unlock();
		rina::kernelIPCProcess->createConnectionArrived(*connection);
		LOG_IPCP_DBG(
				"Requested the creation of a connection to the kernel to support flow with port-id %d",
				port_id_);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems requesting a connection to the kernel: %s ",
				e.what());
		release_remove();
		return;
	}
}

void FlowAllocatorInstance::processCreateConnectionResultEvent(const rina::CreateConnectionResultEvent& event)
{
	rina::InternalEvent * int_event = 0;
	int fd = 0;
	std::string dif_name;

	lock_.lock();

	if (state != CONNECTION_CREATE_REQUESTED) {
		LOG_IPCP_ERR("Received an allocate response event while not in APP_NOTIFIED_OF_INCOMING_FLOW state. Current state: %d",
			     state);
		lock_.unlock();
		return;
	}

	if (event.sourceCepId < 0) {
		LOG_IPCP_ERR("Create connection operation was unsuccessful: %d",
				event.sourceCepId);
		lock_.unlock();
		release_remove();
		return;
	}


	flow_->getActiveConnection()->sourceCepId = event.sourceCepId;
	state = APP_NOTIFIED_OF_INCOMING_FLOW;
	lock_.unlock();

	if (flow_->internal) {
		complete_flow_allocation(true);
		dif_name = ipc_process_->get_dif_information().dif_name_.processName;

		try {
			fd = rina::extendedIPCManager->internal_flow_allocated(flow_->to_flow_information(dif_name, port_id_));
		} catch (rina::Exception &e) {
			LOG_ERR("Problems opening file descriptor associated to flow: %s", e.what());
		}

		int_event = new rina::IPCPInternalFlowAllocatedEvent(port_id_,
								     fd,
								     flow_->to_flow_information(dif_name, port_id_));
		ipc_process_->internal_event_manager_->deliverEvent(int_event);

		return;
	}

	try {
		allocate_response_message_handle_ =
				rina::extendedIPCManager->allocateFlowRequestArrived(flow_->local_naming_info,
						flow_->remote_naming_info,
						flow_->flow_specification,
						port_id_);
		LOG_IPCP_DBG("Informed IPC Manager about incoming flow allocation request, got handle: %ud",
				allocate_response_message_handle_);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems informing the IPC Manager about an incoming flow allocation request: %s",
				e.what());
		release_remove();
		return;
	}
}

void FlowAllocatorInstance::submitAllocateResponse(const rina::AllocateFlowResponseEvent& event)
{
	lock_.lock();

	if (state != APP_NOTIFIED_OF_INCOMING_FLOW) {
		LOG_IPCP_ERR("Received an allocate response event while not in APP_NOTIFIED_OF_INCOMING_FLOW state. Current state: %d",
			     state);
		lock_.unlock();
		return;
	}

	lock_.unlock();
	complete_flow_allocation(event.result == 0);
}

void FlowAllocatorInstance::complete_flow_allocation(bool success)
{
	rina::cdap_rib::con_handle_t con_handle;
	int rv;
	unsigned int rz;
	std::stringstream ss;
	rina::rib::RIBObj * obj;
	std::string aux;

	if (flow_->local_address != flow_->remote_address) {
		//Obtain con_handle to next hop, to be able to send A-data
		con_handle.address = flow_->remote_address;
		con_handle.cdap_dest = rina::cdap_rib::CDAP_DEST_ADATA;
		rv = ipc_process_->enrollment_task_->get_con_handle_to_ipcp(flow_->remote_address,
									    con_handle);
		if (rv != 0 && flow_->internal) {
			//Retry, remote peer address may have changed, we still have its name
			rz = ipc_process_->enrollment_task_->get_con_handle_to_ipcp(flow_->remote_naming_info.processName,
					    	    	    	    	    	    con_handle);
			if (rz != 0) {
				flow_->remote_address = rz;
				rv = 0;
			}
		}

		if (rv != 0) {
			LOG_IPCP_ERR("Could not find con_handle to next hop for destination address %u",
				     flow_->remote_address);

			if (success) {
				try {
					rina::extendedIPCManager->flowDeallocated(port_id_);
				} catch (rina::Exception &e) {
					LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s",
							e.what());
				}
			}

			release_remove();
			return;
		}
	}

	if (success) {
		//Flow has been accepted
		if (flow_->local_address != flow_->remote_address) {
			try {
				rina::cdap_rib::flags_t flags;
				rina::cdap_rib::filt_info_t filt;
				rina::cdap_rib::obj_info_t obj;
				encoders::FlowEncoder encoder;
				obj.class_ = FlowRIBObject::class_name;
				obj.name_ = object_name_;
				encoder.encode(*flow_, obj.value_);
				rina::cdap_rib::res_info_t res;
				res.code_ = rina::cdap_rib::CDAP_SUCCESS;

				rina::cdap::getProvider()->send_create_result(con_handle,
									      obj,
									      flags,
									      res,
									      invoke_id_);
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Problems requesting RIB Daemon to send CDAP Message: %s",
					     e.what());

				try {
					rina::extendedIPCManager->flowDeallocated(port_id_);
				} catch (rina::Exception &e) {
					LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s",
							e.what());
				}

				release_remove();
				return;
			}
		} else {
			//Both source and destination apps are registered at the same DIF
			//Locate source FAI
			IFlowAllocatorInstance * fai = flow_allocator_->getFAI(flow_->remote_port_id);

			if (fai == 0) {
				try {
					rina::extendedIPCManager->flowDeallocated(port_id_);
				} catch (rina::Exception &e) {
					LOG_IPCP_ERR("Problems locating FAI associated to port-id: %d",
							flow_->remote_port_id);
				}

				release_remove();
				return;
			}

			configs::Flow* source_flow = new configs::Flow(*flow_);
			rina::cdap_rib::con_handle_t con;
			rina::cdap_rib::obj_info_t obj;
			encoders::FlowEncoder encoder;
			obj.class_ = FlowRIBObject::class_name;
			ss << flow_->remote_address << "-" << flow_->remote_port_id;
			obj.name_ = ss.str();
			encoder.encode(*source_flow, obj.value_);
			rina::cdap_rib::result_info res;
			res.code_ = rina::cdap_rib::CDAP_SUCCESS;

			fai->remoteCreateResult(con, obj, res);
		}

		try {
			flow_->state = configs::Flow::ALLOCATED;
			obj = new FlowRIBObject(ipc_process_, this);
			rib_daemon_->addObjRIB(object_name_, &obj);
		} catch (rina::Exception &e) {
			LOG_IPCP_WARN("Error adding Flow RIB object with name %s: %s",
				      object_name_.c_str(), e.what());
		}

		try {
			ss.str(std::string());
			ss << ConnectionRIBObject::object_name_prefix
			   << flow_->getActiveConnection()->sourceCepId;
			aux = ss.str();
			obj = new ConnectionRIBObject(ipc_process_, this);
			rib_daemon_->addObjRIB(aux, &obj);
		} catch (rina::Exception &e) {
			LOG_IPCP_WARN("Error adding Connection RIB object with name %s: %s",
				       aux.c_str(), e.what());
		}

		try {
			ss << DTCPRIBObject::object_name_suffix;
			obj = new DTCPRIBObject(ipc_process_, this);
			aux = ss.str();
			rib_daemon_->addObjRIB(aux, &obj);
		} catch (rina::Exception &e) {
			LOG_IPCP_WARN("Error adding DTCP Rib object with name %s: %s",
				      aux.c_str(), e.what());
		}

		lock_.lock();
		state = FLOW_ALLOCATED;
		lock_.unlock();
		return;
	}

	//Flow has been rejected
	if (flow_->local_address != flow_->remote_address) {
		try {
			rina::cdap_rib::flags_t flags;
			rina::cdap_rib::filt_info_t filt;
			rina::cdap_rib::obj_info_t obj;
			encoders::FlowEncoder encoder;
			obj.class_ = FlowRIBObject::class_name;
			obj.name_ = object_name_;
			encoder.encode(*flow_, obj.value_);
			rina::cdap_rib::res_info_t res;
			res.code_ = rina::cdap_rib::CDAP_ERROR;
			res.reason_ = "Application has rejected the flow";

			rina::cdap::getProvider()->send_create_result(con_handle,
								      obj,
								      flags,
								      res,
								      invoke_id_);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems requesting RIB Daemon to send CDAP Message: %s",
					e.what());
		}
	} else {
		//Both source and destination apps are registered at the same DIF
		//Locate source FAI
		IFlowAllocatorInstance * fai = flow_allocator_->getFAI(flow_->remote_port_id);

		if (fai == 0) {
			try {
				rina::extendedIPCManager->flowDeallocated(port_id_);
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Problems locating FAI associated to port-id: %d",
						flow_->remote_port_id);
			}

			release_remove();
			return;
		}

		rina::cdap_rib::con_handle_t con;
		rina::cdap_rib::obj_info_t obj;
		obj.class_ = FlowRIBObject::class_name;
		ss << flow_->remote_address << "-" << flow_->remote_port_id;
		obj.name_ = ss.str();
		rina::cdap_rib::result_info res;
		res.code_ = rina::cdap_rib::CDAP_ERROR;
		res.reason_ = "Application rejected the flow";

		fai->remoteCreateResult(con, obj, res);
	}

	release_remove();
}

void FlowAllocatorInstance::processUpdateConnectionResponseEvent(
		const rina::UpdateConnectionResponseEvent& event)
{
	std::stringstream ss;
	std::string aux;
	rina::rib::RIBObj * obj;

	lock_.lock();

	if (state != CONNECTION_UPDATE_REQUESTED) {
		LOG_IPCP_ERR(
				"Received CDAP Message while not in CONNECTION_UPDATE_REQUESTED state. Current state is: %d",
				state);
		lock_.unlock();
		return;
	}

	//Update connection was unsuccessful
	if (event.getResult() != 0) {
		LOG_IPCP_ERR("The kernel denied the update of a connection: %d",
				event.getResult());
		flow_request_event_.portId = -1;
		lock_.unlock();
		replyToIPCManager(event.getResult());
		release_remove();
		return;
	}

	//Update connection was successful
	flow_->state = configs::Flow::ALLOCATED;
	state = FLOW_ALLOCATED;
	flow_request_event_.portId = port_id_;
	lock_.unlock();

	try {
		obj = new FlowRIBObject(ipc_process_, this);
		rib_daemon_->addObjRIB(object_name_, &obj);
	} catch (rina::Exception &e) {
		LOG_IPCP_WARN("Problems requesting the RIB Daemon to create a Flow object with name %s: %s",
				object_name_.c_str(), e.what());
	}

	try {
		ss << ConnectionRIBObject::object_name_prefix
		   << flow_->getActiveConnection()->sourceCepId;
		aux = ss.str();
		obj = new ConnectionRIBObject(ipc_process_, this);
		rib_daemon_->addObjRIB(aux, &obj);
	} catch (rina::Exception &e) {
		LOG_IPCP_WARN("Problems requesting the RIB Daemon to create a Connection object with name %s: %s",
				aux.c_str(), e.what());
	}

	try {
		ss << DTCPRIBObject::object_name_suffix;
		aux = ss.str();
		obj = new DTCPRIBObject(ipc_process_, this);
		rib_daemon_->addObjRIB(aux, &obj);
	} catch (rina::Exception &e) {
		LOG_IPCP_WARN("Problems requesting the RIB Daemon to create a DTCP object with name %s: %s",
				aux.c_str(), e.what());
	}

	replyToIPCManager(0);
}

void FlowAllocatorInstance::submitDeallocate(const rina::FlowDeallocateRequestEvent& event)
{
	rina::cdap_rib::con_handle_t con_handle;
	IFlowAllocatorInstance * fai = 0;
	int rv;
	unsigned int rz;

	LOG_IPCP_DBG("Requested deallocation of flow %d", event.portId);
	lock_.lock();

	if (state != FLOW_ALLOCATED) {
		lock_.unlock();
		LOG_IPCP_ERR("Received deallocate request while not in FLOW_ALLOCATED state. "
			     "Current state is: %d",
			     state);
		return;
	}

	//1 Update flow state
	flow_->state = configs::Flow::WAITING_2_MPL_BEFORE_TEARING_DOWN;
	state = WAITING_2_MPL_BEFORE_TEARING_DOWN;
	lock_.unlock();

	if (flow_->internal) {
		//Wait 2*MPL before tearing down the flow
		TearDownFlowTimerTask * timerTask = new TearDownFlowTimerTask(this,
									      object_name_,
									      true);
		timer->scheduleTask(timerTask, FlowAllocator::TEARDOWN_FLOW_DELAY);
		return;
	}

	//2 Send M_DELETE
	if (flow_->local_address != flow_->remote_address) {
		try {
			rv = ipc_process_->enrollment_task_->get_con_handle_to_ipcp(flow_->remote_address,
										    con_handle);

			if (rv != 0 && flow_->internal) {
				//Retry, remote peer address may have changed, we still have its name
				rz = ipc_process_->enrollment_task_->get_con_handle_to_ipcp(flow_->remote_naming_info.processName,
						    	    	    	    	    	    con_handle);
				if (rz != 0) {
					flow_->remote_address = rz;
					rv = 0;
				}
			}

			if (rv == 0) {
				con_handle.address = flow_->remote_address;
				con_handle.cdap_dest = rina::cdap_rib::CDAP_DEST_ADATA;

				rina::cdap_rib::flags_t flags;
				rina::cdap_rib::filt_info_t filt;
				rina::cdap_rib::obj_info_t obj;
				obj.class_ = FlowRIBObject::class_name;
				obj.name_ = object_name_;
				rib_daemon_->getProxy()->remote_delete(con_handle,
						obj,
						flags,
						filt,
						NULL);
			} else {
				LOG_IPCP_ERR("Could not find CDAP session for reaching next hop to %d (deallocate)",
						flow_->remote_address);
			}
		} catch (rina::Exception &e) {
			if (!event.internal) {
				LOG_IPCP_ERR("Problems sending M_DELETE flow request: %s",
					     e.what());
			}
		}
	} else {
		fai = flow_allocator_->getFAI(flow_->remote_port_id);
		if (!fai){
			LOG_IPCP_ERR("Problems locating destination Flow Allocator instance");
		} else {
			fai->deleteFlowRequestMessageReceived();
		}
	}

	// 3 Tear down the flow (don't need to wait for 2*MPL at the side that
	// requested the flow deallocation). The spec says that the binding between
	// app and EFCP needs to be removed right away (that is, the port), and EFCP
	// state will be removed automatically after 2 or 3 delta-t.
	TearDownFlowTimerTask * timerTask = new TearDownFlowTimerTask(this,
								      object_name_,
								      true);
	timer->scheduleTask(timerTask, FlowAllocator::TEARDOWN_FLOW_DELAY);
}

void FlowAllocatorInstance::deleteFlowRequestMessageReceived()
{
	rina::InternalEvent * event = 0;

	LOG_IPCP_DBG("Received delete flow request message from peer %d", port_id_);

	lock_.lock();
	if (state != FLOW_ALLOCATED) {
		lock_.unlock();
		LOG_IPCP_ERR("Received deallocate request while not in FLOW_ALLOCATED state. Current state is: %d",
			     state);
		return;
	}

	//1 Update flow state
	flow_->state = configs::Flow::WAITING_2_MPL_BEFORE_TEARING_DOWN;
	state = WAITING_2_MPL_BEFORE_TEARING_DOWN;
	lock_.unlock();

	//Don't wait for 2*MPL anymore before tearing down the flow.
	//The spec says that the binding between app and EFCP needs to
	//be removed right away (that is, the port), and EFCP state will
	//be removed automatically after 2 or 3 delta-t.
	TearDownFlowTimerTask * timerTask = new TearDownFlowTimerTask(this,
								      object_name_,
								      true);

	timer->scheduleTask(timerTask, FlowAllocator::TEARDOWN_FLOW_DELAY);

	if (flow_->internal) {
		event = new rina::IPCPInternalFlowDeallocatedEvent(flow_->local_port_id);
		ipc_process_->internal_event_manager_->deliverEvent(event);
	} else {
		//4 Inform IPC Manager
		try {
			rina::extendedIPCManager->flowDeallocatedRemotely(port_id_,
									  0);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Error communicating with the IPC Manager: %s", e.what());
		}
	}
}

void FlowAllocatorInstance::destroyFlowAllocatorInstance(
		const std::string& flowObjectName, bool requestor)
{
	std::stringstream ss;
	std::string con_name;
	std::string dtcp_name;

	lock_.lock();

	if (state != WAITING_2_MPL_BEFORE_TEARING_DOWN) {
		LOG_IPCP_ERR(
				"Invoked destroy flow allocator instance while not in WAITING_2_MPL_BEFORE_TEARING_DOWN. State: %d",
				state);
		lock_.unlock();
		return;
	}
	lock_.unlock();

	try {
		rib_daemon_->removeObjRIB(object_name_);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems deleting object from RIB with name %s: %s",
				object_name_.c_str(), e.what());
	}

	try {
		ss << ConnectionRIBObject::object_name_prefix
		   << flow_->getActiveConnection()->sourceCepId;
		con_name = ss.str();
		ss << DTCPRIBObject::object_name_suffix;
		dtcp_name = ss.str();
		rib_daemon_->removeObjRIB(dtcp_name);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems deleting object from RIB with name %s: %s",
			     con_name.c_str(), e.what());
	}

	try {
		rib_daemon_->removeObjRIB(con_name);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems deleting object from RIB with name %s: %s",
			     con_name.c_str(), e.what());
	}

	if (flow_->internal) {
		try {
			rina::extendedIPCManager->internal_flow_deallocated(port_id_);
		} catch (rina::Exception &e) {
			LOG_ERR("Problems clearing internal flow state: %s", e.what());
		}
	}

	release_remove();
}

void FlowAllocatorInstance::modify_flow_request(const configs::Flow & flow)
{
	lock_.lock();
	LOG_IPCP_INFO("Modified flow remote address from %u to %u",
			flow_->remote_address, flow.local_address);
	flow_->remote_address = flow.local_address;
	flow_->getActiveConnection()->setDestAddress(flow.local_address);
	lock_.unlock();

	try {
		rina::kernelIPCProcess->modify_connection(*(flow_->getActiveConnection()));
	} catch (rina::IPCException & e) {
		LOG_ERR("Problems calling modify connection: %s", e.what());
	}
}

void FlowAllocatorInstance::remoteCreateResult(const rina::cdap_rib::con_handle_t &con,
					       const rina::cdap_rib::obj_info_t &obj,
					       const rina::cdap_rib::res_info_t &res)
{
	lock_.lock();

	if (state != MESSAGE_TO_PEER_FAI_SENT) {
		LOG_IPCP_ERR(
				"Received CDAP Message while not in MESSAGE_TO_PEER_FAI_SENT state. Current state is: %d",
				state);
		lock_.unlock();
		return;
	}

	//Flow allocation unsuccessful
	if (res.code_!= rina::cdap_rib::CDAP_SUCCESS) {
		LOG_IPCP_DBG("Unsuccessful create flow response message received for flow %s",
				object_name_.c_str());

		//Answer IPC Manager
		flow_request_event_.portId = -1;
		lock_.unlock();
		replyToIPCManager(res.code_);
		release_remove();

		return;
	}

	//Flow allocation successful
	//Update the EFCP connection with the destination cep-id
	state = CONNECTION_UPDATE_REQUESTED;
	lock_.unlock();

	try {
		if (obj.value_.message_) {
			encoders::FlowEncoder encoder;
			configs::Flow receivedFlow;
			encoder.decode(obj.value_, receivedFlow);
			flow_->remote_port_id = receivedFlow.local_port_id;
			flow_->getActiveConnection()->setDestCepId(
					receivedFlow.getActiveConnection()->getSourceCepId());
		}

		rina::kernelIPCProcess->updateConnection(
				*(flow_->getActiveConnection()));
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems requesting kernel to update connection: %s",
				e.what());

		//Answer IPC Manager
		flow_request_event_.portId = -1;
		replyToIPCManager(res.code_);
		release_remove();
		return;
	}
}

void FlowAllocatorInstance::sync_with_kernel()
{
	rina::Connection * con = flow_->getActiveConnection();

	SysfsHelper::get_dtp_tx_bytes(ipc_process_->get_id(),
				      con->sourceCepId,
				      con->stats.tx_bytes);
	SysfsHelper::get_dtp_rx_bytes(ipc_process_->get_id(),
				      con->sourceCepId,
				      con->stats.rx_bytes);
	SysfsHelper::get_dtp_tx_pdus(ipc_process_->get_id(),
				     con->sourceCepId,
				     con->stats.tx_pdus);
	SysfsHelper::get_dtp_rx_pdus(ipc_process_->get_id(),
				     con->sourceCepId,
				     con->stats.rx_pdus);
	SysfsHelper::get_dtp_drop_pdus(ipc_process_->get_id(),
				       con->sourceCepId,
				       con->stats.drop_pdus);
	SysfsHelper::get_dtp_error_pdus(ipc_process_->get_id(),
				        con->sourceCepId,
				        con->stats.err_pdus);
}

void FlowAllocatorInstance::address_changed(unsigned int new_address,
		     	     	     	    unsigned int old_address)
{
	rina::cdap_rib::con_handle_t con_handle;
	int rv;

	lock_.lock();
	flow_->local_address = new_address;
	flow_->getActiveConnection()->setSourceAddress(new_address);
	lock_.unlock();

	//Sent message to peer
	try {
		LOG_IPCP_INFO("Informing peer %u about address change from %u to %u",
			      flow_->remote_address, old_address, new_address);
		rv = ipc_process_->enrollment_task_->get_con_handle_to_ipcp(flow_->remote_address,
									    con_handle);

		if (rv == 0) {
			con_handle.address = flow_->remote_address;
			con_handle.cdap_dest = rina::cdap_rib::CDAP_DEST_ADATA;

			rina::cdap_rib::flags_t flags;
			rina::cdap_rib::filt_info_t filt;
			rina::cdap_rib::obj_info_t obj;
			encoders::FlowEncoder encoder;
			obj.class_ = FlowRIBObject::class_name;
			obj.name_ = object_name_;
			encoder.encode(*flow_, obj.value_);

			rib_daemon_->getProxy()->remote_write(con_handle,
							      obj,
							      flags,
							      filt,
							      NULL);

		} else {
			LOG_IPCP_ERR("Could not find CDAP session for reaching next hop to %d (address change)",
					flow_->remote_address);
		}
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending M_WRITE flow request: %s",
				e.what());
	}
}

//CLASS TEARDOWNFLOW TIMERTASK
TearDownFlowTimerTask::TearDownFlowTimerTask(
		FlowAllocatorInstance * flow_allocator_instance,
		const std::string& flow_object_name, bool requestor)
{
	flow_allocator_instance_ = flow_allocator_instance;
	flow_object_name_ = flow_object_name;
	requestor_ = requestor;
}

void TearDownFlowTimerTask::run()
{
	flow_allocator_instance_->destroyFlowAllocatorInstance(
			flow_object_name_, requestor_);
}

//Class DatatransferConstantsRIBObject
const std::string DataTransferRIBObj::class_name = "DataTransfer";
const std::string DataTransferRIBObj::object_name = "/dt";

DataTransferRIBObj::DataTransferRIBObj(IPCProcess * ipc_process)
	: IPCPRIBObj(ipc_process, class_name)
{
}

void DataTransferRIBObj::read(const rina::cdap_rib::con_handle_t &con,
			      const std::string& fqn,
			      const std::string& class_,
			      const rina::cdap_rib::filt_info_t &filt,
			      const int invoke_id,
			      rina::ser_obj_t &obj_reply,
			      rina::cdap_rib::res_info_t& res)
{
	encoders::DataTransferConstantsEncoder encoder;
	encoder.encode(ipc_process_->get_dif_information().dif_configuration_.efcp_configuration_.data_transfer_constants_,
		       obj_reply);
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

void DataTransferRIBObj::create(const rina::cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const rina::cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const rina::ser_obj_t &obj_req,
				rina::ser_obj_t &obj_reply,
				rina::cdap_rib::res_info_t& res)
{
	//TODO: Ignore, for the moment
}

const std::string DataTransferRIBObj::get_displayable_value() const
{
	rina::DataTransferConstants dtc =
		ipc_process_->get_dif_information().dif_configuration_
		.efcp_configuration_.data_transfer_constants_;
	return dtc.toString();
}

} //namespace rinad
