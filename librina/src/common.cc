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
#include <cstdlib>

#define RINA_PREFIX "librina.common"

#include "librina/logs.h"
#include "config.h"
#include "core.h"
#include "librina/common.h"

namespace rina {

std::string getVersion() {
	return VERSION;
}

// FIXME: not portable, but does the job for now
int createdir(const std::string& dir) {
	std::stringstream ss;

	ss << "mkdir " << dir;

	return system(ss.str().c_str());
}

// FIXME: not portable, but does the job for now
int removedir_all(const std::string& dir) {
	std::stringstream ss;

	ss << "rm -rf " << dir;

	return system(ss.str().c_str());
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

ApplicationProcessNamingInformation::ApplicationProcessNamingInformation(struct name * name)
{
	if (!name)
		return;

	if (name->process_name)
		processName = name->process_name;

	if (name->process_instance)
		processInstance = name->process_instance;

	if (name->entity_name)
		entityName = name->entity_name;

	if (name->entity_instance)
		entityInstance = name->entity_instance;
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

bool ApplicationProcessNamingInformation::operator>=(const ApplicationProcessNamingInformation &other) const
{
	return !(*this < other);
}

const std::string ApplicationProcessNamingInformation::getProcessNamePlusInstance() const
{
	return processName + "-" + processInstance;
}

const std::string ApplicationProcessNamingInformation::getEncodedString() const
{
        return processName + "-" + processInstance +
                        "-" + entityName + "-" + entityInstance;
}

const std::string ApplicationProcessNamingInformation::toString() const
{
        std::stringstream ss;

        ss << processName << ":" << processInstance << ":"
                << entityName << ":" << entityInstance;

        return ss.str();
}

struct name * ApplicationProcessNamingInformation::to_c_name() const
{
	struct name * app_name = new name();

	if (processName != "")
		app_name->process_name = stringToCharArray(processName);
	else
		app_name->process_name = 0;

	if (processInstance != "")
		app_name->process_instance = stringToCharArray(processInstance);
	else
		app_name->process_instance = 0;

	if (entityName != "")
		app_name->entity_name = stringToCharArray(entityName);
	else
		app_name->entity_name = 0;

	if (entityInstance != "")
		app_name->entity_instance = stringToCharArray(entityInstance);
	else
		app_name->entity_instance = 0;

	return app_name;
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
	loss = 10000;
	maxSDUsize = 0;
	msg_boundaries = false;
}

FlowSpecification::FlowSpecification(struct flow_spec * fspec)
{
	averageSDUBandwidth = fspec->average_sdu_bandwidth;
	averageBandwidth = fspec->average_sdu_bandwidth;
	peakBandwidthDuration = fspec->peak_bandwidth_duration;
	peakSDUBandwidthDuration = fspec->peak_sdu_bandwidth_duration;
	undetectedBitErrorRate = 0;
	partialDelivery = fspec->partial_delivery;
	orderedDelivery = fspec->ordered_delivery;
	maxAllowableGap = fspec->max_allowable_gap;
	jitter = fspec->jitter;
	delay = fspec->delay;
	loss = fspec->loss;
	maxSDUsize = fspec->max_sdu_size;
	msg_boundaries = fspec->msg_boundaries;
}

const std::string FlowSpecification::toString() {
        std::stringstream ss;
        ss<<"Jitter: "<<jitter<<"; Delay: "<<delay
          <<"; Max loss probability: "<< loss/10000 <<std::endl;
        ss<<"In oder delivery: "<<orderedDelivery;
        ss<<"; Partial delivery allowed: "<<partialDelivery;
        ss<<"; Preserve message boundaries: "<<msg_boundaries<<std::endl;
        ss<<"Max allowed gap between SDUs: "<<maxAllowableGap;
        ss<<"; Undetected bit error rate: "<<undetectedBitErrorRate<<std::endl;
        ss<<"Average bandwidth (bytes/s): "<<averageBandwidth;
        ss<<"; Average SDU bandwidth (bytes/s): "<<averageSDUBandwidth<<std::endl;
        ss<<"Peak bandwidth duration (ms): "<<peakBandwidthDuration;
        ss<<"; Peak SDU bandwidth duration (ms): "<<peakSDUBandwidthDuration;
        return ss.str();
}

struct flow_spec * FlowSpecification::to_c_flowspec() const
{
	struct flow_spec * fspec;

	fspec = new flow_spec();
	fspec->average_bandwidth = averageBandwidth;
	fspec->average_sdu_bandwidth = averageSDUBandwidth;
	fspec->delay = delay;
	fspec->jitter = jitter;
	fspec->loss = loss;
	fspec->max_allowable_gap = maxAllowableGap;
	fspec->max_sdu_size = maxSDUsize;
	fspec->ordered_delivery = orderedDelivery;
	fspec->partial_delivery = partialDelivery;
	fspec->peak_bandwidth_duration = peakBandwidthDuration;
	fspec->peak_sdu_bandwidth_duration = peakSDUBandwidthDuration;
	fspec->undetected_bit_error_rate = undetectedBitErrorRate;
	fspec->msg_boundaries = msg_boundaries;

	return fspec;
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

	if (loss != other.loss) {
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
FlowInformation::FlowInformation()
{
	fd = -1;
	portId = 0;
	state = FLOW_DEALLOCATED;
	user_ipcp_id = 0;
	pid = 0;
}

bool FlowInformation::operator==(const FlowInformation &other) const
{
	return portId == other.portId;
}

bool FlowInformation::operator!=(const FlowInformation &other) const
{
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
DIFProperties::DIFProperties(){
	maxSDUSize = 0;
}

DIFProperties::DIFProperties(
		const ApplicationProcessNamingInformation& DIFName, int maxSDUSize) {
	this->DIFName = DIFName;
	this->maxSDUSize = maxSDUSize;
}

/* CLASS IPC EVENT */
IPCEvent::IPCEvent() {
	eventType = NO_EVENT;
	sequenceNumber = 0;
	ctrl_port = 0;
	ipcp_id = 0;
}

IPCEvent::IPCEvent(IPCEventType et, unsigned int sn,
		   unsigned int cp, unsigned short ipid)
{
	eventType = et;
	sequenceNumber = sn;
	ctrl_port = cp;
	ipcp_id = ipid;
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
	case IPCM_REGISTER_APP_RESPONSE_EVENT:
		result = "22_IPCM_REGISTER_APP_RESPONSE";
		break;
	case IPCM_UNREGISTER_APP_RESPONSE_EVENT:
		result = "23_IPCM_UNREGISTER_APP_RESPONSE";
		break;
	case IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
		result = "24_IPCM_ALLOCATE_FLOW_RESULT";
		break;
	case QUERY_RIB_RESPONSE_EVENT:
		result = "25_QUERY_RIB_RESPONSE";
		break;
	case IPC_PROCESS_DAEMON_INITIALIZED_EVENT:
		result = "26_IPC_PROCESS_DAEMON_INITIALIZED";
		break;
	case TIMER_EXPIRED_EVENT:
		result = "27_TIMER_EXPIRED";
		break;
	case IPC_PROCESS_CREATE_CONNECTION_RESPONSE:
		result = "28_CREATE_EFCP_CONN_RESPONSE";
		break;
	case IPC_PROCESS_UPDATE_CONNECTION_RESPONSE:
		result = "29_UPDATE_EFCP_CONN_RESPONSE";
		break;
	case IPC_PROCESS_CREATE_CONNECTION_RESULT:
		result = "30_CREATE_EFCP_CONN_RESULT";
		break;
	case IPC_PROCESS_DESTROY_CONNECTION_RESULT:
		result = "31_DESTROY_EFCP_CONN_RESULT";
		break;
	case IPC_PROCESS_DUMP_FT_RESPONSE:
		result = "32_DUMP_FT_RESPONSE";
		break;
        case IPC_PROCESS_SET_POLICY_SET_PARAM:
                result = "33_SET_POLICY_SET_PARAM";
                break;
        case IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE:
                result = "34_SET_POLICY_SET_PARAM_RESPONSE";
                break;
        case IPC_PROCESS_SELECT_POLICY_SET:
                result = "35_SELECT_POLICY_SET";
                break;
        case IPC_PROCESS_SELECT_POLICY_SET_RESPONSE:
                result = "36_SELECT_POLICY_SET_RESPONSE";
                break;
        case IPC_PROCESS_PLUGIN_LOAD:
                result = "37_PLUGIN_LOAD";
                break;
        case IPC_PROCESS_PLUGIN_LOAD_RESPONSE:
                result = "38_PLUGIN_LOAD_RESPONSE";
                break;
        case IPC_PROCESS_UPDATE_CRYPTO_STATE_RESPONSE:
                result = "39_UPDATE_CRYPTO_STATE_RESPONSE";
                break;
	case IPC_PROCESS_FWD_CDAP_MSG:
		result = "40_IPC_PROCESS_FWD_CDAP_MSG";
		break;
	case IPC_PROCESS_FWD_CDAP_RESPONSE_MSG:
		result = "41_IPC_PROCESS_FWD_CDAP_RESPONSE_MSG";
		break;
	case DISCONNECT_NEIGHBOR_REQUEST_EVENT:
		result = "42_DISCONNECT_NEIGHBOR_REQUEST_EVENT";
		break;
	case DISCONNECT_NEIGHBOR_RESPONSE_EVENT:
		result = "43_DISCONNECT_NEIGHBOR_RESPONSE_EVENT";
		break;
	case IPCM_MEDIA_REPORT_EVENT:
		result = "44_MEDIA_REPORT_EVENT";
		break;
	case IPC_PROCESS_ALLOCATE_PORT_RESPONSE:
		result = "45_ALLOCATE_PORT_RESPONSE";
		break;
	case IPC_PROCESS_DEALLOCATE_PORT_RESPONSE:
		result = "46_DEALLOCATE_PORT_RESPONSE";
		break;
	case IPC_PROCESS_WRITE_MGMT_SDU_RESPONSE:
		result = "47_WRITE_MGMT_SDU_RESPONSE";
		break;
	case IPC_PROCESS_READ_MGMT_SDU_NOTIF:
		result = "48_READ_MGMT_SDU_NOTIF";
		break;
	case IPCM_CREATE_IPCP_RESPONSE:
		result = "49_CREATE_IPCP_RESPONSE";
		break;
	case IPCM_DESTROY_IPCP_RESPONSE:
		result = "50_DESTROY_IPCP_RESPONSE";
		break;
	case IPCM_FINALIZATION_REQUEST_EVENT:
		result = "51_IPCM_FINALIZATION_REQUEST_EVENT";
		break;
	case IPCP_SCAN_MEDIA_REQUEST_EVENT:
		result = "52_IPCP_SCAN_MEDIA_REQUEST_EVENT";
	case NO_EVENT:
		result = "55_NO_EVENT";
		break;
	default:
		result = "Unknown event";
	}

	return result;
}

/* CLASS BASE RESPONSE EVENT */
BaseResponseEvent::BaseResponseEvent(int res, IPCEventType eventType,
                        unsigned int sn, unsigned int ctrl_p, unsigned short ipcp_id) :
                              IPCEvent(eventType, sn, ctrl_p, ipcp_id)
{
        result = res;
}

/* CLASS FLOW REQUEST EVENT */
FlowRequestEvent::FlowRequestEvent() :
		IPCEvent (FLOW_ALLOCATION_REQUESTED_EVENT, 0, 0, 0)
{
	localRequest = false;
	portId = 0;
	pid = 0;
	ipcProcessId = 0;
	flowRequestorIpcProcessId = 0;
	internal = false;
}

FlowRequestEvent::FlowRequestEvent(
		const FlowSpecification& flowSpecification,
		bool localRequest,
		const ApplicationProcessNamingInformation& localApplicationName,
		const ApplicationProcessNamingInformation& remoteApplicationName,
		int flowRequestorIpcProcessId,
		unsigned int sequenceNumber, unsigned int ctrl_p,
		unsigned short ipcp_id, pid_t pid):
				IPCEvent(FLOW_ALLOCATION_REQUESTED_EVENT,
					 sequenceNumber, ctrl_p, ipcp_id) {
	this->flowSpecification = flowSpecification;
	this->localRequest = localRequest;
	this->localApplicationName = localApplicationName;
	this->remoteApplicationName = remoteApplicationName;
	this->flowRequestorIpcProcessId = flowRequestorIpcProcessId;
	this->portId = 0;
	this->ipcProcessId = 0;
	this->internal = false;
	this->pid = pid;
}

FlowRequestEvent::FlowRequestEvent(int portId,
		const FlowSpecification& flowSpecification,
		bool localRequest,
		const ApplicationProcessNamingInformation& localApplicationName,
		const ApplicationProcessNamingInformation& remoteApplicationName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber, unsigned int ctrl_p,
		unsigned short ipcp_id, pid_t pid) :
		IPCEvent(FLOW_ALLOCATION_REQUESTED_EVENT,
				sequenceNumber, ctrl_p, ipcp_id) {
	this->flowSpecification = flowSpecification;
	this->localRequest = localRequest;
	this->localApplicationName = localApplicationName;
	this->remoteApplicationName = remoteApplicationName;
	this->DIFName = DIFName;
	this->flowRequestorIpcProcessId = ipcp_id;
	this->portId = portId;
	this->ipcProcessId = ipcp_id;
	this->internal = false;
	this->pid = pid;
}

/* CLASS FLOW DEALLOCATE REQUEST EVENT */
FlowDeallocateRequestEvent::FlowDeallocateRequestEvent(int portId, unsigned int sequenceNumber,
		unsigned int ctrl_p, unsigned short ipcp_id):
			IPCEvent(FLOW_DEALLOCATION_REQUESTED_EVENT,
					sequenceNumber, ctrl_p, ipcp_id){
	this->portId = portId;
	this->internal = false;
}

/* CLASS FLOW DEALLOCATED EVENT */
FlowDeallocatedEvent::FlowDeallocatedEvent(int portId, int code,
		 unsigned int ctrl_port, unsigned short ipcp_id) :
				IPCEvent(FLOW_DEALLOCATED_EVENT, 0, ctrl_port, ipcp_id)
{
	this->portId = portId;
	this->code = code;
}

/* CLASS APPLICATION REGISTRATION INFORMATION */
ApplicationRegistrationInformation::ApplicationRegistrationInformation()
{
	applicationRegistrationType = APPLICATION_REGISTRATION_ANY_DIF;
	ipcProcessId = 0;
	ctrl_port = 0;
	pid = 0;
}

ApplicationRegistrationInformation::ApplicationRegistrationInformation(
		ApplicationRegistrationType applicationRegistrationType)
{
	this->applicationRegistrationType = applicationRegistrationType;
	ipcProcessId = 0;
	ctrl_port = 0;
	pid = 0;
}

const std::string ApplicationRegistrationInformation::toString(){
        std::stringstream ss;

        if (dafName.processName != "") {
        	ss << "DAF name: " << dafName.toString() << "; ";
        }
        ss<<"Application name: "<<appName.toString()<<std::endl;
        ss<<"DIF name: "<<difName.processName;
        ss<<"; IPC Process id: "<<ipcProcessId;

        return ss.str();
}

/* CLASS APPLICATION REGISTRATION REQUEST */
ApplicationRegistrationRequestEvent::ApplicationRegistrationRequestEvent(const ApplicationRegistrationInformation&
			applicationRegistrationInformation, unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id) :
		IPCEvent(APPLICATION_REGISTRATION_REQUEST_EVENT,
				sequenceNumber, ctrl_p, ipcp_id) {
	this->applicationRegistrationInformation =
			applicationRegistrationInformation;
}

/* CLASS BASE APPLICATION REGISTRATION EVENT */
BaseApplicationRegistrationEvent::BaseApplicationRegistrationEvent(const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& DIFName,
                        IPCEventType eventType,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id):
                                IPCEvent(eventType, sequenceNumber, ctrl_p, ipcp_id)
{
        this->applicationName = appName;
        this->DIFName = DIFName;
}

BaseApplicationRegistrationEvent::BaseApplicationRegistrationEvent(const ApplicationProcessNamingInformation& appName,
                        IPCEventType eventType,
                        unsigned int sequenceNumber, unsigned int ctrl_p, unsigned short ipcp_id):
                                IPCEvent(eventType, sequenceNumber, ctrl_p, ipcp_id) {
        this->applicationName = appName;
}

/* CLASS APPLICATION UNREGISTRATION REQUEST EVENT */
ApplicationUnregistrationRequestEvent::ApplicationUnregistrationRequestEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber,
		unsigned int ctrl_p, unsigned short ipcp_id) :
                BaseApplicationRegistrationEvent(
                                appName, DIFName,
                                APPLICATION_UNREGISTRATION_REQUEST_EVENT,
				sequenceNumber, ctrl_p, ipcp_id) {
}

/* CLASS BASE APPLICATION RESPONSE EVENT */
BaseApplicationRegistrationResponseEvent::
        BaseApplicationRegistrationResponseEvent(
                const ApplicationProcessNamingInformation& appName,
                const ApplicationProcessNamingInformation& DIFName,
                int result,
                IPCEventType eventType,
                unsigned int sequenceNumber,
		unsigned int ctrl_p, unsigned short ipcp_id) :
                BaseApplicationRegistrationEvent (appName, DIFName,
                                eventType, sequenceNumber, ctrl_p, ipcp_id){
        this->result = result;
}

BaseApplicationRegistrationResponseEvent::
        BaseApplicationRegistrationResponseEvent(
                const ApplicationProcessNamingInformation& appName,
                int result,
                IPCEventType eventType,
                unsigned int sequenceNumber,
		unsigned int ctrl_p, unsigned short ipcp_id) :
                BaseApplicationRegistrationEvent (appName, eventType,
                		sequenceNumber, ctrl_p, ipcp_id){
        this->result = result;
}

/* CLASS REGISTER APPLICATION RESPONSE EVENT */
RegisterApplicationResponseEvent::RegisterApplicationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& difName,
                        int result,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id):
                BaseApplicationRegistrationResponseEvent(appName, difName, result,
                                       REGISTER_APPLICATION_RESPONSE_EVENT,
                                       sequenceNumber, ctrl_p, ipcp_id){
}

/* CLASS UNREGISTER APPLICATION RESPONSE EVENT */
UnregisterApplicationResponseEvent::UnregisterApplicationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        int result,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id):
                BaseApplicationRegistrationResponseEvent(
                                       appName, result,
                                       UNREGISTER_APPLICATION_RESPONSE_EVENT,
                                       sequenceNumber, ctrl_p, ipcp_id){
}

/* CLASS ALLOCATE FLOW RESPONSE EVENT */
AllocateFlowResponseEvent::AllocateFlowResponseEvent(
                int result,
                bool notifySource,
                int flowAcceptorIpcProcessId,
                unsigned int sequenceNumber,
		unsigned int ctrl_p,
		unsigned short ipcp_id, pid_t pid) :
        BaseResponseEvent(result,
                          ALLOCATE_FLOW_RESPONSE_EVENT,
                          sequenceNumber, ctrl_p, ipcp_id)
{
        this->notifySource             = notifySource;
        this->flowAcceptorIpcProcessId = flowAcceptorIpcProcessId;
        this->pid = pid;
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
			                destName, 0, 24, 23, 2, 0);

	return event;
}

IPCEvent * IPCEventProducer::eventWait()
{
#if STUB_API
	return getIPCEvent();
#else
	return irati_ctrl_mgr->get_next_ctrl_msg();
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
UcharArray::UcharArray(const UcharArray &a, const UcharArray &b)
{
	length = a.length + b.length;
	data = new unsigned char[length];
	memcpy(data, a.data, a.length);
	memcpy(data+a.length, b.data, b.length);

}
UcharArray::UcharArray(const UcharArray &a, const UcharArray &b, const UcharArray &c)
{
	length = a.length + b.length + c.length;
	data = new unsigned char[length];
	memcpy(data, a.data, a.length);
	memcpy(data+a.length, b.data, b.length);
	memcpy(data+a.length+b.length, c.data, c.length);

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
Neighbor::Neighbor()
{
	address_ = 0;
	old_address_ = 0;
	average_rtt_in_ms_ = 0;
	last_heard_from_time_in_ms_ = 0;
	enrolled_ = false;
	underlying_port_id_ = 0;
	number_of_enrollment_attempts_ = 0;
	internal_port_id = 0;
}

void Neighbor::from_c_neighbor(Neighbor & nei, struct ipcp_neighbor * cnei)
{
	struct name_entry * pos;

	if (!cnei)
		return;

	nei.address_ = cnei->address;
	nei.average_rtt_in_ms_ = cnei->average_rtt_in_ms;
	nei.enrolled_ = cnei->enrolled;
	nei.internal_port_id = cnei->intern_port_id;
	nei.last_heard_from_time_in_ms_ = cnei->last_heard_time_ms;
	nei.name_ = ApplicationProcessNamingInformation(cnei->ipcp_name);
	nei.number_of_enrollment_attempts_ = cnei->num_enroll_attempts;
	nei.old_address_ = cnei->old_address;
	nei.supporting_dif_name_ = ApplicationProcessNamingInformation(cnei->sup_dif_name);
	nei.underlying_port_id_ = cnei->under_port_id;
        list_for_each_entry(pos, &(cnei->supporting_difs), next) {
        	nei.supporting_difs_.push_back(ApplicationProcessNamingInformation(pos->entry));
        }
}

struct ipcp_neighbor * Neighbor::to_c_neighbor() const
{
	struct ipcp_neighbor * result;
	std::list<ApplicationProcessNamingInformation>::const_iterator it;
	struct name_entry * pos;

	result = ipcp_neighbor_create();
	result->address = address_;
	result->old_address = old_address_;
	result->average_rtt_in_ms = average_rtt_in_ms_;
	result->last_heard_time_ms = last_heard_from_time_in_ms_;
	result->enrolled = enrolled_;
	result->under_port_id = underlying_port_id_;
	result->intern_port_id = internal_port_id;
	result->num_enroll_attempts = number_of_enrollment_attempts_;
	result->ipcp_name = name_.to_c_name();
	result->sup_dif_name = supporting_dif_name_.to_c_name();
	for (it = supporting_difs_.begin();
			it != supporting_difs_.end(); ++it) {
		pos = new name_entry();
		INIT_LIST_HEAD(&pos->next);
		pos->entry = it->to_c_name();
		list_add_tail(&pos->next, &result->supporting_difs);
	}

	return result;
}

bool Neighbor::operator==(const Neighbor &other) const{
	return name_ == other.get_name();
}

bool Neighbor::operator!=(const Neighbor &other) const{
	return !(*this == other);
}

const ApplicationProcessNamingInformation
Neighbor::get_name() const {
	return name_;
}

void Neighbor::set_name(
		const ApplicationProcessNamingInformation& name) {
	name_ = name;
}

const ApplicationProcessNamingInformation
Neighbor::get_supporting_dif_name() const {
	return supporting_dif_name_;
}

void Neighbor::set_supporting_dif_name(
		const ApplicationProcessNamingInformation& supporting_dif_name) {
	supporting_dif_name_ = supporting_dif_name;
}

const std::list<ApplicationProcessNamingInformation>
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

unsigned int Neighbor::get_old_address() const
{
	return old_address_;
}

void Neighbor::set_old_address(unsigned int address)
{
	old_address_ = address;
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
	ss<<"; N-1 port-id: "<<underlying_port_id_ <<std::endl;
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

        irati_ctrl_mgr->set_irati_ctrl_port(localPort);
	setLogLevel(logLevel.c_str());
	if (setLogFile(pathToLogFile.c_str()) != 0) {
	        LOG_WARN("Error setting log file, using stdout only");
	}
	irati_ctrl_mgr->initialize();

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

        if (setLogFile(pathToLogFile.c_str()) != 0) {
                LOG_WARN("Error setting log file, using stdout only");
        }

        setLogLevel(logLevel.c_str());

        irati_ctrl_mgr->initialize();

        librinaInitialized = true;
        librinaInitializationLock.unlock();
}

void librina_finalize(){
	// delete rinaManager Singleton
	irati_ctrl_mgr.~Singleton();
}

}

int buffer_destroy(struct buffer * b)
{
        if (!b)
        	return -1;

        if (b->data) {
        	delete b->data;
        	b->data = 0;
        }

        delete b;

        return 0;
}

struct buffer * buffer_create()
{
	struct buffer * result;

	result = new buffer();
	if (!result)
		return 0;

	memset(result, 0, sizeof(struct buffer));

	return result;
}
