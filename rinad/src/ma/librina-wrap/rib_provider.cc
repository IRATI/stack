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

#include "rib_provider.h"
#include <librina/cdap.h>
#include "cdap_provider.h"
#include <algorithm>

namespace rib {
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
  RIBObject* getRIBObject(const std::string& objectClass,
                          const std::string& objectName, bool check);
  RIBObject* getRIBObject(const std::string& objectClass, long instance,
                          bool check);
  RIBObject* removeRIBObject(const std::string& objectName);
  RIBObject* removeRIBObject(long instance);
  std::list<RIBObjectData*> getRIBObjectsData();
  char get_separator() const;
  void addRIBObject(RIBObject* ribObject);
  std::string get_parent_name(const std::string child_name) const;
 private:
  std::map<std::string, RIBObject*> rib_by_name_;
  std::map<long, RIBObject*> rib_by_instance_;
  const RIBSchema *rib_schema_;
};

//Class RIB
RIB::RIB(const RIBSchema *schema)
{
  rib_schema_ = schema;
}

RIB::~RIB() throw ()
{
  lock();
  delete rib_schema_;

  for (std::map<std::string, RIBObject*>::iterator it = rib_by_name_.begin();
      it != rib_by_name_.end(); ++it) {
    LOG_DBG("Object %s removed from the RIB", it->second->get_name().c_str());
    delete it->second;
  }
  rib_by_name_.clear();
  rib_by_instance_.clear();
  unlock();
}

RIBObject* RIB::getRIBObject(const std::string& clas, const std::string& name,
                             bool check)
{
  std::string norm_name = name;
  norm_name.erase(std::remove_if(norm_name.begin(), norm_name.end(), ::isspace),
                  norm_name.end());

  RIBObject* ribObject;
  std::map<std::string, RIBObject*>::iterator it;

  lock();
  it = rib_by_name_.find(norm_name);
  unlock();
  if (it == rib_by_name_.end()) {
    std::stringstream ss;
    ss << "Could not find object " << norm_name << " of class " << clas
       << " in the RIB" << std::endl;
    throw Exception(ss.str().c_str());
  }

  ribObject = it->second;
  if (check && ribObject->get_class().compare(clas) != 0) {
    throw Exception("Object class does not match the user specified one");
  }

  return ribObject;
}

RIBObject* RIB::getRIBObject(const std::string& clas, long instance, bool check)
{
  RIBObject* ribObject;
  std::map<long, RIBObject*>::iterator it;

  lock();
  it = rib_by_instance_.find(instance);
  unlock();

  if (it == rib_by_instance_.end()) {
    std::stringstream ss;
    ss << "Could not find object instance " << instance << " of class "
       << clas.c_str() << " in the RIB" << std::endl;
    throw Exception(ss.str().c_str());
  }

  ribObject = it->second;
  if (check && ribObject->get_class().compare(clas) != 0) {
    throw Exception("Object class does not match the user "
                    "specified one");
  }

  return ribObject;
}

void RIB::addRIBObject(RIBObject* rib_object)
{
  RIBObject *parent = 0;
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
  if (parent != 0) {
    parent->add_child(rib_object);
    rib_object->set_parent(parent);
  }
  rib_by_name_[rib_object->get_name()] = rib_object;
  rib_by_instance_[rib_object->get_instance()] = rib_object;
  unlock();
}

RIBObject* RIB::removeRIBObject(const std::string& objectName)
{
  std::map<std::string, RIBObject*>::iterator it;
  RIBObject* rib_object;

  lock();
  it = rib_by_name_.find(objectName);
  if (it == rib_by_name_.end()) {
    unlock();
    throw Exception("Could not find object in the RIB");
  }

  rib_object = it->second;

  RIBObject* parent = rib_object->parent_;
  parent->remove_child(objectName);
  rib_by_name_.erase(it);
  rib_by_instance_.erase(rib_object->instance_);
  unlock();
  return rib_object;
}

RIBObject * RIB::removeRIBObject(long instance)
{
  std::map<long, RIBObject*>::iterator it;
  RIBObject* ribObject;

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

std::list<RIBObjectData*> RIB::getRIBObjectsData()
{
  std::list<RIBObjectData*> result;

  lock();
  for (std::map<std::string, RIBObject*>::iterator it = rib_by_name_.begin();
      it != rib_by_name_.end(); ++it) {
    result.push_back(it->second->get_data());
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
class RIBDaemon : public RIBDInterface
{
 public:
  RIBDaemon(cacep::AppConHandlerInterface *app_con_callback,
            ResponseHandlerInterface* app_resp_callbak,
            const std::string &comm_protocol, cdap::cdap_params_t *params,
            const RIBSchema *schema);
  ~RIBDaemon();
  void open_connection_result(const cdap_rib::con_handle_t &con,
                              const cdap_rib::result_info &res);
  void open_connection(const cdap_rib::con_handle_t &con,
                       const cdap_rib::flags_t &flags,
                       const cdap_rib::result_info &res, int message_id);
  void close_connection_result(const cdap_rib::con_handle_t &con,
                               const cdap_rib::result_info &res);
  void close_connection(const cdap_rib::con_handle_t &con,
                        const cdap_rib::flags_t &flags,
                        const cdap_rib::result_info &res, int message_id);

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

  /// Create or update an object in the RIB
  void createObject(const cdap_rib::obj_info_t &obj);
  /// Delete an object from the RIB
  void deleteObject(const cdap_rib::obj_info_t &obj);
  /// Read an object from the RIB
  cdap_rib::obj_info_t* readObject(const std::string& clas,
                                   const std::string& name);
  /// Read an object from the RIB
  cdap_rib::obj_info_t* readObject(const std::string& clas, long instance);
  /// Update the value of an object in the RIB
  void writeObject(const cdap_rib::obj_info_t &obj);
  /// Start an object at the RIB
  void startObject(const cdap_rib::obj_info_t &obj);
  /// Stop an object at the RIB
  void stopObject(const cdap_rib::obj_info_t &obj);
  void addRIBObject(RIBObject *ribObject);
  void removeRIBObject(RIBObject *ribObject);
  void removeRIBObject(const std::string& name);

 private:
  cacep::AppConHandlerInterface *app_con_callback_;
  ResponseHandlerInterface *app_resp_callback_;
  cdap::CDAPProviderInterface *cdap_manager_;
  RIB *rib_;
};

RIBDaemon::RIBDaemon(cacep::AppConHandlerInterface *app_con_callback,
                     ResponseHandlerInterface* app_resp_callback,
                     const std::string &comm_protocol,
                     cdap::cdap_params_t *params, const RIBSchema *schema)
{
  cdap::CDAPProviderFactory factory;
  app_con_callback_ = app_con_callback;
  app_resp_callback_ = app_resp_callback;
  rib_ = new RIB(schema);
  cdap_manager_ = factory.create(comm_protocol, params->timeout_,
                                 params->is_IPCP_);
}

RIBDaemon::~RIBDaemon()
{
  delete rib_;
}

void RIBDaemon::open_connection_result(const cdap_rib::con_handle_t &con,
                                       const cdap_rib::res_info_t &res)
{
  // FIXME remove message_id

  app_con_callback_->connectResponse(res, con);
}

void RIBDaemon::open_connection(const cdap_rib::con_handle_t &con,
                                const cdap_rib::flags_t &flags,
                                const cdap_rib::result_info &res,
                                int message_id)
{
  app_con_callback_->connect(message_id, con);
  cdap_manager_->open_connection_response(con, flags, res, message_id);
}

void RIBDaemon::close_connection_result(const cdap_rib::con_handle_t &con,
                                        const cdap_rib::result_info &res)
{
  app_con_callback_->releaseResponse(res, con);
}

void RIBDaemon::close_connection(const cdap_rib::con_handle_t &con,
                                 const cdap_rib::flags_t &flags,
                                 const cdap_rib::result_info &res,
                                 int message_id)
{
  app_con_callback_->release(message_id, con);
  cdap_manager_->close_connection_response(con, flags, res, message_id);
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
  cdap_rib::res_info_t res;
  cdap_rib::flags_t flags;

  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->remoteCreateObject(obj.name_, obj.value_);
  cdap_manager_->remote_create_response(con, obj, flags, res, message_id);
}
void RIBDaemon::remote_delete_request(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::obj_info_t &obj,
                                      const cdap_rib::filt_info_t &filt,
                                      int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::res_info_t res;
  cdap_rib::flags_t flags;

  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->remoteDeleteObject(obj.name_, obj.value_);
  cdap_manager_->remote_delete_response(con, obj, flags, res, message_id);
}
void RIBDaemon::remote_read_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt,
                                    int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::res_info_t res;
  cdap_rib::flags_t flags;

  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->remoteReadObject(obj.name_, obj.value_);
  cdap_manager_->remote_read_response(con, obj, flags, res, message_id);
}
void RIBDaemon::remote_cancel_read_request(const cdap_rib::con_handle_t &con,
                                           const cdap_rib::obj_info_t &obj,
                                           const cdap_rib::filt_info_t &filt,
                                           int message_id)
{
  (void) filt;
  (void) filt;

  // FIXME add res and flags
  cdap_rib::res_info_t res;
  cdap_rib::flags_t flags;

  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->remoteCancelReadObject(obj.name_, obj.value_);
  cdap_manager_->remote_cancel_read_response(con, flags, res, message_id);
}
void RIBDaemon::remote_write_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt,
                                     int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::res_info_t res;
  cdap_rib::flags_t flags;

  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->remoteWriteObject(obj.name_, obj.value_);
  cdap_manager_->remote_write_response(con, flags, res, message_id);
}
void RIBDaemon::remote_start_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt,
                                     int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::res_info_t res;
  cdap_rib::flags_t flags;

  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->remoteStartObject(obj.name_, obj.value_);
  cdap_manager_->remote_start_response(con, obj, flags, res, message_id);
}
void RIBDaemon::remote_stop_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt,
                                    int message_id)
{
  (void) filt;
  // FIXME add res and flags
  cdap_rib::res_info_t res;
  cdap_rib::flags_t flags;

  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->remoteStopObject(obj.name_, obj.value_);
  cdap_manager_->remote_stop_response(con, flags, res, message_id);
}

void RIBDaemon::createObject(const cdap_rib::obj_info_t &obj)
{
  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->createObject(obj.class_, obj.name_, obj.value_);
}

void RIBDaemon::deleteObject(const cdap_rib::obj_info_t &obj)
{
  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->deleteObject(obj.value_);
}

cdap_rib::obj_info_t* RIBDaemon::readObject(const std::string& clas,
                                            const std::string& name)
{
  cdap_rib::obj_info_t *obj = new cdap_rib::obj_info_t;
  obj->name_ = name;
  obj->class_ = clas;

  RIBObject* ribObj = rib_->getRIBObject(clas, name, true);

  obj->value_ = ribObj->get_value();

  return obj;
}

cdap_rib::obj_info_t* RIBDaemon::readObject(const std::string& clas,
                                            long instance)
{
  cdap_rib::obj_info_t *obj = new cdap_rib::obj_info_t;
  obj->class_ = clas;

  RIBObject* ribObj = rib_->getRIBObject(clas, instance, true);

  obj->name_ = ribObj->get_name();
  obj->value_ = ribObj->get_value();

  return obj;
}

void RIBDaemon::writeObject(const cdap_rib::obj_info_t &obj)
{
  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->writeObject(obj.value_);
}

void RIBDaemon::startObject(const cdap_rib::obj_info_t &obj)
{
  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->startObject(obj.value_);
}

void RIBDaemon::stopObject(const cdap_rib::obj_info_t &obj)
{
  RIBObject* ribObj = rib_->getRIBObject(obj.class_, obj.name_, true);
  ribObj->stopObject(obj.value_);
}

void RIBDaemon::addRIBObject(RIBObject *ribObject)
{
  rib_->addRIBObject(ribObject);
}
void RIBDaemon::removeRIBObject(RIBObject *ribObject)
{
  rib_->removeRIBObject(ribObject->get_name());
}
void RIBDaemon::removeRIBObject(const std::string& name)
{
  rib_->removeRIBObject(name);
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
RIBObject::RIBObject(RIBDObjectInterface * rib_daemon, const std::string& clas,
                     long instance, std::string name)
{
  name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end());
  name_ = name;
  class_ = clas;
  instance_ = instance;
  rib_daemon_ = rib_daemon;
  parent_ = 0;
}

RIBObject::~RIBObject()
{
  /*
   // FIXME: Remove this when objects have two parents
   while (children_.size() > 0) {
   rib_daemon_->removeRIBObject(children_.front()->get_name());
   }
   */
  LOG_DBG("Object %s destroyed", name_.c_str());
}

RIBObjectData* RIBObject::get_data()
{
  RIBObjectData *result = new RIBObjectData(class_, name_, instance_,
                                            get_displayable_value());

  return result;
}

std::string RIBObject::get_displayable_value()
{
  return "-";
}

void RIBObject::get_children_value(
    std::list<std::pair<std::string, const void *> > &values) const
{
  values.clear();
  for (std::list<RIBObject*>::const_iterator it = children_.begin();
      it != children_.end(); ++it) {
    values.push_back(
        std::pair<std::string, const void *>((*it)->name_, (*it)->get_value()));
  }
}

unsigned int RIBObject::get_children_size() const
{
  return children_.size();
}

void RIBObject::add_child(RIBObject *child)
{
  for (std::list<RIBObject*>::iterator it = children_.begin();
      it != children_.end(); it++) {
    if ((*it)->name_.compare(child->name_) == 0) {
      throw Exception("Object is already a child");
    }
  }

  children_.push_back(child);
  child->parent_ = this;
}

void RIBObject::remove_child(const std::string& name)
{
  for (std::list<RIBObject*>::iterator it = children_.begin();
      it != children_.end(); it++) {
    if ((*it)->name_.compare(name) == 0) {
      children_.erase(it);
      return;
    }
  }

  std::stringstream ss;
  ss << "Unknown child object with object name: " << name.c_str() << std::endl;
  throw Exception(ss.str().c_str());
}

bool RIBObject::createObject(const std::string& clas, const std::string& name,
                             const void* value)
{
  (void) clas;
  (void) name;
  (void) value;
  operation_not_supported();
  return false;
}

bool RIBObject::deleteObject(const void* value)
{
  (void) value;
  operation_not_supported();
  return false;
}

RIBObject * RIBObject::readObject()
{
  return this;
}

bool RIBObject::writeObject(const void* value)
{
  (void) value;
  operation_not_supported();
  return false;
}

bool RIBObject::startObject(const void* object)
{
  (void) object;
  operation_not_supported();
  return false;
}

bool RIBObject::stopObject(const void* object)
{
  (void) object;
  operation_not_supported();
  return false;
}

bool RIBObject::remoteCreateObject(const std::string& name, void * value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return false;
}

bool RIBObject::remoteDeleteObject(const std::string& name, void * value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return false;
}

bool RIBObject::remoteReadObject(const std::string& name, void * value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return false;
}

bool RIBObject::remoteCancelReadObject(const std::string& name, void * value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return false;
}

bool RIBObject::remoteWriteObject(const std::string& name, void * value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return false;
}

bool RIBObject::remoteStartObject(const std::string& name, void * value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return false;
}

bool RIBObject::remoteStopObject(const std::string& name, void * value)
{
  (void) name;
  (void) value;
  operation_not_supported();
  return false;
}

const std::string& RIBObject::get_class() const
{
  return class_;
}

const std::string& RIBObject::get_name() const
{
  return name_;
}

const std::string& RIBObject::get_parent_name() const
{
  return parent_->get_name();
}

const std::string& RIBObject::get_parent_class() const
{
  return parent_->get_class();
}

long RIBObject::get_instance() const
{
  return instance_;
}

void RIBObject::set_parent(RIBObject* parent)
{
  parent_ = parent;
}

void RIBObject::operation_not_supported()
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
RIBSchema::RIBSchema(const cdap_rib::vers_info_t& version, char separator)
{
  version_ = version;
  separator_ = separator;
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

bool RIBSchema::validateAddObject(const RIBObject* obj)
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
RIBDInterface* RIBDFactory::create(cacep::AppConHandlerInterface* app_callback,
                                   ResponseHandlerInterface* app_resp_callbak,
                                   const std::string &comm_protocol,
                                   void* comm_params,
                                   cdap_rib::version_info version,
                                   char separator)
{
  cdap::cdap_params_t *params = (cdap::cdap_params_t*) comm_params;
  RIBSchema *schema = new RIBSchema(version, separator);
  RIBDInterface* ribd = new RIBDaemon(app_callback, app_resp_callbak,
                                      comm_protocol, params, schema);
  delete params;
  return ribd;
}

}

