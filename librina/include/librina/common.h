/*
 * Common
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

/**
 * This library provides definitions common to all the other RINA
 * related libraries presented in this document. Common
 * functionalities shared among framework components (i.e. applications,
 * daemons and libraries) might be made available from this library
 * as well.
 */

#ifndef LIBRINA_COMMON_H
#define LIBRINA_COMMON_H

#ifdef __cplusplus

#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <cstring>
#include <fcntl.h>
#include <stdint.h>
#include <asm/ioctl.h>

#include "librina/concurrency.h"
#include "librina/exceptions.h"
#include "librina/patterns.h"
#include "irati/kucommon.h"

namespace rina {

static std::string NORMAL_IPC_PROCESS= "normal-ipc";
static std::string SHIM_WIFI_IPC_PROCESS_STA= "shim-wifi-sta";
static std::string SHIM_WIFI_IPC_PROCESS_AP= "shim-wifi-ap";
static std::string SHIM_ETH_VLAN_IPC_PROCESS = "shim-eth-vlan";
static std::string SHIM_TCP_UDP_IPC_PROCESS = "shim-tcp-udp";

/**
 * Returns the version number of librina
 */
std::string getVersion();

/*
 * Creates a directory in the file system
 */
int createdir(const std::string& dir);

/*
 * Removes a directory from the file system, and all the files and subfolders
 * recursively
 */
int removedir_all(const std::string& dir);

extern int string2int(const std::string& s, int& ret);

/**
 * Contains application naming information
 */
class ApplicationProcessNamingInformation {
public:
	ApplicationProcessNamingInformation();
	ApplicationProcessNamingInformation(const std::string& processName,
					    const std::string& processInstance);
	ApplicationProcessNamingInformation(struct name * name);
	bool operator==(const ApplicationProcessNamingInformation &other) const;
	bool operator!=(const ApplicationProcessNamingInformation &other) const;
	bool operator>(const ApplicationProcessNamingInformation &other) const;
	bool operator<=(const ApplicationProcessNamingInformation &other) const;
	bool operator<(const ApplicationProcessNamingInformation &other) const;
	bool operator>=(const ApplicationProcessNamingInformation &other) const;

	const std::string getProcessNamePlusInstance() const;
	const std::string getEncodedString() const;
	const std::string toString() const;

	struct name * to_c_name() const;

	/**
	 * The process_name identifies an application process within the
	 * application process namespace. This value is required, it
	 * cannot be NULL. This name has global scope (it is defined by
	 * the chain of IDD databases that are linked together), and is
	 * assigned by an authority that manages the namespace that
	 * particular application name belongs to.
	 */
	std::string processName;

	/**
	 * The process_instance identifies a particular instance of the
	 * process. This value is optional, it may be NULL.
	 */
	std::string processInstance;

	/**
	 * The entity_name identifies an application entity within the
	 * application process. This value is optional, it may be NULL.
	 */
	std::string entityName;

	/**
	 * The entity_name identifies a particular instance of an entity
	 * within the application process. This value is optional, it
	 * may be NULL
	 */
	std::string entityInstance;
};

ApplicationProcessNamingInformation
decode_apnameinfo(const std::string &encodedString);

/**
 * This class defines the characteristics of a flow
 */
class FlowSpecification {
public:
	/** Average bandwidth in bytes/s. A value of 0 means don't care. */
	unsigned int averageBandwidth;

	/** Average bandwidth in SDUs/s. A value of 0 means don't care */
	unsigned int averageSDUBandwidth;

	/** In milliseconds. A value of 0 means don't care*/
	unsigned int peakBandwidthDuration;

	/** In milliseconds. A value of 0 means don't care */
	unsigned int peakSDUBandwidthDuration;

	/** A value of 0 indicates 'do not care' */
	double undetectedBitErrorRate;

	/** Indicates if partial delivery of SDUs is allowed or not */
	bool partialDelivery;

	/** Indicates if SDUs have to be delivered in order */
	bool orderedDelivery;

	/**
	 * Indicates the maximum gap allowed among SDUs, a gap of N SDUs
	 * is considered the same as all SDUs delivered. A value of -1
	 * indicates 'Any'
	 */
	int maxAllowableGap;

	/**
	 * In milliseconds, indicates the maximum delay allowed in this
	 * flow. A value of 0 indicates 'do not care'
	 */
	unsigned int delay;

	/**
	 * In milliseconds, indicates the maximum jitter allowed in this
	 * flow. A value of 0 indicates 'do not care'
	 */
	unsigned int jitter;

	/**
	 * In 1/10000, indicates the maximum loss probability allowed in this
	 * flow. A value >= 10000 indicates 'do not care'
	 */
	unsigned short loss;

	/**
	 * The maximum SDU size for the flow. May influence the choice
	 * of the DIF where the flow will be created.
	 */
	unsigned int maxSDUsize;

	/**
	 * True if message boundaries have to be preserved, false
	 * otherwise
	 */
	bool msg_boundaries;

	FlowSpecification();
	FlowSpecification(struct flow_spec * fspec);
	bool operator==(const FlowSpecification &other) const;
	bool operator!=(const FlowSpecification &other) const;
	const std::string toString();
	struct flow_spec * to_c_flowspec() const;
};

/**
 * Contains the information of an allocated flow
 */
class FlowInformation {
public:
	enum FlowState {
		FLOW_ALLOCATION_REQUESTED,
		FLOW_ALLOCATED,
		FLOW_DEALLOCATION_REQUESTED,
		FLOW_DEALLOCATED
	};

	FlowInformation();

	/** The local application name */
	ApplicationProcessNamingInformation localAppName;

	/** The remote application name */
	ApplicationProcessNamingInformation remoteAppName;

	/** The flow characteristics */
	FlowSpecification flowSpecification;

	/** The portId of the flow */
	int portId;

	/** File descriptor to access this flow */
	int fd;

	/** 0 if the user of this flow is an app, the IPCP id otherwise */
	unsigned short user_ipcp_id;

	/** The name of the DIF where the flow has been allocated */
	ApplicationProcessNamingInformation difName;

	FlowState state;

	/** The PID of the local OS process using this flow */
	pid_t pid;

	bool operator==(const FlowInformation &other) const;
	bool operator!=(const FlowInformation &other) const;
	const std::string toString();
};

/**
 * This class contains the properties of a single DIF
 */
class DIFProperties {
public:
	/** The name of the DIF */
	ApplicationProcessNamingInformation DIFName;

	/**
	 * The maximum SDU size this DIF can handle (writes with bigger
	 * SDUs will return an error, and read will never return an SDUs
	 * bigger than this size
	 */
	unsigned int maxSDUSize;

	DIFProperties();
	DIFProperties(const ApplicationProcessNamingInformation& DIFName,
			int maxSDUSize);
};

/**
 * Enum type that identifies the different types of events
 */
enum IPCEventType {
        FLOW_ALLOCATION_REQUESTED_EVENT,
        ALLOCATE_FLOW_REQUEST_RESULT_EVENT,
        ALLOCATE_FLOW_RESPONSE_EVENT,
        FLOW_DEALLOCATION_REQUESTED_EVENT,
        DEALLOCATE_FLOW_RESPONSE_EVENT,
        APPLICATION_UNREGISTERED_EVENT,
        FLOW_DEALLOCATED_EVENT,
        APPLICATION_REGISTRATION_REQUEST_EVENT,
        REGISTER_APPLICATION_RESPONSE_EVENT,
        APPLICATION_UNREGISTRATION_REQUEST_EVENT,
        UNREGISTER_APPLICATION_RESPONSE_EVENT,
        APPLICATION_REGISTRATION_CANCELED_EVENT,
        ASSIGN_TO_DIF_REQUEST_EVENT,
        ASSIGN_TO_DIF_RESPONSE_EVENT,
        UPDATE_DIF_CONFIG_REQUEST_EVENT,
        UPDATE_DIF_CONFIG_RESPONSE_EVENT,
        ENROLL_TO_DIF_REQUEST_EVENT,
        ENROLL_TO_DIF_RESPONSE_EVENT,
        IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION,
        IPC_PROCESS_QUERY_RIB,
        GET_DIF_PROPERTIES,
        GET_DIF_PROPERTIES_RESPONSE_EVENT,
        IPCM_REGISTER_APP_RESPONSE_EVENT,
        IPCM_UNREGISTER_APP_RESPONSE_EVENT,
        IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
        QUERY_RIB_RESPONSE_EVENT,
        IPC_PROCESS_DAEMON_INITIALIZED_EVENT,
        TIMER_EXPIRED_EVENT,
        IPC_PROCESS_CREATE_CONNECTION_RESPONSE,
        IPC_PROCESS_UPDATE_CONNECTION_RESPONSE,
        IPC_PROCESS_CREATE_CONNECTION_RESULT,
        IPC_PROCESS_DESTROY_CONNECTION_RESULT,
        IPC_PROCESS_DUMP_FT_RESPONSE,
        IPC_PROCESS_SET_POLICY_SET_PARAM,
        IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE,
        IPC_PROCESS_SELECT_POLICY_SET,
        IPC_PROCESS_SELECT_POLICY_SET_RESPONSE,
        IPC_PROCESS_PLUGIN_LOAD,
        IPC_PROCESS_PLUGIN_LOAD_RESPONSE,
        IPC_PROCESS_UPDATE_CRYPTO_STATE_RESPONSE,
        IPC_PROCESS_FWD_CDAP_MSG,
        IPC_PROCESS_FWD_CDAP_RESPONSE_MSG,
        DISCONNECT_NEIGHBOR_REQUEST_EVENT,
        DISCONNECT_NEIGHBOR_RESPONSE_EVENT,
	IPCM_MEDIA_REPORT_EVENT,
	IPC_PROCESS_ALLOCATE_PORT_RESPONSE,
	IPC_PROCESS_DEALLOCATE_PORT_RESPONSE,
	IPC_PROCESS_WRITE_MGMT_SDU_RESPONSE,
	IPC_PROCESS_READ_MGMT_SDU_NOTIF,
	IPCM_CREATE_IPCP_RESPONSE,
	IPCM_DESTROY_IPCP_RESPONSE,
	IPCM_FINALIZATION_REQUEST_EVENT,
	IPCP_SCAN_MEDIA_REQUEST_EVENT,
        NO_EVENT
};

/**
 * Base class for IPC Events
 */
class IPCEvent {
public:
	IPCEvent();
	IPCEvent(IPCEventType eventType, unsigned int sn,
		 unsigned int ctrl_port, unsigned short ipcp_id);
	virtual ~IPCEvent();
	static const std::string eventTypeToString(IPCEventType eventType);

	/** The type of event */
	IPCEventType eventType;

	/**
	 * If the event is a request, this is the number to relate it
	 * witht the response
	 */
	unsigned int sequenceNumber;

	/* Ctrl port that produced the event (to reply to) */
	unsigned int ctrl_port;

	/* IPCP id of the entity that produced the event */
	unsigned short ipcp_id;
};

class BaseResponseEvent: public IPCEvent {
public:
        /** The result of the operation */
        int result;

        BaseResponseEvent(int result, IPCEventType eventType,
			  unsigned int sn, unsigned int ctrl_p,
			  unsigned short ipcp_id);
};

/**
 * Event informing about an incoming flow request
 */
class FlowRequestEvent: public IPCEvent {
public:
	/** The port-id that locally identifies the flow */
	int portId;

	/** The name of the DIF that is providing this flow */
	ApplicationProcessNamingInformation DIFName;

	/** The local application name*/
	ApplicationProcessNamingInformation localApplicationName;

	/** The remote application name*/
	ApplicationProcessNamingInformation remoteApplicationName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

	/** True if it is a local application, false if it is a remote one */
	bool localRequest;

	/** 0 if it is an application, or the ID of the IPC Process otherwise */
	int flowRequestorIpcProcessId;

	/** the ID of the IPC Process that will provide the flow*/
	unsigned short ipcProcessId;

	/** PID of the process that requested the flow */
	pid_t pid;

	/**
	 * True if the flow will be used by internal IPCP tasks (e.g. layer management),
	 * false otherwise (used by an external app)
	 */
	bool internal;

	FlowRequestEvent();
	FlowRequestEvent(const FlowSpecification& flowSpecification,
			bool localRequest,
			const ApplicationProcessNamingInformation& localApplicationName,
			const ApplicationProcessNamingInformation& remoteApplicationName,
			int flowRequestorIpcProcessId,
			unsigned int sequenceNumber, unsigned int ctrl_p,
			unsigned short ipcp_id, pid_t pid);
	FlowRequestEvent(int portId,
			const FlowSpecification& flowSpecification,
			bool localRequest,
			const ApplicationProcessNamingInformation& localApplicationName,
			const ApplicationProcessNamingInformation& remoteApplicationName,
			const ApplicationProcessNamingInformation& DIFName,
			unsigned int sequenceNumber, unsigned int ctrl_p,
			unsigned short ipcp_id, pid_t pid);
	bool isLocalRequest() const;
};

/**
 * Event informing the IPC Process about a flow deallocation request
 */
class FlowDeallocateRequestEvent: public IPCEvent {
public:
	/** The port-id that locally identifies the flow */
	int portId;

	/**
	 * True if the flow will be used by internal IPCP tasks (e.g. layer management),
	 * false otherwise (used by an external app)
	 */
	bool internal;

        FlowDeallocateRequestEvent() : portId(-1), internal(false) { }
	FlowDeallocateRequestEvent(int portId,
				   unsigned int sequenceNumber,
				   unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * Event informing that a flow has been deallocated by an IPC Process, without
 * the application having requested it
 */
class FlowDeallocatedEvent: public IPCEvent {
public:
	/** The port id of the deallocated flow */
	int portId;

	/** An error code indicating why the flow was deallocated */
	int code;

	FlowDeallocatedEvent(int portId, int code, unsigned int ctrl_port,
			     unsigned short ipcp_id);
};

/**
 * Identifies the types of application registrations
 * APPLICATION_REGISTRATION_SINGLE_DIF - registers the application in a single
 * DIF, specified by the application
 * APPLICATION_REGISTRATION_ANY_DIF - registers the application in any of the
 * DIFs available to the application, chosen by the IPC Manager
 */
enum ApplicationRegistrationType {
	APPLICATION_REGISTRATION_SINGLE_DIF,
	APPLICATION_REGISTRATION_ANY_DIF
};

/**
 * Contains information about the registration of an application
 */
class ApplicationRegistrationInformation {
public:
        /** The name of the application being registered */
        ApplicationProcessNamingInformation appName;

        /** The name of the DAF of the application being registered */
        ApplicationProcessNamingInformation dafName;

        /**
         * The id of the IPC process being registered (0 if it is
         * an application
         */
        unsigned short ipcProcessId;

        /** The control port to send flow allocation requests */
        unsigned int ctrl_port;

        /** PID of the process that registered */
        pid_t pid;

        /** The type of registration requested */
        ApplicationRegistrationType applicationRegistrationType;

        /** Optional DIF name where the application wants to register */
        ApplicationProcessNamingInformation difName;

        ApplicationRegistrationInformation();
        ApplicationRegistrationInformation(
        		ApplicationRegistrationType applicationRegistrationType);
        const std::string toString();
};

/**
 * Event informing that an application has requested the
 * registration to a DIF
 */
class ApplicationRegistrationRequestEvent: public IPCEvent {
public:
	/** The application registration information*/
	ApplicationRegistrationInformation applicationRegistrationInformation;

        ApplicationRegistrationRequestEvent() { }
	ApplicationRegistrationRequestEvent(
		const ApplicationRegistrationInformation&
		applicationRegistrationInformation, unsigned int sequenceNumber,
		unsigned int ctrl_p, unsigned short ipcp_id);
};

class BaseApplicationRegistrationEvent: public IPCEvent {
public:
        /** The application that wants to unregister */
        ApplicationProcessNamingInformation applicationName;

        /** The DIF to which the application wants to cancel the registration */
        ApplicationProcessNamingInformation DIFName;

        BaseApplicationRegistrationEvent() { }
        BaseApplicationRegistrationEvent(const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& DIFName,
                        IPCEventType eventType,
                        unsigned int sequenceNumber, unsigned int ctrl_p, unsigned short ipcp_id);
        BaseApplicationRegistrationEvent(const ApplicationProcessNamingInformation& appName,
                        IPCEventType eventType,
                        unsigned int sequenceNumber, unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * Event informing that an application has requested the
 * unregistration from a DIF
 */
class ApplicationUnregistrationRequestEvent:
                public BaseApplicationRegistrationEvent {
public:
        ApplicationUnregistrationRequestEvent() :
                                BaseApplicationRegistrationEvent() { }
	ApplicationUnregistrationRequestEvent(
			const ApplicationProcessNamingInformation& appName,
			const ApplicationProcessNamingInformation& DIFName,
			unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
};

class BaseApplicationRegistrationResponseEvent:
                public BaseApplicationRegistrationEvent {
public:
        /** The result of the operation */
        int result;

        BaseApplicationRegistrationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& DIFName,
                        int result,
                        IPCEventType eventType,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
        BaseApplicationRegistrationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        int result,
                        IPCEventType eventType,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * Event informing about the result of an application registration request
 */
class RegisterApplicationResponseEvent:
                public BaseApplicationRegistrationResponseEvent {
public:
        RegisterApplicationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& difName,
                        int result, unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * Event informing about the result of an application unregistration request
 */
class UnregisterApplicationResponseEvent:
                public BaseApplicationRegistrationResponseEvent {
public:
        UnregisterApplicationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        int result, unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * Event informing about the application decision regarding the
 * acceptance/denial of a flow request
 */
class AllocateFlowResponseEvent: public BaseResponseEvent {
public:
        /**
         * If the flow was denied, this field controls wether the application
         * wants the IPC Process to reply to the source or not
         */
        bool notifySource;

        /** 0 if it is an application, or the ID of the IPC Process otherwise */
        int flowAcceptorIpcProcessId;

        /** PID of the process that accepted the flow */
        pid_t pid;

        AllocateFlowResponseEvent(int result,
                        	  bool notifysource,
				  int flowAcceptorIpcProcessId,
				  unsigned int sequenceNumber,
				  unsigned int ctrl_p,
				  unsigned short ipcp_id, pid_t pid);
};

/**
 * Stores IPC Events that have happened, ready to be consumed and
 * processed by client classes.
 */
class IPCEventProducer {
public:
	/** Blocks until there is an event available */
	IPCEvent * eventWait();
};

/**
 * Make IPCManager singleton
 */
extern Singleton<IPCEventProducer> ipcEventProducer;

/**
 * Base class for all RINA exceptions
 */
class IPCException: public Exception {
public:
	IPCException(const std::string& description);
	static const std::string operation_not_implemented_error;
};

/**
 * Represents a parameter that has a name and value
 */
class Parameter {
public:
        std::string name;
        std::string value;

        Parameter();
        Parameter(const std::string & name, const std::string & value);
        bool operator==(const Parameter &other) const;
        bool operator!=(const Parameter &other) const;
};

typedef struct ser_obj {
	int size_;
	unsigned char * message_;

	ser_obj() : size_(0), message_(0) {};

	~ser_obj()
	{
		if (message_)
			delete[] message_;
		message_ = 0;
	}

	ser_obj& operator=(const ser_obj &other)
	{
		size_ = other.size_;
		message_ = new unsigned char[size_];
		memcpy(message_, other.message_, size_);
		return *this;
	}
} ser_obj_t;

struct UcharArray {
	UcharArray();
	UcharArray(int arrayLength);
	UcharArray(const UcharArray &a, const UcharArray &b);
	UcharArray(const UcharArray &a, const UcharArray &b, const UcharArray &c);
	UcharArray(const ser_obj_t * sobj);
	~UcharArray();
	UcharArray& operator=(const UcharArray &other);
	bool operator==(const UcharArray &other) const;
	bool operator!=(const UcharArray &other) const;
	std::string toString();
	void get_seralized_object(ser_obj_t& result);

	unsigned char * data;
	int length;
};

class ConsecutiveUnsignedIntegerGenerator {
public:
	ConsecutiveUnsignedIntegerGenerator();
	unsigned int next();
	unsigned int counter_;
	Lockable lock_;
};

/// Represents an IPC Process with whom we're enrolled
class Neighbor {

public:
        Neighbor();
        static void from_c_neighbor(Neighbor & nei, struct ipcp_neighbor * cnei);
        struct ipcp_neighbor * to_c_neighbor(void) const;

        bool operator==(const Neighbor &other) const;
        bool operator!=(const Neighbor &other) const;
#ifndef SWIG
        const ApplicationProcessNamingInformation get_name() const;
        void set_name(const ApplicationProcessNamingInformation& name);
        const ApplicationProcessNamingInformation
                get_supporting_dif_name() const;
        void set_supporting_dif_name(
                const ApplicationProcessNamingInformation& supporting_dif_name);
        const std::list<ApplicationProcessNamingInformation> get_supporting_difs();
        void set_supporting_difs(
                        const std::list<ApplicationProcessNamingInformation>& supporting_difs);
        void add_supporting_dif(const ApplicationProcessNamingInformation& supporting_dif);
        unsigned int get_address() const;
        void set_address(unsigned int address);
        unsigned int get_old_address() const;
        void set_old_address(unsigned int address);
        unsigned int get_average_rtt_in_ms() const;
        void set_average_rtt_in_ms(unsigned int average_rtt_in_ms);
        bool is_enrolled() const;
        void set_enrolled(bool enrolled);
        int get_last_heard_from_time_in_ms() const;
        void set_last_heard_from_time_in_ms(int last_heard_from_time_in_ms_);
        int get_underlying_port_id() const;
        void set_underlying_port_id(int underlying_port_id);
        unsigned int get_number_of_enrollment_attempts() const;
        void set_number_of_enrollment_attempts(
                        unsigned int number_of_enrollment_attempts);
#endif
        const std::string toString();

        /// The IPC Process name of the neighbor
        ApplicationProcessNamingInformation name_;

        /// The name of the supporting DIF used to exchange data
        ApplicationProcessNamingInformation supporting_dif_name_;

        /// The names of all the supporting DIFs of this neighbor
        std::list<ApplicationProcessNamingInformation> supporting_difs_;

        /// The current address
        unsigned int address_;

        /// The old address, after an address change if any
         unsigned int old_address_;

        /// Tells if it is enrolled or not
        bool enrolled_;

        /// The average RTT in ms
        unsigned int average_rtt_in_ms_;

        /// The underlying portId used to communicate with this neighbor
        int underlying_port_id_;
        int internal_port_id;

        /// The last time a KeepAlive message was received from
        /// that neighbor, in ms
        int last_heard_from_time_in_ms_;

        /// The number of times we have tried to re-enroll with the
        /// neighbor after the connectivity has been lost
        unsigned int number_of_enrollment_attempts_;
};

/**
 * Thrown when there are problems initializing librina
 */
class InitializationException: public IPCException {
public:
	InitializationException():
		IPCException("Problems initializing librina"){
	}
	InitializationException(const std::string& description):
		IPCException(description){
	}
};

/// Template interface for encoding and decoding of objects (object <--> buffer)
template<class T>
class Encoder{
public:
	virtual ~Encoder(){}
	/// Converts an object to a byte array, if this object is recognized by the encoder
	/// @param object
	/// @throws exception if the object is not recognized by the encoder
	/// @return
	virtual void encode(const T &obj, ser_obj_t& serobj) = 0;
	/// Converts a byte array to an object of the type specified by "className"
	/// @param byte[] serializedObject
	/// @param objectClass The type of object to be decoded
	/// @throws exception if the byte array is not an encoded in a way that the
	/// encoder can recognize, or the byte array value doesn't correspond to an
	/// object of the type "className"
	/// @return
	virtual void decode(const ser_obj_t &serobj,T &des_obj) = 0;
};

/**
 * Initialize librina providing the local Netlink port-id where this librina
 * instantiation will be bound
 * @param localPort NL port used by this instantiation of librina
 * @param logLevel librina log level
 * @param pathToLogFile the path to the log file
 */
void initialize(unsigned int localPort, const std::string& logLevel,
                const std::string& pathToLogFile);

/**
 * Initialize librina letting the OS choose the Netlink port-id where this
 * librina instantiation will be bound
 * @param logLevel librina log level
 * @param pathToLogFile the path to the log file
 */
void initialize(const std::string& logLevel,
                const std::string& pathToLogFile);

void librina_finalize();

}

#endif

#endif
