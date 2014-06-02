/// cdap.cc
/// Created on: May 22, 2014
/// Author: bernat

#include "cdap.h"

namespace rina {

// CLASS ConnectionStateMachine
ConnectionStateMachine::ConnectionStateMachine(
		CDAPSessionInterface *cdap_session, long timeout) {
	cdap_session_ = cdap_session;
	timeout_ = timeout;
}
bool ConnectionStateMachine::is_connected() const {
	return connection_state_ == CONNECTED;
}

// CLASS CDAPSessionInvokeIdManagerImpl
void CDAPSessionInvokeIdManagerImpl::freeInvokeId(int invoke_id) {
	used_invoke_ids_.remove(invoke_id);
}
int CDAPSessionInvokeIdManagerImpl::newInvokeId() {
	int candidate = 1;
	while (std::find(used_invoke_ids_.begin(), used_invoke_ids_.end(),
			candidate) != used_invoke_ids_.end()) {
		candidate = candidate + 1;
	}
	used_invoke_ids_.push_back(candidate);
	return candidate;
}
void CDAPSessionInvokeIdManagerImpl::reserveInvokeId(int invoke_id) {
	used_invoke_ids_.push_back(invoke_id);
}

// CLASS CDAPSessionImpl
CDAPSessionImpl::CDAPSessionImpl(CDAPSessionManager *cdap_session_manager,
		long timeout, WireMessageProviderInterface *wire_message_provider) {
	cdap_session_manager_ = cdap_session_manager;
	connection_state_machine_ = new ConnectionStateMachine(this, timeout);
	wire_message_provider_ = wire_message_provider;
	invoke_id_manager_ = new CDAPSessionInvokeIdManagerImpl();
}

CDAPSessionImpl::~CDAPSessionImpl() throw () {
	delete connection_state_machine_;
	delete session_descriptor_;
	delete invoke_id_manager_;
	pending_messages_.clear();
	cancel_read_pending_messages_.clear();
}
const char* CDAPSessionImpl::encodeNextMessageToBeSent(
		const CDAPMessage &cdap_message) {
	CDAPMessageValidator::validate(cdap_message);

	switch (cdap_message.get_op_code()) {
	case CDAPMessage::M_CONNECT:
		connection_state_machine_->checkConnect();
		break;
	case CDAPMessage::M_CONNECT_R:
		connection_state_machine_->checkConnectResponse();
		break;
	case CDAPMessage::M_RELEASE:
		connection_state_machine_->checkRelease();
		break;
	case CDAPMessage::M_RELEASE_R:
		connection_state_machine_->checkReleaseResponse();
		break;
	case CDAPMessage::M_CREATE:
		checkIsConnected();
		checkInvokeIdNotExists(cdap_message);
		break;
	case CDAPMessage::M_CREATE_R:
		checkIsConnected();
		checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_CREATE,
				true);
		break;
	case CDAPMessage::M_DELETE:
		checkIsConnected();
		checkInvokeIdNotExists(cdap_message);
		break;
	case CDAPMessage::M_DELETE_R:
		checkIsConnected();
		checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_DELETE,
				true);
		break;
	case CDAPMessage::M_START:
		checkIsConnected();
		checkInvokeIdNotExists(cdap_message);
		break;
	case CDAPMessage::M_START_R:
		checkIsConnected();
		checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_START, true);
		break;
	case CDAPMessage::M_STOP:
		checkIsConnected();
		checkInvokeIdNotExists(cdap_message);
		break;
	case CDAPMessage::M_STOP_R:
		checkIsConnected();
		checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_STOP, true);
		break;
	case CDAPMessage::M_WRITE:
		checkIsConnected();
		checkInvokeIdNotExists(cdap_message);
		break;
	case CDAPMessage::M_WRITE_R:
		checkIsConnected();
		checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_WRITE, true);
		break;
	case CDAPMessage::M_READ:
		checkIsConnected();
		checkInvokeIdNotExists(cdap_message);
		break;
	case CDAPMessage::M_READ_R:
		checkIsConnected();
		checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_READ, true);
		break;
	case CDAPMessage::M_CANCELREAD:
		checkIsConnected();
		checkCanSendOrReceiveCancelReadRequest(cdap_message, true);
		break;
	case CDAPMessage::M_CANCELREAD_R:
		checkIsConnected();
		checkCanSendOrReceiveCancelReadResponse(cdap_message, true);
		break;
	default:
		std::stringstream ss;
		ss << "Unrecognized operation code: " << cdap_message.get_op_code();
		throw CDAPException(ss.str());
	}

	return serializeMessage(cdap_message);
}
void CDAPSessionImpl::messageSent(CDAPMessage &cdap_message) {
	messageSentOrReceived(cdap_message, true);
}
const CDAPMessage* CDAPSessionImpl::messageReceived(const char message[]) {
	const CDAPMessage *cdap_message = deserializeMessage(message);
	messageSentOrReceived(*cdap_message, false);
	return cdap_message;
}
void CDAPSessionImpl::messageReceived(CDAPMessage &cdap_message) {
	messageSentOrReceived(cdap_message, false);
}
void CDAPSessionImpl::set_session_descriptor(
		CDAPSessionDescriptor *session_descriptor) {
	session_descriptor_ = session_descriptor;
}
CDAPSessionDescriptor* CDAPSessionImpl::get_session_descriptor() const {
	return session_descriptor_;
}
void CDAPSessionImpl::messageSentOrReceived(const CDAPMessage &cdap_message,
		bool sent) {
	switch (cdap_message.get_op_code()) {
	case CDAPMessage::M_CONNECT:
		connection_state_machine_->connectSentOrReceived(cdap_message, sent);
		populateSessionDescriptor(cdap_message, sent);
		break;
	case CDAPMessage::M_CONNECT_R:
		connection_state_machine_->connectResponseSentOrReceived(cdap_message,
				sent);
		break;
	case CDAPMessage::M_RELEASE:
		connection_state_machine_->releaseSentOrReceived(cdap_message, sent);
		break;
	case CDAPMessage::M_RELEASE_R:
		connection_state_machine_->releaseResponseSentOrReceived(cdap_message,
				sent);
		emptySessionDescriptor();
		break;
	case CDAPMessage::M_CREATE:
		requestMessageSentOrReceived(cdap_message, CDAPMessage::M_CREATE, sent);
		break;
	case CDAPMessage::M_CREATE_R:
		responseMessageSentOrReceived(cdap_message, CDAPMessage::M_CREATE,
				sent);
		break;
	case CDAPMessage::M_DELETE:
		requestMessageSentOrReceived(cdap_message, CDAPMessage::M_DELETE, sent);
		break;
	case CDAPMessage::M_DELETE_R:
		responseMessageSentOrReceived(cdap_message, CDAPMessage::M_DELETE,
				sent);
		break;
	case CDAPMessage::M_START:
		requestMessageSentOrReceived(cdap_message, CDAPMessage::M_START, sent);
		break;
	case CDAPMessage::M_START_R:
		responseMessageSentOrReceived(cdap_message, CDAPMessage::M_START, sent);
		break;
	case CDAPMessage::M_STOP:
		requestMessageSentOrReceived(cdap_message, CDAPMessage::M_STOP, sent);
		break;
	case CDAPMessage::M_STOP_R:
		responseMessageSentOrReceived(cdap_message, CDAPMessage::M_STOP, sent);
		break;
	case CDAPMessage::M_WRITE:
		requestMessageSentOrReceived(cdap_message, CDAPMessage::M_WRITE, sent);
		break;
	case CDAPMessage::M_WRITE_R:
		responseMessageSentOrReceived(cdap_message, CDAPMessage::M_WRITE, sent);
		break;
	case CDAPMessage::M_READ:
		requestMessageSentOrReceived(cdap_message, CDAPMessage::M_READ, sent);
		break;
	case CDAPMessage::M_READ_R:
		responseMessageSentOrReceived(cdap_message, CDAPMessage::M_READ, sent);
		break;
	case CDAPMessage::M_CANCELREAD:
		cancelReadMessageSentOrReceived(cdap_message, sent);
		break;
	case CDAPMessage::M_CANCELREAD_R:
		cancelReadResponseMessageSentOrReceived(cdap_message, sent);
		break;
	default:
		std::stringstream ss;
		ss << "Unrecognized operation code: " << cdap_message.get_op_code();
		throw CDAPException(ss.str());
	}
	CDAPMessageValidator::validate(cdap_message);
	freeOrReserveInvokeId(cdap_message);
}
void CDAPSessionImpl::freeOrReserveInvokeId(const CDAPMessage &cdap_message) {
	CDAPMessage::Opcode op_code = cdap_message.get_op_code();
	if (op_code == CDAPMessage::M_CONNECT_R
			|| op_code == CDAPMessage::M_RELEASE_R
			|| op_code == CDAPMessage::M_CREATE_R
			|| op_code == CDAPMessage::M_DELETE_R
			|| op_code == CDAPMessage::M_START_R
			|| op_code == CDAPMessage::M_STOP_R
			|| op_code == CDAPMessage::M_WRITE_R
			|| op_code == CDAPMessage::M_CANCELREAD_R
			|| (op_code == CDAPMessage::M_READ_R
					&& cdap_message.get_flags() == CDAPMessage::NONE_FLAGS)
			|| cdap_message.get_flags() != CDAPMessage::F_RD_INCOMPLETE) {
		invoke_id_manager_->freeInvokeId(cdap_message.get_invoke_id());
	}

	if (cdap_message.get_invoke_id() != 0) {
		if (op_code == CDAPMessage::M_CONNECT
				|| op_code == CDAPMessage::M_RELEASE
				|| op_code == CDAPMessage::M_CREATE
				|| op_code == CDAPMessage::M_DELETE
				|| op_code == CDAPMessage::M_START
				|| op_code == CDAPMessage::M_STOP
				|| op_code == CDAPMessage::M_WRITE
				|| op_code == CDAPMessage::M_CANCELREAD
				|| op_code == CDAPMessage::M_READ) {
			invoke_id_manager_->reserveInvokeId(cdap_message.get_invoke_id());
		}
	}
}
void CDAPSessionImpl::checkIsConnected() const {
	if (!connection_state_machine_->is_connected()) {
		throw CDAPException(
				"Cannot send a message because the CDAP session is not in CONNECTED state");
	}
}
void CDAPSessionImpl::checkInvokeIdNotExists(
		const CDAPMessage &cdap_message) const {
	if (pending_messages_.find(cdap_message.get_invoke_id())
			== pending_messages_.end()) {
		std::stringstream ss;
		ss << cdap_message.get_invoke_id();
		throw CDAPException("The invokeid " + ss.str() + " is already in use");
	}
}
void CDAPSessionImpl::checkCanSendOrReceiveCancelReadRequest(
		const CDAPMessage &cdap_message, bool sender) const {
	bool validationFailed = false;
	if (pending_messages_.find(cdap_message.get_invoke_id())
			!= pending_messages_.end()) {
		CDAPOperationState *state = pending_messages_.find(
				cdap_message.get_invoke_id())->second;
		if (state->get_op_code() == CDAPMessage::M_READ) {
			validationFailed = true;
		}
		if (sender && !state->is_sender()) {
			validationFailed = true;
		}
		if (!sender && state->is_sender()) {
			validationFailed = true;
		}
		if (validationFailed) {
			std::stringstream ss;
			ss << cdap_message.get_invoke_id();
			throw CDAPException(
					"Cannot set an M_CANCELREAD message because there is no READ transaction associated to the invoke id "
							+ ss.str());
		}
	} else {
		std::stringstream ss;
		ss << cdap_message.get_invoke_id();
		throw CDAPException(
				"Cannot set an M_CANCELREAD message because there is no READ transaction associated to the invoke id "
						+ ss.str());
	}
}
void CDAPSessionImpl::requestMessageSentOrReceived(
		const CDAPMessage &cdap_message, CDAPMessage::Opcode op_code,
		bool sent) {
	checkIsConnected();
	checkInvokeIdNotExists(cdap_message);

	if (cdap_message.get_invoke_id() != 0) {
		CDAPOperationState *new_operation_state = new CDAPOperationState(
				op_code, sent);
		pending_messages_.insert(
				std::pair<int, CDAPOperationState*>(
						cdap_message.get_invoke_id(), new_operation_state));
	}
}
void CDAPSessionImpl::cancelReadMessageSentOrReceived(
		const CDAPMessage &cdap_message, bool sender) {
	checkCanSendOrReceiveCancelReadRequest(cdap_message, sender);
	CDAPOperationState *new_operation_state = new CDAPOperationState(
			CDAPMessage::M_CANCELREAD, sender);
	cancel_read_pending_messages_.insert(
			std::pair<int, CDAPOperationState*>(cdap_message.get_invoke_id(),
					new_operation_state));
}
void CDAPSessionImpl::checkCanSendOrReceiveResponse(
		const CDAPMessage &cdap_message, CDAPMessage::Opcode op_code,
		bool send) const {
	bool validation_failed = false;
	if (pending_messages_.find(cdap_message.get_invoke_id())
			== pending_messages_.map::end()) {
		std::stringstream ss;
		ss << "Cannot send a response for the " << op_code
				<< " operation with invokeId " << cdap_message.get_invoke_id();
		throw CDAPException(ss.str());
	}
	CDAPOperationState* state = pending_messages_.find(
			cdap_message.get_invoke_id())->second;
	if (state->get_op_code() != op_code) {
		validation_failed = true;
	}
	if (send && state->is_sender()) {
		validation_failed = true;
	}
	if (!send && !state->is_sender()) {
		validation_failed = true;
	}
	if (validation_failed) {
		std::stringstream ss;
		ss << "Cannot send a response for the " << op_code
				<< " operation with invokeId " << cdap_message.get_invoke_id();
		throw CDAPException(ss.str());
	}
}
void CDAPSessionImpl::checkCanSendOrReceiveCancelReadResponse(
		const CDAPMessage &cdap_message, bool send) const {
	bool validation_failed = false;

	if (cancel_read_pending_messages_.find(cdap_message.get_invoke_id())
			== cancel_read_pending_messages_.end()) {
		std::stringstream ss;
		ss << "Cannot send a response for the " << CDAPMessage::M_CANCELREAD
				<< " operation with invokeId " << cdap_message.get_invoke_id();
		throw CDAPException(ss.str());
	}
	CDAPOperationState *state = cancel_read_pending_messages_.find(
			cdap_message.get_invoke_id())->second;
	if (state->get_op_code() != CDAPMessage::M_CANCELREAD) {
		validation_failed = true;
	}
	if (send && state->is_sender()) {
		validation_failed = true;
	}
	if (!send && !state->is_sender()) {
		validation_failed = true;
	}
	if (validation_failed) {
		std::stringstream ss;
		ss << "Cannot send a response for the " << CDAPMessage::M_CANCELREAD
				<< " operation with invokeId " << cdap_message.get_invoke_id();
		throw CDAPException(ss.str());
	}
}
void CDAPSessionImpl::responseMessageSentOrReceived(
		const CDAPMessage &cdap_message, CDAPMessage::Opcode op_code,
		bool sent) {
	checkIsConnected();
	checkCanSendOrReceiveResponse(cdap_message, op_code, sent);
	bool operation_complete = true;
	if (op_code != CDAPMessage::M_READ) {
		CDAPMessage::Flags flags = cdap_message.get_flags();
		if (flags != CDAPMessage::NONE_FLAGS
				&& flags != CDAPMessage::F_RD_INCOMPLETE) {
			operation_complete = false;
		}
	}
	if (operation_complete) {
		pending_messages_.erase(cdap_message.get_invoke_id());
	}
	// check for M_READ_R and M_CANCELREAD race condition
	if (!sent) {
		if (op_code != CDAPMessage::M_READ) {
			cancel_read_pending_messages_.erase(cdap_message.get_invoke_id());
		}
	}
}
void CDAPSessionImpl::cancelReadResponseMessageSentOrReceived(
		const CDAPMessage &cdap_message, bool sent) {
	checkIsConnected();
	checkCanSendOrReceiveCancelReadResponse(cdap_message, sent);
	cancel_read_pending_messages_.erase(cdap_message.get_invoke_id());
	pending_messages_.erase(cdap_message.get_invoke_id());
}
const char* CDAPSessionImpl::serializeMessage(
		const CDAPMessage &cdap_message) const {
	return wire_message_provider_->serializeMessage(cdap_message);
}
const CDAPMessage* CDAPSessionImpl::deserializeMessage(
		const char message[]) const {
	return wire_message_provider_->deserializeMessage(message);
}
void CDAPSessionImpl::populateSessionDescriptor(const CDAPMessage &cdap_message,
		bool send) {
	delete session_descriptor_;
	session_descriptor_ = new CDAPSessionDescriptor(
			cdap_message.get_abs_syntax(), cdap_message.get_auth_mech(),
			cdap_message.get_auth_value());

	if (send) {
		session_descriptor_->set_dest_ae_inst(cdap_message.get_dest_ae_inst());
		session_descriptor_->set_dest_ae_name(cdap_message.get_dest_ae_name());
		session_descriptor_->set_dest_ap_inst(cdap_message.get_dest_ap_inst());
		session_descriptor_->set_dest_ap_name(cdap_message.get_dest_ap_name());
		session_descriptor_->set_src_ae_inst(cdap_message.get_src_ae_inst());
		session_descriptor_->set_src_ae_name(cdap_message.get_src_ae_name());
		session_descriptor_->set_src_ap_inst(cdap_message.get_src_ap_inst());
		session_descriptor_->set_src_ap_name(cdap_message.get_src_ap_name());
	} else {
		session_descriptor_->set_dest_ae_inst(cdap_message.get_src_ae_inst());
		session_descriptor_->set_dest_ae_name(cdap_message.get_src_ae_name());
		session_descriptor_->set_dest_ap_inst(cdap_message.get_src_ap_inst());
		session_descriptor_->set_dest_ap_name(cdap_message.get_src_ap_name());
		session_descriptor_->set_src_ae_inst(cdap_message.get_dest_ae_inst());
		session_descriptor_->set_src_ae_name(cdap_message.get_dest_ae_name());
		session_descriptor_->set_src_ap_inst(cdap_message.get_dest_ae_name());
		session_descriptor_->set_src_ap_name(cdap_message.get_dest_ap_name());
	}
	session_descriptor_->set_version(cdap_message.get_version());
}
void CDAPSessionImpl::emptySessionDescriptor() {
	CDAPSessionDescriptor *new_session = new CDAPSessionDescriptor(
			session_descriptor_->get_port_id());
	new_session->set_ap_naming_info(session_descriptor_->get_ap_naming_info());
	delete session_descriptor_;
	session_descriptor_ = new_session;
}

// CLASS CDAPSessionManager
CDAPSessionManager::CDAPSessionManager() {
	throw CDAPException(
			"Not allowed default constructor of CDAPSessionManager has been called.");
}
CDAPSessionManager::CDAPSessionManager(
		WireMessageProviderFactoryInterface *arg0) {
	wire_message_provider_factory_ = arg0;
	timeout_ = DEFAULT_TIMEOUT_IN_MS;
}
CDAPSessionManager::CDAPSessionManager(
		WireMessageProviderFactoryInterface *arg0, long arg1) {
	wire_message_provider_factory_ = arg0;
	timeout_ = arg1;
}
CDAPSessionInterface* CDAPSessionManager::createCDAPSession(int port_id) {
	if (cdap_sessions_.find(port_id) != cdap_sessions_.end()) {
		return cdap_sessions_.find(port_id)->second;
	} else {
		CDAPSessionImpl *cdap_session = new CDAPSessionImpl(this, timeout_,
				wire_message_provider_factory_->createWireMessageProvider());
		CDAPSessionDescriptor *descriptor = new CDAPSessionDescriptor(port_id);
		cdap_session->set_session_descriptor(descriptor);
		cdap_sessions_.insert(
				std::pair<int, CDAPSessionInterface*>(port_id, cdap_session));
		return cdap_session;
	}
}
CDAPSessionManager::~CDAPSessionManager() throw () {
	cdap_sessions_.clear();
}
void CDAPSessionManager::getAllCDAPSessionIds(std::vector<int> &vector) {
	vector.clear();
	for (std::map<int, CDAPSessionImpl*>::iterator it =
			cdap_sessions_.begin(); it != cdap_sessions_.end(); ++it) {
		vector.push_back(it->first);
	}
}
void CDAPSessionManager::getAllCDAPSessions(
		std::vector<CDAPSessionInterface*> &vector) {
	vector.clear();
	for (std::map<int, CDAPSessionImpl*>::iterator it =
			cdap_sessions_.begin(); it != cdap_sessions_.end(); ++it) {
		vector.push_back(it->second);
	}
}
CDAPSessionImpl* CDAPSessionManager::getCDAPSession(int port_id) {
	std::map<int, CDAPSessionImpl*>::iterator itr = cdap_sessions_.find(
			port_id);
	if (itr != cdap_sessions_.end())
		return cdap_sessions_.find(port_id)->second;
	else
		return 0;
}
const char* CDAPSessionManager::encodeCDAPMessage(
		const CDAPMessage &cdap_message) {
	return wire_message_provider_->serializeMessage(cdap_message);
}
const CDAPMessage* CDAPSessionManager::decodeCDAPMessage(char cdap_message[]) {
	return wire_message_provider_->deserializeMessage(cdap_message);
}
void CDAPSessionManager::removeCDAPSession(int portId) {
	cdap_sessions_.erase(portId);
}

const char* CDAPSessionManager::encodeNextMessageToBeSent(
		const CDAPMessage &cdap_message, int port_id) {
	std::map<int, CDAPSessionImpl*>::iterator it = cdap_sessions_.find(
			port_id);
	CDAPSessionInterface* cdap_session;

	if (it == cdap_sessions_.end()) {
		if (cdap_message.get_op_code() == CDAPMessage::M_CONNECT) {
			cdap_session = createCDAPSession(port_id);
		} else {
			std::stringstream ss;
			ss
					<< "There are no open CDAP sessions associated to the flow identified by "
					<< port_id << " right now";
			throw CDAPException(ss.str());
		}
	} else {
		cdap_session = it->second;
	}
	return cdap_session->encodeNextMessageToBeSent(cdap_message);
}

const CDAPMessage* CDAPSessionManager::messageReceived(
		char encoded_cdap_message[], int port_id) {
	const CDAPMessage *cdap_message = decodeCDAPMessage(encoded_cdap_message);
	CDAPSessionImpl *cdap_session = getCDAPSession(port_id);
	if (cdap_session != 0) {
		std::stringstream ss;
		ss << "Received CDAP message from "
				<< cdap_session->get_session_descriptor()->get_destination_application_process_naming_info()
				<< " through underlying portId " << port_id
				<< ". Decoded contents: " << cdap_message->toString();
		LOG_DBG( ss.str() );
	} else {
		std::stringstream ss;
		ss << "Received CDAP message from " << cdap_message->get_dest_ap_name() << " through underlying portId " << port_id <<
				". Decoded contents: " << cdap_message->toString();

		LOG_DBG( ss.str());
	}
	switch (cdap_message->get_op_code()) {
	case CDAPMessage::M_CONNECT:
		if (cdap_session == cdap_sessions_.end()) {
			cdap_session = createCDAPSession(port_id);
			cdap_session->messageReceived(cdap_message);
			LOG_DBG("Created a new CDAP session for port "+port_id);
		} else {
			throw CDAPException(
					"M_CONNECT received on an already open CDAP Session, over flow "
							+ port_id);
		}
		break;
	default:
		if (cdap_session != cdap_sessions_.end()) {
			cdap_session->messageReceived(cdap_message);
		} else {
			throw CDAPException(
					"Receive a " + cdap_message.get_op_code()
							+ " CDAP message on a CDAP session that is not open, over flow "
							+ port_id);
		}
		break;
	}
	return cdap_message;
}
/*
 void messageSent(CDAPMessage cdap_message, int portId);
 int getPortId(std::string destinationApplicationProcessName)
 ;
 CDAPMessage getOpenConnectionRequestMessage(int portId,
 CDAPMessage::AuthTypes authMech, AuthValue authValue,
 std::string destAEInst, std::string destAEName,
 std::string destApInst, std::string destApName,
 std::string srcAEInst, std::string srcAEName, std::string srcApInst,
 std::string srcApName);
 CDAPMessage getOpenConnectionResponseMessage(int portId,
 CDAPMessage::AuthTypes authMech, AuthValue authValue,
 std::string destAEInst, std::string destAEName,
 std::string destApInst, std::string destApName, int result,
 std::string resultReason, std::string srcAEInst,
 std::string srcAEName, std::string srcApInst, std::string srcApName,
 int invokeId);
 CDAPMessage getReleaseConnectionRequestMessage(int portId,
 CDAPMessage::Flags flags, bool invokeID);
 CDAPMessage getReleaseConnectionResponseMessage(int portId,
 CDAPMessage::Flags flags, int result, std::string resultReason,
 int invokeId);
 CDAPMessage getCreateObjectRequestMessage(int portId, char filter[],
 CDAPMessage::Flags flags, std::string objClass, long objInst,
 std::string objName, ObjectValueInterface &objValue, int scope,
 bool invokeId);
 CDAPMessage getCreateObjectResponseMessage(int portId,
 CDAPMessage::Flags flags, std::string objClass, long objInst,
 std::string objName, ObjectValueInterface &objValue, int result,
 std::string resultReason, int invokeId);
 CDAPMessage getDeleteObjectRequestMessage(int portId, char filter[],
 CDAPMessage::Flags flags, std::string objClass, long objInst,
 std::string objName, ObjectValueInterface &objectValue, int scope,
 bool invokeId);
 CDAPMessage getDeleteObjectResponseMessage(int portId,
 CDAPMessage::Flags flags, std::string objClass, long objInst,
 std::string objName, int result, std::string resultReason,
 int invokeId);
 CDAPMessage getStartObjectRequestMessage(int portId, char filter[],
 CDAPMessage::Flags flags, std::string objClass,
 ObjectValueInterface &objValue, long objInst, std::string objName,
 int scope, bool invokeId);
 CDAPMessage getStartObjectResponseMessage(int portId,
 CDAPMessage::Flags flags, int result, std::string resultReason,
 int invokeId);
 CDAPMessage getStartObjectResponseMessage(int portId,
 CDAPMessage::Flags flags, std::string objClass,
 ObjectValueInterface &objValue, long objInst, std::string objName,
 int result, std::string resultReason, int invokeId)
 ;
 CDAPMessage getStopObjectRequestMessage(int portId, char filter[],
 CDAPMessage::Flags flags, std::string objClass,
 ObjectValueInterface &objValue, long objInst, std::string objName,
 int scope, bool invokeId);
 CDAPMessage getStopObjectResponseMessage(int portId,
 CDAPMessage::Flags flags, int result, std::string resultReason,
 int invokeId);
 CDAPMessage getReadObjectRequestMessage(int portId, char filter[],
 CDAPMessage::Flags flags, std::string objClass, long objInst,
 std::string objName, int scope, bool invokeId);
 CDAPMessage getReadObjectResponseMessage(int portId,
 CDAPMessage::Flags flags, std::string objClass, long objInst,
 std::string objName, ObjectValueInterface &objValue, int result,
 std::string resultReason, int invokeId);
 CDAPMessage getWriteObjectRequestMessage(int portId, char filter[],
 CDAPMessage::Flags flags, std::string objClass, long objInst,
 ObjectValueInterface &objValue, std::string objName, int scope,
 bool invokeId);
 CDAPMessage getWriteObjectResponseMessage(int portId,
 CDAPMessage::Flags flags, int result, std::string resultReason,
 int invokeId);
 CDAPMessage getCancelReadRequestMessage(int portId,
 CDAPMessage::Flags flags, int invokeID);
 CDAPMessage getCancelReadResponseMessage(int portId,
 CDAPMessage::Flags flags, int invokeID, int result,
 std::string resultReason);
 */
}
