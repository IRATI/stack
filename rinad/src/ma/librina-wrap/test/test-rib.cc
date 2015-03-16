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

class EspecificEncoder : public rib::EncoderInterface {
 public:
  const rib::SerializedObject* encode(const void* object)
  {
    (void) object;
    return 0;
  }
  void* decode(
      const rib::ObjectValueInterface* serialized_object) const
  {
    (void) serialized_object;
    return 0;
  }
};

class NewObject : public rib::RIBObject
{
 public:
  NewObject(const std::string& clas, std::string name, IdFactory &instance_gen, rib::EncoderInterface *encoder) :
    rib::RIBObject(clas, instance_gen.get_id(), name, encoder)
  {}
  void* get_value() const
  {
    return 0;
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
  }
  void deleteResponse(const cdap_rib::res_info_t &res,
                              const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
  }
  void readResponse(const cdap_rib::res_info_t &res,
                            const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
  }
  void cancelReadResponse(const cdap_rib::res_info_t &res,
                                  const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
  }
  void writeResponse(const cdap_rib::res_info_t &res,
                             const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
  }
  void startResponse(const cdap_rib::res_info_t &res,
                             const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
  }
  void stopResponse(const cdap_rib::res_info_t &res,
                            const cdap_rib::con_handle_t &con)
  {
    (void) res;
    (void) con;
  }
};

class CheckRIB
{
 public:
  CheckRIB()
  {
    EspecificEncoder *enc = new EspecificEncoder();
    rib::RIBDFactory daemon_factory;
    cdap_rib::cdap_params_t *params = new cdap_rib::cdap_params_t;
    params->is_IPCP_ = false;
    params->timeout_ = 2000;
    cdap_rib::version_info *version = new cdap_rib::version_info;
    rib_daemon_ = daemon_factory.create(new ConHandler(), new RespHandler(), "GPB", (void*) params, version, ',');
    object1_ = new NewObject(OBJECT1_RIB_OBJECT_CLASS,
                               OBJECT1_RIB_OBJECT_NAME, idFactory_, enc);
    object2_ = new NewObject(OBJECT2_RIB_OBJECT_CLASS,
                               OBJECT2_RIB_OBJECT_NAME, idFactory_, enc);
    object3_ = new NewObject(OBJECT3_RIB_OBJECT_CLASS,
                               OBJECT3_RIB_OBJECT_NAME, idFactory_, enc);
    object4_ = new NewObject(OBJECT4_RIB_OBJECT_CLASS,
                               OBJECT4_RIB_OBJECT_NAME, idFactory_, enc);
    object5_ = new NewObject(OBJECT5_RIB_OBJECT_CLASS,
                               OBJECT5_RIB_OBJECT_NAME, idFactory_, enc);
    object6_ = new NewObject(OBJECT6_RIB_OBJECT_CLASS,
                               OBJECT6_RIB_OBJECT_NAME, idFactory_, enc);
    object7_ = new NewObject(OBJECT7_RIB_OBJECT_CLASS,
                               OBJECT7_RIB_OBJECT_NAME, idFactory_, enc);
    object8_ = new NewObject(OBJECT8_RIB_OBJECT_CLASS,
                               OBJECT8_RIB_OBJECT_NAME, idFactory_, enc);
  }
  ~CheckRIB()
  {}
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
  }
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
  }
  bool RIBDaemon_addRIBObject_checkCreatedRelations()
  {
    try {
      rib_daemon_->addRIBObject(object2_);
      return false;
    } catch (Exception &e1) {
    }
    return true;
  }
  bool RIBDaemon_readObject()
  {
    try {
      NewObject *object_recovered = (NewObject*) rib_daemon_->getObject(
          OBJECT2_RIB_OBJECT_CLASS, OBJECT2_RIB_OBJECT_NAME);
      if (object2_ != object_recovered)
        return false;
      object_recovered = (NewObject*) rib_daemon_->getObject(
          OBJECT5_RIB_OBJECT_CLASS, OBJECT5_RIB_OBJECT_NAME);
      if (object5_ != object_recovered)
        return false;
      object_recovered = (NewObject*) rib_daemon_->getObject(
          OBJECT3_RIB_OBJECT_CLASS, OBJECT3_RIB_OBJECT_NAME);
    } catch (Exception &e1) {
      std::cout << e1.what();
      return false;
    }
    return true;
  }
 private:
  IdFactory idFactory_;
  rib::RIBDInterface *rib_daemon_;
  NewObject *object1_;
  NewObject *object2_;
  NewObject *object3_;
  NewObject *object4_;
  NewObject *object5_;
  NewObject *object6_;
  NewObject *object7_;
  NewObject *object8_;
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
    std::cout << "TEST FAILED" << std::endl;
    return -1;
  }
  if (!checks_rib.RIBDaemon_addRIBObject_objectWithoutParent()) {
    std::cout << "TEST FAILED" << std::endl;
    return -1;
  }

  std::cout << std::endl << "///////////////////////////////////////"
            << std::endl << "// test-rib TEST 3 : check relations //"
            << std::endl << "///////////////////////////////////////"
            << std::endl;
  if (!checks_rib.RIBDaemon_addRIBObject_checkCreatedRelations()) {
    std::cout << "TEST FAILED" << std::endl;
    return -1;
  }

  std::cout << std::endl << "///////////////////////////////////////"
            << std::endl << "//// test-rib TEST 3 : readObjects ////"
            << std::endl << "///////////////////////////////////////"
            << std::endl;
  if (!checks_rib.RIBDaemon_readObject()) {
    std::cout << "TEST FAILED" << std::endl;
    return -1;
  }

  // REMOVE object twice

  return 0;
}
