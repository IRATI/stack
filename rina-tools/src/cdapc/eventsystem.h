//
// CDAP Connector
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

#ifndef EVENTSYSTEM_HPP
#define EVENTSYSTEM_HPP

#include <vector>
#include <string>
#include <sys/types.h>  // getpid
#include <unistd.h>     // gethostname


/*
 * Class containing the ES header fields
 */
class HeaderKeys {
public:
	/** Key for the event identifier. */
	static constexpr const char* TAX_ID ="TAX_ID";
	/** Key for the event identifier hash. */
	static constexpr const char* TAX_ID_HASH = "TAX_ID_HASH";
	/** Key for the event source. */
	static constexpr const char* TAX_SOURCE = "TAX_SOURCE";
	/** Key for the event language. */
	static constexpr const char* TAX_LANGUAGE = "TAX_LANGUAGE";
	/** Key for the event language version. */
	static constexpr const char* TAX_VERSION ="TAX_VERSION";
	/** Key for the event language. */
	static constexpr const char* TAX_DIALECT = "TAX_DIALECT";
	/** Key for the event type. */
	static constexpr const char* TAX_TYPE = "TAX_TYPE";
	/** Key for the event creation time. */
	static constexpr const char* TAX_TIME = "TAX_TIME";
	/** Key for the event creation time as string. */
	static constexpr const char* TAX_TIME_STRING = "TAX_TIME_STRING";

	// Known header fields
	static std::vector<std::string>& getValues() {
		static std::vector<std::string> gValues;
		if (gValues.empty()) {
			gValues.push_back(TAX_ID);
			gValues.push_back(TAX_ID_HASH);
			gValues.push_back(TAX_SOURCE);
			gValues.push_back(TAX_LANGUAGE);
			gValues.push_back(TAX_VERSION);
			gValues.push_back(TAX_DIALECT);
			gValues.push_back(TAX_TYPE);
			gValues.push_back(TAX_TIME);
			gValues.push_back(TAX_TIME_STRING);
		}
		return gValues;
	}
};

/*
 * Event types
 */
class ESD_Types {
public:

	/** An event telling everyone to shutdown. */
	static constexpr const char* ES_SHUTDOWN = "ES_SHUTDOWN";
	/** An event with information for a server, for instance the Web Socket server. */
	static constexpr const char* ES_SERVER_INFO	= "ES_SERVER_INFO";
	/** An event with information for a console,for instance the APEX console in the DG implementation. */
	static constexpr const char* ES_CONSOLE_INFO = "ES_CONSOLE_INFO";
	/** A log event. */
	static constexpr const char* ES_LOG = "ES_LOG";
	/** A command of some sort, intended to trigger some activities. */
	static constexpr const char* ES_COMMAND = "ES_COMMAND";
	/** A generic event. */
	static constexpr const char* ES_EVENT ="ES_EVENT";
	/** An event with configuration management informations. */
	static constexpr const char* ES_EVENT_CM = "ES_EVENT_CM";		
	/** An event with performance management information. */
	static constexpr const char* ES_EVENT_PM = "ES_EVENT_PM";		
	/** An event with fault management information. */
	static constexpr const char* ES_EVENT_FM	= "ES_EVENT_FM";	
};

class ESD_Dialects {
public:
	// All base event system messages are of this type
	static constexpr const char * ES_EVENTS = "ES_EVENTS";
};

class ESD_Sources {
public:
	/** A generic source. */
	static constexpr const char * GENERIC = "GENERIC";
	/** Events send from the ES system. */
	static constexpr const char * SYSTEM = "SYSTEM";
};

class CDAP_Sources {
public:
	/** Events send by a DMS Manager application. */
	static constexpr const char * DMS_MANAGER = "DMS_MANAGER";
	/** Events send by a DMS Agent application. */
	static constexpr const char * DMS_AGENT = "DMS_AGENT";
};

class CDAP_Dialects {
public:
	/** Events send by a DMS Manager application. */
	static constexpr const char * CDAP_REQ = "CDAP_REQ";
	/** Events send by a DMS Agent application. */
	static constexpr const char * CDAP_RES = "CDAP_RES";
};

class CDAP_Types {
public:
	/** A CDAP message. */
	static constexpr const char * ES_CDAP = "ES_CDAP";
};


// We mangle this one to contain all the languages
// Its a short cut
class EDSL_Language {
public:
	// Base Event DSL
	static constexpr const char * ESEVENT = "EDSL_ESEvent";
	static constexpr const char * ESEVENT_VER = "1.0.0";
	// DIF language
	static constexpr const char * DIF = "DIF";
	static constexpr const char * DIF_VER = "1.0.0";
	// Add CDAP
	static constexpr const char * CDAP = "CDAP";
	static constexpr const char * CDAP_VER = "1.0.0";
};


/* generate system ids
class SystemId {
public:
  static void generateId(std::string& sys_id) {
    if (system.empty()) {
      id = 0;
      char hostname[256];
      gethostname(hostname, 256);
      system.append(hostname);
      system.append("@"); 
      system.append(std::to_string(getpid())); 
      system.append("ES_Event#");
    }
    sys_id = system;
    sys_id.append(std::to_string(id));
    id++;
  }

private:
  static  std::string system;
  static long long             id;
};
 */
#endif // EVENTSYSTEM_HPP
