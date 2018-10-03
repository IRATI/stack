/*
 * Link-state routing policy
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

#ifndef IPCP_LINK_STATE_ROUTING_HH
#define IPCP_LINK_STATE_ROUTING_HH

#include <set>
#include <stdint.h>
#include <librina/internal-events.h>
#include <librina/timer.h>

#include "ipcp/components.h"

namespace rinad {

struct TreeNode {
    std::string name;
    int metric;
    std::set<TreeNode*> chldel;
    std::set<TreeNode*> chl;
    TreeNode(){
            name = std::string();
            metric = UINT16_MAX;
    }
    TreeNode(const std::string &_name, const int &_metric){
        name = _name;
        metric = _metric;
    }
    bool operator == (const TreeNode &b) const
    {
        return name == b.name;
    }
    bool operator < (const TreeNode &b) const
    {
        return name < b.name;
    }

    ~TreeNode(){
	std::set<TreeNode*>::iterator c;
        for(c=chldel.begin(); c != chldel.end(); ++c){
            delete *c;
        }
    }

};

class LinkStateRoutingPolicy;

class LinkStateRoutingPs: public IRoutingPs {
public:
	static std::string LINK_STATE_POLICY;
	LinkStateRoutingPs(IRoutingComponent * rc);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	int set_policy_set_param(const std::string& name,
			const std::string& value);
	~LinkStateRoutingPs();

private:
        // Data model of the routing component.
        IRoutingComponent * rc;
        LinkStateRoutingPolicy * lsr_policy;
};

class Edge {
public:
	Edge(const std::string& name1,
	     const std::string& name2,
	     int weight);
	bool isVertexIn(const std::string& name) const;
	std::string getOtherEndpoint(const std::string& name);
	std::list<std::string> getEndpoints();
	bool operator==(const Edge & other) const;
	bool operator!=(const Edge & other) const;
	const std::string toString() const;

	std::string name1_;
	std::string name2_;
	int weight_;
};

class FlowStateObject;
class Graph {
public:
	Graph(const std::list<FlowStateObject>& flow_state_objects);
	Graph();
	~Graph();

	std::list<Edge *> edges_;
	std::list<std::string> vertices_;

	void set_flow_state_objects(const std::list<FlowStateObject>& flow_state_objects);
	bool contains_vertex(const std::string& name) const;
	bool contains_edge(const std::string& name1,
			   const std::string& name2) const;

	void print() const;

private:
	struct CheckedVertex {
		std::string name_;
		int port_id_;
		std::list<std::string> connections;

		CheckedVertex(const std::string& name) {
			name_ = name;
			port_id_ = 0;
		}

		bool connection_contains_name(const std::string& name) {
			std::list<std::string>::iterator it;
			for(it = connections.begin(); it != connections.end(); ++it) {
				if ((*it) == name) {
					return true;
				}
			}

			return false;
		}
	};

	std::list<FlowStateObject> flow_state_objects_;
	std::list<CheckedVertex *> checked_vertices_;

	void init_vertices();
	CheckedVertex * get_checked_vertex(const std::string& name) const;
	void init_edges();
};

class IRoutingAlgorithm {
public:
	virtual ~IRoutingAlgorithm(){};

	//Compute the next hop for the node identified by source_address
	//towards all the other nodes
	virtual void computeRoutingTable(const Graph& graph,
	 	 	    	 	 const std::list<FlowStateObject>& fsoList,
					 const std::string& source_name,
					 std::list<rina::RoutingTableEntry *>& rt) = 0;

	//Compute the distance of the shortest path between the node identified
	//by source_address and all the other nodes
	virtual void computeShortestDistances(const Graph& graph,
					      const std::string& source_name,
				              std::map<std::string, int>& distances) = 0;
};

/// Contains the information of a predecessor, needed by the Dijkstra Algorithm
class PredecessorInfo {
public:
	PredecessorInfo(const std::string& nPredecessor);

	std::string predecessor_;
};

/// The routing algorithm used to compute the PDU forwarding table is a Shortest
/// Path First (SPF) algorithm. Instances of the algorithm are run independently
/// and concurrently by all IPC processes in their forwarding table generator
/// component, upon detection of an N-1 flow allocation/deallocation/state change.
class DijkstraAlgorithm : public IRoutingAlgorithm {
public:
	DijkstraAlgorithm();
	void computeRoutingTable(const Graph& graph,
	 	 	    	 const std::list<FlowStateObject>& fsoList,
				 const std::string& source_name,
				 std::list<rina::RoutingTableEntry *>& rt);
	void computeShortestDistances(const Graph& graph,
				      const std::string& source_name,
				      std::map<std::string, int>& distances);
private:
	std::set<std::string> settled_nodes_;
	std::set<std::string> unsettled_nodes_;
	std::map<std::string, PredecessorInfo *> predecessors_;
	std::map<std::string, int> distances_;

	void execute(const Graph& graph, const std::string& source);
	std::string getMinimum() const;
	void findMinimalDistances (const Graph& graph, const std::string& node);
	int getShortestDistance(const std::string& destination) const;
	bool isNeighbor(Edge * edge, const std::string& node) const;
	bool isSettled(const std::string& node) const;
	std::string getNextHop(const std::string& name, const std::string& sourceName);
	void clear();
};

/// The routing algorithm used to compute the PDU forwarding table is a Shortest
/// Path First (SPF) algorithm using ECMP approach. Instances of the algorithm 
/// are run independently and concurrently by all IPC processes in their forwarding 
/// table generator component, upon detection of an N-1 flow allocation/deallocation/state change.
class ECMPDijkstraAlgorithm : public IRoutingAlgorithm {
public:
	ECMPDijkstraAlgorithm();
	void computeRoutingTable(const Graph& graph,
	 	 	    	 const std::list<FlowStateObject>& fsoList,
				 const std::string& source_name,
				 std::list<rina::RoutingTableEntry *>& rt);
	void computeShortestDistances(const Graph& graph,
				      const std::string& source_name,
				      std::map<std::string, int>& distances);

private:
	std::set<std::string> settled_nodes_;
	std::set<std::string> unsettled_nodes_;
	std::set<std::string> minimum_nodes_;
	std::map<std::string, std::list<TreeNode *> > predecessors_;
	std::map<std::string, int> distances_;
	TreeNode* t;
	void execute(const Graph& graph,
		     const std::string& source);
	void addRecursive(std::list<rina::RoutingTableEntry *> &table,
			  int qos,
			  const std::string& next,
			  TreeNode * node);
	std::list<rina::RoutingTableEntry *>::iterator findEntry(std::list<rina::RoutingTableEntry *> &table,
							    	 const std::string& name);
	void getMinimum();
	void findMinimalDistances (const Graph& graph, TreeNode * pred);
	int getShortestDistance(const std::string& destination) const;
	bool isNeighbor(Edge * edge, const std::string& node) const;
	bool isSettled(const std::string& node) const;
	void clear();
};

class IResiliencyAlgorithm {
public:
	IResiliencyAlgorithm(IRoutingAlgorithm& ra);
	virtual ~IResiliencyAlgorithm(){};

	// Starting from the routing table computed by the routing algorithm,
	// try to add (for each target nod) different next hops in addition to the
	// existing ones, in order to improve resilency of the source node
	virtual void fortifyRoutingTable(const Graph& graph,
					 const std::string& source_name,
					 std::list<rina::RoutingTableEntry *>& rt) = 0;

protected:
	IRoutingAlgorithm& routing_algorithm;
};

class LoopFreeAlternateAlgorithm : public IResiliencyAlgorithm {
public:
	LoopFreeAlternateAlgorithm(IRoutingAlgorithm& ra);
	void fortifyRoutingTable(const Graph& graph,
				 const std::string& source_name,
				 std::list<rina::RoutingTableEntry *>& rt);
private:
	void extendRoutingTableEntry(std::list<rina::RoutingTableEntry *>& rt,
				     const std::string& target_name,
				     const std::string& nexthop);
};

/// The object exchanged between IPC Processes to disseminate the state of
/// one N-1 flow supporting the IPC Processes in the DIF. This is the RIB
/// target object when the PDU Forwarding Table Generator wants to send
/// information about a single N-1 flow.The flow state object id (fso_id)
/// is generated by concatenating the address and port-id fields.
class FlowStateObject {
public:
	FlowStateObject();
	FlowStateObject(const std::string& name,
			const std::string& neighbor_name,
			unsigned int cost,
			bool state,
			int sequence_number,
			unsigned int age);
	~FlowStateObject();
	const std::string toString() const;
	void deprecateObject(unsigned int max_age);
	//accessors
	void add_address(unsigned int address);
	void remove_address(unsigned int address);
	bool contains_address(unsigned int address) const;
	void add_neighboraddress(unsigned int neighbor_address);
	void remove_neighboraddress(unsigned int neighbor_address);
	bool contains_neighboraddress(unsigned int address) const;
	void set_neighboraddresses(const std::list<unsigned int>& addresses);
	void set_addresses(const std::list<unsigned int>& addresses_);
	const std::string getKey() const;

	// The name of the IPCP (encoded in a string)
	std::string name;

	// The name of the neighbor
	std::string neighbor_name;

	// The object is being erased
	bool being_erased;

	// Flow up (true) or down (false)
	bool state_up;

	// Age of this FSO (in seconds)
	unsigned int age;

	// The port_id assigned by the neighbor IPC Process to the N-1 flow
	unsigned int cost;

	// The object has been marked for propagation
	bool modified;

	// A sequence number to be able to discard old information
	unsigned int seq_num;

	// Avoid port in the next propagation
	int avoid_port;

	// The name of the object in the RIB
	std::string object_name;

	// The addresses of the IPC Process
	std::list<unsigned int> addresses;

	// The address of the neighbor IPC Process
	std::list<unsigned int> neighbor_addresses;
};

class FlowStateManager;
/// A single flow state object
class FlowStateRIBObject: public rina::rib::RIBObj {
public:
	FlowStateRIBObject(FlowStateObject* new_obj);
	void read(const rina::cdap_rib::con_handle_t &con, const std::string& fqn,
		const std::string& clas, const rina::cdap_rib::filt_info_t &filt,
		const int invoke_id, rina::ser_obj_t &obj_reply, 
		rina::cdap_rib::res_info_t& res);
	const std::string get_displayable_value() const;

	FlowStateObject* obj;

	const static std::string clazz_name;
	const static std::string object_name_prefix;
};

class FlowStateObjects;
class KillFlowStateObjectTimerTask : public rina::TimerTask {
public:
	KillFlowStateObjectTimerTask(LinkStateRoutingPolicy *ps, std::string fqn);
	~KillFlowStateObjectTimerTask() throw(){};
	void run();
	std::string name() const {
		return "kill-flow-state-object";
	}

private:
	std::string fqn_;
	LinkStateRoutingPolicy* ps_;
};

class FlowStateRIBObjects;
class FlowStateObjects
{
public:
	FlowStateObjects(LinkStateRoutingPolicy * ps);
	~FlowStateObjects();
	void addAddressToFSOs(const std::string& name,
			      unsigned int address,
			      bool neighbor);
	void removeAddressFromFSOs(const std::string& name,
				   unsigned int address,
				   bool neighbor);
	bool addObject(const FlowStateObject& object);
	void deprecateObject(const std::string& fqn, 
			     unsigned int max_age);
	void deprecateObjects(const std::string& neigh_name,
			      const std::string& name,
			      unsigned int max_age);
	void updateCost(const std::string& neigh_name,
			      const std::string& name,
			      unsigned int cost);
	void deprecateObjectsWithName(const std::string& name,
				      unsigned int max_age,
				      bool neighbor);
	FlowStateObject * getObject(const std::string& fqn);
	void getModifiedFSOs(std::list<FlowStateObject *>& result);
	void getAllFSOs(std::list<FlowStateObject>& result);
	void incrementAge(unsigned int max_age,
			  rina::Timer* timer);
	void updateObject(const std::string& fqn, 
			  unsigned int avoid_port);
	void encodeAllFSOs(rina::ser_obj_t& obj);
	void getAllFSOsForPropagation(std::list< std::list<FlowStateObject> >& fsos,
				      unsigned int max_objects);
	bool is_modified() const;
	void has_modified(bool modified);
	void set_wait_until_remove_object(unsigned int wait_object);
	void removeObject(const std::string& fqn);

private:
	void addCheckedObject(const FlowStateObject& object);
	std::map<std::string,FlowStateObject*> objects;
	//Signals a modification in the FlowStateDB
	bool modified_;
	LinkStateRoutingPolicy * ps_;
	unsigned int wait_until_remove_object;
	rina::Lockable lock;
};

/// A container of flow state objects. This is the RIB target object
/// when routing wants to send
/// information about more than one N-1 flow
// TODO: destructor
class FlowStateRIBObjects: public rina::rib::RIBObj {
public:
	FlowStateRIBObjects(FlowStateObjects* new_objs,
			    LinkStateRoutingPolicy * ps);
	const std::string toString();
	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& clas,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::ser_obj_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);
	void write(const rina::cdap_rib::con_handle_t &con,
		   const std::string& fqn,
		   const std::string& clas,
		   const rina::cdap_rib::filt_info_t &filt,
		   const int invoke_id,
		   const rina::ser_obj_t &obj_req,
		   rina::ser_obj_t &obj_reply,
		   rina::cdap_rib::res_info_t& res);

	const static std::string clazz_name;
	const static std::string object_name;
private:
	FlowStateObjects *objs;
	LinkStateRoutingPolicy * ps_;
};

/// The subset of the RIB that contains all the Flow State objects known by the IPC Process.
/// It exists only in the PDU forwarding table generator. It is used as an input to calculate
/// the routing and forwarding tables. The FSDB is generated by the operations on FSOs received
/// through CDAP messages or created by the resource allocator. The FSOs in the FSDB contain
/// the same information as the one formerly described in the FSO subsection, plus a list of
/// port-ids of N-1 management flows the FSOs should be sent to. Periodically the FSDB is checked
/// to look for FSOs to be propagated. Once FSOs have been written to the corresponding N-1 flows,
/// the list of port-ids is emptied. The list is updated when events that required the propagation
/// of FSO occur (such as local events notifying changes on N-1 flows or remote operations on the
/// FSDB through CDAP).
class FlowStateManager {
public:
	static const int NO_AVOID_PORT;
	static const long WAIT_UNTIL_REMOVE_OBJECT;
	FlowStateManager(rina::Timer* new_timer,
			unsigned int max_age,
			LinkStateRoutingPolicy * ps);
	~FlowStateManager();
	//void setAvoidPort(int avoidPort);
	/// add a FlowStateObject
	void addAddressToFSOs(const std::string& name,
			      unsigned int address,
			      bool neighbor);
	bool addNewFSO(const std::string& name,
		       std::list<unsigned int>& addresses,
		       const std::string& neighbor_name,
		       std::list<unsigned int>& neighbor_addresses,
		       unsigned int cost,
		       int avoid_port);
	/// Set a FSO ready for removal
	void deprecateObject(std::string fqn);
	void removeAddressFromFSOs(const std::string& name,
				   unsigned int address,
				   bool neighbor);
	void deprecateObjectsNeighbor(const std::string& neigh_name,
	                              const std::string& name,
				      bool both);
	void updateCost(const std::string& neigh_name,
			const std::string& name,
			unsigned int cost);
	std::map <int, std::list<FlowStateObject*> > prepareForPropagation
	        (const std::list<rina::FlowInformation>& flows);
	void incrementAge();
	void updateObjects(const std::list<FlowStateObject>& newObjects,
			   unsigned int avoidPort);
	void prepareForPropagation(std::map<int, std::list< std::list<FlowStateObject> > >& to_propagate,
				   unsigned int max_objects) const;
	void encodeAllFSOs(rina::ser_obj_t& obj) const;
	void getAllFSOs(std::list<FlowStateObject>& list) const;
	bool tableUpdate() const;
	void removeObject(const std::string& fqn);
	void getAllFSOsForPropagation(std::list< std::list<FlowStateObject> >& fsos,
				      unsigned int max_objects);

	//Force a routing table update;
	void force_table_update();

	// accessors
	void set_maximum_age(unsigned int max_age);
	void set_wait_until_remove_object(unsigned int wait_object);
private:
	FlowStateObjects* fsos;
	unsigned int maximum_age;
	rina::Timer* timer;
};

class ComputeRoutingTimerTask : public rina::TimerTask {
public:
	ComputeRoutingTimerTask(LinkStateRoutingPolicy * lsr_policy,
			long delay);
	~ComputeRoutingTimerTask() throw(){};
	void run();
	std::string name() const {
		return "compute-routing";
	}

private:
	LinkStateRoutingPolicy* lsr_policy_;
	long delay_;
};

class PropagateFSODBTimerTask : public rina::TimerTask {
public:
	PropagateFSODBTimerTask(LinkStateRoutingPolicy * lsr_policy,
			long delay);
	~PropagateFSODBTimerTask() throw(){};
	void run();
	std::string name() const {
		return "propagate-fso-db";
	}

private:
	LinkStateRoutingPolicy * lsr_policy_;
	long delay_;
};

class UpdateAgeTimerTask : public rina::TimerTask {
public:
	UpdateAgeTimerTask(LinkStateRoutingPolicy * lsr_policy,
			long delay);
	~UpdateAgeTimerTask() throw(){};
	void run();
	std::string name() const {
		return "update-age";
	}

private:
	LinkStateRoutingPolicy * lsr_policy_;
	long delay_;
};

class ExpireOldAddressTimerTask : public rina::TimerTask {
public:
	ExpireOldAddressTimerTask(LinkStateRoutingPolicy * lsr_policy,
				  const std::string& name,
				  unsigned int address,
				  bool neighbor);
	~ExpireOldAddressTimerTask() throw(){};
	void run();
	std::string name() const {
		return "expire-old-address";
	}

private:
	LinkStateRoutingPolicy * lsr_policy_;
	unsigned int address;
	std::string name_;
	bool neighbor;
};

/// This routing policy uses a Flow State Database
/// (FSDB) populated with Flow State Objects (FSOs) to compute the PDU
/// forwarding table. This database is updated based on local events notified
/// by the Resource Allocator, or on remote operations on the Flow State Objects
/// notified by the RIB Daemon. Each IPC Process maintains a view of the
/// data-transfer connectivity graph in the DIF. To do so, information about
/// the state of N-1 flows is disseminated through the DIF periodically. The PDU
/// Forwarding Table generator applies an algorithm to compute the shortest
/// route from this IPC Process to all the other IPC Processes in the DIF. If
/// there is a Flow State Object from both IPC processes that share the flow,
/// the flow is used in the computation. This is to avoid misbehaving IPC
/// processes that disturb the routing in the DIF. The flow has to be announced
/// by both ends. The next hop of the routes to all the other IPC Processes is
/// stored as the routing table. Finally, for each entry in the routing table,
/// the PDU Forwarding Table generator selects the most appropriate N-1 flow that
/// leads to the next hop. This selection of the most appropriate N-1 flow can be
/// performed more frequently in order to perform load-balancing or to quickly route
/// around failed N-1 flows
class LinkStateRoutingPolicy: public rina::InternalEventListener {
public:
	static const std::string OBJECT_MAXIMUM_AGE;
	static const std::string WAIT_UNTIL_READ_CDAP;
	static const std::string WAIT_UNTIL_ERROR;
	static const std::string WAIT_UNTIL_PDUFT_COMPUTATION;
	static const std::string WAIT_UNTIL_FSODB_PROPAGATION;
	static const std::string WAIT_UNTIL_AGE_INCREMENT;
	static const std::string WAIT_UNTIL_REMOVE_OBJECT;
	static const std::string WAIT_UNTIL_DEPRECATE_OLD_ADDRESS;
	static const std::string ROUTING_ALGORITHM;
	static const std::string MAXIMUM_OBJECTS_PER_ROUTING_UPDATE;

        static const int PULSES_UNTIL_FSO_EXPIRATION_DEFAULT = 100000;
        static const int WAIT_UNTIL_READ_CDAP_DEFAULT = 5001;
        static const int WAIT_UNTIL_ERROR_DEFAULT = 5001;
        static const int WAIT_UNTIL_PDUFT_COMPUTATION_DEFAULT = 103;
        static const int WAIT_UNTIL_FSODB_PROPAGATION_DEFAULT = 101;
        static const int WAIT_UNTIL_AGE_INCREMENT_DEFAULT = 997;
        static const long WAIT_UNTIL_REMOVE_OBJECT_DEFAULT = 2300;
        static const long WAIT_UNTIL_DEPRECATE_OLD_ADDRESS_DEFAULT = 10000;
        static const unsigned int MAX_OBJECTS_PER_ROUTING_UPDATE_DEFAULT = 15;
        static const std::string DIJKSTRA_ALG;
        static const std::string ECMP_DIJKSTRA_ALG;

	LinkStateRoutingPolicy(IPCProcess * ipcp);
	~LinkStateRoutingPolicy();
	void set_ipc_process(IPCProcess * ipc_process);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);

	/// N-1 Flow allocated, N-1 Flow deallocated or enrollment to neighbor completed
	void eventHappened(rina::InternalEvent * event);

	/// Invoked periodically by a timer. Every FSO that needs to be propagated is retrieved,
	/// together with the list of port-ids it should be sent on. For every port-id, an
	/// M_WRITE CDAP message targeting the /dif/management/routing/flowstateobjectgroup/
	/// object is created, containing all the FSOs that need to be propagated on this N-1
	/// management flow. In the case that a single CDAP message would be too large (i.e.,
	/// the encoded message is larger than the Maximum SDU size of the N-1 management flow),
	/// the set of FSOs to be propagated can be spread into multiple CDAP messages. All the
	/// CDAP messages are written to the corresponding port-ids, and the list of port-ids of
	/// all FSOs is cleared.
	void propagateFSDB();

	/// Invoked periodically by a timer. The age of every Flow State Object is incremented
	/// with one. If an FSO reaches the maximum age - which is set by DIF policy -, it is
	/// removed from the FSDB. The FSDB is marked as ÒmodifiedÓ if it wasnÕt already and one
	/// or more FSOs are removed from the FSDB during the processing of this event.
	void updateAge();

	/// Invoked periodically by a timer. If the FSDB is marked as ÒmodifiedÓ, the PDU
	/// Forwarding Table Generator marks the FSDB as Ònot modifiedÓ and the PDU forwarding
	/// table computation procedure is initiated . An N-1 flow is only used in the calculation
	/// of the forwarding table if there are two FSOs in the FSDB related to the flow (one
	/// advertised by each IPC Process the N-1 flow connects together), and the state of the
	/// flow is ÒupÓ. The forwarding table is computed according to the algorithm explained in
	/// the Òalgorithm to compute the forwarding tableÓ section. If the FSDB is not marked as
	/// ÒmodifiedÓ nothing happens.
	void routingTableUpdate();

	/// Deprecate all LSOs containing the address passed to the method
	void expireOldAddress(const std::string& name,
			      unsigned int address,
			      bool neighbor);

	void updateObjects(const std::list<FlowStateObject>& newObjects,
			   unsigned int avoidPort);

	void removeFlowStateObject(const std::string& fqn);

	rina::Timer *timer_;
private:
	static const int MAXIMUM_BUFFER_SIZE;
	IPCProcess * ipc_process_;
	IPCPRIBDaemon * rib_daemon_;
	IRoutingAlgorithm * routing_algorithm_;
	IResiliencyAlgorithm * resiliency_algorithm_;
	unsigned int wait_until_deprecate_address_;
	unsigned int maximum_age_;
	unsigned int max_objects_per_rupdate_;
	bool test_;
	FlowStateManager *db_;
	rina::Lockable lock_;

	void subscribeToEvents();

	/// The Resource Allocator has deallocated an existing N-1 flow dedicated to data
	/// transfer. If there is a pending flow allocation over this N-1 flow, it has to
	/// be erased from the list of pending flow allocations. Otherwise, the Flow State
	/// Object corresponding to this flow is retrieved. The state is set false, the age
	/// is set to the maximum age and the sequence number pertaining to this FSO is
	/// incremented by 1. The FSO is marked for propagation (ÔpropagateÕ is set to true)
	/// and the list of port-ids associated with this FSO is filled with the port-ids of
	/// all N-1 management flows. The FSDB is marked as ÒmodifiedÓ if it wasnÕt already.
	void processFlowDeallocatedEvent(rina::NMinusOneFlowDeallocatedEvent * event);

	/// The Resource Allocator has allocated a new N-1 flow dedicated to data transfer.
	/// If no neighbour is found, an entry has to be created and added into the list
	/// of pending flow allocations. Otherwise, a Flow State Object is created, containing
	/// the address and port-id of the IPC process and the address and port-id of the
	/// neighbour IPC process a flow is allocated to. The state is set, the age is set to
	/// 0, and the sequence number is set to 1. The FSO is added to the FSDB, marked for
	/// propagation, and associated with a new list of port-ids, which is filled with the
	/// port-ids of all N-1 management flows. The FSDB is marked as ÒmodifiedÓ if it wasnÕt
	/// already.
	void processFlowAllocatedEvent(rina::NMinusOneFlowAllocatedEvent * event);

	/// The Enrollment Task has completed the enrollment procedure with a new neighbor IPC
	/// Process. If there are pending flow allocations over the enrolled neighbour, they have
	/// to be launched following the procedure descrived in the previous subsection called
	/// Local data transfer N-1 flow allocated. Then, the full FSDB is sent to the IPC process
	/// that a flow has been allocated to, in one or more CDAP M_WRITE messages targeting the
	/// /dif/management/routing/flowstateobjectgroup/ object.
	void processNeighborAddedEvent(rina::NeighborAddedEvent * event);

	void processNeighborLostEvent(rina::ConnectiviyToNeighborLostEvent * event);

	void processAddressChangeEvent(rina::AddressChangeEvent * event);

	void processNeighborAddressChangeEvent(rina::NeighborAddressChangeEvent * event);

	void printNhopTable(std::list<rina::RoutingTableEntry *>& rt);

	void populateAddresses(std::list<rina::RoutingTableEntry *>& rt,
			       const std::list<FlowStateObject>& fsos);

	void _routingTableUpdate();
};

/// Encoder of Flow State object
class FlowStateObjectEncoder: public rina::Encoder<FlowStateObject>{
public:
	void encode(const FlowStateObject &obj, rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, FlowStateObject &des_obj);
};

class FlowStateObjectListEncoder: 
	public rina::Encoder<std::list<FlowStateObject> > {
public:
	void encode(const std::list<FlowStateObject> &obj,
		rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		std::list<FlowStateObject> &des_obj);
};

}

#endif
