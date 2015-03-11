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

#include <climits>
#include <sstream>

#define RINA_PREFIX "pduft-generator"

#include <librina/logs.h>

#include "common/encoders/FlowStateGroupMessage.pb.h"
#include "ipcp/pduft-generator.h"

namespace rinad {

//CLASS PDUFT Generator
const std::string PDUForwardingTableGenerator::LINK_STATE_POLICY = "LinkState";

PDUForwardingTableGenerator::PDUForwardingTableGenerator()
{
	ipc_process_ = 0;
	pduftg_policy_ = 0;
}

void PDUForwardingTableGenerator::set_ipc_process(IPCProcess * ipc_process)
{
	ipc_process_ = ipc_process;
}

void PDUForwardingTableGenerator::set_dif_configuration(
		const rina::DIFConfiguration& dif_configuration)
{
	rina::PDUFTableGeneratorConfiguration pduftgConfig =
			dif_configuration.get_pduft_generator_configuration();

	if (pduftgConfig.get_pduft_generator_policy().get_name().compare(
			LINK_STATE_POLICY) != 0) {
		LOG_WARN("Unsupported PDU Forwarding Table Generation policy: %s.",
				pduftgConfig.get_pduft_generator_policy().get_name().c_str());
		throw Exception("Unknown PDU Forwarting Table Generator Policy");
	}

	pduftg_policy_ = new LinkStatePDUFTGeneratorPolicy();
	pduftg_policy_->set_ipc_process(ipc_process_);
	pduftg_policy_->set_dif_configuration(dif_configuration);
}

IPDUFTGeneratorPolicy * PDUForwardingTableGenerator::get_pdu_ft_generator_policy() const
{
	return pduftg_policy_;
}

//Class Flow State Object
FlowStateObject::FlowStateObject(unsigned int address, int port_id,
		unsigned int neighbor_address, int neighbor_port_id, bool up,
		int sequence_number, int age)
{
	address_ = address;
	port_id_ = port_id;
	neighbor_address_ = neighbor_address;
	neighbor_port_id_ = neighbor_port_id;
	up_ = up;
	sequence_number_ = sequence_number;
	age_ = age;
	std::stringstream ss;
	ss << EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_NAME
			<< EncoderConstants::SEPARATOR;
	ss << address_ << "-" << neighbor_address_;
	object_name_ = ss.str();
	modified_ = true;
	being_erased_ = false;
	avoid_port_ = 0;
}

const std::string FlowStateObject::toString()
{
	std::stringstream ss;
	ss << "Address: " << address_ << "; Neighbor address: " << neighbor_address_
			<< std::endl;
	ss << "Port-id: " << port_id_ << "; Neighbor port-id: " << neighbor_port_id_
			<< std::endl;
	ss << "Up: " << up_ << "; Sequence number: " << sequence_number_
			<< "; Age: " + age_;

	return ss.str();
}

//Class Edge
Edge::Edge(unsigned int address1, int portV1, unsigned int address2, int portV2,
		int weight)
{
	address1_ = address1;
	address2_ = address2;
	port_v1_ = portV1;
	port_v2_ = portV2;
	weight_ = weight;
}

bool Edge::isVertexIn(unsigned int address) const
{
	if (address == address1_) {
		return true;
	}

	if (address == address2_) {
		return true;
	}

	return false;
}

unsigned int Edge::getOtherEndpoint(unsigned int address)
{
	if (address == address1_) {
		return address2_;
	}

	if (address == address2_) {
		return address1_;
	}

	return 0;
}

std::list<unsigned int> Edge::getEndpoints()
{
	std::list<unsigned int> result;
	result.push_back(address1_);
	result.push_back(address2_);

	return result;
}

bool Edge::operator==(const Edge & other) const
{
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

bool Edge::operator!=(const Edge & other) const
{
	return !(*this == other);
}

const std::string Edge::toString() const
{
	std::stringstream ss;

	ss << address1_ << " " << address2_;
	return ss.str();
}

//Class Graph
Graph::Graph(const std::list<FlowStateObject *>& flow_state_objects)
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
	std::list<FlowStateObject *>::const_iterator it;
	for (it = flow_state_objects_.begin(); it != flow_state_objects_.end();
			++it) {
		if (!contains_vertex((*it)->address_)) {
			vertices_.push_back((*it)->address_);
		}

		if (!contains_vertex((*it)->neighbor_address_)) {
			vertices_.push_back((*it)->neighbor_address_);
		}
	}
}

bool Graph::contains_vertex(unsigned int address) const
{
	std::list<unsigned int>::const_iterator it;
	for (it = vertices_.begin(); it != vertices_.end(); ++it) {
		if ((*it) == address) {
			return true;
		}
	}

	return false;
}

void Graph::init_edges()
{
	std::list<unsigned int>::const_iterator it;
	std::list<FlowStateObject *>::const_iterator flowIt;

	for (it = vertices_.begin(); it != vertices_.end(); ++it) {
		checked_vertices_.push_back(new CheckedVertex((*it)));
	}

	CheckedVertex * origin = 0;
	CheckedVertex * dest = 0;
	for (flowIt = flow_state_objects_.begin();
			flowIt != flow_state_objects_.end(); ++flowIt) {
		if (!(*flowIt)->up_) {
			continue;
		}

		LOG_DBG("Processing flow state object: %s",
				(*flowIt)->object_name_.c_str());

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

		if (origin->connection_contains_address(dest->address_)
				&& dest->connection_contains_address(origin->address_)) {
			edges_.push_back(
					new Edge(origin->address_, (*flowIt)->port_id_,
							dest->address_, dest->port_id_, 1));
			origin->connections.remove(dest->address_);
			dest->connections.remove(origin->address_);
		} else {
			origin->port_id_ = (*flowIt)->port_id_;
			origin->connections.push_back(dest->address_);
			dest->connections.push_back(origin->address_);
		}
	}
}

Graph::CheckedVertex * Graph::get_checked_vertex(unsigned int address) const
{
	std::list<Graph::CheckedVertex *>::const_iterator it;
	for (it = checked_vertices_.begin(); it != checked_vertices_.end(); ++it) {
		if ((*it)->address_ == address) {
			return (*it);
		}
	}

	return 0;
}

//Class Predecessor Info
PredecessorInfo::PredecessorInfo(unsigned int nPredecessor, Edge * edge)
{
	predecessor_ = nPredecessor;
	if (edge->address1_ == nPredecessor) {
		port_id_ = edge->port_v1_;
	} else {
		port_id_ = edge->port_v2_;
	}
}

PredecessorInfo::PredecessorInfo(unsigned int nPredecessor)
{
	predecessor_ = nPredecessor;
	port_id_ = -1;
}

//Class DijsktraAlgorithm
DijkstraAlgorithm::DijkstraAlgorithm()
{
	graph_ = 0;
}

std::list<rina::PDUForwardingTableEntry *> DijkstraAlgorithm::computePDUTForwardingTable(
		const std::list<FlowStateObject *>& fsoList,
		unsigned int source_address)
{
	std::list<rina::PDUForwardingTableEntry *> result;
	std::list<unsigned int>::iterator it;
	PredecessorInfo * nextNode;
	rina::PDUForwardingTableEntry * entry;

	graph_ = new Graph(fsoList);

	execute(source_address);

	for (it = graph_->vertices_.begin(); it != graph_->vertices_.end(); ++it) {
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
	unsettled_nodes_.clear();
	settled_nodes_.clear();
	predecessors_.clear();
	distances_.clear();

	return result;
}

void DijkstraAlgorithm::execute(unsigned int source)
{
	distances_[source] = 0;
	unsettled_nodes_.insert(source);

	unsigned int node;
	while (unsettled_nodes_.size() > 0) {
		node = getMinimum();
		settled_nodes_.insert(node);
		unsettled_nodes_.erase(node);
		findMinimalDistances(node);
	}
}

unsigned int DijkstraAlgorithm::getMinimum() const
{
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

int DijkstraAlgorithm::getShortestDistance(unsigned int destination) const
{
	std::map<unsigned int, int>::const_iterator it;
	int distance = INT_MAX;

	it = distances_.find(destination);
	if (it != distances_.end()) {
		distance = it->second;
	}

	return distance;
}

void DijkstraAlgorithm::findMinimalDistances(unsigned int node)
{
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

bool DijkstraAlgorithm::isNeighbor(Edge * edge, unsigned int node) const
{
	if (edge->isVertexIn(node)) {
		if (!isSettled(edge->getOtherEndpoint(node))) {
			return true;
		}
	}

	return false;
}

bool DijkstraAlgorithm::isSettled(unsigned int node) const
{
	std::set<unsigned int>::iterator it;

	for (it = settled_nodes_.begin(); it != settled_nodes_.end(); ++it) {
		if ((*it) == node) {
			return true;
		}
	}

	return false;
}

PredecessorInfo * DijkstraAlgorithm::getNextNode(unsigned int target,
		unsigned int source)
{
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
		LinkStatePDUFTGeneratorPolicy * pduft_generator_policy) :
		BaseIPCPRIBObject(ipc_process,
				EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_CLASS,
				rina::objectInstanceGenerator->getObjectInstance(),
				EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_NAME)
{
	pduft_generator_policy_ = pduft_generator_policy;
}

const void* FlowStateRIBObjectGroup::get_value() const
{
	return 0;
}

void FlowStateRIBObjectGroup::remoteWriteObject(void * object_value,
		int invoke_id, rina::CDAPSessionDescriptor * cdapSessionDescriptor)
{
	(void) invoke_id;

	std::list<FlowStateObject *> * objects =
			(std::list<FlowStateObject *> *) object_value;
	pduft_generator_policy_->writeMessageReceived(*objects,
			cdapSessionDescriptor->get_port_id());
	delete objects;
}

void FlowStateRIBObjectGroup::createObject(const std::string& objectClass,
		const std::string& objectName, const void* objectValue)
{
	SimpleSetMemberIPCPRIBObject * ribObject = new SimpleSetMemberIPCPRIBObject(
			ipc_process_, objectClass, objectName, objectValue);
	add_child(ribObject);
	rib_daemon_->addRIBObject(ribObject);
}

//Class FlowStateDatabase
const int FlowStateDatabase::NO_AVOID_PORT = -1;
const long FlowStateDatabase::WAIT_UNTIL_REMOVE_OBJECT = 23000;

FlowStateDatabase::FlowStateDatabase(Encoder * encoder,
		FlowStateRIBObjectGroup * flow_state_rib_object_group,
		rina::Timer * timer, IPCPRIBDaemon *rib_daemon, unsigned int *maximum_age)
{
	encoder_ = encoder;
	flow_state_rib_object_group_ = flow_state_rib_object_group;
	modified_ = false;
	timer_ = timer;
	rib_daemon_ = rib_daemon;
	maximum_age_ = maximum_age;
}

bool FlowStateDatabase::isEmpty() const
{
	return flow_state_objects_.size() == 0;
}

void FlowStateDatabase::setAvoidPort(int avoidPort)
{
	std::list<FlowStateObject *>::iterator it;
	for (it = flow_state_objects_.begin(); it != flow_state_objects_.end();
			++it) {
		(*it)->avoid_port_ = avoidPort;
	}
}

void FlowStateDatabase::addObjectToGroup(unsigned int address, int portId,
		unsigned int neighborAddress, int neighborPortId)
{
	FlowStateObject * newObject = new FlowStateObject(address, portId,
			neighborAddress, neighborPortId, true, 1, 0);

	for (std::list<FlowStateObject *>::iterator it =
			flow_state_objects_.begin(); it != flow_state_objects_.end();
			++it) {
		if ((*it)->object_name_.compare(newObject->object_name_) == 0) {
			delete newObject;
			(*it)->age_ = 0;
			(*it)->avoid_port_ = portId;
			(*it)->being_erased_ = false;
			(*it)->up_ = true;
			(*it)->sequence_number_ = 1;
			(*it)->modified_ = true;
			(*it)->being_erased_ = false;
			return;
		}
	}
	flow_state_objects_.push_back(newObject);
	flow_state_rib_object_group_->createObject(
			EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS,
			newObject->object_name_, newObject);
	modified_ = true;
}

FlowStateObject * FlowStateDatabase::getByPortId(int portId)
{
	std::list<FlowStateObject *>::iterator it;
	for (it = flow_state_objects_.begin(); it != flow_state_objects_.end();
			++it) {
		if ((*it)->port_id_ == portId) {
			return (*it);
		}
	}

	return 0;
}

void FlowStateDatabase::deprecateObject(int port_id)
{
	FlowStateObject * fso = getByPortId(port_id);
	if (!fso) {
		return;
	}
	LOG_DBG("Object %s deprecated", fso->object_name_.c_str());

	fso->up_ = false;
	fso->age_ = *maximum_age_;
	fso->sequence_number_ = fso->sequence_number_ + 1;
	fso->avoid_port_ = port_id;
	fso->modified_ = true;
	modified_ = true;
}

std::list<FlowStateObject*> FlowStateDatabase::getModifiedFSOs()
{
	std::list<FlowStateObject*> result;
	std::list<FlowStateObject *>::iterator it;
	for (it = flow_state_objects_.begin(); it != flow_state_objects_.end();
			++it) {
		if ((*it)->modified_) {
			result.push_back((*it));
		}
	}

	return result;
}

void FlowStateDatabase::getAllFSOs(
		std::list<FlowStateObject*> &flow_state_objects)
{
	flow_state_objects = flow_state_objects_;
}

unsigned int FlowStateDatabase::get_maximum_age() const
{
	return *maximum_age_;
}

std::map <int, std::list<FlowStateObject*> > FlowStateDatabase::prepareForPropagation(
		const std::list<rina::FlowInformation>& flows)
{
	std::map <int, std::list<FlowStateObject*> > result;
	for (std::list<rina::FlowInformation>::const_iterator it = flows.begin();
	    it != flows.end(); ++it ){
	  std::list<FlowStateObject*> list;
	  result[it->portId] = list;
	}

	std::list<FlowStateObject*> modifiedFSOs = getModifiedFSOs();
	if (modifiedFSOs.size() == 0) {
		return result;
	}

  for (std::list<FlowStateObject*>::iterator it = modifiedFSOs.begin();
      it != modifiedFSOs.end(); ++it) {

    LOG_DBG("Propagation: Check modified object %s with age %d and status %d",
    (*it)->object_name_.c_str(), (*it)->age_, (*it)->up_);

    for(std::map<int, std::list<FlowStateObject*> >::iterator it2 =
        result.begin(); it2 != result.end(); ++it2){
      if(it2->first != (*it)->avoid_port_){
        it2->second.push_back(*it);
      }
    }
    (*it)->modified_ = false;
    (*it)->avoid_port_ = NO_AVOID_PORT;
  }
/*
	int portId = 0;
	for (fsosIterator = modifiedFSOs.begin();
			fsosIterator != modifiedFSOs.end(); ++fsosIterator) {
		LOG_DBG(
				"Propagation: Check modified object %s with age %d and status %d",
				(*fsosIterator)->object_name_.c_str(), (*fsosIterator)->age_, (*fsosIterator)->up_);

		std::list<FlowStateObject*> group;

		for (flowsIterator = flows.begin(); flowsIterator != flows.end();
				++flowsIterator) {
			portId = (*flowsIterator).portId;
			if ((*fsosIterator)->avoid_port_ != portId) {
				LOG_DBG("Object to be sent to port %d", portId);
				group.push_back((*fsosIterator));
			}
		}

		result.push_back(group);
		(*fsosIterator)->modified_ = false;
		(*fsosIterator)->avoid_port_ = NO_AVOID_PORT;
	}
*/
	return result;
}

void FlowStateDatabase::incrementAge()
{
	std::list<FlowStateObject *>::iterator it;
	for (it = flow_state_objects_.begin(); it != flow_state_objects_.end();
			++it) {
		if ((*it)->age_ < UINT_MAX)
			(*it)->age_ = (*it)->age_ + 1;

		if ((*it)->age_ >= *maximum_age_ && !(*it)->being_erased_) {
			LOG_DBG("Object to erase age: %d", (*it)->age_);
			KillFlowStateObjectTimerTask * ksttask =
					new KillFlowStateObjectTimerTask(rib_daemon_, (*it), this);
			timer_->scheduleTask(ksttask, WAIT_UNTIL_REMOVE_OBJECT);
			(*it)->being_erased_ = true;
		}
	}
}

void FlowStateDatabase::updateObjects(
		const std::list<FlowStateObject*>& newObjects, int avoidPort,
		unsigned int address)
{
	LOG_DBG("Update objects from DB launched");

	std::list<FlowStateObject*>::const_iterator newIt, oldIt;
	;
	bool found = false;
	for (newIt = newObjects.begin(); newIt != newObjects.end(); ++newIt) {
		for (oldIt = flow_state_objects_.begin();
				oldIt != flow_state_objects_.end(); ++oldIt) {

			if ((*newIt)->address_ == (*oldIt)->address_
					&& (*newIt)->neighbor_address_
							== (*oldIt)->neighbor_address_) {
				LOG_DBG("Found the object in the DB. Object: %s",
						(*oldIt)->object_name_.c_str());

				if ((*newIt)->sequence_number_ > (*oldIt)->sequence_number_
						|| (*oldIt)->age_ >= *maximum_age_) {
					if ((*newIt)->address_ == address) {
						(*oldIt)->sequence_number_ = (*newIt)->sequence_number_
								+ 1;
						(*oldIt)->avoid_port_ = NO_AVOID_PORT;
						LOG_DBG(
								"Object is self generated, updating the sequence number of %s to %d",
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
				delete (*newIt);
				break;
			}
		}

		if (found) {
			found = false;
			continue;
		}

		if ((*newIt)->address_ == address) {
			LOG_DBG("Object has origin myself, discard object %s",
					(*newIt)->object_name_.c_str());
			delete (*newIt);
			continue;
		}

		LOG_DBG("New object added");
		(*newIt)->avoid_port_ = avoidPort;
		(*newIt)->modified_ = true;
		flow_state_objects_.push_back((*newIt));
		modified_ = true;
		try {
			flow_state_rib_object_group_->createObject(
					EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS,
					(*newIt)->object_name_, (*newIt));
		} catch (Exception &e) {
			LOG_ERR("Problems creating RIB object: %s", e.what());
		}
	}
}

//class LinkStatePDUForwardingTable MEssage Handler
LinkStatePDUFTCDAPMessageHandler::LinkStatePDUFTCDAPMessageHandler(
		LinkStatePDUFTGeneratorPolicy * pduft_generator_policy)
{
	pduft_generator_policy_ = pduft_generator_policy;
}

void LinkStatePDUFTCDAPMessageHandler::readResponse(int result,
		const std::string& result_reason, void * object_value,
		const std::string& object_name,
		rina::CDAPSessionDescriptor * session_descriptor)
{
	(void) object_name;

	if (result != 0) {
		LOG_ERR("Problems reading Flow State Objects from neighbor: %s",
				result_reason.c_str());
	}

	std::list<FlowStateObject *> * objects =
			(std::list<FlowStateObject *> *) object_value;
	pduft_generator_policy_->writeMessageReceived(*objects,
			session_descriptor->port_id_);
	delete objects;
}

//Class ComputePDUFTTimerTask
ComputePDUFTTimerTask::ComputePDUFTTimerTask(
		LinkStatePDUFTGeneratorPolicy * pduft_generator_policy, long delay)
{
	pduft_generator_policy_ = pduft_generator_policy;
	delay_ = delay;
}

void ComputePDUFTTimerTask::run()
{
	pduft_generator_policy_->forwardingTableUpdate();

	//Re-schedule
	ComputePDUFTTimerTask * task = new ComputePDUFTTimerTask(
			pduft_generator_policy_, delay_);
	pduft_generator_policy_->timer_->scheduleTask(task, delay_);
}

//Class KillFlowStateObjectTimerTask
KillFlowStateObjectTimerTask::KillFlowStateObjectTimerTask(
		IPCPRIBDaemon * rib_daemon, FlowStateObject * fso,
		FlowStateDatabase * fs_db)
{
	rib_daemon_ = rib_daemon;
	fso_ = fso;
	fs_db_ = fs_db;
}

void KillFlowStateObjectTimerTask::run()
{
	std::list<FlowStateObject *>::iterator it;
	if (fso_->age_ >= fs_db_->get_maximum_age()) {
		for (it = fs_db_->flow_state_objects_.begin();
				it != fs_db_->flow_state_objects_.end(); ++it) {
			if ((*it)->address_ == fso_->address_
					&& (*it)->neighbor_address_ == fso_->neighbor_address_
					&& (*it)->port_id_ == fso_->port_id_) {
				fs_db_->flow_state_objects_.erase(it);
				break;
			}
		}

		try {
			SimpleSetMemberIPCPRIBObject *fs_rib_o =
					(SimpleSetMemberIPCPRIBObject*) rib_daemon_->readObject(
							EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS,
							fso_->object_name_);
			fs_rib_o->deleteObject(fso_);
		} catch (Exception &e) {
			LOG_ERR("Object could not be removed from the RIB");
		}
		delete fso_;
	}
}

//Class PropagateFSODBTimerTask
PropagateFSODBTimerTask::PropagateFSODBTimerTask(
		LinkStatePDUFTGeneratorPolicy * pduft_generator_policy, long delay)
{
	pduft_generator_policy_ = pduft_generator_policy;
	delay_ = delay;
}

void PropagateFSODBTimerTask::run()
{
	pduft_generator_policy_->propagateFSDB();

	//Re-schedule
	PropagateFSODBTimerTask * task = new PropagateFSODBTimerTask(
			pduft_generator_policy_, delay_);
	pduft_generator_policy_->timer_->scheduleTask(task, delay_);
}

//Class UpdateAgeTimerTask
UpdateAgeTimerTask::UpdateAgeTimerTask(
		LinkStatePDUFTGeneratorPolicy * pduft_generator_policy, long delay)
{
	pduft_generator_policy_ = pduft_generator_policy;
	delay_ = delay;
}

void UpdateAgeTimerTask::run()
{
	pduft_generator_policy_->updateAge();

	//Re-schedule
	UpdateAgeTimerTask * task = new UpdateAgeTimerTask(pduft_generator_policy_,
			delay_);
	pduft_generator_policy_->timer_->scheduleTask(task, delay_);
}

//Class LinkStatePDUFTGPolicy
const int LinkStatePDUFTGeneratorPolicy::MAXIMUM_BUFFER_SIZE = 4096;

LinkStatePDUFTGeneratorPolicy::LinkStatePDUFTGeneratorPolicy()
{
	test_ = false;
	ipc_process_ = 0;
	rib_daemon_ = 0;
	encoder_ = 0;
	cdap_session_manager_ = 0;
	fs_rib_group_ = 0;
	routing_algorithm_ = 0;
	source_vertex_ = 0;
	maximum_age_ = UINT_MAX;
	db_ = 0;
	timer_ = new rina::Timer();
	lock_ = new rina::Lockable();
}

LinkStatePDUFTGeneratorPolicy::~LinkStatePDUFTGeneratorPolicy()
{
	if (routing_algorithm_) {
		delete routing_algorithm_;
	}

	if (db_) {
		delete db_;
	}

	if (timer_) {
		delete timer_;
	}

	if (lock_) {
		delete lock_;
	}
}

void LinkStatePDUFTGeneratorPolicy::set_ipc_process(IPCProcess * ipc_process)
{
	ipc_process_ = ipc_process;
	rib_daemon_ = ipc_process_->rib_daemon_;
	encoder_ = ipc_process_->encoder_;
	cdap_session_manager_ = ipc_process_->cdap_session_manager_;
	populateRIB();
	subscribeToEvents();
	db_ = new FlowStateDatabase(encoder_, fs_rib_group_, timer_, rib_daemon_,
			&maximum_age_);
}

void LinkStatePDUFTGeneratorPolicy::populateRIB()
{
	try {
		fs_rib_group_ = new FlowStateRIBObjectGroup(ipc_process_, this);
		rib_daemon_->addRIBObject(fs_rib_group_);
	} catch (Exception &e) {
		LOG_ERR("Problems adding object to RIB: %s", e.what());
	}
}

void LinkStatePDUFTGeneratorPolicy::subscribeToEvents()
{
	rib_daemon_->subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED, this);
	rib_daemon_->subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED, this);
	rib_daemon_->subscribeToEvent(IPCP_EVENT_NEIGHBOR_ADDED, this);
}

void LinkStatePDUFTGeneratorPolicy::set_dif_configuration(
		const rina::DIFConfiguration& dif_configuration)
{
	pduft_generator_config_ =
			dif_configuration.get_pduft_generator_configuration();
	if (pduft_generator_config_.get_link_state_routing_configuration().get_routing_algorithm().compare(
			"Dijkstra") != 0) {
		LOG_WARN("Unsupported routing algorithm, using Dijkstra instead");
	}

	routing_algorithm_ = new DijkstraAlgorithm();
	source_vertex_ = dif_configuration.get_address();

	if (!test_) {
		maximum_age_ =
				pduft_generator_config_.get_link_state_routing_configuration().get_object_maximum_age();
		long delay = 0;

		// Task to compute PDUFT
		delay =
				pduft_generator_config_.get_link_state_routing_configuration().get_wait_until_pduft_computation();
		ComputePDUFTTimerTask * cttask = new ComputePDUFTTimerTask(this, delay);
		timer_->scheduleTask(cttask, delay);

		// Task to increment age
		delay =
				pduft_generator_config_.get_link_state_routing_configuration().get_wait_until_age_increment();
		UpdateAgeTimerTask * uattask = new UpdateAgeTimerTask(this, delay);
		timer_->scheduleTask(uattask, delay);

		// Task to propagate modified FSO
		delay =
				pduft_generator_config_.get_link_state_routing_configuration().get_wait_until_fsodb_propagation();
		PropagateFSODBTimerTask * pfttask = new PropagateFSODBTimerTask(this,
				delay);
		timer_->scheduleTask(pfttask, delay);
	}
}

const std::list<rina::FlowInformation>& LinkStatePDUFTGeneratorPolicy::get_allocated_flows() const
{
	return allocated_flows_;
}

void LinkStatePDUFTGeneratorPolicy::eventHappened(Event * event)
{
	if (!event)
		return;

	rina::ScopedLock g(*lock_);

	if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED) {
		NMinusOneFlowDeallocatedEvent * flowEvent =
				(NMinusOneFlowDeallocatedEvent *) event;
		processFlowDeallocatedEvent(flowEvent);
	} else if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED) {
		NMinusOneFlowAllocatedEvent * flowEvent =
				(NMinusOneFlowAllocatedEvent *) event;
		processFlowAllocatedEvent(flowEvent);
	} else if (event->get_id() == IPCP_EVENT_NEIGHBOR_ADDED) {
		NeighborAddedEvent * neighEvent = (NeighborAddedEvent *) event;
		processNeighborAddedEvent(neighEvent);
	}
}

void LinkStatePDUFTGeneratorPolicy::processFlowDeallocatedEvent(
		NMinusOneFlowDeallocatedEvent * event)
{
	std::list<rina::FlowInformation>::iterator it;

	for (it = allocated_flows_.begin(); it != allocated_flows_.end(); ++it) {
		if (it->portId == event->port_id_) {
			allocated_flows_.erase(it);
			return;
		}
	}

	db_->deprecateObject(event->port_id_);
}

void LinkStatePDUFTGeneratorPolicy::processFlowAllocatedEvent(
		NMinusOneFlowAllocatedEvent * event)
{
	try {
		db_->addObjectToGroup(ipc_process_->get_address(),
				event->flow_information_.portId,
				ipc_process_->namespace_manager_->getAdressByname(
						event->flow_information_.remoteAppName), 1);
	} catch (Exception &e) {
		LOG_DBG("flow allocation waiting for enrollment");
		allocated_flows_.push_back(event->flow_information_);
	}
}

void LinkStatePDUFTGeneratorPolicy::processNeighborAddedEvent(
		NeighborAddedEvent * event)
{
	std::list<rina::FlowInformation>::iterator it;

	for (it = allocated_flows_.begin(); it != allocated_flows_.end(); ++it) {
		if (it->portId == event->neighbor_->underlying_port_id_)
				/*it->remoteAppName.processName.compare(
				event->neighbor_->get_name().processName) == 0) */{
			LOG_INFO(
					"There was an allocation flow event waiting for enrollment, launching it");
			try {
				db_->addObjectToGroup(ipc_process_->get_address(), it->portId,
						ipc_process_->namespace_manager_->getAdressByname(
								event->neighbor_->get_name()), 1);
				allocated_flows_.erase(it);
				break;
			} catch (Exception &e) {
				LOG_ERR("Could not allocate the flow, no neighbor found");
			}
		}
	}

	if (db_->isEmpty()) {
		return;
	}

	int portId = event->neighbor_->get_underlying_port_id();

	try {
		rina::RIBObjectValue robject_value;
		robject_value.type_ = rina::RIBObjectValue::complextype;
		robject_value.complex_value_ = &(db_->flow_state_objects_);

		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = portId;

		rib_daemon_->remoteWriteObject(
				EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_CLASS,
				EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_NAME,
				robject_value, 0, remote_id, 0);
		db_->setAvoidPort(portId);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message: %s", e.what());
	}
}

void LinkStatePDUFTGeneratorPolicy::propagateFSDB() const
{
	rina::ScopedLock g(*lock_);

	std::list<rina::FlowInformation> nMinusOneFlows =
			ipc_process_->resource_allocator_->get_n_minus_one_flow_manager()->getAllNMinusOneFlowInformation();

  std::map <int, std::list<FlowStateObject*> > objectsToSend =
			db_->prepareForPropagation(nMinusOneFlows);

	if (objectsToSend.size() == 0) {
		return;
	}

	for (std::map <int, std::list<FlowStateObject*> >::iterator it =
	    objectsToSend.begin(); it != objectsToSend.end(); ++it){
	  if (!it->second.empty()){
	    rina::RIBObjectValue robject_value;
	    robject_value.type_ = rina::RIBObjectValue::complextype;
	    robject_value.complex_value_ = &(it->second);
	    rina::RemoteProcessId remote_id;
      remote_id.port_id_ = it->first;
      try {
      rib_daemon_->remoteWriteObject(
          EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_CLASS,
          EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_NAME,
          robject_value, 0, remote_id, 0);
      } catch (Exception &e) {
        LOG_ERR("Errors sending message: %s", e.what());
      }
	  }
	}
}

void LinkStatePDUFTGeneratorPolicy::updateAge()
{
	rina::ScopedLock g(*lock_);

	db_->incrementAge();
}

void LinkStatePDUFTGeneratorPolicy::forwardingTableUpdate()
{
	rina::ScopedLock g(*lock_);

	if (!db_->modified_) {
		return;
	}

	db_->modified_ = false;
	std::list<FlowStateObject *> flow_state_objects;
	db_->getAllFSOs(flow_state_objects);
	std::list<rina::PDUForwardingTableEntry *> pduft =
			routing_algorithm_->computePDUTForwardingTable(flow_state_objects,
					source_vertex_);
	try {
		rina::kernelIPCProcess->modifyPDUForwardingTableEntries(pduft, 2);
	} catch (Exception & e) {
		LOG_ERR("Error setting PDU Forwarding Table in the kernel: %s",
				e.what());
	}
}

void LinkStatePDUFTGeneratorPolicy::writeMessageReceived(
		const std::list<FlowStateObject *> & flow_state_objects, int portId)
{
	rina::ScopedLock g(*lock_);

	try {
		db_->updateObjects(flow_state_objects, portId,
				ipc_process_->get_address());
	} catch (Exception &e) {
		LOG_ERR("Problems decoding Flow State Object Group: %s", e.what());
	}
}

void LinkStatePDUFTGeneratorPolicy::readMessageRecieved(int invoke_id,
		int portId) const
{
	rina::ScopedLock g(*lock_);

	try {
		rina::RIBObjectValue robject_value;
		robject_value.type_ = rina::RIBObjectValue::complextype;
		robject_value.complex_value_ = &(db_->flow_state_objects_);

		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = portId;

		rib_daemon_->remoteReadObjectResponse(fs_rib_group_->class_,
				fs_rib_group_->name_, robject_value, 0, "", invoke_id, false,
				remote_id);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message: %s", e.what());
	}
}

//Class FlowStateObjectEncoder
const rina::SerializedObject* FlowStateObjectEncoder::encode(const void* object)
{
	FlowStateObject * fso = (FlowStateObject*) object;
	rina::messages::flowStateObject_t gpb_fso;

	FlowStateObjectEncoder::convertModelToGPB(&gpb_fso, fso);

	int size = gpb_fso.ByteSize();
	char *serialized_message = new char[size];
	gpb_fso.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object = new rina::SerializedObject(
			serialized_message, size);

	return serialized_object;
}

void* FlowStateObjectEncoder::decode(
		const rina::ObjectValueInterface * object_value) const
{
	rina::messages::flowStateObject_t gpb_fso;

	rina::SerializedObject * serializedObject = Encoder::get_serialized_object(
			object_value);

	gpb_fso.ParseFromArray(serializedObject->message_, serializedObject->size_);

	return (void*) FlowStateObjectEncoder::convertGPBToModel(gpb_fso);
}

void FlowStateObjectEncoder::convertModelToGPB(
		rina::messages::flowStateObject_t * gpb_fso, FlowStateObject * fso)
{
	gpb_fso->set_address(fso->address_);
	gpb_fso->set_age(fso->age_);
	gpb_fso->set_neighbor_address(fso->neighbor_address_);
	gpb_fso->set_neighbor_portid(fso->neighbor_port_id_);
	gpb_fso->set_portid(fso->port_id_);
	gpb_fso->set_sequence_number(fso->sequence_number_);
	gpb_fso->set_state(fso->up_);

	return;
}

FlowStateObject * FlowStateObjectEncoder::convertGPBToModel(
		const rina::messages::flowStateObject_t & gpb_fso)
{
	FlowStateObject * fso = new FlowStateObject(gpb_fso.address(),
			gpb_fso.portid(), gpb_fso.neighbor_address(),
			gpb_fso.neighbor_portid(), gpb_fso.state(),
			gpb_fso.sequence_number(), gpb_fso.age());

	return fso;
}

// Class FlowStateObjectListEncoder
const rina::SerializedObject* FlowStateObjectListEncoder::encode(
		const void* object)
{
	std::list<FlowStateObject*> * list = (std::list<FlowStateObject*> *) object;
	std::list<FlowStateObject*>::const_iterator it;
	rina::messages::flowStateObjectGroup_t gpb_list;

	rina::messages::flowStateObject_t * gpb_fso;
	for (it = list->begin(); it != list->end(); ++it) {
		gpb_fso = gpb_list.add_flow_state_objects();
		FlowStateObjectEncoder::convertModelToGPB(gpb_fso, (*it));
	}

	int size = gpb_list.ByteSize();
	char *serialized_message = new char[size];
	gpb_list.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object = new rina::SerializedObject(
			serialized_message, size);

	return serialized_object;
}

void* FlowStateObjectListEncoder::decode(
		const rina::ObjectValueInterface * object_value) const
{
	rina::messages::flowStateObjectGroup_t gpb_list;

	rina::SerializedObject * serializedObject = Encoder::get_serialized_object(
			object_value);

	gpb_list.ParseFromArray(serializedObject->message_,
			serializedObject->size_);

	std::list<FlowStateObject*> * list = new std::list<FlowStateObject*>();

	for (int i = 0; i < gpb_list.flow_state_objects_size(); ++i) {
		list->push_back(
				FlowStateObjectEncoder::convertGPBToModel(
						gpb_list.flow_state_objects(i)));
	}

	return (void *) list;
}

}
