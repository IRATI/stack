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

/**
 * Encapsulates all the information required to manage a Flow
 *
 */
class Flow {
	/*	Constants	*/
	public:
		static const std::string FLOW_SET_RIB_OBJECT_NAME;
		static const std::string FLOW_SET_RIB_OBJECT_CLASS ;
		static const std::string FLOW_RIB_OBJECT_CLASS;
	/*	Variables	*/
	public:
		enum IPCPFlowState {EMPTY, ALLOCATION_IN_PROGRESS, ALLOCATED, WAITING_2_MPL_BEFORE_TEARING_DOWN, DEALLOCATED};
	protected:
		/**
		 * The application that requested the flow
		 */
		rina::ApplicationProcessNamingInformation sourceNamingInfo;
		/**
		 * The destination application of the flow
		 */
		rina::ApplicationProcessNamingInformation destinationNamingInfo;
		/**
		 * The port-id returned to the Application process that requested the flow. This port-id is used for
		 * the life of the flow.
		 */
		int sourcePortId;
		/**
		 * The port-id returned to the destination Application process. This port-id is used for
		 * the life of the flow.
		 */
		int destinationPortId;
		/**
		 * The address of the IPC process that is the source of this flow
		 */
		long sourceAddress;
		/**
		 * The address of the IPC process that is the destination of this flow
		 */
		long destinationAddress;
		/**
		 * All the possible connections of this flow
		 */
		std::list<rina::Connection> connections;
		/**
		 * The index of the connection that is currently Active in this flow
		 */
		int currentConnectionIndex;
		/**
		 * The status of this flow
		 */
		IPCPFlowState state;
		/**
		 * The list of parameters from the AllocateRequest that generated this flow
		 */
		rina::FlowSpecification flowSpec;
		/**
		 * The list of policies that are used to control this flow. NOTE: Does this provide
		 * anything beyond the list used within the QoS-cube? Can we override or specialize those,
		 * somehow?
		 */
		std::map<std::string, std::string> policies;
		/**
		 * The merged list of parameters from QoS.policy-Default-Parameters and QoS-Params.
		 */
		std::map<std::string, std::string> policyParameters;
		/**
		 * TODO this is just a placeHolder for this piece of data
		 */
		char* accessControl;
		/**
		 * Maximum number of retries to create the flow before giving up.
		 */
		int maxCreateFlowRetries;
		/**
		 * The current number of retries
		 */
		int createFlowRetries;
		/**
		 * While the search rules that generate the forwarding table should allow for a
		 * natural termination condition, it seems wise to have the means to enforce termination.
		 */
		int hopCount;
		/**
		 * True if this IPC process is the source of the flow, false otherwise
		 */
		bool source;
	/*	Constructors and Destructors	*/
	public:
		Flow();
	/*	Accessors	*/
	bool isSource() const;
	void setSource(bool source);
	const rina::ApplicationProcessNamingInformation& getSourceNamingInfo() const;
	void setSourceNamingInfo(const rina::ApplicationProcessNamingInformation &sourceNamingInfo);
	const rina::ApplicationProcessNamingInformation& getDestinationNamingInfo() const;
	void setDestinationNamingInfo(const rina::ApplicationProcessNamingInformation &destinationNamingInfo);
	int getSourcePortId() const;
	void setSourcePortId(int sourcePortId);
	int getDestinationPortId() const;
	void setDestinationPortId(int destinationPortId);
	long getSourceAddress() const;
	void setSourceAddress(long sourceAddress);
	long getDestinationAddress() const;
	void setDestinationAddress(long destinationAddress);
	const std::list<rina::Connection>& getConnections() const;
	void setConnections(const std::list<rina::Connection> &connections);
	int getCurrentConnectionIndex() const;
	void setCurrentConnectionIndex(int currentConnectionIndex);
	IPCPFlowState getState() const;
	void setState(IPCPFlowState state);
	const rina::FlowSpecification& getFlowSpecification() const;
	void setFlowSpecification(const rina::FlowSpecification &flowSpec);
	const std::map<std::string, std::string>& getPolicies() const;
	void setPolicies(const std::map<std::string, std::string> &policies);
	const std::map<std::string, std::string>& getPolicyParameters() const;
	void setPolicyParameters(const std::map<std::string, std::string> &policyParameters);
	char* getAccessControl() const;
	void setAccessControl(char* accessControl);
	int getMaxCreateFlowRetries() const;
	void setMaxCreateFlowRetries(int maxCreateFlowRetries);
	int getCreateFlowRetries() const;
	void setCreateFlowRetries(int createFlowRetries);
	int getHopCount() const;
	void setHopCount(int hopCount);
	/*	Methods	*/
	public:
		std::string toString();
};

}

#endif

#endif
