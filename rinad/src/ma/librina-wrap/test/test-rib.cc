//
// Test-CDAP
//
//	  Bernat Gaston			<bernat.gaston@i2cat.net>
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

#include <string>
#include <iostream>
#include "../rib_provider.h"
#include <librina/patterns.h>
#include <librina/application.h>

class Checker_
{
 public:
  bool check;
};
Singleton<Checker_> Checker;

class IdFactory
{
 public:
  IdFactory()
  {
    id_ = 0;
  }
  unsigned int get_id()
  {
    id_++;
    return id_;
  }
 private:
  unsigned int id_;
};

class IntEncoder : public rib::EncoderInterface<int> {
 public:
  const rib::SerializedObject* encode(const int &object)
  {
    (void) object;
    return 0;
  }
  int* decode(const rib::SerializedObject &serialized_object) const
  {
    (void) serialized_object;
    return 0;
  }
};

class IntRIBObject : public rib::RIBObject<int>
{
 public:
  IntRIBObject(const std::string& clas, std::string name, IdFactory &instance_gen, int* value, IntEncoder *encoder) :
    RIBObject(clas, instance_gen.get_id(), name, value, encoder)
  {}
  cdap_rib::res_info_t* remoteCreateObject(const std::string& name, void* value)
  {
    (void) name;
    (void) value;
    cdap_rib::res_info_t *res = new cdap_rib::res_info_t;
    res->result_ = 1;
    res->result_reason_ = "Ok";
    Checker->check = true;
    return res;
  }
  cdap_rib::res_info_t* remoteDeleteObject(const std::string& name, void* value)
  {
    (void) name;
    (void) value;
    cdap_rib::res_info_t *res = new cdap_rib::res_info_t;
    res->result_ = 1;
    res->result_reason_ = "Ok";
    Checker->check = true;
    return res;
  }
  cdap_rib::res_info_t* remoteReadObject(const std::string& name, void* value)
  {
    (void) name;
    (void) value;
    cdap_rib::res_info_t *res = new cdap_rib::res_info_t;
    res->result_ = 1;
    res->result_reason_ = "Ok";
    Checker->check = true;
    return res;
  }
  cdap_rib::res_info_t* remoteCancelReadObject(const std::string& name, void * value)
  {
    (void) name;
    (void) value;
    cdap_rib::res_info_t *res = new cdap_rib::res_info_t;
    res->result_ = 1;
    res->result_reason_ = "Ok";
    Checker->check = true;
    return res;
  }
  cdap_rib::res_info_t* remoteWriteObject(const std::string& name, void* value)
  {
    (void) name;
    (void) value;
    cdap_rib::res_info_t *res = new cdap_rib::res_info_t;
    res->result_ = 1;
    res->result_reason_ = "Ok";
    Checker->check = true;
    return res;
  }
  cdap_rib::res_info_t* remoteStartObject(const std::string& name, void* value)
  {
    (void) name;
    (void) value;
    cdap_rib::res_info_t *res = new cdap_rib::res_info_t;
    res->result_ = 1;
    res->result_reason_ = "Ok";
    Checker->check = true;
    return res;
  }
  cdap_rib::res_info_t* remoteStopObject(const std::string& name, void* value)
  {
    (void) name;
    (void) value;
    cdap_rib::res_info_t *res = new cdap_rib::res_info_t;
    res->result_ = 1;
    res->result_reason_ = "Ok";
    Checker->check = true;
    return res;
  }
};


class CheckSchema
{
 public:
  static const int RIB_MODEL = 11;
  static const int RIB_MAJ_VERSION = 1;
  static const int RIB_MIN_VERSION = 0;
  static const int RIB_ENCODING = 16;
  CheckSchema(rib::RIBSchema *rib_schema)
  {
    rib_schema_ = rib_schema;
  }
  bool RIBSchema_populateSchema()
  {
    if (rib_schema_->ribSchemaDefContRelation("", "ROOT", true, 1)
        != rib::RIB_SUCCESS) {
      return false;
    }
    if (rib_schema_->ribSchemaDefContRelation("ROOT", "A", false, 2)
        != rib::RIB_SUCCESS) {
      return false;
    }
    if (rib_schema_->ribSchemaDefContRelation("A", "Barcelona", false, 10)
        != rib::RIB_SUCCESS) {
      return false;
    }
    if (rib_schema_->ribSchemaDefContRelation("Barcelona", "1", false, 10)
        != rib::RIB_SUCCESS) {
      return false;
    }
    if (rib_schema_->ribSchemaDefContRelation("1", "test1", false, 10)
        != rib::RIB_SUCCESS) {
      return false;
    }
    if (rib_schema_->ribSchemaDefContRelation("1", "test2", false, 10)
        != rib::RIB_SUCCESS) {
      return false;
    }
    if (rib_schema_->ribSchemaDefContRelation("test2", "test3", false, 10)
        != rib::RIB_SUCCESS) {
      return false;
    }
    return true;
  }
 private:
  rib::RIBSchema *rib_schema_;
};

static const std::string OBJECT1_RIB_OBJECT_NAME = "A = 1";
static const std::string OBJECT1_RIB_OBJECT_CLASS = "A";
static const std::string OBJECT2_RIB_OBJECT_NAME = "A= 1, Barcelona";
static const std::string OBJECT2_RIB_OBJECT_CLASS = "Barcelona";
static const std::string OBJECT3_RIB_OBJECT_NAME = "A = 1, Barcelona, 1 = 1";
static const std::string OBJECT3_RIB_OBJECT_CLASS = "1";
static const std::string OBJECT4_RIB_OBJECT_NAME = "A = 1, Barcelona, 1 = 2";
static const std::string OBJECT4_RIB_OBJECT_CLASS = "1";
static const std::string OBJECT5_RIB_OBJECT_NAME =
    "A=1, Barcelona, 1 = 1, test1";
static const std::string OBJECT5_RIB_OBJECT_CLASS = "test1";
static const std::string OBJECT6_RIB_OBJECT_NAME =
    "A = 1, Barcelona, 1 = 2, test2 = 1";
static const std::string OBJECT6_RIB_OBJECT_CLASS = "test2";
static const std::string OBJECT7_RIB_OBJECT_NAME =
    "A = 1, Barcelona, 1 = 2, test2=1, test3";
static const std::string OBJECT7_RIB_OBJECT_CLASS = "test3";
static const std::string OBJECT8_RIB_OBJECT_NAME = "A = 1, B = 1, C= 1";
static const std::string OBJECT8_RIB_OBJECT_CLASS = "C";

class ConHandler : public cacep::AppConHandlerInterface
{
  void connect(int message_id, const cdap_rib::con_handle_t &con)
  {
    (void) message_id;
    (void) con;
  }
  void connectResponse(const cdap_rib::res_info_t &res,
                               const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
  }
  void release(int message_id, const cdap_rib::con_handle_t &con)
  {
    (void) message_id;
    (void) con;
  }
  void releaseResponse(const cdap_rib::res_info_t &res,
                               const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
  }
};


class RespHandler : public rib::ResponseHandlerInterface
{
  void createResponse(const cdap_rib::res_info_t &res,
                              const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
    Checker->check = true;
  }
  void deleteResponse(const cdap_rib::res_info_t &res,
                              const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
    Checker->check = true;
  }
  void readResponse(const cdap_rib::res_info_t &res,
                            const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
    Checker->check = true;
  }
  void cancelReadResponse(const cdap_rib::res_info_t &res,
                                  const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
    Checker->check = true;
  }
  void writeResponse(const cdap_rib::res_info_t &res,
                             const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
    Checker->check = true;
  }
  void startResponse(const cdap_rib::res_info_t &res,
                             const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
    Checker->check = true;
  }
  void stopResponse(const cdap_rib::res_info_t &res,
                            const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
    Checker->check = true;
  }
};


class CheckRIB
{
 public:
  CheckRIB()
  {
    IntEncoder *enc = new IntEncoder();
    rib::RIBDFactory daemon_factory;
    cdap_rib::cdap_params_t *params = new cdap_rib::cdap_params_t;
    params->is_IPCP_ = false;
    params->timeout_ = 2000;
    cdap_rib::version_info *version = new cdap_rib::version_info;
    rib_daemon_ = daemon_factory.create(new ConHandler(), new RespHandler(), "GPB", (void*) params, version, ',');
    object1_ = new IntRIBObject(OBJECT1_RIB_OBJECT_CLASS,
                               OBJECT1_RIB_OBJECT_NAME, idFactory_, new int(1), enc);
    object2_ = new IntRIBObject(OBJECT2_RIB_OBJECT_CLASS,
                               OBJECT2_RIB_OBJECT_NAME, idFactory_, new int(2), enc);
    object3_ = new IntRIBObject(OBJECT3_RIB_OBJECT_CLASS,
                               OBJECT3_RIB_OBJECT_NAME, idFactory_, new int(3), enc);
    object4_ = new IntRIBObject(OBJECT4_RIB_OBJECT_CLASS,
                               OBJECT4_RIB_OBJECT_NAME, idFactory_, new int(4), enc);
    object5_ = new IntRIBObject(OBJECT5_RIB_OBJECT_CLASS,
                               OBJECT5_RIB_OBJECT_NAME, idFactory_, new int(5), enc);
    object6_ = new IntRIBObject(OBJECT6_RIB_OBJECT_CLASS,
                               OBJECT6_RIB_OBJECT_NAME, idFactory_, new int(6), enc);
    object7_ = new IntRIBObject(OBJECT7_RIB_OBJECT_CLASS,
                               OBJECT7_RIB_OBJECT_NAME, idFactory_, new int(7), enc);
    object8_ = new IntRIBObject(OBJECT8_RIB_OBJECT_CLASS,
                               OBJECT8_RIB_OBJECT_NAME, idFactory_, new int(8), enc);
  };
  ~CheckRIB()
  {};
  bool RIBDaemon_addRIBObject_create_true()
  {
    try {
      rib_daemon_->addRIBObject(object1_);
      rib_daemon_->addRIBObject(object2_);
      rib_daemon_->addRIBObject(object3_);
      rib_daemon_->addRIBObject(object4_);
      rib_daemon_->addRIBObject(object5_);
      rib_daemon_->addRIBObject(object6_);
      rib_daemon_->addRIBObject(object7_);
    } catch (Exception &e1) {
      std::cout << "Failed with error: " << e1.what() << std::endl;
      return false;
    }
    return true;
  };
  bool RIBDaemon_addRIBObject_objectWithoutParent()
  {
    try {
      rib_daemon_->addRIBObject(object8_);
    } catch (Exception &e1) {
      std::cout << "Adding an object without parent. Failed with error: "
                << e1.what() << std::endl;
      return true;
    }
    return false;
  };
  bool RIBDaemon_addRIBObject_checkCreatedRelations()
  {
    try {
      rib_daemon_->addRIBObject(object2_);
      return false;
    } catch (Exception &e1) {
    }
    return true;
  };
  bool RIBDaemon_readObject()
  {
    try {
      IntRIBObject *object_recovered = (IntRIBObject*) rib_daemon_->getObject(
          OBJECT2_RIB_OBJECT_NAME, OBJECT2_RIB_OBJECT_CLASS);
      if (object2_ != object_recovered)
        return false;
      object_recovered = (IntRIBObject*) rib_daemon_->getObject(
          OBJECT5_RIB_OBJECT_NAME, OBJECT5_RIB_OBJECT_CLASS);
      if (object5_ != object_recovered)
        return false;
      object_recovered = (IntRIBObject*) rib_daemon_->getObject(
          OBJECT3_RIB_OBJECT_NAME, OBJECT3_RIB_OBJECT_CLASS);
    } catch (Exception &e1) {
      std::cout << e1.what();
      return false;
    }
    return true;
  };
  bool RIBDaemon_remote_create_result()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::res_info_t res;
    res.result_ = 1;
    Checker->check = false;
    rib_daemon_->remote_create_result(con, res);
    if (Checker->check)
    {
      return true;
    }
    else
      return false;
  };
  bool RIBDaemon_remote_delete_result()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::res_info_t res;
    res.result_ = 1;
    Checker->check = false;
    rib_daemon_->remote_delete_result(con, res);
    if (Checker->check)
    {
      return true;
    }
    else
      return false;
  };
  bool RIBDaemon_remote_read_result()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::res_info_t res;
    res.result_ = 1;
    Checker->check = false;
    rib_daemon_->remote_read_result(con, res);
    if (Checker->check)
    {
      return true;
    }
    else
      return false;
  };
  bool RIBDaemon_remote_cancel_read_result()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::res_info_t res;
    res.result_ = 1;
    Checker->check = false;
    rib_daemon_->remote_cancel_read_result(con, res);
    if (Checker->check)
    {
      return true;
    }
    else
      return false;
  };
  bool RIBDaemon_remote_write_result()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::res_info_t res;
    res.result_ = 1;
    Checker->check = false;
    rib_daemon_->remote_write_result(con, res);
    if (Checker->check)
    {
      return true;
    }
    else
      return false;
  };
  bool RIBDaemon_remote_start_result()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::res_info_t res;
    res.result_ = 1;
    Checker->check = false;
    rib_daemon_->remote_start_result(con, res);
    if (Checker->check)
    {
      return true;
    }
    else
      return false;
  };
  bool RIBDaemon_remote_stop_result()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::res_info_t res;
    res.result_ = 1;
    Checker->check = false;
    rib_daemon_->remote_stop_result(con, res);
    if (Checker->check)
    {
      return true;
    }
    else
      return false;
  };
  bool RIBDaemon_remote_create_request()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::obj_info_t obj;
    obj.class_ = OBJECT3_RIB_OBJECT_CLASS;
    obj.name_ = OBJECT3_RIB_OBJECT_NAME;
    cdap_rib::filt_info_t filt;
    Checker->check = false;
    try{
      rib_daemon_->remote_create_request(con, obj, filt, 1);
    }
    catch(Exception &e1)
    {
      if (Checker->check)
        return true;
    }
    return false;
  };
  bool RIBDaemon_remote_delete_request()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::obj_info_t obj;
    obj.class_ = OBJECT3_RIB_OBJECT_CLASS;
    obj.name_ = OBJECT3_RIB_OBJECT_NAME;
    cdap_rib::filt_info_t filt;
    Checker->check = false;
    try{
      rib_daemon_->remote_delete_request(con, obj, filt, 1);
    }
    catch(Exception &e1)
    {
      if (Checker->check)
        return true;
    }
    return false;
  };
  bool RIBDaemon_remote_read_request()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::obj_info_t obj;
    obj.class_ = OBJECT3_RIB_OBJECT_CLASS;
    obj.name_ = OBJECT3_RIB_OBJECT_NAME;
    cdap_rib::filt_info_t filt;
    Checker->check = false;
    try{
      rib_daemon_->remote_read_request(con, obj, filt, 1);
    }
    catch(Exception &e1)
    {
      if (Checker->check)
        return true;
    }
    return false;
  };
  bool RIBDaemon_remote_cancel_read_request()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::obj_info_t obj;
    obj.class_ = OBJECT3_RIB_OBJECT_CLASS;
    obj.name_ = OBJECT3_RIB_OBJECT_NAME;
    cdap_rib::filt_info_t filt;
    Checker->check = false;
    try{
      rib_daemon_->remote_cancel_read_request(con, obj, filt, 1);
    }
    catch(Exception &e1)
    {
      if (Checker->check)
        return true;
    }
    return false;
  };
  bool RIBDaemon_remote_write_request()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::obj_info_t obj;
    obj.class_ = OBJECT3_RIB_OBJECT_CLASS;
    obj.name_ = OBJECT3_RIB_OBJECT_NAME;
    cdap_rib::filt_info_t filt;
    Checker->check = false;
    try{
      rib_daemon_->remote_write_request(con, obj, filt, 1);
    }
    catch(Exception &e1)
    {
      if (Checker->check)
        return true;
    }
    return false;
  };
  bool RIBDaemon_remote_start_request()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::obj_info_t obj;
    obj.class_ = OBJECT3_RIB_OBJECT_CLASS;
    obj.name_ = OBJECT3_RIB_OBJECT_NAME;
    cdap_rib::filt_info_t filt;
    Checker->check = false;
    try{
      rib_daemon_->remote_start_request(con, obj, filt, 1);
    }
    catch(Exception &e1)
    {
      if (Checker->check)
        return true;
    }
    return false;
  };
  bool RIBDaemon_remote_stop_request()
  {
    cdap_rib::con_handle_t con;
    cdap_rib::obj_info_t obj;
    obj.class_ = OBJECT3_RIB_OBJECT_CLASS;
    obj.name_ = OBJECT3_RIB_OBJECT_NAME;
    cdap_rib::filt_info_t filt;
    Checker->check = false;
    try{
      rib_daemon_->remote_stop_request(con, obj, filt, 1);
    }
    catch(Exception &e1)
    {
      if (Checker->check)
        return true;
    }
    return false;
  };
 private:
  IdFactory idFactory_;
  rib::RIBDInterface *rib_daemon_;
  IntRIBObject *object1_;
  IntRIBObject *object2_;
  IntRIBObject *object3_;
  IntRIBObject *object4_;
  IntRIBObject *object5_;
  IntRIBObject *object6_;
  IntRIBObject *object7_;
  IntRIBObject *object8_;
};


int main()
{
  CheckRIB checks_rib;
  /*
   rina::RIBSchema rib_schema(version);

   CheckSchema checks_schema(0);

   std::cout<<std::endl <<	"///////////////////////////////////////" << std::endl <<
   "// test-rib TEST RIBSchema: creation //" << std::endl <<
   "///////////////////////////////////////" << std::endl;

   if (!checks_schema.RIBSchema_populateSchema_true()){
   std::cout<<"TEST FAILED"<<std::endl;
   return -1;
   }
   */
  std::cout << std::endl << "///////////////////////////////////////"
            << std::endl << "/// test-rib TEST 2 : construct RIB ///"
            << std::endl << "///////////////////////////////////////"
            << std::endl;

  if (!checks_rib.RIBDaemon_addRIBObject_create_true()) {
    std::cout << "TEST IBDaemon_addRIBObject_create_true FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_addRIBObject_objectWithoutParent()) {
    std::cout << "TEST RIBDaemon_addRIBObject_objectWithoutParent FAILED" << std::endl;
    return -1;
  }

  std::cout << std::endl << "///////////////////////////////////////"
            << std::endl << "// test-rib TEST 3 : check relations //"
            << std::endl << "///////////////////////////////////////"
            << std::endl;
  if (!checks_rib.RIBDaemon_addRIBObject_checkCreatedRelations()) {
    std::cout << "TEST RIBDaemon_addRIBObject_checkCreatedRelations FAILED" << std::endl;
    return -1;
  }

  std::cout << std::endl << "///////////////////////////////////////"
            << std::endl << "//// test-rib TEST 3 : readObjects ////"
            << std::endl << "///////////////////////////////////////"
            << std::endl;
  if (!checks_rib.RIBDaemon_readObject()) {
    std::cout << "TEST RIBDaemon_readObject FAILED" << std::endl;
    return -1;
  }

  std::cout << std::endl << "///////////////////////////////////////"
            << std::endl << "//// test-rib TEST 4 : South Int //////"
            << std::endl << "///////////////////////////////////////"
            << std::endl;
  if (!checks_rib.RIBDaemon_remote_create_result()) {
    std::cout << "TEST RIBDaemon_remote_create_result FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_delete_result()) {
    std::cout << "TEST RIBDaemon_remote_delete_result FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_read_result()) {
    std::cout << "TEST RIBDaemon_remote_read_result FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_cancel_read_result()) {
    std::cout << "TEST RIBDaemon_remote_cancel_read_result FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_write_result()) {
    std::cout << "TEST RIBDaemon_remote_write_result FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_start_result()) {
    std::cout << "TEST RIBDaemon_remote_start_result FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_stop_result()) {
    std::cout << "TEST RIBDaemon_remote_stop_result FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_create_request()) {
    std::cout << "TEST RIBDaemon_remote_create_request FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_delete_request()) {
    std::cout << "TEST RIBDaemon_remote_delete_request FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_read_request()) {
    std::cout << "TEST RIBDaemon_remote_read_request FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_cancel_read_request()) {
    std::cout << "TEST RIBDaemon_remote_cancel_read_request FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_write_request()) {
    std::cout << "TEST RIBDaemon_remote_write_request FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_start_request()) {
    std::cout << "TEST RIBDaemon_remote_start_request FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_remote_stop_request()) {
    std::cout << "TEST RIBDaemon_remote_stop_request FAILED" << std::endl;
    return -1;
  }

  // REMOVE object twice

  return 0;
}
