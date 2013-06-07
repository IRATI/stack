/*
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

#include "librina-common.h"
#define RINA_PREFIX "common"
#include "logs.h"

using namespace rina;

/* CLASS APPLICATION PROCESS NAMING INFORMATION */

ApplicationProcessNamingInformation::ApplicationProcessNamingInformation() {
}

ApplicationProcessNamingInformation::ApplicationProcessNamingInformation(
		const std::string & processName, const std::string & processInstance) {
	this->processName = processName;
	this->processInstance = processInstance;
}

bool ApplicationProcessNamingInformation::operator==(
		const ApplicationProcessNamingInformation &other) const {
	if (processName.compare(other.processName) != 0) {
		return false;
	}

	if (processInstance.compare(other.processInstance) != 0) {
		return false;
	}

	if (entityName.compare(other.entityName) != 0) {
		return false;
	}

	if (entityInstance.compare(other.entityInstance) != 0) {
		return false;
	}

	return true;
}

bool ApplicationProcessNamingInformation::operator!=(
		const ApplicationProcessNamingInformation &other) const {
	return !(*this == other);
}

bool ApplicationProcessNamingInformation::operator>(
		const ApplicationProcessNamingInformation &other) const {
	int aux = getProcessName().compare(other.getProcessName());
	if (aux > 0) {
		return true;
	} else if (aux < 0) {
		return false;
	}

	aux = getProcessInstance().compare(other.getProcessInstance());
	if (aux > 0) {
		return true;
	} else if (aux < 0) {
		return false;
	}

	aux = getEntityName().compare(other.getEntityName());
	if (aux > 0) {
		return true;
	} else if (aux < 0) {
		return false;
	}

	aux = getEntityInstance().compare(other.getEntityInstance());
	if (aux > 0) {
		return true;
	} else {
		return false;
	}
}

bool ApplicationProcessNamingInformation::operator<=(
		const ApplicationProcessNamingInformation &other) const {
	return !(*this > other);
}

bool ApplicationProcessNamingInformation::operator<(
		const ApplicationProcessNamingInformation &other) const {
	int aux = getProcessName().compare(other.getProcessName());
	if (aux < 0) {
		return true;
	} else if (aux > 0) {
		return false;
	}

	aux = getProcessInstance().compare(other.getProcessInstance());
	if (aux < 0) {
		return true;
	} else if (aux > 0) {
		return false;
	}

	aux = getEntityName().compare(other.getEntityName());
	if (aux < 0) {
		return true;
	} else if (aux > 0) {
		return false;
	}

	aux = getEntityInstance().compare(other.getEntityInstance());
	if (aux < 0) {
		return true;
	} else {
		return false;
	}
}

bool ApplicationProcessNamingInformation::operator>=(
		const ApplicationProcessNamingInformation &other) const {
	return !(*this < other);
}

const std::string & ApplicationProcessNamingInformation::getEntityInstance() const {
	return entityInstance;
}

void ApplicationProcessNamingInformation::setEntityInstance(
		const std::string & entityInstance) {
	this->entityInstance = entityInstance;
}

const std::string & ApplicationProcessNamingInformation::getEntityName() const {
	return entityName;
}

void ApplicationProcessNamingInformation::setEntityName(
		const std::string & entityName) {
	this->entityName = entityName;
}

const std::string & ApplicationProcessNamingInformation::getProcessInstance() const {
	return processInstance;
}

void ApplicationProcessNamingInformation::setProcessInstance(
		const std::string & processInstance) {
	this->processInstance = processInstance;
}

const std::string & ApplicationProcessNamingInformation::getProcessName() const {
	return processName;
}

void ApplicationProcessNamingInformation::setProcessName(
		const std::string & processName) {
	this->processName = processName;
}

/* CLASS FLOW SPECIFICATION */

FlowSpecification::FlowSpecification() {
	averageSDUBandwidth = 0;
	averageBandwidth = 0;
	peakBandwidthDuration = 0;
	peakSDUBandwidthDuration = 0;
	undetectedBitErrorRate = 0;
	partialDelivery = true;
	orderedDelivery = false;
	maxAllowableGap = -1;
	jitter = 0;
	delay = 0;
	maxSDUsize = 0;
}

unsigned int FlowSpecification::getAverageBandwidth() const {
	return averageBandwidth;
}

void FlowSpecification::setAverageBandwidth(unsigned int averageBandwidth) {
	this->averageBandwidth = averageBandwidth;
}

unsigned int FlowSpecification::getAverageSduBandwidth() const {
	return averageSDUBandwidth;
}

void FlowSpecification::setAverageSduBandwidth(
		unsigned int averageSduBandwidth) {
	averageSDUBandwidth = averageSduBandwidth;
}

unsigned int FlowSpecification::getDelay() const {
	return delay;
}

void FlowSpecification::setDelay(unsigned int delay) {
	this->delay = delay;
}

unsigned int FlowSpecification::getJitter() const {
	return jitter;
}

void FlowSpecification::setJitter(unsigned int jitter) {
	this->jitter = jitter;
}

int FlowSpecification::getMaxAllowableGap() const {
	return maxAllowableGap;
}

void FlowSpecification::setMaxAllowableGap(int maxAllowableGap) {
	this->maxAllowableGap = maxAllowableGap;
}

unsigned int FlowSpecification::getMaxSDUSize() const {
	return maxSDUsize;
}

void FlowSpecification::setMaxSDUSize(unsigned int maxSduSize) {
	maxSDUsize = maxSduSize;
}

bool FlowSpecification::isOrderedDelivery() const {
	return orderedDelivery;
}

void FlowSpecification::setOrderedDelivery(bool orderedDelivery) {
	this->orderedDelivery = orderedDelivery;
}

bool FlowSpecification::isPartialDelivery() const {
	return partialDelivery;
}

void FlowSpecification::setPartialDelivery(bool partialDelivery) {
	this->partialDelivery = partialDelivery;
}

unsigned int FlowSpecification::getPeakBandwidthDuration() const {
	return peakBandwidthDuration;
}

void FlowSpecification::setPeakBandwidthDuration(
		unsigned int peakBandwidthDuration) {
	this->peakBandwidthDuration = peakBandwidthDuration;
}

unsigned int FlowSpecification::getPeakSduBandwidthDuration() const {
	return peakSDUBandwidthDuration;
}

void FlowSpecification::setPeakSduBandwidthDuration(
		unsigned int peakSduBandwidthDuration) {
	peakSDUBandwidthDuration = peakSduBandwidthDuration;
}

double FlowSpecification::getUndetectedBitErrorRate() const {
	return undetectedBitErrorRate;
}

void FlowSpecification::setUndetectedBitErrorRate(
		double undetectedBitErrorRate) {
	this->undetectedBitErrorRate = undetectedBitErrorRate;
}

/* CLASS QoS CUBE */

QoSCube::QoSCube(const std::string& name, int id) {
	this->name = name;
	this->id = id;
	averageBandwidth = 0;
	averageSDUBandwidth = 0;
	peakBandwidthDuration = 0;
	peakSDUBandwidthDuration = 0;
	undetectedBitErrorRate = 0;
	partialDelivery = true;
	orderedDelivery = false;
	maxAllowableGap = -1;
	jitter = 0;
	delay = 0;
}

bool QoSCube::operator==(const QoSCube &other) const {
	return (this->id == other.id);
}

bool QoSCube::operator!=(const QoSCube &other) const {
	return !(*this == other);
}

const std::string& QoSCube::getName() const {
	return this->name;
}

unsigned int QoSCube::getAverageBandwidth() const {
	return averageBandwidth;
}

void QoSCube::setAverageBandwidth(unsigned int averageBandwidth) {
	this->averageBandwidth = averageBandwidth;
}

unsigned int QoSCube::getAverageSduBandwidth() const {
	return averageSDUBandwidth;
}

void QoSCube::setAverageSduBandwidth(unsigned int averageSduBandwidth) {
	this->averageSDUBandwidth = averageSduBandwidth;
}

unsigned int QoSCube::getDelay() const {
	return getDelay();
}

void QoSCube::setDelay(unsigned int delay) {
	this->delay = delay;
}

unsigned int QoSCube::getJitter() const {
	return jitter;
}

void QoSCube::setJitter(unsigned int jitter) {
	this->jitter = jitter;
}

int QoSCube::getMaxAllowableGap() const {
	return maxAllowableGap;
}

void QoSCube::setMaxAllowableGap(int maxAllowableGap) {
	this->maxAllowableGap = maxAllowableGap;
}

bool QoSCube::isOrderedDelivery() const {
	return orderedDelivery;
}

void QoSCube::setOrderedDelivery(bool orderedDelivery) {
	this->orderedDelivery = orderedDelivery;
}

bool QoSCube::isPartialDelivery() const {
	return partialDelivery;
}

void QoSCube::setPartialDelivery(bool partialDelivery) {
	this->partialDelivery = partialDelivery;
}

unsigned int QoSCube::getPeakBandwidthDuration() const {
	return peakBandwidthDuration;
}

void QoSCube::setPeakBandwidthDuration(unsigned int peakBandwidthDuration) {
	this->peakBandwidthDuration = peakBandwidthDuration;
}

unsigned int QoSCube::getPeakSduBandwidthDuration() const {
	return peakSDUBandwidthDuration;
}

void QoSCube::setPeakSduBandwidthDuration(
		unsigned int peakSduBandwidthDuration) {
	this->peakSDUBandwidthDuration = peakSduBandwidthDuration;
}

double QoSCube::getUndetectedBitErrorRate() const {
	return getUndetectedBitErrorRate();
}

void QoSCube::setUndetectedBitErrorRate(double undetectedBitErrorRate) {
	this->undetectedBitErrorRate = undetectedBitErrorRate;
}

/* CLASS DIF PROPERTIES */

DIFProperties::DIFProperties(const ApplicationProcessNamingInformation& DIFName,
		int maxSDUSize) {
	this->DIFName = DIFName;
	this->maxSDUSize = maxSDUSize;
}

const ApplicationProcessNamingInformation& DIFProperties::getDifName() const {
	return this->DIFName;
}

unsigned int DIFProperties::getMaxSduSize() const {
	return maxSDUSize;
}

const std::list<QoSCube>& DIFProperties::getQoSCubes() const {
	return qosCubes;
}

void DIFProperties::addQoSCube(const QoSCube& qosCube) {
	this->qosCubes.push_back(qosCube);
}

void DIFProperties::removeQoSCube(const QoSCube& qosCube) {
	this->qosCubes.remove(qosCube);
}

/* CLASS IPC EVENT */
IPCEvent::~IPCEvent() {
}

/* CLASS IPC EVENT PRODUCER */
void IPCEventProducer::enqueEvent(IPCEvent * event) {
	eventQueue.push_back(event);
}

IPCEvent * IPCEventProducer::eventPoll() {
	if (eventQueue.size() > 0) {
		IPCEvent * result = eventQueue.front();
		eventQueue.pop_front();
		return result;
	}

	return NULL;
}

IPCEvent * IPCEventProducer::eventWait() {
	if (eventQueue.size() > 0) {
		IPCEvent * result = eventQueue.front();
		eventQueue.pop_front();
		return result;
	}

	return NULL;
}

/* CLASS IPC EXCEPTION */

IPCException::IPCException(const std::string& description) :
		Exception(description) {
}

const std::string IPCException::operation_not_implemented_error =
		"This operation is not yet implemented";

/** CLASS DIF CONFIGURATION */

const ApplicationProcessNamingInformation& DIFConfiguration::getDifName() const {
	return difName;
}

void DIFConfiguration::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

DIFType DIFConfiguration::getDifType() const {
	return difType;
}

void DIFConfiguration::setDifType(DIFType difType) {
	this->difType = difType;
}

const std::vector<Policy>& DIFConfiguration::getPolicies() {
	return policies;
}

void DIFConfiguration::setPolicies(const std::vector<Policy>& policies) {
	this->policies = policies;
}

const std::vector<QoSCube>& DIFConfiguration::getQosCubes() const {
	return qosCubes;
}

void DIFConfiguration::setQosCubes(const std::vector<QoSCube>& qosCubes) {
	this->qosCubes = qosCubes;
}

/* CLASS FLOW REQUEST */
const ApplicationProcessNamingInformation& FlowRequest::getDestinationApplicationName() const {
	return destinationApplicationName;
}

void FlowRequest::setDestinationApplicationName(
		const ApplicationProcessNamingInformation& destinationApplicationName) {
	this->destinationApplicationName = destinationApplicationName;
}

const FlowSpecification& FlowRequest::getFlowSpecification() const {
	return flowSpecification;
}

void FlowRequest::setFlowSpecification(
		const FlowSpecification& flowSpecification) {
	this->flowSpecification = flowSpecification;
}

int FlowRequest::getPortId() const {
	return portId;
}

void FlowRequest::setPortId(int portId) {
	this->portId = portId;
}

const ApplicationProcessNamingInformation& FlowRequest::getSourceApplicationName() const {
	return sourceApplicationName;
}

void FlowRequest::setSourceApplicationName(
		const ApplicationProcessNamingInformation& sourceApplicationName) {
	this->sourceApplicationName = sourceApplicationName;
}
