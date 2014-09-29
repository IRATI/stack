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
		processName = other.processName;
		processInstance = other.processInstance;
		entityName = other.entityName;
		entityInstance = other.entityInstance;
	}

	return *this;
}

bool ApplicationProcessNamingInformation::operator>(
		const ApplicationProcessNamingInformation &other) const {
	int aux = processName.compare(other.processName);
	if (aux > 0) {
		return true;
	} else if (aux < 0) {
		return false;
	}

	aux = processInstance.compare(other.processInstance);
	if (aux > 0) {
		return true;
	} else if (aux < 0) {
		return false;
	}

	aux = entityName.compare(other.entityName);
	if (aux > 0) {
		return true;
	} else if (aux < 0) {
		return false;
	}

	aux = entityInstance.compare(other.entityInstance);
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
	int aux = processName.compare(other.processName);
	if (aux < 0) {
		return true;
	} else if (aux > 0) {
		return false;
	}

	aux = processInstance.compare(other.processInstance);
	if (aux < 0) {
		return true;
	} else if (aux > 0) {
		return false;
	}

	aux = entityName.compare(other.entityName);
	if (aux < 0) {
		return true;
	} else if (aux > 0) {
		return false;
	}

	aux = entityInstance.compare(other.entityInstance);
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

std::string ApplicationProcessNamingInformation::
getProcessNamePlusInstance(){
	return processName + "-" + processInstance;
}

const std::string ApplicationProcessNamingInformation::getEncodedString() const {
        return processName + "-" + processInstance +
                        "-" + entityName + "-" + entityInstance;
}

const std::string ApplicationProcessNamingInformation::toString() const{
        std::stringstream ss;

        ss << processName << ":" << processInstance << ":"
                << entityName << ":" << entityInstance;

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
	if (averageBandwidth != other.averageBandwidth) {
		return false;
	}

	if (averageSDUBandwidth != other.averageSDUBandwidth) {
		return false;
	}

	if (peakBandwidthDuration != other.peakBandwidthDuration) {
		return false;
	}

	if (peakSDUBandwidthDuration != other.peakSDUBandwidthDuration) {
		return false;
	}

	if (undetectedBitErrorRate != other.undetectedBitErrorRate) {
		return false;
	}

	if (partialDelivery != other.partialDelivery) {
		return false;
	}

	if (orderedDelivery != other.orderedDelivery) {
		return false;
	}

	if (maxAllowableGap != other.maxAllowableGap) {
		return false;
	}

	if (delay != other.delay) {
		return false;
	}

	if (jitter != other.jitter) {
		return false;
	}

	if (maxSDUsize != other.maxSDUsize) {
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
	return portId == other.portId;
}

bool FlowInformation::operator!=(
		const FlowInformation &other) const {
	return !(*this == other);
}

const std::string FlowInformation::toString(){
        std::stringstream ss;

        ss<<"Local app name: "<<localAppName.toString()<<std::endl;
        ss<<"Remote app name: "<<remoteAppName.toString()<<std::endl;
        ss<<"DIF name: "<<difName.processName;
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

/* CLASS IPC EVENT */
IPCEvent::IPCEvent() {
	eventType = NO_EVENT;
	sequenceNumber = 0;
}

IPCEvent::IPCEvent(IPCEventType eventType, unsigned int sequenceNumber) {
	this->eventType = eventType;
	this->sequenceNumber = sequenceNumber;
}


IPCEvent::~IPCEvent() {
}

const std::string IPCEvent::eventTypeToString(IPCEventType eventType) {
	std::string result;

	switch (eventType) {
	case FLOW_ALLOCATION_REQUESTED_EVENT:
		result = "0_FLOW_ALLOCATION_REQUESTED";
		break;
	case ALLOCATE_FLOW_REQUEST_RESULT_EVENT:
		result = "1_ALLOCATE_FLOW_REQUEST_RESULT";
		break;
	case ALLOCATE_FLOW_RESPONSE_EVENT:
		result = "2_ALLOCATE_FLOW_RESPONSE";
		break;
	case FLOW_DEALLOCATION_REQUESTED_EVENT:
		result = "3_FLOW_DEALLOCATION_REQUESTED";
		break;
	case DEALLOCATE_FLOW_RESPONSE_EVENT:
		result = "4_DEALLOCATE_FLOW_RESPONSE";
		break;
	case APPLICATION_UNREGISTERED_EVENT:
		result = "5_APPLICATION_UNREGISTERED";
		break;
	case FLOW_DEALLOCATED_EVENT:
		result = "6_FLOW_DEALLOCATED";
		break;
	case APPLICATION_REGISTRATION_REQUEST_EVENT:
		result = "7_APPLICATION_REGISTRATION_REQUEST";
		break;
	case REGISTER_APPLICATION_RESPONSE_EVENT:
		result = "8_REGISTER_APP_RESPONSE";
		break;
	case APPLICATION_UNREGISTRATION_REQUEST_EVENT:
		result = "9_APP_UNREGISTRATION_REQUEST";
		break;
	case UNREGISTER_APPLICATION_RESPONSE_EVENT:
		result = "10_UNREGISTER_APP_RESPONSE";
		break;
	case APPLICATION_REGISTRATION_CANCELED_EVENT:
		result = "11_APP_REGISTRATION_CANCELED";
		break;
	case ASSIGN_TO_DIF_REQUEST_EVENT:
		result = "12_ASSIGN_TO_DIF_REQUEST";
		break;
	case ASSIGN_TO_DIF_RESPONSE_EVENT:
		result = "13_ASSIGN_TO_DIF_RESPONSE";
		break;
	case UPDATE_DIF_CONFIG_REQUEST_EVENT:
		result = "14_UPDATE_DIF_CONFIG_REQUEST";
		break;
	case UPDATE_DIF_CONFIG_RESPONSE_EVENT:
		result = "15_UPDATE_DIF_CONFIG_RESPONSE";
		break;
	case ENROLL_TO_DIF_REQUEST_EVENT:
		result = "16_ENROLL_TO_DIF_REQUEST";
		break;
	case ENROLL_TO_DIF_RESPONSE_EVENT:
		result = "17_ENROLL_TO_DIF_RESONSE";
		break;
	case NEIGHBORS_MODIFIED_NOTIFICATION_EVENT:
		result = "18_NEIGHBORS_MODIFIED_NOTIFICATION";
		break;
	case IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION:
		result = "19_DIF_REGISTRATION_NOTIFICATION";
		break;
	case IPC_PROCESS_QUERY_RIB:
		result = "20_QUERY_RIB";
		break;
	case GET_DIF_PROPERTIES:
		result = "21_GET_DIF_PROPERTIES";
		break;
	case GET_DIF_PROPERTIES_RESPONSE_EVENT:
		result = "22_GET_DIF_PROPERTIES_RESPONSE";
		break;
	case OS_PROCESS_FINALIZED:
		result = "23_OS_PROCESS_FINALIZED";
		break;
	case IPCM_REGISTER_APP_RESPONSE_EVENT:
		result = "24_IPCM_REGISTER_APP_RESPONSE";
		break;
	case IPCM_UNREGISTER_APP_RESPONSE_EVENT:
		result = "25_IPCM_UNREGISTER_APP_RESPONSE";
		break;
	case IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT:
		result = "26_IPCM_DEALLOCATE_FLOW_RESPONSE";
		break;
	case IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
		result = "27_IPCM_ALLOCATE_FLOW_RESULT";
		break;
	case QUERY_RIB_RESPONSE_EVENT:
		result = "28_QUERY_RIB_RESPONSE";
		break;
	case IPC_PROCESS_DAEMON_INITIALIZED_EVENT:
		result = "29_IPC_PROCESS_DAEMON_INITIALIZED";
		break;
	case TIMER_EXPIRED_EVENT:
		result = "30_TIMER_EXPIRED";
		break;
	case IPC_PROCESS_CREATE_CONNECTION_RESPONSE:
		result = "31_CREATE_EFCP_CONN_RESPONSE";
		break;
	case IPC_PROCESS_UPDATE_CONNECTION_RESPONSE:
		result = "32_UPDATE_EFCP_CONN_RESPONSE";
		break;
	case IPC_PROCESS_CREATE_CONNECTION_RESULT:
		result = "33_CREATE_EFCP_CONN_RESULT";
		break;
	case IPC_PROCESS_DESTROY_CONNECTION_RESULT:
		result = "34_DESTROY_EFCP_CONN_RESULT";
		break;
	case IPC_PROCESS_DUMP_FT_RESPONSE:
		result = "35_DUMP_FT_RESPONSE";
		break;
	case NO_EVENT:
		result = "36_NO_EVENT";
		break;
	default:
		result = "Unknown event";
	}

	return result;
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

/* CLASS FLOW REQUEST EVENT */
FlowRequestEvent::FlowRequestEvent(){
	localRequest = false;
	portId = 0;
	ipcProcessId = 0;
	flowRequestorIpcProcessId = 0;
}

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

/* CLASS FLOW DEALLOCATED EVENT */
FlowDeallocatedEvent::FlowDeallocatedEvent(
		int portId, int code) :
				IPCEvent(FLOW_DEALLOCATED_EVENT, 0) {
	this->portId = portId;
	this->code = code;
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

const std::string ApplicationRegistrationInformation::toString(){
        std::stringstream ss;

        ss<<"Application name: "<<appName.toString()<<std::endl;
        ss<<"DIF name: "<<difName.processName;
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

/* CLASS IPC EVENT PRODUCER */

/* Auxiliar function called in case of using the stubbed version of the API */
IPCEvent * getIPCEvent(){
	ApplicationProcessNamingInformation sourceName;
	sourceName.processName = "/apps/source";
	sourceName.processInstance = "12";
	sourceName.entityName = "database";
	sourceName.entityInstance = "12";

	ApplicationProcessNamingInformation destName;
	destName.processName = "/apps/dest";
	destName.processInstance = "12345";
	destName.entityName = "printer";
	destName.entityInstance = "12623456";

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
	if (this->name.compare(other.name) == 0 &&
			this->value.compare(other.value) == 0)
		return true;

	return false;
}

bool Parameter::operator!=(const Parameter &other) const {
	return !(*this == other);
}

// CLASS SerializedMessage
SerializedObject::SerializedObject() {
	size_ = 0;
	message_ = 0;
}

SerializedObject::SerializedObject(const SerializedObject& other ) {
	initialize(other);
}

SerializedObject& SerializedObject::operator= (const SerializedObject &other) {
	initialize(other);
	return *this;
}

void SerializedObject::initialize(const SerializedObject& other ) {
	size_ = other.size_;
	message_ = new char[size_];
	memcpy(message_, other.message_, size_);
}

SerializedObject::SerializedObject(char* message, int size){
	size_ = size;
	message_ = message;
}
SerializedObject::~SerializedObject(){
	if (message_) {
		delete message_;
		message_ = 0;
	}
}

int SerializedObject::get_size() const {
	return size_;
}

char* SerializedObject::get_message() const {
	return message_;
}

/* INITIALIZATION OPERATIONS */

bool librinaInitialized = false;
Lockable librinaInitializationLock;

void initialize(unsigned int localPort, const std::string& logLevel,
                const std::string& pathToLogFile) {

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
                const std::string& pathToLogFile) {

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
