//
// Base RIB Object, RIB and RIB Daemon default implementations
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

#define RINA_PREFIX "rib"

#include <librina/logs.h>
#include "librina/rib.h"
#include <iostream>
#include <algorithm>

namespace rina {

/* Class RIBObjectData*/
RIBObjectData::RIBObjectData(){
        instance_ = 0;
}

RIBObjectData::RIBObjectData(
                std::string clazz, std::string name,
                long long instance){
        this->class_ = clazz;
        this->name_ = name;
        this->instance_ = instance;
}

bool RIBObjectData::operator==(const RIBObjectData &other) const{
        if (class_.compare(other.get_class()) != 0) {
                return false;
        }

        if (name_.compare(other.get_name()) != 0) {
                return false;
        }

        return instance_ == other.get_instance();
}

bool RIBObjectData::operator!=(const RIBObjectData &other) const{
        return !(*this == other);
}

const std::string& RIBObjectData::get_class() const {
        return class_;
}

void RIBObjectData::set_class(const std::string& clazz) {
        class_ = clazz;
}

unsigned long RIBObjectData::get_instance() const {
        return instance_;
}

void RIBObjectData::set_instance(unsigned long instance) {
        instance_ = instance;
}

const std::string& RIBObjectData::get_name() const {
        return name_;
}

void RIBObjectData::set_name(const std::string& name) {
        name_ = name;
}

const std::string& RIBObjectData::get_displayable_value() const {
        return displayable_value_;
}

void RIBObjectData::set_displayable_value(const std::string& displayable_value) {
        displayable_value_ = displayable_value;
}

//Class BaseRIBObject
BaseRIBObject::BaseRIBObject(IRIBDaemon * rib_daemon, const std::string& object_class,
		long object_instance, std::string& object_name) {
        object_name.erase( std::remove_if( object_name.begin(),
        		object_name.end(), ::isspace ), object_name.end() );
        name_ = object_name;
        class_ = object_class;
        instance_ = object_instance;
        base_rib_daemon_ = rib_daemon;
        encoder_ = 0;
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

void BaseRIBObject::remove_child(const std::string& object_name) {
        for (std::list<BaseRIBObject*>::iterator it = children_.begin();
                        it != children_.end(); it++) {
                if ( (*it)->name_.compare(object_name) == 0) {
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
                int invoke_id, CDAPSessionDescriptor * session_descriptor) {
        (void) object_value;
        (void) object_name;
        (void) invoke_id;
        (void) session_descriptor;
        operation_not_supported();
}

void BaseRIBObject::remoteDeleteObject(int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
        (void) invoke_id;
        (void) session_descriptor;
        operation_not_supported();
}

void BaseRIBObject::remoteReadObject(int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
        (void) invoke_id;
        (void) session_descriptor;
        operation_not_supported();
}

void BaseRIBObject::remoteCancelReadObject(int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
        (void) invoke_id;
        (void) session_descriptor;
        operation_not_supported();
}

void BaseRIBObject::remoteWriteObject(void * object_value, int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
        (void) object_value;
        (void) invoke_id;
        (void) session_descriptor;
        operation_not_supported();;
}

void BaseRIBObject::remoteStartObject(void * object_value, int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
        (void) object_value;
        (void) invoke_id;
        (void) session_descriptor;
        operation_not_supported();;
}

void BaseRIBObject::remoteStopObject(void * object_value, int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
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

void BaseRIBObject::operation_not_supported(const CDAPMessage * cdapMessage,
               CDAPSessionDescriptor * cdapSessionDescriptor) {
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

const std::string& BaseRIBObject::get_class() const {
	return class_;
}

const std::string& BaseRIBObject::get_name() const {
	return name_;
}

const std::string& BaseRIBObject::get_parent_name() const {
	return parent_->get_name();
}

long BaseRIBObject::get_instance() const {
	return instance_;
}

void BaseRIBObject::set_parent(BaseRIBObject* parent) {
	parent_ = parent;
}

// CLASS NotificationPolicy
NotificationPolicy::NotificationPolicy(const std::list<int>& cdap_session_ids) {
        cdap_session_ids_ = cdap_session_ids;
}

//Class RemoteProcessId
RemoteProcessId::RemoteProcessId() {
        use_address_ = false;
        port_id_ = 0;
        address_ = 0;
}

// Class RIBObjectValue
RIBObjectValue::RIBObjectValue() {
        type_ = notype;
        bool_value_ = false;
        complex_value_ = 0;
        int_value_ = 0;
        double_value_ = 0;
        long_value_ = 0;
        float_value_ = 0;
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

//Class RIB
RIB::RIB()
{ }

RIB::~RIB() throw()
{ }

BaseRIBObject* RIB::getRIBObject(const std::string& object_class,
                                 const std::string& object_name, bool check)
{
        BaseRIBObject* ribObject;
        std::map<std::string, BaseRIBObject*>::iterator it;

        lock();
        it = rib_.find(object_name);
        unlock();

        if (it == rib_.end()) {
                throw Exception("Could not find object in the RIB");
        }

        ribObject = it->second;
        if (check && ribObject->get_class().compare(object_class) != 0) {
                throw Exception("Object class does not match the user specified one");
        }

        return ribObject;
}

void RIB::addRIBObject(BaseRIBObject* ribObject)
{
        lock();
        if (rib_.find(ribObject->get_name()) != rib_.end()) {
                throw Exception("Object already exists in the RIB");
        }
        std::string parent_name = get_parent_name(ribObject->get_name());
        std::cout<<"Parent name: "<<parent_name<<std::endl;
        if (parent_name != "")
        {
        	if (rib_.find(parent_name) == rib_.end()) {
        		throw Exception("Parent is not in the RIB");
        	}
        	BaseRIBObject *parent = rib_[parent_name];
        	parent->add_child(ribObject);
        	ribObject->set_parent(parent);
        }
        rib_[ribObject->get_name()] = ribObject;
        unlock();
}

BaseRIBObject * RIB::removeRIBObject(const std::string& objectName)
{
        std::map<std::string, BaseRIBObject*>::iterator it;
        BaseRIBObject* ribObject;

        lock();
        it = rib_.find(objectName);
        if (it == rib_.end()) {
                throw Exception("Could not find object in the RIB");
        }

        ribObject = it->second;
        rib_.erase(it);
        unlock();

        return ribObject;
}

std::list<BaseRIBObject*> RIB::getRIBObjects()
{
        std::list<BaseRIBObject*> result;

        for (std::map<std::string, BaseRIBObject*>::iterator it = rib_.begin();
                        it != rib_.end(); ++it) {
                result.push_back(it->second);
        }

        return result;
}
std::string RIB::get_parent_name(const std::string child_name)const {
	unsigned int last_field_separator = child_name.find_last_of(RIBSchema::FIELD_SEPARATOR,
			std::string::npos);
	unsigned int last_id_separator = child_name.find_last_of(RIBSchema::ID_SEPARATOR,
			std::string::npos);
	if (last_field_separator == std::string::npos)
		return "";

	if (last_id_separator < last_field_separator)
		return child_name.substr(0, last_id_separator);
	else
		return child_name.substr(0, last_field_separator);
}

///Class RIBDaemon
RIBDaemon::RIBDaemon(){
        cdap_session_manager_ = 0;
        encoder_ = 0;
        app_conn_handler_ = 0;
}

void RIBDaemon::initialize(const std::string& separator, IEncoder * encoder,
                CDAPSessionManagerInterface * cdap_session_manager,
                IApplicationConnectionHandler * app_conn_handler) {
        cdap_session_manager_ = cdap_session_manager;
        encoder_ = encoder;
        separator_ = separator;
        app_conn_handler_ = app_conn_handler;
}

void RIBDaemon::addRIBObject(BaseRIBObject * ribObject)
{
        if (!ribObject)
                throw Exception("Object is null");

        rib_.addRIBObject(ribObject);
        LOG_INFO("Object with name %s, class %s, instance %ld added to the RIB",
                        ribObject->get_name().c_str(), ribObject->get_class().c_str(),
                        ribObject->get_instance());
}

void RIBDaemon::removeRIBObject(BaseRIBObject * ribObject)
{
        if (!ribObject)
                throw Exception("Object is null");

        removeRIBObject(ribObject->get_name());
}

void RIBDaemon::removeRIBObject(const std::string& objectName)
{
        BaseRIBObject * object = rib_.removeRIBObject(objectName);
        LOG_INFO("Object with name %s, class %s, instance %ld removed from the RIB",
                 object->get_name().c_str(), object->get_class().c_str(),
                 object->get_instance());

        delete object;
}

std::list<BaseRIBObject *> RIBDaemon::getRIBObjects()
{
        return rib_.getRIBObjects();
}

bool RIBDaemon::isOnList(int candidate, std::list<int> list)
{
        for (std::list<int>::iterator it = list.begin(); it != list.end(); ++it) {
                if ((*it) == candidate)
                        return true;
        }

        return false;
}

void RIBDaemon::createObject(const std::string& objectClass,
                             const std::string& objectName,
                             const void* objectValue,
                             const NotificationPolicy * notificationPolicy) {
        BaseRIBObject * ribObject;

        try {
                ribObject = rib_.getRIBObject(objectClass, objectName, true);
        } catch (Exception &e) {
                //Delegate creation to the parent if the object is not there
                std::string::size_type position =
                        objectName.rfind(separator_);
                if (position == std::string::npos)
                        throw e;
                std::string parentObjectName = objectName.substr(0, position);
                ribObject = rib_.getRIBObject(objectClass, parentObjectName, false);
        }

        //Create the object
        ribObject->createObject(objectClass, objectName, objectValue);

        //Notify neighbors if needed
        if (!notificationPolicy) {
                return;
        }

        //We need to notify, find out to whom the notifications must be sent to, and do it
        std::list<int> peersToIgnore = notificationPolicy->cdap_session_ids_;
        std::vector<int> peers;
        cdap_session_manager_->getAllCDAPSessionIds(peers);

        rina::CDAPMessage * cdapMessage = 0;

        for (std::vector<int>::size_type i = 0; i < peers.size(); i++) {
                if (!isOnList(peers[i], peersToIgnore)) {
                        try {
                                cdapMessage = cdap_session_manager_->
                                        getCreateObjectRequestMessage(peers[i], 0,
                                                                      rina::CDAPMessage::NONE_FLAGS,
                                                                      objectClass,
                                                                      0,
                                                                      objectName,
                                                                      0,
                                                                      false);
                                encoder_->encode(objectValue, cdapMessage);
                                sendMessage(*cdapMessage, peers[i], 0);
                        } catch(Exception & e) {
                                LOG_ERR("Problems notifying neighbors: %s", e.what());
                        }

                        delete cdapMessage;
                }
        }
}

void RIBDaemon::deleteObject(const std::string& objectClass,
                             const std::string& objectName,
                             const void* objectValue,
                             const NotificationPolicy * notificationPolicy)
{
        BaseRIBObject * ribObject;

        ribObject = rib_.getRIBObject(objectClass, objectName, true);
        ribObject->deleteObject(objectValue);

        //Notify neighbors if needed
        if (!notificationPolicy)
                return;

        //We need to notify, find out to whom the notifications must be sent to, and do it
        std::list<int> peersToIgnore = notificationPolicy->cdap_session_ids_;
        std::vector<int> peers;
        cdap_session_manager_->getAllCDAPSessionIds(peers);

        const rina::CDAPMessage * cdapMessage = 0;
        for (std::vector<int>::size_type i = 0; i<peers.size(); i++) {
                if (!isOnList(peers[i], peersToIgnore)) {
                        try {
                                cdapMessage =
                                        cdap_session_manager_->
                                        getDeleteObjectRequestMessage(peers[i], 0,
                                                                      rina::CDAPMessage::NONE_FLAGS,
                                                                      objectClass,
                                                                      0,
                                                                      objectName,
                                                                      0, false);
                                sendMessage(*cdapMessage, peers[i], 0);
                                delete cdapMessage;
                        } catch(Exception & e) {
                                LOG_ERR("Problems notifying neighbors: %s", e.what());
                                if (cdapMessage) {
                                        delete cdapMessage;
                                }
                        }
                }
        }
}

BaseRIBObject * RIBDaemon::readObject(const std::string& objectClass,
                                      const std::string& objectName)
{ return rib_.getRIBObject(objectClass, objectName, true); }

void RIBDaemon::writeObject(const std::string& objectClass,
                            const std::string& objectName,
                            const void* objectValue)
{
        BaseRIBObject * object = rib_.getRIBObject(objectClass, objectName, true);
        object->writeObject(objectValue);
}

void RIBDaemon::startObject(const std::string& objectClass,
                            const std::string& objectName,
                            const void* objectValue)
{
        BaseRIBObject * object = rib_.getRIBObject(objectClass, objectName, true);
        object->startObject(objectValue);
}

void RIBDaemon::stopObject(const std::string& objectClass,
                           const std::string& objectName,
                           const void* objectValue)
{
        BaseRIBObject * object = rib_.getRIBObject(objectClass, objectName, true);
        object->stopObject(objectValue);
}

ICDAPResponseMessageHandler * RIBDaemon::getCDAPMessageHandler(const rina::CDAPMessage * cdapMessage)
{
        ICDAPResponseMessageHandler * handler;

        if (!cdapMessage) {
                return 0;
        }

        if (cdapMessage->get_flags() != rina::CDAPMessage::F_RD_INCOMPLETE) {
                handler = handlers_waiting_for_reply_.erase(cdapMessage->get_invoke_id());
        } else {
                handler = handlers_waiting_for_reply_.find(cdapMessage->get_invoke_id());
        }

        return handler;
}

void RIBDaemon::processIncomingRequestMessage(const rina::CDAPMessage * cdapMessage,
                                              rina::CDAPSessionDescriptor * cdapSessionDescriptor)
{
        BaseRIBObject * ribObject = 0;
        void * decodedObject = 0;

        LOG_DBG("Remote operation %d called on object %s", cdapMessage->get_op_code(),
                        cdapMessage->get_obj_name().c_str());
        try {
                switch (cdapMessage->get_op_code()) {
                case rina::CDAPMessage::M_CREATE:
                        decodedObject = encoder_->decode(cdapMessage);

                        // Creation is delegated to the parent objects if the object doesn't exist.
                        // Create semantics are CREATE or UPDATE. If the object exists it is an
                        // update, therefore the message is handled to the object. If the object
                        // doesn't exist it is a CREATE, therefore it is handled to the parent object
                        try {
                                ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
                                                                                        cdapMessage->get_obj_name(), true);
                                ribObject->remoteCreateObject(decodedObject, cdapMessage->obj_name_,
                                                cdapMessage->invoke_id_, cdapSessionDescriptor);
                        } catch (Exception &e) {
                                //Look for parent object, delegate creation there
                                std::string::size_type position =
                                                cdapMessage->get_obj_name().rfind(separator_);
                                if (position == std::string::npos) {
                                        throw e;
                                }
                                std::string parentObjectName = cdapMessage->get_obj_name().substr(0, position);
                                LOG_DBG("Looking for parent object, with name %s", parentObjectName.c_str());
                                ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(), parentObjectName, false);
                                ribObject->remoteCreateObject(decodedObject, cdapMessage->obj_name_,
                                                cdapMessage->invoke_id_, cdapSessionDescriptor);
                        }

                        break;
                case rina::CDAPMessage::M_DELETE:
                        ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
                                                        cdapMessage->get_obj_name(), true);
                        ribObject->remoteDeleteObject(cdapMessage->invoke_id_,
                                        cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_START:
                        if (cdapMessage->obj_value_) {
                                decodedObject = encoder_->decode(cdapMessage);
                        }
                        ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
                                                        cdapMessage->get_obj_name(), true);
                        ribObject->remoteStartObject(decodedObject, cdapMessage->invoke_id_,
                                        cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_STOP:
                        if (cdapMessage->obj_value_) {
                                decodedObject = encoder_->decode(cdapMessage);
                        }
                        ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
                                                        cdapMessage->get_obj_name(), true);
                        ribObject->remoteStopObject(decodedObject, cdapMessage->invoke_id_,
                                        cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_READ:
                        ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
                                                        cdapMessage->get_obj_name(), true);
                        ribObject->remoteReadObject(cdapMessage->invoke_id_,
                                        cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_CANCELREAD:
                        ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
                                                        cdapMessage->get_obj_name(), true);
                        ribObject->remoteCancelReadObject(cdapMessage->invoke_id_,
                                        cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_WRITE:
                        decodedObject = encoder_->decode(cdapMessage);
                        ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
                                                        cdapMessage->get_obj_name(), true);
                        ribObject->remoteWriteObject(decodedObject, cdapMessage->invoke_id_,
                                        cdapSessionDescriptor);
                        break;
                default:
                        LOG_ERR("Invalid operation code for a request message: %d", cdapMessage->get_op_code());
                }
        } catch(Exception &e) {
                LOG_ERR("Problems processing incoming CDAP request message %s", e.what());
        }

        return;
}

void RIBDaemon::processIncomingResponseMessage(const rina::CDAPMessage * cdapMessage,
                                               rina::CDAPSessionDescriptor * cdapSessionDescriptor)
{
        ICDAPResponseMessageHandler * handler = 0;
        void * decodedObject = 0;

        handler = getCDAPMessageHandler(cdapMessage);
        if (!handler) {
                LOG_ERR("Could not find a message handler for invoke-id %d",
                                cdapMessage->get_invoke_id());
                return;
        }

        try {
                switch (cdapMessage->get_op_code()) {
                case rina::CDAPMessage::M_CREATE_R:
                        if (cdapMessage->obj_value_) {
                                decodedObject = encoder_->decode(cdapMessage);
                        }
                        handler->createResponse(cdapMessage->result_, cdapMessage->result_reason_,
                                        decodedObject, cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_DELETE_R:
                        handler->deleteResponse(cdapMessage->result_, cdapMessage->result_reason_,
                                        cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_START_R:
                        if (cdapMessage->obj_value_) {
                                decodedObject = encoder_->decode(cdapMessage);
                        }
                        handler->startResponse(cdapMessage->result_, cdapMessage->result_reason_,
                                        decodedObject, cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_STOP_R:
                        if (cdapMessage->obj_value_) {
                                decodedObject = encoder_->decode(cdapMessage);
                        }
                        handler->stopResponse(cdapMessage->result_, cdapMessage->result_reason_,
                                        decodedObject, cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_READ_R:
                        if (cdapMessage->result_ == 0) {
                                decodedObject = encoder_->decode(cdapMessage);
                        }
                        handler->readResponse(cdapMessage->result_, cdapMessage->result_reason_,
                                        decodedObject, cdapMessage->obj_name_, cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_CANCELREAD_R:
                        handler->cancelReadResponse(cdapMessage->result_, cdapMessage->result_reason_,
                                        cdapSessionDescriptor);
                        break;
                case rina::CDAPMessage::M_WRITE_R:
                        if (cdapMessage->obj_value_) {
                                decodedObject = encoder_->decode(cdapMessage);
                        }
                        handler->writeResponse(cdapMessage->result_, cdapMessage->result_reason_,
                                        decodedObject, cdapSessionDescriptor);
                        break;
                default:
                        LOG_ERR("Invalid operation code for a response message %d", cdapMessage->get_op_code());
                }
        } catch (Exception &e) {
                LOG_ERR("Problems processing CDAP response message: %s", e.what());
        }
}

void RIBDaemon::cdapMessageDelivered(char* message, int length, int portId)
{
        const rina::CDAPMessage * cdapMessage;
        const rina::CDAPSessionInterface * cdapSession;
        rina::CDAPSessionDescriptor  * cdapSessionDescriptor;

        //1 Decode the message and obtain the CDAP session descriptor
        atomic_send_lock_.lock();
        try {
                rina::SerializedObject serializedMessage = rina::SerializedObject(message, length);
                cdapMessage = cdap_session_manager_->messageReceived(serializedMessage, portId);
        } catch (Exception &e) {
                atomic_send_lock_.unlock();
                LOG_ERR("Error decoding CDAP message: %s", e.what());
                return;
        }

        cdapSession = cdap_session_manager_->get_cdap_session(portId);
        if (!cdapSession) {
                atomic_send_lock_.unlock();
                LOG_ERR("Could not find open CDAP session related to portId %d", portId);
                delete cdapMessage;
                return;
        }

        cdapSessionDescriptor = cdapSession->get_session_descriptor();
        LOG_DBG("Received CDAP message through portId %d: %s", portId,
                        cdapMessage->to_string().c_str());
        atomic_send_lock_.unlock();

        //2 Find the message recipient and call it
        rina::CDAPMessage::Opcode opcode = cdapMessage->get_op_code();
        try {
                switch (opcode) {
                case rina::CDAPMessage::M_CONNECT:
                        app_conn_handler_->connect(cdapMessage->invoke_id_, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_CONNECT_R:
                        app_conn_handler_->connectResponse(cdapMessage->result_, cdapMessage->result_reason_,
                                        cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_RELEASE:
                        app_conn_handler_->release(cdapMessage->invoke_id_, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_RELEASE_R:
                        app_conn_handler_->releaseResponse(cdapMessage->result_, cdapMessage->result_reason_,
                                        cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_CREATE:
                        processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_CREATE_R:
                        processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_DELETE:
                        processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_DELETE_R:
                        processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_START:
                        processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_START_R:
                        processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_STOP:
                        processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_STOP_R:
                        processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_READ:
                        processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_READ_R:
                        processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_CANCELREAD:
                        processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_CANCELREAD_R:
                        processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_WRITE:
                        processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                case rina::CDAPMessage::M_WRITE_R:
                        processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
                        delete cdapMessage;
                        break;
                default:
                        LOG_ERR("Unrecognized CDAP operation code: %d", cdapMessage->get_op_code());
                        delete cdapMessage;
                }
        } catch(Exception &e) {
                LOG_ERR("Problems processing incoming CDAP message: %s", e.what());
                delete cdapMessage;
        }
}

void RIBDaemon::sendMessage(const rina::CDAPMessage& cdapMessage, int sessionId,
                            ICDAPResponseMessageHandler * cdapMessageHandler)
{
        sendMessageSpecific(false, cdapMessage, sessionId, 0, cdapMessageHandler);
}

void RIBDaemon::sendMessageToAddress(const rina::CDAPMessage& cdapMessage,
                                     int sessionId,
                                     unsigned int address,
                                     ICDAPResponseMessageHandler * cdapMessageHandler)
{
        sendMessageSpecific(true, cdapMessage, sessionId, address, cdapMessageHandler);
}

void
RIBDaemon::sendMessages(const std::list<const rina::CDAPMessage*>& cdapMessages,
                        const IUpdateStrategy& updateStrategy)
{
        (void) cdapMessages; // Stop compiler barfs
        (void) updateStrategy; // Stop compiler barfs

        //TODO
}

void RIBDaemon::sendMessageToProcess(const rina::CDAPMessage & message, const RemoteProcessId& remote_id,
                ICDAPResponseMessageHandler * response_handler) {
        if (remote_id.use_address_) {
                sendMessageToAddress(message, remote_id.port_id_, remote_id.address_, response_handler);
        } else {
                sendMessage(message, remote_id.port_id_, response_handler);
        }
}

void RIBDaemon::encodeObject(RIBObjectValue& object_value, rina::CDAPMessage * message) {
        switch(object_value.type_) {
        case RIBObjectValue::notype :
                break;
        case RIBObjectValue::complextype : {
                if (object_value.complex_value_) {
                        encoder_->encode(object_value.complex_value_, message);
                }
                break;
        }
        case RIBObjectValue::booltype : {
                rina::BooleanObjectValue * bool_value =
                                new rina::BooleanObjectValue(object_value.bool_value_);
                message->obj_value_ = bool_value;
                break;
        }
        case RIBObjectValue::stringtype : {
                rina::StringObjectValue * string_value =
                                new rina::StringObjectValue(object_value.string_value_);
                message->obj_value_ = string_value;
                break;
        }
        case RIBObjectValue::inttype : {
                rina::IntObjectValue * int_value =
                                new rina::IntObjectValue(object_value.int_value_);
                message->obj_value_ = int_value;
                break;
        }
        case RIBObjectValue::longtype : {
                rina::LongObjectValue * long_value =
                                new rina::LongObjectValue(object_value.long_value_);
                message->obj_value_ = long_value;
                break;
        }
        case RIBObjectValue::floattype : {
                rina::FloatObjectValue * float_value =
                                new rina::FloatObjectValue(object_value.float_value_);
                message->obj_value_ = float_value;
                break;
        }
        case RIBObjectValue::doubletype : {
                rina::DoubleObjectValue * double_value =
                                new rina::DoubleObjectValue(object_value.double_value_);
                message->obj_value_ = double_value;
                break;
        }
        default :
                break;
        }
}

void RIBDaemon::openApplicationConnection(rina::CDAPMessage::AuthTypes auth_mech,
                        const rina::AuthValue &auth_value, const std::string &dest_ae_inst,
                        const std::string &dest_ae_name, const std::string &dest_ap_inst,
                        const std::string &dest_ap_name, const std::string &src_ae_inst,
                        const std::string &src_ae_name, const std::string &src_ap_inst,
                        const std::string &src_ap_name, const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;

        try {
                message = cdap_session_manager_->getOpenConnectionRequestMessage(remote_id.port_id_,
                                auth_mech, auth_value, dest_ae_inst, dest_ae_name, dest_ap_inst,
                                dest_ap_name, src_ae_inst, src_ae_name, src_ap_inst, src_ap_name);

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::closeApplicationConnection(const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) {
        rina::CDAPMessage * message = 0;

        bool invoke_id = false;
        if (response_handler) {
                invoke_id = true;
        }

        try {
                message = cdap_session_manager_->getReleaseConnectionRequestMessage(remote_id.port_id_,
                                rina::CDAPMessage::NONE_FLAGS, invoke_id);

                sendMessageToProcess(*message, remote_id, response_handler);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteCreateObject(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) {
        rina::CDAPMessage * message = 0;

        bool invoke_id = false;
        if (response_handler) {
                invoke_id = true;
        }

        try {
                message = cdap_session_manager_->getCreateObjectRequestMessage(remote_id.port_id_,
                                0, rina::CDAPMessage::NONE_FLAGS, object_class,
                                0, object_name, scope, invoke_id);

                encodeObject(object_value, message);

                sendMessageToProcess(*message, remote_id, response_handler);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteDeleteObject(const std::string& object_class, const std::string& object_name,
                        int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) {
        rina::CDAPMessage * message = 0;

        bool invoke_id = false;
        if (response_handler) {
                invoke_id = true;
        }

        try {
                message = cdap_session_manager_->getDeleteObjectRequestMessage(remote_id.port_id_,
                                0, rina::CDAPMessage::NONE_FLAGS, object_class,
                                0, object_name, scope, invoke_id);

                sendMessageToProcess(*message, remote_id, response_handler);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteReadObject(const std::string& object_class, const std::string& object_name,
                        int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) {
        rina::CDAPMessage * message = 0;

        bool invoke_id = false;
        if (response_handler) {
                invoke_id = true;
        }

        try {
                message = cdap_session_manager_->getReadObjectRequestMessage(remote_id.port_id_,
                                0, rina::CDAPMessage::NONE_FLAGS, object_class, 0, object_name,
                                scope, invoke_id);

                sendMessageToProcess(*message, remote_id, response_handler);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteWriteObject(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) {
        rina::CDAPMessage * message = 0;

        bool invoke_id = false;
        if (response_handler) {
                invoke_id = true;
        }

        try {
                message = cdap_session_manager_->getWriteObjectRequestMessage(remote_id.port_id_,
                                0, rina::CDAPMessage::NONE_FLAGS, object_class,
                                0, object_name, scope, invoke_id);

                encodeObject(object_value, message);

                sendMessageToProcess(*message, remote_id, response_handler);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteStartObject(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) {
        rina::CDAPMessage * message = 0;

        bool invoke_id = false;
        if (response_handler) {
                invoke_id = true;
        }

        try {
                message = cdap_session_manager_->getStartObjectRequestMessage(remote_id.port_id_,
                                0, rina::CDAPMessage::NONE_FLAGS, object_class,
                                0, object_name, scope, invoke_id);

                encodeObject(object_value, message);

                sendMessageToProcess(*message, remote_id, response_handler);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteStopObject(const std::string& object_class, const std::string& object_name,
                RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                ICDAPResponseMessageHandler * response_handler) {
        rina::CDAPMessage * message = 0;

        bool invoke_id = false;
        if (response_handler) {
                invoke_id = true;
        }

        try {
                message = cdap_session_manager_->getStopObjectRequestMessage(remote_id.port_id_,
                                0, rina::CDAPMessage::NONE_FLAGS, object_class,
                                0, object_name, scope, invoke_id);

                encodeObject(object_value, message);

                sendMessageToProcess(*message, remote_id, response_handler);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::openApplicationConnectionResponse(rina::CDAPMessage::AuthTypes auth_mech,
                                const rina::AuthValue &auth_value, const std::string &dest_ae_inst,
                                const std::string &dest_ae_name, const std::string &dest_ap_inst, const std::string &dest_ap_name,
                                int result, const std::string &result_reason, const std::string &src_ae_inst,
                                const std::string &src_ae_name, const std::string &src_ap_inst, const std::string &src_ap_name,
                                int invoke_id, const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;

        try {
                message = cdap_session_manager_->getOpenConnectionResponseMessage(auth_mech,
                                auth_value, dest_ae_inst, dest_ae_name, dest_ap_inst, dest_ap_name,
                                result, result_reason, src_ae_inst, src_ae_name, src_ap_inst,
                                src_ap_name, invoke_id);

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::closeApplicationConnectionResponse(int result, const std::string result_reason,
                        int invoke_id, const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;

        try {
                message = cdap_session_manager_->getReleaseConnectionResponseMessage(rina::CDAPMessage::NONE_FLAGS,
                                result, result_reason, invoke_id);

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteCreateObjectResponse(const std::string& object_class, const std::string& object_name,
                RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;

        try {
                message = cdap_session_manager_->getCreateObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
                                object_class, 0, object_name, result, result_reason, invoke_id);

                encodeObject(object_value, message);

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteDeleteObjectResponse(const std::string& object_class, const std::string& object_name,
                int result, const std::string result_reason, int invoke_id, const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;

        try {
                message = cdap_session_manager_->getDeleteObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
                                object_class, 0, object_name, result, result_reason, invoke_id);

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteReadObjectResponse(const std::string& object_class, const std::string& object_name,
                RIBObjectValue& object_value, int result, const std::string result_reason, bool read_incomplete,
                int invoke_id, const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;
        rina::CDAPMessage::Flags flags;
        if (read_incomplete) {
                flags = rina::CDAPMessage::F_RD_INCOMPLETE;
        } else {
                flags = rina::CDAPMessage::NONE_FLAGS;
        }

        try {
                message = cdap_session_manager_->getReadObjectResponseMessage(flags,
                                object_class, 0, object_name, result, result_reason, invoke_id);

                encodeObject(object_value, message);

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteWriteObjectResponse(const std::string& object_class, const std::string& object_name,
                int result, const std::string result_reason, int invoke_id, const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;

        try {
                (void) object_class;
                (void) object_name;
                message = cdap_session_manager_->getWriteObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
                                result, result_reason, invoke_id);

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteStartObjectResponse(const std::string& object_class, const std::string& object_name,
                RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;

        try {
                message = cdap_session_manager_->getStartObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
                                object_class, 0, object_name, result, result_reason, invoke_id);

                encodeObject(object_value, message);

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteStopObjectResponse(const std::string& object_class, const std::string& object_name,
                RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;

        try {
                (void) object_class;
                (void) object_name;
                message = cdap_session_manager_->getStopObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
                                result, result_reason, invoke_id);

                encodeObject(object_value, message);

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}


// CLASS RIBSchemaObject
RIBSchemaObject::RIBSchemaObject(const std::string& class_name,
		const std::string& name,const bool mandatory,
		const unsigned max_objs) {
	name_ = name;
	class_name_ = class_name;
	mandatory_ = mandatory;
	max_objs_ = max_objs;
}

void RIBSchemaObject::addChild(RIBSchemaObject *object) {
	children_.push_back(object);
}

const std::string& RIBSchemaObject::get_class_name() {
	return class_name_;
}

// CLASS RIBSchema
RIBSchema::RIBSchema(rib_ver_t version){
	version_ = version;
}

rib_res_t RIBSchema::ribSchemaDefContRelation(const std::string& container_cn,
		const std::string& container_name,	const std::string& class_name,
		const std::string name, const bool mandatory,
		const unsigned max_objs){
	RIBSchemaObject *object = new RIBSchemaObject(class_name, name, mandatory, max_objs);
	RIBSchemaObject *parent = rib_schema_[container_name];

	if(parent->get_class_name().compare(container_cn) != 0)
		return RIB_SCHEMA_FORMAT_ERR;

	std::pair<std::map<std::string, RIBSchemaObject*>::iterator,bool> ret =
			rib_schema_.insert(std::pair<std::string, RIBSchemaObject*>(name,
					object));

	if (ret.second)
	{
		return RIB_SUCCESS;
	}
	else
	{
		return RIB_SCHEMA_FORMAT_ERR;
	}
}

bool RIBSchema::validateAddObject(BaseRIBObject* obj){
	std::string name_schema = parseName(obj->get_name());
	RIBSchemaObject *schema_object = rib_schema_[name_schema];
	// CHECKS REGION //
	// Existance
	if (schema_object == 0)
		return false;
	// parent existance
	std::string parent_name_schema = parseName(obj->get_parent_name());
	RIBSchemaObject *parent_schema_object = rib_schema_[parent_name_schema];
	if (parent_schema_object == 0)
		return false;

	return true;
}


std::string RIBSchema::parseName(const std::string& name){
	std::string name_schema = "";
	int position = 0;
	int field_separator_position = name.find(FIELD_SEPARATOR, position);
	while (field_separator_position != -1)
	{
		int id_separator_position = name.find(ID_SEPARATOR, position);
		if (id_separator_position < field_separator_position )
			// field with value
			name_schema.append(name, position, id_separator_position);
		else
			// field without value
			name_schema.append(name, position, field_separator_position);

		field_separator_position = name.find(FIELD_SEPARATOR, position);
	}
	return name_schema;
}

}

