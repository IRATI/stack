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
		rina::BaseRIBObject(rib_daemon, object_class, 1, object_name) {
		id_ = id;
	}
    const void* get_value() const {
    	return 0;
    }
private:
	unsigned int id_;
};

NewObject* createNewObject(RIBDaemonSpecific *rib_daemon,
		std::string object_class, std::string object_name, IdFactory &idFactory) {

	NewObject *obj = new NewObject(rib_daemon, object_class, object_name, idFactory.get_id());
	return obj;
}

class ChecksSchema{
public:
	ChecksSchema(){
		rina::rib_ver_t version;
		rina::RIBSchema rib(version);
	}
};

class ChecksRIB {
public:
	ChecksRIB(){
		rib_daemon_ = new RIBDaemonSpecific();
		object1_ = createNewObject(rib_daemon_,  "A", "A", idFactory_);
	 	object2_ = createNewObject(rib_daemon_, "Barcelona", "A = 1, Barcelona", idFactory_);
		object3_ = createNewObject(rib_daemon_, "1", "A = 1, Barcelona = 1, 1", idFactory_);
		object4_ = createNewObject(rib_daemon_, "test1", "A = 1, Barcelona = 1, 1 = 1, test1", idFactory_);
		object5_ = createNewObject(rib_daemon_, "test2", "A = 1, Barcelona = 1, 1 = 2, test2", idFactory_);
		object6_ = createNewObject(rib_daemon_, "test3", "A = 1, Barcelona = 1, 1 = 2, test2=1, test3", idFactory_);
	}
	~ChecksRIB(){
		delete object1_;
	}
	bool constructRIB(){
		try
		{
			rib_daemon_->addRIBObject(object1_);
			rib_daemon_->addRIBObject(object2_);
		}catch(Exception &e1)
		{
			return false;
		}
		return true;
	}
	bool checkRelations(){
		if (object2_->get_parent_name() != object1_->get_name())
			return false;
		/*
		if (object5_->get_parent() != object3_)
			return false;
		try {
			object3_->add_child(object5_);
			return false;
		} catch (Exception &e1){
			std::cout<<"We try to add a child which is already a child: " <<e1.what()<<std::endl;
		}
		*/
		return true;
	}
	bool checkRemoval(){

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

};


int main() {

ChecksRIB checks;

bool test_result = true;

std::cout<<std::endl <<	"///////////////////////////////////////" << std::endl <<
						"/// test-rib TEST 2 : construct RIB ///" << std::endl <<
						"///////////////////////////////////////" << std::endl;

if (!checks.constructRIB()){
	std::cout<<"TEST FAILED"<<std::endl;
	test_result = false;
}

std::cout<<std::endl <<	"///////////////////////////////////////" << std::endl <<
						"// test-rib TEST 3 : check relations //" << std::endl <<
						"///////////////////////////////////////" << std::endl;
if (!checks.checkRelations()){
	std::cout<<"TEST FAILED"<<std::endl;
	test_result = false;
}


if (test_result)
	return 0;
else
	return -1;
}
