//
// IPC Resource Manager. Requires that the application process is
// part of has a RIB Daemon, a CDAP Session Manager and an Internal
// Event Manager.
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

#define RINA_PREFIX "librina.irm"

#include "librina/logs.h"
#include "librina/irm.h"
#include "librina/ipc-process.h"

#include "irm.pb.h"

namespace rina {

IPCResourceManager::IPCResourceManager() : ApplicationEntity(ApplicationEntity::IRM_AE_NAME),
		ipcp(false), rib_daemon_(NULL), rib(0), event_manager_(NULL), flow_acceptor_(NULL)
{
}

IPCResourceManager::IPCResourceManager(bool isIPCP) : ApplicationEntity(ApplicationEntity::IRM_AE_NAME),
		ipcp(isIPCP), rib_daemon_(NULL), rib(0), event_manager_(NULL), flow_acceptor_(NULL)
{
}

void IPCResourceManager::set_application_process(rina::ApplicationProcess * ap)
{
	ApplicationEntity * ae;
	if (!ap) {
		LOG_ERR("Bogus instance of APP passed, return");
		return;
	}

	app = ap;

	ae = app->get_rib_daemon();
	if (!ae) {
		LOG_ERR("App has no RIB Daemon AE, return");
		return;
	}
	rib_daemon_ = dynamic_cast<rib::RIBDaemonProxy*>(ae);

	ae = app->get_internal_event_manager();
	if (!ae) {
		LOG_ERR("App has no Internal Event Manager AE, return");
		return;
	}
	event_manager_ = dynamic_cast<InternalEventManager*>(ae);
}

void IPCResourceManager::set_flow_acceptor(FlowAcceptor * fa)
{
	flow_acceptor_ = fa;
}

void IPCResourceManager::set_rib_handle(rina::rib::rib_handle_t rib_handle)
{
	rib = rib_handle;
}

void IPCResourceManager::populateRIB()
{
	rina::rib::RIBObj* tmp;

	try {
		tmp = new rina::rib::RIBObj("IPCResourceManager");
		rib_daemon_->addObjRIB(rib, "/ipcm/irm", &tmp);

		tmp = new rina::rib::RIBObj(UnderlayingDIFRIBObj::parent_class_name);
		rib_daemon_->addObjRIB(rib,
				       UnderlayingDIFRIBObj::parent_object_name,
				       &tmp);

		tmp = new rina::rib::RIBObj(UnderlayingRegistrationRIBObj::parent_class_name);
		rib_daemon_->addObjRIB(rib,
				       UnderlayingRegistrationRIBObj::parent_object_name,
				       &tmp);

		tmp = new rina::rib::RIBObj(UnderlayingFlowRIBObj::parent_class_name);
		rib_daemon_->addObjRIB(rib,
				       UnderlayingFlowRIBObj::parent_object_name,
				       &tmp);
	} catch (Exception &e) {
		LOG_ERR("Problems adding object to the RIB : %s", e.what());
	}
}

FlowInformation IPCResourceManager::getNMinus1FlowInformation(int portId) const
{
	if (ipcp) {
		return extendedIPCManager->getFlowInformation(portId);
	} else {
		return ipcManager->getFlowInformation(portId);
	}
}

unsigned int IPCResourceManager::allocateNMinus1Flow(const FlowInformation& flowInformation)
{
	unsigned int handle = 0;

	try {
		if (ipcp) {
			handle = extendedIPCManager->requestFlowAllocationInDIF(flowInformation.localAppName,
										flowInformation.remoteAppName,
										flowInformation.difName,
										flowInformation.flowSpecification);
		} else {
			handle = ipcManager->requestFlowAllocationInDIF(flowInformation.localAppName,
								 	flowInformation.remoteAppName,
								 	flowInformation.difName,
									flowInformation.flowSpecification);
		}
	} catch(FlowAllocationException &e) {
		throw Exception(e.what());
	}

	LOG_INFO("Requested the allocation of N-1 flow to application %s-%s through DIF %s",
		  flowInformation.remoteAppName.processName.c_str(),
		  flowInformation.remoteAppName.processInstance.c_str(),
		  flowInformation.difName.processName.c_str());

	return handle;
}

void IPCResourceManager::allocateRequestResult(const AllocateFlowRequestResultEvent& event)
{
	if (event.portId <= 0) {
		std::stringstream ss;
		ss << event.portId;
		LOG_ERR("Allocation of N-1 flow denied. Error code: %d", event.portId);
		FlowInformation flowInformation;
		if (ipcp) {
			flowInformation = extendedIPCManager->withdrawPendingFlow(event.sequenceNumber);
		} else {
			flowInformation = ipcManager->withdrawPendingFlow(event.sequenceNumber);
		}
		InternalEvent * flowFailedEvent =
				new NMinusOneFlowAllocationFailedEvent(event.sequenceNumber,
								       flowInformation, ss.str());
		event_manager_->deliverEvent(flowFailedEvent);
		return;
	}

	FlowInformation flow;
	if (ipcp) {
		flow = extendedIPCManager->commitPendingFlow(event.sequenceNumber,
							     event.portId,
							     event.difName);
	} else {
		flow = ipcManager->commitPendingFlow(event.sequenceNumber,
						     event.portId,
						     event.difName);
	}

	try {
		std::stringstream ss;
		ss << UnderlayingFlowRIBObj::object_name_prefix << event.portId;

		UnderlayingFlowRIBObj * ufrobj = new UnderlayingFlowRIBObj(flow);
		rib_daemon_->addObjRIB(rib, ss.str(), &ufrobj);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}

	InternalEvent * flowAllocatedEvent =
			new NMinusOneFlowAllocatedEvent(event.sequenceNumber, flow);
	event_manager_->deliverEvent(flowAllocatedEvent);
}

void IPCResourceManager::flowAllocationRequested(const FlowRequestEvent& event)
{
	if (event.localApplicationName.processName.compare(app->get_name()) != 0 ||
	    event.localApplicationName.processInstance.compare(app->get_instance()) != 0) {
		LOG_ERR("Rejected flow request from %s-%s since this IPC Process is not the intended target of this flow",
			 event.remoteApplicationName.processName.c_str(),
			 event.remoteApplicationName.processInstance.c_str());
		try {
			if (ipcp) {
				extendedIPCManager->allocateFlowResponse(event, -1, true);
			} else {
				ipcManager->allocateFlowResponse(event, -1, true);
			}
		} catch (Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}
		return;
	}

	// Check optional extra conditions to accept the flow
	if (flow_acceptor_) {
		if (!flow_acceptor_->accept_flow(event)) {
			try {
				if (ipcp) {
					extendedIPCManager->allocateFlowResponse(event, -1, true);
				} else {
					ipcManager->allocateFlowResponse(event, -1, true);
				}
			} catch (Exception &e) {
				LOG_ERR("Problems communicating with the IPC Manager: %s",
					 e.what());
			}
			LOG_INFO("Flow acceptor rejected the flow");
			return;
		}
	}

	FlowInformation flow;
	try {
		if (ipcp) {
			flow = extendedIPCManager->allocateFlowResponse(event, 0, true);
		} else {
			flow = ipcManager->allocateFlowResponse(event, 0, true);
		}
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		return;
	}

	LOG_INFO("Accepted new flow from Application %s-%s",
		  event.remoteApplicationName.processName.c_str(),
	          event.remoteApplicationName.processInstance.c_str());
	try {
		std::stringstream ss;
		ss << UnderlayingFlowRIBObj::object_name_prefix << event.portId;

		UnderlayingFlowRIBObj * ufrobj = new UnderlayingFlowRIBObj(flow);
		rib_daemon_->addObjRIB(rib, ss.str(), &ufrobj);
	} catch (Exception &e){
		LOG_ERR("Error creating RIB object: %s", e.what());
	}

	InternalEvent * flowAllocatedEvent =
			new NMinusOneFlowAllocatedEvent(event.sequenceNumber, flow);
	event_manager_->deliverEvent(flowAllocatedEvent);
}

void IPCResourceManager::deallocateNMinus1Flow(int portId)
{
	try {
		if (ipcp) {
			extendedIPCManager->deallocate_flow(portId);
		} else {
			ipcManager->deallocate_flow(portId);
		}

		cleanFlowAndNotify(portId);
	}catch (Exception &e) {
		LOG_ERR("Problems deallocating N-1 flow: %s", e.what());
	}
}

void IPCResourceManager::flowDeallocatedRemotely(const FlowDeallocatedEvent& event)
{
	try {
		if (ipcp) {
			extendedIPCManager->flowDeallocated(event.portId);
		} else {
			ipcManager->flowDeallocated(event.portId);
		}
	} catch (Exception &e) {
		LOG_ERR("%s", e.what());
	}

	cleanFlowAndNotify(event.portId);
}

void IPCResourceManager::cleanFlowAndNotify(int portId)
{
	try{
		std::stringstream ss;
		ss << UnderlayingFlowRIBObj::object_name_prefix << portId;
		rib_daemon_->removeObjRIB(rib, ss.str());
	}catch(Exception &e) {
		LOG_ERR("Problems deleting object from the RIB: %s", e.what());
	}

	//Notify about the event
	NMinusOneFlowDeallocatedEvent * flowDeEvent = new NMinusOneFlowDeallocatedEvent(portId);
	event_manager_->deliverEvent(flowDeEvent);
}

bool IPCResourceManager::isSupportingDIF(const ApplicationProcessNamingInformation& difName)
{
	std::vector<ApplicationRegistration*> registrations;
	if (ipcp) {
		registrations = extendedIPCManager->getRegisteredApplications();
	} else {
		registrations = ipcManager->getRegisteredApplications();
	}
	std::list<ApplicationProcessNamingInformation> namesList;
	for(unsigned int i=0; i<registrations.size(); i++) {
		namesList = registrations[i]->DIFNames;
		for (std::list<ApplicationProcessNamingInformation>::iterator it = namesList.begin();
			it != namesList.end(); ++it){
			if (it->processName.compare(difName.processName) == 0) {
				return true;
			}
		}
	}

	return false;
}

std::list<FlowInformation> IPCResourceManager::getAllNMinusOneFlowInformation() const
{
	std::vector<FlowInformation> flows;
	std::list<FlowInformation> result;

	if (ipcp) {
		flows = extendedIPCManager->getAllocatedFlows();
	} else {
		flows = ipcManager->getAllocatedFlows();
	}

	for (unsigned int i=0; i<flows.size(); i++) {
		//Don't report internal flows in case of an IPCP
		if (ipcp && flows[i].fd > 0) {
			continue;
		}

		result.push_back(flows[i]);
	}

	return result;
}

//Class UnderlayingRegistrationRIBObjt
const std::string UnderlayingRegistrationRIBObj::class_name = "UnderlayingRegistration";
const std::string UnderlayingRegistrationRIBObj::object_name_prefix = "/ipcm/irm/uregs/dif=";
const std::string UnderlayingRegistrationRIBObj::parent_class_name = "UnderlayingRegistrations";
const std::string UnderlayingRegistrationRIBObj::parent_object_name = "/ipcm/irm/uregs";

UnderlayingRegistrationRIBObj::UnderlayingRegistrationRIBObj(const std::string& dif_name_) :
			rib::RIBObj(class_name), dif_name(dif_name_)
{
}

const std::string UnderlayingRegistrationRIBObj::get_displayable_value() const
{
	std::stringstream ss;
	ss << "N-1 DIF name: " << dif_name;

	return ss.str();
}

void UnderlayingRegistrationRIBObj::read(const rina::cdap_rib::con_handle_t &con,
					 const std::string& fqn,
					 const std::string& class_,
					 const rina::cdap_rib::filt_info_t &filt,
					 const int invoke_id,
					 rina::cdap_rib::obj_info_t &obj_reply,
					 rina::cdap_rib::res_info_t& res)
{
	cdap::StringEncoder encoder;
	encoder.encode(dif_name, obj_reply.value_);

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

//Class UnderlayingFlowRIBObj
const std::string UnderlayingFlowRIBObj::class_name = "UnderlayingFlow";
const std::string UnderlayingFlowRIBObj::object_name_prefix = "/ipcm/irm/uflows/pid=";
const std::string UnderlayingFlowRIBObj::parent_class_name = "UnderlayingFlows";
const std::string UnderlayingFlowRIBObj::parent_object_name = "/ipcm/irm/uflows";

UnderlayingFlowRIBObj::UnderlayingFlowRIBObj(const rina::FlowInformation& flow_info)
		: rib::RIBObj(class_name), flow_information(flow_info)
{
}

const std::string UnderlayingFlowRIBObj::get_displayable_value() const
{
	std::stringstream ss;
	ApplicationProcessNamingInformation name;
	name = flow_information.localAppName;
	ss << "Local app name: " << name.getEncodedString();
	name = flow_information.remoteAppName;
	ss << "Remote app name: " << name.getEncodedString() << std::endl;
	ss << "N-1 DIF name: " << flow_information.difName.processName;
	ss << "; port-id: " << flow_information.portId << std::endl;
	FlowSpecification flowSpec = flow_information.flowSpecification;
	ss << "Flow characteristics: " << flowSpec.toString();
	return ss.str();
}

void UnderlayingFlowRIBObj::read(const rina::cdap_rib::con_handle_t &con,
				 const std::string& fqn,
				 const std::string& class_,
				 const rina::cdap_rib::filt_info_t &filt,
				 const int invoke_id,
				 rina::cdap_rib::obj_info_t &obj_reply,
				 rina::cdap_rib::res_info_t& res)
{
	FlowInformationEncoder encoder;
	encoder.encode(flow_information, obj_reply.value_);

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

//Class UnderlayingDIFRIBObj
const std::string UnderlayingDIFRIBObj::class_name = "UnderlayingDIF";
const std::string UnderlayingDIFRIBObj::object_name_prefix = "/ipcm/irm/udifs/name=";
const std::string UnderlayingDIFRIBObj::parent_class_name = "UnderlayingDIFs";
const std::string UnderlayingDIFRIBObj::parent_object_name = "/ipcm/irm/udifs";
UnderlayingDIFRIBObj::UnderlayingDIFRIBObj(const DIFProperties& dif_info)
		: rib::RIBObj(class_name), dif_properties(dif_info)
{
}

const std::string UnderlayingDIFRIBObj::get_displayable_value() const
{
	std::stringstream ss;
	ss << "DIF name: " << dif_properties.DIFName.getEncodedString();
	ss << "; Max SDU size: " << dif_properties.maxSDUSize;
	return ss.str();
}

void UnderlayingDIFRIBObj::read(const rina::cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const rina::cdap_rib::filt_info_t &filt,
				const int invoke_id,
				rina::cdap_rib::obj_info_t &obj_reply,
				rina::cdap_rib::res_info_t& res)
{
	DIFPropertiesEncoder encoder;
	encoder.encode(dif_properties, obj_reply.value_);

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

//Class DIFPropertiesEncoder
void DIFPropertiesEncoder::encode(const DIFProperties &obj,
				  rina::ser_obj_t& serobj)
{
	rina::messages::difProperties_t gpb;

	gpb.set_max_sdu_size(obj.maxSDUSize);
	gpb.set_dif_name(obj.DIFName.processName);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new unsigned char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void DIFPropertiesEncoder::decode(const rina::ser_obj_t &serobj,
				  DIFProperties &des_obj)
{
	rina::messages::difProperties_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	des_obj.maxSDUSize = gpb.max_sdu_size();
	des_obj.DIFName.processName = gpb.dif_name();
}

//Class FlowInformationEncoder
void FlowInformationEncoder::encode(const FlowInformation &obj,
				    rina::ser_obj_t& serobj)
{
	rina::messages::flowInformation_t gpb;

	gpb.set_local_apn(obj.localAppName.processName);
	gpb.set_local_api(obj.localAppName.processInstance);
	gpb.set_local_aen(obj.localAppName.entityName);
	gpb.set_local_aei(obj.localAppName.entityInstance);

	gpb.set_remote_apn(obj.remoteAppName.processName);
	gpb.set_remote_api(obj.remoteAppName.processInstance);
	gpb.set_remote_aen(obj.remoteAppName.entityName);
	gpb.set_remote_aei(obj.remoteAppName.entityInstance);

	gpb.set_dif_name(obj.difName.processName);
	gpb.set_port_id(obj.portId);
	if (!messages::flowStateValues_t_IsValid(obj.state)) {
		throw Exception("Encoding Message: Not a valid flow state");
	}
	gpb.set_state((messages::flowStateValues_t) obj.state);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new unsigned char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void FlowInformationEncoder::decode(const rina::ser_obj_t &serobj,
				    FlowInformation &des_obj)
{
	rina::messages::flowInformation_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	des_obj.localAppName.processName = gpb.local_apn();
	des_obj.localAppName.processInstance = gpb.local_api();
	des_obj.localAppName.entityName = gpb.local_aen();
	des_obj.localAppName.entityInstance = gpb.local_aei();

	des_obj.remoteAppName.processName = gpb.remote_apn();
	des_obj.remoteAppName.processInstance = gpb.remote_api();
	des_obj.remoteAppName.entityName = gpb.remote_aen();
	des_obj.remoteAppName.entityInstance = gpb.remote_aei();

	des_obj.difName.processName = gpb.dif_name();
	des_obj.portId = gpb.port_id();
	if (gpb.has_state()) {
		int state_value = gpb.state();
		FlowInformation::FlowState state =
				static_cast<FlowInformation::FlowState>(state_value);
		des_obj.state = state;
	}
}

}
