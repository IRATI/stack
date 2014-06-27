/// cdap.cc
/// Created on: May 22, 2014
/// Author: bernat

#define RINA_PREFIX "cdap-manager"
#include "cdap-impl.h"
#include "CDAP.pb.h"

namespace rina {

// CLASS ResetStablishmentTimerTask
ResetStablishmentTimerTask::ResetStablishmentTimerTask(
		ConnectionStateMachine *con_state_machine) {
	LOG_DBG("ResetStablishmentTimerTask: Created");
	con_state_machine_ = con_state_machine;
}
ResetStablishmentTimerTask::~ResetStablishmentTimerTask(){
	LOG_DBG("ResetStablishmentTimerTask: Destroyed");
}
void ResetStablishmentTimerTask::run() {
	LOG_ERR(
			"M_CONNECT_R message not received within %d ms. Reseting the connection",
			con_state_machine_->timeout_);
	con_state_machine_->resetConnection();
}

// CLASS ReleaseConnectionTimerTask
ReleaseConnectionTimerTask::ReleaseConnectionTimerTask(
		ConnectionStateMachine *con_state_machine) {
	LOG_DBG("ReleaseConnectionTimerTask: Created");
	con_state_machine_ = con_state_machine;
}
ReleaseConnectionTimerTask::~ReleaseConnectionTimerTask(){
	LOG_DBG("ReleaseConnectionTimerTask: Destroyed");
}
void ReleaseConnectionTimerTask::run() {
	LOG_ERR(
			"M_RELEASE_R message not received within ms. Seting the connection to NULL",
			con_state_machine_->timeout_);
	con_state_machine_->resetConnection();
}

// CLASS ConnectionStateMachine
ConnectionStateMachine::ConnectionStateMachine(CDAPSessionImpl *cdap_session,
		long timeout) {
	LOG_DBG("ConnectionStateMachine: Created");
	cdap_session_ = cdap_session;
	timeout_ = timeout;
	try {
		trylock();
		connection_state_= NONE;
		unlock();
	} catch(ConcurrentException &e){
		LOG_DBG("ConnectionStateMachine::ConnectionStateMachine(CDAPSessionImpl *cdap_session,	long timeout) exception");
	}
	open_timer_ = 0;
	close_timer_ = 0;
}
ConnectionStateMachine::~ConnectionStateMachine() throw(){
	LOG_DBG("ConnectionStateMachine: Destroyed");
	try {
		trylock();
		delete open_timer_;
		open_timer_ = 0;
		delete close_timer_;
		close_timer_ = 0;
		unlock();
	} catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::~ConnectionStateMachine() exception");
	}
}
bool ConnectionStateMachine::is_connected() const {
	return connection_state_ == CONNECTED;
}
void ConnectionStateMachine::checkConnect() {
	try {
		trylock();
		if (connection_state_ != NONE) {
			std::stringstream ss;
			ss << "Cannot open a new connection because "
					<< "this CDAP session is currently in " << connection_state_
					<< " state";
			unlock();
			throw CDAPException(ss.str());
		}
		unlock();
	} catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::checkConnect() exception");
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
	try {
		trylock();
		if (connection_state_ != AWAITCON) {
			std::stringstream ss;
			ss
					<< "Cannot send a connection response because this CDAP session is currently in "
					<< connection_state_ << " state";
			unlock();
			throw CDAPException(ss.str());
		}
		unlock();
	} catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::checkConnectResponse() exception");
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
	try {
		trylock();
		if (connection_state_ != CONNECTED) {
			std::stringstream ss;
			ss << "Cannot close a connection because " << "this CDAP session is "
					<< "currently in " << connection_state_ << " state";
			unlock();
			throw CDAPException(ss.str());
		}
		unlock();
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::checkRelease() exception");
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
	try {
		trylock();
		if (connection_state_ != AWAITCLOSE) {
			std::stringstream ss;
			ss
					<< "Cannot send a release connect"
							"try {ion response message because this CDAP session is currently in "
					<< connection_state_ << " state";
			unlock();
			throw CDAPException(ss.str());
		}
		unlock();
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::checkReleaseResponse() exception");
	}
}
void ConnectionStateMachine::releaseResponseSentOrReceived(bool sent) {
	if (sent) {
		releaseResponse();
	} else {
		releaseResponseReceived();
	}
}
void ConnectionStateMachine::resetConnection() {
	try {
		trylock();
		connection_state_ = NONE;
		cdap_session_->stopConnection();
		unlock();
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::resetConnection() exception");
	}
}
void ConnectionStateMachine::connect() {
	checkConnect();
	try {
		trylock();
		connection_state_ = AWAITCON;
		unlock();
		LOG_DBG("Waiting timeout %d to receive a connection response", timeout_);
		open_timer_ =  new Timer();
		ResetStablishmentTimerTask *reset = new ResetStablishmentTimerTask(this);
		open_timer_->scheduleTask(reset, timeout_);
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::connect() exception");
	}
}
void ConnectionStateMachine::connectReceived() {
	try {
		trylock();
		if (connection_state_ != NONE) {
			std::stringstream ss;
			ss
					<< "Cannot open a new connection because this CDAP session is currently in"
					<< connection_state_ << " state";
			unlock();
			throw CDAPException(ss.str());
		}
		connection_state_ = AWAITCON;
		unlock();
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::connectReceived() exception");
	}
}
void ConnectionStateMachine::connectResponse() {
	checkConnectResponse();
	try {
		trylock();
		connection_state_ = CONNECTED;
		unlock();
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::connectResponse() exception");
	}
}
void ConnectionStateMachine::connectResponseReceived() {
	try {
		trylock();
		if (connection_state_ != AWAITCON) {
			std::stringstream ss;
			ss << "Received an M_CONNECT_R message, but this CDAP session is currently in "
							+ connection_state_ << " state";
			unlock();
			throw CDAPException(ss.str());
		}
		unlock();
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::connectResponseReceived() 1 exception");
	}
	LOG_DBG("Connection response received");
	delete open_timer_;
	open_timer_ = 0;
	try {
		trylock();
		connection_state_ = CONNECTED;
		unlock();
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::connectResponseReceived() 2 exception");
	}
}
void ConnectionStateMachine::release(const CDAPMessage &cdap_message) {
	checkRelease();
	try {
		trylock();
		connection_state_ = AWAITCLOSE;
		unlock();
		if (cdap_message.get_invoke_id() != 0) {
			close_timer_=  new Timer();
			ReleaseConnectionTimerTask *reset = new ReleaseConnectionTimerTask(
					this);
			LOG_DBG("Waiting timeout %d to receive a release response", timeout_);
			close_timer_->scheduleTask(reset, timeout_);

		}
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::release exception");
	}
}
void ConnectionStateMachine::releaseReceived(const CDAPMessage &message) {
	try {
		trylock();
		if (connection_state_ != CONNECTED && connection_state_ != AWAITCLOSE) {
			std::stringstream ss;
			ss
					<< "Cannot close the connection because this CDAP session is currently in "
					<< connection_state_ << " state";
			unlock();
			throw CDAPException(ss.str());
		}
		if (message.get_invoke_id() != 0 && connection_state_ != AWAITCLOSE) {
			connection_state_ = AWAITCLOSE;
		} else {
			connection_state_ = NONE;
			cdap_session_->stopConnection();
		}
		unlock();
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::releaseReceived");
	}
}
void ConnectionStateMachine::releaseResponse() {
	checkReleaseResponse();
	try {
		trylock();
		connection_state_ = NONE;
		unlock();
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::releaseResponse");
	}
}
void ConnectionStateMachine::releaseResponseReceived() {
	if (connection_state_ != AWAITCLOSE) {
		std::stringstream ss;
		ss
				<< "Received an M_RELEASE_R message, but this CDAP session is currently in "
				<< connection_state_ << " state";
		throw CDAPException(ss.str());
	}
	LOG_DBG("Release response recevied");
	delete close_timer_;
	close_timer_ = 0;
	try {
		trylock();
		cdap_session_->stopConnection();
		unlock();
	}catch(ConcurrentException &e) {
		LOG_DBG("ConnectionStateMachine::releaseResponseReceived");
	}
}

// CLASS CDAPOperationState
CDAPOperationState::CDAPOperationState(CDAPMessage::Opcode op_code,
		bool sender) {
	LOG_DBG("CDAPOperationState: Created");
	op_code_ = op_code;
	sender_ = sender;
}
CDAPOperationState::~CDAPOperationState(){
	LOG_DBG("CDAPOperationState: Destroyed");
}
CDAPMessage::Opcode CDAPOperationState::get_op_code() const {
	return op_code_;
}
bool CDAPOperationState::is_sender() const {
	return is_sender();
}

// CLASS CDAPSessionInvokeIdManagerImpl
CDAPInvokeIdManagerImpl::CDAPInvokeIdManagerImpl() {
	LOG_DBG("CDAPInvokeIdManagerImpl: Created");
}
CDAPInvokeIdManagerImpl::~CDAPInvokeIdManagerImpl() throw(){
	LOG_DBG("CDAPInvokeIdManagerImpl: Destroyed");
	used_invoke_ids_.clear();
}
void CDAPInvokeIdManagerImpl::freeInvokeId(int invoke_id) {
	trylock();
	used_invoke_ids_.remove(invoke_id);
	unlock();
}
int CDAPInvokeIdManagerImpl::newInvokeId() {
	trylock();
	int candidate = 1;
	while (std::find(used_invoke_ids_.begin(), used_invoke_ids_.end(),
			candidate) != used_invoke_ids_.end()) {
		candidate = candidate + 1;
	}
	used_invoke_ids_.push_back(candidate);
	unlock();
	return candidate;
}
void CDAPInvokeIdManagerImpl::reserveInvokeId(int invoke_id) {
	trylock();
	used_invoke_ids_.push_back(invoke_id);
	unlock();
}

// CLASS CDAPSessionImpl
CDAPSessionImpl::CDAPSessionImpl(CDAPSessionManager *cdap_session_manager,
		long timeout, WireMessageProviderInterface *wire_message_provider, CDAPInvokeIdManagerImpl *invoke_id_manager) {
	LOG_DBG("CDAPSessionImpl: Created");
	cdap_session_manager_ = cdap_session_manager;
	connection_state_machine_ = new ConnectionStateMachine(this, timeout);
	wire_message_provider_ = wire_message_provider;
	invoke_id_manager_ = invoke_id_manager;
	session_descriptor_ = 0;
}

CDAPSessionImpl::~CDAPSessionImpl() throw () {
	LOG_DBG("CDAPSessionImpl: Destroyed");
	delete connection_state_machine_;
	connection_state_machine_ = 0;
	delete wire_message_provider_;
	wire_message_provider_ = 0;
	delete session_descriptor_;
	session_descriptor_ = 0;
	for (std::map<int, CDAPOperationState*>::iterator iter = pending_messages_.begin(); iter != pending_messages_.end(); ++iter)	{
		delete iter->second;
		iter->second = 0;
	}
	pending_messages_.clear();
	for (std::map<int, CDAPOperationState*>::iterator iter = cancel_read_pending_messages_.begin(); iter != cancel_read_pending_messages_.end(); ++iter)	{
		delete iter->second;
		iter->second = 0;
	}
	cancel_read_pending_messages_.clear();
}
const SerializedMessage* CDAPSessionImpl::encodeNextMessageToBeSent(
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
const CDAPMessage* CDAPSessionImpl::messageReceived(const SerializedMessage &message) {
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
CDAPInvokeIdManagerImpl* CDAPSessionImpl::get_invoke_id_manager() const {
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
const SerializedMessage* CDAPSessionImpl::serializeMessage(
		const CDAPMessage &cdap_message) const {
	return wire_message_provider_->serializeMessage(cdap_message);
}
const CDAPMessage* CDAPSessionImpl::deserializeMessage(
		const SerializedMessage &message) const {
	return wire_message_provider_->deserializeMessage(message);
}
void CDAPSessionImpl::populateSessionDescriptor(const CDAPMessage &cdap_message,
		bool send) {
	session_descriptor_->set_abs_syntax(cdap_message.get_abs_syntax());
	session_descriptor_->set_auth_mech(cdap_message.get_auth_mech());
	session_descriptor_->set_auth_value(cdap_message.get_auth_value());

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
	session_descriptor_ = 0;
	session_descriptor_ = new_session;
}

// CLASS CDAPSessionManager
CDAPSessionManager::CDAPSessionManager() {
	throw CDAPException(
			"Not allowed default constructor of CDAPSessionManager has been called.");
}
CDAPSessionManager::CDAPSessionManager(WireMessageProviderFactory *arg0) {
	LOG_DBG("CDAPSessionManager: Created");
	wire_message_provider_factory_ = arg0;
	timeout_ = DEFAULT_TIMEOUT_IN_MS;
	invoke_id_manager_ = new CDAPInvokeIdManagerImpl();
}
CDAPSessionManager::CDAPSessionManager(WireMessageProviderFactory *arg0,
		long arg1) {
	LOG_DBG("CDAPSessionManager: Created");
	wire_message_provider_factory_ = arg0;
	timeout_ = arg1;
	invoke_id_manager_ = new CDAPInvokeIdManagerImpl();
}
CDAPSessionImpl* CDAPSessionManager::createCDAPSession(int port_id) {
	if (cdap_sessions_.find(port_id) != cdap_sessions_.end()) {
		return cdap_sessions_.find(port_id)->second;
	} else {
		CDAPSessionImpl *cdap_session = new CDAPSessionImpl(this, timeout_,
				wire_message_provider_factory_->createWireMessageProvider(), invoke_id_manager_);
		CDAPSessionDescriptor *descriptor = new CDAPSessionDescriptor(port_id);
		cdap_session->set_session_descriptor(descriptor);
		cdap_sessions_.insert(
				std::pair<int, CDAPSessionImpl*>(port_id, cdap_session));
		return cdap_session;
	}
}
CDAPSessionManager::~CDAPSessionManager() throw () {
	LOG_DBG("CDAPSessionManager: Destroyed");
	delete invoke_id_manager_;
	invoke_id_manager_ = 0;
	for (std::map<int, CDAPSessionImpl*>::iterator iter = cdap_sessions_.begin(); iter != cdap_sessions_.end(); ++iter)	{
		delete iter->second;
		iter->second = 0;
	}
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
const SerializedMessage* CDAPSessionManager::encodeCDAPMessage(
		const CDAPMessage &cdap_message) {
	return wire_message_provider_->serializeMessage(cdap_message);
}
const CDAPMessage* CDAPSessionManager::decodeCDAPMessage(const SerializedMessage &cdap_message) {
	return wire_message_provider_->deserializeMessage(cdap_message);
}
void CDAPSessionManager::removeCDAPSession(int port_id) {
	std::map<int, CDAPSessionImpl*>::iterator iter = cdap_sessions_.find(port_id);
	delete iter->second;
	iter->second = 0;
	cdap_sessions_.erase(iter);
}
const SerializedMessage* CDAPSessionManager::encodeNextMessageToBeSent(
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
		const SerializedMessage &encoded_cdap_message, int port_id) {
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
GPBWireMessageProvider::GPBWireMessageProvider() {
	LOG_DBG("GPBWireMessageProvider: Created");
}
GPBWireMessageProvider::~GPBWireMessageProvider() throw(){
	LOG_DBG("GPBWireMessageProvider: Destroyed");
}
const CDAPMessage* GPBWireMessageProvider::deserializeMessage(
		const SerializedMessage &message) {
	cdap::impl::googleprotobuf::CDAPMessage gpfCDAPMessage;
	CDAPMessage *cdapMessage = new CDAPMessage;

	gpfCDAPMessage.ParseFromArray(message.message_, message.size_);
	cdapMessage->set_abs_syntax(gpfCDAPMessage.abssyntax());
	int auth_type_val = gpfCDAPMessage.authmech();
	CDAPMessage::AuthTypes auth_type = static_cast<CDAPMessage::AuthTypes>(auth_type_val);
	cdapMessage->set_auth_mech(auth_type);
	AuthValue auth_value(gpfCDAPMessage.authvalue().authname(),
			gpfCDAPMessage.authvalue().authpassword(),
			gpfCDAPMessage.authvalue().authother());
	cdapMessage->set_auth_value(auth_value);
	if (gpfCDAPMessage.has_destaeinst())
		cdapMessage->set_dest_ae_inst(gpfCDAPMessage.destaeinst());
	if (gpfCDAPMessage.has_destaename())
		cdapMessage->set_dest_ae_name(gpfCDAPMessage.destaename());
	if (gpfCDAPMessage.has_destapinst())
		cdapMessage->set_dest_ap_inst(gpfCDAPMessage.destapinst());
	if (gpfCDAPMessage.has_destapname())
		cdapMessage->set_dest_ap_name(gpfCDAPMessage.destapname());
	if (gpfCDAPMessage.has_filter())
	{
		char *filter = new char[gpfCDAPMessage.filter().size() + 1];
		strcpy(filter, gpfCDAPMessage.filter().c_str());;
		cdapMessage->set_filter(filter);
	}
	int flag_value = gpfCDAPMessage.flags();
	CDAPMessage::Flags flags = static_cast<CDAPMessage::Flags>(flag_value);
	cdapMessage->set_flags(flags);
	cdapMessage->set_invoke_id(gpfCDAPMessage.invokeid());
	if (gpfCDAPMessage.has_objclass())
		cdapMessage->set_obj_class(gpfCDAPMessage.objclass());
	cdapMessage->set_obj_inst(gpfCDAPMessage.objinst());
	if (gpfCDAPMessage.has_objname())
		cdapMessage->set_obj_name(gpfCDAPMessage.objname());

	cdap::impl::googleprotobuf::objVal_t obj_val_t = gpfCDAPMessage.objvalue();
	if (obj_val_t.has_intval())
		cdapMessage->set_obj_value(new IntObjectValue(obj_val_t.intval()));
	if (obj_val_t.has_sintval())
		cdapMessage->set_obj_value(new SIntObjectValue(obj_val_t.sintval()));
	if (obj_val_t.has_int64val())
		cdapMessage->set_obj_value(new LongObjectValue(obj_val_t.int64val()));
	if (obj_val_t.has_sint64val())
		cdapMessage->set_obj_value(new SLongObjectValue(obj_val_t.sint64val()));
	if (obj_val_t.has_strval())
		cdapMessage->set_obj_value(new StringObjectValue(obj_val_t.strval()));

	if (obj_val_t.has_byteval())	{
		char *byte_val = new char[obj_val_t.byteval().size() +1];
		strcpy(byte_val, obj_val_t.byteval().c_str());
		cdapMessage->set_obj_value(new ByteArrayObjectValue(byte_val));
	}
	if (obj_val_t.has_floatval())
		cdapMessage->set_obj_value(new FloatObjectValue(obj_val_t.floatval()));
	if (obj_val_t.has_doubleval())
		cdapMessage->set_obj_value(new DoubleObjectValue(obj_val_t.doubleval()));
	if (obj_val_t.has_boolval())
		cdapMessage->set_obj_value(new BooleanObjectValue(obj_val_t.boolval()));
	int opcode_val = gpfCDAPMessage.opcode();
	CDAPMessage::Opcode opcode =
			static_cast<CDAPMessage::Opcode>(opcode_val);
	cdapMessage->set_op_code(opcode);
	cdapMessage->set_result(gpfCDAPMessage.result());
	if (gpfCDAPMessage.has_resultreason())
		cdapMessage->set_result_reason(gpfCDAPMessage.resultreason());
	cdapMessage->set_scope(gpfCDAPMessage.scope());
	if (gpfCDAPMessage.has_srcaeinst())
		cdapMessage->set_src_ae_inst(gpfCDAPMessage.srcaeinst());
	if (gpfCDAPMessage.has_srcaename())
		cdapMessage->set_src_ae_name(gpfCDAPMessage.srcaename());
	if (gpfCDAPMessage.has_srcapinst())
		cdapMessage->set_src_ap_inst(gpfCDAPMessage.srcapinst());
	if (gpfCDAPMessage.has_srcapname())
		cdapMessage->set_src_ap_name(gpfCDAPMessage.srcapname());
	cdapMessage->set_version(gpfCDAPMessage.version());

	return cdapMessage;
}
const SerializedMessage* GPBWireMessageProvider::serializeMessage(
		const CDAPMessage &cdapMessage) {
	cdap::impl::googleprotobuf::CDAPMessage gpfCDAPMessage;
	gpfCDAPMessage.set_abssyntax(cdapMessage.get_abs_syntax());
	if (!cdap::impl::googleprotobuf::authTypes_t_IsValid(cdapMessage.get_auth_mech()))	{
		throw CDAPException("Serializing Message: Not a valid AuthType");
	}
	gpfCDAPMessage.set_authmech((cdap::impl::googleprotobuf::authTypes_t)cdapMessage.get_auth_mech());
	cdap::impl::googleprotobuf::authValue_t *gpb_auth_value = new cdap::impl::googleprotobuf::authValue_t();
	AuthValue auth_value = cdapMessage.get_auth_value();
	gpb_auth_value->set_authname(auth_value.get_auth_name());
	gpb_auth_value->set_authother(auth_value.get_auth_other());
	gpb_auth_value->set_authpassword(auth_value.get_auth_password());
	gpfCDAPMessage.set_allocated_authvalue(gpb_auth_value);
	gpfCDAPMessage.set_destaeinst(cdapMessage.get_dest_ae_inst());
	gpfCDAPMessage.set_destaename(cdapMessage.get_dest_ae_name());
	gpfCDAPMessage.set_destapinst(cdapMessage.get_dest_ap_inst());
	gpfCDAPMessage.set_destapname(cdapMessage.get_dest_ap_name());
	if (cdapMessage.get_filter() != 0)
	{
		gpfCDAPMessage.set_filter(cdapMessage.get_filter());
	}
	gpfCDAPMessage.set_invokeid(cdapMessage.get_invoke_id());
	gpfCDAPMessage.set_objclass(cdapMessage.get_obj_class());
	gpfCDAPMessage.set_objinst(cdapMessage.get_obj_inst());
	gpfCDAPMessage.set_objname(cdapMessage.get_obj_name());

	if (cdapMessage.get_obj_value() != 0 && !cdapMessage.get_obj_value()->is_empty()) {
		cdap::impl::googleprotobuf::objVal_t *gpb_obj_val = new cdap::impl::googleprotobuf::objVal_t();
		switch(cdapMessage.get_obj_value()->isType()) {
		case ObjectValueInterface::inttype:
			gpb_obj_val->set_intval(*(int*)cdapMessage.get_obj_value()->get_value());
			break;
		case ObjectValueInterface::sinttype:
			gpb_obj_val->set_sintval(*(short int*)cdapMessage.get_obj_value()->get_value());
			break;
		case ObjectValueInterface::longtype:
			gpb_obj_val->set_int64val(*(long long*)cdapMessage.get_obj_value()->get_value());
			break;
		case ObjectValueInterface::slongtype:
			gpb_obj_val->set_sint64val(*(long*)cdapMessage.get_obj_value()->get_value());
			break;
		case ObjectValueInterface::stringtype:
			gpb_obj_val->set_strval(*(std::string*)cdapMessage.get_obj_value()->get_value());
			break;
		case ObjectValueInterface::bytetype:
			gpb_obj_val->set_byteval((char*)cdapMessage.get_obj_value()->get_value());
			break;
		case ObjectValueInterface::floattype:
			gpb_obj_val->set_floatval(*(float*)cdapMessage.get_obj_value()->get_value());
			break;
		case ObjectValueInterface::doubletype:
			gpb_obj_val->set_doubleval(*(double*)cdapMessage.get_obj_value()->get_value());
			break;
		case ObjectValueInterface::booltype:
			gpb_obj_val->set_boolval(*(bool*)cdapMessage.get_obj_value()->get_value());
			break;
		}
		gpfCDAPMessage.set_allocated_objvalue(gpb_obj_val);
	}

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

	int size = gpfCDAPMessage.ByteSize();
	char *serialized_message = new char[size];
	gpfCDAPMessage.SerializeToArray(serialized_message,size);
	SerializedMessage *serialized_class = new SerializedMessage(serialized_message, size);

	return serialized_class;
}
}
