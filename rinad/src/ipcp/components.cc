//
// Common interfaces and constants of the IPC Process components
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
#include <iostream>

#define IPCP_MODULE "components"
#include "ipcp-logging.h"
#include "components.h"

namespace rinad {

const std::string IResourceAllocator::RESOURCE_ALLOCATOR_AE_NAME = "resource-allocator";
const std::string IResourceAllocator::PDUFT_GEN_COMPONENT_NAME = "pduft-generator";
const std::string INamespaceManager::NAMESPACE_MANAGER_AE_NAME = "namespace-manager";
const std::string IRoutingComponent::ROUTING_COMPONENT_AE_NAME = "routing";
const std::string IFlowAllocator::FLOW_ALLOCATOR_AE_NAME = "flow-allocator";

//	CLASS Flow
Flow::Flow() {
	source_port_id = 0;
	destination_port_id = 0;
	source_address = 0;
	destination_address = 0;
	current_connection_index = 0;
	max_create_flow_retries = 0;
	create_flow_retries = 0;
	hop_count = 0;
	source = false;
	state = EMPTY;
	access_control = 0;
}

Flow::Flow(const Flow& flow) {
	source_naming_info = flow.source_naming_info;
	destination_naming_info = flow.destination_naming_info;
	flow_specification = flow.flow_specification;
	source_port_id = flow.source_port_id;
	destination_port_id = flow.destination_port_id;
	source_address = flow.source_address;
	destination_address = flow.destination_address;
	current_connection_index = flow.current_connection_index;
	max_create_flow_retries = flow.max_create_flow_retries;
	create_flow_retries = flow.create_flow_retries;
	hop_count = flow.hop_count;
	source = flow.source;
	state = flow.state;
	access_control = 0;

	std::list<rina::Connection*>::const_iterator it;
	rina::Connection * current = 0;
	for (it = flow.connections.begin();
			it != flow.connections.end(); ++it) {
		current = *it;
		connections.push_back(new rina::Connection(*current));
	}
}

Flow::~Flow() {
	std::list<rina::Connection*>::iterator iterator;
	for (iterator = connections.begin(); iterator != connections.end();
			++iterator) {
		if (*iterator) {
			delete *iterator;
			*iterator = 0;
		}
	}
	connections.clear();
}

rina::Connection * Flow::getActiveConnection() {
	rina::Connection result;
	std::list<rina::Connection*>::iterator iterator;

	unsigned int i = 0;
	for (iterator = connections.begin(); iterator != connections.end();
			++iterator) {
		if (i == current_connection_index) {
			return *iterator;
		} else {
			i++;
		}
	}

	throw rina::Exception("No active connection is currently defined");
}

std::string Flow::toString() {
	std::stringstream ss;
	ss << "* State: " << state << std::endl;
	ss << "* Is this IPC Process the requestor of the flow? " << source
			<< std::endl;
	ss << "* Max create flow retries: " << max_create_flow_retries
			<< std::endl;
	ss << "* Hop count: " << hop_count << std::endl;
	ss << "* Source AP Naming Info: " << source_naming_info.toString()
			<< std::endl;
	;
	ss << "* Source address: " << source_address << std::endl;
	ss << "* Source port id: " << source_port_id << std::endl;
	ss << "* Destination AP Naming Info: "
			<< destination_naming_info.toString();
	ss << "* Destination addres: " << destination_address << std::endl;
	ss << "* Destination port id: " << destination_port_id << std::endl;
	if (connections.size() > 0) {
		ss << "* Connection ids of the connection supporting this flow: +\n";
		for (std::list<rina::Connection*>::const_iterator iterator =
				connections.begin(), end = connections.end(); iterator != end;
				++iterator) {
			ss << "Src CEP-id " << (*iterator)->getSourceCepId()
					<< "; Dest CEP-id " << (*iterator)->getDestCepId()
					<< "; Qos-id " << (*iterator)->getQosId() << std::endl;
		}
	}
	ss << "* Index of the current active connection for this flow: "
			<< current_connection_index << std::endl;
	return ss.str();
}

// Class IResourceAllocator
int IResourceAllocator::set_pduft_gen_policy_set(const std::string& name)
{
	if (!ipcp) {
		LOG_IPCP_ERR("IPCP is null");
		return -1;
	}

	if (pduft_gen_ps) {
		LOG_IPCP_ERR("PDUFT Generator policy set already present");
		return -1;
	}

	std::stringstream ss;
	ss << RESOURCE_ALLOCATOR_AE_NAME;
        pduft_gen_ps = (IPDUFTGeneratorPs *) ipcp->psCreate(ss.str(),
        					     	    name,
        					     	    this);
        if (!pduft_gen_ps) {
                LOG_IPCP_ERR("failed to allocate instance of policy set %s",
                	     name.c_str());
                return -1;
        }

        LOG_INFO("PDUFT Generator policy-set %s added to the resource-allocator",
                 name.c_str());
        return 0;
}

// Class BaseRIBObject
BaseIPCPRIBObject::BaseIPCPRIBObject(IPCProcess * ipc_process, const std::string& object_class,
		long object_instance, const std::string& object_name):
		        rina::BaseRIBObject(ipc_process->rib_daemon_,
		                        object_class, object_instance, object_name){
	ipc_process_ = ipc_process;
	if (ipc_process) {
		rib_daemon_ =  ipc_process->rib_daemon_;
		encoder_ = ipc_process->encoder_;
	} else {
		rib_daemon_ = 0;
		encoder_ = 0;
	}
}

// CLASS IPC Process
const std::string IPCProcess::MANAGEMENT_AE = "Management";
const std::string IPCProcess::DATA_TRANSFER_AE = "Data Transfer";
const int IPCProcess::DEFAULT_MAX_SDU_SIZE_IN_BYTES = 10000;

//Class SimpleIPCPRIBObject
SimpleIPCPRIBObject::SimpleIPCPRIBObject(IPCProcess* ipc_process, const std::string& object_class,
			const std::string& object_name, const void* object_value) :
					BaseIPCPRIBObject(ipc_process, object_class,
							rina::objectInstanceGenerator->getObjectInstance(), object_name) {
	object_value_ = object_value;
}

const void* SimpleIPCPRIBObject::get_value() const {
	return object_value_;
}

void SimpleIPCPRIBObject::writeObject(const void* object_value) {
	object_value_ = object_value;
}

void SimpleIPCPRIBObject::createObject(const std::string& objectClass, const std::string& objectName,
		const void* objectValue) {
	if (objectName.compare("") != 0 && objectClass.compare("") != 0) {
		object_value_ = objectValue;
	}
}

//Class SimpleSetRIBObject
SimpleSetIPCPRIBObject::SimpleSetIPCPRIBObject(IPCProcess * ipc_process, const std::string& object_class,
		const std::string& set_member_object_class, const std::string& object_name) :
					SimpleIPCPRIBObject(ipc_process, object_class, object_name, 0){
	set_member_object_class_ = set_member_object_class;
}

void SimpleSetIPCPRIBObject::createObject(const std::string& objectClass, const std::string& objectName,
		const void* objectValue) {
	if (set_member_object_class_.compare(objectClass) != 0) {
		throw rina::Exception("Class of set member does not match the expected value");
	}

	SimpleSetMemberIPCPRIBObject * ribObject = new SimpleSetMemberIPCPRIBObject(ipc_process_, objectClass,
			objectName, objectValue);
	add_child(ribObject);
	rib_daemon_->addRIBObject(ribObject);
}

//Class SimpleSetMemberRIBObject
SimpleSetMemberIPCPRIBObject::SimpleSetMemberIPCPRIBObject(IPCProcess* ipc_process,
                                                   const std::string& object_class,
                                                   const std::string& object_name,
                                                   const void* object_value) :
        SimpleIPCPRIBObject(ipc_process, object_class, object_name, object_value)
{
}

void SimpleSetMemberIPCPRIBObject::deleteObject(const void* objectValue)
{
	parent_->remove_child(name_);
	rib_daemon_->removeRIBObject(name_);
}

//Class IPCProcess
IPCProcess::IPCProcess(const std::string& name, const std::string& instance)
			: rina::ApplicationProcess(name, instance)
{
	delimiter_ = 0;
	encoder_ = 0;
	cdap_session_manager_ = 0;
	internal_event_manager_ = 0;
	enrollment_task_ = 0;
	flow_allocator_ = 0;
	namespace_manager_ = 0;
	resource_allocator_ = 0;
	security_manager_ = 0;
	rib_daemon_ = 0;
	routing_component_ = 0;
}

} //namespace rinad
