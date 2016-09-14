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
