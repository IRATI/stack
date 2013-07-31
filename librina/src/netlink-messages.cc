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

/*
 * librina-netlink-messages.cc
 *
 *  Created on: 12/06/2013
 *      Author: eduardgrasa
 */

#include <sstream>
#include <unistd.h>

#define RINA_PREFIX "netlink-messages"

#include "logs.h"
#include "netlink-messages.h"
#include "librina-application.h"
#include "librina-ipc-process.h"
#include "librina-ipc-manager.h"

namespace rina {

/* CLASS BASE NETLINK MESSAGE */
BaseNetlinkMessage::BaseNetlinkMessage(
		RINANetlinkOperationCode operationCode) {
	this->operationCode = operationCode;
	sourcePortId = 0;
	destPortId = 0;
	sourceIPCProcessId = 0;
	destIPCProcessId = 0;
	sequenceNumber = 0;
	family = -1;
	responseMessage = false;
	requestMessage = false;
	notificationMessage = false;
}

BaseNetlinkMessage::~BaseNetlinkMessage() {
}

unsigned int BaseNetlinkMessage::getDestPortId() const {
	return destPortId;
}

void BaseNetlinkMessage::setDestPortId(unsigned int destPortId) {
	this->destPortId = destPortId;
}

unsigned int BaseNetlinkMessage::getSequenceNumber() const {
	return sequenceNumber;
}

void BaseNetlinkMessage::setSequenceNumber(unsigned int sequenceNumber) {
	this->sequenceNumber = sequenceNumber;
}

unsigned int BaseNetlinkMessage::getSourcePortId() const {
	return sourcePortId;
}

void BaseNetlinkMessage::setSourcePortId(unsigned int sourcePortId) {
	this->sourcePortId = sourcePortId;
}

unsigned short BaseNetlinkMessage::getDestIpcProcessId() const {
	return destIPCProcessId;
}

void BaseNetlinkMessage::setDestIpcProcessId(unsigned short destIpcProcessId) {
	destIPCProcessId = destIpcProcessId;
}

unsigned short BaseNetlinkMessage::getSourceIpcProcessId() const {
	return sourceIPCProcessId;
}

void BaseNetlinkMessage::setSourceIpcProcessId(
		unsigned short sourceIpcProcessId) {
	sourceIPCProcessId = sourceIpcProcessId;
}

RINANetlinkOperationCode BaseNetlinkMessage::getOperationCode() const {
	return operationCode;
}

int BaseNetlinkMessage::getFamily() const {
	return family;
}

void BaseNetlinkMessage::setFamily(int family) {
	this->family = family;
}

bool BaseNetlinkMessage::isNotificationMessage() const {
	return notificationMessage;
}

void BaseNetlinkMessage::setNotificationMessage(bool notificationMessage) {
	this->notificationMessage = notificationMessage;
}

void BaseNetlinkMessage::setOperationCode(
		RINANetlinkOperationCode operationCode) {
	this->operationCode = operationCode;
}

bool BaseNetlinkMessage::isRequestMessage() const {
	return requestMessage;
}

void BaseNetlinkMessage::setRequestMessage(bool requestMessage) {
	this->requestMessage = requestMessage;
}

bool BaseNetlinkMessage::isResponseMessage() const {
	return responseMessage;
}

void BaseNetlinkMessage::setResponseMessage(bool responseMessage) {
	this->responseMessage = responseMessage;
}

std::string BaseNetlinkMessage::toString(){
	std::stringstream ss;
	ss << "Family: " << family << "; Operation code: "
			<< operationCode << "; Source port: " << sourcePortId
			<< "; Destination port: " << destPortId
			<< "; Sequence Number: " << sequenceNumber << "\n"
			<< "Is request message? " << requestMessage
			<< "; Is response message? " << responseMessage
			<< "; Is notification message? " << notificationMessage << "\n"
			<< "Source IPC Process: " << sourceIPCProcessId
			<< "; Destination IPC Process: " << destIPCProcessId;
	return ss.str();
}

/* CLASS BASE NETLINK RESPONSE MESSAGE */
BaseNetlinkResponseMessage::BaseNetlinkResponseMessage(
		RINANetlinkOperationCode operationCode):
			BaseNetlinkMessage(operationCode){
	result = 0;
}

int BaseNetlinkResponseMessage::getResult() const{
	return result;
}

void BaseNetlinkResponseMessage::setResult(int result){
	this->result = result;
}

const std::string&
BaseNetlinkResponseMessage::getErrorDescription() const{
	return errorDescription;
}
void BaseNetlinkResponseMessage::setErrorDescription(
		const std::string& errorDescription){
	this->errorDescription = errorDescription;
}

/* CLASS RINA APP ALLOCATE FLOW MESSAGE */
AppAllocateFlowRequestMessage::AppAllocateFlowRequestMessage() :
				NetlinkRequestOrNotificationMessage(
						RINA_C_APP_ALLOCATE_FLOW_REQUEST) {
}

const ApplicationProcessNamingInformation&
AppAllocateFlowRequestMessage::getDestAppName() const {
	return destAppName;
}

void AppAllocateFlowRequestMessage::setDestAppName(
		const ApplicationProcessNamingInformation& destAppName) {
	this->destAppName = destAppName;
}

const FlowSpecification&
AppAllocateFlowRequestMessage::getFlowSpecification() const {
	return flowSpecification;
}

void AppAllocateFlowRequestMessage::setFlowSpecification(
		const FlowSpecification& flowSpecification) {
	this->flowSpecification = flowSpecification;
}

const ApplicationProcessNamingInformation&
AppAllocateFlowRequestMessage::getSourceAppName() const {
	return sourceAppName;
}

void AppAllocateFlowRequestMessage::setSourceAppName(
		const ApplicationProcessNamingInformation& sourceAppName) {
	this->sourceAppName = sourceAppName;
}

IPCEvent* AppAllocateFlowRequestMessage::toIPCEvent(){
	FlowRequestEvent * event =
			new FlowRequestEvent(
					this->flowSpecification,
					this->sourceAppName,
					this->destAppName,
					this->getSequenceNumber());
	return event;
}

/* CLASS APP ALLOCATE FLOW REQUEST RESULT MESSAGE */
AppAllocateFlowRequestResultMessage::AppAllocateFlowRequestResultMessage() :
				BaseNetlinkMessage(RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT) {
	this->portId = 0;
	this->ipcProcessPortId = 0;
	this->ipcProcessId = 0;
}

const std::string&
AppAllocateFlowRequestResultMessage::getErrorDescription() const {
	return errorDescription;
}

void AppAllocateFlowRequestResultMessage::setErrorDescription(
		const std::string& errorDescription) {
	this->errorDescription = errorDescription;
}

const ApplicationProcessNamingInformation&
AppAllocateFlowRequestResultMessage::getDifName() const {
	return difName;
}

void AppAllocateFlowRequestResultMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

const ApplicationProcessNamingInformation&
AppAllocateFlowRequestResultMessage::getSourceAppName() const {
	return sourceAppName;
}

void AppAllocateFlowRequestResultMessage::setSourceAppName(
		const ApplicationProcessNamingInformation& sourceAppName) {
	this->sourceAppName = sourceAppName;
}

unsigned int AppAllocateFlowRequestResultMessage::getIpcProcessPortId() const {
	return ipcProcessPortId;
}

void AppAllocateFlowRequestResultMessage::setIpcProcessPortId(
		unsigned int ipcProcessPortId) {
	this->ipcProcessPortId = ipcProcessPortId;
}

int AppAllocateFlowRequestResultMessage::getPortId() const {
	return portId;
}

void AppAllocateFlowRequestResultMessage::setPortId(int portId) {
	this->portId = portId;
}

unsigned short AppAllocateFlowRequestResultMessage::getIpcProcessId() const{
	return ipcProcessId;
}

void AppAllocateFlowRequestResultMessage::setIpcProcessId(
		unsigned short ipcProcessId){
	this->ipcProcessId = ipcProcessId;
}

/* CLASS APP ALLOCATE FLOW REQUEST ARRIVED MESSAGE */
AppAllocateFlowRequestArrivedMessage::AppAllocateFlowRequestArrivedMessage() :
				NetlinkRequestOrNotificationMessage(
						RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED) {
	this->portId = 0;
}

const ApplicationProcessNamingInformation&
AppAllocateFlowRequestArrivedMessage::getDestAppName() const {
	return destAppName;
}

void AppAllocateFlowRequestArrivedMessage::setDestAppName(
		const ApplicationProcessNamingInformation& destAppName) {
	this->destAppName = destAppName;
}

const FlowSpecification&
AppAllocateFlowRequestArrivedMessage::getFlowSpecification() const {
	return flowSpecification;
}

void AppAllocateFlowRequestArrivedMessage::setFlowSpecification(
		const FlowSpecification& flowSpecification) {
	this->flowSpecification = flowSpecification;
}

const ApplicationProcessNamingInformation&
AppAllocateFlowRequestArrivedMessage::getSourceAppName() const {
	return sourceAppName;
}

void AppAllocateFlowRequestArrivedMessage::setSourceAppName(
		const ApplicationProcessNamingInformation& sourceAppName) {
	this->sourceAppName = sourceAppName;
}

int AppAllocateFlowRequestArrivedMessage::getPortId() const {
	return portId;
}

void AppAllocateFlowRequestArrivedMessage::setPortId(int portId) {
	this->portId = portId;
}

const ApplicationProcessNamingInformation&
AppAllocateFlowRequestArrivedMessage::getDifName() const {
	return difName;
}

void AppAllocateFlowRequestArrivedMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

IPCEvent* AppAllocateFlowRequestArrivedMessage::toIPCEvent(){
	FlowRequestEvent * event =
			new FlowRequestEvent(
					this->portId,
					this->flowSpecification,
					this->destAppName,
					this->sourceAppName,
					this->difName,
					this->getSequenceNumber());
	return event;
}

/* CLASS APP ALLOCATE FLOW RESPONSE MESSAGE */
AppAllocateFlowResponseMessage::AppAllocateFlowResponseMessage() :
				BaseNetlinkMessage(RINA_C_APP_ALLOCATE_FLOW_RESPONSE) {
	this->accept = false;
	this->notifySource = false;
}

const ApplicationProcessNamingInformation&
AppAllocateFlowResponseMessage::getDifName() const {
	return difName;
}

void AppAllocateFlowResponseMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

bool AppAllocateFlowResponseMessage::isAccept() const {
	return accept;
}

void AppAllocateFlowResponseMessage::setAccept(bool accept) {
	this->accept = accept;
}

const std::string& AppAllocateFlowResponseMessage::getDenyReason() const {
	return denyReason;
}

void AppAllocateFlowResponseMessage::setDenyReason(
		const std::string& denyReason) {
	this->denyReason = denyReason;
}

bool AppAllocateFlowResponseMessage::isNotifySource() const {
	return notifySource;
}

void AppAllocateFlowResponseMessage::setNotifySource(bool notifySource) {
	this->notifySource = notifySource;
}

/* CLASS APP DEALLOCATE FLOW REQUEST MESSAGE */
AppDeallocateFlowRequestMessage::AppDeallocateFlowRequestMessage() :
				NetlinkRequestOrNotificationMessage(RINA_C_APP_DEALLOCATE_FLOW_REQUEST) {
	this->portId = 0;
}

const ApplicationProcessNamingInformation&
AppDeallocateFlowRequestMessage::getApplicationName() const {
	return applicationName;
}

void AppDeallocateFlowRequestMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

int AppDeallocateFlowRequestMessage::getPortId() const {
	return portId;
}

void AppDeallocateFlowRequestMessage::setPortId(int portId) {
	this->portId = portId;
}

const ApplicationProcessNamingInformation&
AppDeallocateFlowRequestMessage::getDifName() const {
	return difName;
}

void AppDeallocateFlowRequestMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

IPCEvent* AppDeallocateFlowRequestMessage::toIPCEvent(){
	FlowDeallocateRequestEvent * event = new FlowDeallocateRequestEvent(
			portId, difName, applicationName, getDestIpcProcessId(),
			getSequenceNumber());
	return event;
}

/* CLASS APP DEALLOCATE FLOW RESPONSE MESSAGE */
AppDeallocateFlowResponseMessage::AppDeallocateFlowResponseMessage() :
				BaseNetlinkResponseMessage(RINA_C_APP_DEALLOCATE_FLOW_RESPONSE) {
}

const ApplicationProcessNamingInformation&
AppDeallocateFlowResponseMessage::getApplicationName() const {
	return applicationName;
}

void AppDeallocateFlowResponseMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

/* CLASS APP FLOW DEALLOCATED NOTIFICATION MESSAGE */
AppFlowDeallocatedNotificationMessage::AppFlowDeallocatedNotificationMessage() :
				NetlinkRequestOrNotificationMessage(
						RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION) {
	this->portId = 0;
	this->code = 0;
}

int AppFlowDeallocatedNotificationMessage::getCode() const {
	return code;
}

void AppFlowDeallocatedNotificationMessage::setCode(int code) {
	this->code = code;
}

int AppFlowDeallocatedNotificationMessage::getPortId() const {
	return portId;
}

void AppFlowDeallocatedNotificationMessage::setPortId(int portId) {
	this->portId = portId;
}

const std::string& AppFlowDeallocatedNotificationMessage::getReason() const {
	return reason;
}

void AppFlowDeallocatedNotificationMessage::setReason(
		const std::string& reason) {
	this->reason = reason;
}

const ApplicationProcessNamingInformation&
AppFlowDeallocatedNotificationMessage::getDifName() const {
	return difName;
}

void AppFlowDeallocatedNotificationMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

const ApplicationProcessNamingInformation&
AppFlowDeallocatedNotificationMessage::getApplicationName() const {
	return applicationName;
}

void AppFlowDeallocatedNotificationMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

IPCEvent* AppFlowDeallocatedNotificationMessage::toIPCEvent(){
	FlowDeallocatedEvent * event = new FlowDeallocatedEvent(
			portId, code, reason, difName);
	return event;
}

/* CLASS APP REGISTER APPLICATION REQUEST MESSAGE */
AppRegisterApplicationRequestMessage::AppRegisterApplicationRequestMessage() :
				NetlinkRequestOrNotificationMessage(
						RINA_C_APP_REGISTER_APPLICATION_REQUEST) {
}

const ApplicationProcessNamingInformation&
AppRegisterApplicationRequestMessage::getApplicationName() const {
	return applicationName;
}

void AppRegisterApplicationRequestMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const ApplicationRegistrationInformation&
AppRegisterApplicationRequestMessage::getApplicationRegistrationInformation()
const {
	return applicationRegistrationInformation;
}

void
AppRegisterApplicationRequestMessage::setApplicationRegistrationInformation(
		const ApplicationRegistrationInformation& appRegistrationInfo) {
	this->applicationRegistrationInformation = appRegistrationInfo;
}

IPCEvent* AppRegisterApplicationRequestMessage::toIPCEvent(){
	ApplicationRegistrationRequestEvent * event =
			new ApplicationRegistrationRequestEvent(
					applicationName,
					applicationRegistrationInformation,
					getSequenceNumber());

	return event;
}

/* CLASS APP REGISTER APPLICATION RESPONSE MESSAGE */
AppRegisterApplicationResponseMessage::AppRegisterApplicationResponseMessage() :
				BaseNetlinkResponseMessage(RINA_C_APP_REGISTER_APPLICATION_RESPONSE) {
}

const ApplicationProcessNamingInformation&
AppRegisterApplicationResponseMessage::getApplicationName() const {
	return applicationName;
}

void AppRegisterApplicationResponseMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const ApplicationProcessNamingInformation&
AppRegisterApplicationResponseMessage::getDifName() const {
	return difName;
}

void AppRegisterApplicationResponseMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

/* CLASS APP UNREGISTER APPLICATION REQUEST MESSAGE */
AppUnregisterApplicationRequestMessage::AppUnregisterApplicationRequestMessage() :
				NetlinkRequestOrNotificationMessage(
						RINA_C_APP_UNREGISTER_APPLICATION_REQUEST) {
}

const ApplicationProcessNamingInformation&
AppUnregisterApplicationRequestMessage::getApplicationName() const {
	return applicationName;
}

void AppUnregisterApplicationRequestMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const ApplicationProcessNamingInformation&
AppUnregisterApplicationRequestMessage::getDifName() const {
	return difName;
}

void AppUnregisterApplicationRequestMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

IPCEvent* AppUnregisterApplicationRequestMessage::toIPCEvent(){
	ApplicationUnregistrationRequestEvent * event =
			new ApplicationUnregistrationRequestEvent(
					applicationName,
					difName,
					getSequenceNumber());

	return event;
}


/* CLASS APP UNREGISTER APPLICATION RESPONSE MESSAGE */
AppUnregisterApplicationResponseMessage::AppUnregisterApplicationResponseMessage() :
				BaseNetlinkResponseMessage(RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE) {
}

const ApplicationProcessNamingInformation&
AppUnregisterApplicationResponseMessage::getApplicationName() const {
	return applicationName;
}

void AppUnregisterApplicationResponseMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}


/* CLASS APP REGISTRATION CANCELLED NOTIFICATION MESSAGE */
AppRegistrationCanceledNotificationMessage::AppRegistrationCanceledNotificationMessage() :
				NetlinkRequestOrNotificationMessage(
						RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION) {
	this->code = 0;
}

int AppRegistrationCanceledNotificationMessage::getCode() const {
	return code;
}

void AppRegistrationCanceledNotificationMessage::setCode(int code) {
	this->code = code;
}

const std::string& AppRegistrationCanceledNotificationMessage::getReason() const {
	return reason;
}

void AppRegistrationCanceledNotificationMessage::setReason(
		const std::string& reason) {
	this->reason = reason;
}

const ApplicationProcessNamingInformation&
AppRegistrationCanceledNotificationMessage::getDifName() const {
	return difName;
}

void AppRegistrationCanceledNotificationMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

const ApplicationProcessNamingInformation&
AppRegistrationCanceledNotificationMessage::getApplicationName() const {
	return applicationName;
}

void AppRegistrationCanceledNotificationMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

IPCEvent* AppRegistrationCanceledNotificationMessage::toIPCEvent(){
	AppRegistrationCanceledEvent * event = new AppRegistrationCanceledEvent(
			code, reason, difName, getSequenceNumber());
	return event;
}

/*CLASS APP GET DIF PROPERTIES REQUEST MESSAGE */
AppGetDIFPropertiesRequestMessage::AppGetDIFPropertiesRequestMessage():
		NetlinkRequestOrNotificationMessage(
				RINA_C_APP_GET_DIF_PROPERTIES_REQUEST){
}

const ApplicationProcessNamingInformation&
AppGetDIFPropertiesRequestMessage::getApplicationName() const {
	return applicationName;
}

void AppGetDIFPropertiesRequestMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const ApplicationProcessNamingInformation&
AppGetDIFPropertiesRequestMessage::getDifName() const {
	return difName;
}

void AppGetDIFPropertiesRequestMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

IPCEvent* AppGetDIFPropertiesRequestMessage::toIPCEvent(){
	GetDIFPropertiesRequestEvent * event =
			new GetDIFPropertiesRequestEvent(
					applicationName,
					difName,
					getSequenceNumber());

	return event;
}

/* CLASS APP GET DIF PROPERTIES RESPONSE MESSAGE */
AppGetDIFPropertiesResponseMessage::AppGetDIFPropertiesResponseMessage()
		:BaseNetlinkResponseMessage(RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE){
}

const ApplicationProcessNamingInformation&
AppGetDIFPropertiesResponseMessage::getApplicationName() const {
	return applicationName;
}

void AppGetDIFPropertiesResponseMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const std::list<DIFProperties>&
	AppGetDIFPropertiesResponseMessage::getDIFProperties() const{
	return difProperties;
}

void AppGetDIFPropertiesResponseMessage::setDIFProperties(
		const std::list<DIFProperties>& difProperties){
	this->difProperties = difProperties;
}

void AppGetDIFPropertiesResponseMessage::addDIFProperty(
		const DIFProperties& difProperties){
	this->difProperties.push_back(difProperties);
}

/* CLASS IPCM REGISTER APPLICATION REQUEST MESSAGE */
IpcmRegisterApplicationRequestMessage::IpcmRegisterApplicationRequestMessage():
				NetlinkRequestOrNotificationMessage(
						RINA_C_IPCM_REGISTER_APPLICATION_REQUEST) {
}

const ApplicationProcessNamingInformation&
IpcmRegisterApplicationRequestMessage::getApplicationName() const{
	return applicationName;
}

void IpcmRegisterApplicationRequestMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName){
	this->applicationName = applicationName;
}

const ApplicationProcessNamingInformation&
IpcmRegisterApplicationRequestMessage::getDifName() const{
	return difName;
}

void IpcmRegisterApplicationRequestMessage::setDifName(
		const ApplicationProcessNamingInformation& difName){
	this->difName = difName;
}

IPCEvent* IpcmRegisterApplicationRequestMessage::toIPCEvent(){
	ApplicationRegistrationInformation information =
		ApplicationRegistrationInformation(APPLICATION_REGISTRATION_SINGLE_DIF);
	information.setDIFName(difName);
	ApplicationRegistrationRequestEvent * event =
			new ApplicationRegistrationRequestEvent(
					applicationName,
					information,
					getSequenceNumber());

	return event;
}

/* CLASS IPCM REGISTER APPLICATION RESPONSE MESSAGE */
IpcmRegisterApplicationResponseMessage::IpcmRegisterApplicationResponseMessage() :
				BaseNetlinkResponseMessage(RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE) {
}

const ApplicationProcessNamingInformation&
IpcmRegisterApplicationResponseMessage::getApplicationName() const {
	return applicationName;
}

void IpcmRegisterApplicationResponseMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const ApplicationProcessNamingInformation&
IpcmRegisterApplicationResponseMessage::getDifName() const {
	return difName;
}

void IpcmRegisterApplicationResponseMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}


/* CLASS IPCM UNREGISTER APPLICATION REQUEST MESSAGE */
IpcmUnregisterApplicationRequestMessage::IpcmUnregisterApplicationRequestMessage() :
				NetlinkRequestOrNotificationMessage(
						RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST) {
}

const ApplicationProcessNamingInformation&
IpcmUnregisterApplicationRequestMessage::getApplicationName() const {
	return applicationName;
}

void IpcmUnregisterApplicationRequestMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const ApplicationProcessNamingInformation&
IpcmUnregisterApplicationRequestMessage::getDifName() const {
	return difName;
}

void IpcmUnregisterApplicationRequestMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

IPCEvent* IpcmUnregisterApplicationRequestMessage::toIPCEvent(){
	ApplicationUnregistrationRequestEvent * event =
			new ApplicationUnregistrationRequestEvent(
					applicationName,
					difName,
					getSequenceNumber());
	return event;
}

/* CLASS IPCM UNREGISTER APPLICATION RESPONSE MESSAGE */
IpcmUnregisterApplicationResponseMessage::
IpcmUnregisterApplicationResponseMessage() :
	BaseNetlinkResponseMessage(RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE) {
}


/* CLASS IPCM ASSIGN TO DIF REQUEST MESSAGE */
IpcmAssignToDIFRequestMessage::IpcmAssignToDIFRequestMessage():
				NetlinkRequestOrNotificationMessage(
						RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST) {
}

const DIFConfiguration&
IpcmAssignToDIFRequestMessage::getDIFConfiguration() const{
	return difconfiguration;
}

void IpcmAssignToDIFRequestMessage::setDIFConfiguration(
		const DIFConfiguration& difConfiguration){
	this->difconfiguration = difConfiguration;
}

IPCEvent* IpcmAssignToDIFRequestMessage::toIPCEvent(){
	AssignToDIFRequestEvent * event =
			new AssignToDIFRequestEvent(getDIFConfiguration(),
					getSequenceNumber());
	return event;
}

/* CLASS IPCM ASSIGN TO DIF RESPONSE MESSAGE */
IpcmAssignToDIFResponseMessage::IpcmAssignToDIFResponseMessage():
				BaseNetlinkResponseMessage(RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE) {
}

/* CLASS IPCM ALLOCATE FLOW REQUEST MESSAGE */
IpcmAllocateFlowRequestMessage::IpcmAllocateFlowRequestMessage():
						NetlinkRequestOrNotificationMessage(
								RINA_C_IPCM_ALLOCATE_FLOW_REQUEST) {
	portId = 0;
	applicationPortId = 0;
}

unsigned int IpcmAllocateFlowRequestMessage::getApplicationPortId() const {
	return applicationPortId;
}

void IpcmAllocateFlowRequestMessage::setApplicationPortId(
		unsigned int applicationPortId) {
	this->applicationPortId = applicationPortId;
}

const ApplicationProcessNamingInformation&
IpcmAllocateFlowRequestMessage::getDestAppName() const {
	return destAppName;
}

void IpcmAllocateFlowRequestMessage::setDestAppName(
		const ApplicationProcessNamingInformation& destAppName) {
	this->destAppName = destAppName;
}

const FlowSpecification&
IpcmAllocateFlowRequestMessage::getFlowSpec() const {
	return flowSpec;
}

void IpcmAllocateFlowRequestMessage::setFlowSpec(
		const FlowSpecification& flowSpec) {
	this->flowSpec = flowSpec;
}

int IpcmAllocateFlowRequestMessage::getPortId() const {
	return portId;
}

void IpcmAllocateFlowRequestMessage::setPortId(int portId) {
	this->portId = portId;
}

const ApplicationProcessNamingInformation&
IpcmAllocateFlowRequestMessage::getSourceAppName() const {
	return sourceAppName;
}

void IpcmAllocateFlowRequestMessage::setSourceAppName(
		const ApplicationProcessNamingInformation& sourceAppName) {
	this->sourceAppName = sourceAppName;
}

const ApplicationProcessNamingInformation&
IpcmAllocateFlowRequestMessage::getDifName() const {
	return difName;
}

void IpcmAllocateFlowRequestMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

IPCEvent* IpcmAllocateFlowRequestMessage::toIPCEvent(){
	FlowRequestEvent * event =
			new FlowRequestEvent(portId, flowSpec, sourceAppName, destAppName,
					difName, getSequenceNumber());
	return event;
}

/* CLASS IPCM ALLOCATE FLOW RESPONSE MESSAGE */
IpcmAllocateFlowResponseMessage::IpcmAllocateFlowResponseMessage():
				BaseNetlinkResponseMessage(RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE) {
}

/* CLASS IPCM IPC PROCESS REGISTERED TO DIF NOTIFICATION MESSAGE */
IpcmDIFRegistrationNotification::
IpcmDIFRegistrationNotification():NetlinkRequestOrNotificationMessage(
		RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION){
	registered = false;
}

const ApplicationProcessNamingInformation&
IpcmDIFRegistrationNotification::getDifName() const {
	return difName;
}

void IpcmDIFRegistrationNotification::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

const ApplicationProcessNamingInformation&
IpcmDIFRegistrationNotification::getIpcProcessName() const {
	return ipcProcessName;
}

void IpcmDIFRegistrationNotification::setIpcProcessName(
		const ApplicationProcessNamingInformation& ipcProcessName) {
	this->ipcProcessName = ipcProcessName;
}

void IpcmDIFRegistrationNotification::setRegistered(bool registered){
	this->registered = registered;
}

bool IpcmDIFRegistrationNotification::isRegistered() const{
	return registered;
}

IPCEvent* IpcmDIFRegistrationNotification::toIPCEvent(){
	IPCEvent * event;
	if (registered){
		event = new IPCProcessRegisteredToDIFEvent(ipcProcessName, difName,
					getSequenceNumber());
	}else{
		event = new IPCProcessUnregisteredFromDIFEvent(ipcProcessName, difName,
							getSequenceNumber());
	}

	return event;
}


/* CLASS QUERY RIB REQUEST MESSAGE */
IpcmDIFQueryRIBRequestMessage::IpcmDIFQueryRIBRequestMessage():
		NetlinkRequestOrNotificationMessage(RINA_C_IPCM_QUERY_RIB_REQUEST) {
	objectInstance = -1;
	scope = 0;
}

const std::string& IpcmDIFQueryRIBRequestMessage::getFilter() const {
	return filter;
}

void IpcmDIFQueryRIBRequestMessage::setFilter(const std::string& filter) {
	this->filter = filter;
}

const std::string& IpcmDIFQueryRIBRequestMessage::getObjectClass() const {
	return objectClass;
}

void IpcmDIFQueryRIBRequestMessage::setObjectClass(
		const std::string& objectClass) {
	this->objectClass = objectClass;
}

unsigned long IpcmDIFQueryRIBRequestMessage::getObjectInstance() const {
	return objectInstance;
}

void IpcmDIFQueryRIBRequestMessage::setObjectInstance(
		unsigned long objectInstance) {
	this->objectInstance = objectInstance;
}

const std::string&
IpcmDIFQueryRIBRequestMessage::getObjectName() const {
	return objectName;
}

void IpcmDIFQueryRIBRequestMessage::setObjectName(
		const std::string& objectName) {
	this->objectName = objectName;
}

unsigned int IpcmDIFQueryRIBRequestMessage::getScope() const {
	return scope;
}

void IpcmDIFQueryRIBRequestMessage::setScope(unsigned int scope) {
	this->scope = scope;
}

IPCEvent* IpcmDIFQueryRIBRequestMessage::toIPCEvent(){
	IPCEvent * event = new QueryRIBRequestEvent(objectClass, objectName,
			objectInstance, scope, filter, getSequenceNumber());
	return event;
}

/* CLASS QUERY RIB RESPONSE MESSAGE */
IpcmDIFQueryRIBResponseMessage::IpcmDIFQueryRIBResponseMessage()
	:BaseNetlinkResponseMessage(RINA_C_IPCM_QUERY_RIB_RESPONSE){
}

const std::list<RIBObject>&
	IpcmDIFQueryRIBResponseMessage::getRIBObjects() const{
	return ribObjects;
}

void IpcmDIFQueryRIBResponseMessage::setRIBObjects(
		const std::list<RIBObject>& ribObjects){
	this->ribObjects = ribObjects;
}

void IpcmDIFQueryRIBResponseMessage::addRIBObject(const RIBObject& ribObject){
	ribObjects.push_back(ribObject);
}

}

