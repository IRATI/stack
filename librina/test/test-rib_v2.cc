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
// End of mockups
//

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

void ribBasicOps::testDeassociation(){

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
