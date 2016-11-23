// CDAP Connector - Unit tests
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

// #include <rinad/common/configuration.h>
// #include <rinad/common/encoder.h>

#include <iostream>
#include <stack>

// These are supplied from librina
#include <encoders/CDAP.pb.h>
#include <encoders/auth-policies.pb.h>
#include <encoders/irm.pb.h>
#include <encoders/MA-IPCP.pb.h>

#include "json_format.h"

// These are supplied from rinad library

// #include <encoders/DataTransferConstantsMessage.pb.h>
// #include <encoders/DirectoryForwardingTableEntryArrayMessage.pb.h>
// #include <encoders/QoSCubeArrayMessage.pb.h>
// #include "encoders/WhatevercastNameArrayMessage.pb.h"
// #include "encoders/NeighborArrayMessage.pb.h"
// #include "encoders/QoSSpecification.pb.h"
// #include "encoders/FlowMessage.pb.h"
// #include "encoders/EnrollmentInformationMessage.pb.h"
// #include "encoders/RoutingForwarding.pb.h"
// #include "encoders/MA-IPCP.pb.h"
// #include "encoders/RIBObjectData.pb.h"

// Test harness
#include "catch.h"

#include "eventsystem.h"

using namespace rina::messages;
using namespace std;
using namespace google::protobuf;


/*
* Quick a dirty testing helpers
*/

// Function to check whether two characters are opening 
// and closing of same type. 
bool ArePair(char opening,char closing)
{
	if(opening == '"' && closing == '"') return true;
	else if(opening == '{' && closing == '}') return true;
	else if(opening == '[' && closing == ']') return true;
	return false;
}

// Some basic checks on the JSON
bool JsonBalanced(const string& exp)
{
	stack<char>  S;
	for(unsigned int i =0;i<exp.length();i++)
	{
		if(/*exp[i] == '"' ||*/ exp[i] == '{' || exp[i] == '[')
			S.push(exp[i]);
		else if(/*exp[i] == '"' ||*/ exp[i] == '}' || exp[i] == ']')
		{
			if(S.empty() || !ArePair(S.top(),exp[i]))
				return false;
			else
				S.pop();
		}
	}
	return S.empty() ? true:false;
}

// allow comparisons between protobuf messages
bool operator==(const google::protobuf::MessageLite& msg_a,
                const google::protobuf::MessageLite& msg_b) {
  return (msg_a.GetTypeName() == msg_b.GetTypeName()) &&
      (msg_a.SerializeAsString() == msg_b.SerializeAsString());
}

// Get the dialect for the message
string getDialect(const CDAPMessage& message ) {
	opCode_t code = message.opcode();
	if ((code & 0x0001) == 1) {
		return string(CDAP_Dialects::CDAP_RES);
	} else {
		return string(CDAP_Dialects::CDAP_REQ);		
	}
}

// Create a default message
CDAPMessage& construct_message(opCode_t opCode, int messageId) {
        CDAPMessage* builder = CDAPMessage::default_instance().New();
        builder->set_abssyntax(0);
        builder->set_invokeid(messageId);
        // Set opcode
        builder->set_opcode(opCode);
        builder->set_flags(F_NO_FLAGS);

        // Source bits
        builder->set_srcapname("rina.app.a");
        builder->set_srcapinst("2");
        builder->set_srcaename("");
        builder->set_srcaeinst("");
        // Destination bits
        builder->set_destapname("rina.app.b");
        builder->set_destapinst("2");
        builder->set_destaename("");
        builder->set_destaeinst("");

        return *builder;
}

TEST_CASE( "JSON verifier", "[JSON]") {
  // Construct a CDAP connect
  
	
	SECTION( "Basic JSON") {
		string encMessage = "{\"greeting\":\"hello\",\"bye\":\"goodbye\"}";
		REQUIRE(JsonBalanced(encMessage));
	}
	
	SECTION( "JSON with embedded obj") {
		string encMessage = "{\"greeting\":\"hello\",\"bye\":\"goodbye\", \"action\":{\"type\":\"wave\"}}";
		REQUIRE(JsonBalanced(encMessage));
	}

	SECTION( "JSON with array of embedded obj") {
		string encMessage = "{\"greeting\":\"hello\",\"bye\":\"goodbye\", \"action\":[{\"type\":\"wave\"},{\"type\":\"walk\"}]}";
		REQUIRE(JsonBalanced(encMessage));
	}
	//
	// Other tests
	//
	SECTION( "Empty JSON stringvalue") {
		string encMessage = "";
		REQUIRE(JsonBalanced(encMessage));
	}

	SECTION( "JSON with missing {") {
		string encMessage = "{\"greeting\":\"hello\",\"bye\":\"goodbye\", \"action\":\"type\":\"wave\"}}";
		REQUIRE_FALSE(JsonBalanced(encMessage));
	}
	
	SECTION( "JSON with missing }") {
		string encMessage = "{\"greeting\":\"hello\",\"bye\":\"goodbye\", \"action\":{\"type\":\"wave\"}";
		REQUIRE_FALSE(JsonBalanced(encMessage));
	}
	
	
	SECTION( "JSON with missing array ]") {
		string encMessage = "{\"greeting\":\"hello\",\"bye\":\"goodbye\", \"action\":[{\"type\":\"wave\"},{\"type\":\"walk\"}}";
		REQUIRE_FALSE(JsonBalanced(encMessage));
	}
	
	SECTION( "JSON with missing array [") {
		string encMessage = "{\"greeting\":\"hello\",\"bye\":\"goodbye\", \"action\":{\"type\":\"wave\"},{\"type\":\"walk\"}]}";
		REQUIRE_FALSE(JsonBalanced(encMessage));
	}
	
}



TEST_CASE( "CDAP Connect message", "[M_CONNECT]") {
  // Construct a CDAP connect
  CDAPMessage request = construct_message(M_CONNECT, 8000);
  request.set_version(0);
  
  // Encode
	string dialect = getDialect(request);
	string encMessage;
	JsonFormat::PrintToString((Message&)request, dialect, &encMessage);
	
	SECTION( "Encoded correctly") {
		REQUIRE(request.IsInitialized());
		REQUIRE(!encMessage.empty());
		REQUIRE(JsonBalanced(encMessage));
	}

	SECTION( "Has non zero length") {
		REQUIRE(encMessage.length() > 0);
	}
	
	SECTION("Decodes to a similar object") {
		// Decode it to check
		CDAPMessage decoded;
		JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
		REQUIRE(request == decoded);
	}
}


TEST_CASE( "CDAP Connect reply message", "[M_CONNECT_R]") {
    // Construct a CDAP connection reply
    CDAPMessage reply = construct_message(M_CONNECT_R, 8000);
    reply.set_result(1);
    reply.set_resultreason("OK");
    reply.set_version(0);

    // Encode
		string dialect = getDialect(reply);
		string encMessage;
		JsonFormat::PrintToString((Message&)reply, dialect, &encMessage);

		SECTION( "Encoded correctly") {
			REQUIRE(reply.IsInitialized());
			REQUIRE(!encMessage.empty());
			REQUIRE(JsonBalanced(encMessage));
		}

		SECTION( "Has non zero length") {
			REQUIRE(encMessage.length() > 0);
		}
		
		SECTION("Decodes to a similar object") {
			// Decode it to check
			CDAPMessage decoded;
			JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
			REQUIRE(reply == decoded);
		}
}



/*
 * JSON tests
 */
TEST_CASE( "JSON read message", "[M_READ]") {
    //cout << "Here" <<endl;
    CDAPMessage builder = construct_message(M_READ, 8000);
    //cout << "Post" << endl;
    //  byte[] filter = new byte[1];
    //  filter[0]=0x00;
    // builder.setFlags(flagValues_t.F_NO_FLAGS)
    builder.set_objname("root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=2, RIBDaemon");
    builder.set_objinst(0);
    builder.set_objclass("RIBDaemon");
    builder.set_scope(0);
    //builder.set_filter();
		string dialect = CDAP_Dialects::CDAP_REQ;
    string encMessage;
    JsonFormat::PrintToString((Message&)builder, dialect, &encMessage);
    
    SECTION( "Encoded correctly") {
      REQUIRE(builder.IsInitialized());
      REQUIRE(!encMessage.empty());
      REQUIRE(JsonBalanced(encMessage));
			cout << "JSON:" << encMessage << endl;
    }
       
    SECTION( "Has non zero length") {
      REQUIRE(encMessage.length() > 0);
    }
    
    SECTION("Decodes to a similar object") {
      //cout << "JSON:" << encMessage << endl;
      // Decode it to check
      CDAPMessage decoded;
      JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
      REQUIRE(builder == decoded);
    }
}

TEST_CASE( "JSON read reply message", "[M_READ_R]") {
     // Construct a CDAP connection reply
     objVal_t* value = objVal_t::default_instance().New();
     value->set_strval("stringvalue");

     CDAPMessage reply = construct_message(M_READ_R, 8000);
     reply.set_result(1);
     reply.set_resultreason("OK");
     reply.set_version(0);
     reply.set_objname("root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=2, RIBDaemon");
     reply.set_objinst(0);
     reply.set_objclass("RIBDaemon");
     reply.set_allocated_objvalue(value);
     
     // Encode
		 string dialect = getDialect(reply);		 
     string encMessage;
     JsonFormat::PrintToString((Message&)reply, dialect, &encMessage);
     SECTION( "Encoded correctly") {
       REQUIRE(reply.IsInitialized());
       REQUIRE(!encMessage.empty());
       REQUIRE(JsonBalanced(encMessage));
     }
     
     SECTION("Has non zero length") {
       REQUIRE(encMessage.length() > 0);
     }
     
     SECTION("Decodes to a similar object") {
       // Decode it to check
       CDAPMessage decoded;
       JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));

       REQUIRE(reply == decoded);
     }
}

TEST_CASE( "JSON write message", "[M_WRITE]") {
     CDAPMessage builder = construct_message(M_WRITE, 8000);
    //  byte[] filter = new byte[1];
    //  filter[0]=0x00;

     objVal_t* value = objVal_t::default_instance().New();
     value->set_strval("stringvalue");

     builder.set_version(0);
     builder.set_objname("root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=2, RIBDaemon");
     builder.set_objinst(0);
     builder.set_objclass("RIBDaemon");
     builder.set_allocated_objvalue(value);
     
     // Encode
		 string dialect = CDAP_Dialects::CDAP_REQ;
     string encMessage;
     JsonFormat::PrintToString((Message&)builder, dialect, &encMessage);
     
     SECTION( "Encoded correctly") {
       REQUIRE(builder.IsInitialized());
       REQUIRE(!encMessage.empty());
       REQUIRE(JsonBalanced(encMessage));
     }
     
     SECTION( "Non zero length") {
       REQUIRE(encMessage.length() > 0);
     }
     
     SECTION("Decodes to a similar object") {
       // Decode it to checks
       CDAPMessage decoded;
       JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
       REQUIRE(builder == decoded);
     }
}

TEST_CASE( "JSON write reply message", "[M_WRITE_R]") {
    // Construct a CDAP connection reply
    CDAPMessage reply = construct_message(M_WRITE_R, 8000);
    reply.set_result(1);
    reply.set_resultreason("OK");

    // Encode
		string dialect = getDialect(reply);		
    string encMessage;
    JsonFormat::PrintToString((Message&)reply, dialect, &encMessage);
		
    SECTION( "Encoded correctly") {
      REQUIRE(reply.IsInitialized());
      REQUIRE(!encMessage.empty());
      REQUIRE(JsonBalanced(encMessage));
    }
    SECTION( "Non zero length") {
     REQUIRE(encMessage.length() > 0);
    }
    SECTION("Decodes to a similar object") {
     // Decode it to check
     CDAPMessage decoded;
     JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
     REQUIRE(reply == decoded);
    }
}


TEST_CASE( "JSON start message", "[M_START]") {
    CDAPMessage builder = construct_message(M_START, 8000);
    //  byte[] filter = new byte[1];
    //  filter[0]=0x00;
    builder.set_objname("root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=2, RIBDaemon");
    builder.set_objinst(0);
    builder.set_objclass("RIBDaemon");
    builder.set_scope(0);
    //builder.set_filter(Bytestring.copyFrom(filter));

		string dialect = getDialect(builder);
    string encMessage;
		JsonFormat::PrintToString((Message&)builder, dialect, &encMessage);
		
		SECTION( "Encoded correctly") {
      REQUIRE(builder.IsInitialized());
      REQUIRE(!encMessage.empty());
      REQUIRE(JsonBalanced(encMessage));
    }
    SECTION( "Non zero length") {
     REQUIRE(encMessage.length() > 0);
    }
    SECTION("Decodes to a similar object") {
     // Decode it to check
     CDAPMessage decoded;
     JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
     REQUIRE(builder == decoded);
    }
}

TEST_CASE( "JSON start reply message", "[M_START_R]") {
    // Construct a CDAP connection reply
    CDAPMessage reply = construct_message(M_START_R, 8000);
    reply.set_result(1);
    reply.set_resultreason("OK");

    // Encode
		string dialect = getDialect(reply);		
    string encMessage;
		JsonFormat::PrintToString((Message&)reply, dialect, &encMessage);

		SECTION( "Encoded correctly") {
			REQUIRE(reply.IsInitialized());
			REQUIRE(!encMessage.empty());
			REQUIRE(JsonBalanced(encMessage));
		}
		
		SECTION( "Non zero length") {
		 REQUIRE(encMessage.length() > 0);
		}
		
		SECTION("Decodes to a similar object") {
		 // Decode it to check
		 CDAPMessage decoded;
		 JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
		 REQUIRE(reply == decoded);
		}
}


TEST_CASE( "JSON stop message", "[J_M_STOP]") {
    CDAPMessage builder = construct_message(M_STOP, 8000);
    // byte[] filter = new byte[1];
    // filter[0]=0x00;
    builder.set_objname("root/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/ipcProcessID=2/ RIBDaemon");
    builder.set_objinst(0);
    builder.set_objclass("RIBDaemon");
    builder.set_scope(0);
    //builder.set_filter();
    
    // Encode it
		string dialect = getDialect(builder);
    string encMessage;
		JsonFormat::PrintToString((Message&)builder, dialect, &encMessage);
    
		SECTION( "Encoded correctly") {
      REQUIRE(builder.IsInitialized());
      REQUIRE(!encMessage.empty());
      REQUIRE(JsonBalanced(encMessage));
    }
		
    SECTION( "Non zero length") {
     REQUIRE(encMessage.length() > 0);
    }
		
    SECTION("Decodes to a similar object") {
     // Decode it to check
     CDAPMessage decoded;
     JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
     REQUIRE(builder == decoded);
    }
}

TEST_CASE( "JSON stop reply message", "[J_M_STOP_R]") {
    // Construct a CDAP connection reply
    CDAPMessage reply = construct_message(M_STOP_R, 8003);
    reply.set_result(1);
    reply.set_resultreason("OK");

    // Encode
		string dialect = getDialect(reply);
    string encMessage;
		JsonFormat::PrintToString((Message&)reply, dialect, &encMessage);
		
		SECTION( "Encoded correctly") {
			REQUIRE(reply.IsInitialized());
			REQUIRE(!encMessage.empty());
			REQUIRE(JsonBalanced(encMessage));
		}

		SECTION( "Non zero length") {
		 REQUIRE(encMessage.length() > 0);
		}

		SECTION("Decodes to a similar object") {
			cout << "Json:" << encMessage << endl;
		 // Decode it to check
		 CDAPMessage decoded;
		 JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
		 REQUIRE(reply == decoded);
		}
}


TEST_CASE( "JSON malformed neighbor message", "[J_M_CREATE]") {
  
  // Encode it
  string encMessage;
  encMessage = "{\"TAX_VERSION\":\"1.0.0\",\"TAX_LANGUAGE\":\"CDAP\",\"TAX_DIALECT\":\"CDAP_REQ\",\"TAX_TIME\":1474570428082,\"TAX_TYPE\":\"ES_CDAP\",\"TAX_SOURCE\":\"DMS_MANAGER\",\"TAX_ID\":\"rina3@1289@ES_Event#66\",\"TAX_ID_HASH\":1947264915,\"TAX_TIME_STRING\":\"2016-09-22 19:53:48.082\",\"flag\":\"F_NO_FLAGS\",\"objClass\":\"Neighbor\",\"destAEInst\":\"\",\"objValue\":{\"typeval\":\"rina.messages.applicationProcessNamingInfo_t\",\"jsonval\":\"{\"applicationProcessInstance\":\"1\",\"supportingDifs\":\"[110]\",\"applicationProcessName\":\"n1\"}\"},\"objName\":\"/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/ipcProcessID=3/difManagement/enrollment/neighbors/neighbor=0\",\"version\":1,\"filter\":\"\",\"destApInst\":\"1\",\"scope\":0,\"destAEName\":\"\",\"destApName\":\"rina.apps.mad.1\",\"opCode\":\"M_CREATE\",\"objInst\":0}";
  
  
  SECTION( "Encoded correctly") {
    //REQUIRE(builder.IsInitialized());
    REQUIRE(!encMessage.empty());
    REQUIRE(JsonBalanced(encMessage));
  }
  
  SECTION( "Non zero length") {
   REQUIRE(encMessage.length() > 0);
  }
  
  SECTION("Decodes to a similar object") {
   // Decode it to check
   CDAPMessage decoded;
   JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
   REQUIRE(decoded.IsInitialized() == false);
   //REQUIRE(builder == decoded);
  }
}

TEST_CASE( "JSON M_CONNECT message", "[J_M_CONNECT]") {
    CDAPMessage builder = construct_message(M_CONNECT_R, 8000);
	// builder.set_abssyntax(9);
	// builder.set_version(4);
	// Set opcode
	// Source bits
	// builder.set_srcapname("S_Apname";
	// builder.set_srcapinst("2");
	// if (message.has_destaename()) {
	// 	builder->set_srcaename(message.destaename());
	// 	builder->set_srcaeinst(message.destaeinst());
	// }
	// Destination bits
	// builder.set_destapname("D_Apname");
	// builder.set_destapinst("3");
	// if (message.has_srcaename()) {
	// 	builder->set_destaename(message.srcaename());
	// 	builder->set_destaeinst(message.srcaeinst());
	// }
	// Reply bits
	builder.set_result(0);
	builder.set_resultreason(string("OK"));
  
  authPolicy_t* auth = authPolicy_t::default_instance().New();
  auth->set_name("PSOC_auth");
  builder.set_allocated_authpolicy(auth);

    // Encode it
		string dialect = getDialect(builder);
    string encMessage;
		JsonFormat::PrintToString((Message&)builder, dialect, &encMessage);
    
		SECTION( "Encoded correctly") {
      REQUIRE(builder.IsInitialized());
      REQUIRE(!encMessage.empty());
      REQUIRE(JsonBalanced(encMessage));
      cout << "JSON:" << encMessage << endl;
    }
		
    SECTION( "Non zero length") {
     REQUIRE(encMessage.length() > 0);
    }
		
    SECTION("Decodes to a similar object") {
     // Decode it to check
     CDAPMessage decoded;
     JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
     REQUIRE(builder == decoded);
    }
}


TEST_CASE( "JSON M_CREATE IPCP_S", "[J_M_CREATE]") {
  
  // Encode it
  string encMessage;
  encMessage = 
    "{\"dif_to_assign\":{\"dif_name\":{\"applicationProcessName\":\"normal.DIF\"},\"dif_config\":{\"address\":17,\"efcp_config\":{\"data_transfer_constants\":{\"maxPDUSize\":10000,\"addressLength\":2,\"portIdLength\":2,\"cepIdLength\":2,\"qosidLength\":2,\"sequenceNumberLength\":4,\"lengthLength\":2,\"maxPDULifetime\":60000,\"rateLength\":4,\"frameLength\":4,\"ctrlSequenceNumberLength\":4},\"qos_cubes\":[{\"qosId\":1,\"name\":\"unreliablewithflowcontrol\",\"partialDelivery\":false,\"order\":true,\"dtpConfiguration\":{\"dtcpPresent\":true,\"initialATimer\":300,\"dtppolicyset\":{\"policyName\":\"default\",\"version\":\"0\"}},\"dtcpConfiguration\":{\"flowControl\":true,\"flowControlConfig\":{\"windowBased\":true,\"windowBasedConfig\":{\"maxclosedwindowqueuelength\":50,\"initialcredit\":50},\"rateBased\":false},\"rtxControl\":false,\"dtcppolicyset\":{\"policyName\":\"default\",\"version\":\"0\"}}},{\"qosId\":2,\"name\":\"reliablewithflowcontrol\",\"partialDelivery\":false,\"order\":true,\"maxAllowableGapSdu\":0,\"dtpConfiguration\":{\"dtcpPresent\":true,\"initialATimer\":300,\"dtppolicyset\":{\"policyName\":\"default\",\"version\":\"0\"}},\"dtcpConfiguration\":{\"flowControl\":true,\"flowControlConfig\":{\"windowBased\":true,\"windowBasedConfig\":{\"maxclosedwindowqueuelength\":50,\"initialcredit\":50},\"rateBased\":false},\"rtxControl\":false,\"rtxControlConfig\":{\"datarxmsnmax\":5,\"initialRtxTime\":1000},\"dtcppolicyset\":{\"policyName\":\"default\",\"version\":\"0\"}}}]},\"rmt_config\":{\"policy_set\":{\"policyName\":\"default\",\"version\":\"1\"},\"pft_conf\":{\"policyName\":\"default\",\"version\":\"0\"}},\"fa_config\":{\"policy_set\":{\"policyName\":\"default\",\"version\":\"1\"}},\"et_config\":{\"policyName\":\"default\",\"version\":\"1\",\"policyParameters\":[{\"name\":\"enrollTimeoutInMs\",\"value\":\"10000\"},{\"name\":\"watchdogPeriodInMs\",\"value\":\"30000\"},{\"name\":\"declaredDeadIntervalInMs\",\"value\":\"120000\"},{\"name\":\"neighborsEnrollerPeriodInMs\",\"value\":\"30000\"},{\"name\":\"maxEnrollmentRetries\",\"value\":\"3\"}]},\"nsm_config\":{\"addressing_config\":{\"address\":[{\"ap_name\":\"test1.IRATI\",\"ap_instance\":\"1\",\"address\":16},{\"ap_name\":\"test2.IRATI\",\"ap_instance\":\"1\",\"address\":17}],\"prefixes\":[{\"address_prefix\":0,\"organization\":\"N.Bourbaki\"},{\"address_prefix\":16,\"organization\":\"IRATI\"}]},\"policy_set\":{\"policyName\":\"default\",\"version\":\"1\"}},\"routing_config\":{\"policyName\":\"link-state\",\"version\":\"1\",\"policyParameters\":[{\"name\":\"objectMaximumAge\",\"value\":\"10000\"},{\"name\":\"waitUntilReadCDAP\",\"value\":\"5001\"},{\"name\":\"waitUntilError\",\"value\":\"5001\"},{\"name\":\"waitUntilPFUFTComputation\",\"value\":\"103\"},{\"name\":\"waitUntilFSODBPropagation\",\"value\":\"101\"},{\"name\":\"waitUntilAgeIncement\",\"value\":\"997\"}]},\"ra_config\":{\"policyName\":\"default\",\"version\":\"0\"},\"sm_config\":{\"policy_set\":{\"policyName\":\"default\",\"version\":\"1\"}}},\"dif_type\":\"normal-ipc\"},\"process_name\":{\"applicationProcessInstance\":\"1\",\"applicationProcessName\":\"micheal\"}}";

  
  SECTION( "Encoded correctly") {
    //REQUIRE(builder.IsInitialized());
    REQUIRE(!encMessage.empty());
    REQUIRE(JsonBalanced(encMessage));
  }
  
  SECTION( "Non zero length") {
   REQUIRE(encMessage.length() > 0);
  }
  
  SECTION("Decodes to a similar object") {
    // Decode it to check
    ipcp_config_t decoded;
    JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));
    REQUIRE(decoded.IsInitialized() == true);
    REQUIRE(decoded.descriptor()->full_name() == "rina.messages.ipcp_config_t");

    // Check the values
    SECTION("Has the correct process name") {
      // Process name
      applicationProcessNamingInfo_t* expected_name = applicationProcessNamingInfo_t::default_instance().New();
      expected_name->set_applicationprocessname("micheal");
      expected_name->set_applicationprocessinstance("1"); 
      REQUIRE(decoded.process_name() == *expected_name );  
    }

    SECTION("Has no difs to register at") {
      // Dif to register at
      REQUIRE(decoded.difs_to_register_size() == 0);
    }

    SECTION("Has the correct dif name") {
      // Dif to assign
      REQUIRE(decoded.has_dif_to_assign());
      dif_info_t dif_info = decoded.dif_to_assign();

      REQUIRE(dif_info.has_dif_name());
      applicationProcessNamingInfo_t* expected_name = applicationProcessNamingInfo_t::default_instance().New();
      expected_name->set_applicationprocessname("normal.DIF");
      expected_name->clear_applicationprocessinstance(); 
      REQUIRE(dif_info.dif_name() == *expected_name );  
    }

    SECTION("Has the correct dif type") {
      // Dif to assign
      REQUIRE(decoded.has_dif_to_assign());
      dif_info_t dif_info = decoded.dif_to_assign();

      // Dif type
      REQUIRE(dif_info.has_dif_type());
      REQUIRE(dif_info.dif_type() == "normal-ipc");  
    }

    SECTION("Has the correct dif config") {
      // Dif to assign
      dif_info_t dif_info = decoded.dif_to_assign();
      REQUIRE(dif_info.has_dif_config());
      
      // dif config
      dif_config_t dif_conf = dif_info.dif_config();
      REQUIRE(dif_conf.address() == 17 );  
      REQUIRE(dif_conf.has_efcp_config());
    
      // Embedded decoding seems to work
    }
    
  }
}


/*
TEST_CASE( "JSON create message", "[J_M_CREATE]") {

    applicationProcessNamingInfo_t proc_name = applicationProcessNamingInfo_t.newBuilder()
                    .setApplicationProcessInstance("1")
                    .setApplicationProcessName("normal-1.IPCP)")
                    .build();

    // Create IPCP config
    ipcp_config_t ipc_config = ipcp_config_t.newBuilder()
                    .setProcessName(proc_name)
                    //.setProcessType("normal-ipc")
                    //.setDifToAssign("normal.DIF")
                    //.addDifsToRegister("400")
                    .build();

    //objValue
    objVal_t.Builder value = objVal_t.newBuilder()
                    .setByteval(ipc_config.toBytestring());

    CDAPMessage.Builder builder = construct_message(opCode_t.M_CREATE, 8000);
    byte[] filter = new byte[1];
    filter[0]=0x00;

    CDAPMessage reply = builder.setFlags(flagValues_t.F_NO_FLAGS)
                    .setObjName("root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=2, RIBDaemon")
                    .setObjInst(0)
                    .setObjClass("RIBDaemon")
                    .setScope(0)
                    .setFilter(Bytestring.copyFrom(filter))
                    .setObjValue(value)
                    .build();

    // Encode it
    string encMessage = CDAPSerialiser.encodeJSONMessage(reply);
    SECTION( "Encoded correctly") {
      System.out.println("Message:" + encMessage);
      REQUIRE(NULL != encMessage);
    }
    SECTION( "Has non zero length") {
      REQUIRE(encMessage.length() > 0);
    }
    SECTION("Decodes to a similar object") {
      // Decode it to check
      CDAPMessage decoded;
JsonFormat::ParseFromString(encMessage, ((Message*)&decoded));

      REQUIRE("Decoded", reply, decoded.build());
    }
}
*/
