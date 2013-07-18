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

#define RINA_PREFIX "common"

#include "logs.h"
#include "config.h"
#include "librina-common.h"
#include "core.h"

namespace rina {

std::string getVersion() {
	return VERSION;
}

/* CLASS APPLICATION PROCESS NAMING INFORMATION */

ApplicationProcessNamingInformation::ApplicationProcessNamingInformation() {
}

ApplicationProcessNamingInformation::ApplicationProcessNamingInformation(
		const std::string & processName,
		const std::string & processInstance) {
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

const std::string & ApplicationProcessNamingInformation::getEntityInstance()
		const {
	return entityInstance;
}

void ApplicationProcessNamingInformation::setEntityInstance(
		const std::string & entityInstance) {
	this->entityInstance = entityInstance;
}

const std::string & ApplicationProcessNamingInformation::getEntityName()
		const {
	return entityName;
}

void ApplicationProcessNamingInformation::setEntityName(
		const std::string & entityName) {
	this->entityName = entityName;
}

const std::string & ApplicationProcessNamingInformation::getProcessInstance()
		const {
	return processInstance;
}

void ApplicationProcessNamingInformation::setProcessInstance(
		const std::string & processInstance) {
	this->processInstance = processInstance;
}

const std::string & ApplicationProcessNamingInformation::getProcessName()
		const {
	return processName;
}

void ApplicationProcessNamingInformation::setProcessName(
		const std::string & processName) {
	this->processName = processName;
}

std::string ApplicationProcessNamingInformation::toString() {
	return "Process name: " + processName + "; Process instance: "
			+ processInstance + "; Entity name: " + entityName
			+ "; Entity instance: " + entityInstance;
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

bool FlowSpecification::operator==(const FlowSpecification &other) const {
	if (averageBandwidth != other.getAverageBandwidth()) {
		return false;
	}

	if (averageSDUBandwidth != other.getAverageSduBandwidth()) {
		return false;
	}

	if (peakBandwidthDuration != other.getPeakBandwidthDuration()) {
		return false;
	}

	if (peakSDUBandwidthDuration != other.getPeakSduBandwidthDuration()) {
		return false;
	}

	if (undetectedBitErrorRate != other.getUndetectedBitErrorRate()) {
		return false;
	}

	if (partialDelivery != other.isPartialDelivery()) {
		return false;
	}

	if (orderedDelivery != other.isOrderedDelivery()) {
		return false;
	}

	if (maxAllowableGap != other.getMaxAllowableGap()) {
		return false;
	}

	if (delay != other.getDelay()) {
		return false;
	}

	if (jitter != other.getJitter()) {
		return false;
	}

	if (maxSDUsize != other.getMaxSDUSize()) {
		return false;
	}

	return true;
}

bool FlowSpecification::operator!=(const FlowSpecification &other) const {
	return !(*this == other);
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

int QoSCube::getId() const{
	return id;
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

DIFProperties::DIFProperties(
		const ApplicationProcessNamingInformation& DIFName, int maxSDUSize) {
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

/* CLASS FLOW REQUEST EVENT */
FlowRequestEvent::FlowRequestEvent(
		const FlowSpecification& flowSpecification,
		const ApplicationProcessNamingInformation& localApplicationName,
		const ApplicationProcessNamingInformation& remoteApplicationName,
		unsigned int sequenceNumber):
				IPCEvent(FLOW_ALLOCATION_REQUESTED_EVENT,
						sequenceNumber) {
	this->flowSpecification = flowSpecification;
	this->localApplicationName = localApplicationName;
	this->remoteApplicationName = remoteApplicationName;
	this->portId = 0;
}

FlowRequestEvent::FlowRequestEvent(int portId,
		const FlowSpecification& flowSpecification,
		const ApplicationProcessNamingInformation& localApplicationName,
		const ApplicationProcessNamingInformation& destApplicationName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber) :
		IPCEvent(FLOW_ALLOCATION_REQUESTED_EVENT,
				sequenceNumber) {
	this->flowSpecification = flowSpecification;
	this->localApplicationName = localApplicationName;
	this->remoteApplicationName = remoteApplicationName;
	this->DIFName = DIFName;
	this->portId = portId;
}

void FlowRequestEvent::setPortId(int portId){
	this->portId = portId;
}

void FlowRequestEvent::setDIFName(
		const ApplicationProcessNamingInformation& difName){
	this->DIFName = difName;
}

int FlowRequestEvent::getPortId() const {
	return portId;
}

const FlowSpecification& FlowRequestEvent::getFlowSpecification() const {
	return flowSpecification;
}

const ApplicationProcessNamingInformation&
	FlowRequestEvent::getDIFName() const {
	return DIFName;
}

const ApplicationProcessNamingInformation&
	FlowRequestEvent::getLocalApplicationName() const {
	return localApplicationName;
}

const ApplicationProcessNamingInformation&
	FlowRequestEvent::getRemoteApplicationName() const {
	return remoteApplicationName;
}

/* CLASS APPLICATION REGISTRATION REQUEST */
ApplicationRegistrationRequestEvent::ApplicationRegistrationRequestEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber) :
		IPCEvent(APPLICATION_REGISTRATION_REQUEST_EVENT,
				sequenceNumber) {
	this->applicationName = appName;
	this->DIFName = DIFName;
}

const ApplicationProcessNamingInformation&
ApplicationRegistrationRequestEvent::getApplicationName() const {
	return applicationName;
}

const ApplicationProcessNamingInformation&
ApplicationRegistrationRequestEvent::getDIFName() const {
	return DIFName;
}

/* CLASS APPLICATION UNREGISTRATION REQUEST */
ApplicationUnregistrationRequestEvent::ApplicationUnregistrationRequestEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber) :
		IPCEvent(APPLICATION_UNREGISTRATION_REQUEST_EVENT,
				sequenceNumber) {
	this->applicationName = appName;
	this->DIFName = DIFName;
}

const ApplicationProcessNamingInformation&
ApplicationUnregistrationRequestEvent::getApplicationName() const {
	return applicationName;
}

const ApplicationProcessNamingInformation&
ApplicationUnregistrationRequestEvent::getDIFName() const {
	return DIFName;
}


/* CLASS IPC EVENT PRODUCER */

/* Auxiliar function called in case of using the stubbed version of the API */
IPCEvent * getIPCEvent(){
	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation();
	sourceName->setProcessName("/apps/source");
	sourceName->setProcessInstance("12");
	sourceName->setEntityName("database");
	sourceName->setEntityInstance("12");

	ApplicationProcessNamingInformation * destName =
			new ApplicationProcessNamingInformation();
	destName->setProcessName("/apps/dest");
	destName->setProcessInstance("12345");
	destName->setEntityName("printer");
	destName->setEntityInstance("12623456");

	FlowSpecification * flowSpec = new FlowSpecification();

	FlowRequestEvent * event = new
			FlowRequestEvent(*flowSpec,*sourceName, *destName, 24);

	return event;
}

IPCEvent * IPCEventProducer::eventPoll() {
#if STUB_API
	return getIPCEvent();
#else
	return rinaManager->getEventQueue()->poll();
#endif
}

IPCEvent * IPCEventProducer::eventWait() {
#if STUB_API
	return getIPCEvent();
#else
	return rinaManager->getEventQueue()->take();
#endif
}

IPCEvent * IPCEventProducer::eventTimedWait(
		int seconds, int nanoseconds){
#if STUB_API
	return getIPCEvent();
#else
	return rinaManager->getEventQueue()->timedtake(seconds, nanoseconds);
#endif
}

Singleton<IPCEventProducer> ipcEventProducer;

/* CLASS IPC EXCEPTION */

IPCException::IPCException(const std::string& description) :
		Exception(description) {
}

const std::string IPCException::operation_not_implemented_error =
		"This operation is not yet implemented";

/** CLASS DIF CONFIGURATION */

const ApplicationProcessNamingInformation& DIFConfiguration::getDifName()
		const {
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
const ApplicationProcessNamingInformation&
		FlowRequest::getDestinationApplicationName() const {
	return destinationApplicationName;
}

void FlowRequest::setDestinationApplicationName(
		const ApplicationProcessNamingInformation& destinationApplicationName){
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

const ApplicationProcessNamingInformation& FlowRequest::getSourceApplicationName()
		const {
	return sourceApplicationName;
}

void FlowRequest::setSourceApplicationName(
		const ApplicationProcessNamingInformation& sourceApplicationName) {
	this->sourceApplicationName = sourceApplicationName;
}

/* CLAS RIBOBJECT */
RIBObject::RIBObject(){
	instance = generateObjectInstance();
}

RIBObject::RIBObject(
		std::string clazz, std::string name, RIBObjectValue value){
	this->clazz = clazz;
	this->name = name;
	this->value = value;
	instance = generateObjectInstance();
}

bool RIBObject::operator==(const RIBObject &other) const{
	if (clazz.compare(other.getClazz()) != 0) {
		return false;
	}

	if (name.compare(other.getName()) != 0) {
		return false;
	}

	return instance == other.getInstance();
}

bool RIBObject::operator!=(const RIBObject &other) const{
	return !(*this == other);
}

long RIBObject::generateObjectInstance(){
	//TODO generate instance properly
	return 0;
}

const std::string& RIBObject::getClazz() const {
	return clazz;
}

void RIBObject::setClazz(const std::string& clazz) {
	this->clazz = clazz;
}

long RIBObject::getInstance() const {
	return instance;
}

void RIBObject::setInstance(long instance) {
	this->instance = instance;
}

const std::string& RIBObject::getName() const {
	return name;
}

void RIBObject::setName(const std::string& name) {
	this->name = name;
}

RIBObjectValue RIBObject::getValue() const {
	return value;
}

void RIBObject::setValue(RIBObjectValue value) {
	this->value = value;
}

}
