

//
// Link-state policy set for Routing
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//    Vincenzo Maffione <v.maffione@nextworks.it>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <assert.h>
#include <climits>
#include <set>
#include <sstream>
#include <string>

#define IPCP_MODULE "routing-ps-link-state"
#include "../../ipcp-logging.h"

#include <librina/timer.h>

#include "ipcp/components.h"
#include "routing-ps.h"
#include "common/encoders/FlowStateMessage.pb.h"
#include "common/encoders/FlowStateGroupMessage.pb.h"
#include "ipcp/ipc-process.h"

namespace rinad {

std::string LinkStateRoutingPs::LINK_STATE_POLICY = "LinkState";

LinkStateRoutingPs::LinkStateRoutingPs(IRoutingComponent * rc_) : rc(rc_)
{
	lsr_policy = 0;
}

void LinkStateRoutingPs::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	lsr_policy = new LinkStateRoutingPolicy(rc->ipcp);
	lsr_policy->set_dif_configuration(dif_configuration);
}

int LinkStateRoutingPs::set_policy_set_param(const std::string& name,
                                            const std::string& value)
{
        LOG_IPCP_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

LinkStateRoutingPs::~LinkStateRoutingPs()
{
	delete lsr_policy;
}

extern "C" rina::IPolicySet *
createRoutingComponentPs(rina::ApplicationEntity * ctx)
{
	IRoutingComponent * rc = dynamic_cast<IRoutingComponent *>(ctx);

	if (!rc) {
		return NULL;
	}

	return new LinkStateRoutingPs(rc);
}

extern "C" void
destroyRoutingComponentPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

Edge::Edge(const std::string& name1,
	   const std::string& name2,
	   int weight)
{
	name1_ = name1;
	name2_ = name2;
	weight_ = weight;
}

bool Edge::isVertexIn(const std::string& name) const
{
	if (name == name1_) {
		return true;
	}

	if (name == name2_) {
		return true;
	}

	return false;
}

std::string Edge::getOtherEndpoint(const std::string& name)
{
	if (name == name1_) {
		return name2_;
	}

	if (name == name2_) {
		return name1_;
	}

	return 0;
}

std::list<std::string> Edge::getEndpoints()
{
	std::list<std::string> result;
	result.push_back(name1_);
	result.push_back(name2_);

	return result;
}

bool Edge::operator==(const Edge & other) const
{
	if (!isVertexIn(other.name1_)) {
		return false;
	}

	if (!isVertexIn(other.name2_)) {
		return false;
	}

	if (weight_ != other.weight_) {
		return false;
	}

	return true;
}

bool Edge::operator!=(const Edge & other) const
{
	return !(*this == other);
}

const std::string Edge::toString() const
{
	std::stringstream ss;

	ss << name1_ << " " << name2_;
	return ss.str();
}

Graph::Graph(const std::list<FlowStateObject>& flow_state_objects)
{
	set_flow_state_objects(flow_state_objects);
}

Graph::Graph()
{
}

void Graph::set_flow_state_objects(const std::list<FlowStateObject>& flow_state_objects)
{
	flow_state_objects_ = flow_state_objects;
	init_vertices();
	init_edges();
}

Graph::~Graph()
{
	std::list<CheckedVertex *>::iterator it;
	for (it = checked_vertices_.begin(); it != checked_vertices_.end(); ++it) {
		delete (*it);
	}

	std::list<Edge *>::iterator edgeIt;
	for (edgeIt = edges_.begin(); edgeIt != edges_.end(); ++edgeIt) {
		delete (*edgeIt);
	}
}

void Graph::init_vertices()
{
	std::list<FlowStateObject>::const_iterator it;
	for (it = flow_state_objects_.begin(); it != flow_state_objects_.end();
			++it) {
		if (!contains_vertex(it->name)) {
			vertices_.push_back(it->name);
		}

		if (!contains_vertex(it->neighbor_name)) {
			vertices_.push_back(it->neighbor_name);
		}
	}
}

bool Graph::contains_vertex(const std::string& name) const
{
	std::list<std::string>::const_iterator it;
	for (it = vertices_.begin(); it != vertices_.end(); ++it) {
		if ((*it) == name) {
			return true;
		}
	}

	return false;
}

bool Graph::contains_edge(const std::string& name1,
			  const std::string& name2) const
{
	for(std::list<Edge *>::const_iterator eit = edges_.begin();
					eit != edges_.end(); ++eit) {
		if (*(*eit) == Edge(name1, name2, (*eit)->weight_)) {
			return true;
		}
	}

	return false;
}

void Graph::init_edges()
{
	std::list<std::string>::const_iterator it;
	std::list<FlowStateObject>::const_iterator flowIt;

	for (it = vertices_.begin(); it != vertices_.end(); ++it) {
		checked_vertices_.push_back(new CheckedVertex((*it)));
	}

	CheckedVertex * origin = 0;
	CheckedVertex * dest = 0;
	for (flowIt = flow_state_objects_.begin();
			flowIt != flow_state_objects_.end(); ++flowIt) {
		if (!flowIt->state_up) {
			continue;
		}

		LOG_IPCP_DBG("Processing flow state object: %s",
				flowIt->object_name.c_str());

		origin = get_checked_vertex(flowIt->name);
		if (origin == 0) {
			LOG_IPCP_WARN("Could not find checked vertex for address %s",
					flowIt->name.c_str());
			continue;
		}

		dest = get_checked_vertex(flowIt->neighbor_name);
		if (dest == 0) {
			LOG_IPCP_WARN("Could not find checked vertex for address %s",
					flowIt->neighbor_name.c_str());
			continue;
		}

		if (origin->connection_contains_name(dest->name_)
				&& dest->connection_contains_name(origin->name_)) {
			edges_.push_back(new Edge(origin->name_,
						  dest->name_,
						  flowIt->cost));
			origin->connections.remove(dest->name_);
			dest->connections.remove(origin->name_);
		} else {
			origin->connections.push_back(dest->name_);
			dest->connections.push_back(origin->name_);
		}
	}
}

Graph::CheckedVertex * Graph::get_checked_vertex(const std::string& name) const
{
	std::list<Graph::CheckedVertex *>::const_iterator it;
	for (it = checked_vertices_.begin(); it != checked_vertices_.end(); ++it) {
		if ((*it)->name_ == name) {
			return (*it);
		}
	}

	return 0;
}

void Graph::print() const
{
	LOG_IPCP_DBG("Graph edges:");

	for (std::list<Edge *>::const_iterator it = edges_.begin();
					it != edges_.end(); it++) {
		const Edge& e = **it;

		LOG_IPCP_DBG("    (%s --> %s, %d)", e.name1_.c_str(),
			      e.name2_.c_str(), e.weight_);
	}
}

PredecessorInfo::PredecessorInfo(const std::string& nPredecessor)
{
	predecessor_ = nPredecessor;
}

DijkstraAlgorithm::DijkstraAlgorithm()
{
}

void DijkstraAlgorithm::clear()
{
	unsettled_nodes_.clear();
	settled_nodes_.clear();
	predecessors_.clear();
	distances_.clear();
}

void DijkstraAlgorithm::computeShortestDistances(const Graph& graph,
						 const std::string& source_name,
						 std::map<std::string, int>& distances)
{
	execute(graph, source_name);

	// Write back the result
	distances = distances_;

	clear();
}

void DijkstraAlgorithm::computeRoutingTable(const Graph& graph,
 	 	    	    	    	    const std::list<FlowStateObject>& fsoList,
					    const std::string& source_name,
					    std::list<rina::RoutingTableEntry *>& rt)
{
	std::list<std::string>::const_iterator it;
	rina::RoutingTableEntry * entry;
	rina::IPCPNameAddresses ipcpna;

	execute(graph, source_name);

	for (it = graph.vertices_.begin(); it != graph.vertices_.end(); ++it) {
		if ((*it) != source_name) {
			ipcpna.name = getNextHop((*it), source_name);
			if (ipcpna.name != std::string()) {
				entry = new rina::RoutingTableEntry();
				entry->destination.name = (*it);
				entry->nextHopNames.push_back(rina::NHopAltList(ipcpna));
				entry->qosId = 0;
				entry->cost = 1;
				rt.push_back(entry);
				LOG_IPCP_DBG("Added entry to routing table: destination %s, next-hop %s",
						entry->destination.name.c_str(), ipcpna.name.c_str());
			}
		}
	}

	clear();
}

void DijkstraAlgorithm::execute(const Graph& graph, const std::string& source)
{
	distances_[source] = 0;
	unsettled_nodes_.insert(source);

	std::string node;
	while (unsettled_nodes_.size() > 0) {
		node = getMinimum();
		settled_nodes_.insert(node);
		unsettled_nodes_.erase(node);
		findMinimalDistances(graph, node);
	}
}

std::string DijkstraAlgorithm::getMinimum() const
{
	std::string minimum = std::string();
	std::set<std::string>::iterator it;

	for (it = unsettled_nodes_.begin(); it != unsettled_nodes_.end(); ++it) {
		if (minimum == std::string()) {
			minimum = (*it);
		} else {
			if (getShortestDistance((*it)) < getShortestDistance(minimum)) {
				minimum = (*it);
			}
		}
	}

	return minimum;
}

int DijkstraAlgorithm::getShortestDistance(const std::string& destination) const
{
	std::map<std::string, int>::const_iterator it;
	int distance = INT_MAX;

	it = distances_.find(destination);
	if (it != distances_.end()) {
		distance = it->second;
	}

	return distance;
}

void DijkstraAlgorithm::findMinimalDistances(const Graph& graph,
					     const std::string& node)
{
	std::list<std::string> adjacentNodes;
	std::list<Edge *>::const_iterator edgeIt;

	std::string target = std::string();
	int shortestDistance;
	for (edgeIt = graph.edges_.begin(); edgeIt != graph.edges_.end();
			++edgeIt) {
		if (isNeighbor((*edgeIt), node)) {
			target = (*edgeIt)->getOtherEndpoint(node);
			shortestDistance = getShortestDistance(node) + (*edgeIt)->weight_;
			if (getShortestDistance(target) > shortestDistance) {
				distances_[target] = shortestDistance;
				predecessors_[target] = new PredecessorInfo(node);
				unsettled_nodes_.insert(target);
			}
		}
	}
}

bool DijkstraAlgorithm::isNeighbor(Edge * edge, const std::string& node) const
{
	if (edge->isVertexIn(node)) {
		if (!isSettled(edge->getOtherEndpoint(node))) {
			return true;
		}
	}

	return false;
}

bool DijkstraAlgorithm::isSettled(const std::string& node) const
{
	std::set<std::string>::iterator it;

	for (it = settled_nodes_.begin(); it != settled_nodes_.end(); ++it) {
		if ((*it) == node) {
			return true;
		}
	}

	return false;
}

std::string DijkstraAlgorithm::getNextHop(const std::string& target,
					  const std::string& source)
{
	std::map<std::string, PredecessorInfo *>::iterator it;
	PredecessorInfo * step;
	std::string nextHop = target;

	it = predecessors_.find(target);
	if (it == predecessors_.end()) {
		return std::string();
	} else {
		step = it->second;
	}

	it = predecessors_.find(step->predecessor_);
	while (it != predecessors_.end()) {
		nextHop = step->predecessor_;
		step = it->second;
		if (step->predecessor_ == source) {
			break;
		}

		it = predecessors_.find(step->predecessor_);
	}

	if (step->predecessor_ == target) {
		return std::string();
	}

	return nextHop;
}

// ECMP Dijkstra algorithm
ECMPDijkstraAlgorithm::ECMPDijkstraAlgorithm()
{
	t = NULL;
}

void ECMPDijkstraAlgorithm::clear()
{
	unsettled_nodes_.clear();
	settled_nodes_.clear();
	predecessors_.clear();
	distances_.clear();
	delete t;
}

void ECMPDijkstraAlgorithm::computeShortestDistances(const Graph& graph,
			      	      	      	     const std::string& source_name,
						     std::map<std::string, int>& distances)
{
	execute(graph, source_name);

	// Write back the result
	distances = distances_;

	clear();
}

void ECMPDijkstraAlgorithm::computeRoutingTable(const Graph& graph,
 	 	    	    	       	        const std::list<FlowStateObject>& fsoList,
						const std::string& source_name,
						std::list<rina::RoutingTableEntry *>& rt)
{
	std::list<std::string>::const_iterator it;
	std::list<std::string>::const_iterator nextHopsIt;
	std::list<std::string> nextHops;
	rina::RoutingTableEntry * entry;
	rina::IPCPNameAddresses ipcpna;

	(void)fsoList; // avoid compiler barfs

	execute(graph, source_name);

	for(std::set<TreeNode *>::iterator it = t->chl.begin(); it != t->chl.end(); it++){
		std::list<rina::RoutingTableEntry *>::iterator pos = findEntry(rt,
									       (*it)->name);

		if(pos != rt.end()){
			ipcpna.name = (*it)->name;
			(*pos)->nextHopNames.push_back(ipcpna);
		}
		else{
			entry = new rina::RoutingTableEntry();
		        entry->destination.name = (*it)->name;
		        entry->qosId = 1;
		        entry->cost = (*it)->metric;
		        ipcpna.name = (*it)->name;
			entry->nextHopNames.push_back(ipcpna);
			LOG_IPCP_DBG("Added entry to routing table: destination %s, next-hop %s",
                        entry->destination.name.c_str(), (*it)->name.c_str());
			rt.push_back(entry);
		}
		addRecursive(rt, 1, (*it)->name, *it);
	}
	clear();
}

void ECMPDijkstraAlgorithm::addRecursive(std::list<rina::RoutingTableEntry *> &table,
					 int qos,
					 const std::string& next,
					 TreeNode * node)
{
	rina::IPCPNameAddresses ipcpna;

	for(std::set<TreeNode *>::iterator it = node->chl.begin(); it != node->chl.end(); it++){
		std::list<rina::RoutingTableEntry *>::iterator pos = findEntry(table,
									       (*it)->name);


		if(pos != table.end()){
			ipcpna.name = next;
			(*pos)->nextHopNames.push_back(ipcpna);
		}
		else{
			rina::RoutingTableEntry * entry = new rina::RoutingTableEntry();
		        entry->destination.name = (*it)->name;
		        entry->qosId = 1;
		        entry->cost = (*it)->metric;
		        ipcpna.name = next;
			entry->nextHopNames.push_back(ipcpna);
			LOG_IPCP_DBG("Added entry to routing table: destination %s, next-hop %s",
                        entry->destination.name.c_str(), next.c_str());
			table.push_back(entry);
		}
		addRecursive(table, 1, next, *it);
	}
}

std::list<rina::RoutingTableEntry *>::iterator ECMPDijkstraAlgorithm::findEntry(std::list<rina::RoutingTableEntry *> &table,
										const std::string& name)
{
	std::list<rina::RoutingTableEntry *>::iterator it;
	for(it = table.begin(); it != table.end(); it++)
	{
		if((*it)->destination.name == name){
			return it;
		}
	}
	return it;
}

void ECMPDijkstraAlgorithm::execute(const Graph& graph,
				    const std::string& source)
{
	distances_[source] = 0;
	settled_nodes_.insert(source);
	t = new TreeNode(source, 0);

	std::list<Edge *>::const_iterator edgeIt;
	int cost;
	std::string target = std::string();
	int shortestDistance;
	for (edgeIt = graph.edges_.begin();
			edgeIt != graph.edges_.end(); ++edgeIt) {
		if (isNeighbor((*edgeIt), source)) {
			target = (*edgeIt)->getOtherEndpoint(source);
			distances_[target]=(*edgeIt)->weight_;
			predecessors_[target].push_front(t);
			unsettled_nodes_.insert(target);
		}
	}

	while (unsettled_nodes_.size() > 0) {
		std::set<std::string>::iterator it;
		getMinimum();
		for(it = minimum_nodes_.begin(); it != minimum_nodes_.end(); ++it){
			settled_nodes_.insert(*it);

			TreeNode * nt = new TreeNode(*it, distances_.find(*it)->second);
			bool fPar = true;
			for(std::list<TreeNode *>::iterator par = predecessors_.find(*it)->second.begin();
					par != predecessors_.find(*it)->second.end(); ++par){
				if(fPar){
					(*par)->chldel.insert(nt);
					fPar = false;
				}
				(*par)->chl.insert(nt);
			}

			unsettled_nodes_.erase(*it);
			findMinimalDistances(graph, nt);
		}
	}
}

void ECMPDijkstraAlgorithm::getMinimum()
{
	std::string minimum = std::string();
	std::set<std::string>::iterator it;
	minimum_nodes_.clear();
	std::list<Edge *>::const_iterator edgeIt;
	for (it = unsettled_nodes_.begin(); it != unsettled_nodes_.end(); ++it) {
		if (minimum == std::string()) {
			minimum_nodes_.insert(*it);
			minimum = (*it);
		} else {
			if (getShortestDistance((*it)) < getShortestDistance(minimum)) {
								
				minimum = (*it);
				minimum_nodes_.clear();
			}
			if (getShortestDistance((*it)) == getShortestDistance(minimum)) {
								
				minimum_nodes_.insert(*it);
			}
		}
	}
}

int ECMPDijkstraAlgorithm::getShortestDistance(const std::string& destination) const
{
	std::map<std::string, int>::const_iterator it;
	int distance = INT_MAX;

	it = distances_.find(destination);
	if (it != distances_.end()) {
		distance = it->second;
	}

	return distance;
}

void ECMPDijkstraAlgorithm::findMinimalDistances(const Graph& graph,
						 TreeNode * pred)
{
	std::list<std::string> adjacentNodes;
	std::list<Edge *>::const_iterator edgeIt;
	int cost;

	std::string target = std::string();
	int shortestDistance;
	for (edgeIt = graph.edges_.begin(); edgeIt != graph.edges_.end();
			++edgeIt) {
		if (isNeighbor((*edgeIt), pred->name)) {
			target = (*edgeIt)->getOtherEndpoint(pred->name);
			cost = (*edgeIt)->weight_;
			shortestDistance = getShortestDistance(pred->name) + cost;
			if (shortestDistance < getShortestDistance(target)) {
				distances_[target] = shortestDistance;
				predecessors_[target].clear();
			}
			if (shortestDistance == getShortestDistance(target)) {
				predecessors_[target].push_front(pred);
				unsettled_nodes_.insert(target);
			}
		}
	}
}

bool ECMPDijkstraAlgorithm::isNeighbor(Edge * edge,
				       const std::string& node) const
{
	if (edge->isVertexIn(node)) {
		if (!isSettled(edge->getOtherEndpoint(node))) {
			return true;
		}
	}

	return false;
}

bool ECMPDijkstraAlgorithm::isSettled(const std::string& node) const
{
	std::set<std::string>::iterator it;

	for (it = settled_nodes_.begin();
			it != settled_nodes_.end(); ++it) {
		if ((*it) == node) {
			return true;
		}
	}

	return false;
}

//Class IResiliencyAlgorithm
IResiliencyAlgorithm::IResiliencyAlgorithm(IRoutingAlgorithm& ra)
						: routing_algorithm(ra)
{
}

//Class LoopFreeAlternateAlgorithm
LoopFreeAlternateAlgorithm::LoopFreeAlternateAlgorithm(IRoutingAlgorithm& ra)
						: IResiliencyAlgorithm(ra)
{
}

void LoopFreeAlternateAlgorithm::extendRoutingTableEntry(
			std::list<rina::RoutingTableEntry *>& rt,
			const std::string& target_name,
			const std::string& nexthop)
{
	std::list<rina::RoutingTableEntry *>::iterator rit;
	bool found = false;
	rina::IPCPNameAddresses ipcpna;

	// Find the involved routing table entry
	for (rit = rt.begin(); rit != rt.end(); rit++) {
		if ((*rit)->destination.name == target_name) {
			break;
		}
	}

	if (rit == rt.end()) {
		LOG_WARN("LFA: Couldn't find routing table entry for "
			 "target name %s", target_name.c_str());
		return;
	}

	// Assume unicast and try to extend the routing table entry
	// with the new alternative 'nexthop'
	rina::NHopAltList& altlist = (*rit)->nextHopNames.front();

	for (std::list<rina::IPCPNameAddresses>::iterator
			hit = altlist.alts.begin();
				hit != altlist.alts.end(); hit++) {
		if (hit->name == nexthop) {
			// The nexthop is already in the alternatives
			found = true;
			break;
		}
	}

	if (!found) {
		ipcpna.name = nexthop;
		altlist.alts.push_back(ipcpna);
		LOG_DBG("Node %s selected as LFA node towards the "
			 "destination node %s", nexthop.c_str(), target_name.c_str());
	}
}

void LoopFreeAlternateAlgorithm::fortifyRoutingTable(const Graph& graph,
						     const std::string& source_name,
						     std::list<rina::RoutingTableEntry *>& rt)
{
	std::map<std::string, std::map< std::string, int > > neighbors_dist_trees;
	std::map<std::string, int> src_dist_tree;

	// TODO avoid this, can be computed when invoke computeRoutingTable()
	routing_algorithm.computeShortestDistances(graph, source_name, src_dist_tree);

	// Collect all the neighbors, and for each one use the routing algorithm to
	// compute the shortest distance map rooted at that neighbor
	for (std::list<std::string>::const_iterator it = graph.vertices_.begin();
						it != graph.vertices_.end(); ++it) {
		if ((*it) != source_name && graph.contains_edge(source_name, *it)) {
			neighbors_dist_trees[*it] = std::map<std::string, int>();
			routing_algorithm.computeShortestDistances(graph,
						*it, neighbors_dist_trees[*it]);
		}
	}

	// For each node X other than than the source node
	for (std::list<std::string>::const_iterator it = graph.vertices_.begin();
						it != graph.vertices_.end(); ++it) {
		if ((*it) == source_name) {
			continue;
		}

		// For each neighbor of the source node, excluding X
		for (std::map<std::string, std::map<std::string, int> >::iterator
			nit = neighbors_dist_trees.begin();
				nit != neighbors_dist_trees.end(); nit++) {
			// If this neighbor is a LFA node for the current
			// destination (*it) extend the routing table to take it
			// into account
			std::map< std::string, int>& neigh_dist_map = nit->second;
			std::string neigh = nit->first;

			if (neigh == *it) {
				continue;
			}

			// dist(neigh, target) < dist(neigh, source) + dist(source, target)
			if (neigh_dist_map[*it] < src_dist_tree[neigh] + src_dist_tree[*it]) {
				//LOG_DBG("Node %u is a possible LFA for destination %u",
				//	neigh, *it);
				extendRoutingTableEntry(rt, *it, neigh);
			}
		}
	}
}

// CLASS FlowStateObject
FlowStateObject::FlowStateObject()
{
	cost = 0;
	state_up = false;
	seq_num = 0;
	age = 0;
	modified = false;
	avoid_port = 0;
	being_erased = true;
}

FlowStateObject::FlowStateObject(const std::string& name_,
				 const std::string& neighbor_name_,
				 unsigned int cost_,
				 bool up,
				 int sequence_number,
				 unsigned int age_)
{
	name = name_;
	neighbor_name = neighbor_name_;
	cost = cost_;
	state_up = up;
	seq_num = sequence_number;
	age = age_;
	std::stringstream ss;
	ss << FlowStateRIBObject::object_name_prefix
	   << getKey();
	object_name = ss.str();
	modified = true;
	being_erased = false;
	avoid_port = 0;
}

FlowStateObject::~FlowStateObject()
{
}

const std::string FlowStateObject::toString() const
{
	std::stringstream ss;
	std::list<unsigned int>::const_iterator it;

	ss << "Name: " << name << "; Neighbor name: " << neighbor_name
		<< "; cost: " << cost << std::endl;
	ss << "Addresses: ";
	for (it = addresses.begin(); it != addresses.end(); ++it) {
		ss << *it << "; ";
	}
	ss << "Neighbor addresses: ";
	for (it = neighbor_addresses.begin(); it != neighbor_addresses.end(); ++it) {
		ss << *it << "; ";
	}
	ss << std::endl;
	ss << "Up: " << state_up << "; Sequence number: " << seq_num
		<< "; Age: " << age;

	return ss.str();
}

void FlowStateObject::add_address(unsigned int address)
{
	if (!contains_address(address))
		addresses.push_back(address);
}

void FlowStateObject::remove_address(unsigned int address)
{
	std::list<unsigned int>::iterator it;
	for (it = addresses.begin(); it != addresses.end(); ++it) {
		if (*it == address) {
			addresses.erase(it);
			return;
		}
	}
}

bool FlowStateObject::contains_address(unsigned int address) const
{
	std::list<unsigned int>::const_iterator it;
	for (it = addresses.begin(); it != addresses.end(); ++it) {
		if (*it == address) {
			return true;
		}
	}

	return false;
}

void FlowStateObject::set_addresses(const std::list<unsigned int>& addresses_)
{
	addresses.clear();
	for(std::list<unsigned int>::const_iterator it = addresses_.begin();
			it != addresses_.end(); ++it) {
		addresses.push_back(*it);
	}
}

void FlowStateObject::add_neighboraddress(unsigned int neighbor_address)
{
	if (!contains_neighboraddress(neighbor_address))
		neighbor_addresses.push_back(neighbor_address);
}

void FlowStateObject::remove_neighboraddress(unsigned int address)
{
	std::list<unsigned int>::iterator it;
	for (it = neighbor_addresses.begin();
			it != neighbor_addresses.end(); ++it) {
		if (*it == address) {
			neighbor_addresses.erase(it);
			return;
		}
	}
}

bool FlowStateObject::contains_neighboraddress(unsigned int address) const
{
	std::list<unsigned int>::const_iterator it;
	for (it = neighbor_addresses.begin();
			it != neighbor_addresses.end(); ++it) {
		if (*it == address) {
			return true;
		}
	}

	return false;
}

void FlowStateObject::set_neighboraddresses(const std::list<unsigned int>& addresses_)
{
	neighbor_addresses.clear();
	for(std::list<unsigned int>::const_iterator it = addresses_.begin();
			it != addresses_.end(); ++it) {
		neighbor_addresses.push_back(*it);
	}
}

void FlowStateObject::deprecateObject(unsigned int max_age)
{
	LOG_IPCP_DBG("Object %s deprecated", object_name.c_str());
	state_up = false;
	age = max_age +1 ;
	seq_num++;
	modified = true;
}

const std::string FlowStateObject::getKey() const
{
	std::stringstream ss;
	ss << name << "-" << neighbor_name;
	return ss.str();
}

// CLASS FlowStateRIBObject
const std::string FlowStateRIBObject::clazz_name = "FlowStateObject";
const std::string FlowStateRIBObject::object_name_prefix = "/ra/fsos/key=";

FlowStateRIBObject::FlowStateRIBObject(FlowStateObject* new_obj):
rina::rib::RIBObj(clazz_name)
{
	obj = new_obj;
}

void FlowStateRIBObject::read(const rina::cdap_rib::con_handle_t &con, 
	const std::string& fqn, const std::string& clas, 
	const rina::cdap_rib::filt_info_t &filt, const int invoke_id,
	rina::ser_obj_t &obj_reply, rina::cdap_rib::res_info_t& res)
{
	FlowStateObjectEncoder encoder;
	encoder.encode(*obj, obj_reply);

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

const std::string FlowStateRIBObject::get_displayable_value() const
{
	return obj->toString();
}

// CLASS FlowStateObjects
FlowStateObjects::FlowStateObjects(LinkStateRoutingPolicy * ps)
{
	modified_ = false;
	ps_ = ps;
	rina::rib::RIBObj *rib_objects = new FlowStateRIBObjects(this, ps);
	IPCPRIBDaemon* rib_daemon = (IPCPRIBDaemon*)IPCPFactory::getIPCP()
		->get_rib_daemon();
	rib_daemon->addObjRIB(FlowStateRIBObjects::object_name, &rib_objects);
	wait_until_remove_object = 0;
}

FlowStateObjects::~FlowStateObjects()
{
	for (std::map<std::string, FlowStateObject*>::iterator it = 
		objects.begin(); it != objects.end(); ++it)
	{
		delete it->second;
	}
	objects.clear();
	IPCPRIBDaemon* rib_daemon = (IPCPRIBDaemon*)IPCPFactory::getIPCP()
		->get_rib_daemon();
	rib_daemon->removeObjRIB(FlowStateRIBObjects::object_name);
}

void FlowStateObjects::set_wait_until_remove_object(unsigned int wait_object)
{
	wait_until_remove_object = wait_object;
}

bool FlowStateObjects::addObject(const FlowStateObject& object)
{
	rina::ScopedLock g(lock);

	if (objects.find(object.object_name) == objects.end())
	{
		addCheckedObject(object);
		return true;
	}
	else
	{
		LOG_DBG("FlowStateObject with name %s already present in the database",
			object.object_name.c_str());
		return false;
	}
}

void FlowStateObjects::addAddressToFSOs(const std::string& name,
		      	      	        unsigned int address,
					bool neighbor)
{
	rina::ScopedLock g(lock);
	std::map<std::string, FlowStateObject *>::iterator it;
	std::string my_name = IPCPFactory::getIPCP()->get_name();

	for (it = objects.begin(); it != objects.end(); ++it) {
		if (neighbor) {
			if (it->second->name == my_name
					&& it->second->neighbor_name == name) {
				it->second->add_neighboraddress(address);
				it->second->modified = true;
				it->second->age = 0;
				it->second->seq_num = it->second->seq_num + 1;
			}
		} else if (it->second->name == name) {
			it->second->add_address(address);
			it->second->modified = true;
			it->second->age = 0;
			it->second->seq_num = it->second->seq_num + 1;
		}
	}

	modified_ = true;
}

void FlowStateObjects::removeAddressFromFSOs(const std::string& name,
			   	   	     unsigned int address,
					     bool neighbor)
{
	rina::ScopedLock g(lock);
	std::map<std::string, FlowStateObject *>::iterator it;
	std::string my_name = IPCPFactory::getIPCP()->get_name();

	for (it = objects.begin(); it != objects.end(); ++it) {
		if (neighbor) {
			if (it->second->name == my_name
					&& it->second->neighbor_name == name) {
				it->second->remove_neighboraddress(address);
				it->second->modified = true;
				it->second->age = 0;
				it->second->seq_num = it->second->seq_num + 1;
			}
		} else if (it->second->name == name) {
			it->second->remove_address(address);
			it->second->modified = true;
			it->second->age = 0;
			it->second->seq_num = it->second->seq_num + 1;
		}
	}

	modified_ = true;
}

void FlowStateObjects::addCheckedObject(const FlowStateObject& object)
{
	FlowStateObject * fso = new FlowStateObject(object.name,
						    object.neighbor_name,
						    object.cost,
						    object.state_up,
						    object.seq_num,
						    object.age);

	fso->set_addresses(object.addresses);
	fso->set_neighboraddresses(object.neighbor_addresses);

	objects[object.object_name] = fso;
	rina::rib::RIBObj* rib_obj = new FlowStateRIBObject(fso);
	IPCPRIBDaemon* rib_daemon = (IPCPRIBDaemon*)IPCPFactory::getIPCP()->get_rib_daemon();
	rib_daemon->addObjRIB(fso->object_name, &rib_obj);
	modified_ = true;
}

void FlowStateObjects::deprecateObject(const std::string& fqn, 
				       unsigned int max_age)
{
	rina::ScopedLock g(lock);

	std::map<std::string, FlowStateObject*>::iterator it =
			objects.find(fqn);
	if(it != objects.end())
	{
		it->second->deprecateObject(max_age);
	}
}

void FlowStateObjects::deprecateObjects(const std::string& neigh_name,
		      	      	        const std::string& name,
		     	     	        unsigned int max_age)
{
	rina::ScopedLock g(lock);

	std::map<std::string, FlowStateObject *>::iterator it;
	for (it = objects.begin(); it != objects.end();
			++it) {
		if (it->second->neighbor_name == neigh_name &&
				it->second->name == name) {
			it->second->deprecateObject(max_age);
			modified_ = true;
		}
	}
}

void FlowStateObjects::updateCost(const std::string& neigh_name,
		      	      	  const std::string& name,
				  unsigned int cost)
{
	rina::ScopedLock g(lock);

	std::map<std::string, FlowStateObject *>::iterator it;
	for (it = objects.begin(); it != objects.end();
			++it) {
		if (it->second->neighbor_name == neigh_name &&
				it->second->name == name) {
			it->second->cost = cost;
			it->second->seq_num = it->second->seq_num + 1;
			it->second->modified = true;
			modified_ = true;
		}
	}
}

void FlowStateObjects::deprecateObjectsWithName(const std::string& name,
						unsigned int max_age,
						bool neighbor)
{
	rina::ScopedLock g(lock);
	std::string my_name = IPCPFactory::getIPCP()->get_name();

	std::map<std::string, FlowStateObject *>::iterator it;
	for (it = objects.begin(); it != objects.end();
			++it) {
		if (!neighbor && it->second->name == name) {
			it->second->deprecateObject(max_age);
			modified_ = true;
		} else if (neighbor && it->second->neighbor_name == name &&
				it->second->name == my_name) {
			it->second->deprecateObject(max_age);
			modified_ = true;
		}
	}
}

void FlowStateObjects::removeObject(const std::string& fqn)
{
	rina::ScopedLock g(lock);

	LOG_IPCP_DBG("Trying to remove object %s", fqn.c_str());

	std::map<std::string, FlowStateObject*>::iterator it =
			objects.find(fqn);

	if (it == objects.end())
		return;

	IPCPRIBDaemon* rib_daemon = (IPCPRIBDaemon*) IPCPFactory::getIPCP()->get_rib_daemon();
	rib_daemon->removeObjRIB(it->second->object_name);

	objects.erase(it);
	delete it->second;
}

FlowStateObject* FlowStateObjects::getObject(const std::string& fqn)
{
	rina::ScopedLock g(lock);

	std::map<std::string, FlowStateObject*>::iterator it =
		objects.find(fqn);
	if ( it != objects.end())
		return it->second;

	return 0;
}

void FlowStateObjects::has_modified(bool modified)
{
	modified_ = modified;
}

void FlowStateObjects::getModifiedFSOs(std::list<FlowStateObject *>& result)
{
	rina::ScopedLock g(lock);

	for (std::map<std::string, FlowStateObject*>::iterator it
		= objects.begin(); it != objects.end();++it)
	{
		if (it->second->modified)
		{
			result.push_back(it->second);
		}
	}
}

void FlowStateObjects::getAllFSOs(std::list<FlowStateObject>& result)
{
	rina::ScopedLock g(lock);

	for (std::map<std::string, FlowStateObject*>::iterator it
			= objects.begin(); it != objects.end();++it)
	{
		result.push_back(*(it->second));
	}
}

void FlowStateObjects::incrementAge(unsigned int max_age, rina::Timer* timer)
{
	rina::ScopedLock g(lock);

	for (std::map<std::string, FlowStateObject *>::iterator
		it = objects.begin(); it != objects.end(); ++it) 
	{
		if (it->second->age < UINT_MAX)
			it->second->age = it->second->age + 1;

		if (it->second->age >= max_age && !it->second->being_erased) {
			LOG_IPCP_DBG("Object to erase age: %d", it->second->age);
			it->second->being_erased = true;
			KillFlowStateObjectTimerTask* ksttask =
				new KillFlowStateObjectTimerTask(ps_, it->second->object_name);

			timer->scheduleTask(ksttask, wait_until_remove_object);
		}
	}
}

void FlowStateObjects::updateObject(const std::string& fqn, 
				    unsigned int avoid_port_)
{
	rina::ScopedLock g(lock);
	std::map<std::string, FlowStateObject*>::iterator it =
		objects.find(fqn);
	if (it == objects.end())
		return;

	FlowStateObject* obj = it->second;
	obj->age = 0;
	obj->avoid_port = avoid_port_;
	obj->being_erased = false;
	obj->state_up = true;
	obj->seq_num = 1;
	obj->modified = true;
}

void FlowStateObjects::encodeAllFSOs(rina::ser_obj_t& obj)
{
	rina::ScopedLock g(lock);

	FlowStateObjectListEncoder encoder;
	std::list<FlowStateObject> result;
	if (!objects.empty())
	{
		for (std::map<std::string, FlowStateObject*>::iterator it
			= objects.begin(); it != objects.end();++it)
		{
			result.push_back(*(it->second));
		}
		encoder.encode(result, obj);
	}
	else
	{
		obj.size_ = 0;
		obj.message_ = 0;
	}
}

void FlowStateObjects::getAllFSOsForPropagation(std::list< std::list<FlowStateObject> >& fsos,
						unsigned int max_objects)
{
	rina::ScopedLock g(lock);
	std::list<FlowStateObject> fsolist;

	if (objects.empty())
		return;

	for (std::map<std::string, FlowStateObject*>::iterator it
			= objects.begin(); it != objects.end();++it)
	{
		if (fsolist.size() == max_objects) {
			fsos.push_back(fsolist);
			fsolist.clear();
		}

		fsolist.push_back(*(it->second));
	}

	if (fsolist.size() != 0) {
		fsos.push_back(fsolist);
	}
}

bool FlowStateObjects::is_modified() const
{
	return modified_;
}

//Class FlowStateRIBObjects
const std::string FlowStateRIBObjects::clazz_name = "FlowStateObjects";
const std::string FlowStateRIBObjects::object_name= "/ra/fsos";

FlowStateRIBObjects::FlowStateRIBObjects(FlowStateObjects* new_objs,
					 LinkStateRoutingPolicy* ps) :
		rina::rib::RIBObj(clazz_name)
{
	objs = new_objs;
	ps_ = ps;
}

void FlowStateRIBObjects::read(const rina::cdap_rib::con_handle_t &con,
			       const std::string& fqn,
			       const std::string& clas,
			       const rina::cdap_rib::filt_info_t &filt,
			       const int invoke_id,
			       rina::ser_obj_t &obj_reply,
			       rina::cdap_rib::res_info_t& res)
{
	//TODO, implement following LSR policy specification
}

void FlowStateRIBObjects::write(const rina::cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& clas,
				const rina::cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const rina::ser_obj_t &obj_req,
				rina::ser_obj_t &obj_reply,
				rina::cdap_rib::res_info_t& res)
{
	FlowStateObjectListEncoder encoder;
	std::list<FlowStateObject> new_objects;
	encoder.decode(obj_req, new_objects);
	ps_->updateObjects(new_objects,
			   con.port_id);
}

// CLASS FlowStateManager
const int FlowStateManager::NO_AVOID_PORT = -1;
const long FlowStateManager::WAIT_UNTIL_REMOVE_OBJECT = 23000;

FlowStateManager::FlowStateManager(rina::Timer *new_timer,
				   unsigned int max_age,
				   LinkStateRoutingPolicy * ps)
{
	maximum_age = max_age;
	fsos = new FlowStateObjects(ps);
	timer = new_timer;
}

FlowStateManager::~FlowStateManager()
{
	delete fsos;
}

void FlowStateManager::addAddressToFSOs(const std::string& name,
					unsigned int address,
					bool neighbor)
{
	fsos->addAddressToFSOs(name, address, neighbor);
}

bool FlowStateManager::addNewFSO(const std::string& name,
				 std::list<unsigned int>& addresses,
				 const std::string& neighbor_name,
				 std::list<unsigned int>& neighbor_addresses,
				 unsigned int cost,
				 int avoid_port)
{
	FlowStateObject newObject(name,
				  neighbor_name,
				  cost,
				  true,
				  1,
				  0);

	newObject.set_addresses(addresses);
	newObject.set_neighboraddresses(neighbor_addresses);

	return fsos->addObject(newObject);
}

void FlowStateManager::deprecateObject(std::string fqn)
{
	fsos->deprecateObject(fqn, maximum_age);
}

void FlowStateManager::removeAddressFromFSOs(const std::string& name,
			   	   	     unsigned int address,
					     bool neighbor)
{
	fsos->removeAddressFromFSOs(name, address, neighbor);
}

void FlowStateManager::incrementAge()
{
	fsos->incrementAge(maximum_age, timer);
}

void FlowStateManager::updateObjects(const std::list<FlowStateObject>& newObjects,
				     unsigned int avoidPort)
{
	FlowStateObject * obj_to_up = 0;
	LOG_IPCP_DBG("Update objects from DB launched");

	std::string my_name = IPCPFactory::getIPCP()->get_name();

	for (std::list<FlowStateObject>::const_iterator
		newIt = newObjects.begin(); newIt != newObjects.end(); ++newIt)
	{
		FlowStateObject * obj_to_up = fsos->getObject(newIt->object_name);

		//1 If the object exists update
		if (obj_to_up != NULL)
		{
			LOG_IPCP_DBG("Found the object in the DB. Object: %s",
				obj_to_up->object_name.c_str());

			//1.1 If the object has a higher sequence number update
			if (newIt->seq_num > obj_to_up->seq_num)
			{
				LOG_IPCP_DBG("Update the object %s with seq num %d",
						obj_to_up->object_name.c_str(),
						newIt->seq_num);

				if (newIt->name == my_name)
				{
					LOG_IPCP_DBG("Object is self generated, updating the sequence number and age of %s to %d",
						     obj_to_up->object_name.c_str(),
						     obj_to_up->seq_num);
					obj_to_up->seq_num = newIt->seq_num+ 1;
					obj_to_up->avoid_port = NO_AVOID_PORT;
					obj_to_up->age = 0;
					obj_to_up->cost = newIt->cost;
				} else {
					obj_to_up->avoid_port = avoidPort;
					if (newIt->age >= maximum_age) {
						obj_to_up->deprecateObject(maximum_age);
					} else {
						obj_to_up->age = 0;
						obj_to_up->seq_num = newIt->seq_num;
						obj_to_up->set_addresses(newIt->addresses);
						obj_to_up->set_neighboraddresses(newIt->neighbor_addresses);
						obj_to_up->cost = newIt->cost;
					}
				}

				obj_to_up->modified = true;
				fsos->has_modified(true);
			}
		}
		//2. If the object does not exist create
		else
		{
			if(newIt->name != my_name)
			{
				LOG_IPCP_DBG("New object added");
				FlowStateObject fso(*newIt);
				fso.avoid_port = avoidPort;
				fso.modified = true;
				fsos->addObject(fso);
				fsos->has_modified(true);
			}
		}
	}
}

void FlowStateManager::prepareForPropagation(std::map<int, std::list< std::list<FlowStateObject> > >&  to_propagate,
					     unsigned int max_objects) const
{
	std::list<FlowStateObject> newfsolist;
	bool added = false;

	//1 Get the FSOs to propagate
	std::list<FlowStateObject *> modifiedFSOs;
	fsos->getModifiedFSOs(modifiedFSOs);

	//2 add each modified object to its port list
	for (std::list<FlowStateObject*>::iterator it = modifiedFSOs.begin();
			it != modifiedFSOs.end(); ++it)
	{
		LOG_DBG("Propagation: Check modified object %s with age %d and status %d",
			(*it)->object_name.c_str(),
			(*it)->age,
			(*it)->state_up);

		for(std::map<int, std::list< std::list<FlowStateObject> > >::iterator it2 =
				to_propagate.begin(); it2 != to_propagate.end(); ++it2)
		{
			if(it2->first != (*it)->avoid_port)
			{
				added = false;
				newfsolist.clear();

				for(std::list< std::list<FlowStateObject> >::iterator it3 = it2->second.begin();
						it3 != it2->second.end(); ++it3) {
					if (it3->size() < max_objects) {
						it3->push_back(**it);
						added = true;
						break;
					}
				}

				if (!added) {
					newfsolist.push_back(**it);
					it2->second.push_back(newfsolist);
				}
			}
		}

		(*it)->modified = false;
		(*it)->avoid_port = NO_AVOID_PORT;
	}
}

void FlowStateManager::removeObject(const std::string& fqn)
{
	fsos->removeObject(fqn);
}

void FlowStateManager::encodeAllFSOs(rina::ser_obj_t& obj) const
{
	fsos->encodeAllFSOs(obj);
}

void FlowStateManager::set_maximum_age(unsigned int max_age)
{
	maximum_age = max_age;
}

void FlowStateManager::set_wait_until_remove_object(unsigned int wait_object)
{
	fsos->set_wait_until_remove_object(wait_object);
}

void FlowStateManager::getAllFSOs(std::list<FlowStateObject>& list) const
{
	fsos->getAllFSOs(list);
}

void FlowStateManager::getAllFSOsForPropagation(std::list< std::list<FlowStateObject> >& fsolist,
						unsigned int max_objects)
{
	fsos->getAllFSOsForPropagation(fsolist, max_objects);
}

void FlowStateManager::deprecateObjectsNeighbor(const std::string& neigh_name,
                                                const std::string& name,
						bool both)
{
	fsos->deprecateObjects(neigh_name, name, maximum_age);

	if (both) {
		fsos->deprecateObjects(name, neigh_name, maximum_age);
	}
}

void FlowStateManager::updateCost(const std::string& neigh_name,
				  const std::string& name,
				  unsigned int cost)
{
	fsos->updateCost(neigh_name, name, cost);
	fsos->updateCost(name, neigh_name, cost);
}

void FlowStateManager::force_table_update()
{
	fsos->has_modified(true);
}

bool FlowStateManager::tableUpdate() const
{
	bool result = fsos->is_modified();
	if (result)
		fsos->has_modified(false);
	return result;
}

// ComputeRoutingTimerTask
ComputeRoutingTimerTask::ComputeRoutingTimerTask(
		LinkStateRoutingPolicy * lsr_policy, long delay)
{
	lsr_policy_ = lsr_policy;
	delay_ = delay;
}

void ComputeRoutingTimerTask::run()
{
	lsr_policy_->routingTableUpdate();

	if (delay_ < 0) {
		return;
	}

	//Re-schedule
	ComputeRoutingTimerTask * task = new ComputeRoutingTimerTask(
			lsr_policy_, delay_);

	lsr_policy_->timer_->scheduleTask(task, delay_);
}

KillFlowStateObjectTimerTask::KillFlowStateObjectTimerTask(LinkStateRoutingPolicy *ps, std::string fqn)
{
	ps_ = ps;
	fqn_ = fqn;
}

void KillFlowStateObjectTimerTask::run()
{
	ps_->removeFlowStateObject(fqn_);
}

PropagateFSODBTimerTask::PropagateFSODBTimerTask(
		LinkStateRoutingPolicy * lsr_policy, long delay)
{
	lsr_policy_ = lsr_policy;
	delay_ = delay;
}

void PropagateFSODBTimerTask::run()
{
	lsr_policy_->propagateFSDB();

	//Re-schedule
	PropagateFSODBTimerTask * task = new PropagateFSODBTimerTask(
			lsr_policy_, delay_);
	lsr_policy_->timer_->scheduleTask(task, delay_);
}

UpdateAgeTimerTask::UpdateAgeTimerTask(LinkStateRoutingPolicy * lsr_policy,
				       long delay)
{
	lsr_policy_ = lsr_policy;
	delay_ = delay;
}

void UpdateAgeTimerTask::run()
{
	lsr_policy_->updateAge();

	//Re-schedule
	UpdateAgeTimerTask * task = new UpdateAgeTimerTask(lsr_policy_,
			delay_);
	lsr_policy_->timer_->scheduleTask(task, delay_);
}

ExpireOldAddressTimerTask::ExpireOldAddressTimerTask(LinkStateRoutingPolicy * lsr_policy,
						     const std::string& name__,
						     unsigned int addr,
						     bool neigh)
{
	lsr_policy_ = lsr_policy;
	name_ = name__;
	address = addr;
	neighbor = neigh;
}

void ExpireOldAddressTimerTask::run()
{
	lsr_policy_->expireOldAddress(name_, address, neighbor);
}

// CLASS LinkStateRoutingPolicy
const std::string LinkStateRoutingPolicy::OBJECT_MAXIMUM_AGE = "objectMaximumAge";
const std::string LinkStateRoutingPolicy::WAIT_UNTIL_READ_CDAP = "waitUntilReadCDAP";
const std::string LinkStateRoutingPolicy::WAIT_UNTIL_ERROR = "waitUntilError";
const std::string LinkStateRoutingPolicy::WAIT_UNTIL_PDUFT_COMPUTATION = "waitUntilPDUFTComputation";
const std::string LinkStateRoutingPolicy::WAIT_UNTIL_FSODB_PROPAGATION = "waitUntilFSODBPropagation";
const std::string LinkStateRoutingPolicy::WAIT_UNTIL_AGE_INCREMENT = "waitUntilAgeIncrement";
const std::string LinkStateRoutingPolicy::WAIT_UNTIL_REMOVE_OBJECT = "waitUntilRemoveObject";
const std::string LinkStateRoutingPolicy::WAIT_UNTIL_DEPRECATE_OLD_ADDRESS = "waitUntilDeprecateAddress";
const std::string LinkStateRoutingPolicy::ROUTING_ALGORITHM = "routingAlgorithm";
const int LinkStateRoutingPolicy::MAXIMUM_BUFFER_SIZE = 4096;
const std::string LinkStateRoutingPolicy::DIJKSTRA_ALG = "Dijkstra";
const std::string LinkStateRoutingPolicy::ECMP_DIJKSTRA_ALG = "ECMPDijkstra";
const std::string LinkStateRoutingPolicy::MAXIMUM_OBJECTS_PER_ROUTING_UPDATE = "maxObjectsPerUpdate";

LinkStateRoutingPolicy::LinkStateRoutingPolicy(IPCProcess * ipcp)
{
	test_ = false;
	ipc_process_ = ipcp;
	rib_daemon_ = ipc_process_->rib_daemon_;
	routing_algorithm_ = 0;
	resiliency_algorithm_ = 0;
	db_ = 0;
	wait_until_deprecate_address_ = 0;
	max_objects_per_rupdate_ = MAX_OBJECTS_PER_ROUTING_UPDATE_DEFAULT;

	subscribeToEvents();
	timer_ = new rina::Timer();
	db_ = new FlowStateManager(timer_, UINT_MAX, this);
}

LinkStateRoutingPolicy::~LinkStateRoutingPolicy()
{
	delete timer_;
	delete routing_algorithm_;
	delete resiliency_algorithm_;
	delete db_;
}

void LinkStateRoutingPolicy::subscribeToEvents()
{
	ipc_process_->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED, this);
	ipc_process_->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED, this);
	ipc_process_->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::APP_NEIGHBOR_ADDED, this);
	ipc_process_->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::APP_CONNECTIVITY_TO_NEIGHBOR_LOST, this);
	ipc_process_->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::ADDRESS_CHANGE, this);
	ipc_process_->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::NEIGHBOR_ADDRESS_CHANGE, this);
}

void LinkStateRoutingPolicy::set_dif_configuration(
		const rina::DIFConfiguration& dif_configuration)
{
	std::string routing_alg;
        rina::PolicyConfig psconf;
        long delay;

        psconf = dif_configuration.routing_configuration_.policy_set_;

        try {
        	routing_alg = psconf.get_param_value_as_string(ROUTING_ALGORITHM);
        } catch (rina::Exception &e) {
        	LOG_WARN("No routing algorithm specified, using Dijkstra");
        	routing_alg = DIJKSTRA_ALG;
        }

        if (routing_alg == DIJKSTRA_ALG) {
        	routing_algorithm_ = new DijkstraAlgorithm();
                LOG_IPCP_DBG("Using Dijkstra as routing algorithm");
        } else if (routing_alg == ECMP_DIJKSTRA_ALG)  {
                routing_algorithm_ = new ECMPDijkstraAlgorithm();
                LOG_IPCP_DBG("Using ECMP Dijkstra as routing algorithm");
        } else {
        	throw rina::Exception("Unsupported routing algorithm");
        }
#if 0
	resiliency_algorithm_ = new LoopFreeAlternateAlgorithm(*routing_algorithm_);
#endif


	if (!test_) {
		try {

			db_->set_maximum_age(psconf.get_param_value_as_int(OBJECT_MAXIMUM_AGE));
		} catch (rina::Exception &e) {
			db_->set_maximum_age(PULSES_UNTIL_FSO_EXPIRATION_DEFAULT);
		}

		try {
			db_->set_wait_until_remove_object(psconf.get_param_value_as_long(WAIT_UNTIL_REMOVE_OBJECT));
		} catch (rina::Exception &e) {
			db_->set_wait_until_remove_object(WAIT_UNTIL_REMOVE_OBJECT_DEFAULT);
		}

		try {
			wait_until_deprecate_address_ = psconf.get_param_value_as_long(WAIT_UNTIL_DEPRECATE_OLD_ADDRESS);
		} catch (rina::Exception &e) {
			wait_until_deprecate_address_ = WAIT_UNTIL_DEPRECATE_OLD_ADDRESS_DEFAULT;
		}

		// Task to compute PDUFT
		try {
			delay = psconf.get_param_value_as_long(WAIT_UNTIL_PDUFT_COMPUTATION);
		} catch (rina::Exception &e) {
			delay = WAIT_UNTIL_PDUFT_COMPUTATION_DEFAULT;
		}
		ComputeRoutingTimerTask * cttask = new ComputeRoutingTimerTask(this, delay);
		timer_->scheduleTask(cttask, delay);

		// Task to increment age
		try {
			delay = psconf.get_param_value_as_long(WAIT_UNTIL_AGE_INCREMENT);
		} catch (rina::Exception &e) {
			delay = WAIT_UNTIL_AGE_INCREMENT_DEFAULT;
		}
		UpdateAgeTimerTask * uattask = new UpdateAgeTimerTask(this, delay);
		timer_->scheduleTask(uattask, delay);

		// Task to propagate modified FSO
		try {
			delay = psconf.get_param_value_as_long(WAIT_UNTIL_FSODB_PROPAGATION);
		} catch (rina::Exception &e) {
			delay = WAIT_UNTIL_FSODB_PROPAGATION_DEFAULT;
		}
		PropagateFSODBTimerTask * pfttask = new PropagateFSODBTimerTask(this,
				delay);
		timer_->scheduleTask(pfttask, delay);

		// Maximum objects per routing update
		try {
			max_objects_per_rupdate_ = psconf.get_param_value_as_uint(MAXIMUM_OBJECTS_PER_ROUTING_UPDATE);
		} catch (rina::Exception &e) {
			max_objects_per_rupdate_ = MAX_OBJECTS_PER_ROUTING_UPDATE_DEFAULT;
		}
	}

}

void LinkStateRoutingPolicy::eventHappened(rina::InternalEvent * event)
{
	if (!event)
		return;

	rina::ScopedLock g(lock_);

	if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED) {
			rina::NMinusOneFlowDeallocatedEvent * flowEvent =
				(rina::NMinusOneFlowDeallocatedEvent *) event;
			processFlowDeallocatedEvent(flowEvent);
	} else if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED) {
			rina::NMinusOneFlowAllocatedEvent * flowEvent =
				(rina::NMinusOneFlowAllocatedEvent *) event;
			processFlowAllocatedEvent(flowEvent);
	} else if (event->type == rina::InternalEvent::APP_NEIGHBOR_ADDED) {
			rina::NeighborAddedEvent * neighEvent = (rina::NeighborAddedEvent *) event;
			processNeighborAddedEvent(neighEvent);
	} else if (event->type == rina::InternalEvent::APP_CONNECTIVITY_TO_NEIGHBOR_LOST) {
			rina::ConnectiviyToNeighborLostEvent * neighEvent =
					(rina::ConnectiviyToNeighborLostEvent *) event;
			processNeighborLostEvent(neighEvent);
	} else if (event->type == rina::InternalEvent::ADDRESS_CHANGE) {
		rina::AddressChangeEvent * addrEvent =
			(rina::AddressChangeEvent *) event;
		processAddressChangeEvent(addrEvent);
	} else if (event->type == rina::InternalEvent::NEIGHBOR_ADDRESS_CHANGE) {
		rina::NeighborAddressChangeEvent * addrEvent =
			(rina::NeighborAddressChangeEvent *) event;
		processNeighborAddressChangeEvent(addrEvent);
	}
}

void LinkStateRoutingPolicy::processAddressChangeEvent(rina::AddressChangeEvent * event)
{
	std::list<FlowStateObject> all_fsos;
	std::string name = ipc_process_->get_name();

	db_->addAddressToFSOs(name,
			      event->new_address,
			      false);

	//Schedule task to remove all objects with old address
	ExpireOldAddressTimerTask * task = new ExpireOldAddressTimerTask(this,
							           	 name,
									 event->old_address,
									 false);
	timer_->scheduleTask(task, event->deprecate_old_timeout);
}

void LinkStateRoutingPolicy::processNeighborAddressChangeEvent(rina::NeighborAddressChangeEvent * event)
{
	std::list<FlowStateObject> all_fsos;
	unsigned int address = IPCPFactory::getIPCP()->get_address();
	unsigned int old_address = IPCPFactory::getIPCP()->get_old_address();

	db_->getAllFSOs(all_fsos);

	LOG_IPCP_DBG("Neighbor %s address changed: old address %d, new address %d",
		      event->neigh_name.c_str(),
		      event->old_address,
		      event->new_address);

	db_->addAddressToFSOs(event->neigh_name,
			      event->new_address,
			      true);

	//Schedule task to remove all objects with old address
	ExpireOldAddressTimerTask * task = new ExpireOldAddressTimerTask(this,
									 event->neigh_name,
									 event->old_address,
									 true);
	timer_->scheduleTask(task, wait_until_deprecate_address_);
}

void LinkStateRoutingPolicy::processFlowDeallocatedEvent(
		rina::NMinusOneFlowDeallocatedEvent * event)
{
	LOG_IPCP_DBG("N-1 Flow with neighbor lost");
	//TODO update cost

	//Force a routing table update
	db_->force_table_update();
}

void LinkStateRoutingPolicy::processNeighborLostEvent(rina::ConnectiviyToNeighborLostEvent* event)
{
	db_->deprecateObjectsNeighbor(event->neighbor_.name_.processName,
				      ipc_process_->get_name(),
				      true);
}


void LinkStateRoutingPolicy::processFlowAllocatedEvent(rina::NMinusOneFlowAllocatedEvent * event)
{
	std::list<unsigned int> addresses;
	std::list<unsigned int> neigh_addresses;

	//TODO, if we are already neighbors, check if cost has to be updated or
	//new FSOs have to be added for different (paralel) N-1 flows to neighbor

	//Force a routing table update
	db_->force_table_update();
}

void LinkStateRoutingPolicy::processNeighborAddedEvent(rina::NeighborAddedEvent * event)
{
	int portId = event->neighbor_.get_underlying_port_id();
	std::list<unsigned int> addresses;
	std::list<unsigned int> neigh_addresses;

	LOG_IPCP_DBG("Adding new FSO to neighbor");
	try {
		addresses.push_back(ipc_process_->get_address());
		neigh_addresses.push_back(
				ipc_process_->namespace_manager_->getAdressByname(
						event->neighbor_.get_name()));

		db_->addNewFSO(ipc_process_->get_name(),
				addresses,
				event->neighbor_.get_name().processName,
				neigh_addresses,
				1,
				portId);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Could not allocate the flow, no neighbor found");
	}

	if (event->prepare_handover) {
		db_->updateCost(event->disc_neigh_name.processName,
				ipc_process_->get_name(), 10000);
	}

	std::list< std::list<FlowStateObject> > all_fsos;
	FlowStateObjectListEncoder encoder;
	db_->getAllFSOsForPropagation(all_fsos, max_objects_per_rupdate_);
	for (std::list< std::list<FlowStateObject> >::iterator it = all_fsos.begin();
			it != all_fsos.end(); ++it) {
		try {
			rina::cdap_rib::obj_info_t obj;
			rina::cdap_rib::con_handle_t con;
			obj.class_ = FlowStateRIBObjects::clazz_name;
			obj.name_ = FlowStateRIBObjects::object_name;
			encoder.encode(*it, obj.value_);
			obj.inst_ = 0;
			rina::cdap_rib::flags_t flags;
			rina::cdap_rib::filt_info_t filt;
			con.port_id = portId;
			if (obj.value_.size_ != 0)
				rib_daemon_->getProxy()->remote_write(con,
						obj,
						flags,
						filt,
						0);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems encoding and sending CDAP message: %s", e.what());
		}
	}

	//Force a routing table update
	db_->force_table_update();
	_routingTableUpdate();
}

void LinkStateRoutingPolicy::propagateFSDB()
{
	rina::ScopedLock g(lock_);

	//1 Get the active N-1 flows
	std::list<int> n1_ports =
			ipc_process_->resource_allocator_->get_n_minus_one_flow_manager()->getManagementFlowsToAllNeighbors();
	//2 Initialize the map
	std::map <int, std::list< std::list<FlowStateObject> > > objectsToSend;
	for(std::list<int>::iterator it = n1_ports.begin();
		it != n1_ports.end(); ++it)
	{
		objectsToSend[*it] = std::list< std::list<FlowStateObject> >();
	}

	//3 Get the objects to send
	db_->prepareForPropagation(objectsToSend, max_objects_per_rupdate_);

	if (objectsToSend.size() == 0) {
		return;
	}

	FlowStateObjectListEncoder encoder;
	rina::cdap_rib::con_handle_t con;
	for (std::map<int, std::list< std::list<FlowStateObject> > >::iterator it = objectsToSend.begin();
		it != objectsToSend.end(); ++it)

	{
		if (it->second.size() == 0)
			continue;

		for (std::list< std::list<FlowStateObject> >::iterator it2 = it->second.begin();
				it2 != it->second.end(); ++it2) {
			rina::cdap_rib::flags flags;
			rina::cdap_rib::filt_info_t filter;
			try
			{
				rina::cdap_rib::object_info obj;
				obj.class_ = FlowStateRIBObjects::clazz_name;
				obj.name_ = FlowStateRIBObjects::object_name;
				encoder.encode(*it2, obj.value_);
				con.port_id = it->first;
				rib_daemon_->getProxy()->remote_write(con,
						obj,
						flags,
						filter,
						0);
			}
			catch (rina::Exception &e)
			{
				LOG_IPCP_ERR("Errors sending message: %s", e.what());
			}
		}
	}
}

void LinkStateRoutingPolicy::updateAge()
{
	rina::ScopedLock g(lock_);
	db_->incrementAge();
}

void LinkStateRoutingPolicy::printNhopTable(std::list<rina::RoutingTableEntry *>& rt)
{
	std::list<rina::RoutingTableEntry *>::iterator it;
	std::list<rina::NHopAltList>::iterator jt;
	rina::RoutingTableEntry * current = 0;
	std::stringstream ss;

	for(it = rt.begin(); it != rt.end(); ++it) {
		ss.str(std::string());
		ss.clear();
		current = *it;
		ss << "Dest. addresses: " << current->destination.get_addresses_as_string()
		   << "; QoS-id: " << current->qosId
		   << "; Next hops: ";
		for (jt = current->nextHopNames.begin();
				jt != current->nextHopNames.end(); ++jt) {
			 ss << jt->alts.front().get_addresses_as_string() << "; ";
		}

		LOG_IPCP_DBG("%s", ss.str().c_str());
	}
}

void LinkStateRoutingPolicy::populateAddresses(std::list<rina::RoutingTableEntry *>& rt,
		      	      	               const std::list<rinad::FlowStateObject>& fsos)
{
	std::map<std::string, std::list<unsigned int> > name_address_map;
	std::list<rinad::FlowStateObject>::const_iterator it;
	std::map<std::string, std::list<unsigned int> >::iterator jt;
	std::list<rina::RoutingTableEntry *>::iterator kt;
	std::list<rina::NHopAltList>::iterator nt;
	std::list<rina::IPCPNameAddresses>::iterator ot;
	std::list<unsigned int> aux;

	for (it = fsos.begin(); it != fsos.end(); ++it) {
		jt = name_address_map.find(it->name);
		if (jt == name_address_map.end())
			name_address_map[it->name] = it->addresses;
	}

	for (kt = rt.begin(); kt != rt.end(); ++kt) {
		jt = name_address_map.find((*kt)->destination.name);
		if (jt == name_address_map.end()) {
			LOG_IPCP_WARN("Could not find addresses for IPCP %s",
				      (*kt)->destination.name.c_str());
			continue;
		}
		(*kt)->destination.addresses = jt->second;

		for (nt = (*kt)->nextHopNames.begin();
				nt != (*kt)->nextHopNames.end(); ++nt) {

			for (ot = nt->alts.begin(); ot != nt->alts.end(); ++ot) {
				jt = name_address_map.find(ot->name);
				if (jt == name_address_map.end()) {
					LOG_IPCP_WARN("Could not find addresses for IPCP %s",
							ot->name.c_str());
					continue;
				}

				ot->addresses = jt->second;
			}
		}
	}
}

void LinkStateRoutingPolicy::routingTableUpdate()
{
	rina::ScopedLock g(lock_);
	_routingTableUpdate();
}

void LinkStateRoutingPolicy::_routingTableUpdate()
{
	std::list<rina::RoutingTableEntry *> rt;
	std::string my_name = ipc_process_->get_name();
	std::list<FlowStateObject> flow_state_objects;

	if (!db_->tableUpdate()) {
		return;
	}

	db_->getAllFSOs(flow_state_objects);

	// Build a graph out of the FSO database
	Graph graph(flow_state_objects);

	// Invoke the routing algorithm to compute the routing table
	// Main arguments are the graph and the source vertex.
	// The list of FSOs may be useless, but has been left there
	// for the moment (and it is currently unused by the Dijkstra
	// algorithm).
	routing_algorithm_->computeRoutingTable(graph,
						flow_state_objects,
						my_name,
						rt);

	// Run the resiliency algorithm, if any, to extend the routing table
	if (resiliency_algorithm_) {
		resiliency_algorithm_->fortifyRoutingTable(graph,
							   my_name,
							   rt);
	}


	//Populate addresses (right now there are only names int he RT entries)
	populateAddresses(rt, flow_state_objects);

	LOG_IPCP_DBG("Computed new Next Hop and PDU Forwarding Tables");
	printNhopTable(rt);

	assert(ipc_process_->resource_allocator_->pduft_gen_ps);
	ipc_process_->resource_allocator_->pduft_gen_ps->routingTableUpdated(rt);
}

void LinkStateRoutingPolicy::expireOldAddress(const std::string& name,
					      unsigned int address,
					      bool neighbor)
{
	rina::ScopedLock g(lock_);
	db_->removeAddressFromFSOs(name, address, neighbor);
}

void LinkStateRoutingPolicy::updateObjects(const std::list<FlowStateObject>& newObjects,
		   	   	   	   unsigned int avoidPort)
{
	rina::ScopedLock g(lock_);
	db_->updateObjects(newObjects,
			   avoidPort);
}

void LinkStateRoutingPolicy::removeFlowStateObject(const std::string& fqn)
{
	rina::ScopedLock g(lock_);
	db_->removeObject(fqn);
}

// CLASS FlowStateObjectEncoder
namespace fso_helpers{
void toGPB(const FlowStateObject &fso,
	   rina::messages::flowStateObject_t &gpb_fso)
{
	gpb_fso.set_name(fso.name);
	gpb_fso.set_age(fso.age);
	gpb_fso.set_neighbor_name(fso.neighbor_name);
	gpb_fso.set_cost(fso.cost);
	gpb_fso.set_state(fso.state_up);
	gpb_fso.set_sequence_number(fso.seq_num);

	std::list<unsigned int> addresses = fso.addresses;
	for(std::list<unsigned int>::const_iterator it = addresses.begin();
			it != addresses.end(); ++it) {
		gpb_fso.add_addresses(*it);
	}

	std::list<unsigned int> neigh_addresses = fso.neighbor_addresses;
	for(std::list<unsigned int>::const_iterator it = neigh_addresses.begin();
			it != neigh_addresses.end(); ++it) {
		gpb_fso.add_neighbor_addresses(*it);
	}
}

void toModel(
	const rina::messages::flowStateObject_t &gpb_fso, FlowStateObject &fso)
{
	fso.name = gpb_fso.name();
	fso.neighbor_name = gpb_fso.neighbor_name();
	fso.cost = gpb_fso.cost();
	fso.state_up = gpb_fso.state();
	fso.seq_num = gpb_fso.sequence_number();
	fso.age = gpb_fso.age();

	for (int i = 0; i < gpb_fso.addresses_size(); ++i) {
		fso.add_address(gpb_fso.addresses(i));
	}

	for (int i = 0; i < gpb_fso.neighbor_addresses_size(); ++i) {
		fso.add_neighboraddress(gpb_fso.neighbor_addresses(i));
	}
}
} //namespace fso_helpers

void FlowStateObjectEncoder::encode(const FlowStateObject &obj, 
				    rina::ser_obj_t &serobj)
{
	rina::messages::flowStateObject_t gpb;

	fso_helpers::toGPB(obj, gpb);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new unsigned char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void FlowStateObjectEncoder::decode(const rina::ser_obj_t &serobj,
				    FlowStateObject &des_obj)
{
	rina::messages::flowStateObject_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	fso_helpers::toModel(gpb, des_obj);
}

void FlowStateObjectListEncoder::encode(const std::list<FlowStateObject> &obj,
					rina::ser_obj_t& serobj)
{
	rina::messages::flowStateObjectGroup_t gpb;

	for (std::list<FlowStateObject>::const_iterator it= obj.begin();
		it != obj.end(); ++it) 
	{
		rina::messages::flowStateObject_t *gpb_fso;
		gpb_fso = gpb.add_flow_state_objects();
		fso_helpers::toGPB(*it, *gpb_fso);
	}

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new unsigned char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void FlowStateObjectListEncoder::decode(const rina::ser_obj_t &serobj,
					std::list<FlowStateObject> &des_obj)
{
	rina::messages::flowStateObjectGroup_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);
	for(int i=0; i<gpb.flow_state_objects_size(); i++)
	{
		FlowStateObject fso;
		fso_helpers::toModel(gpb.flow_state_objects(i), fso);
		std::stringstream ss;
		ss << FlowStateRIBObject::object_name_prefix
		   << fso.getKey();
		fso.object_name = ss.str();
		des_obj.push_back(fso);
	}
}

}// namespace rinad
