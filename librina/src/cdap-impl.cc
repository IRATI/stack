/// cdap.cc
/// Created on: May 22, 2014
/// Author: bernat

#define RINA_PREFIX "cdap-manager"
#include "cdap-impl.h"

namespace rina {

// CLASS ResetStablishmentTimerTask
ResetStablishmentTimerTask::ResetStablishmentTimerTask(
		ConnectionStateMachine *con_state_machine) {
	con_state_machine_ = con_state_machine;
}
void ResetStablishmentTimerTask::run() {
	LOG_ERR(
			"M_CONNECT_R message not received within %d ms. Reseting the connection",
			con_state_machine_->timeout_);
	con_state_machine_->connection_state_ = con_state_machine_->NONE;
	con_state_machine_->cdap_session_->stopConnection();
}

// CLASS ReleaseConnectionTimerTask
ReleaseConnectionTimerTask::ReleaseConnectionTimerTask(
		ConnectionStateMachine *con_state_machine) {
	con_state_machine_ = con_state_machine;
}
void ReleaseConnectionTimerTask::run() {
	LOG_ERR(
			"M_RELEASE_R message not received within ms. Seting the connection to NULL",
			con_state_machine_->timeout_);
	con_state_machine_->connection_state_ = con_state_machine_->NONE;
	con_state_machine_->cdap_session_->stopConnection();
}

// CLASS ConnectionStateMachine
ConnectionStateMachine::ConnectionStateMachine(CDAPSessionImpl *cdap_session,
		long timeout) {
	cdap_session_ = cdap_session;
	timeout_ = timeout;
}
bool ConnectionStateMachine::is_connected() const {
	return connection_state_ == CONNECTED;
}
void ConnectionStateMachine::checkConnect() {
	if (connection_state_ != NONE) {
		std::stringstream ss;
		ss << "Cannot open a new connection because "
				<< "this CDAP session is currently in " << connection_state_
				<< " state";
		throw CDAPException(ss.str());
	}
}
void ConnectionStateMachine::connectSentOrReceived(bool sent) {
	if (sent) {
		connect();
	} else {
		connectReceived();
	}
}
void ConnectionStateMachine::checkConnectResponse() {
	if (connection_state_ != AWAITCON) {
		std::stringstream ss;
		ss
				<< "Cannot send a connection response because this CDAP session is currently in "
				<< connection_state_ << " state";
		throw CDAPException(ss.str());
	}
}
void ConnectionStateMachine::connectResponseSentOrReceived(bool sent) {
	if (sent) {
		connectResponse();
	} else {
		connectResponseReceived();
	}
}
void ConnectionStateMachine::checkRelease() {
	if (connection_state_ != CONNECTED) {
		std::stringstream ss;
		ss << "Cannot close a connection because " << "this CDAP session is "
				<< "currently in " << connection_state_ << " state";
		throw CDAPException(ss.str());
	}
}
void ConnectionStateMachine::releaseSentOrReceived(
		const CDAPMessage &cdap_message, bool sent) {
	if (sent) {
		release(cdap_message);
	} else {
		releaseReceived(cdap_message);
	}
}
void ConnectionStateMachine::checkReleaseResponse() {
	if (connection_state_ != AWAITCLOSE) {
		std::stringstream ss;
		ss
				<< "Cannot send a release connection response message because this CDAP session is currently in "
				<< connection_state_ << " state";
		throw CDAPException(ss.str());
	}
}
void ConnectionStateMachine::releaseResponseSentOrReceived(bool sent) {
	if (sent) {
		releaseResponse();
	} else {
		releaseResponseReceived();
	}
}
void ConnectionStateMachine::connect() {
	checkConnect();
	connection_state_ = AWAITCON;
	ResetStablishmentTimerTask *reset = new ResetStablishmentTimerTask(this);
	open_timer_.scheduleTask(reset, timeout_);
}
void ConnectionStateMachine::connectReceived() {
	if (connection_state_ != NONE) {
		std::stringstream ss;
		ss
				<< "Cannot open a new connection because this CDAP session is currently in"
				<< connection_state_ << " state";
		throw CDAPException(ss.str());
	}
	connection_state_ = AWAITCON;
}
void ConnectionStateMachine::connectResponse() {
	checkConnectResponse();
	connection_state_ = CONNECTED;
}
void ConnectionStateMachine::connectResponseReceived() {
	if (connection_state_ != AWAITCON) {
		std::stringstream ss;
		ss
				<< "Received an M_CONNECT_R message, but this CDAP session is currently in "
						+ connection_state_ << " state";
		throw CDAPException(ss.str());
	}
	open_timer_.clear();
	connection_state_ = CONNECTED;
}
void ConnectionStateMachine::release(const CDAPMessage &cdap_message) {
	checkRelease();
	connection_state_ = AWAITCLOSE;
	if (cdap_message.get_invoke_id() != 0) {
		ReleaseConnectionTimerTask *reset = new ReleaseConnectionTimerTask(
				this);
		close_timer_.scheduleTask(reset, timeout_);
	}
}
void ConnectionStateMachine::releaseReceived(const CDAPMessage &message) {
	if (connection_state_ != CONNECTED && connection_state_ != AWAITCLOSE) {
		std::stringstream ss;
		ss
				<< "Cannot close the connection because this CDAP session is currently in "
				<< connection_state_ << " state";
		throw CDAPException(ss.str());
	}
	if (message.get_invoke_id() != 0 && connection_state_ != AWAITCLOSE) {
		connection_state_ = AWAITCLOSE;
	} else {
		connection_state_ = NONE;
		cdap_session_->stopConnection();
	}
}
void ConnectionStateMachine::releaseResponse() {
	checkReleaseResponse();
	connection_state_ = NONE;
}
void ConnectionStateMachine::releaseResponseReceived() {
	if (connection_state_ != AWAITCLOSE) {
		std::stringstream ss;
		ss
				<< "Received an M_RELEASE_R message, but this CDAP session is currently in "
				<< connection_state_ << " state";
		throw CDAPException(ss.str());
	}
	close_timer_.clear();
	connection_state_ = NONE;
	cdap_session_->stopConnection();
}

// CLASS CDAPOperationState
CDAPOperationState::CDAPOperationState(CDAPMessage::Opcode op_code,
		bool sender) {
	op_code_ = op_code;
	sender_ = sender;
}

CDAPMessage::Opcode CDAPOperationState::get_op_code() const {
	return op_code_;
}
bool CDAPOperationState::is_sender() const {
	return is_sender();
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
	CDAPMessageValidator::validate(&cdap_message);

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
void CDAPSessionImpl::messageSent(const CDAPMessage &cdap_message) {
	messageSentOrReceived(cdap_message, true);
}
const CDAPMessage* CDAPSessionImpl::messageReceived(const char message[]) {
	const CDAPMessage *cdap_message = deserializeMessage(message);
	messageSentOrReceived(*cdap_message, false);
	return cdap_message;
}
void CDAPSessionImpl::messageReceived(const CDAPMessage &cdap_message) {
	messageSentOrReceived(cdap_message, false);
}
void CDAPSessionImpl::set_session_descriptor(
		CDAPSessionDescriptor *session_descriptor) {
	session_descriptor_ = session_descriptor;
}
int CDAPSessionImpl::get_port_id() const {
	return session_descriptor_->get_port_id();
}
CDAPSessionDescriptor* CDAPSessionImpl::get_session_descriptor() const {
	return session_descriptor_;
}
CDAPSessionInvokeIdManagerImpl* CDAPSessionImpl::get_invoke_id_manager() const {
	return invoke_id_manager_;
}
void CDAPSessionImpl::stopConnection() {
	cdap_session_manager_->removeCDAPSession(get_port_id());
}
void CDAPSessionImpl::messageSentOrReceived(const CDAPMessage &cdap_message,
		bool sent) {
	switch (cdap_message.get_op_code()) {
	case CDAPMessage::M_CONNECT:
		connection_state_machine_->connectSentOrReceived(sent);
		populateSessionDescriptor(cdap_message, sent);
		break;
	case CDAPMessage::M_CONNECT_R:
		connection_state_machine_->connectResponseSentOrReceived(sent);
		break;
	case CDAPMessage::M_RELEASE:
		connection_state_machine_->releaseSentOrReceived(cdap_message, sent);
		break;
	case CDAPMessage::M_RELEASE_R:
		connection_state_machine_->releaseResponseSentOrReceived(sent);
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
	CDAPMessageValidator::validate(&cdap_message);
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
		session_descriptor_->set_dest_ae_inst(&cdap_message.get_dest_ae_inst());
		session_descriptor_->set_dest_ae_name(&cdap_message.get_dest_ae_name());
		session_descriptor_->set_dest_ap_inst(&cdap_message.get_dest_ap_inst());
		session_descriptor_->set_dest_ap_name(&cdap_message.get_dest_ap_name());
		session_descriptor_->set_src_ae_inst(&cdap_message.get_src_ae_inst());
		session_descriptor_->set_src_ae_name(&cdap_message.get_src_ae_name());
		session_descriptor_->set_src_ap_inst(&cdap_message.get_src_ap_inst());
		session_descriptor_->set_src_ap_name(&cdap_message.get_src_ap_name());
	} else {
		session_descriptor_->set_dest_ae_inst(&cdap_message.get_src_ae_inst());
		session_descriptor_->set_dest_ae_name(&cdap_message.get_src_ae_name());
		session_descriptor_->set_dest_ap_inst(&cdap_message.get_src_ap_inst());
		session_descriptor_->set_dest_ap_name(&cdap_message.get_src_ap_name());
		session_descriptor_->set_src_ae_inst(&cdap_message.get_dest_ae_inst());
		session_descriptor_->set_src_ae_name(&cdap_message.get_dest_ae_name());
		session_descriptor_->set_src_ap_inst(&cdap_message.get_dest_ae_name());
		session_descriptor_->set_src_ap_name(&cdap_message.get_dest_ap_name());
	}
	session_descriptor_->set_version(cdap_message.get_version());
}
void CDAPSessionImpl::emptySessionDescriptor() {
	CDAPSessionDescriptor *new_session = new CDAPSessionDescriptor(
			session_descriptor_->get_port_id());
	new_session->set_ap_naming_info(&session_descriptor_->get_ap_naming_info());
	delete session_descriptor_;
	session_descriptor_ = new_session;
}

// CLASS CDAPSessionManager
CDAPSessionManager::CDAPSessionManager() {
	throw CDAPException(
			"Not allowed default constructor of CDAPSessionManager has been called.");
}
CDAPSessionManager::CDAPSessionManager(WireMessageProviderFactory *arg0) {
	wire_message_provider_factory_ = arg0;
	timeout_ = DEFAULT_TIMEOUT_IN_MS;
}
CDAPSessionManager::CDAPSessionManager(WireMessageProviderFactory *arg0,
		long arg1) {
	wire_message_provider_factory_ = arg0;
	timeout_ = arg1;
}
CDAPSessionImpl* CDAPSessionManager::createCDAPSession(int port_id) {
	if (cdap_sessions_.find(port_id) != cdap_sessions_.end()) {
		return cdap_sessions_.find(port_id)->second;
	} else {
		CDAPSessionImpl *cdap_session = new CDAPSessionImpl(this, timeout_,
				wire_message_provider_factory_->createWireMessageProvider());
		CDAPSessionDescriptor *descriptor = new CDAPSessionDescriptor(port_id);
		cdap_session->set_session_descriptor(descriptor);
		cdap_sessions_.insert(
				std::pair<int, CDAPSessionImpl*>(port_id, cdap_session));
		return cdap_session;
	}
}
CDAPSessionManager::~CDAPSessionManager() throw () {
	cdap_sessions_.clear();
}
void CDAPSessionManager::getAllCDAPSessionIds(std::vector<int> &vector) {
	vector.clear();
	for (std::map<int, CDAPSessionImpl*>::iterator it = cdap_sessions_.begin();
			it != cdap_sessions_.end(); ++it) {
		vector.push_back(it->first);
	}
}
void CDAPSessionManager::getAllCDAPSessions(
		std::vector<CDAPSessionInterface*> &vector) {
	vector.clear();
	for (std::map<int, CDAPSessionImpl*>::iterator it = cdap_sessions_.begin();
			it != cdap_sessions_.end(); ++it) {
		vector.push_back(it->second);
	}
}
CDAPSessionImpl* CDAPSessionManager::get_cdap_session(int port_id) {
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
	std::map<int, CDAPSessionImpl*>::iterator it = cdap_sessions_.find(port_id);
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
	CDAPSessionImpl *cdap_session = get_cdap_session(port_id);
	if (cdap_session != 0) {
		std::stringstream ss;
		ss << "Received CDAP message from "
				<< (cdap_session->get_session_descriptor()->get_destination_application_process_naming_info()).toString()
				<< " through underlying portId " << port_id
				<< ". Decoded contents: " << cdap_message->to_string();
		LOG_DBG( "%s", ss.str().c_str());
	} else {
		std::stringstream ss;
		ss << "Received CDAP message from " << cdap_message->get_dest_ap_name()
				<< " through underlying portId " << port_id
				<< ". Decoded contents: " << cdap_message->to_string();

		LOG_DBG( "%s", ss.str().c_str());
	}
	switch (cdap_message->get_op_code()) {
	case CDAPMessage::M_CONNECT:
		if (cdap_session != 0) {
			cdap_session = createCDAPSession(port_id);
			cdap_session->messageReceived(*cdap_message);
			LOG_DBG("Created a new CDAP session for port %d", port_id);
		} else {
			throw CDAPException(
					"M_CONNECT received on an already open CDAP Session, over flow "
							+ port_id);
		}
		break;
	default:
		if (cdap_session != 0) {
			cdap_session->messageReceived(*cdap_message);
		} else {
			std::stringstream ss;
			ss << "Receive a " << cdap_message->get_op_code()
					<< " CDAP message on a CDAP session that is not open, over flow "
					<< port_id;
			throw CDAPException(ss.str());
		}
		break;
	}
	return cdap_message;
}
void CDAPSessionManager::messageSent(const CDAPMessage &cdap_message,
		int port_id) {
	CDAPSessionImpl *cdap_session = get_cdap_session(port_id);
	if (cdap_session == 0
			&& cdap_message.get_op_code() == CDAPMessage::M_CONNECT) {
		cdap_session = createCDAPSession(port_id);
	} else if (cdap_session == 0
			&& cdap_message.get_op_code() != CDAPMessage::M_CONNECT) {
		std::stringstream ss;
		ss
				<< "There are no open CDAP sessions associated to the flow identified by "
				<< port_id << " right now";
		throw CDAPException(ss.str());
	}
	cdap_session->messageSent(cdap_message);
}
int CDAPSessionManager::get_port_id(
		std::string destination_application_process_name) {
	std::map<int, CDAPSessionImpl*>::iterator itr = cdap_sessions_.begin();
	CDAPSessionImpl *current_session;
	while (itr != cdap_sessions_.end()) {
		current_session = itr->second;
		if (current_session->get_session_descriptor()->get_dest_ap_name()
				== (destination_application_process_name)) {
			return current_session->get_port_id();
		}
	}
	std::stringstream ss;
	ss << "Don't have a running CDAP sesion to "
			<< destination_application_process_name;
	throw CDAPException(ss.str());
}
const CDAPMessage* CDAPSessionManager::getOpenConnectionRequestMessage(
		int port_id, CDAPMessage::AuthTypes auth_mech,
		const AuthValue &auth_value, const std::string &dest_ae_inst,
		const std::string &dest_ae_name, const std::string &dest_ap_inst,
		const std::string &dest_ap_name, const std::string &src_ae_inst,
		const std::string &src_ae_name, const std::string &src_ap_inst,
		const std::string &src_ap_name) {
	CDAPSessionImpl *cdap_session = get_cdap_session(port_id);
	if (cdap_session == 0) {
		cdap_session = createCDAPSession(port_id);
	}
	cdap_session->get_invoke_id_manager()->newInvokeId();
	return CDAPMessage::getOpenConnectionRequestMessage(auth_mech, auth_value,
			dest_ae_inst, dest_ae_name, dest_ap_inst, dest_ap_name, src_ae_inst,
			src_ae_name, src_ap_inst, src_ap_name, 1);
}
const CDAPMessage* CDAPSessionManager::getOpenConnectionResponseMessage(
		CDAPMessage::AuthTypes auth_mech, const AuthValue &auth_value,
		const std::string &dest_ae_inst, const std::string &dest_ae_name,
		const std::string &dest_ap_inst, const std::string &dest_ap_name,
		int result, const std::string &result_reason,
		const std::string &src_ae_inst, const std::string &src_ae_name,
		const std::string &src_ap_inst, const std::string &src_ap_name,
		int invoke_id) {
	return CDAPMessage::getOpenConnectionResponseMessage(auth_mech, auth_value,
			dest_ae_inst, dest_ae_name, dest_ap_inst, dest_ap_name, result,
			result_reason, src_ae_inst, src_ae_name, src_ap_inst, src_ap_name,
			invoke_id);
}
const CDAPMessage* CDAPSessionManager::getReleaseConnectionRequestMessage(
		int port_id, CDAPMessage::Flags flags, bool invoke_id) {
	CDAPMessage *cdap_message = CDAPMessage::getReleaseConnectionRequestMessage(
			flags);
	assignInvokeId(*cdap_message, invoke_id, port_id);
	return cdap_message;
}
const CDAPMessage* CDAPSessionManager::getReleaseConnectionResponseMessage(
		CDAPMessage::Flags flags, int result, const std::string &result_reason,
		int invoke_id) {
	return CDAPMessage::getReleaseConnectionResponseMessage(flags, result,
			result_reason, invoke_id);
}
const CDAPMessage* CDAPSessionManager::getCreateObjectRequestMessage(
		int port_id, char filter[], CDAPMessage::Flags flags,
		const std::string &obj_class, long obj_inst,
		const std::string &obj_name, ObjectValueInterface *obj_value, int scope,
		bool invoke_id) {
	CDAPMessage *cdap_message = CDAPMessage::getCreateObjectRequestMessage(
			filter, flags, obj_class, obj_inst, obj_name, obj_value, scope);
	assignInvokeId(*cdap_message, invoke_id, port_id);
	return cdap_message;
}
const CDAPMessage* CDAPSessionManager::getCreateObjectResponseMessage(
		CDAPMessage::Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name, ObjectValueInterface *obj_value,
		int result, const std::string &result_reason, int invoke_id) {
	return CDAPMessage::getCreateObjectResponseMessage(flags, obj_class,
			obj_inst, obj_name, obj_value, result, result_reason, invoke_id);
}
const CDAPMessage* CDAPSessionManager::getDeleteObjectRequestMessage(
		int port_id, char* filter, CDAPMessage::Flags flags,
		const std::string &obj_class, long obj_inst,
		const std::string &obj_name, ObjectValueInterface *object_value,
		int scope, bool invoke_id) {
	CDAPMessage *cdap_message = CDAPMessage::getDeleteObjectRequestMessage(
			filter, flags, obj_class, obj_inst, obj_name, object_value, scope);
	assignInvokeId(*cdap_message, invoke_id, port_id);
	return cdap_message;
}
const CDAPMessage* CDAPSessionManager::getDeleteObjectResponseMessage(
		CDAPMessage::Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name, int result,
		const std::string &result_reason, int invoke_id) {
	return CDAPMessage::getDeleteObjectResponseMessage(flags, obj_class,
			obj_inst, obj_name, result, result_reason, invoke_id);
}
const CDAPMessage* CDAPSessionManager::getStartObjectRequestMessage(int port_id,
		char filter[], CDAPMessage::Flags flags, const std::string &obj_class,
		ObjectValueInterface *obj_value, long obj_inst,
		const std::string &obj_name, int scope, bool invoke_id) {
	CDAPMessage *cdap_message = CDAPMessage::getStartObjectRequestMessage(
			filter, flags, obj_class, obj_value, obj_inst, obj_name, scope);
	assignInvokeId(*cdap_message, invoke_id, port_id);
	return cdap_message;
}
const CDAPMessage* CDAPSessionManager::getStartObjectResponseMessage(
		CDAPMessage::Flags flags, int result, const std::string &result_reason,
		int invoke_id) {
	return CDAPMessage::getStartObjectResponseMessage(flags, result,
			result_reason, invoke_id);
}
const CDAPMessage* CDAPSessionManager::getStartObjectResponseMessage(
		CDAPMessage::Flags flags, const std::string &obj_class,
		ObjectValueInterface *obj_value, long obj_inst,
		const std::string &obj_name, int result,
		const std::string &result_reason, int invoke_id) {
	return CDAPMessage::getStartObjectResponseMessage(flags, obj_class,
			obj_value, obj_inst, obj_name, result, result_reason, invoke_id);
}
const CDAPMessage* CDAPSessionManager::getStopObjectRequestMessage(int port_id,
		char* filter, CDAPMessage::Flags flags, const std::string &obj_class,
		ObjectValueInterface *obj_value, long obj_inst,
		const std::string &obj_name, int scope, bool invoke_id) {
	CDAPMessage *cdap_message = CDAPMessage::getStopObjectRequestMessage(filter,
			flags, obj_class, obj_value, obj_inst, obj_name, scope);
	assignInvokeId(*cdap_message, invoke_id, port_id);
	return cdap_message;
}
const CDAPMessage* CDAPSessionManager::getStopObjectResponseMessage(
		CDAPMessage::Flags flags, int result, const std::string &result_reason,
		int invoke_id) {
	return CDAPMessage::getStopObjectResponseMessage(flags, result,
			result_reason, invoke_id);
}
const CDAPMessage* getStopObjectResponseMessage(CDAPMessage::Flags flags,
		int result, const std::string &result_reason, int invoke_id) {
	return CDAPMessage::getStopObjectResponseMessage(flags, result,
			result_reason, invoke_id);
}
const CDAPMessage* CDAPSessionManager::getReadObjectRequestMessage(int port_id,
		char filter[], CDAPMessage::Flags flags, const std::string &obj_class,
		long obj_inst, const std::string &obj_name, int scope, bool invoke_id) {
	CDAPMessage *cdap_message = CDAPMessage::getReadObjectRequestMessage(filter,
			flags, obj_class, obj_inst, obj_name, scope);
	assignInvokeId(*cdap_message, invoke_id, port_id);
	return cdap_message;
}
const CDAPMessage* CDAPSessionManager::getReadObjectResponseMessage(
		CDAPMessage::Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name, ObjectValueInterface *obj_value,
		int result, const std::string &result_reason, int invoke_id) {
	return CDAPMessage::getReadObjectResponseMessage(flags, obj_class, obj_inst,
			obj_name, obj_value, result, result_reason, invoke_id);
}
const CDAPMessage* CDAPSessionManager::getWriteObjectRequestMessage(int port_id,
		char filter[], CDAPMessage::Flags flags, const std::string &obj_class,
		long obj_inst, ObjectValueInterface *obj_value,
		const std::string &obj_name, int scope, bool invoke_id) {
	CDAPMessage *cdap_message = CDAPMessage::getWriteObjectRequestMessage(
			filter, flags, obj_class, obj_inst, obj_value, obj_name, scope);
	assignInvokeId(*cdap_message, invoke_id, port_id);
	return cdap_message;
}
const CDAPMessage* CDAPSessionManager::getWriteObjectResponseMessage(
		CDAPMessage::Flags flags, int result, const std::string &result_reason,
		int invoke_id) {
	return CDAPMessage::getWriteObjectResponseMessage(flags, result, invoke_id,
			result_reason);
}
const CDAPMessage* CDAPSessionManager::getCancelReadRequestMessage(
		CDAPMessage::Flags flags, int invoke_id) {
	return CDAPMessage::getCancelReadRequestMessage(flags, invoke_id);
}
const CDAPMessage* CDAPSessionManager::getCancelReadResponseMessage(
		CDAPMessage::Flags flags, int invoke_id, int result,
		const std::string &result_reason) {
	return CDAPMessage::getCancelReadResponseMessage(flags, invoke_id, result,
			result_reason);
}
void CDAPSessionManager::assignInvokeId(CDAPMessage &cdap_message,
		bool invoke_id, int port_id) {
	if (invoke_id) {
		CDAPSessionImpl *cdap_session = get_cdap_session(port_id);
		cdap_message.set_invoke_id(
				cdap_session->get_invoke_id_manager()->newInvokeId());
	}
}

// CLASS GPBWireMessageProvider
const CDAPMessage* GPBWireMessageProvider::deserializeMessage(
		const char message[]) {
	cdap::impl::googleprotobuf::CDAPMessage gpfCDAPMessage;
	CDAPMessage *cdapMessage = new CDAPMessage;

	gpfCDAPMessage.ParseFromArray(message, sizeof(message) / sizeof(*message));

	cdapMessage->set_abs_syntax(gpfCDAPMessage.abssyntax());
	CDAPMessage::AuthTypes auth_type =
			static_cast<CDAPMessage::AuthTypes>(cdap::impl::googleprotobuf::authTypes_t_Name(
					gpfCDAPMessage.authmech()));
	cdapMessage->set_auth_mech(auth_type);
	AuthValue auth_value(gpfCDAPMessage.authvalue().authname(),
			gpfCDAPMessage.authvalue().authpassword(),
			gpfCDAPMessage.authvalue().authother());
	cdapMessage->set_auth_value(auth_value);
	cdapMessage->set_dest_ae_inst(gpfCDAPMessage.destaeinst());
	cdapMessage->set_dest_ae_name(gpfCDAPMessage.destaename());
	cdapMessage->set_dest_ap_inst(gpfCDAPMessage.destapinst());
	cdapMessage->set_dest_ap_name(gpfCDAPMessage.destapname());
	char *filter = new char[gpfCDAPMessage.filter() + 1];
	strcpy(filter, gpfCDAPMessage.filter().c_str());
	cdapMessage->set_filter(filter);
	CDAPMessage::Flags flags =
			static_cast<CDAPMessage::Flags>(cdap::impl::googleprotobuf::flagValues_t_Name(
					gpfCDAPMessage.flags()));
	cdapMessage->set_flags(flags);
	cdapMessage->set_invoke_id(gpfCDAPMessage.invokeid());
	cdapMessage->set_obj_class(gpfCDAPMessage.objclass());
	cdapMessage->set_obj_inst(gpfCDAPMessage.objinst());
	cdapMessage->set_obj_name(gpfCDAPMessage.objname());
	ObjectValueInterface *obj_val = gpfCDAPMessage.objvalue();
	cdapMessage->set_obj_value(obj_val);
	CDAPMessage::Opcode opcode =
			static_cast<CDAPMessage::Opcode>(cdap::impl::googleprotobuf::opCode_t_Name(
					gpfCDAPMessage.opcode()));
	cdapMessage->set_op_code(opcode);
	cdapMessage->set_result(gpfCDAPMessage.result());
	cdapMessage->set_result_reason(gpfCDAPMessage.resultreason());
	cdapMessage->set_scope(gpfCDAPMessage.scope());
	cdapMessage->set_src_ae_inst(gpfCDAPMessage.srcaeinst());
	cdapMessage->set_src_ae_name(gpfCDAPMessage.srcaename());
	cdapMessage->set_src_ap_inst(gpfCDAPMessage.srcapinst());
	cdapMessage->set_src_ap_name(gpfCDAPMessage.srcapname());
	cdapMessage->set_version(gpfCDAPMessage.version());

	return cdapMessage;
}
const char* GPBWireMessageProvider::serializeMessage(
		const CDAPMessage &cdapMessage) {
	cdap::impl::googleprotobuf::CDAPMessage gpfCDAPMessage;

	gpfCDAPMessage.set_abssyntax(cdapMessage.get_abs_syntax());
	cdap::impl::googleprotobuf::authTypes_t auth_types;
	if (!cdap::impl::googleprotobuf::authTypes_t_IsValid(cdapMessage.get_auth_mech()))	{
		throw CDAPException("Serializing Message: Not a valid AuthType");
	}
	gpfCDAPMessage.set_authmech((cdap::impl::googleprotobuf::authTypes_t)cdapMessage.get_auth_mech());

	cdap::impl::googleprotobuf::authValue_t gpb_auth_value;
	AuthValue auth_value = cdapMessage.get_auth_value();
	gpb_auth_value.set_authname(auth_value.get_auth_name());
	gpb_auth_value.set_authother(auth_value.get_auth_other());
	gpb_auth_value.set_authpassword(auth_value.get_auth_password());
	gpfCDAPMessage.set_allocated_authvalue(&gpb_auth_value);

	gpfCDAPMessage.set_destaeinst(cdapMessage.get_dest_ae_inst());
	gpfCDAPMessage.set_destaename(cdapMessage.get_dest_ae_name());
	gpfCDAPMessage.set_destapinst(cdapMessage.get_dest_ap_inst());
	gpfCDAPMessage.set_destapname(cdapMessage.get_dest_ap_name());
	gpfCDAPMessage.set_filter(cdapMessage.get_filter());
	gpfCDAPMessage.set_invokeid(cdapMessage.get_invoke_id());
	gpfCDAPMessage.set_objclass(cdapMessage.get_obj_class());
	gpfCDAPMessage.set_objinst(cdapMessage.get_obj_inst());
	gpfCDAPMessage.set_objname(cdapMessage.get_obj_name());

	gpfCDAPMessage.set_objvalue(cdapMessage.get_obj_value());
	if (!cdap::impl::googleprotobuf::opCode_t_IsValid(cdapMessage.get_op_code())) {
		throw CDAPException("Serializing Message: Not a valid OpCode");
	}
	gpfCDAPMessage.set_opcode((cdap::impl::googleprotobuf::opCode_t)cdapMessage.get_op_code());
	gpfCDAPMessage.set_result(cdapMessage.get_result());
	gpfCDAPMessage.set_resultreason(cdapMessage.get_result_reason());
	gpfCDAPMessage.set_scope(cdapMessage.get_scope());
	gpfCDAPMessage.set_srcaeinst(cdapMessage.get_src_ae_inst());
	gpfCDAPMessage.set_srcaename(cdapMessage.get_src_ae_name());
	gpfCDAPMessage.set_srcapname(cdapMessage.get_src_ap_name());
	gpfCDAPMessage.set_srcapinst(cdapMessage.get_src_ap_inst());
	gpfCDAPMessage.set_version(cdapMessage.get_version());
}
}
