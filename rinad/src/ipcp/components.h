/*
 * Common interfaces and constants of the IPC Process components
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef IPCP_COMPONENTS_HH
#define IPCP_COMPONENTS_HH

#ifdef __cplusplus

#include <list>

#include <librina/cdap.h>
#include <librina/ipc-process.h>

#include "common/encoder.h"
#include "events.h"

namespace rinad {

const unsigned int max_sdu_size_in_bytes = 10000;

enum IPCProcessOperationalState {
	NOT_INITIALIZED,
	INITIALIZED,
	ASSIGN_TO_DIF_IN_PROCESS,
	ASSIGNED_TO_DIF
};

class IPCProcess;

/// IPC process component interface
class IPCProcessComponent {
public:
	virtual ~IPCProcessComponent(){};
	virtual void set_ipc_process(IPCProcess * ipc_process) = 0;
	virtual void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) = 0;
};

/// Interface
/// Delimits and undelimits SDUs, to allow multiple SDUs to be concatenated in the same PDU
class IDelimiter
{
public:
  virtual ~IDelimiter() { };

  /// Takes a single rawSdu and produces a single delimited byte array, consisting in
  /// [length][sdu]
  /// @param rawSdus
  /// @return
  virtual char* getDelimitedSdu(char rawSdu[]) = 0;

  /// Takes a list of raw sdus and produces a single delimited byte array, consisting in
  /// the concatenation of the sdus followed by their encoded length: [length][sdu][length][sdu] ...
  /// @param rawSdus
  /// @return
  virtual char* getDelimitedSdus(const std::list<char*>& rawSdus) = 0;

  /// Assumes that the first length bytes of the "byteArray" are an encoded varint (encoding an integer of 32 bytes max), and returns the value
  /// of this varint. If the byteArray is still not a complete varint, doesn't start with a varint or it is encoding an
  /// integer of more than 4 bytes the function will return -1.
  ///  @param byteArray
  /// @return the value of the integer encoded as a varint, or -1 if there is not a valid encoded varint32, or -2 if
  ///this may be a complete varint32 but still more bytes are needed
  virtual int readVarint32(char byteArray[],
                           int  length) = 0;

  /// Takes a delimited byte array ([length][sdu][length][sdu] ..) and extracts the sdus
  /// @param delimitedSdus
  /// @return
  virtual std::list<char*>& getRawSdus(char delimitedSdus[]) = 0;
};

/// Contains the objects needed to request the Enrollment
class EnrollmentRequest
{
public:
	EnrollmentRequest(const rina::Neighbor &neighbor,
                          const rina::EnrollToDIFRequestEvent &event);
	rina::Neighbor neighbor_;
	rina::EnrollToDIFRequestEvent event_;
};

/// Interface that must be implementing by classes that provide
/// the behavior of an enrollment task
class IEnrollmentTask : public IPCProcessComponent {
public:
	virtual ~IEnrollmentTask(){};
	virtual const std::list<rina::Neighbor>& get_neighbors() const = 0;
	virtual const std::list<std::string>& get_enrolled_ipc_process_names() const = 0;

	/// A remote IPC process Connect request has been received.
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	virtual void connect(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;

	/// A remote IPC process Connect response has been received.
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	virtual void connectResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;

	/// A remote IPC process Release request has been received.
	/// @param cdapMessage
	/// @param cdapSessionDescripton
	virtual void release(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;

	/// A remote IPC process Release response has been received.
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	virtual void releaseResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;

	/// Process a request to initiate enrollment with a new Neighbor, triggered by the IPC Manager
	/// @param event
	/// @param flowInformation
	virtual void processEnrollmentRequestEvent(const rina::EnrollToDIFRequestEvent& event,
			const rina::DIFInformation& difInformation) = 0;

	/// Starts the enrollment program
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	virtual void initiateEnrollment(EnrollmentRequest request) = 0;

	/// Called by the enrollment state machine when the enrollment request has been completed,
	/// either successfully or unsuccessfully
	/// @param candidate the IPC process we were trying to enroll to
	/// @param enrollee true if this IPC process is the one that initiated the
	/// enrollment sequence (i.e. it is the application process that wants to
	/// join the DIF)
	virtual void enrollmentCompleted(const rina::Neighbor& candidate,
                                         bool enrollee) = 0;

	/// Called by the enrollment state machine when the enrollment sequence fails
	/// @param remotePeer
	/// @param portId
	/// @param enrollee
	/// @param sendMessage
	/// @param reason
	virtual void enrollmentFailed(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
                                      int portId,
                                      const std::string& reason,
                                      bool enrolle,
                                      bool sendReleaseMessage) = 0;

	/// Finds out if the ICP process is already enrolled to the IPC process identified by
	/// the provided apNamingInfo
	/// @param apNamingInfo
	/// @return
	virtual bool isEnrolledTo(const std::string& applicationProcessName) const = 0;
};

/// Interface that must be implementing by classes that provide
/// the behavior of a Flow Allocator task
class IFlowAllocator : public IPCProcessComponent {
public:
	virtual ~IFlowAllocator(){};

	/// The Flow Allocator is invoked when an Allocate_Request.submit is received.  The source Flow
	/// Allocator determines if the request is well formed.  If not well-formed, an Allocate_Response.deliver
	/// is invoked with the appropriate error code.  If the request is well-formed, a new instance of an
	/// FlowAllocator is created and passed the parameters of this Allocate_Request to handle the allocation.
	/// It is a matter of DIF policy (AllocateNoificationPolicy) whether an Allocate_Request.deliver is invoked
	/// with a status of pending, or whether a response is withheld until an Allocate_Response can be delivered
	/// with a status of success or failure.
	/// @param allocateRequest the characteristics of the flow to be allocated.
	/// to honour the request
	virtual void submitAllocateRequest(rina::FlowRequestEvent * flowRequestEvent) = 0;

	virtual void processCreateConnectionResponseEvent(const rina::CreateConnectionResponseEvent& event) = 0;

	/// Forward the allocate response to the Flow Allocator Instance.
	/// @param portId the portId associated to the allocate response
	/// @param AllocateFlowResponseEvent - the response from the application
	virtual void submitAllocateResponse(const rina::AllocateFlowResponseEvent& event) = 0;

	virtual void processCreateConnectionResultEvent(const rina::CreateConnectionResultEvent& event) = 0;

	virtual void processUpdateConnectionResponseEvent(const rina::UpdateConnectionResponseEvent& event) = 0;

	/// Forward the deallocate request to the Flow Allocator Instance.
	/// @param the flow deallocate request event
	/// @throws IPCException
	virtual void submitDeallocate(const rina::FlowDeallocateRequestEvent& event) = 0;

	/// When an Flow Allocator receives a Create_Request PDU for a Flow object,
	/// it consults its local Directory to see if it has an entry. If there is an
	/// entry and the address is this IPC Process, it creates an FAI and passes
	/// the Create_request to it.If there is an entry and the address is not this IPC
	/// Process, it forwards the Create_Request to the IPC Process designated by the address.
	/// @param cdapMessage
	/// @param underlyingPortId
	virtual void createFlowRequestMessageReceived(const rina::CDAPMessage * cdapMessage,
                                                      int                       underlyingPortId) = 0;

	/// Called by the flow allocator instance when it finishes to cleanup the state.
	/// @param portId
	virtual void removeFlowAllocatorInstance(int portId) = 0;
};

/// Namespace Manager Interface
class INamespaceManager : public IPCProcessComponent {
public:
	virtual ~INamespaceManager(){};

	/// Returns the address of the IPC process where the application process is, or
	/// null otherwise
	/// @param apNamingInfo
	/// @return
	virtual unsigned int getDFTNextHop(const rina::ApplicationProcessNamingInformation& apNamingInfo) = 0;

	/// Returns the IPC Process id (0 if not an IPC Process) of the registered
	/// application, or -1 if the app is not registered
	/// @param apNamingInfo
	/// @return
	virtual int getRegIPCProcessId(const rina::ApplicationProcessNamingInformation& apNamingInfo) = 0;

	/// Add an entry to the directory forwarding table
	/// @param entry
	virtual void addDFTEntry(rina::DirectoryForwardingTableEntry * entry) = 0;

	/// Get an entry from the application name
	/// @param apNamingInfo
	/// @return
	virtual rina::DirectoryForwardingTableEntry * getDFTEntry(
			const rina::ApplicationProcessNamingInformation& apNamingInfo) = 0;

	/// Remove an entry from the directory forwarding table
	/// @param apNamingInfo
	virtual void removeDFTEntry(const rina::ApplicationProcessNamingInformation& apNamingInfo) = 0;

	/// Process an application registration request
	/// @param event
	virtual void processApplicationRegistrationRequestEvent(
			const rina::ApplicationRegistrationRequestEvent& event) = 0;

	/// Process an application unregistration request
	/// @param event
	virtual void processApplicationUnregistrationRequestEvent(
			const rina::ApplicationUnregistrationRequestEvent& event) = 0;
};

///N-1 Flow Manager interface
class INMinusOneFlowManager {
public:
	virtual ~INMinusOneFlowManager(){};

	virtual void set_ipc_process(IPCProcess * ipc_process) = 0;

	virtual void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) = 0;

	/// Allocate an N-1 Flow with the requested QoS to the destination
	/// IPC Process
	/// @param flowInformation contains the destination IPC Process and requested
    /// QoS information
	/// @return handle to the flow request
	virtual unsigned int allocateNMinus1Flow(const rina::FlowInformation& flowInformation) = 0;

	/// Process the result of an allocate request event
	/// @param event
	/// @throws IPCException
	virtual void allocateRequestResult(const rina::AllocateFlowRequestResultEvent& event) = 0;

	/// Process a flow allocation request
	/// @param event
	/// @throws IPCException if something goes wrong
	virtual void flowAllocationRequested(const rina::FlowRequestEvent& event) = 0;

	/// Deallocate the N-1 Flow identified by portId
	/// @param portId
	/// @throws IPCException if no N-1 Flow identified by portId exists
	virtual void deallocateNMinus1Flow(int portId) = 0;

	/// Process the response of a flow deallocation request
	/// @throws IPCException
	virtual void deallocateFlowResponse(const rina::DeallocateFlowResponseEvent& event) = 0;

	/// A flow has been deallocated remotely, process
	/// @param portId
	virtual void flowDeallocatedRemotely(const rina::FlowDeallocatedEvent& event) = 0;

	/// Return the N-1 Flow descriptor associated to the flow identified by portId
	/// @param portId
	/// @return the N-1 Flow information
    /// @throws IPCException if no N-1 Flow identified by portId exists
	virtual const rina::FlowInformation& getNMinus1FlowInformation(int portId) const = 0;

	/// The IPC Process has been unregistered from or registered to an N-1 DIF
	/// @param evet
	/// @throws IPCException
	virtual void processRegistrationNotification(const rina::IPCProcessDIFRegistrationEvent& event) = 0;

	/// True if the DIF name is a supoprting DIF, false otherwise
	/// @param difName
	/// @return
	virtual bool isSupportingDIF(const rina::ApplicationProcessNamingInformation& difName) = 0;

	virtual std::list<rina::FlowInformation> getAllNMinusOneFlowInformation() const = 0;
};

/// Interface PDU Forwarding Table Generator Policy
class IPDUFTGeneratorPolicy {
public:
	virtual ~IPDUFTGeneratorPolicy(){};
	virtual void set_ipc_process(IPCProcess * ipc_process) = 0;
	virtual void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) = 0;
};

/// Interface PDU Forwarding Table Generator
class IPDUForwardingTableGenerator {
public:
	virtual ~IPDUForwardingTableGenerator(){};
	virtual void set_ipc_process(IPCProcess * ipc_process) = 0;
	virtual void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) = 0;
	virtual IPDUFTGeneratorPolicy * get_pdu_ft_generator_policy() const = 0;
};

/// Resource Allocator Interface
/// The Resource Allocator (RA) is the core of management in the IPC Process.
/// The degree of decentralization depends on the policies and how it is used. The RA has a set of meters
/// and dials that it can manipulate. The meters fall in 3 categories:
/// 	Traffic characteristics from the user of the DIF
/// 	Traffic characteristics of incoming and outgoing flows
/// 	Information from other members of the DIF
/// The Dials:
///     Creation/Deletion of QoS Classes
///     Data Transfer QoS Sets
///     Modifying Data Transfer Policy Parameters
///     Creation/Deletion of RMT Queues
///     Modify RMT Queue Servicing
///     Creation/Deletion of (N-1)-flows
///     Assignment of RMT Queues to (N-1)-flows
///     Forwarding Table Generator Output
class IResourceAllocator: public IPCProcessComponent {
public:
	virtual ~IResourceAllocator(){};
	virtual INMinusOneFlowManager * get_n_minus_one_flow_manager() const = 0;
	virtual IPDUForwardingTableGenerator * get_pdu_forwarding_table_generator() const = 0;
};

class IRIBDaemon;

/// Base RIB Object. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
class BaseRIBObject {
public:
	virtual ~BaseRIBObject(){};
	BaseRIBObject(IPCProcess* ipc_process,
                      const std::string& object_class,
                      long object_instance,
                      const std::string& object_name);
	rina::RIBObjectData get_data();
	virtual const void* get_value() const = 0;

	/// Parent-child management operations
	const std::list<BaseRIBObject*>& get_children() const;
	void add_child(BaseRIBObject * child);
	void remove_child(const std::string& objectName);

	/// Local invocations
	virtual void createObject(const std::string& objectClass,
                                  const std::string& objectName,
                                  const void* objectValue);
	virtual void deleteObject(const void* objectValue);
	virtual BaseRIBObject * readObject();
	virtual void writeObject(const void* object_value);
	virtual void startObject(const void* object);
	virtual void stopObject(const void* object);

	/// Remote invocations via CDAP messages
	virtual void remoteCreateObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	virtual void remoteDeleteObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	virtual void remoteReadObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	virtual void remoteCancelReadObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	virtual void remoteWriteObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	virtual void remoteStartObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	virtual void remoteStopObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	std::string class_;
	std::string name_;
	long instance_;
	BaseRIBObject * parent_;
	IPCProcess * ipc_process_;
	IRIBDaemon * rib_daemon_;
	Encoder * encoder_;

private:
	std::list<BaseRIBObject*> children_;
	void operation_not_supported();
	void operation_not_supported(const void* object);
	void operation_not_supported(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	void operartion_not_supported(const std::string& objectClass,
                                      const std::string& objectName,
			const void* objectValue);
};

/// Common interface for update strategies implementations. Can be on demand, scheduled, periodic
class IUpdateStrategy {
public:
	virtual ~IUpdateStrategy(){};
};

/// Interface of classes that handle CDAP response message.
class ICDAPResponseMessageHandler {
public:
	virtual ~ICDAPResponseMessageHandler(){};
	virtual void createResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;
	virtual void deleteResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;
	virtual void readResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;
	virtual void cancelReadResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;
	virtual void writeResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;
	virtual void startResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;
	virtual void stopResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) = 0;
};

class BaseCDAPResponseMessageHandler: public ICDAPResponseMessageHandler {
public:
	virtual void createResponse(const rina::CDAPMessage * cdapMessage,
                                    rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
                (void) cdapMessage; // Stop compiler barfs
                (void) cdapSessionDescriptor; // Stop compiler barfs
	}
	virtual void deleteResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
                (void) cdapMessage; // Stop compiler barfs
                (void) cdapSessionDescriptor; // Stop compiler barfs
	}
	virtual void readResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
                (void) cdapMessage; // Stop compiler barfs
                (void) cdapSessionDescriptor; // Stop compiler barfs
	}
	virtual void cancelReadResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
                (void) cdapMessage; // Stop compiler barfs
                (void) cdapSessionDescriptor; // Stop compiler barfs
	}
	virtual void writeResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
                (void) cdapMessage; // Stop compiler barfs
                (void) cdapSessionDescriptor; // Stop compiler barfs
	}
	virtual void startResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
                (void) cdapMessage; // Stop compiler barfs
                (void) cdapSessionDescriptor; // Stop compiler barfs
	}
	virtual void stopResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
                (void) cdapMessage; // Stop compiler barfs
                (void) cdapSessionDescriptor; // Stop compiler barfs
	}
};

/// Part of the RIB Daemon API to control if the changes have to be notified
class NotificationPolicy {
public:
	NotificationPolicy(const std::list<int>& cdap_session_ids);
	std::list<int> cdap_session_ids_;
};

/// Interface that provides de RIB Daemon API
class IRIBDaemon : public IPCProcessComponent, public EventManager {
public:
	virtual ~IRIBDaemon(){};

	/// Add an object to the RIB
	/// @param ribHandler
	/// @param objectName
	/// @throws Exception
	virtual void addRIBObject(BaseRIBObject * ribObject) = 0;

	/// Remove an object from the RIB. Ownership is passed to the RIB daemon,
	/// who will delete the memory associated to the object.
	/// @param ribObject
	/// @throws Exception
	virtual void removeRIBObject(BaseRIBObject * ribObject) = 0;

	/// Remove an object from the RIB by objectname. Ownership is passed to the RIB daemon,
	/// who will delete the memory associated to the object.
	/// @param objectName
	/// @throws Exception
	virtual void removeRIBObject(const std::string& objectName) = 0;

	/// Send an information update, consisting on a set of CDAP messages, using the updateStrategy update strategy
	/// (on demand, scheduled). Takes ownership of the CDAP messages
	/// @param cdapMessages
	/// @param updateStrategy
	virtual void sendMessages(const std::list<const rina::CDAPMessage*>& cdapMessages,
			const IUpdateStrategy& updateStrategy) = 0;

	/// Causes a CDAP message to be sent. Takes ownership of the CDAP message
	/// @param cdapMessage the message to be sent
	/// @param sessionId the CDAP session id
	/// @param cdapMessageHandler the class to be called when the response message is received (if required)
	/// @throws Exception
	virtual void sendMessage(const rina::CDAPMessage & cdapMessage,
                                 int sessionId,
                                 ICDAPResponseMessageHandler * cdapMessageHandler) = 0;

	/// Causes a CDAP message to be sent. Takes ownership of the CDAPMessage
	/// @param cdapMessage the message to be sent
	/// @param sessionId the CDAP session id
	/// @param address the address of the IPC Process to send the Message To
	/// @param cdapMessageHandler the class to be called when the response message is received (if required)
	/// @throws Exception
	virtual void sendMessageToAddress(const rina::CDAPMessage & cdapMessage,
                                          int sessionId,
                                          unsigned int address,
                                          ICDAPResponseMessageHandler * cdapMessageHandler) = 0;

	/// The RIB Daemon has to process the CDAP message and,
	/// if valid, it will either pass it to interested subscribers and/or write to storage and/or modify other
	/// tasks data structures. It may be the case that the CDAP message is not addressed to an application
	/// entity within this IPC process, then the RMT may decide to rely the message to the right destination
	/// (after consulting an adequate forwarding table).
	/// @param message the encoded CDAP message
	/// @param length the length of the encoded message
	/// @param portId the portId the message came from
	virtual void cdapMessageDelivered(char* message,
                                          int length,
                                          int portId) = 0;

	/// Create or update an object in the RIB
	/// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @param objectValue the value of the object
	/// @param notify if not null notify some of the neighbors about the change
	/// @throws Exception
	virtual void createObject(const std::string& objectClass,
                                  const std::string& objectName,
                                  const void* objectValue,
                                  const NotificationPolicy * notificationPolicy) = 0;

	/// Delete an object from the RIB
	/// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @param object the value of the object
	/// @param notify if not null notify some of the neighbors about the change
	/// @throws Exception
	virtual void deleteObject(const std::string& objectClass,
                                  const std::string& objectName,
                                  const void* objectValue,
                                  const NotificationPolicy * notificationPolicy) = 0;

	/// Read an object from the RIB
	/// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @return a RIB object
	/// @throws Exception
	virtual BaseRIBObject * readObject(const std::string& objectClass,
			const std::string& objectName) = 0;

	/// Update the value of an object in the RIB
    /// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @param objectValue the new value of the object
	/// @param notify if not null notify some of the neighbors about the change
	/// @throws Exception
	virtual void writeObject(const std::string& objectClass,
                                 const std::string& objectName,
                                 const void* objectValue) = 0;

	/// Start an object at the RIB
	/// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @param objectValue the new value of the object
	/// @throws Exception
	virtual void startObject(const std::string& objectClass,
                                 const std::string& objectName,
                                 const void* objectValue) = 0;

	/// Stop an object at the RIB
	/// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @param objectValue the new value of the object
	/// @throws Exception
	virtual void stopObject(const std::string& objectClass,
                                const std::string& objectName,
                                const void* objectValue) = 0;

	/// Process a Query RIB Request from the IPC Manager
	/// @param event
	virtual void processQueryRIBRequestEvent(const rina::QueryRIBRequestEvent& event) = 0;

	virtual std::list<BaseRIBObject *> getRIBObjects() = 0;
};

/// IPC Process interface
class IPCProcess {
public:
	virtual ~IPCProcess(){};
	virtual unsigned short get_id() = 0;
	virtual IDelimiter* get_delimiter() = 0;
	virtual Encoder* get_encoder() = 0;
	virtual rina::CDAPSessionManagerInterface* get_cdap_session_manager() = 0;
	virtual IEnrollmentTask* get_enrollment_task() = 0;
	virtual IFlowAllocator* get_flow_allocator() = 0;
	virtual INamespaceManager* get_namespace_manager() = 0;
	virtual IResourceAllocator* get_resource_allocator() = 0;
	virtual IRIBDaemon* get_rib_daemon() = 0;
	virtual unsigned int get_address() const = 0;
	virtual void set_address(unsigned int address) = 0;
	virtual const rina::ApplicationProcessNamingInformation& get_name() const = 0;
	virtual const IPCProcessOperationalState& get_operational_state() const = 0;
	virtual void set_operational_state(const IPCProcessOperationalState& operational_state) = 0;
	virtual rina::DIFInformation& get_dif_information() const = 0;
	virtual void set_dif_information(const rina::DIFInformation& dif_information) = 0;
	virtual const std::list<rina::Neighbor*>& get_neighbors() const = 0;
	virtual unsigned int getAdressByname(const rina::ApplicationProcessNamingInformation& name) = 0;
};

/// Generates unique object instances
class ObjectInstanceGenerator: public rina::Lockable {
public:
	ObjectInstanceGenerator();
	long getObjectInstance();
private:
	long instance_;
};

/// Make Object instance generator singleton
extern Singleton<ObjectInstanceGenerator> objectInstanceGenerator;


/// A simple RIB object that just acts as a wrapper. Represents an object in the RIB that just
/// can be read or written, and whose read/write operations have no side effects other than
/// updating the value of the object
class SimpleRIBObject: public BaseRIBObject {
public:
	SimpleRIBObject(IPCProcess* ipc_process,
                        const std::string& object_class,
			const std::string& object_name,
                        const void* object_value);
	virtual const void* get_value() const;
	virtual void        writeObject(const void* object);

	/// Create has the semantics of update
	virtual void createObject(const std::string& objectClass,
                                  const std::string& objectName,
                                  const void* objectValue);

private:
	const void* object_value_;
};

/// Class SimpleSetRIBObject. A RIB object that is a set and has no side effects
class SimpleSetRIBObject: public SimpleRIBObject {
public:
	SimpleSetRIBObject(IPCProcess * ipc_process,
                           const std::string& object_class,
                           const std::string& set_member_object_class,
                           const std::string& object_name);
	void createObject(const std::string& objectClass,
                          const std::string& objectName,
                          const void* objectValue);

private:
	std::string set_member_object_class_;
};

/// Class SimpleSetMemberRIBObject. A RIB object that is member of a set
class SimpleSetMemberRIBObject: public SimpleRIBObject {
public:
	SimpleSetMemberRIBObject(IPCProcess* ipc_process,
                                 const std::string& object_class,
                                 const std::string& object_name,
                                 const void* object_value);
	virtual void deleteObject(const void* objectValue);
};

}

#endif

#endif
