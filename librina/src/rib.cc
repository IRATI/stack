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
RIBObjectData::RIBObjectData()
{
  instance_ = 0;
}

RIBObjectData::RIBObjectData(std::string clazz, std::string name,
                             long long instance)
{
  this->class_ = clazz;
  this->name_ = name;
  this->instance_ = instance;
}

bool RIBObjectData::operator==(const RIBObjectData &other) const
{
  if (class_.compare(other.get_class()) != 0) {
    return false;
  }

  if (name_.compare(other.get_name()) != 0) {
    return false;
  }

  return instance_ == other.get_instance();
}

bool RIBObjectData::operator!=(const RIBObjectData &other) const
{
  return !(*this == other);
}

const std::string& RIBObjectData::get_class() const
{
  return class_;
}

void RIBObjectData::set_class(const std::string& clazz)
{
  class_ = clazz;
}

unsigned long RIBObjectData::get_instance() const
{
  return instance_;
}

void RIBObjectData::set_instance(unsigned long instance)
{
  instance_ = instance;
}

const std::string& RIBObjectData::get_name() const
{
  return name_;
}

void RIBObjectData::set_name(const std::string& name)
{
  name_ = name;
}

const std::string& RIBObjectData::get_displayable_value() const
{
  return displayable_value_;
}

void RIBObjectData::set_displayable_value(const std::string& displayable_value)
{
  displayable_value_ = displayable_value;
}

//Class BaseRIBObject
BaseRIBObject::BaseRIBObject(IRIBDaemon * rib_daemon,
                             const std::string& object_class,
                             long object_instance, std::string object_name)
{
  object_name.erase(
      std::remove_if(object_name.begin(), object_name.end(), ::isspace),
      object_name.end());
  name_ = object_name;
  class_ = object_class;
  instance_ = object_instance;
  base_rib_daemon_ = rib_daemon;
  parent_ = 0;
}

BaseRIBObject::~BaseRIBObject()
{
  /*
  // FIXME: Remove this when objects have two parents
  while (children_.size() > 0) {
    base_rib_daemon_->removeRIBObject(children_.front()->get_name());
  }
  */
  LOG_DBG("Object %s destroyed", name_.c_str());
}

rina::RIBObjectData BaseRIBObject::get_data()
{
  rina::RIBObjectData result;
  result.class_ = class_;
  result.name_ = name_;
  result.instance_ = instance_;
  result.displayable_value_ = get_displayable_value();

  return result;
}

std::string BaseRIBObject::get_displayable_value()
{
  return "-";
}

void BaseRIBObject::get_children_value(
    std::list<std::pair<std::string, const void *> > &values) const
{
  values.clear();
  for (std::list<BaseRIBObject*>::const_iterator it = children_.begin();
      it != children_.end(); ++it) {
    values.push_back(
        std::pair<std::string, const void *>((*it)->name_, (*it)->get_value()));
  }
}

unsigned int BaseRIBObject::get_children_size() const
{
  return children_.size();
}

void BaseRIBObject::add_child(BaseRIBObject *child)
{
  for (std::list<BaseRIBObject*>::iterator it = children_.begin();
      it != children_.end(); it++) {
    if ((*it)->name_.compare(child->name_) == 0) {
      throw Exception("Object is already a child");
    }
  }

  children_.push_back(child);
  child->parent_ = this;
}

void BaseRIBObject::remove_child(const std::string& object_name)
{
  for (std::list<BaseRIBObject*>::iterator it = children_.begin();
      it != children_.end(); it++) {
    if ((*it)->name_.compare(object_name) == 0) {
      children_.erase(it);
      return;
    }
  }

  std::stringstream ss;
  ss << "Unknown child object with object name: " << object_name.c_str()
     << std::endl;
  throw Exception(ss.str().c_str());
}

void BaseRIBObject::createObject(const std::string& objectClass,
                                 const std::string& objectName,
                                 const void* objectValue)
{
  operartion_not_supported(objectClass, objectName, objectValue);
}

void BaseRIBObject::deleteObject(const void* objectValue)
{
  operation_not_supported(objectValue);
}

BaseRIBObject * BaseRIBObject::readObject()
{
  return this;
}

void BaseRIBObject::writeObject(const void* object_value)
{
  operation_not_supported(object_value);
}

void BaseRIBObject::startObject(const void* object)
{
  operation_not_supported(object);
}

void BaseRIBObject::stopObject(const void* object)
{
  operation_not_supported(object);
}

void BaseRIBObject::remoteCreateObject(
    void * object_value, const std::string& object_name, int invoke_id,
    CDAPSessionDescriptor * session_descriptor)
{
  (void) object_value;
  (void) object_name;
  (void) invoke_id;
  (void) session_descriptor;
  operation_not_supported();
}

void BaseRIBObject::remoteDeleteObject(
    int invoke_id, CDAPSessionDescriptor * session_descriptor)
{
  (void) invoke_id;
  (void) session_descriptor;
  operation_not_supported();
}

void BaseRIBObject::remoteReadObject(int invoke_id,
                                     CDAPSessionDescriptor * session_descriptor)
{
  (void) invoke_id;
  (void) session_descriptor;
  operation_not_supported();
}

void BaseRIBObject::remoteCancelReadObject(
    int invoke_id, CDAPSessionDescriptor * session_descriptor)
{
  (void) invoke_id;
  (void) session_descriptor;
  operation_not_supported();
}

void BaseRIBObject::remoteWriteObject(
    void * object_value, int invoke_id,
    CDAPSessionDescriptor * session_descriptor)
{
  (void) object_value;
  (void) invoke_id;
  (void) session_descriptor;
  operation_not_supported();
  ;
}

void BaseRIBObject::remoteStartObject(
    void * object_value, int invoke_id,
    CDAPSessionDescriptor * session_descriptor)
{
  (void) object_value;
  (void) invoke_id;
  (void) session_descriptor;
  operation_not_supported();
  ;
}

void BaseRIBObject::remoteStopObject(void * object_value, int invoke_id,
                                     CDAPSessionDescriptor * session_descriptor)
{
  (void) object_value;
  (void) invoke_id;
  (void) session_descriptor;
  operation_not_supported();
  ;
}

void BaseRIBObject::operation_not_supported()
{
  throw Exception("Operation not supported");
}

void BaseRIBObject::operation_not_supported(const void* object)
{
  std::stringstream ss;
  ss << "Operation not allowed. Data: " << std::endl;
  ss << "Object value memory @: " << object;

  throw Exception(ss.str().c_str());
}

void BaseRIBObject::operation_not_supported(
    const CDAPMessage * cdapMessage,
    CDAPSessionDescriptor * cdapSessionDescriptor)
{
  std::stringstream ss;
  ss << "Operation not allowed. Data: " << std::endl;
  ss << "CDAP Message code: " << cdapMessage->get_op_code();
  ss << " N-1 port-id: " << cdapSessionDescriptor->get_port_id() << std::endl;

  throw Exception(ss.str().c_str());
}

void BaseRIBObject::operartion_not_supported(const std::string& objectClass,
                                             const std::string& objectName,
                                             const void* objectValue)
{
  std::stringstream ss;
  ss << "Operation not allowed. Data: " << std::endl;
  ss << "Class: " << objectClass << "; Name: " << objectName;
  ss << "; Value memory @: " << objectValue;

  throw Exception(ss.str().c_str());
}

const std::string& BaseRIBObject::get_class() const
{
  return class_;
}

const std::string& BaseRIBObject::get_name() const
{
  return name_;
}

const std::string& BaseRIBObject::get_parent_name() const
{
  return parent_->get_name();
}

const std::string& BaseRIBObject::get_parent_class() const
{
  return parent_->get_class();
}

long BaseRIBObject::get_instance() const
{
  return instance_;
}

void BaseRIBObject::set_parent(BaseRIBObject* parent)
{
  parent_ = parent;
}

// CLASS NotificationPolicy
NotificationPolicy::NotificationPolicy(const std::list<int>& cdap_session_ids)
{
  cdap_session_ids_ = cdap_session_ids;
}

//Class RemoteProcessId
RemoteProcessId::RemoteProcessId()
{
  use_address_ = false;
  port_id_ = 0;
  address_ = 0;
}

// Class RIBObjectValue
RIBObjectValue::RIBObjectValue()
{
  type_ = notype;
  bool_value_ = false;
  complex_value_ = 0;
  int_value_ = 0;
  double_value_ = 0;
  long_value_ = 0;
  float_value_ = 0;
}

// Class ObjectInstanceGenerator
ObjectInstanceGenerator::ObjectInstanceGenerator()
    : rina::Lockable()
{
  instance_ = 0;
}

long ObjectInstanceGenerator::getObjectInstance()
{
  long result = 0;

  lock();
  instance_++;
  result = instance_;
  unlock();

  return result;
}

Singleton<ObjectInstanceGenerator> objectInstanceGenerator;

//Class RIB
RIB::RIB(const RIBSchema *schema)
{
  rib_schema_ = schema;
}

RIB::~RIB() throw ()
{
  lock();
  delete rib_schema_;

  for (std::map<std::string, BaseRIBObject*>::iterator it =
      rib_by_name_.begin(); it != rib_by_name_.end(); ++it) {
    LOG_DBG("Object %s removed from the RIB", it->second->get_name().c_str());
    delete it->second;
  }
  rib_by_name_.clear();
  rib_by_instance_.clear();
  unlock();
}

BaseRIBObject* RIB::getRIBObject(const std::string& object_class,
                                 const std::string& object_name, bool check)
{
  std::string norm_object_name = object_name;
  norm_object_name.erase(
      std::remove_if(norm_object_name.begin(), norm_object_name.end(), ::isspace),
      norm_object_name.end());

  BaseRIBObject* ribObject;
  std::map<std::string, BaseRIBObject*>::iterator it;

  lock();
  it = rib_by_name_.find(norm_object_name);
  unlock();
  if (it == rib_by_name_.end()) {
    std::stringstream ss;
    ss<<"Could not find object "<< norm_object_name<<" of class "<<
        object_class<<" in the RIB"<<std::endl;
    throw Exception(ss.str().c_str());
  }

  ribObject = it->second;
  if (check && ribObject->get_class().compare(object_class) != 0) {
    throw Exception("Object class does not match the user specified one");
  }

  return ribObject;
}

BaseRIBObject* RIB::getRIBObject(const std::string& object_class, long instance,
                                 bool check)
{
  BaseRIBObject* ribObject;
  std::map<long, BaseRIBObject*>::iterator it;

  lock();
  it = rib_by_instance_.find(instance);
  unlock();

  if (it == rib_by_instance_.end()) {
    std::stringstream ss;
    ss<<"Could not find object instance "<< instance<<" of class "<<
        object_class.c_str() <<" in the RIB"<<std::endl;
    throw Exception(ss.str().c_str());
  }

  ribObject = it->second;
  if (check && ribObject->get_class().compare(object_class) != 0) {
    throw Exception("Object class does not match the user "
                    "specified one");
  }

  return ribObject;
}

void RIB::addRIBObject(BaseRIBObject* rib_object)
{
  BaseRIBObject *parent = 0;
  std::string parent_name = get_parent_name(rib_object->get_name());
  if (!parent_name.empty()) {
    lock();
    if (rib_by_name_.find(parent_name) == rib_by_name_.end()) {
      std::stringstream ss;
      ss << "Exception in object "<<rib_object->get_name() <<". Parent name ("
          << parent_name << ") is not in the RIB"<< std::endl;
      throw Exception(ss.str().c_str());
    }
    parent = rib_by_name_[parent_name];
    unlock();
  }
  // TODO: add schema validation
//	if (rib_schema_->validateAddObject(rib_object, parent))
//	{
  lock();
  if (rib_by_name_.find(rib_object->get_name()) != rib_by_name_.end()) {
    unlock();
    std::stringstream ss;
    ss << "Object with the same name (" << rib_object->get_name()
       << ") already exists in the RIB"<< std::endl;
    throw Exception(ss.str().c_str());
  }
  if (rib_by_instance_.find(rib_object->get_instance())
      != rib_by_instance_.end()) {
    unlock();
    std::stringstream ss;
    ss << "Object with the same instance (" << rib_object->get_instance()
       << "already exists "
       "in the RIB"
       << std::endl;
    throw Exception(ss.str().c_str());
  }
  if (parent != 0) {
    parent->add_child(rib_object);
    rib_object->set_parent(parent);
  }
  rib_by_name_[rib_object->get_name()] = rib_object;
  rib_by_instance_[rib_object->get_instance()] = rib_object;
  unlock();
}

BaseRIBObject* RIB::removeRIBObject(const std::string& objectName)
{
  std::map<std::string, BaseRIBObject*>::iterator it;
  BaseRIBObject* rib_object;

  lock();
  it = rib_by_name_.find(objectName);
  if (it == rib_by_name_.end()) {
    unlock();
    throw Exception("Could not find object in the RIB");
  }

  rib_object = it->second;

  BaseRIBObject* parent = rib_object->parent_;
  parent->remove_child(objectName);
  rib_by_name_.erase(it);
  rib_by_instance_.erase(rib_object->instance_);
  unlock();
  return rib_object;
}

BaseRIBObject * RIB::removeRIBObject(long instance)
{
  std::map<long, BaseRIBObject*>::iterator it;
  BaseRIBObject* ribObject;

  lock();
  it = rib_by_instance_.find(instance);
  if (it == rib_by_instance_.end()) {
    unlock();
    throw Exception("Could not find object in the RIB");
  }

  ribObject = it->second;
  rib_by_instance_.erase(it);
  unlock();

  return ribObject;
}

std::list<RIBObjectData> RIB::getRIBObjectsData()
{
  std::list<RIBObjectData> result;

  lock();
  for (std::map<std::string, BaseRIBObject*>::iterator it =
      rib_by_name_.begin(); it != rib_by_name_.end(); ++it) {
    result.push_back(it->second->get_data());
  }
  unlock();
  return result;
}

char RIB::get_name_separator() const
{
  return rib_schema_->get_field_separator();
}

std::string RIB::get_parent_name(const std::string child_name) const
{
  size_t last_field_separator = child_name.find_last_of(
      rib_schema_->field_separator_, std::string::npos);
  size_t last_id_separator = child_name.find_last_of(
      rib_schema_->id_separator_, std::string::npos);
  if (last_field_separator == std::string::npos)
    return "";

  if (last_id_separator < last_field_separator)
    return child_name.substr(0, last_id_separator);
  else
    return child_name.substr(0, last_field_separator);

}

///Class RIBDaemon
RIBDaemon::RIBDaemon(const RIBSchema *rib_schema)
{
  cdap_session_manager_ = 0;
  encoder_ = 0;
  app_conn_handler_ = 0;
  rib_ = new RIB(rib_schema);
}

RIBDaemon::~RIBDaemon()
{
  delete rib_;
}

void RIBDaemon::initialize(ManagerEncoderInterface *encoder,
                           CDAPSessionManagerInterface * cdap_session_manager,
                           IApplicationConnectionHandler * app_conn_handler)
{
  cdap_session_manager_ = cdap_session_manager;
  encoder_ = encoder;
  app_conn_handler_ = app_conn_handler;
}

void RIBDaemon::addRIBObject(BaseRIBObject * ribObject)
{
  if (!ribObject)
    throw Exception("Object is null");

  try{
  rib_->addRIBObject(ribObject);
  }catch(Exception &e){
    LOG_ERR("Error while adding initial rib objects for rib v1. Error: %s", e.what());
    throw e;
  }
  LOG_INFO(
      "Object with name %s, class %s, instance %ld added to the RIB",
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
  BaseRIBObject * object = rib_->removeRIBObject(objectName);
  LOG_INFO(
      "Object with name %s, class %s, instance %ld removed from the RIB",
      object->get_name().c_str(), object->get_class().c_str(), object->get_instance());

  delete object;
}

std::list<RIBObjectData> RIBDaemon::getRIBObjectsData() const
{
  return rib_->getRIBObjectsData();
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
                             const NotificationPolicy * notificationPolicy)
{
  BaseRIBObject * ribObject;

  try {
    ribObject = rib_->getRIBObject(objectClass, objectName, true);
  } catch (Exception &e) {
    //Delegate creation to the parent if the object is not there
    /*std::string::size_type position = objectName.rfind(
        rib_->get_name_separator());
    if (position == std::string::npos)
      throw e;*/
    //std::string parentObjectName = objectName.substr(0, position);
    std::string parentObjectName = rib_->get_parent_name(objectName);
    try{
      ribObject = rib_->getRIBObject(objectClass, parentObjectName, false);
    }catch(Exception &e){
      LOG_ERR("Can not create the object in the RIB: %s", e.what());
      return;
    }
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
        cdapMessage = cdap_session_manager_->getCreateObjectRequestMessage(
            peers[i], 0, rina::CDAPMessage::NONE_FLAGS, objectClass, 0,
            objectName, 0, false);
        encoder_->encode(objectValue, cdapMessage);
        RemoteProcessId remote_proc;
        remote_proc.port_id_ = peers[i];
        sendMessageSpecific(remote_proc, *cdapMessage, 0);
      } catch (Exception & e) {
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

  ribObject = rib_->getRIBObject(objectClass, objectName, true);
  ribObject->deleteObject(objectValue);

  //Notify neighbors if needed
  if (!notificationPolicy)
    return;

  //We need to notify, find out to whom the notifications must be sent to, and do it
  std::list<int> peersToIgnore = notificationPolicy->cdap_session_ids_;
  std::vector<int> peers;
  cdap_session_manager_->getAllCDAPSessionIds(peers);

  const rina::CDAPMessage * cdapMessage = 0;
  for (std::vector<int>::size_type i = 0; i < peers.size(); i++) {
    if (!isOnList(peers[i], peersToIgnore)) {
      try {
        cdapMessage = cdap_session_manager_->getDeleteObjectRequestMessage(
            peers[i], 0, rina::CDAPMessage::NONE_FLAGS, objectClass, 0,
            objectName, 0, false);
        RemoteProcessId remote_proc;
        remote_proc.port_id_ = peers[i];
        sendMessageSpecific(remote_proc, *cdapMessage, 0);
        delete cdapMessage;
      } catch (Exception & e) {
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
{
  return rib_->getRIBObject(objectClass, objectName, true);
}

BaseRIBObject * RIBDaemon::readObject(const std::string& objectClass,
                                      long object_instance)
{
  return rib_->getRIBObject(objectClass, object_instance, true);
}

void RIBDaemon::writeObject(const std::string& objectClass,
                            const std::string& objectName,
                            const void* objectValue)
{
  BaseRIBObject * object = rib_->getRIBObject(objectClass, objectName, true);
  object->writeObject(objectValue);
}

void RIBDaemon::startObject(const std::string& objectClass,
                            const std::string& objectName,
                            const void* objectValue)
{
  BaseRIBObject * object = rib_->getRIBObject(objectClass, objectName, true);
  object->startObject(objectValue);
}

void RIBDaemon::stopObject(const std::string& objectClass,
                           const std::string& objectName,
                           const void* objectValue)
{
  BaseRIBObject * object = rib_->getRIBObject(objectClass, objectName, true);
  object->stopObject(objectValue);
}

ICDAPResponseMessageHandler * RIBDaemon::getCDAPMessageHandler(
    const rina::CDAPMessage * cdapMessage)
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

void RIBDaemon::processIncomingRequestMessage(
    const rina::CDAPMessage * cdapMessage,
    rina::CDAPSessionDescriptor * cdapSessionDescriptor)
{
  BaseRIBObject * ribObject = 0;
  void * decodedObject = 0;

  LOG_DBG("Remote operation %d called on object %s",
          cdapMessage->get_op_code(), cdapMessage->get_obj_name().c_str());
  try {
    switch (cdapMessage->get_op_code()) {
      case rina::CDAPMessage::M_CREATE:
        decodedObject = encoder_->decode(cdapMessage);

        // Creation is delegated to the parent objects if the object doesn't
        //exist.
        // Create semantics are CREATE or UPDATE. If the object exists it is an
        // update, therefore the message is handled to the object. If the object
        // doesn't exist it is a CREATE, therefore it is handled to the parent
        //object
        try {
          ribObject = rib_->getRIBObject(cdapMessage->get_obj_class(),
                                         cdapMessage->get_obj_name(), true);
          ribObject->remoteCreateObject(decodedObject, cdapMessage->obj_name_,
                                        cdapMessage->invoke_id_,
                                        cdapSessionDescriptor);
        } catch (Exception &e) {
          //Look for parent object, delegate creation there
          std::string::size_type position = cdapMessage->get_obj_name().rfind(
              rib_->get_name_separator());
          if (position == std::string::npos) {
            throw e;
          }
          std::string parentObjectName = cdapMessage->get_obj_name().substr(
              0, position);
          LOG_DBG("Looking for parent object, with name %s",
                  parentObjectName.c_str());
          ribObject = rib_->getRIBObject(cdapMessage->get_obj_class(),
                                         parentObjectName, false);
          ribObject->remoteCreateObject(decodedObject, cdapMessage->obj_name_,
                                        cdapMessage->invoke_id_,
                                        cdapSessionDescriptor);
        }

        break;
      case rina::CDAPMessage::M_DELETE:
        ribObject = rib_->getRIBObject(cdapMessage->get_obj_class(),
                                       cdapMessage->get_obj_name(), true);
        ribObject->remoteDeleteObject(cdapMessage->invoke_id_,
                                      cdapSessionDescriptor);
        break;
      case rina::CDAPMessage::M_START:
        if (cdapMessage->obj_value_) {
          decodedObject = encoder_->decode(cdapMessage);
        }
        ribObject = rib_->getRIBObject(cdapMessage->get_obj_class(),
                                       cdapMessage->get_obj_name(), true);
        ribObject->remoteStartObject(decodedObject, cdapMessage->invoke_id_,
                                     cdapSessionDescriptor);
        break;
      case rina::CDAPMessage::M_STOP:
        if (cdapMessage->obj_value_) {
          decodedObject = encoder_->decode(cdapMessage);
        }
        ribObject = rib_->getRIBObject(cdapMessage->get_obj_class(),
                                       cdapMessage->get_obj_name(), true);
        ribObject->remoteStopObject(decodedObject, cdapMessage->invoke_id_,
                                    cdapSessionDescriptor);
        break;
      case rina::CDAPMessage::M_READ:
        ribObject = rib_->getRIBObject(cdapMessage->get_obj_class(),
                                       cdapMessage->get_obj_name(), true);
        ribObject->remoteReadObject(cdapMessage->invoke_id_,
                                    cdapSessionDescriptor);
        break;
      case rina::CDAPMessage::M_CANCELREAD:
        ribObject = rib_->getRIBObject(cdapMessage->get_obj_class(),
                                       cdapMessage->get_obj_name(), true);
        ribObject->remoteCancelReadObject(cdapMessage->invoke_id_,
                                          cdapSessionDescriptor);
        break;
      case rina::CDAPMessage::M_WRITE:
        decodedObject = encoder_->decode(cdapMessage);
        ribObject = rib_->getRIBObject(cdapMessage->get_obj_class(),
                                       cdapMessage->get_obj_name(), true);
        ribObject->remoteWriteObject(decodedObject, cdapMessage->invoke_id_,
                                     cdapSessionDescriptor);
        break;
      default:
        LOG_ERR("Invalid operation code for a request message: %d",
                cdapMessage->get_op_code());
    }
  } catch (Exception &e) {
    LOG_ERR("Problems processing incoming CDAP request message %s", e.what());
  }

  return;
}

void RIBDaemon::processIncomingResponseMessage(
    const rina::CDAPMessage * cdapMessage,
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
        handler->createResponse(cdapMessage->result_,
                                cdapMessage->result_reason_, decodedObject,
                                cdapSessionDescriptor);
        break;
      case rina::CDAPMessage::M_DELETE_R:
        handler->deleteResponse(cdapMessage->result_,
                                cdapMessage->result_reason_,
                                cdapSessionDescriptor);
        break;
      case rina::CDAPMessage::M_START_R:
        if (cdapMessage->obj_value_) {
          decodedObject = encoder_->decode(cdapMessage);
        }
        handler->startResponse(cdapMessage->result_,
                               cdapMessage->result_reason_, decodedObject,
                               cdapSessionDescriptor);
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
                              decodedObject, cdapMessage->obj_name_,
                              cdapSessionDescriptor);
        break;
      case rina::CDAPMessage::M_CANCELREAD_R:
        handler->cancelReadResponse(cdapMessage->result_,
                                    cdapMessage->result_reason_,
                                    cdapSessionDescriptor);
        break;
      case rina::CDAPMessage::M_WRITE_R:
        if (cdapMessage->obj_value_) {
          decodedObject = encoder_->decode(cdapMessage);
        }
        handler->writeResponse(cdapMessage->result_,
                               cdapMessage->result_reason_, decodedObject,
                               cdapSessionDescriptor);
        break;
      default:
        LOG_ERR("Invalid operation code for a response message %d",
                cdapMessage->get_op_code());
    }
  } catch (Exception &e) {
    LOG_ERR("Problems processing CDAP response message: %s", e.what());
  }
}

void RIBDaemon::cdapMessageDelivered(char* message, int length, int portId)
{
  const rina::CDAPMessage * cdapMessage;
  const rina::CDAPSessionInterface * cdapSession;
  rina::CDAPSessionDescriptor * cdapSessionDescriptor;

  //1 Decode the message and obtain the CDAP session descriptor
  atomic_send_lock_.lock();
  try {
    rina::SerializedObject serializedMessage = rina::SerializedObject(message,
                                                                      length);
    cdapMessage = cdap_session_manager_->messageReceived(serializedMessage,
                                                         portId);
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
  LOG_DBG("Received CDAP message through portId %d: %s",
          portId, cdapMessage->to_string().c_str());
  atomic_send_lock_.unlock();

  //2 Find the message recipient and call it
  rina::CDAPMessage::Opcode opcode = cdapMessage->get_op_code();
  try {
    switch (opcode) {
      case rina::CDAPMessage::M_CONNECT:
        app_conn_handler_->connect(cdapMessage->invoke_id_,
                                   cdapSessionDescriptor);
        delete cdapMessage;
        break;
      case rina::CDAPMessage::M_CONNECT_R:
        app_conn_handler_->connectResponse(cdapMessage->result_,
                                           cdapMessage->result_reason_,
                                           cdapSessionDescriptor);
        delete cdapMessage;
        break;
      case rina::CDAPMessage::M_RELEASE:
        app_conn_handler_->release(cdapMessage->invoke_id_,
                                   cdapSessionDescriptor);
        delete cdapMessage;
        break;
      case rina::CDAPMessage::M_RELEASE_R:
        app_conn_handler_->releaseResponse(cdapMessage->result_,
                                           cdapMessage->result_reason_,
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
        LOG_ERR("Unrecognized CDAP operation code: %d",
                cdapMessage->get_op_code());
        delete cdapMessage;
    }
  } catch (Exception &e) {
    LOG_ERR("Problems processing incoming CDAP message: %s", e.what());
    delete cdapMessage;
  }
}

void RIBDaemon::sendMessages(
    const std::list<const rina::CDAPMessage*>& cdapMessages,
    const IUpdateStrategy& updateStrategy)
{
  (void) cdapMessages;  // Stop compiler barfs
  (void) updateStrategy;  // Stop compiler barfs

  //TODO
}

void RIBDaemon::sendMessageToProcess(
    const rina::CDAPMessage & message, const RemoteProcessId& remote_proc,
    ICDAPResponseMessageHandler * response_handler)
{

  sendMessageSpecific(remote_proc, message, response_handler);
}

void RIBDaemon::encodeObject(RIBObjectValue& object_value,
                             rina::CDAPMessage * message)
{
  switch (object_value.type_) {
    case RIBObjectValue::notype:
      break;
    case RIBObjectValue::complextype: {
      if (object_value.complex_value_) {
        encoder_->encode(object_value.complex_value_, message);
      }
      break;
    }
    case RIBObjectValue::booltype: {
      rina::BooleanObjectValue * bool_value = new rina::BooleanObjectValue(
          object_value.bool_value_);
      message->obj_value_ = bool_value;
      break;
    }
    case RIBObjectValue::stringtype: {
      rina::StringObjectValue * string_value = new rina::StringObjectValue(
          object_value.string_value_);
      message->obj_value_ = string_value;
      break;
    }
    case RIBObjectValue::inttype: {
      rina::IntObjectValue * int_value = new rina::IntObjectValue(
          object_value.int_value_);
      message->obj_value_ = int_value;
      break;
    }
    case RIBObjectValue::longtype: {
      rina::LongObjectValue * long_value = new rina::LongObjectValue(
          object_value.long_value_);
      message->obj_value_ = long_value;
      break;
    }
    case RIBObjectValue::floattype: {
      rina::FloatObjectValue * float_value = new rina::FloatObjectValue(
          object_value.float_value_);
      message->obj_value_ = float_value;
      break;
    }
    case RIBObjectValue::doubletype: {
      rina::DoubleObjectValue * double_value = new rina::DoubleObjectValue(
          object_value.double_value_);
      message->obj_value_ = double_value;
      break;
    }
    default:
      break;
  }
}

void RIBDaemon::openApplicationConnection(
    rina::CDAPMessage::AuthTypes auth_mech, const rina::AuthValue &auth_value,
    const std::string &dest_ae_inst, const std::string &dest_ae_name,
    const std::string &dest_ap_inst, const std::string &dest_ap_name,
    const std::string &src_ae_inst, const std::string &src_ae_name,
    const std::string &src_ap_inst, const std::string &src_ap_name,
    const RemoteProcessId& remote_id)
{
  rina::CDAPMessage * message = 0;

  try {
    message = cdap_session_manager_->getOpenConnectionRequestMessage(
        remote_id.port_id_, auth_mech, auth_value, dest_ae_inst, dest_ae_name,
        dest_ap_inst, dest_ap_name, src_ae_inst, src_ae_name, src_ap_inst,
        src_ap_name);

    sendMessageToProcess(*message, remote_id, 0);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::closeApplicationConnection(
    const RemoteProcessId& remote_id,
    ICDAPResponseMessageHandler * response_handler)
{
  rina::CDAPMessage * message = 0;

  bool invoke_id = false;
  if (response_handler) {
    invoke_id = true;
  }

  try {
    message = cdap_session_manager_->getReleaseConnectionRequestMessage(
        remote_id.port_id_, rina::CDAPMessage::NONE_FLAGS, invoke_id);

    sendMessageToProcess(*message, remote_id, response_handler);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteCreateObject(
    const std::string& object_class, const std::string& object_name,
    RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
    ICDAPResponseMessageHandler * response_handler)
{
  rina::CDAPMessage * message = 0;

  bool invoke_id = false;
  if (response_handler) {
    invoke_id = true;
  }

  try {
    message = cdap_session_manager_->getCreateObjectRequestMessage(
        remote_id.port_id_, 0, rina::CDAPMessage::NONE_FLAGS, object_class, 0,
        object_name, scope, invoke_id);

    encodeObject(object_value, message);

    sendMessageToProcess(*message, remote_id, response_handler);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteDeleteObject(
    const std::string& object_class, const std::string& object_name, int scope,
    const RemoteProcessId& remote_id,
    ICDAPResponseMessageHandler * response_handler)
{
  rina::CDAPMessage * message = 0;

  bool invoke_id = false;
  if (response_handler) {
    invoke_id = true;
  }

  try {
    message = cdap_session_manager_->getDeleteObjectRequestMessage(
        remote_id.port_id_, 0, rina::CDAPMessage::NONE_FLAGS, object_class, 0,
        object_name, scope, invoke_id);

    sendMessageToProcess(*message, remote_id, response_handler);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteReadObject(const std::string& object_class,
                                 const std::string& object_name, int scope,
                                 const RemoteProcessId& remote_id,
                                 ICDAPResponseMessageHandler * response_handler)
{
  rina::CDAPMessage * message = 0;

  bool invoke_id = false;
  if (response_handler) {
    invoke_id = true;
  }

  try {
    message = cdap_session_manager_->getReadObjectRequestMessage(
        remote_id.port_id_, 0, rina::CDAPMessage::NONE_FLAGS, object_class, 0,
        object_name, scope, invoke_id);

    sendMessageToProcess(*message, remote_id, response_handler);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteWriteObject(
    const std::string& object_class, const std::string& object_name,
    RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
    ICDAPResponseMessageHandler * response_handler)
{
  rina::CDAPMessage * message = 0;

  bool invoke_id = false;
  if (response_handler) {
    invoke_id = true;
  }

  try {
    message = cdap_session_manager_->getWriteObjectRequestMessage(
        remote_id.port_id_, 0, rina::CDAPMessage::NONE_FLAGS, object_class, 0,
        object_name, scope, invoke_id);

    encodeObject(object_value, message);

    sendMessageToProcess(*message, remote_id, response_handler);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteStartObject(
    const std::string& object_class, const std::string& object_name,
    RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
    ICDAPResponseMessageHandler * response_handler)
{
  rina::CDAPMessage * message = 0;

  bool invoke_id = false;
  if (response_handler) {
    invoke_id = true;
  }

  try {
    message = cdap_session_manager_->getStartObjectRequestMessage(
        remote_id.port_id_, 0, rina::CDAPMessage::NONE_FLAGS, object_class, 0,
        object_name, scope, invoke_id);

    encodeObject(object_value, message);

    sendMessageToProcess(*message, remote_id, response_handler);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteStopObject(const std::string& object_class,
                                 const std::string& object_name,
                                 RIBObjectValue& object_value, int scope,
                                 const RemoteProcessId& remote_id,
                                 ICDAPResponseMessageHandler * response_handler)
{
  rina::CDAPMessage * message = 0;

  bool invoke_id = false;
  if (response_handler) {
    invoke_id = true;
  }

  try {
    message = cdap_session_manager_->getStopObjectRequestMessage(
        remote_id.port_id_, 0, rina::CDAPMessage::NONE_FLAGS, object_class, 0,
        object_name, scope, invoke_id);

    encodeObject(object_value, message);

    sendMessageToProcess(*message, remote_id, response_handler);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::openApplicationConnectionResponse(
    rina::CDAPMessage::AuthTypes auth_mech, const rina::AuthValue &auth_value,
    const std::string &dest_ae_inst, const std::string &dest_ae_name,
    const std::string &dest_ap_inst, const std::string &dest_ap_name,
    int result, const std::string &result_reason,
    const std::string &src_ae_inst, const std::string &src_ae_name,
    const std::string &src_ap_inst, const std::string &src_ap_name,
    int invoke_id, const RemoteProcessId& remote_id)
{
  rina::CDAPMessage * message = 0;

  try {
    message = cdap_session_manager_->getOpenConnectionResponseMessage(
        auth_mech, auth_value, dest_ae_inst, dest_ae_name, dest_ap_inst,
        dest_ap_name, result, result_reason, src_ae_inst, src_ae_name,
        src_ap_inst, src_ap_name, invoke_id);

    sendMessageToProcess(*message, remote_id, 0);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::closeApplicationConnectionResponse(
    int result, const std::string result_reason, int invoke_id,
    const RemoteProcessId& remote_id)
{
  rina::CDAPMessage * message = 0;

  try {
    message = cdap_session_manager_->getReleaseConnectionResponseMessage(
        rina::CDAPMessage::NONE_FLAGS, result, result_reason, invoke_id);

    sendMessageToProcess(*message, remote_id, 0);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteCreateObjectResponse(const std::string& object_class,
                                           const std::string& object_name,
                                           RIBObjectValue& object_value,
                                           int result,
                                           const std::string result_reason,
                                           int invoke_id,
                                           const RemoteProcessId& remote_id)
{
  rina::CDAPMessage * message = 0;

  try {
    message = cdap_session_manager_->getCreateObjectResponseMessage(
        rina::CDAPMessage::NONE_FLAGS, object_class, 0, object_name, result,
        result_reason, invoke_id);

    encodeObject(object_value, message);

    sendMessageToProcess(*message, remote_id, 0);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteDeleteObjectResponse(const std::string& object_class,
                                           const std::string& object_name,
                                           int result,
                                           const std::string result_reason,
                                           int invoke_id,
                                           const RemoteProcessId& remote_id)
{
  rina::CDAPMessage * message = 0;

  try {
    message = cdap_session_manager_->getDeleteObjectResponseMessage(
        rina::CDAPMessage::NONE_FLAGS, object_class, 0, object_name, result,
        result_reason, invoke_id);

    sendMessageToProcess(*message, remote_id, 0);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteReadObjectResponse(const std::string& object_class,
                                         const std::string& object_name,
                                         RIBObjectValue& object_value,
                                         int result,
                                         const std::string result_reason,
                                         bool read_incomplete, int invoke_id,
                                         const RemoteProcessId& remote_id)
{
  rina::CDAPMessage * message = 0;
  rina::CDAPMessage::Flags flags;
  if (read_incomplete) {
    flags = rina::CDAPMessage::F_RD_INCOMPLETE;
  } else {
    flags = rina::CDAPMessage::NONE_FLAGS;
  }

  try {
    message = cdap_session_manager_->getReadObjectResponseMessage(flags,
                                                                  object_class,
                                                                  0,
                                                                  object_name,
                                                                  result,
                                                                  result_reason,
                                                                  invoke_id);

    encodeObject(object_value, message);

    sendMessageToProcess(*message, remote_id, 0);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteWriteObjectResponse(const std::string& object_class,
                                          const std::string& object_name,
                                          int result,
                                          const std::string result_reason,
                                          int invoke_id,
                                          const RemoteProcessId& remote_id)
{
  rina::CDAPMessage * message = 0;

  try {
    (void) object_class;
    (void) object_name;
    message = cdap_session_manager_->getWriteObjectResponseMessage(
        rina::CDAPMessage::NONE_FLAGS, result, result_reason, invoke_id);

    sendMessageToProcess(*message, remote_id, 0);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteStartObjectResponse(const std::string& object_class,
                                          const std::string& object_name,
                                          RIBObjectValue& object_value,
                                          int result,
                                          const std::string result_reason,
                                          int invoke_id,
                                          const RemoteProcessId& remote_id)
{
  rina::CDAPMessage * message = 0;

  try {
    message = cdap_session_manager_->getStartObjectResponseMessage(
        rina::CDAPMessage::NONE_FLAGS, object_class, 0, object_name, result,
        result_reason, invoke_id);

    encodeObject(object_value, message);

    sendMessageToProcess(*message, remote_id, 0);
  } catch (Exception &e) {
    delete message;
    throw e;
  }

  delete message;
}

void RIBDaemon::remoteStopObjectResponse(const std::string& object_class,
                                         const std::string& object_name,
                                         RIBObjectValue& object_value,
                                         int result,
                                         const std::string result_reason,
                                         int invoke_id,
                                         const RemoteProcessId& remote_id)
{
  rina::CDAPMessage * message = 0;

  try {
    (void) object_class;
    (void) object_name;
    message = cdap_session_manager_->getStopObjectResponseMessage(
        rina::CDAPMessage::NONE_FLAGS, result, result_reason, invoke_id);

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
                                 const std::string& name, const bool mandatory,
                                 const unsigned max_objs)
{
  name_ = name;
  class_name_ = class_name;
  mandatory_ = mandatory;
  max_objs_ = max_objs;
}

void RIBSchemaObject::addChild(RIBSchemaObject *object)
{
  children_.push_back(object);
}

const std::string& RIBSchemaObject::get_class_name() const
{
  return class_name_;
}

unsigned RIBSchemaObject::get_max_objs() const
{
  return max_objs_;
}

// CLASS RIBSchema
RIBSchema::RIBSchema(const rib_ver_t& version, char field_separator,
                     char id_separator)
{
  version_ = version;
  field_separator_ = field_separator;
  id_separator_ = id_separator;
}

rib_res RIBSchema::ribSchemaDefContRelation(const std::string& container_cn,
                                            const std::string& class_name,
                                            const std::string name,
                                            const bool mandatory,
                                            const unsigned max_objs)
{
  RIBSchemaObject *object = new RIBSchemaObject(class_name, name, mandatory,
                                                max_objs);
  RIBSchemaObject *parent = rib_schema_[getParentName(name)];

  if (parent->get_class_name().compare(container_cn) != 0)
    return RIB_SCHEMA_FORMAT_ERR;

  std::pair<std::map<std::string, RIBSchemaObject*>::iterator, bool> ret =
      rib_schema_.insert(
          std::pair<std::string, RIBSchemaObject*>(name, object));

  if (ret.second) {
    return RIB_SUCCESS;
  } else {
    return RIB_SCHEMA_FORMAT_ERR;
  }
}

bool RIBSchema::validateAddObject(const BaseRIBObject* obj,
                                  const BaseRIBObject *parent)
{
  std::string name_schema = parseName(obj->get_name());
  RIBSchemaObject *schema_object = rib_schema_[name_schema];
  // CHECKS REGION //
  // Existance
  if (schema_object == 0)
    LOG_INFO();
  return false;
  // parent existance
  std::string parent_name_schema = parseName(obj->get_parent_name());
  RIBSchemaObject *parent_schema_object = rib_schema_[parent_name_schema];
  if (parent_schema_object == 0)
    return false;
  // maximum number of objects
  if (parent->get_children_size() >= schema_object->get_max_objs()) {
    return false;
  }

  return true;
}

std::string RIBSchema::parseName(const std::string& name)
{
  std::string name_schema = "";
  int position = 0;
  int field_separator_position = name.find(field_separator_, position);
  while (field_separator_position != -1) {
    int id_separator_position = name.find(id_separator_, position);
    if (id_separator_position < field_separator_position)
      // field with value
      name_schema.append(name, position, id_separator_position);
    else
      // field without value
      name_schema.append(name, position, field_separator_position);

    field_separator_position = name.find(field_separator_, position);
  }
  return name_schema;
}

std::string RIBSchema::getParentName(const std::string& name)
{
  int field_separator_position = name.find_last_of(field_separator_, 0);
  if (field_separator_position != -1) {
    return name.substr(0, field_separator_position);
  }
  return "";
}

char RIBSchema::get_field_separator() const
{
  return field_separator_;
}
char RIBSchema::get_id_separator() const
{
  return id_separator_;
}

}

