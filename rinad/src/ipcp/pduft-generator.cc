//
// PDU Forwarding Table Generator
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <sstream>

#define RINA_PREFIX "pduft-generator"

#include <librina/logs.h>

#include "pduft-generator.h"

namespace rinad {

//CLASS PDUFT Generator
const std::string PDUForwardingTableGenerator::LINK_STATE_POLICY = "LinkState";

PDUForwardingTableGenerator::PDUForwardingTableGenerator(){
	ipc_process_ = 0;
	pduftg_policy_ = 0;
}

void PDUForwardingTableGenerator::set_ipc_process(IPCProcess * ipc_process) {
	ipc_process_ = ipc_process;
}

void PDUForwardingTableGenerator::set_dif_configuration(
		const rina::DIFConfiguration& dif_configuration) {
	rina::PDUFTableGeneratorConfiguration pduftgConfig = dif_configuration.get_pduft_generator_configuration();

	if (pduftgConfig.get_pduft_generator_policy().get_name().compare(LINK_STATE_POLICY) != 0) {
		LOG_WARN("Unsupported PDU Forwarding Table Generation policy: %s.",
				pduftgConfig.get_pduft_generator_policy().get_name().c_str());
		throw Exception("Unknown PDU Forwarting Table Generator Policy");
	}

	//TODO pduftg_policy_ = new LinkStatePDUFTGeneratorPolicyImpl();
	pduftg_policy_->set_ipc_process(ipc_process_);
	pduftg_policy_->set_dif_configuration(dif_configuration);
}

IPDUFTGeneratorPolicy * PDUForwardingTableGenerator::get_pdu_ft_generator_policy() const {
	return pduftg_policy_;
}

//Class Flow State Object
FlowStateObject::FlowStateObject(unsigned int address, int port_id, unsigned int neighbor_address,
			int neighbor_port_id, bool up, int sequence_number, int age) {
	address_ = address;
	port_id_ = port_id;
	neighbor_address_ = neighbor_address;
	neighbor_port_id_ = neighbor_port_id;
	up_ = up;
	sequence_number_ = sequence_number;
	age_ = age;
	std::stringstream ss;
	ss<<EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_NAME<<EncoderConstants::SEPARATOR;
	ss<<address_<<"-"<<neighbor_address_;
	object_name_ = ss.str();
	modified_ = true;
	being_erased_ = false;
	avoid_port_ = 0;
	LOG_DBG("Created object with id: %s", object_name_.c_str());
}

const std::string FlowStateObject::toString() {
	std::stringstream ss;
	ss<<"Address: "<<address_<<"; Neighbor address: "<<neighbor_address_<<std::endl;
	ss<<"Port-id: "<<port_id_<<"; Neighbor port-id: "<<neighbor_port_id_<<std::endl;
	ss<<"Up: "<<up_<<"; Sequence number: "<<sequence_number_<<"; Age: "+ age_;

	return ss.str();
}

//Class Edge
Edge::Edge(unsigned int address1, int portV1, unsigned int address2, int portV2, int weight) {
	address1_ = address1;
	address2_ = address2;
	port_v1_ = portV1;
	port_v2_ = portV2;
	weight_ = weight;
}

bool Edge::isVertexIn(unsigned int address) const{
	if (address == address1_) {
		return true;
	}

	if (address == address2_) {
		return true;
	}

	return false;
}

unsigned int Edge::getOtherEndpoint(unsigned int address) {
	if (address == address1_) {
		return address2_;
	}

	if (address == address2_) {
		return address1_;
	}

	return 0;
}

std::list<unsigned int> Edge::getEndpoints() {
	std::list<unsigned int> result;
	result.push_back(address1_);
	result.push_back(address2_);

	return result;
}

bool Edge::operator==(const Edge & other) const {
	if (isVertexIn(other.address1_)) {
		return false;
	}

	if (!isVertexIn(other.address2_)) {
		return false;
	}

	if (weight_ != other.weight_) {
		return false;
	}

	return true;
}

bool Edge::operator!=(const Edge & other) const {
	return !(*this == other);
}

const std::string Edge::toString() const {
	std::stringstream ss;

	ss<<address1_<<" "<<address2_;
	return ss.str();
}

//Class Graph
Graph::Graph(const std::list<FlowStateObject *>& flow_state_objects) {
	flow_state_objects_ = flow_state_objects;
	init_vertices();
	init_edges();
}

Graph::~Graph() {
	std::list<CheckedVertex *>::iterator it;
	for(it = checked_vertices_.begin(); it != checked_vertices_.end(); ++it) {
		delete (*it);
	}

	std::list<Edge *>::iterator edgeIt;
	for(edgeIt = edges_.begin(); edgeIt != edges_.end(); ++edgeIt) {
		delete (*edgeIt);
	}
}

void Graph::init_vertices() {
	std::list<FlowStateObject *>::const_iterator it;
	for(it = flow_state_objects_.begin(); it!= flow_state_objects_.end(); ++it) {
		if (!contains_vertex((*it)->address_)) {
			vertices_.push_back((*it)->address_);
		}

		if (!contains_vertex((*it)->neighbor_address_)) {
			vertices_.push_back((*it)->neighbor_address_);
		}
	}
}

bool Graph::contains_vertex(unsigned int address) const {
	std::list<unsigned int>::const_iterator it;
	for(it = vertices_.begin(); it!= vertices_.end(); ++it) {
		if ((*it) == address) {
			return true;
		}
	}

	return false;
}

void Graph::init_edges() {
	std::list<unsigned int>::const_iterator it;
	std::list<FlowStateObject *>::const_iterator flowIt;

	for(it = vertices_.begin(); it!= vertices_.end(); ++it) {
		checked_vertices_.push_back(new CheckedVertex((*it)));
	}

	CheckedVertex * origin = 0;
	CheckedVertex * dest = 0;
	for(flowIt = flow_state_objects_.begin(); flowIt!= flow_state_objects_.end();
			++flowIt) {
		if (!(*flowIt)->up_) {
			continue;
		}

		LOG_DBG("Processing flow state object: %s", (*flowIt)->object_name_.c_str());

		origin = get_checked_vertex((*flowIt)->address_);
		if (origin == 0) {
			LOG_WARN("Could not find checked vertex for address %ud",
					(*flowIt)->address_);
			continue;
		}

		dest = get_checked_vertex((*flowIt)->neighbor_address_);
		if (dest == 0) {
			LOG_WARN("Could not find checked vertex for address %ud",
					(*flowIt)->neighbor_address_);
			continue;
		}

		if (origin->connection_contains_address(dest->address_) &&
				dest->connection_contains_address(origin->address_)) {
			edges_.push_back(new Edge(origin->address_, (*flowIt)->port_id_, dest->address_, dest->port_id_, 1));
			origin->connections_.remove(dest->address_);
			dest->connections_.remove(origin->address_);
		} else {
			origin->port_id_ = (*flowIt)->port_id_;
			origin->connections_.push_back(dest->address_);
			dest->connections_.push_back(origin->address_);
		}
	}
}

Graph::CheckedVertex * Graph::get_checked_vertex(unsigned int address) const {
	std::list<Graph::CheckedVertex *>::iterator it;
	for (it = checked_vertices_.begin(); it != checked_vertices_.end(); ++it) {
		if ((*it)->address_ == address) {
			return (*it);
		}
	}

	return 0;
}

//Class Predecessor Info
PredecessorInfo::PredecessorInfo(unsigned int nPredecessor, Edge * edge) {
	predecessor_ = nPredecessor;
	if (edge->address1_ == nPredecessor) {
		port_id_ = edge->port_v1_;
	} else {
		port_id_ = edge->port_v2_;
	}
}

PredecessorInfo::PredecessorInfo(unsigned int nPredecessor) {
	predecessor_ = nPredecessor;
	port_id_ = -1;
}

//Class DijsktraAlgorithm
DijkstraAlgorithm::DijkstraAlgorithm(){
	graph_ = 0;
}

std::list<rina::PDUForwardingTableEntry *> DijkstraAlgorithm::computePDUTForwardingTable(
			const std::list<FlowStateObject *>& fsoList, unsigned int source_address) {
	std::list<rina::PDUForwardingTableEntry *> result;
	std::list<unsigned int>::iterator it;
	PredecessorInfo * nextNode;
	rina::PDUForwardingTableEntry * entry;

	graph_ = new Graph(fsoList);

	execute(source_address);

	for(it = graph_->vertices_.begin(); it != graph_->vertices_.end(); ++it) {
		if ((*it) != source_address) {
			nextNode = getNextNode((*it), source_address);
			if (nextNode) {
				entry = new rina::PDUForwardingTableEntry();
				entry->setAddress((*it));
				entry->addPortId(nextNode->port_id_);
				entry->setQosId(1);
				result.push_back(entry);
			}
		}
	}

	delete graph_;

	return result;
}

void DijkstraAlgorithm::execute(unsigned int source) {
	distances_[source] = 0;
	unsettled_nodes_.insert(source);

	unsigned int node;
	while(unsettled_nodes_.size() > 0) {
		node = getMinimum();
		settled_nodes_.insert(node);
		unsettled_nodes_.erase(node);
		findMinimalDistances(node);
	}
}

unsigned int DijkstraAlgorithm::getMinimum() const {
	unsigned int minimum = UINT_MAX;
	std::set<unsigned int>::iterator it;

	for (it = unsettled_nodes_.begin(); it != unsettled_nodes_.end(); ++it) {
		if (minimum == UINT_MAX) {
			minimum = (*it);
		} else {
			if (getShortestDistance((*it)) < getShortestDistance(minimum)) {
				minimum = (*it);
			}
		}
	}

	return minimum;
}

int DijkstraAlgorithm::getShortestDistance(unsigned int destination) const {
	std::map<unsigned int, int>::iterator it;
	int distance = INT_MAX;

	it = distances_.find(destination);
	if (it != distances_.end()) {
		distance = it->second;
	}

	return distance;
}

void DijkstraAlgorithm::findMinimalDistances (unsigned int node) {
	std::list<unsigned int> adjacentNodes;
	std::list<Edge *>::iterator edgeIt;

	unsigned int target = 0;
	int shortestDistance;
	for (edgeIt = graph_->edges_.begin(); edgeIt != graph_->edges_.end();
			++edgeIt) {
		if (isNeighbor((*edgeIt), node)) {
			target = (*edgeIt)->getOtherEndpoint(node);
			shortestDistance = getShortestDistance(node) + (*edgeIt)->weight_;
			if (getShortestDistance(target) > shortestDistance) {
				distances_[target] = shortestDistance;
				predecessors_[target] = new PredecessorInfo(node, (*edgeIt));
				unsettled_nodes_.insert(target);
			}
		}
	}
}


bool DijkstraAlgorithm::isNeighbor(Edge * edge, unsigned int node) const {
	if (edge->isVertexIn(node)) {
		if (!isSettled(edge->getOtherEndpoint(node))) {
			return true;
		}
	}

	return false;
}

bool DijkstraAlgorithm::isSettled(unsigned int node) const {
	std::set<unsigned int>::iterator it;

	for(it = settled_nodes_.begin(); it!= settled_nodes_.end(); ++it) {
		if ((*it) == node) {
			return true;
		}
	}

	return false;
}

PredecessorInfo * DijkstraAlgorithm::getNextNode(unsigned int target, unsigned int source) {
	std::map<unsigned int, PredecessorInfo *>::iterator it;
	PredecessorInfo * step;

	it = predecessors_.find(target);
	if (it == predecessors_.end()) {
		return 0;
	} else {
		step = it->second;
	}

	it = predecessors_.find(step->predecessor_);
	while (it != predecessors_.end()) {
		step = it->second;
		if (step->predecessor_ == source) {
			break;
		}

		it = predecessors_.find(step->predecessor_);
	}

	if (step->predecessor_ == target) {
		return 0;
	}

	return step;
}

//Class FlowState RIB Object Group
FlowStateRIBObjectGroup::FlowStateRIBObjectGroup(IPCProcess * ipc_process,
			LinkStatePDUFTGeneratorPolicy * pduft_generator_policy):
					BaseRIBObject(ipc_process, EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_CLASS,
							objectInstanceGenerator->getObjectInstance(),
							EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_NAME) {
	pduft_generator_policy_ = pduft_generator_policy;
}

const void* FlowStateRIBObjectGroup::get_value() const {
	return 0;
}

void FlowStateRIBObjectGroup::remoteWriteObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	pduft_generator_policy_->writeMessageReceived(cdapMessage, cdapSessionDescriptor->get_port_id());
}

void FlowStateRIBObjectGroup::createObject(const std::string& objectClass,
		const std::string& objectName, const void* objectValue) {
	SimpleSetMemberRIBObject * ribObject = new SimpleSetMemberRIBObject(ipc_process_,
			objectClass, objectName, objectValue);
	add_child(ribObject);
	rib_daemon_->addRIBObject(ribObject);
}

//Class FlowStateDatabase
const int FlowStateDatabase::NO_AVOID_PORT = -1;
const int FlowStateDatabase::WAIT_UNTIL_REMOVE_OBJECT = 23000;

FlowStateDatabase::FlowStateDatabase(Encoder * encoder, FlowStateRIBObjectGroup *
		flow_state_rib_object_group, int maximum_age) {
	encoder_ = encoder;
	flow_state_rib_object_group_ = flow_state_rib_object_group;
	maximum_age_ = maximum_age;
	modified_ = false;
}

const std::list<FlowStateObject *>& FlowStateDatabase::get_flow_state_objects() const {
	return flow_state_objects_;
}

bool FlowStateDatabase::isEmpty() const {
	return flow_state_objects_.size() == 0;
}

const rina::SerializedObject * FlowStateDatabase::encode() {
	return encoder_->encode(&flow_state_objects_,
			EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_CLASS);
}

void FlowStateDatabase::setAvoidPort(int avoidPort) {
	std::list<FlowStateObject *>::iterator it;
	for(it=flow_state_objects_.begin(); it!=flow_state_objects_.end(); ++it) {
		(*it)->avoid_port_ = avoidPort;
	}
}

void FlowStateDatabase::addObjectToGroup(unsigned int address, int portId,
		unsigned int neighborAddress, int neighborPortId) {
	FlowStateObject * newObject = new FlowStateObject(address, portId,
			neighborAddress, neighborPortId, true, 1, 0);
	flow_state_objects_.push_back(newObject);
	flow_state_rib_object_group_->createObject(EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS,
			newObject->object_name_, newObject);
	modified_ = true;
}

FlowStateObject * FlowStateDatabase::getByPortId(int portId) {
	std::list<FlowStateObject *>::iterator it;
	for(it=flow_state_objects_.begin(); it!=flow_state_objects_.end(); ++it) {
		if ((*it)->port_id_ == portId) {
			return (*it);
		}
	}

	return 0;
}

bool FlowStateDatabase::deprecateObject(int portId) {
	FlowStateObject * fso = getByPortId(portId);
	if (!fso) {
		return false;
	}

	fso->up_ = false;
	fso->age_ = maximum_age_;
	fso->sequence_number_ = fso->sequence_number_ + 1;
	fso->avoid_port_ = NO_AVOID_PORT;
	fso->modified_ = true;
	modified_ = true;

	return true;
}

std::list<FlowStateObject*> FlowStateDatabase::getModifiedFSOs() {
	std::list<FlowStateObject*> result;
	std::list<FlowStateObject *>::iterator it;
	for(it=flow_state_objects_.begin(); it!=flow_state_objects_.end(); ++it) {
		if ((*it)->modified_) {
			result.push_back((*it));
		}
	}

	return result;
}

std::vector< std::list<FlowStateObject*> > FlowStateDatabase::prepareForPropagation(
		const std::list<rina::FlowInformation>& flows) {
	std::vector< std::list<FlowStateObject*> > result;
	std::list<FlowStateObject*> modifiedFSOs = getModifiedFSOs();

	if (modifiedFSOs.size() == 0) {
		return result;
	}

	std::list<FlowStateObject*>::iterator fsosIterator;
	std::list<rina::FlowInformation>::const_iterator flowsIterator;
	int portId = 0;
	for (fsosIterator = modifiedFSOs.begin(); fsosIterator != modifiedFSOs.end();
			++fsosIterator) {
		LOG_DBG("Check modified object: %s to be sent with age %d and status %d",
				(*fsosIterator)->object_name_.c_str(), (*fsosIterator)->age_,
				(*fsosIterator)->up_);

		std::list<FlowStateObject*> group;

		for(flowsIterator = flows.begin(); flowsIterator!= flows.end();
				++flowsIterator) {
			portId = (*flowsIterator).getPortId();
			if ((*fsosIterator)->avoid_port_ != portId) {
				LOG_DBG("To be sent to port %d", portId);
				group.push_back((*fsosIterator));
			}
		}

		result.push_back(group);
		(*fsosIterator)->modified_ = false;
		(*fsosIterator)->avoid_port_ = NO_AVOID_PORT;
	}

	return result;
}

void FlowStateDatabase::incrementAge() {
	std::list<FlowStateObject *>::iterator it;
	for(it=flow_state_objects_.begin(); it!=flow_state_objects_.end(); ++it) {
		(*it)->age_ = (*it)->age_ + 1;
		if ((*it)->age_ >= maximum_age_ && !(*it)->being_erased_) {
			LOG_DBG("Object to erase age: %d", (*it)->age_);
			//TODO statrt Kill Flow Satte Object Timer
			//Timer killFlowStateObjectTimer = new Timer();
			//killFlowStateObjectTimer.schedule(new KillFlowStateObject(fsRIBGroup, flowStateObjectArray.get(i), this),
					//WAIT_UNTIL_REMOVE_OBJECT);
			(*it)->being_erased_ = true;
		}

	}
}

void FlowStateDatabase::updateObjects(const std::list<FlowStateObject*>& newObjects,
		int avoidPort, unsigned int address) {
	LOG_DBG("Update objects from DB launched");

	std::list<FlowStateObject*>::const_iterator newIt, oldIt;;
	bool found = false;
	for(newIt = newObjects.begin(); newIt != newObjects.end(); ++newIt) {
		for(oldIt = flow_state_objects_.begin(); oldIt != flow_state_objects_.end(); ++oldIt) {

			if ((*newIt)->address_ == (*oldIt)->address_ &&
					(*newIt)->neighbor_address_ == (*oldIt)->neighbor_address_) {
				LOG_DBG("Found the object in the DB. Object: %s",
						(*oldIt)->object_name_.c_str());

				if ((*newIt)->sequence_number_ >= (*oldIt)->sequence_number_) {
					if ((*newIt)->address_ == address) {
						(*oldIt)->sequence_number_ = (*oldIt)->sequence_number_ + 1;
						(*oldIt)->avoid_port_ = NO_AVOID_PORT;
						LOG_DBG("Object is self generated, updating the sequence number of %s to %d",
								(*oldIt)->object_name_.c_str(), (*oldIt)->sequence_number_);
					} else {
						LOG_DBG("Update the object %s with seq num %d",
								(*oldIt)->object_name_.c_str(), (*newIt)->sequence_number_);
					    (*oldIt)->age_ = (*newIt)->age_;
					    (*oldIt)->up_ = (*newIt)->up_;
					    (*oldIt)->sequence_number_ = (*newIt)->sequence_number_;
					    (*oldIt)->avoid_port_ = avoidPort;
					}

					(*oldIt)->modified_ = true;
					modified_ = true;
				}

				found = true;
				break;
			}
		}

		if (found) {
			found = false;
			continue;
		}

		if ((*newIt)->address_ == address) {
			LOG_DBG("Object has origin myself, discard object %s", (*newIt)->object_name_.c_str());
			delete (*newIt);
			continue;
		}

		LOG_DBG("New object added");
		(*newIt)->avoid_port_ = avoidPort;
		(*newIt)->modified_ = true;
		flow_state_objects_.push_back((*newIt));
		modified_ = true;
		try {
			flow_state_rib_object_group_->createObject(EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS,
					(*newIt)->object_name_, (*newIt));
		} catch(Exception &e) {
			LOG_ERR("Problems creating RIB object: %s", e.what());
		}
	}
}

//class LinkStatePDUForwardingTable MEssage Handler
LinkStatePDUFTCDAPMessageHandler::LinkStatePDUFTCDAPMessageHandler(
		LinkStatePDUFTGeneratorPolicy * pduft_generator_policy) {
	pduft_generator_policy_ = pduft_generator_policy;
}

void LinkStatePDUFTCDAPMessageHandler::readResponse(const rina::CDAPMessage * cdapMessage,
				rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	pduft_generator_policy_->writeMessageReceived(cdapMessage, cdapSessionDescriptor->get_port_id());
}

//Class LinkStatePDUFTGPolicy
LinkStatePDUFTGeneratorPolicy::LinkStatePDUFTGeneratorPolicy(){
}

void LinkStatePDUFTGeneratorPolicy::writeMessageReceived(
		const rina::CDAPMessage * cdapMessage, int portId){
	LOG_DBG("Called; %d, %d", cdapMessage->get_op_code(), portId);
}

}
