//
// Flow Allocator
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

#include "flow-allocator.h"

namespace rinad {

//	CLASS Flow
const std::string Flow::FLOW_SET_RIB_OBJECT_NAME = RIBObjectNames::SEPARATOR +
	RIBObjectNames::DIF + RIBObjectNames::SEPARATOR + RIBObjectNames::RESOURCE_ALLOCATION
	+ RIBObjectNames::SEPARATOR + RIBObjectNames::FLOW_ALLOCATOR + RIBObjectNames::SEPARATOR
	+ RIBObjectNames::FLOWS;
const std::string Flow::FLOW_SET_RIB_OBJECT_CLASS = "flow set";
const std::string Flow::FLOW_RIB_OBJECT_CLASS = "flow";

Flow::Flow() {
	sourcePortId = 0;
	destinationPortId = 0;
	sourceAddress = 0;
	destinationAddress = 0;
	currentConnectionIndex = 0;
	maxCreateFlowRetries = 0;
	createFlowRetries = 0;
	hopCount = 0;
	source = false;
	state = EMPTY;
	accessControl = 0;
}

bool Flow::isSource() const {
	return source;
}

void Flow::setSource(bool source) {
	this->source = source;
}

const rina::ApplicationProcessNamingInformation& Flow::getSourceNamingInfo() const {
	return sourceNamingInfo;
}

void Flow::setSourceNamingInfo(const rina::ApplicationProcessNamingInformation &sourceNamingInfo) {
	this->sourceNamingInfo = sourceNamingInfo;
}

const rina::ApplicationProcessNamingInformation& Flow::getDestinationNamingInfo() const {
	return destinationNamingInfo;
}

void Flow::setDestinationNamingInfo(const rina::ApplicationProcessNamingInformation &destinationNamingInfo) {
	this->destinationNamingInfo = destinationNamingInfo;
}

int Flow::getSourcePortId() const {
	return sourcePortId;
}

void Flow::setSourcePortId(int sourcePortId) {
	this->sourcePortId = sourcePortId;
}

int Flow::getDestinationPortId() const {
	return destinationPortId;
}

void Flow::setDestinationPortId(int destinationPortId) {
	this->destinationPortId = destinationPortId;
}

long Flow::getSourceAddress() const {
	return sourceAddress;
}

void Flow::setSourceAddress(long sourceAddress) {
	this->sourceAddress = sourceAddress;
}

long Flow::getDestinationAddress() const {
	return destinationAddress;
}

void Flow::setDestinationAddress(long destinationAddress) {
	this->destinationAddress = destinationAddress;
}

const std::list<rina::Connection>& Flow::getConnections() const {
	return connections;
}

void Flow::setConnections(const std::list<rina::Connection> &connections) {
	this->connections = connections;
}

int Flow::getCurrentConnectionIndex() const {
	return currentConnectionIndex;
}

void Flow::setCurrentConnectionIndex(int currentConnectionIndex) {
	this->currentConnectionIndex = currentConnectionIndex;
}

Flow::IPCPFlowState Flow::getState() const{
	return state;
}

void Flow::setState(IPCPFlowState state) {
	this->state = state;
}

const rina::FlowSpecification& Flow::getFlowSpecification() const {
	return flowSpec;
}

void Flow::setFlowSpecification(const rina::FlowSpecification &flowSpec) {
	this->flowSpec = flowSpec;
}

const std::map<std::string, std::string>& Flow::getPolicies() const {
	return policies;
}

void Flow::setPolicies(const std::map<std::string, std::string> &policies) {
	this->policies = policies;
}

const std::map<std::string, std::string>& Flow::getPolicyParameters() const {
	return policyParameters;
}

void Flow::setPolicyParameters(const std::map<std::string, std::string> &policyParameters) {
	this->policyParameters = policyParameters;
}

char* Flow::getAccessControl() const {
	return accessControl;
}

void Flow::setAccessControl(char* accessControl) {
	this->accessControl = accessControl;
}

int Flow::getMaxCreateFlowRetries() const {
	return maxCreateFlowRetries;
}

void Flow::setMaxCreateFlowRetries(int maxCreateFlowRetries) {
	this->maxCreateFlowRetries = maxCreateFlowRetries;
}

int Flow::getCreateFlowRetries() const {
	return createFlowRetries;
}

void Flow::setCreateFlowRetries(int createFlowRetries) {
	this->createFlowRetries = createFlowRetries;
}

int Flow::getHopCount() const {
	return hopCount;
}

void Flow::setHopCount(int hopCount) {
	this->hopCount = hopCount;
}

std::string Flow::toString() {
    std::stringstream ss;
    ss << "* State: " << this->state << std::endl;
    ss << "* Is this IPC Process the requestor of the flow? " << this->source << std::endl;
    ss << "* Max create flow retries: " << this->maxCreateFlowRetries << std::endl;
    ss << "* Hop count: " << this->hopCount << std::endl;
    ss << "* Source AP Naming Info: " << this->sourceNamingInfo.toString() << std::endl;;
    ss << "* Source address: " << this->sourceAddress << std::endl;
    ss << "* Source port id: " << this->sourcePortId << std::endl;
    ss <<  "* Destination AP Naming Info: " << this->destinationNamingInfo.toString();
    ss <<  "* Destination addres: " + this->destinationAddress << std::endl;
    ss << "* Destination port id: "+ this->destinationPortId << std::endl;
    if (connections.size() > 0) {
		ss << "* Connection ids of the connection supporting this flow: +\n";
		for(std::list<rina::Connection>::const_iterator iterator = connections.begin(), end = connections.end(); iterator != end; ++iterator) {
			ss << "Src CEP-id " << iterator->getSourceCepId()
					<< "; Dest CEP-id " << iterator->getDestCepId()
					<< "; Qos-id " << iterator->getQosId() << std::endl;
		}
	}
	ss << "* Index of the current active connection for this flow: " << this->currentConnectionIndex << std::endl;
	if (!this->policies.empty()) {
		ss << "* Policies: " << std::endl;
		for (std::map<std::string, std::string>::const_iterator iterator = policies.begin(), end = policies.end();
				iterator != end; ++iterator)
		{
			ss << "   * " << iterator->first << " = " << iterator->second << std::endl;
		}

	}
	if (!this->policyParameters.empty()) {
		ss << "* Policy parameters: " << std::endl;
		for (std::map<std::string, std::string>::const_iterator iterator = policyParameters.begin(), end = policyParameters.end();
				iterator != end; ++iterator)
		{
			ss << "   * " + iterator->first << " = " << iterator->second << std::endl;
		}
	}
	return ss.str();
}

}
