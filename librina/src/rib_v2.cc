/*
 * RIB API
 *
 *    Bernat Gastón <bernat.gaston@i2cat.net>
 *
 * This library is free software{} you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation{} either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY{} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library{} if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#define RINA_PREFIX "rib"
#include <librina/logs.h>
//FIXME iostream is only for debuging purposes
//#include <iostream>

#include "librina/rib_v2.h"
#include "librina/cdap.h"
#include "librina/cdap_v2.h"

namespace rina {
namespace rib {

class RIB;

class RIBIntObject
{
  friend class RIB;
 private:
  RIBIntObject(BaseRIBObject *object);
  ~RIBIntObject();

  void add_child(RIBIntObject *child);
  void remove_child(const std::string& name);
  BaseRIBObject* object_;
  RIBIntObject* parent_;
  std::list<RIBIntObject*> children_;
};

RIBIntObject::RIBIntObject(BaseRIBObject *object)
{
  object_ = object;
}

RIBIntObject::~RIBIntObject()
{
  delete object_;
}

void RIBIntObject::add_child(RIBIntObject *child)
{
  for (std::list<RIBIntObject*>::iterator it = children_.begin();
      it != children_.end(); it++) {
    if ((*it)->object_->get_name().compare(child->object_->get_name()) == 0) {
      throw Exception("Object is already a child");
    }
  }

  children_.push_back(child);
  child->parent_ = this;
}

void RIBIntObject::remove_child(const std::string& name)
{
  for (std::list<RIBIntObject*>::iterator it = children_.begin();
      it != children_.end(); it++) {
    if ((*it)->object_->get_name().compare(name) == 0) {
      children_.erase(it);
      return;
    }
  }

  std::stringstream ss;
  ss << "Unknown child object with object name: " << name.c_str() << std::endl;
  throw Exception(ss.str().c_str());
}

/// A simple RIB implementation, based on a hashtable of RIB objects
/// indexed by object name
class RIB : public rina::Lockable
{
 public:
  RIB(const RIBSchema *schema);
  ~RIB() throw ();
  /// Given an objectname of the form "substring\0substring\0...substring" locate
  /// the RIBObject that corresponds to it
  /// @param objectName
  /// @return
  BaseRIBObject* getRIBObject(const std::string& clas, const std::string& name,
                              bool check);
  BaseRIBObject* getRIBObject(const std::string& clas, long instance,
                              bool check);
  BaseRIBObject* removeRIBObject(const std::string& name);
  BaseRIBObject* removeRIBObject(long instance);
  std::list<RIBObjectData*> getRIBObjectsData();
  char get_separator() const;
  void addRIBObject(BaseRIBObject* ribObject);
  std::string get_parent_name(const std::string child_name) const;
 private:
  std::map<std::string, RIBIntObject*> rib_by_name_;
  std::map<long, RIBIntObject*> rib_by_instance_;
  const RIBSchema *rib_schema_;
};

//Class RIB
RIB::RIB(const RIBSchema *schema)
{
  rib_schema_ = schema;
}

RIB::~RIB() throw ()
{
  delete rib_schema_;
  lock();
  for (std::map<std::string, RIBIntObject*>::iterator it = rib_by_name_.begin();
      it != rib_by_name_.end(); ++it) {
    LOG_INFO("Object %s removed from the RIB",
             it->second->object_->get_name().c_str());
    delete it->second;
  }
  rib_by_name_.clear();
  rib_by_instance_.clear();
  unlock();
}

BaseRIBObject* RIB::getRIBObject(const std::string& clas,
                                 const std::string& name, bool check)
{
  std::string norm_name = name;
  norm_name.erase(std::remove_if(norm_name.begin(), norm_name.end(), ::isspace),
                  norm_name.end());

  RIBIntObject* rib_object;
  std::map<std::string, RIBIntObject*>::iterator it;

  lock();
  it = rib_by_name_.find(norm_name);
  unlock();
  if (it == rib_by_name_.end()) {
    std::stringstream ss;
    ss << "Could not find object " << norm_name << " of class " << clas
       << " in the RIB" << std::endl;
    throw Exception(ss.str().c_str());
  }

  rib_object = it->second;
  if (check && rib_object->object_->get_class().compare(clas) != 0) {
    throw Exception("Object class does not match the user specified one");
  }

  return rib_object->object_;
}

BaseRIBObject* RIB::getRIBObject(const std::string& clas, long instance,
                                 bool check)
{
  RIBIntObject* rib_object;
  std::map<long, RIBIntObject*>::iterator it;

  lock();
  it = rib_by_instance_.find(instance);
  unlock();

  if (it == rib_by_instance_.end()) {
    std::stringstream ss;
    ss << "Could not find object instance " << instance << " of class "
       << clas.c_str() << " in the RIB" << std::endl;
    throw Exception(ss.str().c_str());
  }

  rib_object = it->second;
  if (check && rib_object->object_->get_class().compare(clas) != 0) {
    throw Exception("Object class does not match the user "
                    "specified one");
  }

  return rib_object->object_;
}

void RIB::addRIBObject(BaseRIBObject* rib_object)
{
  RIBIntObject *parent = 0;

  std::string parent_name = get_parent_name(rib_object->get_name());
  if (!parent_name.empty()) {
    lock();
    if (rib_by_name_.find(parent_name) == rib_by_name_.end()) {
      std::stringstream ss;
      ss << "Exception in object " << rib_object->get_name()
         << ". Parent name (" << parent_name << ") is not in the RIB"
         << std::endl;
      unlock();
      throw Exception(ss.str().c_str());
    }
    parent = rib_by_name_[parent_name];
    unlock();
  }
  // TODO: add schema validation
//  if (rib_schema_->validateAddObject(rib_object, parent))
//  {
  lock();
  if (rib_by_name_.find(rib_object->get_name()) != rib_by_name_.end()) {
    unlock();
    std::stringstream ss;
    ss << "Object with the same name (" << rib_object->get_name()
       << ") already exists in the RIB" << std::endl;
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
  RIBIntObject *int_object = new RIBIntObject(rib_object);
  if (parent != 0) {
    parent->add_child(int_object);
    int_object->parent_ = parent;
  }
  rib_by_name_[int_object->object_->get_name()] = int_object;
  rib_by_instance_[int_object->object_->get_instance()] = int_object;
  unlock();
}

BaseRIBObject* RIB::removeRIBObject(const std::string& name)
{
  std::map<std::string, RIBIntObject*>::iterator it;
  RIBIntObject* rib_object;

  lock();
  it = rib_by_name_.find(name);
  if (it == rib_by_name_.end()) {
    unlock();
    throw Exception("Could not find object in the RIB");
  }

  rib_object = it->second;

  RIBIntObject* parent = rib_object->parent_;
  parent->remove_child(name);
  rib_by_name_.erase(it);
  rib_by_instance_.erase(rib_object->object_->get_instance());
  unlock();
  return rib_object->object_;
}

BaseRIBObject * RIB::removeRIBObject(long instance)
{
  std::map<long, RIBIntObject*>::iterator it;
  RIBIntObject* rib_object;

  lock();
  it = rib_by_instance_.find(instance);
  if (it == rib_by_instance_.end()) {
    unlock();
    throw Exception("Could not find object in the RIB");
  }

  rib_object = it->second;
  rib_by_instance_.erase(it);
  unlock();

  return rib_object->object_;
}

std::list<RIBObjectData*> RIB::getRIBObjectsData()
{
  std::list<RIBObjectData*> result;

  lock();
  for (std::map<std::string, RIBIntObject*>::iterator it = rib_by_name_.begin();
      it != rib_by_name_.end(); ++it) {
    result.push_back(it->second->object_->get_data());
  }
  unlock();
  return result;
}

char RIB::get_separator() const
{
  return rib_schema_->get_separator();
}

std::string RIB::get_parent_name(const std::string child_name) const
{
  size_t last_separator = child_name.find_last_of(rib_schema_->separator_,
                                                  std::string::npos);
  if (last_separator == std::string::npos)
    return "";

  return child_name.substr(0, last_separator);

}

/// Interface that provides the RIB Daemon API
class RIBDaemon : public RIBDNorthInterface, cdap::CDAPCallbackInterface
{
 public:
  RIBDaemon(cacep::AppConHandlerInterface *app_con_callback,
            ResponseHandlerInterface* app_resp_callback,
            cdap_rib::cdap_params *params, const RIBSchema *schema);
  ~RIBDaemon();
  void open_connection_result(const cdap_rib::con_handle_t &con,
                              const cdap_rib::result_info &res);
  void open_connection(const cdap_rib::con_handle_t &con,
                       const cdap_rib::flags_t &flags, int message_id);
  void close_connection_result(const cdap_rib::con_handle_t &con,
                               const cdap_rib::result_info &res);
  void close_connection(const cdap_rib::con_handle_t &con,
                        const cdap_rib::flags_t &flags, int message_id);

  void remote_create_result(const cdap_rib::con_handle_t &con,
                            const cdap_rib::res_info_t &res);
  void remote_delete_result(const cdap_rib::con_handle_t &con,
                            const cdap_rib::res_info_t &res);
  void remote_read_result(const cdap_rib::con_handle_t &con,
                          const cdap_rib::res_info_t &res);
  void remote_cancel_read_result(const cdap_rib::con_handle_t &con,
                                 const cdap_rib::res_info_t &res);
  void remote_write_result(const cdap_rib::con_handle_t &con,
                           const cdap_rib::res_info_t &res);
  void remote_start_result(const cdap_rib::con_handle_t &con,
                           const cdap_rib::res_info_t &res);
  void remote_stop_result(const cdap_rib::con_handle_t &con,
                          const cdap_rib::res_info_t &res);

  void remote_create_request(const cdap_rib::con_handle_t &con,
                             const cdap_rib::obj_info_t &obj,
                             const cdap_rib::filt_info_t &filt, int message_id);
  void remote_delete_request(const cdap_rib::con_handle_t &con,
                             const cdap_rib::obj_info_t &obj,
                             const cdap_rib::filt_info_t &filt, int message_id);
  void remote_read_request(const cdap_rib::con_handle_t &con,
                           const cdap_rib::obj_info_t &obj,
                           const cdap_rib::filt_info_t &filt, int message_id);
  void remote_cancel_read_request(const cdap_rib::con_handle_t &con,
                                  const cdap_rib::obj_info_t &obj,
                                  const cdap_rib::filt_info_t &filt,
                                  int message_id);
  void remote_write_request(const cdap_rib::con_handle_t &con,
                            const cdap_rib::obj_info_t &obj,
                            const cdap_rib::filt_info_t &filt, int message_id);
  void remote_start_request(const cdap_rib::con_handle_t &con,
                            const cdap_rib::obj_info_t &obj,
                            const cdap_rib::filt_info_t &filt, int message_id);
  void remote_stop_request(const cdap_rib::con_handle_t &con,
                           const cdap_rib::obj_info_t &obj,
                           const cdap_rib::filt_info_t &filt, int message_id);
  void addRIBObject(BaseRIBObject *ribObject);
  void removeRIBObject(BaseRIBObject *ribObject);
  void removeRIBObject(const std::string& name);
  BaseRIBObject* getObject(const std::string& name,
                           const std::string& clas) const;
  BaseRIBObject* getObject(unsigned long instance,
                           const std::string& clas) const;
  void process_message(cdap_rib::SerializedObject &message, int port);

 private:
  cacep::AppConHandlerInterface *app_con_callback_;
  ResponseHandlerInterface *app_resp_callback_;
  cdap::CDAPProviderInterface *cdap_provider_;
  RIB *rib_;
};

RIBDaemon::RIBDaemon(cacep::AppConHandlerInterface *app_con_callback,
                     ResponseHandlerInterface* app_resp_callback,
                     cdap_rib::cdap_params *params, const RIBSchema *schema)
{
  app_con_callback_ = app_con_callback;
  app_resp_callback_ = app_resp_callback;
  rib_ = new RIB(schema);
  cdap::CDAPProviderFactory::init(params->timeout_);
  cdap_provider_ = cdap::CDAPProviderFactory::create(params->is_IPCP_, this);
  delete params;
}

RIBDaemon::~RIBDaemon()
{
  delete app_con_callback_;
  delete app_resp_callback_;
  delete rib_;
  delete cdap_provider_;
  cdap::CDAPProviderFactory::finit();
}

void RIBDaemon::open_connection_result(const cdap_rib::con_handle_t &con,
                                       const cdap_rib::res_info_t &res)
{
  // FIXME remove message_id

  app_con_callback_->connectResponse(res, con);
}

void RIBDaemon::open_connection(const cdap_rib::con_handle_t &con,
                                const cdap_rib::flags_t &flags, int message_id)
{
  // FIXME add result
  cdap_rib::result_info res;
  (void) res;
  (void) flags;
  app_con_callback_->connect(message_id, con);
  cdap_provider_->open_connection_response(con, res, message_id);
}

void RIBDaemon::close_connection_result(const cdap_rib::con_handle_t &con,
                                        const cdap_rib::result_info &res)
{
  app_con_callback_->releaseResponse(res, con);
}

void RIBDaemon::close_connection(const cdap_rib::con_handle_t &con,
                                 const cdap_rib::flags_t &flags, int message_id)
{
  // FIXME add result
  cdap_rib::result_info res;
  (void) res;
  app_con_callback_->release(message_id, con);
  cdap_provider_->close_connection_response(con, flags, res, message_id);
}

void RIBDaemon::remote_create_result(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::res_info_t &res)
{
  app_resp_callback_->createResponse(res, con);
}
void RIBDaemon::remote_delete_result(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::res_info_t &res)
{
  app_resp_callback_->deleteResponse(res, con);
}
void RIBDaemon::remote_read_result(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::res_info_t &res)
{
  app_resp_callback_->readResponse(res, con);
}
void RIBDaemon::remote_cancel_read_result(const cdap_rib::con_handle_t &con,
                                          const cdap_rib::res_info_t &res)
{
  app_resp_callback_->cancelReadResponse(res, con);
}
void RIBDaemon::remote_write_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::res_info_t &res)
{
  app_resp_callback_->writeResponse(res, con);
}
void RIBDaemon::remote_start_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::res_info_t &res)
{
  app_resp_callback_->startResponse(res, con);
}
void RIBDaemon::remote_stop_result(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::res_info_t &res)
{
  app_resp_callback_->stopResponse(res, con);
}

void RIBDaemon::remote_create_request(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::obj_info_t &obj,
                                      const cdap_rib::filt_info_t &filt,
                                      int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::flags_t flags;

  BaseRIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  cdap_rib::res_info_t* res = ribObj->remoteCreateObject(obj.name_, obj.value_);
  try {
    cdap_provider_->remote_create_response(con, obj, flags, *res, message_id);
  } catch (Exception &e) {
    LOG_ERR("Unable to send the response");
  }
  delete res;
}
void RIBDaemon::remote_delete_request(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::obj_info_t &obj,
                                      const cdap_rib::filt_info_t &filt,
                                      int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::flags_t flags;

  BaseRIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  cdap_rib::res_info_t* res = ribObj->remoteDeleteObject(obj.name_, obj.value_);
  try {
    cdap_provider_->remote_delete_response(con, obj, flags, *res, message_id);
  } catch (Exception &e) {
    LOG_ERR("Unable to send the response");
  }
  delete res;
}
void RIBDaemon::remote_read_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt,
                                    int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::flags_t flags;

  BaseRIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  cdap_rib::res_info_t* res = ribObj->remoteReadObject(obj.name_, obj.value_);
  try {
    cdap_provider_->remote_read_response(con, obj, flags, *res, message_id);
  } catch (Exception &e) {
    LOG_ERR("Unable to send the response");
  }
  delete res;
}
void RIBDaemon::remote_cancel_read_request(const cdap_rib::con_handle_t &con,
                                           const cdap_rib::obj_info_t &obj,
                                           const cdap_rib::filt_info_t &filt,
                                           int message_id)
{
  (void) filt;

  // FIXME add res and flags
  cdap_rib::flags_t flags;

  BaseRIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  cdap_rib::res_info_t* res = ribObj->remoteCancelReadObject(obj.name_,
                                                             obj.value_);
  try {
    cdap_provider_->remote_cancel_read_response(con, flags, *res, message_id);
  } catch (Exception &e) {
    LOG_ERR("Unable to send the response");
  }
  delete res;
}
void RIBDaemon::remote_write_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt,
                                     int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::flags_t flags;

  BaseRIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  cdap_rib::res_info_t* res = ribObj->remoteWriteObject(obj.name_, obj.value_);
  try {
    cdap_provider_->remote_write_response(con, flags, *res, message_id);
  } catch (Exception &e) {
    LOG_ERR("Unable to send the response");
  }
  delete res;
}
void RIBDaemon::remote_start_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt,
                                     int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::flags_t flags;

  BaseRIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  cdap_rib::res_info_t* res = ribObj->remoteStartObject(obj.name_, obj.value_);
  try {
    cdap_provider_->remote_start_response(con, obj, flags, *res, message_id);
  } catch (Exception &e) {
    LOG_ERR("Unable to send the response");
  }
  delete res;
}
void RIBDaemon::remote_stop_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt,
                                    int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::flags_t flags;

  BaseRIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  cdap_rib::res_info_t* res = ribObj->remoteStopObject(obj.name_, obj.value_);
  try {
    cdap_provider_->remote_stop_response(con, flags, *res, message_id);
  } catch (Exception &e) {
    LOG_ERR("Unable to send the response");
  }
  delete res;
}

void RIBDaemon::addRIBObject(BaseRIBObject *ribObject)
{
  rib_->addRIBObject(ribObject);
}
void RIBDaemon::removeRIBObject(BaseRIBObject *ribObject)
{
  rib_->removeRIBObject(ribObject->get_name());
}
void RIBDaemon::removeRIBObject(const std::string& name)
{
  rib_->removeRIBObject(name);
}

BaseRIBObject* RIBDaemon::getObject(const std::string& name,
                                    const std::string& clas) const
{
  return rib_->getRIBObject(clas, name, true);
}
BaseRIBObject* RIBDaemon::getObject(unsigned long instance,
                                    const std::string& clas) const
{
  return rib_->getRIBObject(clas, instance, true);
}
void RIBDaemon::process_message(cdap_rib::SerializedObject &message, int port)
{
  cdap_provider_->process_message(message, port);
}

// Class RIBObjectData
RIBObjectData::RIBObjectData()
{
  instance_ = 0;
}

RIBObjectData::RIBObjectData(std::string clazz, std::string name,
                             unsigned long instance, std::string disp_value)
{
  class_ = clazz;
  name_ = name;
  instance_ = instance;
  displayable_value_ = disp_value;
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

unsigned long RIBObjectData::get_instance() const
{
  return instance_;
}

const std::string& RIBObjectData::get_name() const
{
  return name_;
}

const std::string& RIBObjectData::get_displayable_value() const
{
  return displayable_value_;
}

//Class RIBObject

RIBObjectData* BaseRIBObject::get_data()
{
  RIBObjectData *result = new RIBObjectData(class_, name_, instance_,
                                            get_displayable_value());
  return result;
}

std::string BaseRIBObject::get_displayable_value()
{
  return "-";
}

bool BaseRIBObject::createObject(const std::string& clas,
                                 const std::string& name, const void* value)
{
  (void) clas;
  (void) name;
  (void) value;
  operation_not_supported();
  return false;
}

bool BaseRIBObject::deleteObject(const void* value)
{
  (void) value;
  operation_not_supported();
  return false;
}

BaseRIBObject* BaseRIBObject::readObject()
{
  return this;
}

bool BaseRIBObject::writeObject(const void* value)
{
  (void) value;
  operation_not_supported();
  return false;
}

bool BaseRIBObject::startObject(const void* object)
{
  (void) object;
  operation_not_supported();
  return false;
}

bool BaseRIBObject::stopObject(const void* object)
{
  (void) object;
  operation_not_supported();
  return false;
}

cdap_rib::res_info_t* BaseRIBObject::remoteCreateObject(const std::string& name,
                                                        const cdap_rib::SerializedObject &value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return 0;
}

cdap_rib::res_info_t* BaseRIBObject::remoteDeleteObject(const std::string& name,
                                                        const cdap_rib::SerializedObject &value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return 0;
}

cdap_rib::res_info_t* BaseRIBObject::remoteReadObject(const std::string& name,
                                                      const cdap_rib::SerializedObject &value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return 0;
}

cdap_rib::res_info_t* BaseRIBObject::remoteCancelReadObject(
    const std::string& name, const cdap_rib::SerializedObject &value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return 0;
}

cdap_rib::res_info_t* BaseRIBObject::remoteWriteObject(const std::string& name,
                                                       const cdap_rib::SerializedObject &value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return 0;
}

cdap_rib::res_info_t* BaseRIBObject::remoteStartObject(const std::string& name,
                                                       const cdap_rib::SerializedObject &value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return 0;
}

cdap_rib::res_info_t* BaseRIBObject::remoteStopObject(const std::string& name,
                                                      const cdap_rib::SerializedObject &value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return 0;
}

const std::string& BaseRIBObject::get_class() const
{
  return class_;
}

const std::string& BaseRIBObject::get_name() const
{
  return name_;
}

long BaseRIBObject::get_instance() const
{
  return instance_;
}

void BaseRIBObject::operation_not_supported()
{
  LOG_ERR("Operation not supported");
}

// CLASS RIBSchemaObject
RIBSchemaObject::RIBSchemaObject(const std::string& class_name,
                                 const bool mandatory, const unsigned max_objs)
{
  class_name_ = class_name;
  mandatory_ = mandatory;
  max_objs_ = max_objs;
  (void)parent_;
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
RIBSchema::RIBSchema(const cdap_rib::vers_info_t *version, char separator)
{
  version_ = version;
  separator_ = separator;
}
RIBSchema::~RIBSchema()
{
  delete version_;
}

rib_schema_res RIBSchema::ribSchemaDefContRelation(
    const std::string& cont_class_name, const std::string& class_name,
    const bool mandatory, const unsigned max_objs)
{
  RIBSchemaObject *object = new RIBSchemaObject(class_name, mandatory,
                                                max_objs);
  std::map<std::string, RIBSchemaObject*>::iterator parent_it =
      rib_schema_.find(cont_class_name);

  if (parent_it == rib_schema_.end())
    return RIB_SCHEMA_FORMAT_ERR;

  std::pair<std::map<std::string, RIBSchemaObject*>::iterator, bool> ret =
      rib_schema_.insert(
          std::pair<std::string, RIBSchemaObject*>(class_name, object));

  if (ret.second) {
    return RIB_SUCCESS;
  } else {
    return RIB_SCHEMA_FORMAT_ERR;
  }
}

bool RIBSchema::validateAddObject(const BaseRIBObject* obj)
{
  (void) obj;
  /*
   RIBSchemaObject *schema_object = rib_schema_.find(obj->get_class());
   // CHECKS REGION //
   // Existance
   if (schema_object == 0)
   LOG_INFO();
   return false;
   // parent existance
   RIBSchemaObject *parent_schema_object = rib_schema_[obj->get_parent_class()];
   if (parent_schema_object == 0)
   return false;
   // maximum number of objects
   if (parent->get_children_size() >= schema_object->get_max_objs()) {
   return false;
   }
   */
  return true;
}
/*
 std::string RIBSchema::parseName(const std::string& name)
 {
 std::string name_schema = "";
 int position = 0;
 int field_separator_position = name.find(separator_, position);

 while (field_separator_position != -1) {
 int id_separator_position = name.find(separator_, position);
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
 */
char RIBSchema::get_separator() const
{
  return separator_;
}

// CLASS RIBDFactory
RIBDNorthInterface* RIBDFactory::create(
    cacep::AppConHandlerInterface* app_callback,
    ResponseHandlerInterface* app_resp_callbak, void* comm_params,
    const cdap_rib::version_info *version, char separator)
{
  cdap_rib::cdap_params_t *params = (cdap_rib::cdap_params_t*) comm_params;
  RIBSchema *schema = new RIBSchema(version, separator);
  RIBDNorthInterface* ribd = new RIBDaemon(app_callback, app_resp_callbak,
                                           params, schema);
  return ribd;
}
const cdap_rib::SerializedObject* IntEncoder::encode(const int &object)
{
  (void) object;
  return 0;
}
int* IntEncoder::decode(
    const cdap_rib::SerializedObject &serialized_object) const
{
  (void) serialized_object;
  return 0;
}
const cdap_rib::SerializedObject* SIntEncoder::encode(const short int &object)
{
  (void) object;
  return 0;
}
short int* SIntEncoder::decode(
    const cdap_rib::SerializedObject &serialized_object) const
{
  (void) serialized_object;
  return 0;
}
const cdap_rib::SerializedObject* LongEncoder::encode(const long long &object)
{
  (void) object;
  return 0;
}
long long* LongEncoder::decode(
    const cdap_rib::SerializedObject &serialized_object) const
{
  (void) serialized_object;
  return 0;
}
const cdap_rib::SerializedObject* SLongEncoder::encode(const long &object)
{
  (void) object;
  return 0;
}
long* SLongEncoder::decode(
    const cdap_rib::SerializedObject &serialized_object) const
{
  (void) serialized_object;
  return 0;
}
const cdap_rib::SerializedObject* StringEncoder::encode(
    const std::string &object)
{
  (void) object;
  return 0;
}
std::string* StringEncoder::decode(
    const cdap_rib::SerializedObject &serialized_object) const
{
  (void) serialized_object;
  return 0;
}
const cdap_rib::SerializedObject* FloatEncoder::encode(const float &object)
{
  (void) object;
  return 0;
}
float* FloatEncoder::decode(
    const cdap_rib::SerializedObject &serialized_object) const
{
  (void) serialized_object;
  return 0;
}

const cdap_rib::SerializedObject* DoubleEncoder::encode(const double &object)
{
  (void) object;
  return 0;
}
double* DoubleEncoder::decode(
    const cdap_rib::SerializedObject &serialized_object) const
{
  (void) serialized_object;
  return 0;
}
const cdap_rib::SerializedObject* BoolEncoder::encode(const bool &object)
{
  (void) object;
  return 0;
}
bool* BoolEncoder::decode(
    const cdap_rib::SerializedObject &serialized_object) const
{
  (void) serialized_object;
  return 0;
}

const cdap_rib::SerializedObject* EmptyEncoder::encode(const empty &object)
{
  (void) object;
  LOG_ERR("Can not encode an empty object");
  return 0;
}

empty* EmptyEncoder::decode(
    const cdap_rib::SerializedObject &serialized_object) const
{
  (void) serialized_object;
  LOG_ERR("Can not decode an empty object");
  return 0;
}
}
}

