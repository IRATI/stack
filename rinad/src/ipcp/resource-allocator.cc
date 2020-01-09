//
// Resource Allocator
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

#include <sstream>

#include <librina/internal-events.h>

#define IPCP_MODULE "resource-allocator"

#include "ipcp-logging.h"
#include "resource-allocator.h"
#include "utils.h"

namespace rinad {

// Class QoSCube RIB object
const std::string QoSCubeRIBObject::class_name = "QoSCube";
const std::string QoSCubeRIBObject::object_name_prefix = "/ra/qoscubes/id=";

QoSCubeRIBObject::QoSCubeRIBObject(rina::QoSCube* cube)
	: rina::rib::RIBObj(class_name), qos_cube(cube)
{
}

const std::string QoSCubeRIBObject::get_displayable_value() const
{
	std::stringstream ss;
	ss << "Name: " << qos_cube->name_ << "; Id: " << qos_cube->id_;
	ss << "; Jitter: " << qos_cube->jitter_ << "; Delay: " << qos_cube->delay_
		<< std::endl;
	ss << "In oder delivery: " << qos_cube->ordered_delivery_;
	ss << "; Partial delivery allowed: " << qos_cube->partial_delivery_
		<< std::endl;
	ss << "Max allowed gap between SDUs: " << qos_cube->max_allowable_gap_;
	ss << "; Undetected bit error rate: "
		<< qos_cube->undetected_bit_error_rate_ << std::endl;
	ss << "Average bandwidth (bytes/s): " << qos_cube->average_bandwidth_;
	ss << "; Average SDU bandwidth (bytes/s): "
		<< qos_cube->average_sdu_bandwidth_ << std::endl;
	ss << "Peak bandwidth duration (ms): "
		<< qos_cube->peak_bandwidth_duration_;
	ss << "; Peak SDU bandwidth duration (ms): "
		<< qos_cube->peak_sdu_bandwidth_duration_ << std::endl;
	rina::DTPConfig dtp_conf = qos_cube->dtp_config_;
	ss << "DTP Configuration: " << dtp_conf.toString();
	rina::DTCPConfig dtcp_conf = qos_cube->dtcp_config_;
	ss << "DTCP Configuration: " << dtcp_conf.toString();
	return ss.str();
}

void QoSCubeRIBObject::read(const rina::cdap_rib::con_handle_t &con,
			    const std::string& fqn,
			    const std::string& class_,
			    const rina::cdap_rib::filt_info_t &filt,
			    const int invoke_id,
			    rina::cdap_rib::obj_info_t &obj_reply,
			    rina::cdap_rib::res_info_t& res)
{
	if (qos_cube) {
		encoders::QoSCubeEncoder encoder;
		encoder.encode(*qos_cube, obj_reply.value_);
	}

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

//Class QoS Cube Set RIB Object
const std::string QoSCubesRIBObject::class_name = "QoSCubes";
const std::string QoSCubesRIBObject::object_name = "/ra/qoscubes";

QoSCubesRIBObject::QoSCubesRIBObject(IPCProcess * ipc_process)
	: IPCPRIBObj(ipc_process, class_name)
{
}

void QoSCubesRIBObject::create(const rina::cdap_rib::con_handle_t &con,
			       const std::string& fqn,
			       const std::string& class_,
			       const rina::cdap_rib::filt_info_t &filt,
			       const int invoke_id,
			       const rina::ser_obj_t &obj_req,
			       rina::ser_obj_t &obj_reply,
			       rina::cdap_rib::res_info_t& res)
{
	//TODO
	LOG_IPCP_ERR("Missing code");
}

//Class RMTN1Flow
void RMTN1Flow::sync_with_kernel()
{
	SysfsHelper::get_rmt_queued_pdus(ipcp_id, port_id, queued_pdus);
	SysfsHelper::get_rmt_drop_pdus(ipcp_id, port_id, dropped_pdus);
	SysfsHelper::get_rmt_error_pdus(ipcp_id, port_id, error_pdus);
	SysfsHelper::get_rmt_rx_pdus(ipcp_id, port_id, rx_pdus);
	SysfsHelper::get_rmt_tx_pdus(ipcp_id, port_id, tx_pdus);
	SysfsHelper::get_rmt_rx_bytes(ipcp_id, port_id, rx_bytes);
	SysfsHelper::get_rmt_tx_bytes(ipcp_id, port_id, tx_bytes);
}

// Class RMTN1Flow RIB object
const std::string RMTN1FlowRIBObj::class_name = "RMTN1Flow";
const std::string RMTN1FlowRIBObj::object_name_prefix = "/rmt/n1flows/pid=";

RMTN1FlowRIBObj::RMTN1FlowRIBObj(RMTN1Flow * flow)
	: rina::rib::RIBObj(class_name)
{
	n1_flow = flow;
}

const std::string RMTN1FlowRIBObj::get_displayable_value() const
{
	std::stringstream ss;
	ss << "Port-id: " << n1_flow->port_id << "; Enabled: " << n1_flow->enabled
	   << "; Queued pdus: " << n1_flow->queued_pdus << "; Dropped pdus: " << n1_flow->dropped_pdus
	   << "; Error pdus: " << n1_flow->dropped_pdus << std::endl;
	ss << "Tx: " << n1_flow->tx_bytes << " (bytes),  " << n1_flow->tx_pdus << " (pdus); "
	   << "Rx: " << n1_flow->rx_bytes << " (bytes), " << n1_flow->rx_pdus << " (pdus)";
	return ss.str();
}

void RMTN1FlowRIBObj::read(const rina::cdap_rib::con_handle_t &con,
			   const std::string& fqn,
			   const std::string& class_,
			   const rina::cdap_rib::filt_info_t &filt,
			   const int invoke_id,
			   rina::cdap_rib::obj_info_t &obj_reply,
			   rina::cdap_rib::res_info_t& res)
{
	//TODO
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

// Class NextHopTEntryRIBObj
const std::string NextHopTEntryRIBObj::parent_class_name = "NextHopTable";
const std::string NextHopTEntryRIBObj::parent_object_name = "/ra/nhopt";
const std::string NextHopTEntryRIBObj::class_name = "NextHopTableEntry";
const std::string NextHopTEntryRIBObj::object_name_prefix = "/ra/nhopt/key=";

NextHopTEntryRIBObj::NextHopTEntryRIBObj(rina::RoutingTableEntry* entry)
	: rina::rib::RIBObj(class_name), rt_entry(entry)
{
}

const std::string NextHopTEntryRIBObj::get_displayable_value() const
{
	std::stringstream ss;
	ss << "Destination name: " << rt_entry->destination.name
	   << "; Addresses: " << rt_entry->destination.get_addresses_as_string()
	   << "; QoS-id: " << rt_entry->qosId
	   << "; Cost: " << rt_entry->cost
	   << "; Next hop addresses: ";
	std::list<rina::NHopAltList>::iterator it;
	for (it = rt_entry->nextHopNames.begin(); it !=
			rt_entry->nextHopNames.end(); ++it) {
		ss << it->alts.front().name << " "
		   << it->alts.front().get_addresses_as_string() << "/ ";
	}
	return ss.str();
}

void NextHopTEntryRIBObj::read(const rina::cdap_rib::con_handle_t &con,
			       const std::string& fqn,
			       const std::string& class_,
			       const rina::cdap_rib::filt_info_t &filt,
			       const int invoke_id,
			       rina::cdap_rib::obj_info_t &obj_reply,
			       rina::cdap_rib::res_info_t& res)
{
	if (rt_entry) {
		encoders::RoutingTableEntryEncoder encoder;
		encoder.encode(*rt_entry, obj_reply.value_);
	}

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

// Class PDUFTEntryRIBObj
const std::string PDUFTEntryRIBObj::parent_class_name = "PDUForwardingTable";
const std::string PDUFTEntryRIBObj::parent_object_name = "/ra/pduft";
const std::string PDUFTEntryRIBObj::class_name = "PDUForwardingTableEntry";
const std::string PDUFTEntryRIBObj::object_name_prefix = "/ra/pduft/key=";

PDUFTEntryRIBObj::PDUFTEntryRIBObj(rina::PDUForwardingTableEntry* entry)
	: rina::rib::RIBObj(class_name), ft_entry(entry)
{
}

const std::string PDUFTEntryRIBObj::get_displayable_value() const
{
	std::stringstream ss;
	ss << "Destination address: " << ft_entry->address
	   << "; QoS-id: " << ft_entry->qosId
	   << "; Port-ids to be forwarded: ";
	std::list<rina::PortIdAltlist>::iterator it;
	for (it = ft_entry->portIdAltlists.begin(); it !=
			ft_entry->portIdAltlists.end(); ++it) {
		ss << it->alts.front() << "/ ";
	}
	return ss.str();
}

void PDUFTEntryRIBObj::read(const rina::cdap_rib::con_handle_t &con,
			    const std::string& fqn,
			    const std::string& class_,
			    const rina::cdap_rib::filt_info_t &filt,
			    const int invoke_id,
			    rina::cdap_rib::obj_info_t &obj_reply,
			    rina::cdap_rib::res_info_t& res)
{
	if (ft_entry) {
		encoders::PDUForwardingTableEntryEncoder encoder;
		encoder.encode(*ft_entry, obj_reply.value_);
	}

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

//Class NMinusOneFlowManager
NMinusOneFlowManager::NMinusOneFlowManager()
{
	rib_daemon_ = 0;
	ipc_process_ = 0;
	flow_acceptor_ = 0;
}

NMinusOneFlowManager::~NMinusOneFlowManager()
{
	if (flow_acceptor_) {
		delete flow_acceptor_;
	}
}

void NMinusOneFlowManager::set_ipc_process(IPCProcess * ipc_process)
{
	app = ipc_process;
	ipc_process_ = ipc_process;
	rib_daemon_ = ipc_process->rib_daemon_->getProxy();
	event_manager_ = ipc_process->internal_event_manager_;
	flow_acceptor_ = new IPCPFlowAcceptor(ipc_process_);
	set_flow_acceptor(flow_acceptor_);
	populateRIB();
}

void NMinusOneFlowManager::set_dif_configuration(const rina::DIFInformation& dif_information) {
	LOG_IPCP_DBG("DIF configuration set %u", dif_information.dif_configuration_.address_);
}

void NMinusOneFlowManager::processRegistrationNotification(const rina::IPCProcessDIFRegistrationEvent& event) {
	if (event.isRegistered()) {
		try {
			rina::extendedIPCManager->appRegistered(event.getIPCProcessName(), event.getDIFName());
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		LOG_IPCP_INFO("IPC Process registered to N-1 DIF %s",
				event.getDIFName().processName.c_str());
		try{
			std::stringstream ss;
			ss << rina::UnderlayingRegistrationRIBObj::object_name_prefix;
			ss << event.getDIFName().processName;
			rina::rib::RIBObj * obj =
					new rina::UnderlayingRegistrationRIBObj(event.getDIFName().processName);

			ipc_process_->rib_daemon_->addObjRIB(ss.str(), &obj);
		}catch(rina::Exception &e){
			LOG_IPCP_ERR("Problems adding object to RIB: %s",
				     e.what());
		}

		return;
	}

	try {
		rina::extendedIPCManager->appUnregistered(event.getIPCProcessName(), event.getDIFName());
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}

	LOG_IPCP_INFO("IPC Process unregistered from N-1 DIF %s",
		      event.getDIFName().processName.c_str());

	try {
		std::stringstream ss;
		ss << rina::UnderlayingRegistrationRIBObj::object_name_prefix;
		ss << event.getDIFName().processName;

		ipc_process_->rib_daemon_->removeObjRIB(ss.str());
	}catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems removing object from RIB: %s",
			     e.what());
	}
}

std::list<int> NMinusOneFlowManager::getNMinusOneFlowsToNeighbour(unsigned int address) {
	std::vector<rina::FlowInformation> flows = rina::extendedIPCManager->getAllocatedFlows();
	std::list<int> result;
	unsigned int target_address = 0;
	for (unsigned int i=0; i<flows.size(); i++) {
		target_address = ipc_process_->namespace_manager_->getAdressByname(
				flows[i].remoteAppName);
		if (target_address == address) {
			result.push_back(flows[i].portId);
		}
	}

	return result;
}

std::list<int> NMinusOneFlowManager::getNMinusOneFlowsToNeighbour(const std::string& name)
{
	std::vector<rina::FlowInformation> flows;
	std::list<int> result;

	flows = rina::extendedIPCManager->getAllocatedFlows();
	for (unsigned int i=0; i<flows.size(); i++) {
		if (flows[i].remoteAppName.processName == name) {
			result.push_back(flows[i].portId);
		}
	}

	return result;
}

int NMinusOneFlowManager::getManagementFlowToNeighbour(const std::string& name) {
	const std::list<rina::Neighbor> neighbors =
			ipc_process_->enrollment_task_->get_neighbors();
	for (std::list<rina::Neighbor>::const_iterator it = neighbors.begin();
			it != neighbors.end(); ++it) {
		if (it->name_.processName == name) {
			return it->underlying_port_id_;
		}
	}

	return -1;
}

int NMinusOneFlowManager::get_n1flow_to_neighbor(const rina::FlowSpecification& fspec,
					         const std::string& name)
{
	std::vector<rina::FlowInformation> flows = rina::extendedIPCManager->getAllocatedFlows();
	for (unsigned int i=0; i<flows.size(); i++) {
		if (flows[i].remoteAppName.processName == name &&
				flows[i].flowSpecification.delay == fspec.delay &&
				flows[i].flowSpecification.loss == fspec.loss) {
			return flows[i].portId;
		}
	}

	return -1;
}

int NMinusOneFlowManager::getManagementFlowToNeighbour(unsigned int address)
{
	const std::list<rina::Neighbor> neighbors =
			ipc_process_->enrollment_task_->get_neighbors();
	for (std::list<rina::Neighbor>::const_iterator it = neighbors.begin();
			it != neighbors.end(); ++it) {
		if (it->address_ == address) {
			return it->underlying_port_id_;
		}
	}

	return -1;
}

std::list<int> NMinusOneFlowManager::getManagementFlowsToAllNeighbors(void)
{
	std::list<int> result;

	const std::list<rina::Neighbor> neighbors =
			ipc_process_->enrollment_task_->get_neighbors();
	for (std::list<rina::Neighbor>::const_iterator it = neighbors.begin();
			it != neighbors.end(); ++it) {
		result.push_back(it->underlying_port_id_);
	}

	return result;
}

unsigned int NMinusOneFlowManager::numberOfFlowsToNeighbour(const std::string& apn,
		const std::string& api) {
	std::vector<rina::FlowInformation> flows = rina::extendedIPCManager->getAllocatedFlows();
	unsigned int result = 0;
	for (unsigned int i=0; i<flows.size(); i++) {
		if (flows[i].remoteAppName.processName == apn &&
				flows[i].remoteAppName.processInstance == api) {
			result ++;
		}
	}

	return result;
}

//Class IPCP Flow Acceptor
bool IPCPFlowAcceptor::accept_flow(const rina::FlowRequestEvent& event)
{
	bool have_flow_with_remote_app;

	if (ipcp_->get_operational_state() != ASSIGNED_TO_DIF) {
		return false;
	}

	// TODO Deal with the different AEs (Management vs. Data transfer), right now assuming the flow
	//is both used for data transfer and management purposes. Right now accepting all flows
	//this is an obvious problem for (D)DoS. Implement a better policy here (max. number of flows
	//per peer, for instance)

	return true;
}

//CLASS Resource Allocator
ResourceAllocator::ResourceAllocator() : IResourceAllocator() {
	n_minus_one_flow_manager_ = new NMinusOneFlowManager();
	ipcp = 0;
	rib_daemon_ = 0;
}

ResourceAllocator::~ResourceAllocator() {
	delete ps;
}

void ResourceAllocator::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
		return;

	app = ap;
	ipcp = dynamic_cast<IPCProcess*>(app);
	if (!ipcp) {
		LOG_IPCP_ERR("Bogus instance of IPCP passed, return");
		return;
	}

	if (n_minus_one_flow_manager_) {
		n_minus_one_flow_manager_->set_rib_handle(ipcp->rib_daemon_->get_rib_handle());
		n_minus_one_flow_manager_->set_ipc_process(ipcp);
	}

	rib_daemon_ = ipcp->rib_daemon_;

	populateRIB();
	subscribeToEvents();
}

/// Create initial RIB objects
void ResourceAllocator::populateRIB()
{
	rina::rib::RIBObj* tmp;

	try {
		tmp = new rina::rib::RIBObj("ResourceAllocator");
		rib_daemon_->addObjRIB("/ra", &tmp);

		tmp = new QoSCubesRIBObject(ipcp);
		rib_daemon_->addObjRIB(QoSCubesRIBObject::object_name, &tmp);

		tmp = new rina::rib::RIBObj(NextHopTEntryRIBObj::parent_class_name);
		rib_daemon_->addObjRIB(NextHopTEntryRIBObj::parent_object_name, &tmp);

		tmp = new rina::rib::RIBObj(PDUFTEntryRIBObj::parent_class_name);
		rib_daemon_->addObjRIB(PDUFTEntryRIBObj::parent_object_name, &tmp);

		tmp = new rina::rib::RIBObj("RMT");
		rib_daemon_->addObjRIB("/rmt", &tmp);

		tmp = new rina::rib::RIBObj("RMTN1Flows");
		rib_daemon_->addObjRIB("/rmt/n1flows", &tmp);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems adding object to the RIB : %s", e.what());
	}
}

void ResourceAllocator::eventHappened(rina::InternalEvent * event)
{
	if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED){
		rina::NMinusOneFlowDeallocatedEvent * flowEvent =
				(rina::NMinusOneFlowDeallocatedEvent *) event;
		nMinusOneFlowDeallocated(flowEvent);
	}else if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED){
		rina::NMinusOneFlowAllocatedEvent * flowEvent =
				(rina::NMinusOneFlowAllocatedEvent *) event;
		nMinusOneFlowAllocated(flowEvent);
	}
}

void ResourceAllocator::sync_with_kernel()
{
	std::map<int, RMTN1Flow*>::iterator iterator;

	n1_flows_lock.lock();
	for(iterator = n1_flows.begin(); iterator != n1_flows.end(); ++iterator) {
		iterator->second->sync_with_kernel();
	}
	n1_flows_lock.unlock();
}

void ResourceAllocator::nMinusOneFlowAllocated(rina::NMinusOneFlowAllocatedEvent * flowEvent)
{
	n1_flows_lock.lock();
	RMTN1Flow * flow = new RMTN1Flow(flowEvent->flow_information_.portId,
					 ipcp->get_id());
	n1_flows[flow->port_id] = flow;
	n1_flows_lock.unlock();

	rina::rib::RIBObj * rib_obj = new RMTN1FlowRIBObj(flow);
	std::stringstream ss;
	ss << RMTN1FlowRIBObj::object_name_prefix
	   << flowEvent->flow_information_.portId;

	try {
		rib_daemon_->addObjRIB(ss.str(), &rib_obj);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems adding object to RIB: %s",
			     e.what());
	}
}

void ResourceAllocator::nMinusOneFlowDeallocated(rina::NMinusOneFlowDeallocatedEvent  * event)
{
	std::map<int, RMTN1Flow*>::iterator iterator;
	RMTN1Flow * flow;

	n1_flows_lock.lock();
	iterator = n1_flows.find(event->port_id_);
	if (iterator == n1_flows.end())
		return;
	flow = iterator->second;
	n1_flows.erase(iterator);
	n1_flows_lock.unlock();

	std::stringstream ss;
	ss << RMTN1FlowRIBObj::object_name_prefix
	   << event->port_id_;

	try {
		rib_daemon_->removeObjRIB(ss.str());
		delete flow;
	} catch (rina::Exception &e) {
		delete flow;
		LOG_IPCP_ERR("Problems removing object from RIB: %s",
			     e.what());
	}
}

void ResourceAllocator::subscribeToEvents()
{
	ipcp->internal_event_manager_->subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED,
					       	        this);
	ipcp->internal_event_manager_->subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED,
					 	 	this);
}

void ResourceAllocator::set_dif_configuration(const rina::DIFInformation& dif_information)
{
	std::string ps_name = dif_information.dif_configuration_.ra_configuration_.pduftg_conf_.policy_set_.name_;
	if (set_pduft_gen_policy_set(ps_name) != 0) {
		throw rina::Exception("Cannot create PDU Forwarding Table Generator policy-set");
	}

	if (n_minus_one_flow_manager_) {
		n_minus_one_flow_manager_->set_dif_configuration(dif_information);
	}

	if (pduft_gen_ps) {
		pduft_gen_ps->set_dif_configuration(dif_information.dif_configuration_);
	}

	//Create QoS cubes RIB objects
	std::list<rina::QoSCube*>::const_iterator it;
	std::stringstream ss;
	const std::list<rina::QoSCube*>& cubes = dif_information.dif_configuration_
		.efcp_configuration_.qos_cubes_;
	for (it = cubes.begin(); it != cubes.end(); ++it) {
		try {
			ss << QoSCubeRIBObject::object_name_prefix
			   << (*it)->id_;
			rina::rib::RIBObj * nrobj = new QoSCubeRIBObject(*it);
			rib_daemon_->addObjRIB(ss.str(), &nrobj);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems creating RIB object: %s",
					e.what());
		}
		ss.str(std::string());
		ss.clear();
	}
}

INMinusOneFlowManager * ResourceAllocator::get_n_minus_one_flow_manager() const
{
	return n_minus_one_flow_manager_;
}

std::list<rina::QoSCube*> ResourceAllocator::getQoSCubes()
{
	return ipcp->get_dif_information().dif_configuration_.efcp_configuration_.qos_cubes_;
}

void ResourceAllocator::addQoSCube(const rina::QoSCube& cube)
{
	rina::ScopedLock g(lock);

	rina::QoSCube * qos_cube;
	std::list<rina::QoSCube*> cubes =
			ipcp->get_dif_information().dif_configuration_.efcp_configuration_.qos_cubes_;

	std::list<rina::QoSCube*>::const_iterator it;
	for (it = cubes.begin(); it != cubes.end(); ++it) {
		if ((*it)->id_ == cube.id_) {
			LOG_IPCP_WARN("Tried to add an already existing QoS cube: %u",
				      cube.id_);
			return;
		}
	}

	qos_cube = new rina::QoSCube(cube);

	try {
		std::stringstream ss;
		ss << QoSCubeRIBObject::object_name_prefix
		   << cube.id_;

		rina::rib::RIBObj * nrobj = new QoSCubeRIBObject(qos_cube);
		rib_daemon_->addObjRIB(ss.str(), &nrobj);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems creating RIB object: %s",
			     e.what());
	}

	cubes.push_back(qos_cube);
}

std::list<rina::PDUForwardingTableEntry> ResourceAllocator::get_pduft_entries()
{
	std::list<rina::PDUForwardingTableEntry> result;
	std::map<std::string, rina::PDUForwardingTableEntry *>::iterator it;

	rina::ReadScopedLock g(pduft_lock);

	for (it = pduft.begin(); it != pduft.end(); ++it) {
		result.push_back(*(it->second));
	}

	return result;
}

/// This operation takes ownership of the entries
void ResourceAllocator::set_pduft_entries(const std::list<rina::PDUForwardingTableEntry*>& pduft_entries)
{
	rina::PDUForwardingTableEntry * pdufte;
	std::map<std::string, rina::PDUForwardingTableEntry *>::iterator it;
	std::list<rina::PDUForwardingTableEntry*>::const_iterator it2;
	rina::rib::RIBObj * ribObj;
	std::string obj_name;
	std::stringstream ss;

	rina::WriteScopedLock g(pduft_lock);

	//1 Scrap the old entries
	for (it = pduft.begin(); it != pduft.end(); ++it) {
		try {
			rib_daemon_->removeObjRIB(it->first);
		} catch (rina::Exception &e) {
			LOG_WARN("Problems removing RIB obj: %s", e.what());
		}

		pdufte = it->second;
		delete pdufte;
		pdufte = 0;
	}

	pduft.clear();

	//2 Add the new entries
	for (it2 = pduft_entries.begin();
			it2 != pduft_entries.end(); ++it2) {
		ss << PDUFTEntryRIBObj::object_name_prefix;
		ss << (*it2)->getKey();
		obj_name = ss.str();
		ss.str(std::string());
		ss.clear();

		try {
			ribObj = new PDUFTEntryRIBObj(*it2);
			rib_daemon_->addObjRIB(obj_name, &ribObj);
		} catch (rina::Exception &e) {
			//LOG_WARN("Problems adding RIB obj: %s", e.what());
			continue;
		}

		pduft[obj_name] = *it2;
	}

	//3 Update temp entries
	update_temp_entries();
}

void ResourceAllocator::update_temp_entries()
{
	std::list<rina::PDUForwardingTableEntry*>::iterator it;
	std::list<rina::PDUForwardingTableEntry*> to_add;

	for(it = temp_entries.begin(); it != temp_entries.end(); ++it) {
		if (!entry_is_in_pduft((*it)->address)) {
			to_add.push_back(*it);
		}
	}

	if (to_add.size() == 0) {
		return;
	}

	try {
		rina::kernelIPCProcess->modifyPDUForwardingTableEntries(to_add, 0);
	} catch (rina::Exception & e) {
		LOG_IPCP_ERR("Error adding entries to PDU Forwarding Table in the kernel: %s",
				e.what());
	}
}

std::list<rina::RoutingTableEntry> ResourceAllocator::get_rt_entries()
{
	std::list<rina::RoutingTableEntry> result;
	std::map<std::string, rina::RoutingTableEntry *>::iterator it;

	rina::ReadScopedLock g(rt_lock);

	for (it = rt.begin(); it != rt.end(); ++it) {
		result.push_back(*(it->second));
	}

	return result;
}

void ResourceAllocator::set_rt_entries(const std::list<rina::RoutingTableEntry*>& rt_entries)
{
	rina::RoutingTableEntry * rte;
	std::map<std::string, rina::RoutingTableEntry *>::iterator it;
	std::list<rina::RoutingTableEntry*>::const_iterator it2;
	rina::rib::RIBObj * ribObj;
	std::string obj_name;
	std::stringstream ss;

	rina::WriteScopedLock g(rt_lock);

	//1 Scrap the old entries
	for (it = rt.begin(); it != rt.end(); ++it) {
		try {
			rib_daemon_->removeObjRIB(it->first);
		} catch (rina::Exception &e) {
			LOG_WARN("Problems removing RIB obj: %s", e.what());
		}

		rte = it->second;
		delete rte;
		rte = 0;
	}

	rt.clear();

	//2 Add the new entries
	for (it2 = rt_entries.begin();
			it2 != rt_entries.end(); ++it2) {
		ss << NextHopTEntryRIBObj::object_name_prefix;
		ss << (*it2)->getKey();
		obj_name = ss.str();
		ss.str(std::string());
		ss.clear();

		try {
			ribObj = new NextHopTEntryRIBObj(*it2);
			rib_daemon_->addObjRIB(obj_name, &ribObj);
		} catch (rina::Exception &e) {
			LOG_WARN("Problems adding RIB obj: %s", e.what());
			continue;
		}

		rt[obj_name] = *it2;
	}
}

int ResourceAllocator::get_next_hop_addresses(unsigned int dest_address,
					      std::list<unsigned int>& addresses)
{
	std::map<std::string, rina::RoutingTableEntry *>::iterator it;
	std::list<unsigned int>::iterator it2;

	rina::ReadScopedLock g(rt_lock);

	for (it = rt.begin(); it != rt.end(); ++it) {
		for (it2 = it->second->destination.addresses.begin();
				it2 != it->second->destination.addresses.end(); ++it2) {
			if (dest_address == *it2) {
				addresses = it->second->nextHopNames.front().alts.front().addresses;
				return 0;
			}
		}
	}

	return -1;
}

int ResourceAllocator::get_next_hop_name(const std::string& dest_name,
					 std::string& name)
{
	std::map<std::string, rina::RoutingTableEntry *>::iterator it;

	rina::ReadScopedLock g(rt_lock);

	for (it = rt.begin(); it != rt.end(); ++it) {
		if (it->second->destination.name == dest_name) {
			name = it->second->nextHopNames.front().alts.front().name;
			return 0;
		}
	}

	return -1;
}

unsigned int ResourceAllocator::get_n1_port_to_address(unsigned int dest_address)
{
	std::map<std::string, rina::PDUForwardingTableEntry *>::iterator it;

	rina::ReadScopedLock g(pduft_lock);

	for (it = pduft.begin(); it != pduft.end(); ++it) {
		if (it->second->address == dest_address) {
			return it->second->portIdAltlists.front().alts.front();
		}
	}

	return 0;
}

bool ResourceAllocator::contains_temp_entry(unsigned int dest_address)
{
	std::list<rina::PDUForwardingTableEntry*>::iterator it;

	for (it = temp_entries.begin(); it != temp_entries.end(); ++it) {
		if ((*it)->address == dest_address) {
			return true;
		}
	}

	return false;
}

bool ResourceAllocator::entry_is_in_pduft(unsigned int dest_address)
{
	std::map<std::string, rina::PDUForwardingTableEntry*>::iterator it;

	for (it = pduft.begin(); it != pduft.end(); ++it) {
		if (it->second->address == dest_address) {
			return true;
		}
	}

	return false;
}

void ResourceAllocator::add_temp_pduft_entry(unsigned int dest_address, int port_id)
{
	std::list<unsigned int>::iterator it2;
	rina::PortIdAltlist pid_list;
	rina::PDUForwardingTableEntry* entry;
	std::list<rina::PDUForwardingTableEntry*> to_add;

	rina::WriteScopedLock g(pduft_lock);

	if (contains_temp_entry(dest_address) ||
			entry_is_in_pduft(dest_address)) {
		return;
	}

	entry = new rina::PDUForwardingTableEntry();
	entry->address = dest_address;
	pid_list.add_alt(port_id);
	entry->portIdAltlists.push_back(pid_list);
	to_add.push_back(entry);
	temp_entries.push_back(entry);

	try {
		rina::kernelIPCProcess->modifyPDUForwardingTableEntries(to_add, 0);
	} catch (rina::Exception & e) {
		LOG_IPCP_ERR("Error adding entry to PDU Forwarding Table in the kernel: %s",
				e.what());
	}
}

void ResourceAllocator::remove_temp_pduft_entry(unsigned int dest_address)
{
	std::list<rina::PDUForwardingTableEntry*>::iterator it;
	rina::PDUForwardingTableEntry * entry;

	rina::WriteScopedLock g(pduft_lock);

	it = temp_entries.begin();
	while (it != temp_entries.end()) {
	    entry = *it;
	    if (entry->address == dest_address) {
		    it = temp_entries.erase(it);
		    LOG_IPCP_DBG("Deleting temp entry %s", entry->toString().c_str());
		    delete entry;
		    entry = 0;
	    } else {
	        ++it;
	    }
	}
}

} //namespace rinad
