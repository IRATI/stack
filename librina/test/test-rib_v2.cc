//
// RIB v2 test suite
//
//	  Marc Sune <marc.sune (at) bisdn.de>
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
#include <iostream>
#include <stdlib.h>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "librina/cdap.h"
#include "librina/ipc-process.h"
#include "librina/common.h"
#include "librina/rib_v2.h"

using namespace rina;
using namespace rina::rib;

//fwd decl
class MyObj;

//RIB daemon proxy
static rina::rib::RIBDaemonProxy* ribd = NULL;
//Application Entity name
static const std::string ae ="ae_name";
//Version
cdap_rib::vers_info_t version;
//RIB handle
static rib_handle_t handle = 1234;
static rib_handle_t handle2;


class ribBasicOps : public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE( ribBasicOps );

	CPPUNIT_TEST( testInit );
	CPPUNIT_TEST( testCreation );
	CPPUNIT_TEST( testAssociation );
	CPPUNIT_TEST( testAddObj );
	CPPUNIT_TEST( testDeassociation );
	CPPUNIT_TEST( testDestruction );
	CPPUNIT_TEST( testFini );

	CPPUNIT_TEST_SUITE_END();

private:


public:
	void setUp();
	void tearDown();

	void testInit();
	void testCreation();
	void testAssociation();
	void testAddObj();
	void testDeassociation();
	void testDestruction();
	void testFini();
};

//Register
CPPUNIT_TEST_SUITE_REGISTRATION( ribBasicOps );

class remoteHandlers : public rina::rib::RIBOpsRespHandlers {

//
//Required mock ups
//
public:
	~remoteHandlers(){};

	void remoteCreateResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res){ (void)con; (void)obj; (void)res; };
	void remoteDeleteResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::res_info_t &res){ (void)con; (void)res; };
	void remoteReadResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res){ (void)con; (void)obj; (void)res; };
	void remoteCancelReadResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::res_info_t &res){ (void)con; (void)res; };
	void remoteWriteResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res){ (void)con; (void)obj; (void)res; };
	void remoteStartResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res){ (void)con; (void)obj; (void)res; };
	void remoteStopResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res){ (void)con; (void)obj; (void)res; };
};

static class remoteHandlers remote_handlers;

class AppHandlers : public rina::cacep::AppConHandlerInterface {

public:
	~AppHandlers(){};

	/// A remote Connect request has been received.
	void connect(int invoke_id, const cdap_rib::con_handle_t &con){ (void)con; (void)invoke_id;};
	/// A remote Connect response has been received.
	void connectResult(const cdap_rib::res_info_t &res,
			const cdap_rib::con_handle_t &con){ (void)con; (void)res; };
	/// A remote Release request has been received.
	void release(int invoke_id, const cdap_rib::con_handle_t &con){ (void)con; (void)invoke_id; };
	/// A remote Release response has been received.
	void releaseResult(const cdap_rib::res_info_t &res,
			const cdap_rib::con_handle_t &con){ (void)con; (void)res; };
};

static class AppHandlers app_handlers;


//
//
//

class MyObjEncoder: public Encoder<uint32_t> {

public:
	virtual ~MyObjEncoder(){}

	void encode(const uint32_t &obj, cdap_rib::ser_obj_t& serobj){
		//TODO fill in
		(void)obj;
		(void)serobj;
	};

	virtual void decode(const cdap_rib::ser_obj_t &serobj,
							uint32_t& des_obj){
		//TODO fill in
		(void)serobj;
		(void)des_obj;
	};
};

class MyObj : public RIBObj<uint32_t> {

public:
	MyObj(uint32_t initial_value) : RIBObj<uint32_t>(initial_value){};
	virtual ~MyObj(){};

	const std::string& get_class() const{
		return class_;
	}

	AbstractEncoder* get_encoder(){
		return &encoder;
	};

	MyObjEncoder encoder;
	static const std::string class_;
};

const std::string MyObj::class_ = "MyObj";

//
// End of mockups
//

//Objects
MyObj *obj1, *obj2, *obj3, *obj4;
std::string name1 = "/x";
std::string name2 = "/y";
std::string name3 = "/x/z";
std::string name4 = "/x/z/t";

//Setups

void ribBasicOps::setUp(){
}

void ribBasicOps::tearDown(){

}

void ribBasicOps::testInit(){
	rina::rib::RIBDaemonProxy* ribd_;
	cdap_rib::cdap_params params;
	params.is_IPCP_ = false;
	ribd = NULL;

	//Before init getting a proxy should throw an exception
	try{
		ribd = rina::rib::RIBDaemonProxyFactory();
	}catch(...){
		printf("Captured exception\n");
	}

	CPPUNIT_ASSERT_MESSAGE("Invalid ribdproxy before init", ribd == NULL);

	rina::rib::init(&app_handlers, &remote_handlers, params);

	//Double call to init should throw an exception
	try{
		rina::rib::init(&app_handlers, &remote_handlers, params);
		CPPUNIT_ASSERT_MESSAGE("Double call to init did not throw an exception", 0);
	}catch(...){}

	//Create the proxy
	ribd = rina::rib::RIBDaemonProxyFactory();
	CPPUNIT_ASSERT_MESSAGE("Unable to recover a valid proxy", ribd != NULL);

	ribd_ = rina::rib::RIBDaemonProxyFactory();
	CPPUNIT_ASSERT_MESSAGE("Unable to recover a second valid proxy", ribd_ != NULL);
	delete ribd_;
}

void ribBasicOps::testCreation(){

	version.version_ = 0x1;

		//Get a handle of an inexistent RIB
	try{
		handle = ribd->get(version, ae);
		CPPUNIT_ASSERT_MESSAGE("Got an invalid handle for an inexistent RIB", 0);
	}catch(eRIBNotFound& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Wrong exception thrown during get invalid RIB handle", 0);
	}

	//Create a schema (empty)
	try{
		ribd->createSchema(version);
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Exception throw during the creation of a schema", 0);
	}

	//try to recreate
	try{
		ribd->createSchema(version);
		CPPUNIT_ASSERT_MESSAGE("Exception not thrown during creation of a duplicate schema", 0);
	}catch(eSchemaExists& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Wrong exception thrown during creation of a dubplicate schema", 0);
	}

	//List versions and check if it exists
	std::list<cdap_rib::vers_info_t> vers;
	try{
		vers = ribd->listVersions();
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Exception thrown during listVersions method", 0);
	}

	//Check the result
	CPPUNIT_ASSERT_MESSAGE("Invalid list of versions length", vers.size() == 1);
	if(vers.size() == 1){
		CPPUNIT_ASSERT_MESSAGE("Schema has not been created with the right version or listVersions() has a bug", version.version_ == (*vers.begin()).version_);
	}

	//Create a RIB with an invalid schema
	try{
		cdap_rib::vers_info_t wrong_version;
		wrong_version.version_ = 0x11111;

		handle = ribd->createRIB(wrong_version);

		CPPUNIT_ASSERT_MESSAGE("Exception not thrown during creation of a RIB of an invalid version", 0);
	}catch(eSchemaNotFound& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid Exception thrown during creation of a RIB of an invalid version", 0);
	}

	//Create a valid RIB
	try{
		handle = ribd->createRIB(version);
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Exception thrown during creation of a RIB of a valid version", 0);
	}

	//Check handle
	CPPUNIT_ASSERT_MESSAGE("Invalid handle during valid RIB creation", handle == 1);

	//Create another valid RIB
	try{
		handle2 = ribd->createRIB(version);
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Exception thrown during creation of a RIB of a valid version", 0);
	}

	//Check handle
	CPPUNIT_ASSERT_MESSAGE("Invalid handle during valid RIB creation", handle2 == 2);
	CPPUNIT_ASSERT_MESSAGE("Invalid handle during valid RIB creation", handle2 != handle);

}

void ribBasicOps::testAssociation(){

	rib_handle_t tmp;
	rib_handle_t wrong_handle = 9999;

	//Associate an invalid RIB handle
	//Destroy an inexistent schema (not yet implemented)
	try{
		ribd->associateRIBtoAE(wrong_handle, ae);
		CPPUNIT_ASSERT_MESSAGE("Associate to with an invalid RIB handle succeeded", 0);
	}catch(eRIBNotFound& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during associate with an invalid RIB handle", 0);
	}

	//Perform a valid association
	try{
		ribd->associateRIBtoAE(handle, ae);
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Exception throw during associate with an valid RIB handle", 0);
	}

	//Check get with a valid version but invalid ae
	try{
		std::string wrong_ae = "fff";
		tmp = ribd->get(version, wrong_ae);
	}catch(eRIBNotFound& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during get() with a valid version but invalid ae_name", 0);
	}

	//Check get with a valid an invalid version but valid ae
	try{
		cdap_rib::vers_info_t wrong_version;
		wrong_version.version_ = 0x1234;
		tmp = ribd->get(wrong_version, ae);
	}catch(eRIBNotFound& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during get() with an invalid version but a valid ae_name", 0);
	}

	//Check get with a valid version and ae
	try{
		tmp = ribd->get(version, ae);
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Exception throw during get() with a valid version and ae_name", 0);
	}
	CPPUNIT_ASSERT_MESSAGE("Bug in get(); invalid handle returned", tmp == handle);

	//Retry with the same handle
	try{
		ribd->associateRIBtoAE(handle, ae);
		CPPUNIT_ASSERT_MESSAGE("Double associate with a valid RIB handle succeeded", 0);
	}catch(eRIBAlreadyAssociated& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during double associate with a valid RIB handle", 0);
	}

	//Retry with another handle (same AE+version)
	try{
		ribd->associateRIBtoAE(handle2, ae);
		CPPUNIT_ASSERT_MESSAGE("Associate with a valid RIB handle but with the same version (already registered) succeeded", 0);
	}catch(eRIBAlreadyAssociated& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during associate with a valid RIB handle but with the same version (already registered) succeeded", 0);
	}
}

void ribBasicOps::testAddObj(){

	MyObj* tmp;
	rib_handle_t wrong_handle = 9999;
	std::string invalid_name1 = "";
	std::string invalid_name2 = "x";
	std::string invalid_name3 = "/x/";
	int64_t inst_id1, inst_id2, inst_id3;

	obj1 = new MyObj(1);
	obj2 = new MyObj(2);
	obj3 = new MyObj(3);
	obj4 = new MyObj(4);

	//Attempt to add them into an invalid RIB handle
	try{
		ribd->addObjRIB(wrong_handle, name1, &obj1);
		CPPUNIT_ASSERT_MESSAGE("Add obj to with an invalid RIB handle succeeded", 0);
	}catch(eRIBNotFound& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during Add obj with an invalid RIB handle", 0);
	}

	//Attempt to add them into a valid RIB handle but with invalid names
	try{
		ribd->addObjRIB(handle, invalid_name1, &obj1);
		CPPUNIT_ASSERT_MESSAGE("Add obj to with an invalid name to RIB handle succeeded", 0);
	}catch(eObjInvalidName& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during Add obj with an invalid name to RIB handle", 0);
	}
	try{
		ribd->addObjRIB(handle, invalid_name2, &obj1);
		CPPUNIT_ASSERT_MESSAGE("Add obj to with an invalid name to RIB handle succeeded", 0);
	}catch(eObjInvalidName& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during Add obj with an invalid name to RIB handle", 0);
	}
	try{
		ribd->addObjRIB(handle, invalid_name3, &obj1);
		CPPUNIT_ASSERT_MESSAGE("Add obj to with an invalid name to RIB handle succeeded", 0);
	}catch(eObjInvalidName& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during Add obj with an invalid name to RIB handle", 0);
	}

	//Add the right object
	try{
		tmp = obj1;
		inst_id1 = ribd->addObjRIB(handle, name1, &obj1);
		CPPUNIT_ASSERT_MESSAGE("Did not set to null obj1", obj1 == NULL);
		CPPUNIT_ASSERT_MESSAGE("Invalid instance id for obj1", inst_id1 == 1);
		obj1 = tmp;
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Could not add obj with in a valid RIB", 0);
	}

	//Check that inst_id matches
	CPPUNIT_ASSERT_MESSAGE("Insertion id and getObjInstId() mismatch", inst_id1 == ribd->getObjInstId(handle, name1));

	//Check that utils work
	try{
		std::string ts;
		CPPUNIT_ASSERT_MESSAGE("Insertion id and getObjInstId() mismatch", inst_id1 == ribd->getObjInstId(handle, name1));
		CPPUNIT_ASSERT_MESSAGE("Class validation in getObjInstId() fails", inst_id1 == ribd->getObjInstId(handle, name1, MyObj::class_));
		ts = ribd->getObjfqn(handle, inst_id1);
		CPPUNIT_ASSERT_MESSAGE("FQN mismatch with getObjfqn()", name1 == ts);
		ts = ribd->getObjfqn(handle, inst_id1, MyObj::class_);
		CPPUNIT_ASSERT_MESSAGE("Class validation in getObjfqn() fails", name1 == ts);
		ts = ribd->getObjClass(handle, inst_id1);
		CPPUNIT_ASSERT_MESSAGE("Class of getObjClass() mismatch", ts.compare(MyObj::class_) == 0);
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Exception thrown during utils API validation", 0);
	}

	tmp = obj2;
	try{
		ribd->addObjRIB(handle, name1, &obj2);
		CPPUNIT_ASSERT_MESSAGE("Add overlapping object to RIB succeeded", 0);
	}catch(eObjExists& e){
		CPPUNIT_ASSERT_MESSAGE("Add overlapping object failed, but modified the object pointer",  tmp == obj2);
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception thrown during Add obj with an overlapping object", 0);
	}

	//Add another valid object
	try{
		tmp = obj2;
		inst_id2 = ribd->addObjRIB(handle, name2, &obj2);
		CPPUNIT_ASSERT_MESSAGE("Did not set to null obj2", obj2 == NULL);
		CPPUNIT_ASSERT_MESSAGE("Invalid instance id for obj2", inst_id2 == 2);
		obj2 = tmp;
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Could not add obj2 with in a valid RIB", 0);
	}
}


void ribBasicOps::testDeassociation(){

	rib_handle_t wrong_handle = 9999;

	//Deassociate with an invalid RIB handle
	try{
		ribd->deassociateRIBfromAE(wrong_handle, ae);
		CPPUNIT_ASSERT_MESSAGE("Deassociate to with an invalid RIB handle succeeded", 0);
	}catch(eRIBNotFound& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during deassociate with an invalid RIB handle", 0);
	}

	//Deassociate with an valid RIB handle but not associated
	try{
		ribd->deassociateRIBfromAE(handle2, ae);
		CPPUNIT_ASSERT_MESSAGE("Deassociate to with a valid not associated RIB handle succeeded", 0);
	}catch(eRIBNotAssociated& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during deassociate with a valid not associated RIB handle", 0);
	}

	//Deassociate with an invalid ae
	try{
		std::string wrong_ae = "fff";
		ribd->deassociateRIBfromAE(wrong_handle, wrong_ae);
		CPPUNIT_ASSERT_MESSAGE("Deassociate to with an invalid RIB handle succeeded", 0);
	}catch(eRIBNotFound& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during deassociate with an invalid AE name", 0);
	}

	//Deassociate from an existing association relation
	try{
		ribd->deassociateRIBfromAE(handle, ae);
	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Exception throw during deassociate with a valid RIB handle and version", 0);
	}

	//Try it again
	try{
		ribd->deassociateRIBfromAE(handle, ae);
		CPPUNIT_ASSERT_MESSAGE("Deassociate to with an invalid RIB handle succeeded (retry). State not properly cleaned?", 0);
	}catch(eRIBNotAssociated& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Invalid exception throw during deassociate with an invalid RIB handle(retry). State not properly cleaned?", 0);
	}

}

void ribBasicOps::testDestruction(){

	cdap_rib::vers_info_t wrong_version;
	wrong_version.version_ = 0x99999;
	rib_handle_t wrong_handle = 9999;

	//Destroy an inexistent schema (not yet implemented)
	try{
		ribd->destroySchema(wrong_version);
		CPPUNIT_ASSERT_MESSAGE("Destroy invalid schema succeeded", 0);
	}catch(eNotImplemented& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Wrong exception thrown during destroy invalid schema", 0);
	}

	//Destroy an inexistent RIB (not yet implemented)
	try{
		ribd->destroyRIB(wrong_handle);
		CPPUNIT_ASSERT_MESSAGE("Destroy invalid RIB succeeded", 0);
	}catch(eNotImplemented& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Wrong exception thrown during destroy invalid RIB", 0);
	}

	//Destroy an existent schema (not yet implemented)
	try{
		ribd->destroySchema(version);
		CPPUNIT_ASSERT_MESSAGE("Destroy a valid schema succeeded", 0);
	}catch(eNotImplemented& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Wrong exception thrown during destroy of a valid schema", 0);
	}

	//Destroy an inexistent RIB (not yet implemented)
	try{
		ribd->destroyRIB(handle);
		CPPUNIT_ASSERT_MESSAGE("Destroy a valid RIB succeeded", 0);
	}catch(eNotImplemented& e){

	}catch(...){
		CPPUNIT_ASSERT_MESSAGE("Wrong exception thrown during destroy a valid RIB", 0);
	}
}

void ribBasicOps::testFini(){
	rina::rib::fini();
}

int main(int args, char** argv){

	(void)args;
	(void)argv;

	CppUnit::TextUi::TestRunner runner;
	CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
	runner.addTest( registry.makeTest() );
	bool wasSuccessful = runner.run( "", false );

	int rc = (wasSuccessful) ? EXIT_SUCCESS : EXIT_FAILURE;
	return rc;
}
