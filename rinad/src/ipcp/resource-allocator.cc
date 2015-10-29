//
// Resource Allocator
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
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

#include <sstream>

#include <librina/internal-events.h>

#define IPCP_MODULE "resource-allocator"

#include "ipcp-logging.h"
#include "resource-allocator.h"

namespace rinad {

// Class QoSCube RIB object
const std::string QoSCubeRIBObject::class_name = "QoSCube";
const std::string QoSCubeRIBObject::object_name_prefix = "/resalloc/qoscubes/id=";

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

//Class QoS Cube Set RIB Object
const std::string QoSCubesRIBObject::class_name = "QoSCubes";
const std::string QoSCubesRIBObject::object_name = "/resalloc/qoscubes";

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

void NMinusOneFlowManager::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	LOG_IPCP_DBG("DIF configuration set %u", dif_configuration.address_);
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

int NMinusOneFlowManager::getManagementFlowToNeighbour(unsigned int address) {
	const std::list<rina::Neighbor*> neighbors =
			ipc_process_->enrollment_task_->get_neighbor_pointers();
	for (std::list<rina::Neighbor*>::const_iterator it = neighbors.begin();
			it != neighbors.end(); ++it) {
		if ((*it)->address_ == address) {
			return (*it)->underlying_port_id_;
		}
	}

	return -1;
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
	if (ipcp_->get_operational_state() != ASSIGNED_TO_DIF) {
		return false;
	}

	//TODO deal with the different AEs (Management vs. Data transfer), right now assuming the flow
	//is both used for data transfer and management purposes
	try {
		rina::extendedIPCManager->getPortIdToRemoteApp(event.remoteApplicationName);
		LOG_IPCP_INFO("Rejecting flow request since we already have a flow to the remote IPC Process: %s-%s",
				event.remoteApplicationName.processName.c_str(),
				event.remoteApplicationName.processInstance.c_str());
		return false;
	} catch(rina::Exception & ex) {
		return true;
	}
}

//CLASS Resource Allocator
ResourceAllocator::ResourceAllocator() : IResourceAllocator() {
	n_minus_one_flow_manager_ = new NMinusOneFlowManager();
	ipcp = 0;
	rib_daemon_ = 0;
}

ResourceAllocator::~ResourceAllocator() {
	if (n_minus_one_flow_manager_) {
		delete n_minus_one_flow_manager_;
	}
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
		n_minus_one_flow_manager_->set_ipc_process(ipcp);
	}

	rib_daemon_ = ipcp->rib_daemon_;

	populateRIB();
}

/// Create initial RIB objects
void ResourceAllocator::populateRIB()
{
	rina::rib::RIBObj* tmp;

	try {
		tmp = new QoSCubesRIBObject(ipcp);
		rib_daemon_->addObjRIB(QoSCubesRIBObject::object_name, &tmp);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems adding object to the RIB : %s", e.what());
	}
}

void ResourceAllocator::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	std::string ps_name = dif_configuration.ra_configuration_.pduftg_conf_.policy_set_.name_;
	if (set_pduft_gen_policy_set(ps_name) != 0) {
		throw rina::Exception("Cannot create PDU Forwarding Table Generator policy-set");
	}

	if (n_minus_one_flow_manager_) {
		n_minus_one_flow_manager_->set_dif_configuration(dif_configuration);
	}

	//Create QoS cubes RIB objects
	std::list<rina::QoSCube*>::const_iterator it;
	std::stringstream ss;
	const std::list<rina::QoSCube*>& cubes = dif_configuration
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

INMinusOneFlowManager * ResourceAllocator::get_n_minus_one_flow_manager() const {
	return n_minus_one_flow_manager_;
}

std::list<rina::QoSCube*> ResourceAllocator::getQoSCubes()
{
	rina::ScopedLock g(lock);
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

} //namespace rinad
