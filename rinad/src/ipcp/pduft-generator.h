/*
 * PDU Forwarding Table Generator
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

#ifndef IPCP_PDUFT_GENERATOR_HH
#define IPCP_PDUFT_GENERATOR_HH

#ifdef __cplusplus

#include <set>

#include "ipcp/components.h"
#include <librina/timer.h>

namespace rinad {

class PDUForwardingTableGenerator : public IPDUForwardingTableGenerator {
public:
	static const std::string LINK_STATE_POLICY;

	PDUForwardingTableGenerator();
	void set_ipc_process(IPCProcess * ipc_process);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	IPDUFTGeneratorPolicy * get_pdu_ft_generator_policy() const;

private:
	IPCProcess * ipc_process_;
	IPDUFTGeneratorPolicy * pduftg_policy_;
};

/// Captures the state of an N-1 flow
class FlowStateObject {
public:
	FlowStateObject(unsigned int address, int port_id, unsigned int neighbor_address,
			int neighbor_port_id, bool state, int sequence_number, int age);
	const std::string toString();

	// The address of the IPC Process
	unsigned int address_;

	// The port-id of the N-1 flow
	int port_id_;

	 // The address of the neighbor IPC Process
	unsigned int neighbor_address_;

	// The port_id assigned by the neighbor IPC Process to the N-1 flow
	int neighbor_port_id_;

	// Flow up (true) or down (false)
	bool up_;

	// A sequence number to be able to discard old information
	int sequence_number_;

	// Age of this FSO (in seconds)
	int age_;

	// The object has been marked for propagation
	bool modified_;

	// Avoid port in the next propagation
	int avoid_port_;

	// The object is being erased
	bool being_erased_;

	// The name of the object in the RIB
	std::string object_name_;
};

class Edge {
public:
	Edge(unsigned int address1, int portV1, unsigned int address2, int portV2, int weight);
	bool isVertexIn(unsigned int address) const;
	unsigned int getOtherEndpoint(unsigned int address);
	std::list<unsigned int> getEndpoints();
	bool operator==(const Edge & other) const;
	bool operator!=(const Edge & other) const;
	const std::string toString() const;

	unsigned int address1_;
	unsigned int address2_;
	int weight_;
	int port_v1_;
	int port_v2_;
};

class Graph {
public:
	Graph(const std::list<FlowStateObject *>& flow_state_objects);
	~Graph();

	std::list<Edge *> edges_;
	std::list<unsigned int> vertices_;

private:
	struct CheckedVertex {
		unsigned int address_;
		int port_id_;
		std::list<unsigned int> connections_;

		CheckedVertex(unsigned int address) {
			address_ = address;
			port_id_ = 0;
		}

		bool connection_contains_address(unsigned int address) {
			std::list<unsigned int>::iterator it;
			for(it = connections_.begin(); it != connections_.end(); ++it) {
				if ((*it) == address) {
					return true;
				}
			}

			return false;
		}
	};

	std::list<FlowStateObject *> flow_state_objects_;
	std::list<CheckedVertex *> checked_vertices_;

	bool contains_vertex(unsigned int address) const;
	void init_vertices();
	CheckedVertex * get_checked_vertex(unsigned int address) const;
	void init_edges();
};

class IRoutingAlgorithm {
public:
	virtual ~IRoutingAlgorithm(){};

	//Compute the next hop to other addresses. Ownership of
	//PDUForwardingTableEntries in the list is passed to the
	//caller
	virtual std::list<rina::PDUForwardingTableEntry *> computePDUTForwardingTable(const std::list<FlowStateObject *>& fsoList,
			unsigned int source_address) = 0;
};

/// Contains the information of a predecessor, needed by the Dijkstra Algorithm
class PredecessorInfo {
public:
	PredecessorInfo(unsigned int nPredecessor, Edge * edge);
	PredecessorInfo(unsigned int nPredecessor);

	unsigned int predecessor_;
	unsigned int port_id_;
};

/// Implementation of the Dijkstra shortes path algorithgm
class DijkstraAlgorithm : public IRoutingAlgorithm {
public:
	DijkstraAlgorithm();
	std::list<rina::PDUForwardingTableEntry *> computePDUTForwardingTable(
			const std::list<FlowStateObject *>& fsoList, unsigned int source_address);
private:
	Graph * graph_;
	std::set<unsigned int> settled_nodes_;
	std::set<unsigned int> unsettled_nodes_;
	std::map<unsigned int, PredecessorInfo *> predecessors_;
	std::map<unsigned int, int> distances_;

	void execute(unsigned int source);
	unsigned int getMinimum() const;
	void findMinimalDistances (unsigned int node);
	int getShortestDistance(unsigned int destination) const;
	bool isNeighbor(Edge * edge, unsigned int node) const;
	bool isSettled(unsigned int node) const;
	PredecessorInfo * getNextNode(unsigned int address, unsigned int sourceAddress);
};

class LinkStatePDUFTGeneratorPolicy;

class FlowStateRIBObjectGroup: public BaseRIBObject {
public:
	FlowStateRIBObjectGroup(IPCProcess * ipc_process,
			LinkStatePDUFTGeneratorPolicy * pduft_generator_policy);
	const void* get_value() const;
	void remoteWriteObject(const rina::CDAPMessage * cdapMessage,
				rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	void createObject(const std::string& objectClass, const std::string& objectName,
			const void* objectValue);

private:
	LinkStatePDUFTGeneratorPolicy * pduft_generator_policy_;
};

class FlowStateDatabase {
public:
	static const int NO_AVOID_PORT;
	static const long WAIT_UNTIL_REMOVE_OBJECT;

	FlowStateDatabase(Encoder * encoder, FlowStateRIBObjectGroup *
			flow_state_rib_object_group, rina::Timer * timer);
	bool isEmpty() const;
	const rina::SerializedObject * encode();
	void setAvoidPort(int avoidPort);
	void addObjectToGroup(unsigned int address, int portId,
			unsigned int neighborAddress, int neighborPortId);
	bool deprecateObject(int portId, int maximum_age);
	std::vector< std::list<FlowStateObject*> > prepareForPropagation(const std::list<rina::FlowInformation>& flows);
	void incrementAge(int maximum_age);
	void updateObjects(const std::list<FlowStateObject*>& newObjects, int avoidPort, unsigned int address);
	std::list<FlowStateObject*> getModifiedFSOs();

	//Signals a modification in the FlowStateDB
	bool modified_;
	std::list<FlowStateObject *> flow_state_objects_;

private:
	Encoder * encoder_;
	FlowStateRIBObjectGroup * flow_state_rib_object_group_;
	rina::Timer * timer_;

	FlowStateObject * getByPortId(int portId);
};

class LinkStatePDUFTCDAPMessageHandler: public BaseCDAPResponseMessageHandler {
public:
	LinkStatePDUFTCDAPMessageHandler(LinkStatePDUFTGeneratorPolicy * pduft_generator_policy);
	void readResponse(const rina::CDAPMessage * cdapMessage,
				rina::CDAPSessionDescriptor * cdapSessionDescriptor);

private:
	LinkStatePDUFTGeneratorPolicy * pduft_generator_policy_;
};

class ComputePDUFTTimerTask : public rina::TimerTask {
public:
	ComputePDUFTTimerTask(LinkStatePDUFTGeneratorPolicy * pduft_generator_policy,
			long delay);
	~ComputePDUFTTimerTask() throw(){};
	void run();

private:
	LinkStatePDUFTGeneratorPolicy * pduft_generator_policy_;
	long delay_;
};

class KillFlowStateObjectTimerTask : public rina::TimerTask {
public:
	KillFlowStateObjectTimerTask(FlowStateRIBObjectGroup * fs_rib_group,
			FlowStateObject * fso, FlowStateDatabase * fs_db);
	~KillFlowStateObjectTimerTask() throw(){};
	void run();

private:
	FlowStateRIBObjectGroup * fs_rib_group_;
	FlowStateObject * fso_;
	FlowStateDatabase * fs_db_;
};

class PropagateFSODBTimerTask : public rina::TimerTask {
public:
	PropagateFSODBTimerTask(LinkStatePDUFTGeneratorPolicy * pduft_generator_policy,
			long delay);
	~PropagateFSODBTimerTask() throw(){};
	void run();

private:
	LinkStatePDUFTGeneratorPolicy * pduft_generator_policy_;
	long delay_;
};

class UpdateAgeTimerTask : public rina::TimerTask {
public:
	UpdateAgeTimerTask(LinkStatePDUFTGeneratorPolicy * pduft_generator_policy,
			long delay);
	~UpdateAgeTimerTask() throw(){};
	void run();

private:
	LinkStatePDUFTGeneratorPolicy * pduft_generator_policy_;
	long delay_;
};

class LinkStatePDUFTGeneratorPolicy: public IPDUFTGeneratorPolicy, public EventListener {
public:
	LinkStatePDUFTGeneratorPolicy();
	~LinkStatePDUFTGeneratorPolicy();
	void set_ipc_process(IPCProcess * ipc_process);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	void eventHappened(Event * event);
	bool propagateFSDB() const;
	void updateAge();
	void forwardingTableUpdate();
	void writeMessageReceived(const rina::CDAPMessage * cdapMessage, int portId);
	bool readMessageRecieved(const rina::CDAPMessage * cdapMessage, int srcPort) const;

	bool test_;
	FlowStateDatabase * db_;
	rina::Timer * timer_;

private:
	static const int MAXIMUM_BUFFER_SIZE;
	IPCProcess * ipc_process_;
	IRIBDaemon * rib_daemon_;
	Encoder * encoder_;
	rina::CDAPSessionManagerInterface * cdap_session_manager_;
	FlowStateRIBObjectGroup * fs_rib_group_;
	rina::PDUFTableGeneratorConfiguration pduft_generator_config_;
	IRoutingAlgorithm * routing_algorithm_;
	unsigned int source_vertex_;
	int maximum_age_;
	std::list<rina::FlowInformation> allocated_flows_;
	rina::Lockable * lock_;

	void populateRIB();
	void subscribeToEvents();
	void processFlowDeallocatedEvent(NMinusOneFlowDeallocatedEvent * event);
	void processFlowAllocatedEvent(NMinusOneFlowAllocatedEvent * event);
	void processNeighborAddedEvent(NeighborAddedEvent * event);
	void enrollmentToNeighbor(int portId);
};

}

#endif

#endif
