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

#define RINA_PREFIX "librina.rib"

#include <librina/logs.h>
#include "librina/rib.h"

namespace rina {

const std::string RIBNamingConstants::DAF = "daf";
const std::string RIBNamingConstants::DIF_REGISTRATIONS = "difregistrations";
const std::string RIBNamingConstants::IRM = "irm";
const std::string RIBNamingConstants::MANAGEMENT = "management";
const std::string RIBNamingConstants::N_MINUS_ONE_FLOWS = "nminusoneflows";
const std::string RIBNamingConstants::NEIGHBORS = "neighbors";
const std::string RIBNamingConstants::SEPARATOR = "/";

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
                long object_instance, const std::string& object_name) {
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
                int invoke_id, CDAPSessionDescriptor * session_descriptor) {
        operation_not_supported();
}

void BaseRIBObject::remoteDeleteObject(int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
        operation_not_supported();
}

void BaseRIBObject::remoteReadObject(int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
        operation_not_supported();
}

void BaseRIBObject::remoteCancelReadObject(int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
        operation_not_supported();
}

void BaseRIBObject::remoteWriteObject(void * object_value, int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
        operation_not_supported();;
}

void BaseRIBObject::remoteStartObject(void * object_value, int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
        operation_not_supported();;
}

void BaseRIBObject::remoteStopObject(void * object_value, int invoke_id,
                CDAPSessionDescriptor * session_descriptor) {
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

//Class SimpleRIBObject
SimpleRIBObject::SimpleRIBObject(IRIBDaemon * rib_daemon, const std::string& object_class,
                        const std::string& object_name, const void* object_value) :
                                        BaseRIBObject(rib_daemon, object_class,
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
SimpleSetRIBObject::SimpleSetRIBObject(IRIBDaemon * rib_daemon, const std::string& object_class,
                const std::string& set_member_object_class, const std::string& object_name) :
                                        SimpleRIBObject(rib_daemon, object_class, object_name, 0){
        set_member_object_class_ = set_member_object_class;
}

void SimpleSetRIBObject::createObject(const std::string& objectClass, const std::string& objectName,
                const void* objectValue) {
        if (set_member_object_class_.compare(objectClass) != 0) {
                throw Exception("Class of set member does not match the expected value");
        }

        SimpleSetMemberRIBObject * ribObject = new SimpleSetMemberRIBObject(base_rib_daemon_,
                        objectClass, objectName, objectValue);
        add_child(ribObject);
        base_rib_daemon_->addRIBObject(ribObject);
}

//Class SimpleSetMemberRIBObject
SimpleSetMemberRIBObject::SimpleSetMemberRIBObject(IRIBDaemon * rib_daemon,
                const std::string& object_class, const std::string& object_name,
                const void* object_value) : SimpleRIBObject(rib_daemon,
                                object_class, object_name, object_value)
{
}

void SimpleSetMemberRIBObject::deleteObject(const void* objectValue)
{
        parent_->remove_child(name_);
        base_rib_daemon_->removeRIBObject(name_);
}

//Class RIB
RIB::RIB()
{ }

RIB::~RIB() throw()
{ }

BaseRIBObject* RIB::getRIBObject(const std::string& objectClass,
                                 const std::string& objectName, bool check)
{
        BaseRIBObject* ribObject;
        std::map<std::string, BaseRIBObject*>::iterator it;

        lock();
        it = rib_.find(objectName);
        unlock();

        if (it == rib_.end()) {
                throw Exception("Could not find object in the RIB");
        }

        ribObject = it->second;
        if (check && ribObject->class_.compare(objectClass) != 0) {
                throw Exception("Object class does not match the user specified one");
        }

        return ribObject;
}

void RIB::addRIBObject(BaseRIBObject* ribObject)
{
        lock();
        if (rib_.find(ribObject->name_) != rib_.end()) {
                throw Exception("Object already exists in the RIB");
        }
        rib_[ribObject->name_] = ribObject;
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

///Class RIBDaemon
RIBDaemon::RIBDaemon(){
        cdap_session_manager_ = 0;
        encoder_ = 0;
        cacep_handler_ = 0;
}

void RIBDaemon::initialize(const std::string& separator, IEncoder * encoder,
                CDAPSessionManagerInterface * cdap_session_manager,
                CACEPHandler * cacep_handler) {
	if (!cdap_session_manager) {
		LOG_ERR("Bogus CDAP Session Manager passed");
		throw Exception("Bogus CDAP Session Manager passed");
	}
        cdap_session_manager_ = cdap_session_manager;

        if (!encoder) {
        	LOG_ERR("Bogus Encoder passed");
        	throw Exception("Bogus Encoder passed");
        }
        encoder_ = encoder;

        if (!cacep_handler) {
        	LOG_ERR("Bogus CACEP Handler passed");
        	throw Exception("Bogus CACEP Handler passed");
        }
        cacep_handler_ = cacep_handler;

        separator_ = separator;
}

void RIBDaemon::addRIBObject(BaseRIBObject * ribObject)
{
        if (!ribObject)
                throw Exception("Object is null");

        rib_.addRIBObject(ribObject);
        LOG_INFO("Object with name %s, class %s, instance %ld added to the RIB",
                        ribObject->name_.c_str(), ribObject->class_.c_str(),
                        ribObject->instance_);
}

void RIBDaemon::removeRIBObject(BaseRIBObject * ribObject)
{
        if (!ribObject)
                throw Exception("Object is null");

        removeRIBObject(ribObject->name_);
}

void RIBDaemon::removeRIBObject(const std::string& objectName)
{
        BaseRIBObject * object = rib_.removeRIBObject(objectName);
        LOG_INFO("Object with name %s, class %s, instance %ld removed from the RIB",
                 object->name_.c_str(), object->class_.c_str(),
                 object->instance_);

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
        //Copy objectClass and objectName since they may be deleted when
        //deleting the object (depending who calls deleteObject)
        std::string oClass = std::string(objectClass);
        std::string oName = std::string(objectName);

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
                                                                      oClass,
                                                                      0,
                                                                      oName,
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

void RIBDaemon::processIncomingCDAPMessage(const rina::CDAPMessage * cdapMessage,
					   rina::CDAPSessionDescriptor * descriptor,
		        		   const std::string& session_state)
{
	rina::CDAPMessage::Opcode opcode = cdapMessage->get_op_code();

	if (session_state == rina::CDAPSessionInterface::SESSION_STATE_AWAIT_CON) {
		if (opcode == rina::CDAPMessage::M_CONNECT) {
			cacep_handler_->connect(*cdapMessage,
			                        descriptor);
		} else {
			//These must be authentication messages, delegate to CACEPHandler
			//who will delegate to the authentication policy
			cacep_handler_->process_authentication_message(*cdapMessage,
								       descriptor);
		}

		return;
	}

        try {
                switch (opcode) {
                case rina::CDAPMessage::M_CONNECT_R:
                	cacep_handler_->connectResponse(cdapMessage->result_,
                	                		cdapMessage->result_reason_,
                	                		descriptor);
                	break;
                case rina::CDAPMessage::M_RELEASE:
                	cacep_handler_->release(cdapMessage->invoke_id_,
                				descriptor);
                        break;
                case rina::CDAPMessage::M_RELEASE_R:
                	cacep_handler_->releaseResponse(cdapMessage->result_,
                					cdapMessage->result_reason_,
                					descriptor);
                	break;
                case rina::CDAPMessage::M_CREATE:
                        processIncomingRequestMessage(cdapMessage,
                        			      descriptor);
                        break;
                case rina::CDAPMessage::M_CREATE_R:
                        processIncomingResponseMessage(cdapMessage,
                        			       descriptor);
                        break;
                case rina::CDAPMessage::M_DELETE:
                        processIncomingRequestMessage(cdapMessage,
                        			      descriptor);
                        break;
                case rina::CDAPMessage::M_DELETE_R:
                        processIncomingResponseMessage(cdapMessage,
                        			       descriptor);
                        break;
                case rina::CDAPMessage::M_START:
                        processIncomingRequestMessage(cdapMessage,
                        			      descriptor);
                        break;
                case rina::CDAPMessage::M_START_R:
                        processIncomingResponseMessage(cdapMessage,
                        			       descriptor);
                        break;
                case rina::CDAPMessage::M_STOP:
                        processIncomingRequestMessage(cdapMessage,
                        			      descriptor);
                        break;
                case rina::CDAPMessage::M_STOP_R:
                        processIncomingResponseMessage(cdapMessage,
                        			       descriptor);
                        break;
                case rina::CDAPMessage::M_READ:
                        processIncomingRequestMessage(cdapMessage,
                        			      descriptor);
                        break;
                case rina::CDAPMessage::M_READ_R:
                        processIncomingResponseMessage(cdapMessage,
                        			       descriptor);
                        break;
                case rina::CDAPMessage::M_CANCELREAD:
                        processIncomingRequestMessage(cdapMessage,
                        			      descriptor);
                        break;
                case rina::CDAPMessage::M_CANCELREAD_R:
                        processIncomingResponseMessage(cdapMessage,
                        			       descriptor);
                        break;
                case rina::CDAPMessage::M_WRITE:
                        processIncomingRequestMessage(cdapMessage,
                        			      descriptor);
                        break;
                case rina::CDAPMessage::M_WRITE_R:
                        processIncomingResponseMessage(cdapMessage,
                        		               descriptor);
                        break;
                default:
                        LOG_ERR("Unrecognized CDAP operation code: %d",
                        	 cdapMessage->get_op_code());
                }
        } catch(Exception &e) {
                LOG_ERR("Problems processing incoming CDAP message: %s",
                	 e.what());
        }
}

void RIBDaemon::sendMessage(const rina::CDAPMessage& cdapMessage, int sessionId,
                            ICDAPResponseMessageHandler * cdapMessageHandler)
{
        sendMessageSpecific(false, cdapMessage, sessionId, 0, cdapMessageHandler);
}

void RIBDaemon::sendAData(const rina::CDAPMessage& cdapMessage,
                                     unsigned int address,
                                     ICDAPResponseMessageHandler * cdapMessageHandler)
{
        sendMessageSpecific(true, cdapMessage, 0, address, cdapMessageHandler);
}

void
RIBDaemon::sendMessages(const std::list<const rina::CDAPMessage*>& cdapMessages,
                        const IUpdateStrategy& updateStrategy)
{
        //TODO
}

void RIBDaemon::sendMessageToProcess(const rina::CDAPMessage & message, const RemoteProcessId& remote_id,
                ICDAPResponseMessageHandler * response_handler) {
        if (remote_id.use_address_) {
                sendAData(message, remote_id.address_, response_handler);
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
        case RIBObjectValue::bytestype : {
        	rina::ByteArrayObjectValue * bytes_value =
        			new rina::ByteArrayObjectValue(object_value.bytes_value_);
        	message->obj_value_ = bytes_value;
        	break;
        }
        default :
                break;
        }
}

void RIBDaemon::openApplicationConnection(
                        const rina::AuthPolicy &auth_policy, const std::string &dest_ae_inst,
                        const std::string &dest_ae_name, const std::string &dest_ap_inst,
                        const std::string &dest_ap_name, const std::string &src_ae_inst,
                        const std::string &src_ae_name, const std::string &src_ap_inst,
                        const std::string &src_ap_name, const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;

        try {
                message = cdap_session_manager_->getOpenConnectionRequestMessage(remote_id.port_id_,
                                auth_policy, dest_ae_inst, dest_ae_name, dest_ap_inst,
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

void RIBDaemon::assign_invoke_id_if_needed(CDAPMessage * message, bool invoke_id)
{
	if (invoke_id) {
		message->invoke_id_ =
			cdap_session_manager_->get_invoke_id_manager()->newInvokeId(true);
	}
}

void RIBDaemon::remote_operation_on_object(rina::CDAPMessage::Opcode opcode, const std::string& object_class,
		const std::string& object_name, int scope, const RemoteProcessId& remote_id,
		ICDAPResponseMessageHandler * response_handler)
{
	rina::CDAPMessage * message = 0;

	bool invoke_id = false;
	if (response_handler) {
		invoke_id = true;
	}

	try {
		if (remote_id.use_address_) {
			message = rina::CDAPMessage::getRequestMessage(opcode, 0,
					rina::CDAPMessage::NONE_FLAGS, object_class,
					0, object_name, scope);
			assign_invoke_id_if_needed(message, invoke_id);
		} else {
			message = cdap_session_manager_->getRequestMessage(remote_id.port_id_, opcode,
					0, rina::CDAPMessage::NONE_FLAGS, object_class,
					0, object_name, scope, invoke_id);
		}

		sendMessageToProcess(*message, remote_id, response_handler);
	} catch (Exception &e) {
		delete message;
		throw e;
	}

	delete message;
}

void RIBDaemon::remote_operation_on_object_with_value(rina::CDAPMessage::Opcode opcode, const std::string& object_class,
		const std::string& object_name, RIBObjectValue& object_value, int scope,
		const RemoteProcessId& remote_id, ICDAPResponseMessageHandler * response_handler)
{
	rina::CDAPMessage * message = 0;

	bool invoke_id = false;
	if (response_handler) {
		invoke_id = true;
	}

	try {
		if (remote_id.use_address_) {
			message = rina::CDAPMessage::getRequestMessage(opcode, 0,
					rina::CDAPMessage::NONE_FLAGS, object_class,
					0, object_name, scope);
			assign_invoke_id_if_needed(message, invoke_id);
		} else {
			message = cdap_session_manager_->getRequestMessage(remote_id.port_id_, opcode,
					0, rina::CDAPMessage::NONE_FLAGS, object_class,
					0, object_name, scope, invoke_id);
		}

		encodeObject(object_value, message);

		sendMessageToProcess(*message, remote_id, response_handler);
	} catch (Exception &e) {
		delete message;
		throw e;
	}

	delete message;
}

void RIBDaemon::remoteCreateObject(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler)
{
	remote_operation_on_object_with_value(rina::CDAPMessage::M_CREATE, object_class, object_name,
			object_value, scope, remote_id, response_handler);
}

void RIBDaemon::remoteDeleteObject(const std::string& object_class, const std::string& object_name,
                        int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler)
{
	remote_operation_on_object(rina::CDAPMessage::M_DELETE, object_class, object_name,
			scope, remote_id, response_handler);
}

void RIBDaemon::remoteReadObject(const std::string& object_class, const std::string& object_name,
                        int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler)
{
	remote_operation_on_object(rina::CDAPMessage::M_READ, object_class, object_name,
			scope, remote_id, response_handler);
}

void RIBDaemon::remoteWriteObject(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler)
{
	remote_operation_on_object_with_value(rina::CDAPMessage::M_WRITE, object_class, object_name,
			object_value, scope, remote_id, response_handler);
}

void RIBDaemon::remoteStartObject(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler)
{
	remote_operation_on_object_with_value(rina::CDAPMessage::M_START, object_class, object_name,
			object_value, scope, remote_id, response_handler);
}

void RIBDaemon::remoteStopObject(const std::string& object_class, const std::string& object_name,
                RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                ICDAPResponseMessageHandler * response_handler)
{
	remote_operation_on_object_with_value(rina::CDAPMessage::M_STOP, object_class, object_name,
			object_value, scope, remote_id, response_handler);;
}

void RIBDaemon::openApplicationConnectionResponse(
                                const rina::AuthPolicy &auth_policy, const std::string &dest_ae_inst,
                                const std::string &dest_ae_name, const std::string &dest_ap_inst, const std::string &dest_ap_name,
                                int result, const std::string &result_reason, const std::string &src_ae_inst,
                                const std::string &src_ae_name, const std::string &src_ap_inst, const std::string &src_ap_name,
                                int invoke_id, const RemoteProcessId& remote_id) {
        rina::CDAPMessage * message = 0;

        try {
                message = cdap_session_manager_->getOpenConnectionResponseMessage(auth_policy,
                                dest_ae_inst, dest_ae_name, dest_ap_inst, dest_ap_name,
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

void RIBDaemon::remote_operation_response_with_value(rina::CDAPMessage::Opcode opcode,
        		const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int result, const std::string result_reason,
                        int invoke_id, const RemoteProcessId& remote_id,
                        rina::CDAPMessage::Flags flags)
{
        rina::CDAPMessage * message = 0;

        try {
        	if (remote_id.use_address_) {
        		message = CDAPMessage::getResponseMessage(opcode, flags,
        				object_class, 0,object_name, result,
        				result_reason, invoke_id);

        	} else {
        		message = cdap_session_manager_->getResponseMessage(opcode, flags,
        				object_class, 0, object_name, result, result_reason,
        				invoke_id);
        	}

                encodeObject(object_value, message);

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remote_operation_response(rina::CDAPMessage::Opcode opcode,
        		const std::string& object_class, const std::string& object_name,
                        int result, const std::string result_reason,
                        int invoke_id, const RemoteProcessId& remote_id)
{
        rina::CDAPMessage * message = 0;

        try {
        	if (remote_id.use_address_) {
        		message = CDAPMessage::getResponseMessage(opcode,
        				rina::CDAPMessage::NONE_FLAGS, object_class, 0,object_name,
        				result, result_reason, invoke_id);

        	} else {
        		message = cdap_session_manager_->getResponseMessage(opcode,
        				rina::CDAPMessage::NONE_FLAGS, object_class, 0, object_name, result,
        				result_reason, invoke_id);
        	}

                sendMessageToProcess(*message, remote_id, 0);
        } catch (Exception &e) {
                delete message;
                throw e;
        }

        delete message;
}

void RIBDaemon::remoteCreateObjectResponse(const std::string& object_class, const std::string& object_name,
                RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                const RemoteProcessId& remote_id)
{
	remote_operation_response_with_value(rina::CDAPMessage::M_CREATE_R, object_class,
			object_name, object_value, result, result_reason, invoke_id,
			remote_id, rina::CDAPMessage::NONE_FLAGS);
}

void RIBDaemon::remoteDeleteObjectResponse(const std::string& object_class, const std::string& object_name,
                int result, const std::string result_reason, int invoke_id, const RemoteProcessId& remote_id)
{
	remote_operation_response(rina::CDAPMessage::M_DELETE_R, object_class, object_name,
			result, result_reason, invoke_id, remote_id);
}

void RIBDaemon::remoteReadObjectResponse(const std::string& object_class, const std::string& object_name,
                RIBObjectValue& object_value, int result, const std::string result_reason, bool read_incomplete,
                int invoke_id, const RemoteProcessId& remote_id)
{
        rina::CDAPMessage::Flags flags;
        if (read_incomplete) {
                flags = rina::CDAPMessage::F_RD_INCOMPLETE;
        } else {
                flags = rina::CDAPMessage::NONE_FLAGS;
        }

	remote_operation_response_with_value(rina::CDAPMessage::M_READ_R, object_class,
			object_name, object_value, result, result_reason, invoke_id,
			remote_id, flags);
}

void RIBDaemon::remoteWriteObjectResponse(const std::string& object_class, const std::string& object_name,
                int result, const std::string result_reason, int invoke_id, const RemoteProcessId& remote_id)
{
	remote_operation_response(rina::CDAPMessage::M_WRITE_R, object_class, object_name,
			result, result_reason, invoke_id, remote_id);
}

void RIBDaemon::remoteStartObjectResponse(const std::string& object_class, const std::string& object_name,
                RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                const RemoteProcessId& remote_id)
{
	remote_operation_response_with_value(rina::CDAPMessage::M_START_R, object_class,
			object_name, object_value, result, result_reason, invoke_id,
			remote_id, rina::CDAPMessage::NONE_FLAGS);
}

void RIBDaemon::remoteStopObjectResponse(const std::string& object_class, const std::string& object_name,
                RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                const RemoteProcessId& remote_id)
{
	remote_operation_response_with_value(rina::CDAPMessage::M_STOP_R, object_class,
			object_name, object_value, result, result_reason, invoke_id,
			remote_id, rina::CDAPMessage::NONE_FLAGS);
}

/// Class ADataObject
const std::string ADataObject::A_DATA_OBJECT_CLASS = "a_data";
const std::string ADataObject::A_DATA = "a_data";
const std::string ADataObject::A_DATA_OBJECT_NAME = A_DATA;

ADataObject::ADataObject() {
	source_address_ = 0;
	dest_address_ = 0;
	encoded_cdap_message_ = 0;
}

ADataObject::ADataObject(unsigned int source_address,
		unsigned int dest_address) {
	source_address_ = source_address;
	dest_address_ = dest_address;
	encoded_cdap_message_ = 0;
}

ADataObject::~ADataObject() {
	if (encoded_cdap_message_) {
		delete encoded_cdap_message_;
		encoded_cdap_message_ = 0;
	}
}

}

