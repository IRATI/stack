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
#include <algorithm>

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
                                         const cdap_rib::res_info_t &res) = 0;
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

template <class T>
class EncoderInterface
{
 public:
  virtual ~EncoderInterface()
  {
  }
  /// Converts an object to a byte array, if this object is recognized by the encoder
  /// @param object
  /// @throws exception if the object is not recognized by the encoder
  /// @return
  virtual const cdap_rib::SerializedObject* encode(const T &object) = 0;
  /// Converts a byte array to an object of the type specified by "className"
  /// @param byte[] serializedObject
  /// @param objectClass The type of object to be decoded
  /// @throws exception if the byte array is not an encoded in a way that the encoder can recognize, or the
  /// byte array value doesn't correspond to an object of the type "className"
  /// @return
  virtual T* decode(const cdap_rib::SerializedObject &serialized_object) const = 0;
};

/// Contains the data of an object in the RIB
class RIBObjectData
{
 public:
  RIBObjectData();
  RIBObjectData(std::string clas, std::string name, unsigned long instance,
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

class RIBDNorthInterface;
/// Base RIB Object. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
class BaseRIBObject
{
 public:
  virtual ~BaseRIBObject(){};
  virtual std::string get_displayable_value();
  // FIXME fix object data displayable
  virtual RIBObjectData* get_data();

  /// Local invocations
  virtual bool createObject(const std::string& clas, const std::string& name,
                            const void* value);
  virtual bool deleteObject(const void* value);
  virtual BaseRIBObject* readObject();
  virtual bool writeObject(const void* value);
  virtual bool startObject(const void* object);
  virtual bool stopObject(const void* object);

  /// Remote invocations, resulting from CDAP messages
  virtual cdap_rib::res_info_t* remoteCreateObject(const std::string& name,
                                                   void* value);
  virtual cdap_rib::res_info_t* remoteDeleteObject(const std::string& name,
                                                   void* value);
  virtual cdap_rib::res_info_t* remoteReadObject(const std::string& name,
                                                 void* value);
  virtual cdap_rib::res_info_t* remoteCancelReadObject(const std::string& name,
                                                       void * value) ;
  virtual cdap_rib::res_info_t* remoteWriteObject(const std::string& name,
                                                  void* value);
  virtual cdap_rib::res_info_t* remoteStartObject(const std::string& name,
                                                  void* value);
  virtual cdap_rib::res_info_t* remoteStopObject(const std::string& name,
                                                 void* value);
  virtual const std::string& get_class() const;
  virtual const std::string& get_name() const;
  virtual long get_instance() const;
 protected:
  std::string class_;
  std::string name_;
  unsigned long instance_;
 private:
  void operation_not_supported();
};


/// Base RIB Object. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
template<class T>
class RIBObject : public BaseRIBObject
{
 public:
  RIBObject(const std::string& clas, long instance, std::string name,
            T* value, EncoderInterface<T> *encoder)
  {
    name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end());
    name_ =  name;
    class_ = clas;
    instance_ = instance;
    value_ = value;
    encoder_ = encoder;
  }
  RIBObject(const std::string& clas, long instance, std::string name,
            cdap_rib::SerializedObject* value, EncoderInterface<T> *encoder)
  {
    name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end());
    name_ =  name;
    class_ = clas;
    instance_ = instance;
    encoder_ = encoder;
    value_ = encoder->decode(value);
  }
  virtual ~RIBObject()
  {}
  const T* get_value() const
  {
    return value_;
  }
 protected:
  T* value_;
 private:
  EncoderInterface<T> *encoder_;
};

// RIB daemon Interface to be used by RIBObjects
class RIBDNorthInterface
{
 public:
  virtual ~RIBDNorthInterface()
  {
  }
  ;
  virtual void addRIBObject(BaseRIBObject *ribObject) = 0;
  virtual void removeRIBObject(BaseRIBObject *ribObject) = 0;
  virtual void removeRIBObject(const std::string& name) = 0;
  virtual BaseRIBObject* getObject(const std::string& name,
                               const std::string& clas) const = 0;
  virtual BaseRIBObject* getObject(unsigned long instance,
                               const std::string& clas) const = 0;
};

class RIBDInterface : public RIBDNorthInterface, public RIBDSouthInterface
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
  RIBSchema(const cdap_rib::vers_info_t *version, char separator);
  ~RIBSchema();
  rib_schema_res ribSchemaDefContRelation(const std::string& cont_class_name,
                                          const std::string& class_name,
                                          const bool mandatory,
                                          const unsigned max_objs);
  char get_separator() const;
 private:
  bool validateAddObject(const BaseRIBObject* obj);
  bool validateRemoveObject(const BaseRIBObject* obj, const BaseRIBObject* parent);
  const cdap_rib::vers_info_t *version_;
  std::map<std::string, RIBSchemaObject*> rib_schema_;
  char separator_;
};

class RIBDFactory
{
 public:
  RIBDInterface* create(cacep::AppConHandlerInterface* app_callback,
                        ResponseHandlerInterface* app_resp_callbak,
                        const std::string &comm_protocol, void* comm_params,
                        const cdap_rib::version_info *version, char separator);
};

class IntEncoder : public rib::EncoderInterface<int> {
 public:
  const cdap_rib::SerializedObject* encode(const int &object);
  int* decode(const cdap_rib::SerializedObject &serialized_object) const;
};

class SIntEncoder : public rib::EncoderInterface<short int> {
 public:
  const cdap_rib::SerializedObject* encode(const short int &object);
  short int* decode(const cdap_rib::SerializedObject &serialized_object) const;
};

class LongEncoder : public rib::EncoderInterface<long long> {
 public:
  const cdap_rib::SerializedObject* encode(const long long &object);
  long long* decode(const cdap_rib::SerializedObject &serialized_object) const;
};

class SLongEncoder : public rib::EncoderInterface<long> {
 public:
  const cdap_rib::SerializedObject* encode(const long &object);
  long* decode(const cdap_rib::SerializedObject &serialized_object) const;
};

class StringEncoder : public rib::EncoderInterface<std::string> {
 public:
  const cdap_rib::SerializedObject* encode(const std::string &object);
  std::string* decode(const cdap_rib::SerializedObject &serialized_object) const;
};

class FloatEncoder : public rib::EncoderInterface<float> {
 public:
  const cdap_rib::SerializedObject* encode(const float &object);
  float* decode(const cdap_rib::SerializedObject &serialized_object) const;
};

class DoubleEncoder : public rib::EncoderInterface<double> {
 public:
  const cdap_rib::SerializedObject* encode(const double &object);
  double* decode(const cdap_rib::SerializedObject &serialized_object) const;
};

class BoolEncoder : public rib::EncoderInterface<bool> {
 public:
  const cdap_rib::SerializedObject* encode(const bool &object);
  bool* decode(const cdap_rib::SerializedObject &serialized_object) const;
};

class IntRIBObject : public rib::RIBObject<int>
{
 public:
  IntRIBObject(const std::string& clas, std::string name, long instance, int* value, IntEncoder *encoder) :
    RIBObject(clas, instance, name, value, encoder)
  {}
};

class SIntRIBObject : public rib::RIBObject<short int>
{
 public:
  SIntRIBObject(const std::string& clas, std::string name, long instance, short int* value, SIntEncoder *encoder) :
    RIBObject(clas, instance, name, value, encoder)
  {}
};

class SLongRIBObject : public rib::RIBObject<long>
{
 public:
  SLongRIBObject(const std::string& clas, std::string name, long instance, long* value, SLongEncoder *encoder) :
    RIBObject(clas, instance, name, value, encoder)
  {}
};

class LongRIBObject : public rib::RIBObject<long long>
{
 public:
  LongRIBObject(const std::string& clas, std::string name, long instance, long long* value, LongEncoder *encoder) :
    RIBObject(clas, instance, name, value, encoder)
  {}
};

class StringRIBObject : public rib::RIBObject<std::string>
{
 public:
  StringRIBObject(const std::string& clas, std::string name, long instance, std::string* value, StringEncoder *encoder) :
    RIBObject(clas, instance, name, value, encoder)
  {}
};

class FloatRIBObject : public rib::RIBObject<float>
{
 public:
  FloatRIBObject(const std::string& clas, std::string name, long instance, float* value, FloatEncoder *encoder) :
    RIBObject(clas, instance, name, value, encoder)
  {}
};

class DoubleRIBObject : public rib::RIBObject<double>
{
 public:
  DoubleRIBObject(const std::string& clas, std::string name, long instance, double* value, DoubleEncoder *encoder) :
    RIBObject(clas, instance, name, value, encoder)
  {}
};

class BoolRIBObject : public rib::RIBObject<bool>
{
 public:
  BoolRIBObject(const std::string& clas, std::string name, long instance, bool* value, BoolEncoder *encoder) :
    RIBObject(clas, instance, name, value, encoder)
  {}
};

}

#endif /* RIB_PROVIDER_H_ */
