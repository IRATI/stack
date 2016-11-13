//
// CDAP connector
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


#include <string>
#include <iostream>

#include "eventsystem.h"  // For constants
#include "eventfilters.h" // For classes
#include "catch.h"        // test harness


TEST_CASE( "JSON processsing", "[filtering]") {
  
  // Create a CDAP command / response filter
  ES_EventFilterBuilder builder; 
  builder.ofType(CDAP_Types::ES_CDAP)
    .fromSource(CDAP_Sources::DMS_MANAGER)
    .inLanguage(EDSL_Language::CDAP);
  ES_EventFilter* cdap_filter = builder.build();  

  // Verify behaviour
	SECTION( "Ignores TriggerSim") {
    const char * aTS = "  {\"TAX_VERSION\":\"1.0.0\",\"TAX_LANGUAGE\":\"TriggerSim_Lang\",\"TAX_DIALECT\":\"TRIGGER_CMD_100\",\"TAX_TIME\":1471433330216,\"TAX_TYPE\":\"ES_COMMAND\",\"TAX_SOURCE\":\"DMS_SHELL\",\"TAX_ID\":\"rina3@1105@ES_Event#59\",\"TAX_ID_HASH\":1362826810,\"TAX_TIME_STRING\":\"2016-08-17 12:28:50.216\",\"command\":\"sendTrigger\",\"id\":\"tconnect\"}";
		REQUIRE_FALSE(cdap_filter->match(aTS));
	}
	
  SECTION( "Ignores Strategy") {
    const char * aSTRAT_O = "{\"TAX_VERSION\":\"1.0.0\",\"TAX_LANGUAGE\":\"Strat_Lang\",\"TAX_DIALECT\":\"STRAT_STATES_100\",\"TAX_TIME\":1471433330267,\"TAX_TYPE\":\"ES_STRAT_OBSERVE\",\"TAX_SOURCE\":\"DMS_STRATEGY_OBSERVE\",\"TAX_ID\":\"rina3@1106@ES_Event#58\",\"TAX_ID_HASH\":1069423802,\"TAX_TIME_STRING\":\"2016-08-17 12:28:50.267\",\"invokeID\":9006,\"reason\":\"simulated connect request\",\"srcAEInst\":\"\",\"destAEInst\":\"\",\"version\":0,\"destApInst\":\"1\",\"scope\":0,\"destAEName\":\"\",\"srcApName\":\"rina.apps.mad.2\",\"name\":\"tconnect\",\"destApName\":\"rina.apps.manager\",\"absSyntax\":2,\"opCode\":\"m_connect\",\"srcAEName\":\"\",\"srcApInst\":\"1\",\"info\":\"simulated trigger for connect (rina.apps.mad.N)\"}";
    REQUIRE_FALSE(cdap_filter->match(aSTRAT_O));
    
    const char * aSTRAT_D = "{\"TAX_VERSION\":\"1.0.0\",\"TAX_LANGUAGE\":\"Strat_Lang\",\"TAX_DIALECT\":\"STRAT_STATES_100\",\"TAX_TIME\":1471433330282,\"TAX_TYPE\":\"ES_STRAT_DECIDE\",\"TAX_SOURCE\":\"DMS_STRATEGY_DECIDE\",\"TAX_ID\":\"rina3@1106@ES_Event#62\",\"TAX_ID_HASH\":1069423827,\"TAX_TIME_STRING\":\"2016-08-17 12:28:50.282\",\"result\":1,\"resultReason\":\"Updating the RIB\",\"srcApName\":\"rina.apps.mad.2\",\"srcAEInst\":\"\",\"srcAEName\":\"\",\"srcApInst\":\"1\"}";
    REQUIRE_FALSE(cdap_filter->match(aSTRAT_D));    
	}

  SECTION( "Matches CDAP") {
    const char* aCDAP_REQ = "{\"TAX_VERSION\":\"1.0.0\",\"TAX_LANGUAGE\":\"CDAP\",\"TAX_DIALECT\":\"CDAP_REQ\",\"TAX_TIME\":1471433330227,\"TAX_TYPE\":\"ES_CDAP\",\"TAX_SOURCE\":\"DMS_AGENT\",\"TAX_ID\":\"rina3@1104@ES_Event#17\",\"TAX_ID_HASH\":1656229691,\"TAX_TIME_STRING\":\"2016-08-17 12:28:50.227\",\"invokeID\":9006,\"reason\":\"simulated connect request\",\"destAEInst\":\"\",\"srcAEInst\":\"\",\"version\":0,\"destApInst\":\"1\",\"scope\":0,\"destAEName\":\"\",\"name\":\"tconnect\",\"destApName\":\"rina.apps.manager\",\"srcApName\":\"rina.apps.mad.2\",\"absSyntax\":2,\"opCode\":\"M_CONNECT\",\"srcAEName\":\"\",\"srcApInst\":\"1\",\"info\":\"simulated trigger for connect (rina.apps.mad.N)\"}";

    REQUIRE_FALSE(cdap_filter->match(aCDAP_REQ));
    
    const char* aCDAP_REQ2 = "{\"TAX_VERSION\":\"1.0.0\",\"TAX_LANGUAGE\":\"CDAP\",\"TAX_DIALECT\":\"CDAP_REQ\",\"TAX_TIME\":1471433330227,\"TAX_TYPE\":\"ES_CDAP\",\"TAX_SOURCE\":\"DMS_MANAGER\",\"TAX_ID\":\"rina3@1104@ES_Event#17\",\"TAX_ID_HASH\":1656229691,\"TAX_TIME_STRING\":\"2016-08-17 12:28:50.227\",\"invokeID\":9006,\"reason\":\"simulated connect request\",\"destAEInst\":\"\",\"srcAEInst\":\"\",\"version\":0,\"destApInst\":\"1\",\"scope\":0,\"destAEName\":\"\",\"name\":\"tconnect\",\"destApName\":\"rina.apps.manager\",\"srcApName\":\"rina.apps.mad.2\",\"absSyntax\":2,\"opCode\":\"M_CONNECT\",\"srcAEName\":\"\",\"srcApInst\":\"1\",\"info\":\"simulated trigger for connect (rina.apps.mad.N)\"}";

    REQUIRE(cdap_filter->match(aCDAP_REQ2));
	}

}
