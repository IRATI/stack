//
// CDAP connector worker classes
//
// Micheal Crotty <mcrotty@tssg.org>
// Copyright (C) 2016, PRISTINE, Waterford Insitute of Technology
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

#include <unistd.h>
#include <iostream>
// #include "librina/concurrency.h"
// #include <librina/application.h>
#include <encoders/CDAP.pb.h> // For CDAPMessage
#include <encoders/ApplicationProcessNamingInfoMessage.pb.h>

// #include "server.h"
#include "json_format.h"
#include "connector.h"
#include "catch.h"

using namespace std;
using namespace rina::messages;
using namespace google::protobuf;

TEST_CASE( "JSON read reply message with complex value", "[M_READ_R]") {
  
    const string address = "ws://localhost";
    const unsigned int max_size = 4096;
    DMSWorker*  w = new DMSWorker(nullptr,
           address,
           max_size,
           nullptr) ;    
  
    applicationProcessNamingInfo_t* neighbor_name = new applicationProcessNamingInfo_t();
    neighbor_name->set_applicationprocessname("micheal");
    neighbor_name->set_applicationprocessinstance("10");
    
    string dialect = "mine";
    string json_name; 
    JsonFormat::PrintToString((*neighbor_name), dialect, &json_name);
    SECTION( "Encoded correctly") {
      REQUIRE_FALSE(json_name.empty());
    }
    
    unsigned char* gpb_name = new unsigned char[256];
    int gpb_size = neighbor_name->ByteSize();
    neighbor_name->SerializeToArray(gpb_name, 256);
    
    
    CDAPMessage* reply = CDAPMessage::default_instance().New();
    reply->set_result(0);
    
    
    // Now start the test    
    SECTION("Ignores non json types") {
      objVal_t* value = objVal_t::default_instance().New();
      value->set_strval("more information");
      // Construct a CDAP read reply with the value
      reply->set_allocated_objvalue(value);
      w->process_value(*reply);
            
      REQUIRE(reply->has_objvalue());      
      objVal_t objvalue = reply->objvalue();
      REQUIRE(objvalue.has_strval());
      REQUIRE_FALSE(objvalue.has_jsonval());
      REQUIRE_FALSE(objvalue.has_typeval());
      REQUIRE_FALSE(objvalue.has_byteval());
    }
    
    // Now start the test    
    SECTION("Ignores json types without adequate information") {
      objVal_t* value = objVal_t::default_instance().New();
      value->set_jsonval(json_name);
      // Construct a CDAP read reply with the value
      reply->set_allocated_objvalue(value);
      w->process_value(*reply);
            
      REQUIRE(reply->has_objvalue());      
      objVal_t objvalue = reply->objvalue();
      REQUIRE_FALSE(objvalue.has_strval());
      REQUIRE_FALSE(objvalue.has_typeval());
      REQUIRE_FALSE(objvalue.has_byteval());
      REQUIRE(objvalue.has_jsonval());
    }
    
    /* Now start the test    
    SECTION("Ignores json types without adequate information") {
      objVal_t* value = objVal_t::default_instance().New();
      value->set_jsonval(json_name);
      value->set_typeval("rina.messages.applicationProcessNamingInfo_t");
      // Construct a CDAP read reply with the value
      reply.set_allocated_objvalue(value);
      w->process_value(*reply);
            
      REQUIRE(reply.has_objvalue());      
      objValue_t objvalue = reply.objvalue();
      REQUIRE(objvalue.has_stringval());
      REQUIRE_FALSE(objvalue.has_jsonval());
      REQUIRE_FALSE(objvalue.has_typeval());
      REQUIRE_FALSE(objvalue.has_byteval());
    }*/
      
    // Now start the test    
    SECTION("Converts applicationProcessName type when all information is present") {
        objVal_t* value = objVal_t::default_instance().New();
        value->set_jsonval(json_name);
        value->set_typeval("rina.messages.applicationProcessNamingInfo_t");
        // Construct a CDAP read reply with the value
        reply->set_allocated_objvalue(value);
        w->process_value(*reply);
              
        REQUIRE(reply->has_objvalue());      
        objVal_t objvalue = reply->objvalue();
        REQUIRE_FALSE(objvalue.has_jsonval());
        REQUIRE(objvalue.has_typeval());
        REQUIRE(objvalue.has_byteval());
    }
      
    SECTION("Decodes to a similar GPB object") {
       objVal_t* value = objVal_t::default_instance().New();
       value->set_jsonval(json_name);
       value->set_typeval("rina.messages.applicationProcessNamingInfo_t");
       // Construct a CDAP read reply with the value
       reply->set_allocated_objvalue(value);
       w->process_value(*reply);
             
       REQUIRE(reply->has_objvalue());      
       REQUIRE(reply->objvalue().has_byteval());
       string objvalue = reply->objvalue().byteval();
       
       REQUIRE(objvalue.length() == gpb_size);
       for (int i=0;i < gpb_size;++i) {
         REQUIRE(objvalue[i] == gpb_name[i]);
       }
    }
}


TEST_CASE( "JSON read reply message with ipcp config value", "[M_READ_R]") {
  /*
  char buffer[max_sdu_size_in_bytes];

  mad_manager::ipcp_config_t ipc_config;
  ipc_config.process_instance = "1";
  ipc_config.process_name = "test2.IRATI";
  ipc_config.process_type = "normal-ipc";
  ipc_config.dif_to_assign = "normal.DIF";
  ipc_config.difs_to_register.push_back("410");
  ipc_config.enr_conf.enr_dif = "normal.DIF";
  ipc_config.enr_conf.enr_un_dif = "400";
  ipc_config.enr_conf.neighbor_name = "test1.IRATI";
  ipc_config.enr_conf.neighbor_instance = "1";

  cdap_rib::obj_info_t obj;
  obj.name_ = IPCP_2;
  obj.class_ = "IPCProcess";
  obj.inst_ = 0;
  mad_manager::IPCPConfigEncoder().encode(ipc_config,
  obj.value_);
  mad_manager::ipcp_config_t object;
  mad_manager::IPCPConfigEncoder().decode(obj.value_, object);

  cdap_rib::flags_t flags;
  flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

  cdap_rib::filt_info_t filt;
  filt.filter_ = 0;
  filt.scope_ = 0;
  */
}
