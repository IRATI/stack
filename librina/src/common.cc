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

#include <climits>
#include <ostream>
#include <sstream>

#define RINA_PREFIX "librina.common"

#include "librina/logs.h"
#include "config.h"
#include "core.h"
#include "librina/common.h"

namespace rina {

std::string getVersion() {
	return VERSION;
}

int string2int(const std::string& s, int& ret)
{
	char *dummy;
	const char *cstr = s.c_str();

	ret = strtoul(cstr, &dummy, 10);
	if (!s.size() || *dummy != '\0') {
		ret = ~0U;
		return -1;
	}

	return 0;
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

ApplicationProcessNamingInformation
decode_apnameinfo(const std::string &encodedString)
{
        ApplicationProcessNamingInformation ret;
        std::stringstream ss(encodedString);
        std::string elem;
        std::vector<std::string> elems;

        while (std::getline(ss, elem, '-')) {
                elems.push_back(elem);
        }

        if (elems.size() < 3) {
                return ret;
        }

        ret.processName = elems[0];
        ret.processInstance = elems[1];
        ret.entityName = elems[2];
        if (elems.size() >= 4) {
                ret.entityInstance = elems[3];
        }

        return ret;
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
        ss<<"; Port-id: "<<portId<<"; State: "<<state<<std::endl;
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
	case IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION:
		result = "18_DIF_REGISTRATION_NOTIFICATION";
		break;
	case IPC_PROCESS_QUERY_RIB:
		result = "19_QUERY_RIB";
		break;
	case GET_DIF_PROPERTIES:
		result = "20_GET_DIF_PROPERTIES";
		break;
	case GET_DIF_PROPERTIES_RESPONSE_EVENT:
		result = "21_GET_DIF_PROPERTIES_RESPONSE";
		break;
	case OS_PROCESS_FINALIZED:
		result = "22_OS_PROCESS_FINALIZED";
		break;
	case IPCM_REGISTER_APP_RESPONSE_EVENT:
		result = "23_IPCM_REGISTER_APP_RESPONSE";
		break;
	case IPCM_UNREGISTER_APP_RESPONSE_EVENT:
		result = "24_IPCM_UNREGISTER_APP_RESPONSE";
		break;
	case IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT:
		result = "25_IPCM_DEALLOCATE_FLOW_RESPONSE";
		break;
	case IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
		result = "26_IPCM_ALLOCATE_FLOW_RESULT";
		break;
	case QUERY_RIB_RESPONSE_EVENT:
		result = "27_QUERY_RIB_RESPONSE";
		break;
	case IPC_PROCESS_DAEMON_INITIALIZED_EVENT:
		result = "28_IPC_PROCESS_DAEMON_INITIALIZED";
		break;
	case TIMER_EXPIRED_EVENT:
		result = "29_TIMER_EXPIRED";
		break;
	case IPC_PROCESS_CREATE_CONNECTION_RESPONSE:
		result = "30_CREATE_EFCP_CONN_RESPONSE";
		break;
	case IPC_PROCESS_UPDATE_CONNECTION_RESPONSE:
		result = "31_UPDATE_EFCP_CONN_RESPONSE";
		break;
	case IPC_PROCESS_CREATE_CONNECTION_RESULT:
		result = "32_CREATE_EFCP_CONN_RESULT";
		break;
	case IPC_PROCESS_DESTROY_CONNECTION_RESULT:
		result = "33_DESTROY_EFCP_CONN_RESULT";
		break;
	case IPC_PROCESS_DUMP_FT_RESPONSE:
		result = "34_DUMP_FT_RESPONSE";
		break;
        case IPC_PROCESS_SET_POLICY_SET_PARAM:
                result = "35_SET_POLICY_SET_PARAM";
                break;
        case IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE:
                result = "36_SET_POLICY_SET_PARAM_RESPONSE";
                break;
        case IPC_PROCESS_SELECT_POLICY_SET:
                result = "37_SELECT_POLICY_SET";
                break;
        case IPC_PROCESS_SELECT_POLICY_SET_RESPONSE:
                result = "38_SELECT_POLICY_SET_RESPONSE";
                break;
        case IPC_PROCESS_PLUGIN_LOAD:
                result = "39_PLUGIN_LOAD";
                break;
        case IPC_PROCESS_PLUGIN_LOAD_RESPONSE:
                result = "40_PLUGIN_LOAD_RESPONSE";
                break;
        case IPC_PROCESS_UPDATE_CRYPTO_STATE_RESPONSE:
                result = "41_UPDATE_CRYPTO_STATE_RESPONSE";
                break;
	case IPC_PROCESS_FWD_CDAP_MSG:
		result = "42_IPC_PROCESS_FWD_CDAP_MSG";
		break;
	case NO_EVENT:
		result = "42_NO_EVENT";
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

//Class UCharArray
UcharArray::UcharArray()
{
	data = 0;
	length = 0;
}

UcharArray::UcharArray(int arrayLength)
{
	data = new unsigned char[arrayLength];
	length = arrayLength;
}

UcharArray::UcharArray(const ser_obj_t * sobj)
{
	data = new unsigned char[sobj->size_];
	length = sobj->size_;
	memcpy(data, sobj->message_, length);
}

UcharArray::~UcharArray()
{
	if (data) {
		delete[] data;
		data = 0;
	}
}

UcharArray& UcharArray::operator=(const UcharArray &other)
{
	length = other.length;
	if (length > 0) {
		data = new unsigned char[length];
		memcpy(data, other.data, length);
	} else
		data = 0;

	return *this;
}

bool UcharArray::operator==(const UcharArray &other) const
{
	if (length != other.length) {
		return false;
	}

	for (int i=0; i<length; i++) {
		if (data[i] != other.data[i]) {
			return false;
		}
	}

	return true;
}

bool UcharArray::operator!=(const UcharArray &other) const
{
	return !(*this == other);
}

std::string UcharArray::toString()
{
	std::stringstream ss;
	ss << std::hex;
	for (int i = 0; i < length; i++) {
		ss << std::setw(2) << std::setfill('0') << (int)data[i];
	}
	return ss.str();
}

void UcharArray::get_seralized_object(ser_obj_t& result)
{
	result.size_ = length;
	result.message_ = new unsigned char[result.size_];
	memcpy(result.message_, data, result.size_);
}

//Class ConsecutiveUnsignedIntegerGenerator
ConsecutiveUnsignedIntegerGenerator::ConsecutiveUnsignedIntegerGenerator() {
	counter_ = 0;
}

unsigned int ConsecutiveUnsignedIntegerGenerator::next(){
	unsigned int result = 0;
	lock_.lock();
	if (counter_ == UINT_MAX) {
		counter_ = 0;
	}
	counter_++;
	result = counter_;
	lock_.unlock();

	return result;
}

/* CLASS NEIGHBOR */
Neighbor::Neighbor() {
	address_ = false;
	average_rtt_in_ms_ = 0;
	last_heard_from_time_in_ms_ = 0;
	enrolled_ = false;
	underlying_port_id_ = 0;
	number_of_enrollment_attempts_ = 0;
}

Neighbor::Neighbor(const Neighbor &other)
{
	address_ = other.address_;
	average_rtt_in_ms_ = other.average_rtt_in_ms_;
	last_heard_from_time_in_ms_ = other.last_heard_from_time_in_ms_;
	enrolled_ = other.enrolled_;
	underlying_port_id_ = other.underlying_port_id_;
	number_of_enrollment_attempts_ = other.number_of_enrollment_attempts_;
	name_ = other.name_;
	supporting_dif_name_ = other.supporting_dif_name_;
	supporting_difs_ = other.supporting_difs_;
}

Neighbor& Neighbor::operator=(const Neighbor &other)
{
	address_ = other.address_;
	average_rtt_in_ms_ = other.average_rtt_in_ms_;
	last_heard_from_time_in_ms_ = other.last_heard_from_time_in_ms_;
	enrolled_ = other.enrolled_;
	underlying_port_id_ = other.underlying_port_id_;
	number_of_enrollment_attempts_ = other.number_of_enrollment_attempts_;
	name_ = other.name_;
	supporting_dif_name_ = other.supporting_dif_name_;
	supporting_difs_ = other.supporting_difs_;
	return *this;
}

bool Neighbor::operator==(const Neighbor &other) const{
	return name_ == other.get_name();
}

bool Neighbor::operator!=(const Neighbor &other) const{
	return !(*this == other);
}

const ApplicationProcessNamingInformation&
Neighbor::get_name() const {
	return name_;
}

void Neighbor::set_name(
		const ApplicationProcessNamingInformation& name) {
	name_ = name;
}

const ApplicationProcessNamingInformation&
Neighbor::get_supporting_dif_name() const {
	return supporting_dif_name_;
}

void Neighbor::set_supporting_dif_name(
		const ApplicationProcessNamingInformation& supporting_dif_name) {
	supporting_dif_name_ = supporting_dif_name;
}

const std::list<ApplicationProcessNamingInformation>&
Neighbor::get_supporting_difs() {
	return supporting_difs_;
}

void Neighbor::set_supporting_difs(
		const std::list<ApplicationProcessNamingInformation>& supporting_difs) {
	supporting_difs_ = supporting_difs;
}

void Neighbor::add_supporting_dif(
		const ApplicationProcessNamingInformation& supporting_dif) {
	supporting_difs_.push_back(supporting_dif);
}

unsigned int Neighbor::get_address() const {
	return address_;
}

void Neighbor::set_address(unsigned int address) {
	address_ = address;
}

unsigned int Neighbor::get_average_rtt_in_ms() const {
	return average_rtt_in_ms_;
}

void Neighbor::set_average_rtt_in_ms(unsigned int average_rtt_in_ms) {
	average_rtt_in_ms_ = average_rtt_in_ms;
}

bool Neighbor::is_enrolled() const {
	return enrolled_;
}

void Neighbor::set_enrolled(bool enrolled){
	enrolled_ = enrolled;
}

int Neighbor::get_last_heard_from_time_in_ms() const {
	return last_heard_from_time_in_ms_;
}

void Neighbor::set_last_heard_from_time_in_ms(int last_heard_from_time_in_ms) {
	last_heard_from_time_in_ms_ = last_heard_from_time_in_ms;
}

int Neighbor::get_underlying_port_id() const {
	return underlying_port_id_;
}

void Neighbor::set_underlying_port_id(int underlying_port_id) {
	underlying_port_id_ = underlying_port_id;
}

unsigned int Neighbor::get_number_of_enrollment_attempts() const {
	return number_of_enrollment_attempts_;
}

void Neighbor::set_number_of_enrollment_attempts(
		unsigned int number_of_enrollment_attempts) {
	number_of_enrollment_attempts_ = number_of_enrollment_attempts;
}

const std::string Neighbor::toString(){
	std::stringstream ss;

	ss<<"Address: "<<address_;
	ss<<"; Average RTT(ms): "<<average_rtt_in_ms_;
	ss<<"; Is enrolled: "<<enrolled_<<std::endl;
	ss<<"Name: "<<name_.toString()<<std::endl;
	ss<<"Supporting DIF in common: "<<supporting_dif_name_.processName;
	ss<<"; N-1 port-id: "<<underlying_port_id_<<std::endl;
	ss<<"List of supporting DIFs: ";
	for (std::list<ApplicationProcessNamingInformation>::iterator it = supporting_difs_.begin();
			it != supporting_difs_.end(); it++)
		ss<< it->processName << "; ";
	ss<<std::endl;
	ss<<"Last heard from time (ms): "<<last_heard_from_time_in_ms_;
	ss<<"; Number of enrollment attempts: "<<number_of_enrollment_attempts_;

	return ss.str();
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
	setLogLevel(logLevel.c_str());
	if (setLogFile(pathToLogFile.c_str()) != 0) {
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

        setLogLevel(logLevel.c_str());
        if (setLogFile(pathToLogFile.c_str()) != 0) {
                LOG_WARN("Error setting log file, using stdout only");
        }

        rinaManager->getNetlinkManager();

        librinaInitialized = true;
        librinaInitializationLock.unlock();
}

void librina_finalize(){
	// delete rinaManager Singleton
	rinaManager.~Singleton();
}

}
