/*
 * Flow Allocator
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef IPCP_FLOW_ALLOCATOR_HH
#define IPCP_FLOW_ALLOCATOR_HH

#include <librina/timer.h>

#include "common/concurrency.h"
#include "ipcp/components.h"

namespace rinad {

/// Flow Allocator Instance Interface
class IFlowAllocatorInstance : public rina::rib::RIBOpsRespHandler,
	public rina::ApplicationEntityInstance {
public:
	IFlowAllocatorInstance(const std::string& instance_id) :
		rina::ApplicationEntityInstance(instance_id) { };
	virtual ~IFlowAllocatorInstance() {
	}
	;

	/// Returns the portId associated to this Flow Allocator Instance
	/// @return
	virtual int get_port_id() const = 0;

	/// Return the Flow object associated to this Flow Allocator Instance
	/// @return
	virtual configs::Flow * get_flow() const = 0;

	/// True if FAI is no longer operative, false otherwise
	virtual bool isFinished() const = 0;

	/// Generate the flow object, create the local DTP and optionally DTCP instances, generate a CDAP
	/// M_CREATE request with the flow object and send it to the appropriate IPC process (search the
	/// directory and the directory forwarding table if needed)
	/// @param flowRequestEvent The flow allocation request
	/// @throws rina::Exception if there are not enough resources to fulfill the allocate request
	virtual void submitAllocateRequest(const rina::FlowRequestEvent& event,
					   unsigned int address = 0) = 0;

	virtual void processCreateConnectionResponseEvent(
			const rina::CreateConnectionResponseEvent& event) = 0;

	/// When an FAI is created with a Create_Request(Flow) as input, it will inspect the parameters
	/// first to determine if the requesting Application (Source_Naming_Info) has access to the requested
	/// Application (Destination_Naming_Info) by inspecting the Access Control parameter.  If not, a
	/// negative Create_Response primitive will be returned to the requesting FAI. If it does have access,
	/// the FAI will determine if the policies proposed are acceptable, invoking the NewFlowRequestPolicy.
	/// If not, a negative Create_Response is sent.  If they are acceptable, the FAI will invoke a
	/// Allocate_Request.deliver primitive to notify the requested Application that it has an outstanding
	/// allocation request.  (If the application is not executing, the FAI will cause the application
	/// to be instantiated.)
	/// @param flow
	/// @param object_name the object name
	/// @param invoke_id the invoke id for the M_CREATE_R message
	virtual void createFlowRequestMessageReceived(configs::Flow* flow,
						      const std::string& object_name,
						      int invoke_id) = 0;

	/// When the FAI gets a Allocate_Response from the destination application,
	/// it formulates a Create_Response on the flow object requested.If the
	/// response was positive, the FAI will cause DTP and if required DTCP instances
	/// to be created to support this allocation. A positive Create_Response Flow is
	/// sent to the requesting FAI with the connection-endpoint-id and other information
	/// provided by the destination FAI. The Create_Response is sent to requesting FAI
	/// with the necessary information reflecting the existing flow, or an indication as
	/// to why the flow was refused. If the response was negative, the FAI does any
	/// necessary housekeeping and terminates.
	/// @param AllocateFlowResponseEvent - the reply from the application
	/// @throws IPCException
	virtual void submitAllocateResponse(
			const rina::AllocateFlowResponseEvent& event) = 0;

	virtual void processCreateConnectionResultEvent(
			const rina::CreateConnectionResultEvent& event) = 0;

	virtual void processUpdateConnectionResponseEvent(
			const rina::UpdateConnectionResponseEvent& event) = 0;

	/// When a deallocate primitive is invoked, it is passed to the FAI responsible
	/// for that port-id. The FAI sends an M_DELETE request CDAP PDU on the Flow
	/// object referencing the destination port-id, deletes the local binding between
	/// the Application and the DTP-instance and waits for a response.  (Note that
	/// the DTP and DTCP if it exists will be deleted automatically after 2MPL)
	/// @param the flow deallocate request event
	/// @throws IPCException
	virtual void submitDeallocate(
			const rina::FlowDeallocateRequestEvent& event) = 0;

	/// When this PDU is received by the FAI with this port-id, the FAI invokes
	/// a Deallocate.deliver to notify the local Application,
	/// deletes the binding between the Application and the local DTP-instance,
	/// and sends a Delete_Response indicating the result.
	virtual void deleteFlowRequestMessageReceived() = 0;

	virtual void modify_flow_request(const configs::Flow & flow) = 0;

	virtual void destroyFlowAllocatorInstance(const std::string& flowObjectName,
			bool requestor) = 0;

	virtual unsigned int get_allocate_response_message_handle() const = 0;
	virtual void set_allocate_response_message_handle(
			unsigned int allocate_response_message_handle) = 0;
	virtual void sync_with_kernel() = 0;
};

/// Representation of a flow object in the RIB
class FlowRIBObject: public IPCPRIBObj {
public:
	FlowRIBObject(IPCProcess * ipc_process,
		      IFlowAllocatorInstance * flow_allocator_instance);
	const std::string get_displayable_value() const;

	const std::string& get_class() const {
		return class_name;
	};

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	void write(const rina::cdap_rib::con_handle_t &con,
		   const std::string& fqn,
		   const std::string& class_,
		   const rina::cdap_rib::filt_info_t &filt,
		   const int invoke_id,
		   const rina::ser_obj_t &obj_req,
		   rina::ser_obj_t &obj_reply,
		   rina::cdap_rib::res_info_t& res);

	bool delete_(const rina::cdap_rib::con_handle_t &con,
		     const std::string& fqn,
		     const std::string& class_,
		     const rina::cdap_rib::filt_info_t &filt,
		     const int invoke_id,
		     rina::cdap_rib::res_info_t& res);

	static void create_cb(const rina::rib::rib_handle_t rib,
			      const rina::cdap_rib::con_handle_t &con,
			      const std::string& fqn,
			      const std::string& class_,
			      const rina::cdap_rib::filt_info_t &filt,
			      const int invoke_id,
			      const rina::ser_obj_t &obj_req,
			      rina::cdap_rib::obj_info_t &obj_reply,
			      rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name_prefix;

private:
	IFlowAllocatorInstance * flow_allocator_instance_;
};

/// Representation of a set of Flow objects in the RIB
class FlowsRIBObject: public IPCPRIBObj {
public:
	FlowsRIBObject(IPCProcess * ipc_process,
		       IFlowAllocator * flow_allocator);

	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name;
private:
	IFlowAllocator * flow_allocator_;
};

/// Representation of a set of Connection objects in the RIB
class ConnectionsRIBObj: public IPCPRIBObj {
public:
	ConnectionsRIBObj(IPCProcess * ipc_process,
		       IFlowAllocator * flow_allocator);

	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name;
private:
	IFlowAllocator * flow_allocator_;
};

/// Representation of a connection object in the RIB
class ConnectionRIBObject: public IPCPRIBObj {
public:
	ConnectionRIBObject(IPCProcess * ipc_process,
			    IFlowAllocatorInstance * fai);
	const std::string get_displayable_value() const;

	const std::string& get_class() const {
		return class_name;
	};

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name_prefix;

private:
	IFlowAllocatorInstance * fai;
};

/// Representation of a connection object in the RIB
class DTCPRIBObject: public IPCPRIBObj {
public:
	DTCPRIBObject(IPCProcess * ipc_process,
		      IFlowAllocatorInstance * fai);
	const std::string get_displayable_value() const;

	const std::string& get_class() const {
		return class_name;
	};

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name_suffix;

private:
	IFlowAllocatorInstance * fai;
};

struct OngoingFlowAllocState {
	rina::FlowRequestEvent flow_event;
	configs::Flow * flow;
	std::string object_name;
	int invoke_id;
	bool local_request;
	unsigned int address;
};

class FlowAllocatorInstance;

/// Implementation of the Flow Allocator component
class FlowAllocator: public IFlowAllocator, public rina::InternalEventListener {
public:
	FlowAllocator();
	~FlowAllocator();
	void eventHappened(rina::InternalEvent * event);
	IFlowAllocatorInstance * getFAI(int portId);
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFInformation& dif_information);
	void createFlowRequestMessageReceived(configs::Flow * flow,
					      const std::string& object_name,
					      int invoke_id);
	void submitAllocateRequest(const rina::FlowRequestEvent& flowRequestEvent,
				   unsigned int address = 0);
	void processCreateConnectionResponseEvent(
			const rina::CreateConnectionResponseEvent& event);
	void submitAllocateResponse(const rina::AllocateFlowResponseEvent& event);
	void processCreateConnectionResultEvent(
			const rina::CreateConnectionResultEvent& event);
	void processUpdateConnectionResponseEvent(
			const rina::UpdateConnectionResponseEvent& event);
	void submitDeallocate(const rina::FlowDeallocateRequestEvent& event);
	void removeFlowAllocatorInstance(int portId);
	void sync_with_kernel();
	void processAllocatePortResponse(const rina::AllocatePortResponseEvent& event);
	void processDeallocatePortResponse(const rina::DeallocatePortResponseEvent& event);
	void address_changed(unsigned int new_address, unsigned int old_address);

        // Plugin support
        configs::Flow* createFlow() { return new configs::Flow(); }
        void destroyFlow(configs::Flow* flow) { if (flow) delete flow; }

        // Constants
        const static int DEALLOCATE_PORT_DELAY;
        const static int TEARDOWN_FLOW_DELAY;

private:
	IPCPRIBDaemon * rib_daemon_;
	INamespaceManager * namespace_manager_;
	rina::Timer timer;

	std::map<unsigned int, OngoingFlowAllocState> pending_port_allocs;
	rina::Lockable port_alloc_lock;
	std::map<int, FlowAllocatorInstance *> fa_instances;
	rina::Lockable fai_lock;

	/// Create initial RIB objects
	void populateRIB();
	void subscribeToEvents();

	/// Reply to the IPC Manager
	void replyToIPCManager(const rina::FlowRequestEvent& event, int result);

	void __submitAllocateRequest(const rina::FlowRequestEvent& event,
				     int port_id,
				     unsigned int address);
	void __createFlowRequestMessageReceived(configs::Flow * flow,
		             	     	     	const std::string& object_name,
						int invoke_id,
						int port_id);
};

class FAAddressChangeTimerTask: public rina::TimerTask {
public:
	FAAddressChangeTimerTask(FlowAllocator * fa,
			       unsigned int naddr,
			       unsigned int oaddr);
	~FAAddressChangeTimerTask() throw() {};
	void run();
	std::string name() const {
		return "fa-address-change";
	}

	FlowAllocator * fall;
	unsigned int new_address;
	unsigned int old_address;
};

///Implementation of the FlowAllocatorInstance
class FlowAllocatorInstance: public IFlowAllocatorInstance {
public:
	enum FAIState {
		NO_STATE,
		CONNECTION_CREATE_REQUESTED,
		MESSAGE_TO_PEER_FAI_SENT,
		APP_NOTIFIED_OF_INCOMING_FLOW,
		CONNECTION_UPDATE_REQUESTED,
		FLOW_ALLOCATED,
		CONNECTION_DESTROY_REQUESTED,
		WAITING_2_MPL_BEFORE_TEARING_DOWN,
		FINISHED
	};

	FlowAllocatorInstance(IPCProcess * ipc_process,
			      IFlowAllocator * flow_allocator,
			      int port_id,
			      bool loc,
			      const std::string& instance_id,
			      rina::Timer * timer);
	~FlowAllocatorInstance();
	void set_application_entity(rina::ApplicationEntity * ae);
	int get_port_id() const;
	configs::Flow * get_flow() const;
	bool isFinished() const;
	unsigned int get_allocate_response_message_handle() const;
	void set_allocate_response_message_handle(
			unsigned int allocate_response_message_handle);
	void submitAllocateRequest(const rina::FlowRequestEvent& event,
				   unsigned int address = 0);
	void processCreateConnectionResponseEvent(
			const rina::CreateConnectionResponseEvent& event);
	void createFlowRequestMessageReceived(configs::Flow * flow,
					      const std::string& object_name,
					      int invoke_id);
	void processCreateConnectionResultEvent(
			const rina::CreateConnectionResultEvent& event);
	void submitAllocateResponse(const rina::AllocateFlowResponseEvent& event);
	void processUpdateConnectionResponseEvent(
			const rina::UpdateConnectionResponseEvent& event);
	void submitDeallocate(const rina::FlowDeallocateRequestEvent& event);
	void deleteFlowRequestMessageReceived();
	void destroyFlowAllocatorInstance(const std::string& flowObjectName,
			bool requestor);

	void modify_flow_request(const configs::Flow & flow);

	/// If the response to the allocate request is negative
	/// the Allocation invokes the AllocateRetryPolicy. If the AllocateRetryPolicy returns a
	/// positive result, a new Create_Flow Request is sent and the CreateFlowTimer is reset.
	/// Otherwise, if the AllocateRetryPolicy returns a negative result or the MaxCreateRetries
	/// has been exceeded, an Allocate_Request.deliver primitive to notify the Application that
	/// the flow could not be created. (If the reason was "Application Not Found", the primitive
	/// will be delivered to the Inter-DIF Directory to search elsewhere.The FAI deletes the DTP
	/// and DTCP instances it created and does any other housekeeping necessary, before
	/// terminating.  If the response is positive, it completes the binding of the DTP-instance
	/// with this connection-endpoint-id to the requesting Application and invokes a
	/// Allocate_Request.submit primitive to notify the requesting Application that its allocation
	/// request has been satisfied.
	void remoteCreateResult(const rina::cdap_rib::con_handle_t &con,
				const rina::cdap_rib::obj_info_t &obj,
				const rina::cdap_rib::res_info_t &res);

	void sync_with_kernel();

	void address_changed(unsigned int new_address,
			     unsigned int old_address);

private:

	void initialize(IPCProcess * ipc_process,
			IFlowAllocator * flow_allocator,
			int port_id, bool local,
			rina::Timer * timer);
	void replyToIPCManager(int result);
	void complete_flow_allocation(bool success);

	/// Release the port-id, unlock and remove the FAI from the FA
	void release_remove();

	IPCProcess * ipc_process_;
	IFlowAllocator * flow_allocator_;
	IPCPRIBDaemon * rib_daemon_;
	INamespaceManager * namespace_manager_;
	IPCPSecurityManager * security_manager_;
	FAIState state;

	rina::Timer * timer;

	/// The portId associated to this Flow Allocator instance
	int port_id_;

	/// The flow object related to this Flow Allocator Instance
	configs::Flow * flow_;

	/// The event requesting the allocation of the flow
	rina::FlowRequestEvent flow_request_event_;

	/// The name of the flow object associated to this FlowAllocatorInstance
	std::string object_name_;

	unsigned int allocate_response_message_handle_;
	int invoke_id_;
	bool local;
	rina::Lockable lock_;
};

class TearDownFlowTimerTask: public rina::TimerTask {
public:

	TearDownFlowTimerTask(FlowAllocatorInstance * flow_allocator_instance,
			const std::string& flow_object_name, bool requestor);
	~TearDownFlowTimerTask() throw () {
	}

	void run();
	std::string name() const {
		return "tear-down-flow";
	}

private:
	FlowAllocatorInstance * flow_allocator_instance_;
	std::string flow_object_name_;
	bool requestor_;
};

class DeallocatePortTimerTask: public rina::TimerTask {
public:
	DeallocatePortTimerTask(int port_id_): port_id(port_id_){};
	~DeallocatePortTimerTask() throw () {}

	void run();
	std::string name() const {
		return "deallocate-port";
	}

private:
	int port_id;
};

class DataTransferRIBObj: public IPCPRIBObj {
public:
	DataTransferRIBObj(IPCProcess * ipc_process);
	const std::string get_displayable_value() const;
	const std::string& get_class() const {
		return class_name;
	};

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::ser_obj_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	void create(const rina::cdap_rib::con_handle_t &con,
		    const std::string& fqn,
		    const std::string& class_,
		    const rina::cdap_rib::filt_info_t &filt,
		    const int invoke_id,
		    const rina::ser_obj_t &obj_req,
		    rina::ser_obj_t &obj_reply,
		    rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name;
};
} //namespace rinad

#endif //IPCP_FLOW_ALLOCATOR_HH
