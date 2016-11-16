//
// CDAP connector worker tests
//
// Micheal Crotty <mcrotty AT tssg DOT org>
// Copyright (c) 2016, PRISTINE project, Waterford Institute of Technology. 
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

#include "catch.h"

#include <encoders/ApplicationProcessNamingInfoMessage.pb.h>

#include "connector.h"


using namespace rina::messages;
using namespace std;

// Redefine a worker to make it easier to test
class TestingWorker : public MAWorker {
public:
	TestingWorker(rina::ThreadAttributes * threadAttributes,
	              rina::FlowInformation flow) : MAWorker(threadAttributes, flow, 2400, nullptr)
                { };
	~TestingWorker() throw() { };
	
  int internal_run() {
    rina::Sleep sleeper;
    while (true) {
      sleeper.sleepForSec(1);
    }
		return 0;
  };
};


TEST_CASE( "Test MA", "[MA]") {
    // Construct a CDAP connection reply
    SECTION("sendMessage to MA") {
      rina::FlowInformation flow1;
      flow1.localAppName = rina::ApplicationProcessNamingInformation("local","1");
      flow1.remoteAppName = rina::ApplicationProcessNamingInformation("remote","45");
      flow1.portId = 2;
      
      const unsigned int max_size = 4096;
      MAWorker  w(nullptr,
             flow1,
             max_size,
             nullptr);

      REQUIRE_NOTHROW(w.newFlowRequest(flow1));
      
      // MAWorker has a single connection
      
      // Enocode a buffer
      applicationProcessNamingInfo_t* neighbor_name = new applicationProcessNamingInfo_t();
      neighbor_name->set_applicationprocessname("micheal");
      neighbor_name->set_applicationprocessinstance("10");
      
      unsigned char* gpb_name = new unsigned char[256];
      int gpb_size = neighbor_name->ByteSize();
      neighbor_name->SerializeToArray(gpb_name, 256);

      // Call the function
      std::string dest = flow1.remoteAppName.getProcessNamePlusInstance();
      
      REQUIRE_NOTHROW(w.send_message(gpb_name, gpb_size, dest));
        
    }
    
    SECTION("sendMessage to MA threaded") {
      rina::FlowInformation flow1;
      flow1.localAppName = rina::ApplicationProcessNamingInformation("local","1");
      flow1.remoteAppName = rina::ApplicationProcessNamingInformation("remote","45");
      flow1.portId = 2;
      
      // Spawn on a new thread
      rina::ThreadAttributes threadAttributes;
      MAWorker * worker = new TestingWorker(&threadAttributes, flow1);
      worker->start();
      worker->detach();
    
      // Wait for th eworker to initialise
      rina::Sleep sleeper;
      sleeper.sleepForSec(1);

      REQUIRE_NOTHROW(worker->newFlowRequest(flow1));
      
      // MAWorker has a single connection
      
      // Enocode a buffer
      applicationProcessNamingInfo_t* neighbor_name = new applicationProcessNamingInfo_t();
      neighbor_name->set_applicationprocessname("micheal");
      neighbor_name->set_applicationprocessinstance("10");
      
      unsigned char* gpb_name = new unsigned char[256];
      int gpb_size = neighbor_name->ByteSize();
      neighbor_name->SerializeToArray(gpb_name, 256);

      // Call the function
      std::string dest = flow1.remoteAppName.getProcessNamePlusInstance();
      
      REQUIRE_NOTHROW(worker->send_message(gpb_name, gpb_size, dest));
        
    }

}

TEST_CASE( "Test Connection identifiers", "[Basic]") {
    // Construct a CDAP connection reply
    rina::FlowInformation flow;
    flow.localAppName = rina::ApplicationProcessNamingInformation("local","9");
    flow.remoteAppName = rina::ApplicationProcessNamingInformation("remote","44");
    flow.portId=100;
    
    SECTION("ConnectionInfo and MA identify the same flow") {
      ConnectionInfo info(flow);
      
      REQUIRE(info.get_id() == flow.remoteAppName.getProcessNamePlusInstance());
    }
}


TEST_CASE("Flow management", "[Flow]") {
  
  rina::FlowInformation flow;
  flow.localAppName = rina::ApplicationProcessNamingInformation("local","9");
  flow.remoteAppName = rina::ApplicationProcessNamingInformation("remote","44");
  flow.portId=100;
  
  MAWorker w(nullptr, flow, 10, nullptr);
  
  SECTION("newFlowRequest adds a Connection") {
    rina::FlowInformation flow1;
    flow1.localAppName = rina::ApplicationProcessNamingInformation("local","1");
    flow1.remoteAppName = rina::ApplicationProcessNamingInformation("remote","45");
    flow1.portId = 2;
    REQUIRE_NOTHROW(w.newFlowRequest(flow1));
    
    REQUIRE(w.count_flows() == 1);
    REQUIRE(w.check_flow(flow1.remoteAppName.getProcessNamePlusInstance()));

    rina::FlowInformation flow2;
    flow2.localAppName = rina::ApplicationProcessNamingInformation("local","2");
    flow2.remoteAppName = rina::ApplicationProcessNamingInformation("remote","46");
    flow2.portId = 3;

    REQUIRE_NOTHROW(w.newFlowRequest(flow2));
    REQUIRE(w.count_flows() == 2);
    REQUIRE(w.check_flow(flow2.remoteAppName.getProcessNamePlusInstance()));
    
  }
  
  SECTION("closeFlowRequest removes a Connection") {
    rina::FlowInformation flow1;
    flow1.localAppName = rina::ApplicationProcessNamingInformation("local","1");
    flow1.remoteAppName = rina::ApplicationProcessNamingInformation("remote","45");
    flow1.portId = 2;

    rina::FlowInformation flow2;
    flow2.localAppName = rina::ApplicationProcessNamingInformation("local","2");
    flow2.remoteAppName = rina::ApplicationProcessNamingInformation("remote","46");
    flow2.portId = 3;

    REQUIRE_NOTHROW(w.newFlowRequest(flow1));
    REQUIRE_NOTHROW(w.newFlowRequest(flow2));
    REQUIRE(w.count_flows() == 2);
    
    rina::FlowInformation existing;
    existing.localAppName = rina::ApplicationProcessNamingInformation("local","1");
    existing.remoteAppName = rina::ApplicationProcessNamingInformation("remote","45");
    existing.portId = 2;
    REQUIRE_NOTHROW(w.closeFlowRequest(existing));
    //REQUIRE_THROWS_AS(w.closeFlowRequest(existing), rina::UnknownFlowException);
    REQUIRE(w.count_flows() == 1);
    // Flow 1 is gone
    REQUIRE_FALSE(w.check_flow(flow1.remoteAppName.getProcessNamePlusInstance()));
    // Flow 2 is still okay
    REQUIRE(w.check_flow(flow2.remoteAppName.getProcessNamePlusInstance()));
  }

  SECTION("closeFlowRequest does not remove a non existing Connection") {
    rina::FlowInformation flow1;
    flow1.localAppName = rina::ApplicationProcessNamingInformation("local","1");
    flow1.remoteAppName = rina::ApplicationProcessNamingInformation("remote","45");
    flow1.portId = 2;

    rina::FlowInformation flow2;
    flow2.localAppName = rina::ApplicationProcessNamingInformation("local","2");
    flow2.remoteAppName = rina::ApplicationProcessNamingInformation("remote","46");
    flow2.portId = 3;

    w.newFlowRequest(flow1);
    w.newFlowRequest(flow2);
    REQUIRE(w.count_flows() == 2);
    
    // Attempt to remove a non-existing connection
    rina::FlowInformation non_existing;
    non_existing.localAppName = rina::ApplicationProcessNamingInformation("local","2");
    non_existing.remoteAppName = rina::ApplicationProcessNamingInformation("remote","0");
    non_existing.portId = 199;
    REQUIRE_NOTHROW(w.closeFlowRequest(non_existing));
    REQUIRE(w.count_flows() == 2);
    // Flow 1 is still okay
    REQUIRE(w.check_flow(flow1.remoteAppName.getProcessNamePlusInstance()));
    // Flow 2 is still okay
    REQUIRE(w.check_flow(flow2.remoteAppName.getProcessNamePlusInstance()));
  }
  
}
