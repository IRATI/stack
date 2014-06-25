//
// Common
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#include <ostream>
#include <sstream>

#define RINA_PREFIX "common"

#include "librina/logs.h"
#include "config.h"
#include "core.h"

#include "librina/common.h"

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

ApplicationProcessNamingInformation &
ApplicationProcessNamingInformation::operator=(
		const ApplicationProcessNamingInformation & other){
	if (this != &other){
		processName = other.getProcessName();
		processInstance = other.getProcessInstance();
		entityName = other.getEntityName();
		entityInstance = other.getEntityInstance();
	}

	return *this;
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

std::string ApplicationProcessNamingInformation::
getProcessNamePlusInstance(){
	return processName + "-" + processInstance;
}

std::string ApplicationProcessNamingInformation::getEncodedString() {
        return processName + "-" + processInstance +
                        "-" + entityName + "-" + entityInstance;
}

const std::string ApplicationProcessNamingInformation::toString() const{
        std::stringstream ss;

        ss<<"Process name: "<<processName;
        ss<<"; Process instance: "<<processInstance<<std::endl;
        ss<<"Entity name: "<<entityName;
        ss<<"; Entity instance: "<<entityInstance;

        return ss.str();
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

const std::string FlowSpecification::toString() {
        std::stringstream ss;
        ss<<"Jitter: "<<jitter<<"; Delay: "<<delay<<std::endl;
        ss<<"In oder delivery: "<<orderedDelivery;
        ss<<"; Partial delivery allowed: "<<partialDelivery<<std::endl;
        ss<<"Max allowed gap between SDUs: "<<maxAllowableGap;
        ss<<"; Undetected bit error rate: "<<undetectedBitErrorRate<<std::endl;
        ss<<"Average bandwidth (bytes/s): "<<averageBandwidth;
        ss<<"; Average SDU bandwidth (bytes/s): "<<averageSDUBandwidth<<std::endl;
        ss<<"Peak bandwidth duration (ms): "<<peakBandwidthDuration;
        ss<<"; Peak SDU bandwidth duration (ms): "<<peakSDUBandwidthDuration;
        return ss.str();
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

/* CLASS FLOW INFORMATION */
bool FlowInformation::operator==(
		const FlowInformation &other) const {
	return getPortId() == other.getPortId();
}

bool FlowInformation::operator!=(
		const FlowInformation &other) const {
	return !(*this == other);
}

const ApplicationProcessNamingInformation&
FlowInformation::getDifName() const {
	return difName;
}

void FlowInformation::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

const FlowSpecification& FlowInformation::getFlowSpecification() const {
	return flowSpecification;
}

void FlowInformation::setFlowSpecification(
		const FlowSpecification& flowSpecification) {
	this->flowSpecification = flowSpecification;
}

const ApplicationProcessNamingInformation&
FlowInformation::getLocalAppName() const {
	return localAppName;
}

void FlowInformation::setLocalAppName(
		const ApplicationProcessNamingInformation& localAppName) {
	this->localAppName = localAppName;
}

int FlowInformation::getPortId() const {
	return portId;
}

void FlowInformation::setPortId(int portId) {
	this->portId = portId;
}

const ApplicationProcessNamingInformation&
FlowInformation::getRemoteAppName() const {
	return remoteAppName;
}

void FlowInformation::setRemoteAppName(
		const ApplicationProcessNamingInformation& remoteAppName) {
	this->remoteAppName = remoteAppName;
}

const std::string FlowInformation::toString(){
        std::stringstream ss;

        ss<<"Local app name: "<<localAppName.toString()<<std::endl;
        ss<<"Remote app name: "<<remoteAppName.toString()<<std::endl;
        ss<<"DIF name: "<<difName.getProcessName();
        ss<<"; Port-id: "<<portId<<std::endl;
        ss<<"Flow specification: "<<flowSpecification.toString();

        return ss.str();
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

/* CLASS BASE RESPONSE EVENT */
BaseResponseEvent::BaseResponseEvent(
                        int result,
                        IPCEventType eventType,
                        unsigned int sequenceNumber) :
                              IPCEvent(eventType,
                                             sequenceNumber){
        this->result = result;
}


int BaseResponseEvent::getResult() const {
        return result;
}

/* CLASS FLOW REQUEST EVENT */
FlowRequestEvent::FlowRequestEvent(
		const FlowSpecification& flowSpecification,
		bool localRequest,
		const ApplicationProcessNamingInformation& localApplicationName,
		const ApplicationProcessNamingInformation& remoteApplicationName,
		int flowRequestorIpcProcessId,
		unsigned int sequenceNumber):
				IPCEvent(FLOW_ALLOCATION_REQUESTED_EVENT,
						sequenceNumber) {
	this->flowSpecification = flowSpecification;
	this->localRequest = localRequest;
	this->localApplicationName = localApplicationName;
	this->remoteApplicationName = remoteApplicationName;
	this->flowRequestorIpcProcessId = flowRequestorIpcProcessId;
	this->portId = 0;
	this->ipcProcessId = 0;
}

FlowRequestEvent::FlowRequestEvent(int portId,
		const FlowSpecification& flowSpecification,
		bool localRequest,
		const ApplicationProcessNamingInformation& localApplicationName,
		const ApplicationProcessNamingInformation& remoteApplicationName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned short ipcProcessId,
		unsigned int sequenceNumber) :
		IPCEvent(FLOW_ALLOCATION_REQUESTED_EVENT,
				sequenceNumber) {
	this->flowSpecification = flowSpecification;
	this->localRequest = localRequest;
	this->localApplicationName = localApplicationName;
	this->remoteApplicationName = remoteApplicationName;
	this->DIFName = DIFName;
	this->flowRequestorIpcProcessId = ipcProcessId;
	this->portId = portId;
	this->ipcProcessId = ipcProcessId;
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

bool FlowRequestEvent::isLocalRequest() const{
	return localRequest;
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

int FlowRequestEvent::getFlowRequestorIPCProcessId() const {
        return flowRequestorIpcProcessId;
}

unsigned short FlowRequestEvent::getIPCProcessId() const {
        return ipcProcessId;
}

/* CLASS FLOW DEALLOCATE REQUEST EVENT */
FlowDeallocateRequestEvent::FlowDeallocateRequestEvent(int portId,
			const ApplicationProcessNamingInformation& appName,
			unsigned int sequenceNumber):
						IPCEvent(FLOW_DEALLOCATION_REQUESTED_EVENT,
								sequenceNumber){
	this->portId = portId;
	this->applicationName = appName;
}

FlowDeallocateRequestEvent::FlowDeallocateRequestEvent(int portId,
		unsigned int sequenceNumber):
			IPCEvent(FLOW_DEALLOCATION_REQUESTED_EVENT,
					sequenceNumber){
	this->portId = portId;
}

int FlowDeallocateRequestEvent::getPortId() const{
	return portId;
}

const ApplicationProcessNamingInformation&
	FlowDeallocateRequestEvent::getApplicationName() const{
	return applicationName;
}

/* CLASS FLOW DEALLOCATED EVENT */
FlowDeallocatedEvent::FlowDeallocatedEvent(
		int portId, int code) :
				IPCEvent(FLOW_DEALLOCATED_EVENT, 0) {
	this->portId = portId;
	this->code = code;
}

int FlowDeallocatedEvent::getPortId() const {
	return portId;
}

int FlowDeallocatedEvent::getCode() const{
	return code;
}

/* CLASS APPLICATION REGISTRATION INFORMATION */
ApplicationRegistrationInformation::ApplicationRegistrationInformation(){
	applicationRegistrationType = APPLICATION_REGISTRATION_ANY_DIF;
	ipcProcessId = 0;
}

ApplicationRegistrationInformation::ApplicationRegistrationInformation(
		ApplicationRegistrationType applicationRegistrationType){
	this->applicationRegistrationType = applicationRegistrationType;
	ipcProcessId = 0;
}

ApplicationRegistrationType
ApplicationRegistrationInformation::getRegistrationType() const{
	return applicationRegistrationType;
}

const ApplicationProcessNamingInformation &
ApplicationRegistrationInformation::getDIFName() const{
	return difName;
}

void ApplicationRegistrationInformation::setDIFName(
		const ApplicationProcessNamingInformation& difName){
	this->difName = difName;
}

const ApplicationProcessNamingInformation&
        ApplicationRegistrationInformation::getApplicationName() const {
        return appName;
}

void ApplicationRegistrationInformation::setApplicationName(
                const ApplicationProcessNamingInformation& appName) {
        this->appName = appName;
}

unsigned short ApplicationRegistrationInformation::getIpcProcessId() const {
        return ipcProcessId;
}

void ApplicationRegistrationInformation::setIpcProcessId(
                unsigned short ipcProcessId) {
        this->ipcProcessId = ipcProcessId;
}

const std::string ApplicationRegistrationInformation::toString(){
        std::stringstream ss;

        ss<<"Application name: "<<appName.toString()<<std::endl;
        ss<<"DIF name: "<<difName.getProcessName();
        ss<<"; IPC Process id: "<<ipcProcessId;

        return ss.str();
}

/* CLASS APPLICATION REGISTRATION REQUEST */
ApplicationRegistrationRequestEvent::ApplicationRegistrationRequestEvent(
	const ApplicationRegistrationInformation&
	applicationRegistrationInformation, unsigned int sequenceNumber) :
		IPCEvent(APPLICATION_REGISTRATION_REQUEST_EVENT,
				sequenceNumber) {
	this->applicationRegistrationInformation =
			applicationRegistrationInformation;
}

const ApplicationRegistrationInformation&
ApplicationRegistrationRequestEvent::getApplicationRegistrationInformation()
const {
	return applicationRegistrationInformation;
}

/* CLASS BASE APPLICATION REGISTRATION EVENT */
BaseApplicationRegistrationEvent::BaseApplicationRegistrationEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& DIFName,
                        IPCEventType eventType,
                        unsigned int sequenceNumber):
                                IPCEvent(eventType, sequenceNumber) {
        this->applicationName = appName;
        this->DIFName = DIFName;
}

BaseApplicationRegistrationEvent::BaseApplicationRegistrationEvent(
                        const ApplicationProcessNamingInformation& appName,
                        IPCEventType eventType,
                        unsigned int sequenceNumber):
                                IPCEvent(eventType, sequenceNumber) {
        this->applicationName = appName;
}

const ApplicationProcessNamingInformation&
BaseApplicationRegistrationEvent::getApplicationName() const {
        return applicationName;
}

const ApplicationProcessNamingInformation&
BaseApplicationRegistrationEvent::getDIFName() const {
        return DIFName;
}

/* CLASS APPLICATION UNREGISTRATION REQUEST EVENT */
ApplicationUnregistrationRequestEvent::ApplicationUnregistrationRequestEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber) :
                BaseApplicationRegistrationEvent(
                                appName, DIFName,
                                APPLICATION_UNREGISTRATION_REQUEST_EVENT,
				sequenceNumber) {
}

/* CLASS BASE APPLICATION RESPONSE EVENT */
BaseApplicationRegistrationResponseEvent::
        BaseApplicationRegistrationResponseEvent(
                const ApplicationProcessNamingInformation& appName,
                const ApplicationProcessNamingInformation& DIFName,
                int result,
                IPCEventType eventType,
                unsigned int sequenceNumber) :
                BaseApplicationRegistrationEvent (
                                appName, DIFName,
                                eventType, sequenceNumber){
        this->result = result;
}

BaseApplicationRegistrationResponseEvent::
        BaseApplicationRegistrationResponseEvent(
                const ApplicationProcessNamingInformation& appName,
                int result,
                IPCEventType eventType,
                unsigned int sequenceNumber) :
                BaseApplicationRegistrationEvent (
                                appName,
                                eventType, sequenceNumber){
        this->result = result;
}

int BaseApplicationRegistrationResponseEvent::getResult() const{
        return result;
}

/* CLASS REGISTER APPLICATION RESPONSE EVENT */
RegisterApplicationResponseEvent::RegisterApplicationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& difName,
                        int result,
                        unsigned int sequenceNumber):
                BaseApplicationRegistrationResponseEvent(
                                       appName, difName, result,
                                       REGISTER_APPLICATION_RESPONSE_EVENT,
                                       sequenceNumber){
}

/* CLASS UNREGISTER APPLICATION RESPONSE EVENT */
UnregisterApplicationResponseEvent::UnregisterApplicationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        int result,
                        unsigned int sequenceNumber):
                BaseApplicationRegistrationResponseEvent(
                                       appName, result,
                                       UNREGISTER_APPLICATION_RESPONSE_EVENT,
                                       sequenceNumber){
}

/* CLASS ALLOCATE FLOW RESPONSE EVENT */
AllocateFlowResponseEvent::AllocateFlowResponseEvent(
                int result,
                bool notifySource,
                int flowAcceptorIpcProcessId,
                unsigned int sequenceNumber) :
        BaseResponseEvent(result,
                          ALLOCATE_FLOW_RESPONSE_EVENT,
                          sequenceNumber)
{
        this->notifySource             = notifySource;
        this->flowAcceptorIpcProcessId = flowAcceptorIpcProcessId;
}

bool AllocateFlowResponseEvent::isNotifySource() const {
        return notifySource;
}

int AllocateFlowResponseEvent::getFlowAcceptorIpcProcessId() const {
        return flowAcceptorIpcProcessId;
}

/* CLASS OS PROCESS FINALIZED EVENT */
OSProcessFinalizedEvent::OSProcessFinalizedEvent(
		const ApplicationProcessNamingInformation& appName,
		unsigned int ipcProcessId,
		unsigned int sequenceNumber) :
		IPCEvent(OS_PROCESS_FINALIZED,
				sequenceNumber) {
	this->applicationName = appName;
	this->ipcProcessId = ipcProcessId;
}

const ApplicationProcessNamingInformation&
OSProcessFinalizedEvent::getApplicationName() const {
	return applicationName;
}

unsigned int OSProcessFinalizedEvent::getIPCProcessId() const {
	return ipcProcessId;
}


/* CLASS IPC EVENT PRODUCER */

/* Auxiliar function called in case of using the stubbed version of the API */
IPCEvent * getIPCEvent(){
	ApplicationProcessNamingInformation sourceName;
	sourceName.setProcessName("/apps/source");
	sourceName.setProcessInstance("12");
	sourceName.setEntityName("database");
	sourceName.setEntityInstance("12");

	ApplicationProcessNamingInformation destName;
	destName.setProcessName("/apps/dest");
	destName.setProcessInstance("12345");
	destName.setEntityName("printer");
	destName.setEntityInstance("12623456");

	FlowSpecification flowSpec;

	FlowRequestEvent * event = new
			FlowRequestEvent(flowSpec, true, sourceName,
			                destName, 0, 24);

	return event;
}

IPCEvent * IPCEventProducer::eventPoll()
{
#if STUB_API
	return getIPCEvent();
#else
	return rinaManager->getEventQueue()->poll();
#endif
}

IPCEvent * IPCEventProducer::eventWait()
{
#if STUB_API
	return getIPCEvent();
#else
	return rinaManager->getEventQueue()->take();
#endif
}

IPCEvent * IPCEventProducer::eventTimedWait(int seconds,
                                            int nanoseconds)
{
#if STUB_API
        (void) seconds;
        (void) nanoseconds;

	return getIPCEvent();
#else
	return rinaManager->getEventQueue()->timedtake(seconds, nanoseconds);
#endif
}

Singleton<IPCEventProducer> ipcEventProducer;

/* CLASS IPC EXCEPTION */

IPCException::IPCException(const std::string& description) :
        Exception(description.c_str())
{ }

const std::string IPCException::operation_not_implemented_error =
		"This operation is not yet implemented";

/* CLASS PARAMETER */
Parameter::Parameter(){
}

Parameter::Parameter(const std::string & name, const std::string & value){
	this->name = name;
	this->value = value;
}

bool Parameter::operator==(const Parameter &other) const {
	if (this->name.compare(other.getName()) == 0 &&
			this->value.compare(other.getValue()) == 0)
		return true;

	return false;
}

bool Parameter::operator!=(const Parameter &other) const {
	return !(*this == other);
}

const std::string& Parameter::getName() const {
	return name;
}

void Parameter::setName(const std::string& name) {
	this->name = name;
}

const std::string& Parameter::getValue() const {
	return value;
}

void Parameter::setValue(const std::string& value) {
	this->value = value;
}

/* INITIALIZATION OPERATIONS */

bool librinaInitialized = false;
Lockable librinaInitializationLock;

void initialize(unsigned int localPort, const std::string& logLevel,
                const std::string& pathToLogFile)
        throw(InitializationException) {

        librinaInitializationLock.lock();
        if (librinaInitialized) {
                librinaInitializationLock.unlock();
                throw InitializationException("Librina already initialized");
        }

	setNetlinkPortId(localPort);
	setLogLevel(logLevel);
	if (setLogFile(pathToLogFile) != 0) {
	        LOG_WARN("Error setting log file, using stdout only");
	}
	rinaManager->getNetlinkManager();

	librinaInitialized = true;
	librinaInitializationLock.unlock();
}

void initialize(const std::string& logLevel,
                const std::string& pathToLogFile)
throw (InitializationException){

        librinaInitializationLock.lock();
        if (librinaInitialized) {
                librinaInitializationLock.unlock();
                throw InitializationException("Librina already initialized");
        }

        setLogLevel(logLevel);
        if (setLogFile(pathToLogFile) != 0) {
                LOG_WARN("Error setting log file, using stdout only");
        }

        rinaManager->getNetlinkManager();

        librinaInitialized = true;
        librinaInitializationLock.unlock();
}

}
