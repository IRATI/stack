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
#include "librina/rib.h"


class RIBDaemonSpecific : public rina::RIBDaemon {
public:
	RIBDaemonSpecific(const rina::RIBSchema *rib_schema): rina::RIBDaemon(rib_schema){}
    void sendMessageSpecific(bool useAddress, const rina::CDAPMessage & cdapMessage, int sessionId,
                    unsigned int address, rina::ICDAPResponseMessageHandler* cdapMessageHandler) {
    }
};

class IdFactory {
public:
	IdFactory(){
		id_ = 0;
	}
	unsigned int get_id(){
		id_++;
		return id_;
	}
private:
	unsigned int id_;
};

class NewObject: public rina::BaseRIBObject {
public:
	NewObject(RIBDaemonSpecific *rib_daemon,
			std::string object_class, std::string object_name,
			unsigned int id) :
		rina::BaseRIBObject(rib_daemon, object_class, id, object_name)
	{}
    const void* get_value() const {
    	return 0;
    }
};

NewObject* createNewObject(RIBDaemonSpecific *rib_daemon,
		std::string object_class, std::string object_name, IdFactory &idFactory) {

	NewObject *obj = new NewObject(rib_daemon, object_class, object_name, idFactory.get_id());
	return obj;
}

class CheckSchema{
public:
	static const int RIB_MODEL = 11;
	static const int RIB_MAJ_VERSION = 1;
	static const int RIB_MIN_VERSION = 0;
	static const int RIB_ENCODING = 16;
	CheckSchema(rina::RIBSchema *rib_schema){
		rib_schema_ = rib_schema;
	}
	bool RIBSchema_populateSchema_true() {
		if (rib_schema_->ribSchemaDefContRelation("", "ROOT", "root", true, 1) != rina::RIB_SUCCESS){
			return false;
		}
		if (rib_schema_->ribSchemaDefContRelation("ROOT", "A", "A", false, 2) != rina::RIB_SUCCESS){
			return false;
		}
		if (rib_schema_->ribSchemaDefContRelation("A", "Barcelona", "A, Barcelona", false, 10) != rina::RIB_SUCCESS){
			return false;
		}
		if (rib_schema_->ribSchemaDefContRelation("Barcelona", "1", "A, Barcelona, 1", false, 10) != rina::RIB_SUCCESS){
			return false;
		}
		if (rib_schema_->ribSchemaDefContRelation("1", "test1", "A, Barcelona, 1, test1", false, 10) != rina::RIB_SUCCESS){
			return false;
		}
		if (rib_schema_->ribSchemaDefContRelation("1", "test2", "A, Barcelona, 1, test2", false, 10) != rina::RIB_SUCCESS){
			return false;
		}
		if (rib_schema_->ribSchemaDefContRelation("test2", "test3", "A,Barcelona, 1, test2, test3", false, 10) != rina::RIB_SUCCESS){
			return false;
		}
		return true;
	}
private:
	rina::RIBSchema *rib_schema_;
};

class CheckRIB {
public:
	CheckRIB(const rina::RIBSchema *rib_schema){
		rib_daemon_ = new RIBDaemonSpecific(rib_schema);
		object1_ = createNewObject(rib_daemon_,  "A", "A", idFactory_);
	 	object2_ = createNewObject(rib_daemon_, "Barcelona", "A= 1, Barcelona", idFactory_);
		object3_ = createNewObject(rib_daemon_, "1", "A = 1, Barcelona = 1, 1", idFactory_);
		object4_ = createNewObject(rib_daemon_, "test1", "A=1, Barcelona = 1, 1 = 1, test1", idFactory_);
		object5_ = createNewObject(rib_daemon_, "test2", "A = 1, Barcelona = 1, 1 = 2, test2", idFactory_);
		object6_ = createNewObject(rib_daemon_, "test3", "A = 1, Barcelona = 1, 1 = 2, test2=1, test3", idFactory_);
	}
	~CheckRIB(){
		delete rib_daemon_;
	}
	bool RIBDaemon_addRIBObject_create_true(){
		try
		{
			rib_daemon_->addRIBObject(object1_);
			rib_daemon_->addRIBObject(object2_);
			rib_daemon_->addRIBObject(object3_);
			rib_daemon_->addRIBObject(object4_);
			rib_daemon_->addRIBObject(object5_);
			rib_daemon_->addRIBObject(object6_);
		}catch(Exception &e1)
		{
			std::cout<<"Failed with error: "<<e1.what()<<std::endl;
			return false;
		}
		return true;
	}
	bool RIBDaemon_addRIBObject_checkCreatedRelations_true(){
		if (object2_->get_parent_name() != object1_->get_name())
			return false;
		if (object3_->get_parent_name() != object2_->get_name())
			return false;
		if (object4_->get_parent_name() != object3_->get_name())
			return false;
		if (object5_->get_parent_name() != object3_->get_name())
			return false;
		if (object6_->get_parent_name() != object5_->get_name())
			return false;
		try {
			rib_daemon_->addRIBObject(object2_);
			return false;
		} catch (Exception &e1){}
		return true;
	}
	bool RIBDaemon_readObject_true(){
		try{
		NewObject *object_recovered = (NewObject*)rib_daemon_->readObject(object2_->get_class(), object2_->get_name());
		if (object2_ != object_recovered)
			return false;
		object_recovered = (NewObject*) rib_daemon_->readObject(object5_->get_class(), object5_->get_instance());
		if (object5_ != object_recovered)
			return false;
		}
		catch(Exception &e1)
		{
			return false;
		}
		return true;
	}
private:
	IdFactory idFactory_;
	RIBDaemonSpecific *rib_daemon_;
	NewObject *object1_;
	NewObject *object2_;
	NewObject *object3_;
	NewObject *object4_;
	NewObject *object5_;
	NewObject *object6_;
	NewObject *object7_;
};


int main() {

	rina::rib_ver_t version;
	version.model = CheckSchema::RIB_MODEL;
	version.major_version = CheckSchema::RIB_MAJ_VERSION;
	version.minor_version = CheckSchema::RIB_MIN_VERSION;
	version.encoding = CheckSchema::RIB_ENCODING;
	rina::RIBSchema *schema = new rina::RIBSchema(version, ',', '=');
	CheckRIB checks_rib(schema);
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
	std::cout<<std::endl <<	"///////////////////////////////////////" << std::endl <<
							"/// test-rib TEST 2 : construct RIB ///" << std::endl <<
							"///////////////////////////////////////" << std::endl;

	if (!checks_rib.RIBDaemon_addRIBObject_create_true()){
		std::cout<<"TEST FAILED"<<std::endl;
		return -1;
	}

	std::cout<<std::endl <<	"///////////////////////////////////////" << std::endl <<
							"// test-rib TEST 3 : check relations //" << std::endl <<
							"///////////////////////////////////////" << std::endl;
	if (!checks_rib.RIBDaemon_addRIBObject_checkCreatedRelations_true()){
		std::cout<<"TEST FAILED"<<std::endl;
		return -1;
	}

	std::cout<<std::endl <<	"///////////////////////////////////////" << std::endl <<
							"//// test-rib TEST 3 : readObjects ////" << std::endl <<
							"///////////////////////////////////////" << std::endl;
	if (!checks_rib.RIBDaemon_readObject_true()){
		std::cout<<"TEST FAILED"<<std::endl;
		return -1;
	}

	return 0;
}
