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

	throw Exception("No active connection is currently defined");
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

// CLASS NotificationPolicy
NotificationPolicy::NotificationPolicy(const std::list<int>& cdap_session_ids) {
	cdap_session_ids_ = cdap_session_ids;
}

// Class BaseRIBObject
BaseRIBObject::BaseRIBObject(IPCProcess * ipc_process, const std::string& object_class,
		long object_instance, const std::string& object_name) {
	name_ = object_name;
	class_ = object_class;
	instance_ = object_instance;
	ipc_process_ = ipc_process;
	if (ipc_process) {
		rib_daemon_ =  ipc_process->rib_daemon;
		encoder_ = ipc_process->encoder;
	} else {
		rib_daemon_ = 0;
		encoder_ = 0;
	}
	parent_ = 0;
}

rina::RIBObjectData BaseRIBObject::get_data() {
	rina::RIBObjectData result;
	result.class_ = class_;
    result.name_ = name_;
    result.instance_ = instance_;
    result.displayable_value_ = get_displayable_value();

    return result;
}

std::string BaseRIBObject::get_displayable_value() {
	return "-";
}

const std::list<BaseRIBObject*>& BaseRIBObject::get_children() const {
	return children_;
}

void BaseRIBObject::add_child(BaseRIBObject * child) {
	for (std::list<BaseRIBObject*>::iterator it = children_.begin();
			it != children_.end(); it++) {
		if ((*it)->name_.compare(child->name_) == 0) {
			throw Exception("Object is already a child");
		}
	}

	children_.push_back(child);
	child->parent_ = this;
}

void BaseRIBObject::remove_child(const std::string& objectName) {
	for (std::list<BaseRIBObject*>::iterator it = children_.begin();
			it != children_.end(); it++) {
		if ( (*it)->name_.compare(objectName) == 0) {
			children_.erase(it);
			return;
		}
	}

	throw Exception("Unknown child object");
}

void BaseRIBObject::createObject(const std::string& objectClass, const std::string& objectName,
		const void* objectValue) {
	operartion_not_supported(objectClass, objectName, objectValue);
}

void BaseRIBObject::deleteObject(const void* objectValue) {
	operation_not_supported(objectValue);
}

BaseRIBObject * BaseRIBObject::readObject() {
	return this;
}

void BaseRIBObject::writeObject(const void* object_value) {
	operation_not_supported(object_value);
}

void BaseRIBObject::startObject(const void* object) {
	operation_not_supported(object);
}

void BaseRIBObject::stopObject(const void* object) {
	operation_not_supported(object);
}

void BaseRIBObject::remoteCreateObject(void * object_value, const std::string& object_name,
		int invoke_id, rina::CDAPSessionDescriptor * session_descriptor) {
	(void) object_value;
	(void) object_name;
	(void) invoke_id;
	(void) session_descriptor;
	operation_not_supported();
}

void BaseRIBObject::remoteDeleteObject(int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	(void) invoke_id;
	(void) session_descriptor;
	operation_not_supported();
}

void BaseRIBObject::remoteReadObject(int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	(void) invoke_id;
	(void) session_descriptor;
	operation_not_supported();
}

void BaseRIBObject::remoteCancelReadObject(int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	(void) invoke_id;
	(void) session_descriptor;
	operation_not_supported();
}

void BaseRIBObject::remoteWriteObject(void * object_value, int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	(void) object_value;
	(void) invoke_id;
	(void) session_descriptor;
	operation_not_supported();;
}

void BaseRIBObject::remoteStartObject(void * object_value, int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	(void) object_value;
	(void) invoke_id;
	(void) session_descriptor;
	operation_not_supported();;
}

void BaseRIBObject::remoteStopObject(void * object_value, int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	(void) object_value;
	(void) invoke_id;
	(void) session_descriptor;
	operation_not_supported();;
}

void BaseRIBObject::operation_not_supported() {
	throw Exception("Operation not supported");
}

void BaseRIBObject::operation_not_supported(const void* object) {
	std::stringstream ss;
	ss<<"Operation not allowed. Data: "<<std::endl;
	ss<<"Object value memory @: "<<object;

	throw Exception(ss.str().c_str());
}

void BaseRIBObject::operation_not_supported(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	std::stringstream ss;
	ss<<"Operation not allowed. Data: "<<std::endl;
	ss<<"CDAP Message code: "<<cdapMessage->get_op_code();
	ss<<" N-1 port-id: "<<cdapSessionDescriptor->get_port_id()<<std::endl;

	throw Exception(ss.str().c_str());
}

void BaseRIBObject::operartion_not_supported(const std::string& objectClass,
		const std::string& objectName, const void* objectValue) {
	std::stringstream ss;
	ss<<"Operation not allowed. Data: "<<std::endl;
	ss<<"Class: "<<objectClass<<"; Name: "<<objectName;
	ss<<"; Value memory @: "<<objectValue;

	throw Exception(ss.str().c_str());
}


///Class RemoteIPCProcessId
RemoteIPCProcessId::RemoteIPCProcessId() {
	use_address_ = false;
	port_id_ = 0;
	address_ = 0;
}

/// Class RIBObjectValue
RIBObjectValue::RIBObjectValue() {
	type_ = notype;
	bool_value_ = false;
	complex_value_ = 0;
	int_value_ = 0;
	double_value_ = 0;
	long_value_ = 0;
	float_value_ = 0;
}

// CLASS IPC Process
const std::string IPCProcess::MANAGEMENT_AE = "Management";
const std::string IPCProcess::DATA_TRANSFER_AE = "Data Transfer";
const int IPCProcess::DEFAULT_MAX_SDU_SIZE_IN_BYTES = 10000;

// Class ObjectInstanceGenerator
ObjectInstanceGenerator::ObjectInstanceGenerator() : rina::Lockable() {
	instance_ = 0;
}

long ObjectInstanceGenerator::getObjectInstance() {
	long result = 0;

	lock();
	instance_++;
	result = instance_;
	unlock();

	return result;
}

Singleton<ObjectInstanceGenerator> objectInstanceGenerator;

//Class SimpleRIBObject
SimpleRIBObject::SimpleRIBObject(IPCProcess* ipc_process, const std::string& object_class,
			const std::string& object_name, const void* object_value) :
					BaseRIBObject(ipc_process, object_class,
							objectInstanceGenerator->getObjectInstance(), object_name) {
	object_value_ = object_value;
}

const void* SimpleRIBObject::get_value() const {
	return object_value_;
}

void SimpleRIBObject::writeObject(const void* object_value) {
	object_value_ = object_value;
}

void SimpleRIBObject::createObject(const std::string& objectClass, const std::string& objectName,
		const void* objectValue) {
	if (objectName.compare("") != 0 && objectClass.compare("") != 0) {
		object_value_ = objectValue;
	}
}

//Class SimpleSetRIBObject
SimpleSetRIBObject::SimpleSetRIBObject(IPCProcess * ipc_process, const std::string& object_class,
		const std::string& set_member_object_class, const std::string& object_name) :
					SimpleRIBObject(ipc_process, object_class, object_name, 0){
	set_member_object_class_ = set_member_object_class;
}

void SimpleSetRIBObject::createObject(const std::string& objectClass, const std::string& objectName,
		const void* objectValue) {
	if (set_member_object_class_.compare(objectClass) != 0) {
		throw Exception("Class of set member does not match the expected value");
	}

	SimpleSetMemberRIBObject * ribObject = new SimpleSetMemberRIBObject(ipc_process_, objectClass,
			objectName, objectValue);
	add_child(ribObject);
	rib_daemon_->addRIBObject(ribObject);
}

//Class SimpleSetMemberRIBObject
SimpleSetMemberRIBObject::SimpleSetMemberRIBObject(IPCProcess* ipc_process,
                                                   const std::string& object_class,
                                                   const std::string& object_name,
                                                   const void* object_value) :
        SimpleRIBObject(ipc_process, object_class, object_name, object_value)
{
}

void SimpleSetMemberRIBObject::deleteObject(const void* objectValue)
{
        (void) objectValue; // Stop compiler barfs

	parent_->remove_child(name_);
	rib_daemon_->removeRIBObject(name_);
}

//Class IPCProcess
IPCProcess::IPCProcess()
{
	delimiter = 0;
	encoder = 0;
	cdap_session_manager = 0;
	enrollment_task = 0;
	flow_allocator = 0;
	namespace_manager = 0;
	resource_allocator = 0;
	security_manager = 0;
	rib_daemon = 0;
}

//Class SecurityManager
SecurityManager::SecurityManager() {
	ipcp = 0;
        ps = 0;
}

void SecurityManager::set_ipc_process(IPCProcess * ipc_process)
{
	ipcp = ipc_process;
}

void SecurityManager::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	LOG_DBG("Set dif configuration: %u", dif_configuration.address_);
}

int SecurityManager::select_policy_set(const std::string& name)
{
        IPolicySet *candidate = NULL;

        if (!ipcp) {
                LOG_ERR("bug: NULL ipcp reference");
                return -1;
        }

        if (name == selected_ps_name) {
                LOG_INFO("policy set %s already selected", name.c_str());
                return 0;
        }

        candidate = ipcp->componentFactoryCreate("security-manager", name, this);
        if (!candidate) {
                LOG_ERR("failed to allocate instance of policy set %s");
                return -1;
        }

        if (ps) {
                // Remove the old one.
                ipcp->componentFactoryDestroy("security-manager", selected_ps_name, ps);
        }

        // Install the new one.
        ps = dynamic_cast<ISecurityManagerPs *>(candidate);
        selected_ps_name = name;
        LOG_INFO("Policy-set %s selected", name.c_str());

        return ps ? 0 : -1;
}

}
