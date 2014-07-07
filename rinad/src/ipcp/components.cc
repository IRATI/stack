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

#include "components.h"

namespace rinad {

//	CLASS EnrollmentRequest
EnrollmentRequest::EnrollmentRequest(
		const rina::Neighbor& neighbor, const rina::EnrollToDIFRequestEvent& event) {
	neighbor_ = neighbor;
	event_ = event;
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
		rib_daemon_ =  ipc_process->get_rib_daemon();
		encoder_ = ipc_process->get_encoder();
	} else {
		rib_daemon_ = 0;
		encoder_ = 0;
	}
	parent_ = 0;
}

rina::RIBObjectData BaseRIBObject::get_data() {
	rina::RIBObjectData result;
	result.set_class(class_);
    result.set_name(name_);
    result.set_instance(instance_);

    //TODO set displayable_value
    //TODO set value

    return result;
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

void BaseRIBObject::remoteCreateObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	operation_not_supported(cdapMessage, cdapSessionDescriptor);
}

void BaseRIBObject::remoteDeleteObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	operation_not_supported(cdapMessage, cdapSessionDescriptor);
}

void BaseRIBObject::remoteReadObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	operation_not_supported(cdapMessage, cdapSessionDescriptor);
}

void BaseRIBObject::remoteCancelReadObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	operation_not_supported(cdapMessage, cdapSessionDescriptor);
}

void BaseRIBObject::remoteWriteObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	operation_not_supported(cdapMessage, cdapSessionDescriptor);
}

void BaseRIBObject::remoteStartObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	operation_not_supported(cdapMessage, cdapSessionDescriptor);
}

void BaseRIBObject::remoteStopObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	operation_not_supported(cdapMessage, cdapSessionDescriptor);
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

}
