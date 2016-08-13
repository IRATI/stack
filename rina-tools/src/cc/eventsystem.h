
#ifndef EVENTSYSTEM_HPP
#define EVENTSYSTEM_HPP

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
  // DIF language
  static constexpr const char * DIF = "DIF";
  // Add CDAP
  static constexpr const char * CDAP = "CDAP";
    
};

#endif // EVENTSYSTEM_HPP
