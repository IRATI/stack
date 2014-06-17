/*
 * Flow Allocator
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef IPCP_FLOW_ALLOCATOR_HH
#define IPCP_FLOW_ALLOCATOR_HH

#ifdef __cplusplus

#include "components.h"

namespace rinad {

/// Encapsulates all the information required to manage a Flow
class Flow {
public:
	static const std::string FLOW_SET_RIB_OBJECT_NAME;
	static const std::string FLOW_SET_RIB_OBJECT_CLASS ;
	static const std::string FLOW_RIB_OBJECT_CLASS;

	enum IPCPFlowState {EMPTY, ALLOCATION_IN_PROGRESS, ALLOCATED, WAITING_2_MPL_BEFORE_TEARING_DOWN, DEALLOCATED};

	Flow();
	bool is_source() const;
	void set_source(bool source);
	const rina::ApplicationProcessNamingInformation& get_source_naming_info() const;
	void set_source_naming_info(const rina::ApplicationProcessNamingInformation &source_naming_info);
	const rina::ApplicationProcessNamingInformation& get_destination_naming_info() const;
	void set_destination_naming_info(const rina::ApplicationProcessNamingInformation& destination_naming_info);
	unsigned int get_source_port_id() const;
	void set_source_port_id(unsigned int source_port_id);
	unsigned int get_destination_port_id() const;
	void set_destination_port_id(unsigned int destination_port_id);
	unsigned int get_source_address() const;
	void set_source_address(unsigned int source_address);
	unsigned int get_destination_address() const;
	void set_destination_address(unsigned int destination_address);
	const std::list<rina::Connection>& get_connections() const;
	void set_connections(const std::list<rina::Connection>& connections);
	unsigned int get_current_connection_index() const;
	void set_current_connection_index(unsigned int current_connection_index);
	IPCPFlowState get_state() const;
	void set_state(IPCPFlowState state);
	const rina::FlowSpecification& get_flow_specification() const;
	void set_flow_specification(const rina::FlowSpecification& flow_specification);
	char* get_access_control() const;
	void set_access_control(char* access_control);
	unsigned int get_max_create_flow_retries() const;
	void set_max_create_flow_retries(unsigned int max_create_flow_retries);
	unsigned int get_create_flow_retries() const;
	void set_create_flow_retries(unsigned int create_flow_retries);
	unsigned int get_hop_count() const;
	void set_hop_count(unsigned int hopCount);
	std::string toString();

private:
	/// The application that requested the flow
	rina::ApplicationProcessNamingInformation source_naming_info_;

	/// The destination application of the flow
	rina::ApplicationProcessNamingInformation destination_naming_info_;

	/// The port-id returned to the Application process that requested the flow. This port-id is used for
    /// the life of the flow.
	unsigned int source_port_id_;

	/// The port-id returned to the destination Application process. This port-id is used for
	// the life of the flow
	unsigned int destination_port_id_;

	/// The address of the IPC process that is the source of this flow
	unsigned int source_address_;

	/// The address of the IPC process that is the destination of this flow
	unsigned int destination_address_;

	/// All the possible connections of this flow
	std::list<rina::Connection> connections_;

	/// The index of the connection that is currently Active in this flow
	unsigned int current_connection_index_;

	/// The status of this flow
	IPCPFlowState state_;

	/// The list of parameters from the AllocateRequest that generated this flow
	rina::FlowSpecification flow_specification_;

	/// TODO this is just a placeHolder for this piece of data
	char* access_control_;

	/// Maximum number of retries to create the flow before giving up.
	unsigned int max_create_flow_retries_;

	/// The current number of retries
	unsigned int create_flow_retries_;

	/// While the search rules that generate the forwarding table should allow for a
	/// natural termination condition, it seems wise to have the means to enforce termination.
	unsigned int hop_count_;

	///True if this IPC process is the source of the flow, false otherwise
	bool source_;
};

/// Flow Allocator Instance Interface
class IFlowAllocatorInstance {
public:
	virtual ~IFlowAllocatorInstance(){};

	/// Returns the portId associated to this Flow Allocator Instance
	/// @return
	virtual int get_port_id() = 0;

	/// Return the Flow object associated to this Flow Allocator Instance
	/// @return
	virtual Flow get_flow() = 0;

	/// Called by the FA to forward an Allocate request to a FAI
	/// @param event
	/// @param applicationCallback the callback to invoke the application for allocateResponse and any other calls
	/// @throws IPCException
	virtual void submitAllocateRequest(const rina::FlowRequestEvent& event) throw (Exception) = 0;

	virtual void processCreateConnectionResponseEvent(const rina::CreateConnectionResponseEvent& event) = 0;

	/// Called by the Flow Allocator when an M_CREATE CDAP PDU with a Flow object
	/// is received by the Flow Allocator
	/// @param flow
	/// @param portId the destination portid as decided by the Flow allocator
	/// @param requestMessate the CDAP request message
	/// @param underlyingPortId the port id to reply later on
	virtual void createFlowRequestMessageReceived(const Flow& flow, const rina::CDAPMessage& requestMessage,
			int underlyingPortId) = 0;

	/// When the FAI gets a Allocate_Response from the destination application, it formulates a Create_Response
	/// on the flow object requested.If the response was positive, the FAI will cause DTP and if required DTCP
	/// instances to be created to support this allocation. A positive Create_Response Flow is sent to the
	/// requesting FAI with the connection-endpoint-id and other information provided by the destination FAI.
	/// The Create_Response is sent to requesting FAI with the necessary information reflecting the existing flow,
	/// or an indication as to why the flow was refused.
	/// If the response was negative, the FAI does any necessary housekeeping and terminates.
	/// @param AllocateFlowResponseEvent - the reply from the application
	/// @throws IPCException
	virtual void submitAllocateResponse(const rina::AllocateFlowResponseEvent& event) = 0;

	virtual void processCreateConnectionResultEvent(const rina::CreateConnectionResultEvent& event) = 0;

	virtual void processUpdateConnectionResponseEvent(const rina::UpdateConnectionResponseEvent& event) = 0;

	/// When a deallocate primitive is invoked, it is passed to the FAI responsible for that port-id.
	/// The FAI sends an M_DELETE request CDAP PDU on the Flow object referencing the destination port-id, deletes the local
	/// binding between the Application and the DTP-instance and waits for a response.  (Note that
	/// the DTP and DTCP if it exists will be deleted automatically after 2MPL)
	/// @param the flow deallocate request event
	/// @throws IPCException
	virtual void submitDeallocate(const rina::FlowDeallocateRequestEvent& event) = 0;

	/// When this PDU is received by the FAI with this port-id, the FAI invokes a Deallocate.deliver to notify the local Application,
	/// deletes the binding between the Application and the local DTP-instance, and sends a Delete_Response indicating the result.
	virtual void deleteFlowRequestMessageReceived(const rina::CDAPMessage& requestMessage, int underlyingPortId) = 0;

	virtual long get_allocate_response_message_handle() = 0;
};

}

#endif

#endif
