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

std::string BaseNetlinkMessage::toString() {
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
					this->sourceAppName,
					this->destAppName,
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
		BaseNetlinkMessage(RINA_C_APP_DEALLOCATE_FLOW_RESPONSE) {
	this->result = 0;
}

const ApplicationProcessNamingInformation&
	AppDeallocateFlowResponseMessage::getApplicationName() const {
	return applicationName;
}

void AppDeallocateFlowResponseMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const std::string&
AppDeallocateFlowResponseMessage::getErrorDescription() const {
	return errorDescription;
}

void AppDeallocateFlowResponseMessage::setErrorDescription(
		const std::string& errorDescription) {
	this->errorDescription = errorDescription;
}

int AppDeallocateFlowResponseMessage::getResult() const {
	return result;
}

void AppDeallocateFlowResponseMessage::setResult(int result) {
	this->result = result;
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

const ApplicationProcessNamingInformation&
		AppRegisterApplicationRequestMessage::getDifName() const {
	return difName;
}

void AppRegisterApplicationRequestMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

IPCEvent* AppRegisterApplicationRequestMessage::toIPCEvent(){
	ApplicationRegistrationRequestEvent * event =
			new ApplicationRegistrationRequestEvent(
					applicationName,
					difName,
					getSequenceNumber());

	return event;
}

/* CLASS APP REGISTER APPLICATION RESPONSE MESSAGE */
AppRegisterApplicationResponseMessage::AppRegisterApplicationResponseMessage() :
		BaseNetlinkMessage(RINA_C_APP_REGISTER_APPLICATION_RESPONSE) {
	this->ipcProcessPortId = 0;
	this->ipcProcessId = 0;
	this->result = 0;
}

const ApplicationProcessNamingInformation&
	AppRegisterApplicationResponseMessage::getApplicationName() const {
		return applicationName;
	}

void AppRegisterApplicationResponseMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const std::string&
		AppRegisterApplicationResponseMessage::getErrorDescription() const {
	return errorDescription;
}

void AppRegisterApplicationResponseMessage::setErrorDescription(
		const std::string& errorDescription) {
	this->errorDescription = errorDescription;
}

unsigned int AppRegisterApplicationResponseMessage::getIpcProcessPortId() const {
	return ipcProcessPortId;
}

void AppRegisterApplicationResponseMessage::setIpcProcessPortId(
		unsigned int ipcProcessPortId) {
	this->ipcProcessPortId = ipcProcessPortId;
}

int AppRegisterApplicationResponseMessage::getResult() const {
	return result;
}

void AppRegisterApplicationResponseMessage::setResult(int result) {
	this->result = result;
}

const ApplicationProcessNamingInformation&
	AppRegisterApplicationResponseMessage::getDifName() const {
	return difName;
}

void AppRegisterApplicationResponseMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
}

unsigned short AppRegisterApplicationResponseMessage::getIpcProcessId() const{
	return ipcProcessId;
}

void AppRegisterApplicationResponseMessage::setIpcProcessId(
		unsigned short ipcProcessId){
	this->ipcProcessId = ipcProcessId;
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
		BaseNetlinkMessage(RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE) {
	this->result = 0;
}

const std::string&
AppUnregisterApplicationResponseMessage::getErrorDescription() const {
	return errorDescription;
}

void AppUnregisterApplicationResponseMessage::setErrorDescription(
		const std::string& errorDescription) {
	this->errorDescription = errorDescription;
}

int AppUnregisterApplicationResponseMessage::getResult() const {
	return result;
}

void AppUnregisterApplicationResponseMessage::setResult(int result) {
	this->result = result;
}


/* CLASS IPCM REGISTER APPLICATION REQUEST MESSAGE */
IpcmRegisterApplicationRequestMessage::IpcmRegisterApplicationRequestMessage():
		NetlinkRequestOrNotificationMessage(
				RINA_C_IPCM_REGISTER_APPLICATION_REQUEST) {
	applicationPortId = 0;
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

unsigned int
IpcmRegisterApplicationRequestMessage::getApplicationPortId() const {
	return applicationPortId;
}

void IpcmRegisterApplicationRequestMessage::setApplicationPortId(
		unsigned int applicationPortId) {
	this->applicationPortId = applicationPortId;
}

IPCEvent* IpcmRegisterApplicationRequestMessage::toIPCEvent(){
	ApplicationRegistrationRequestEvent * event =
			new ApplicationRegistrationRequestEvent(
					applicationName,
					difName,
					getSequenceNumber());

	return event;
}

/* CLASS IPCM REGISTER APPLICATION RESPONSE MESSAGE */
IpcmRegisterApplicationResponseMessage::IpcmRegisterApplicationResponseMessage() :
		BaseNetlinkMessage(RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE) {
	this->result = 0;
}

const ApplicationProcessNamingInformation&
IpcmRegisterApplicationResponseMessage::getApplicationName() const {
		return applicationName;
	}

void IpcmRegisterApplicationResponseMessage::setApplicationName(
		const ApplicationProcessNamingInformation& applicationName) {
	this->applicationName = applicationName;
}

const std::string&
IpcmRegisterApplicationResponseMessage::getErrorDescription() const {
	return errorDescription;
}

void IpcmRegisterApplicationResponseMessage::setErrorDescription(
		const std::string& errorDescription) {
	this->errorDescription = errorDescription;
}

int IpcmRegisterApplicationResponseMessage::getResult() const {
	return result;
}

void IpcmRegisterApplicationResponseMessage::setResult(int result) {
	this->result = result;
}

const ApplicationProcessNamingInformation&
IpcmRegisterApplicationResponseMessage::getDifName() const {
	return difName;
}

void IpcmRegisterApplicationResponseMessage::setDifName(
		const ApplicationProcessNamingInformation& difName) {
	this->difName = difName;
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
		BaseNetlinkMessage(RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE) {
	result = 0;
}

int IpcmAssignToDIFResponseMessage::getResult() const{
	return result;
}

void IpcmAssignToDIFResponseMessage::setResult(int result){
	this->result = result;
}

const std::string& IpcmAssignToDIFResponseMessage::getErrorDescription() const{
	return errorDescription;
}

void IpcmAssignToDIFResponseMessage::setErrorDescription(
		const std::string& errorDescription){
	this->errorDescription = errorDescription;
}

}

