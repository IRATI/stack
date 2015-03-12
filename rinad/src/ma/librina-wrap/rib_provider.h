/*
 * RIB API
 *
 *    Bernat Gastón <bernat.gaston@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */
#ifndef RIB_PROVIDER_H_
#define RIB_PROVIDER_H_
#include "cdap_rib_structures.h"
#include <string>
#include <list>
#include <map>

namespace cacep {
// FIXME: this class is only used in enrollment, it must go in a different file that rib
class AppConHandlerInterface
{
 public:
  virtual ~AppConHandlerInterface()
  {
  }
  /// A remote IPC process Connect request has been received.
  virtual void connect(int message_id, const cdap_rib::con_handle_t &con) = 0;
  /// A remote IPC process Connect response has been received.
  virtual void connectResponse(const cdap_rib::res_info_t &res,
                               const cdap_rib::con_handle_t &con) = 0;
  /// A remote IPC process Release request has been received.
  virtual void release(int message_id, const cdap_rib::con_handle_t &con) = 0;
  /// A remote IPC process Release response has been received.
  virtual void releaseResponse(const cdap_rib::res_info_t &res,
                               const cdap_rib::con_handle_t &con) = 0;
};
}

namespace rib {

class ResponseHandlerInterface
{
 public:
  virtual ~ResponseHandlerInterface()
  {
  }

  virtual void createResponse(const cdap_rib::res_info_t &res,
                              const cdap_rib::con_handle_t &con) = 0;
  virtual void deleteResponse(const cdap_rib::res_info_t &res,
                              const cdap_rib::con_handle_t &con) = 0;
  virtual void readResponse(const cdap_rib::res_info_t &res,
                            const cdap_rib::con_handle_t &con) = 0;
  virtual void cancelReadResponse(const cdap_rib::res_info_t &res,
                                  const cdap_rib::con_handle_t &con) = 0;
  virtual void writeResponse(const cdap_rib::res_info_t &res,
                             const cdap_rib::con_handle_t &con) = 0;
  virtual void startResponse(const cdap_rib::res_info_t &res,
                             const cdap_rib::con_handle_t &con) = 0;
  virtual void stopResponse(const cdap_rib::res_info_t &res,
                            const cdap_rib::con_handle_t &con) = 0;
};

// Interface of the RIB to be used from the communication protocol
class RIBDSouthInterface
{
 public:
  virtual ~RIBDSouthInterface()
  {
  }
  ;
  virtual void open_connection_result(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::result_info &res) = 0;
  virtual void open_connection(const cdap_rib::con_handle_t &con,
                               const cdap_rib::flags_t &flags,
                               const cdap_rib::result_info &res,
                               int message_id) = 0;
  virtual void close_connection_result(const cdap_rib::con_handle_t &con,
                                       const cdap_rib::result_info &res) = 0;
  virtual void close_connection(const cdap_rib::con_handle_t &con,
                                const cdap_rib::flags_t &flags,
                                const cdap_rib::result_info &res,
                                int message_id) = 0;

  virtual void remote_create_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::res_info_t &res) = 0;
  virtual void remote_delete_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::res_info_t &res) = 0;
  virtual void remote_read_result(const cdap_rib::con_handle_t &con,
                                  const cdap_rib::res_info_t &res) = 0;
  virtual void remote_cancel_read_result(const cdap_rib::con_handle_t &con,
                                         const cdap_rib::res_info_t &res);
  virtual void remote_write_result(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::res_info_t &res) = 0;
  virtual void remote_start_result(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::res_info_t &res) = 0;
  virtual void remote_stop_result(const cdap_rib::con_handle_t &con,
                                  const cdap_rib::res_info_t &res) = 0;

  virtual void remote_create_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt,
                                     int message_id) = 0;
  virtual void remote_delete_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt,
                                     int message_id) = 0;
  virtual void remote_read_request(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::obj_info_t &obj,
                                   const cdap_rib::filt_info_t &filt,
                                   int message_id) = 0;
  virtual void remote_cancel_read_request(const cdap_rib::con_handle_t &con,
                                          const cdap_rib::obj_info_t &obj,
                                          const cdap_rib::filt_info_t &filt,
                                          int message_id) = 0;
  virtual void remote_write_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt,
                                    int message_id) = 0;
  virtual void remote_start_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt,
                                    int message_id) = 0;
  virtual void remote_stop_request(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::obj_info_t &obj,
                                   const cdap_rib::filt_info_t &filt,
                                   int message_id) = 0;
};

// Interface of the RIB to be used from the application
class RIBDNorthInterface
{
 public:
  virtual ~RIBDNorthInterface()
  {
  }
  ;

  /// Create or update an object in the RIB
  virtual void createObject(const cdap_rib::obj_info_t &obj) = 0;

  /// Delete an object from the RIB
  virtual void deleteObject(const cdap_rib::obj_info_t &obj) = 0;

  /// Read an object from the RIB
  virtual cdap_rib::obj_info_t* readObject(const std::string& clas,
                                           const std::string& name) = 0;

  /// Read an object from the RIB
  virtual cdap_rib::obj_info_t* readObject(const std::string& clas,
                                           long instance) = 0;

  /// Update the value of an object in the RIB
  virtual void writeObject(const cdap_rib::obj_info_t &obj) = 0;

  /// Start an object at the RIB
  virtual void startObject(const cdap_rib::obj_info_t &obj) = 0;

  /// Stop an object at the RIB
  virtual void stopObject(const cdap_rib::obj_info_t &obj) = 0;
};

class EncoderInterface
{

};

/// Contains the data of an object in the RIB
class RIBObjectData
{
 public:
  RIBObjectData();
  RIBObjectData(std::string clazz, std::string name, unsigned long instance,
                std::string disp_value);
  virtual ~RIBObjectData()
  {
  }
  ;
  bool operator==(const RIBObjectData &other) const;
  bool operator!=(const RIBObjectData &other) const;
  const std::string& get_class() const;
  unsigned long get_instance() const;
  const std::string& get_name() const;
  virtual const std::string& get_displayable_value() const;
 private:
  /** The class (type) of object */
  std::string class_;
  /** The name of the object (unique within a class)*/
  std::string name_;
  /** A synonim for clazz+name (unique within the RIB) */
  unsigned long instance_;
  /**
   * The value of the object, encoded in an string for
   * displayable purposes
   */
  std::string displayable_value_;
};

class RIBDObjectInterface;
/// Base RIB Object. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
class RIBObject
{
 public:
  RIBObject(RIBDObjectInterface *rib_daemon, const std::string& clas,
            long instance, std::string name);
  virtual ~RIBObject();
  virtual std::string get_displayable_value();
  virtual void* get_value() const = 0;
  // FIXME fix object data displayable
  RIBObjectData* get_data();

  /// Local invocations
  virtual bool createObject(const std::string& clas, const std::string& name,
                            const void* value);
  virtual bool deleteObject(const void* value);
  virtual RIBObject * readObject();
  virtual bool writeObject(const void* value);
  virtual bool startObject(const void* object);
  virtual bool stopObject(const void* object);

  /// Remote invocations, resulting from CDAP messages
  virtual bool remoteCreateObject(const std::string& name, void * value);
  virtual bool remoteDeleteObject(const std::string& name, void * value);
  virtual bool remoteReadObject(const std::string& name, void * value);
  virtual bool remoteCancelReadObject(const std::string& name, void * value);
  virtual bool remoteWriteObject(const std::string& name, void * value);
  virtual bool remoteStartObject(const std::string& name, void * value);
  virtual bool remoteStopObject(const std::string& name, void * value);
  const std::string& get_class() const;
  const std::string& get_name() const;
  const std::string& get_parent_name() const;
  const std::string& get_parent_class() const;
  long get_instance() const;
  void get_children_value(
      std::list<std::pair<std::string, const void *> > &values) const;
  unsigned int get_children_size() const;
 private:
  void set_parent(RIBObject* parent);
  void add_child(RIBObject *child);
  void remove_child(const std::string& name);
  /// Auxiliar functions
  void operation_not_supported();

  std::string class_;
  std::string name_;
  unsigned long instance_;
  RIBObject *parent_;
  RIBDObjectInterface *rib_daemon_;
  std::list<RIBObject*> children_;
  EncoderInterface *encoder;
};

// RIB daemon Interface to be used by RIBObjects
class RIBDObjectInterface
{
 public:
  virtual ~RIBDObjectInterface()
  {
  }
  ;
  virtual void addRIBObject(RIBObject *ribObject) = 0;
  virtual void removeRIBObject(RIBObject *ribObject) = 0;
  virtual void removeRIBObject(const std::string& objectName) = 0;
};

class RIBDInterface : public RIBDNorthInterface, RIBDSouthInterface,
    RIBDObjectInterface
{
};

/**
 * RIB library result codes
 */
enum rib_schema_res
{
  RIB_SUCCESS,
  /* The RIB schema file extension is unknown */
  RIB_SCHEMA_EXT_ERR = -3,
  /* Error during RIB scheema file parsing */
  RIB_SCHEMA_FORMAT_ERR = -4,
  /* General validation error (unknown) */
  RIB_SCHEMA_VALIDATION_ERR = -5,
  /* Validation error, missing mandatory object */
  RIB_SCHEMA_VAL_MAN_ERR = -6,
//
// Misc
//
//TODO: Other error codes
};

class RIBSchemaObject
{
 public:
  RIBSchemaObject(const std::string& class_name, const bool mandatory,
                  const unsigned max_objs);
  void addChild(RIBSchemaObject *object);
  const std::string& get_class_name() const;
  unsigned get_max_objs() const;
 private:
  std::string class_name_;
  RIBSchemaObject *parent_;
  std::list<RIBSchemaObject*> children_;
  bool mandatory_;
  unsigned max_objs_;
};

class RIBSchema
{
 public:
  friend class RIB;
  RIBSchema(const cdap_rib::vers_info_t &version, char separator);
  rib_schema_res ribSchemaDefContRelation(const std::string& cont_class_name,
                                          const std::string& class_name,
                                          const bool mandatory,
                                          const unsigned max_objs);
  char get_separator() const;
 private:
  bool validateAddObject(const RIBObject* obj);
  bool validateRemoveObject(const RIBObject* obj, const RIBObject* parent);
  cdap_rib::vers_info_t version_;
  std::map<std::string, RIBSchemaObject*> rib_schema_;
  char separator_;
};

class RIBDFactory
{
 public:
  RIBDInterface* create(cacep::AppConHandlerInterface* app_callback,
                        ResponseHandlerInterface* app_resp_callbak,
                        const std::string &comm_protocol, void* comm_params,
                        cdap_rib::version_info version, char separator);
};
}

#endif /* RIB_PROVIDER_H_ */
