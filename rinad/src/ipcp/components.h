/*
 * Common interfaces and constants of the IPC Process components
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#ifndef IPCP_COMPONENTS_HH
#define IPCP_COMPONENTS_HH

#include <list>
#include <vector>
#include <string>

#include <librina/ipc-process.h>
#include <librina/internal-events.h>
#include <librina/irm.h>
#include <librina/rib_v2.h>
#include <librina/security-manager.h>
#include "common/encoder.h"
#include "common/configuration.h"

namespace rinad {

const unsigned int max_sdu_size_in_bytes = 10000;

enum IPCProcessOperationalState {
	NOT_INITIALIZED,
	INITIALIZED,
	ASSIGN_TO_DIF_IN_PROCESS,
	ASSIGNED_TO_DIF
};

class IPCProcess;

/// IPC process component
class IPCProcessComponent {
public:
        IPCProcessComponent() : ipcp(NULL) { };
        virtual ~IPCProcessComponent() { };
        virtual void set_dif_configuration(const rina::DIFInformation& dif_information) = 0;

        // Periodically read the kernel information exported via sysfs relevant
        // to this IPCP component, by default do nothing
        virtual void sync_with_kernel() { };

        IPCProcess * ipcp;
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

class IEnrollmentStateMachine;

/// Interface that must be implementing by classes that provide
/// the behavior of an enrollment task
class IPCPEnrollmentTask : public IPCProcessComponent,
			   public rina::IEnrollmentTask {
public:
	IPCPEnrollmentTask() : IEnrollmentTask() { };
	virtual ~IPCPEnrollmentTask(){};
	virtual IEnrollmentStateMachine * getEnrollmentStateMachine(int portId, bool remove) = 0;
	virtual void deallocateFlow(int portId) = 0;
	virtual void add_enrollment_state_machine(int portId, IEnrollmentStateMachine * stateMachine) = 0;
	virtual void update_neighbor_address(const rina::Neighbor& neighbor) = 0;
	// Return the con_handle to the next hop to reach the address
	virtual int get_con_handle_to_ipcp(unsigned int address,
					   rina::cdap_rib::con_handle_t& con) = 0;
	virtual unsigned int get_con_handle_to_ipcp(const std::string& ipcp_name,
					   	    rina::cdap_rib::con_handle_t& con) = 0;
	virtual int get_neighbor_info(rina::Neighbor& neigh) = 0;
	virtual void clean_state(unsigned int port_id) = 0;

	/// The maximum time to wait between steps of the enrollment sequence (in ms)
	int timeout_;

	/// Maximum number of enrollment attempts
	unsigned int max_num_enroll_attempts_;

	/// Watchdog period in ms
	int watchdog_per_ms_;

	/// The neighbor declared dead interval
	int declared_dead_int_ms_;

	/// True if a reliable_n_flow is to be used, false otherwise
	bool use_reliable_n_flow;

	/// List of N-1 flows to create when enrolling to a neighbor,
	/// with their QoS characteristics
	std::map< std::string, std::list<rina::FlowSpecification> > n1_flows_to_create;

	/// List of N-1 DIFs for automatic peer discovery
	std::list<std::string> n1_difs_peer_discovery;

	/// The peer discovery period in ms
	int peer_discovery_period_ms;

	/// The maximum number of peer discovery attempts
	int max_peer_discovery_attempts;
};

/// Policy set of the IPCP enrollment task
class IPCPEnrollmentTaskPS : public rina::IPolicySet {
public:
        virtual ~IPCPEnrollmentTaskPS() {};
	virtual void connect_received(const rina::cdap::CDAPMessage& cdapMessage,
			     	      const rina::cdap_rib::con_handle_t &con) = 0;
        virtual void connect_response_received(int result,
        				       const std::string& result_reason,
        				       const rina::cdap_rib::con_handle_t &con,
					       const rina::cdap_rib::auth_policy_t& auth) = 0;
        virtual void process_authentication_message(const rina::cdap::CDAPMessage& message,
        					    const rina::cdap_rib::con_handle_t &con) = 0;
	virtual void authentication_completed(int port_id, bool success) = 0;
        virtual void initiate_enrollment(const rina::NMinusOneFlowAllocatedEvent & event,
        				 const rina::EnrollmentRequest& request) = 0;
        virtual void inform_ipcm_about_failure(IEnrollmentStateMachine * state_machine) = 0;
        virtual void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) = 0;
};

class IFlowAllocatorInstance;

class IFlowAllocatorPs : public rina::IPolicySet {
// This class is used by the IPCP to access the plugin functionalities
public:
        virtual configs::Flow *newFlowRequest(IPCProcess * ipc_process,
                        const rina::FlowRequestEvent& flowRequestEvent) = 0;

        virtual ~IFlowAllocatorPs() {}
};

/// Interface that must be implementing by classes that provide
/// the behavior of a Flow Allocator task
class IFlowAllocator : public IPCProcessComponent, public rina::ApplicationEntity {
public:
	static const std::string FLOW_ALLOCATOR_AE_NAME;

	IFlowAllocator() : rina::ApplicationEntity(FLOW_ALLOCATOR_AE_NAME) { };
	virtual ~IFlowAllocator(){};

	virtual IFlowAllocatorInstance * getFAI(int portId) = 0;

	/// The Flow Allocator is invoked when an Allocate_Request.submit is received.  The source Flow
	/// Allocator determines if the request is well formed.  If not well-formed, an Allocate_Response.deliver
	/// is invoked with the appropriate error code.  If the request is well-formed, a new instance of an
	/// FlowAllocator is created and passed the parameters of this Allocate_Request to handle the allocation.
	/// It is a matter of DIF policy (AllocateNoificationPolicy) whether an Allocate_Request.deliver is invoked
	/// with a status of pending, or whether a response is withheld until an Allocate_Response can be delivered
	/// with a status of success or failure.
	/// @param allocateRequest the characteristics of the flow to be allocated.
	/// @param address if the task that requests the flow already knows the address where to forward the
	/// flow request, then the DFT lookup is ommitted
	/// to honour the request
	virtual void submitAllocateRequest(const rina::FlowRequestEvent& flowRequestEvent,
					   unsigned int address = 0) = 0;

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
	virtual void createFlowRequestMessageReceived(configs::Flow * flow,
						      const std::string& object_name,
						      int invoke_id) = 0;

	/// Called by the flow allocator instance when it finishes to cleanup the state.
	/// @param portId
	virtual void removeFlowAllocatorInstance(int portId) = 0;

	virtual void processAllocatePortResponse(const rina::AllocatePortResponseEvent& event) = 0;
	virtual void processDeallocatePortResponse(const rina::DeallocatePortResponseEvent& event) = 0;

        // Plugin support
	virtual configs::Flow* createFlow() = 0;
	virtual void destroyFlow(configs::Flow *) = 0;
};

class IRoutingPs : public rina::IPolicySet {
	// This class is used by the IPCP to access the plugin functionalities
public:
	virtual ~IRoutingPs() {};
	virtual void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) = 0;
};

class IRoutingComponent : public IPCProcessComponent, public rina::ApplicationEntity {
public:
	static const std::string ROUTING_COMPONENT_AE_NAME;
	IRoutingComponent() : rina::ApplicationEntity(ROUTING_COMPONENT_AE_NAME) { };
	virtual ~IRoutingComponent(){};
};

class RoutingComponent: public IRoutingComponent {
public:
	RoutingComponent() : IRoutingComponent() { };
	~RoutingComponent();
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFInformation& dif_information);
};

/// Namespace Manager Interface
class INamespaceManagerPs : public rina::IPolicySet {
// This class is used by the IPCP to access the plugin functionalities
public:
	/// Decides if a given address is valid or not
	/// @param address
	///	@return true if valid, false otherwise
	virtual bool isValidAddress(unsigned int address, const std::string& ipcp_name,
			const std::string& ipcp_instance) = 0;

	/// Return a valid address for the IPC process that
	/// wants to join the DIF
	virtual unsigned int getValidAddress(const std::string& ipcp_name,
				const std::string& ipcp_instance) = 0;

	virtual void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) = 0;

	virtual ~INamespaceManagerPs() {}
};

class INamespaceManager : public IPCProcessComponent, public rina::ApplicationEntity {
public:
	static const std::string NAMESPACE_MANAGER_AE_NAME;
	INamespaceManager() : rina::ApplicationEntity(NAMESPACE_MANAGER_AE_NAME) { };
	virtual ~INamespaceManager(){};

	/// Returns the address of the IPC process where the application process is, or
	/// null otherwise
	/// @param apNamingInfo
	/// @return
	virtual unsigned int getDFTNextHop(rina::ApplicationProcessNamingInformation& apNamingInfo) = 0;

	/// Returns the IPC Process id (0 if not an IPC Process) of the registered
	/// application, or -1 if the app is not registered
	/// @param apNamingInfo
	/// @return
	virtual unsigned short getRegIPCProcessId(rina::ApplicationProcessNamingInformation& apNamingInfo) = 0;

	/// Add an entry to the directory forwarding table
	/// @param entry
	virtual void addDFTEntries(const std::list<rina::DirectoryForwardingTableEntry>& entries,
				   bool notify_neighs,
				   std::list<int>& neighs_to_exclude) = 0;

	/// Get an entry from the application name
	/// @param apNamingInfo
	/// @return
	virtual rina::DirectoryForwardingTableEntry * getDFTEntry(const std::string& key) = 0;

	virtual std::list<rina::DirectoryForwardingTableEntry> getDFTEntries() = 0;

	/// Remove an entry from the directory forwarding table
	/// @param apNamingInfo
	virtual void removeDFTEntry(const std::string& key,
			 	    bool notify_neighs,
			 	    bool remove_from_rib,
			 	    std::list<int>& neighs_to_exclude) = 0;

	/// Process an application registration request
	/// @param event
	virtual void processApplicationRegistrationRequestEvent(
			const rina::ApplicationRegistrationRequestEvent& event) = 0;

	/// Process an application unregistration request
	/// @param event
	virtual void processApplicationUnregistrationRequestEvent(
			const rina::ApplicationUnregistrationRequestEvent& event) = 0;

	virtual unsigned int getAdressByname(const rina::ApplicationProcessNamingInformation& name) = 0;

	virtual rina::ApplicationRegistrationInformation
		get_reg_app_info(const rina::ApplicationProcessNamingInformation name) = 0;

	virtual std::list<rina::WhatevercastName> get_whatevercast_names() = 0;

	virtual void add_whatevercast_name(rina::WhatevercastName * name) = 0;

	virtual void remove_whatevercast_name(const std::string& name_key) = 0;

	virtual void addressChangeUpdateDFT(unsigned int new_address,
				    	    unsigned int old_address) = 0;

	virtual void notify_neighbors_add(const std::list<rina::DirectoryForwardingTableEntry>& entries,
			          std::list<int>& neighs_to_exclude) = 0;
};

///N-1 Flow Manager interface
class INMinusOneFlowManager : public rina::IPCResourceManager {
public:
	INMinusOneFlowManager() : rina::IPCResourceManager(true) { };
	virtual ~INMinusOneFlowManager(){};

	virtual void set_ipc_process(IPCProcess * ipc_process) = 0;

	virtual void set_dif_configuration(const rina::DIFInformation& dif_information) = 0;

	/// The IPC Process has been unregistered from or registered to an N-1 DIF
	/// @param evet
	/// @throws IPCException
	virtual void processRegistrationNotification(const rina::IPCProcessDIFRegistrationEvent& event) = 0;

	virtual std::list<int> getNMinusOneFlowsToNeighbour(unsigned int address) = 0;

	virtual std::list<int> getNMinusOneFlowsToNeighbour(const std::string& name) = 0;

	virtual int getManagementFlowToNeighbour(const std::string& name) = 0;

	virtual int getManagementFlowToNeighbour(unsigned int address) = 0;

	virtual int get_n1flow_to_neighbor(const rina::FlowSpecification& fspec,
					   const std::string& name) = 0;

	virtual std::list<int> getManagementFlowsToAllNeighbors(void) = 0;

	virtual unsigned int numberOfFlowsToNeighbour(const std::string& apn,
			const std::string& api) = 0;
};

/// PDUFT Generator Policy Set Interface
class IPDUFTGeneratorPs : public rina::IPolicySet {
// This class is used by the IPCP to access the plugin functionalities
public:
	/// The routing table has been updated; decide if
	/// the PDU Forwarding Table has to be updated and do it
	///	@return true if valid, false otherwise
	virtual void routingTableUpdated(const std::list<rina::RoutingTableEntry*>& routing_table) = 0;
	virtual void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) = 0;

	virtual ~IPDUFTGeneratorPs() {}
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
class IResourceAllocator: public IPCProcessComponent, public rina::ApplicationEntity {
public:
	static const std::string RESOURCE_ALLOCATOR_AE_NAME;
	static const std::string PDUFT_GEN_COMPONENT_NAME;

	IResourceAllocator() : rina::ApplicationEntity(RESOURCE_ALLOCATOR_AE_NAME),
			pduft_gen_ps(NULL){ };
	virtual ~IResourceAllocator(){};
	virtual INMinusOneFlowManager * get_n_minus_one_flow_manager() const = 0;
	virtual std::list<rina::QoSCube*> getQoSCubes() = 0;
	int set_pduft_gen_policy_set(const std::string& name);
	virtual void addQoSCube(const rina::QoSCube& cube) = 0;

	virtual std::list<rina::PDUForwardingTableEntry> get_pduft_entries() = 0;
	/// This operation takes ownership of the entries
	virtual void set_pduft_entries(const std::list<rina::PDUForwardingTableEntry*>& pduft) = 0;

	virtual std::list<rina::RoutingTableEntry> get_rt_entries() = 0;
	/// This operation takes ownership of the entries
	virtual void set_rt_entries(const std::list<rina::RoutingTableEntry*>& rt) = 0;
	// Returns the next hop addresses towards the destination
	virtual int get_next_hop_addresses(unsigned int dest_address,
					   std::list<unsigned int>& addresses) = 0;
	// Returns the next hop names towards the destination
	virtual int get_next_hop_name(const std::string& dest_name, std::string& name) = 0;
	// Returns the N-1 port towards the destination
	virtual unsigned int get_n1_port_to_address(unsigned int dest_address) = 0;

	/// Add a temporary entry to the PDU FTE, until the routing policy
	/// provides it when it modifies the forwarding table.
	/// Takes ownership of the entry.
	virtual void add_temp_pduft_entry(unsigned int dest_address, int port_id) = 0;
	virtual void remove_temp_pduft_entry(unsigned int dest_address) = 0;

	IPDUFTGeneratorPs * pduft_gen_ps;
};

class IPCPSecurityManagerPs : public rina::ISecurityManagerPs {
// This class is used by the IPCP to access the plugin functionalities
public:
	/// Decide if a new flow to the IPC process should be accepted
	virtual bool acceptFlow(const configs::Flow& newFlow) = 0;

        virtual ~IPCPSecurityManagerPs() {}
};

class IPCPSecurityManager: public rina::ISecurityManager, public IPCProcessComponent {
// Used by IPCP to access the functionalities of the security manager
public:
	IPCPSecurityManager(){ };
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFInformation& dif_information);
	~IPCPSecurityManager();
	rina::AuthSDUProtectionProfile get_auth_sdup_profile(const std::string& under_dif_name);
        rina::IAuthPolicySet::AuthStatus update_crypto_state(const rina::CryptoState& state,
        						     rina::IAuthPolicySet * caller);
        void process_update_crypto_state_response(const rina::UpdateCryptoStateResponseEvent& event);

private:
	rina::SecurityManagerConfiguration config;
	rina::Lockable lock;
	std::map<unsigned int, rina::IAuthPolicySet *> pending_update_crypto_state_requests;
};

class IPCPRIBDaemon;

/// Base RIB Object. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
class IPCPRIBObj: public rina::rib::RIBObj {
public:
	virtual ~IPCPRIBObj(){};
	IPCPRIBObj(IPCProcess* ipc_process,
                   const std::string& object_class);

	IPCProcess * ipc_process_;
	IPCPRIBDaemon * rib_daemon_;
};

/// Interface that provides the RIB Daemon API
class IPCPRIBDaemon : public rina::rib::RIBDaemonAE, public IPCProcessComponent {

public:
	IPCPRIBDaemon() { };
	virtual ~IPCPRIBDaemon(){};

	/// Process a Query RIB Request from the IPC Manager
	/// @param event
	virtual void processQueryRIBRequestEvent(const rina::QueryRIBRequestEvent& event) = 0;
	virtual const rina::rib::rib_handle_t & get_rib_handle() = 0;
        virtual int64_t addObjRIB(const std::string& fqn,
        			  rina::rib::RIBObj** obj) = 0;
        virtual void removeObjRIB(const std::string& fqn) = 0;
        virtual void processReadManagementSDUEvent(rina::ReadMgmtSDUResponseEvent& event) = 0;
};

/// IPC Process interface
class IPCProcess : public rina::ApplicationProcess {
public:
	static const std::string MANAGEMENT_AE;
	static const std::string DATA_TRANSFER_AE;
	static const int DEFAULT_MAX_SDU_SIZE_IN_BYTES;

	IDelimiter * delimiter_;
	rina::InternalEventManager * internal_event_manager_;
	IPCPEnrollmentTask * enrollment_task_;
	IFlowAllocator * flow_allocator_;
	INamespaceManager * namespace_manager_;
	IResourceAllocator * resource_allocator_;
	IPCPSecurityManager * security_manager_;
	IRoutingComponent * routing_component_;
	IPCPRIBDaemon * rib_daemon_;

	IPCProcess(const std::string& name, const std::string& instance);
	virtual ~IPCProcess(){};
	virtual unsigned short get_id() = 0;
	virtual void set_address(unsigned int address) = 0;
	virtual const IPCProcessOperationalState& get_operational_state() const = 0;
	virtual void set_operational_state(const IPCProcessOperationalState& operational_state) = 0;
	virtual rina::DIFInformation& get_dif_information() = 0;
	virtual void set_dif_information(const rina::DIFInformation& dif_information) = 0;
	virtual const std::list<rina::Neighbor> get_neighbors() const = 0;
	virtual unsigned int get_old_address() = 0;
	virtual unsigned int get_active_address() = 0;
	virtual bool check_address_is_mine(unsigned int address) = 0;
};

} //namespace rinad

#endif //IPCP_COMPONENTS_HH
