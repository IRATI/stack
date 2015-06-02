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

//RIB daemon proxy
static rina::rib::RIBDaemonProxy* ribd = NULL;
//Application Entity name
static const std::string ae ="ae_name";

class ribBasicOps : public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE( ribBasicOps );
	CPPUNIT_TEST( testInit );
	CPPUNIT_TEST( testCreationRIB );
/*
	CPPUNIT_TEST( testAddObj );
	CPPUNIT_TEST( testUserData );
	CPPUNIT_TEST( testRemoveObj );
*/
	CPPUNIT_TEST( testFini );
	CPPUNIT_TEST_SUITE_END();

private:


public:
	void setUp();
	void tearDown();

	void testInit();
	void testCreationRIB();
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

void ribBasicOps::testCreationRIB(){

	rina::rib::RIB* rib = NULL;

	//Double call to init should throw an exception
	try{
		ribd->unregisterRIB(rib, ae);
		CPPUNIT_ASSERT_MESSAGE("Unregister an invalid RIB succeeded", 0);
	}catch(...){}


	//Destroy an inexistent RIB
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
