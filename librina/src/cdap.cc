/// cdap.cc
/// Created on: May 22, 2014
/// Author: bernat

#include "cdap.h"

namespace rina {

// CLASS CDAPSessionImpl
CDAPSessionImpl::CDAPSessionImpl(CDAPSessionManager *cdap_session_manager, long timeout, WireMessageProviderInterface *wire_message_provider) {
	cdap_session_manager_ = cdap_session_manager;
	connection_state_machine_ = ConnectionStateMachine(this, timeout);
	wire_message_provider_ = wire_message_provider;
}

// CLASS CDAPSessionManager
CDAPSessionManager::CDAPSessionManager() {
	throw CDAPException("Not allowed default constructor of CDAPSessionManager has been called.");
}
CDAPSessionManager::CDAPSessionManager(	WireMessageProviderFactoryInterface &arg0) {
	wire_message_provider_factory_ = arg0;
	timeout_ = DEFAULT_TIMEOUT_IN_MS;
}
CDAPSessionManager::CDAPSessionManager(	WireMessageProviderFactoryInterface &arg0, long arg1) {
	wire_message_provider_factory_ = arg0;
	timeout_ = arg1;
}
CDAPSession* CDAPSessionManager::createCDAPSession(int port_id) {
	if (cdap_sessions_.find(port_id) != cdap_sessions_.end()) {
		return cdap_sessions_.find(port_id)->second;
	}
	else
	{
		// TODO: Implement DELETE
		CDAPSessionImpl *cdapSession = new cdapSession(this, timeout_, wire_message_provider_factory_->createWireMessageProvider());
		CDAPSessionDescriptor descriptor = new CDAPSessionDescriptor(port_id);
		cdapSession->setSessionDescriptor(descriptor);
		cdap_sessions_.insert(std::pair<int,CDAPSession*>(port_id, cdapSession));
		return cdapSession;
	}
}
CDAPSessionManager::~CDAPSessionManager() throw() {
	cdap_sessions_.erase(cdap_sessions_.begin(), cdap_sessions_.end());
}
/*
int* getAllCDAPSessionIds();
std::list<CDAPSession> getAllCDAPSessions();
CDAPSession* getCDAPSession(int port_id);
char* encodeCDAPMessage(CDAPMessage cdap_message) throw (CDAPException);
CDAPMessage decodeCDAPMessage(char cdap_message[]) throw (CDAPException);
void removeCDAPSession(int portId);
char* encodeNextMessageToBeSent(CDAPMessage cdap_message, int portId)
		throw (CDAPException);
CDAPMessage messageReceived(char encodedCDAPMessage[], int portId)
		throw (CDAPException);
void messageSent(CDAPMessage cdap_message, int portId) throw (CDAPException);
int getPortId(std::string destinationApplicationProcessName)
		throw (CDAPException);
CDAPMessage getOpenConnectionRequestMessage(int portId,
		CDAPMessage::AuthTypes authMech, AuthValue authValue,
		std::string destAEInst, std::string destAEName,
		std::string destApInst, std::string destApName,
		std::string srcAEInst, std::string srcAEName, std::string srcApInst,
		std::string srcApName) throw (CDAPException);
CDAPMessage getOpenConnectionResponseMessage(int portId,
		CDAPMessage::AuthTypes authMech, AuthValue authValue,
		std::string destAEInst, std::string destAEName,
		std::string destApInst, std::string destApName, int result,
		std::string resultReason, std::string srcAEInst,
		std::string srcAEName, std::string srcApInst, std::string srcApName,
		int invokeId) throw (CDAPException);
CDAPMessage getReleaseConnectionRequestMessage(int portId,
		CDAPMessage::Flags flags, bool invokeID) throw (CDAPException);
CDAPMessage getReleaseConnectionResponseMessage(int portId,
		CDAPMessage::Flags flags, int result, std::string resultReason,
		int invokeId) throw (CDAPException);
CDAPMessage getCreateObjectRequestMessage(int portId, char filter[],
		CDAPMessage::Flags flags, std::string objClass, long objInst,
		std::string objName, ObjectValueInterface &objValue, int scope,
		bool invokeId) throw (CDAPException);
CDAPMessage getCreateObjectResponseMessage(int portId,
		CDAPMessage::Flags flags, std::string objClass, long objInst,
		std::string objName, ObjectValueInterface &objValue, int result,
		std::string resultReason, int invokeId) throw (CDAPException);
CDAPMessage getDeleteObjectRequestMessage(int portId, char filter[],
		CDAPMessage::Flags flags, std::string objClass, long objInst,
		std::string objName, ObjectValueInterface &objectValue, int scope,
		bool invokeId) throw (CDAPException);
CDAPMessage getDeleteObjectResponseMessage(int portId,
		CDAPMessage::Flags flags, std::string objClass, long objInst,
		std::string objName, int result, std::string resultReason,
		int invokeId) throw (CDAPException);
CDAPMessage getStartObjectRequestMessage(int portId, char filter[],
		CDAPMessage::Flags flags, std::string objClass,
		ObjectValueInterface &objValue, long objInst, std::string objName,
		int scope, bool invokeId) throw (CDAPException);
CDAPMessage getStartObjectResponseMessage(int portId,
		CDAPMessage::Flags flags, int result, std::string resultReason,
		int invokeId) throw (CDAPException);
CDAPMessage getStartObjectResponseMessage(int portId,
		CDAPMessage::Flags flags, std::string objClass,
		ObjectValueInterface &objValue, long objInst, std::string objName,
		int result, std::string resultReason, int invokeId)
				throw (CDAPException);
CDAPMessage getStopObjectRequestMessage(int portId, char filter[],
		CDAPMessage::Flags flags, std::string objClass,
		ObjectValueInterface &objValue, long objInst, std::string objName,
		int scope, bool invokeId) throw (CDAPException);
CDAPMessage getStopObjectResponseMessage(int portId,
		CDAPMessage::Flags flags, int result, std::string resultReason,
		int invokeId) throw (CDAPException);
CDAPMessage getReadObjectRequestMessage(int portId, char filter[],
		CDAPMessage::Flags flags, std::string objClass, long objInst,
		std::string objName, int scope, bool invokeId) throw (CDAPException);
CDAPMessage getReadObjectResponseMessage(int portId,
		CDAPMessage::Flags flags, std::string objClass, long objInst,
		std::string objName, ObjectValueInterface &objValue, int result,
		std::string resultReason, int invokeId) throw (CDAPException);
CDAPMessage getWriteObjectRequestMessage(int portId, char filter[],
		CDAPMessage::Flags flags, std::string objClass, long objInst,
		ObjectValueInterface &objValue, std::string objName, int scope,
		bool invokeId) throw (CDAPException);
CDAPMessage getWriteObjectResponseMessage(int portId,
		CDAPMessage::Flags flags, int result, std::string resultReason,
		int invokeId) throw (CDAPException);
CDAPMessage getCancelReadRequestMessage(int portId,
		CDAPMessage::Flags flags, int invokeID) throw (CDAPException);
CDAPMessage getCancelReadResponseMessage(int portId,
		CDAPMessage::Flags flags, int invokeID, int result,
		std::string resultReason) throw (CDAPException);
		*/
}
