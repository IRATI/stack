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

#define RINA_PREFIX "ipcp-components"
#include <librina/logs.h>

#include "components.h"

namespace rinad {

//	CLASS EnrollmentRequest
EnrollmentRequest::EnrollmentRequest(rina::Neighbor * neighbor) {
	neighbor_ = neighbor;
	ipcm_initiated_ = false;
}

EnrollmentRequest::EnrollmentRequest(
		rina::Neighbor * neighbor, const rina::EnrollToDIFRequestEvent & event) {
	neighbor_ = neighbor;
	event_ = event;
	ipcm_initiated_ = true;
}

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
	ss << "* Destination addres: " + destination_address << std::endl;
	ss << "* Destination port id: " + destination_port_id << std::endl;
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
        (void) objectValue; // Stop compiler barfs

	parent_->remove_child(name_);
	rib_daemon_->removeRIBObject(name_);
}

//Class IPCProcess
IPCProcess::IPCProcess()
{
	delimiter_ = 0;
	encoder_ = 0;
	cdap_session_manager_ = 0;
	enrollment_task_ = 0;
	flow_allocator_ = 0;
	namespace_manager_ = 0;
	resource_allocator_ = 0;
	security_manager_ = 0;
	rib_daemon_ = 0;
	routing_component_ = 0;
}

//Class IPCProcessComponent
int IPCProcessComponent::select_policy_set_common(struct IPCProcess * ipcp,
                                           const std::string& component,
                                           const std::string& path,
                                           const std::string& ps_name)
{
        IPolicySet *candidate = NULL;

        if (path != std::string()) {
                LOG_ERR("No subcomponents to address");
                return -1;
        }

        if (!ipcp) {
                LOG_ERR("bug: NULL ipcp reference");
                return -1;
        }

        if (ps_name == selected_ps_name) {
                LOG_INFO("policy set %s already selected", ps_name.c_str());
                return 0;
        }

        candidate = ipcp->psCreate(component, ps_name, this);
        if (!candidate) {
                LOG_ERR("failed to allocate instance of policy set %s", ps_name.c_str());
                return -1;
        }

        if (ps) {
                // Remove the old one.
                ipcp->psDestroy(component, selected_ps_name, ps);
        }

        // Install the new one.
        ps = candidate;
        selected_ps_name = ps_name;
        LOG_INFO("Policy-set %s selected for component %s",
                        ps_name.c_str(), component.c_str());

        return ps ? 0 : -1;
}

int IPCProcessComponent::set_policy_set_param_common(IPCProcess * ipcp,
                                              const std::string& path,
                                              const std::string& param_name,
                                              const std::string& param_value)
{
        LOG_DBG("set_policy_set_param(%s, %s) called",
                param_name.c_str(), param_value.c_str());

        if (!ipcp) {
                LOG_ERR("bug: NULL ipcp reference");
                return -1;
        }

        if (path == selected_ps_name) {
                // This request is for the currently selected
                // policy set, forward to it
                return ps->set_policy_set_param(param_name, param_value);
        } else if (path != std::string()) {
                LOG_ERR("Invalid component address '%s'", path.c_str());
                return -1;
        }

        // This request is for the component itself
        LOG_ERR("No such parameter '%s' exists", param_name.c_str());

        return -1;
}


}
