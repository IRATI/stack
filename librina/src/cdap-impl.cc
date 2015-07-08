/// cdap.cc
/// Created on: May 22, 2014
/// Author: bernat

#define RINA_PREFIX "librina.cdap-manager"
#include "librina/logs.h"
#include "cdap-impl.h"
#include "CDAP.pb.h"

#include <iostream>

namespace rina {

// CLASS ResetStablishmentTimerTask
ResetStablishmentTimerTask::ResetStablishmentTimerTask(
    ConnectionStateMachine *con_state_machine) {
  con_state_machine_ = con_state_machine;
}
void ResetStablishmentTimerTask::run() {
  con_state_machine_->lock();
  if (con_state_machine_->connection_state_
      == ConnectionStateMachine::AWAITCON) {
    LOG_ERR(
        "M_CONNECT_R message not received within %d ms. Reseting the connection",
        con_state_machine_->timeout_);
    con_state_machine_->resetConnection();
  } else
    con_state_machine_->unlock();
}

// CLASS ReleaseConnectionTimerTask
ReleaseConnectionTimerTask::ReleaseConnectionTimerTask(
    ConnectionStateMachine *con_state_machine) {
  con_state_machine_ = con_state_machine;
}
void ReleaseConnectionTimerTask::run() {
  con_state_machine_->lock();
  if (con_state_machine_->connection_state_
      == ConnectionStateMachine::AWAITCLOSE) {
    LOG_ERR(
        "M_RELEASE_R message not received within %d ms. Reseting the connection",
        con_state_machine_->timeout_);
    con_state_machine_->resetConnection();
  } else
    con_state_machine_->unlock();
}

// CLASS ConnectionStateMachine
ConnectionStateMachine::ConnectionStateMachine(CDAPSessionImpl *cdap_session,
                                               long timeout)
{
	cdap_session_ = cdap_session;
	timeout_ = timeout;
	last_timer_task_ = 0;
	connection_state_ = NONE;
	timer = new Timer();
}

ConnectionStateMachine::~ConnectionStateMachine() throw ()
{
	if (last_timer_task_) {
		timer->cancelTask(last_timer_task_);
	}

	if (timer) {
		delete timer;
		timer = 0;
	}
}

bool ConnectionStateMachine::is_connected()
{
	bool result = false;
	lock();
	result = connection_state_ == CONNECTED;
	unlock();
	return result;
}

bool ConnectionStateMachine::can_send_or_receive_messages()
{
	bool result = false;
	lock();

	//Messages can be sent or received after the M_CONNECT
	//since there might be authentication messages exchanged
	//before the M_CONNECT_R
	if (connection_state_ == CONNECTED ||
			connection_state_ == AWAITCON) {
		result = true;
	}
	unlock();
	return result;
}

void ConnectionStateMachine::checkConnect() {
  lock();
  if (connection_state_ != NONE) {
    std::stringstream ss;
    ss << "Cannot open a new connection because "
       << "this CDAP session is currently in " << connection_state_ << " state";
    unlock();
    throw CDAPException(ss.str());
  }
  unlock();
}
void ConnectionStateMachine::connectSentOrReceived(bool sent) {
  if (sent) {
    connect();
  } else {
    connectReceived();
  }
}
void ConnectionStateMachine::checkConnectResponse() {
  lock();
  if (connection_state_ != AWAITCON) {
    std::stringstream ss;
    ss << "Cannot send a connection response because this CDAP session is currently in "
       << connection_state_ << " state";
    unlock();
    throw CDAPException(ss.str());
  }
  unlock();
}
void ConnectionStateMachine::connectResponseSentOrReceived(bool sent) {
  if (sent) {
    connectResponse();
  } else {
    connectResponseReceived();
  }
}
void ConnectionStateMachine::checkRelease() {
  lock();
  if (connection_state_ != CONNECTED && connection_state_ != AWAITCON) {
    std::stringstream ss;
    ss << "Cannot close a connection because " << "this CDAP session is "
       << "currently in " << connection_state_ << " state";
    unlock();
    throw CDAPException(ss.str());
  }
  unlock();
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
  lock();
  if (connection_state_ != AWAITCLOSE) {
    std::stringstream ss;
    ss << "Cannot send a release connection response message because this CDAP session is currently in "
       << connection_state_ << " state";
    unlock();
    throw CDAPException(ss.str());
  }
  unlock();
}
void ConnectionStateMachine::releaseResponseSentOrReceived(bool sent) {
  if (sent) {
    releaseResponse();
  } else {
    releaseResponseReceived();
  }
}

std::string ConnectionStateMachine::get_state()
{
	std::string result;
	lock();

	switch (connection_state_) {
	case NONE:
		result = CDAPSessionInterface::SESSION_STATE_NONE;
		break;
	case AWAITCON:
		result = CDAPSessionInterface::SESSION_STATE_AWAIT_CON;
		break;
	case CONNECTED:
		result = CDAPSessionInterface::SESSION_STATE_CON;
		break;
	case AWAITCLOSE:
		result = CDAPSessionInterface::SESSION_STATE_AWAIT_CLOSE;
		break;
	default:
		result = "Unknown state";
	}

	unlock();
	return result;
}

void ConnectionStateMachine::resetConnection() {
  connection_state_ = NONE;
  unlock();
}

void ConnectionStateMachine::connect()
{
	checkConnect();
	lock();
	connection_state_ = AWAITCON;
	unlock();
	LOG_DBG("Waiting timeout %d to receive a connection response", timeout_);

	last_timer_task_ = new ResetStablishmentTimerTask(this);
	timer->scheduleTask(last_timer_task_, timeout_);
}

void ConnectionStateMachine::connectReceived()
{
	lock();
	if (connection_state_ != NONE) {
		std::stringstream ss;
		ss << "Cannot open a new connection because this CDAP session is currently in"
		   << connection_state_ << " state";
		unlock();
		throw CDAPException(ss.str());
	}
	connection_state_ = AWAITCON;
	unlock();
}

void ConnectionStateMachine::connectResponse() {
  checkConnectResponse();
  lock();
  connection_state_ = CONNECTED;
  unlock();
}
void ConnectionStateMachine::connectResponseReceived()
{
	lock();
	if (connection_state_ != AWAITCON) {
		std::stringstream ss;
		ss << "Received an M_CONNECT_R message, but this CDAP session is currently in "
		   << connection_state_ << " state";
		unlock();
		throw CDAPException(ss.str());
	}
	LOG_DBG("Connection response received");

	if (last_timer_task_) {
		timer->cancelTask(last_timer_task_);
		last_timer_task_ = 0;
	}

	connection_state_ = CONNECTED;
	unlock();
}

void ConnectionStateMachine::release(const CDAPMessage &cdap_message)
{
	checkRelease();
	lock();
	connection_state_ = AWAITCLOSE;
	unlock();
	if (cdap_message.get_invoke_id() != 0) {
		last_timer_task_ = new ReleaseConnectionTimerTask(this);
		LOG_DBG("Waiting timeout %d to receive a release response", timeout_);
		timer->scheduleTask(last_timer_task_, timeout_);
	}
}

void ConnectionStateMachine::releaseReceived(const CDAPMessage &message) {
  lock();
  if (connection_state_ != CONNECTED && connection_state_ != AWAITCLOSE
		  && connection_state_ != AWAITCON) {
    std::stringstream ss;
    ss << "Cannot close the connection because this CDAP session is currently in "
       << connection_state_ << " state";
    unlock();
    throw CDAPException(ss.str());
  }
  if (message.get_invoke_id() != 0 && connection_state_ != AWAITCLOSE) {
    connection_state_ = AWAITCLOSE;
  } else {
    connection_state_ = NONE;
  }
  unlock();
}

void ConnectionStateMachine::releaseResponse() {
  checkReleaseResponse();
  lock();
  connection_state_ = NONE;
  unlock();
}

void ConnectionStateMachine::releaseResponseReceived()
{
	lock();
	if (connection_state_ != AWAITCLOSE) {
		std::stringstream ss;
		ss << "Received an M_RELEASE_R message, but this CDAP session is currently in "
		   << connection_state_ << " state";
		throw CDAPException(ss.str());
	}
	LOG_DBG("Release response received");
	if (last_timer_task_) {
		timer->cancelTask(last_timer_task_);
		last_timer_task_ = 0;
	}
	unlock();
}

// CLASS CDAPOperationState
CDAPOperationState::CDAPOperationState(CDAPMessage::Opcode op_code,
                                       bool sender) {
  op_code_ = op_code;
  sender_ = sender;
}
CDAPOperationState::~CDAPOperationState() {
}
CDAPMessage::Opcode CDAPOperationState::get_op_code() const {
  return op_code_;
}
bool CDAPOperationState::is_sender() const {
  return sender_;
}

// CLASS CDAPSessionInvokeIdManagerImpl
CDAPInvokeIdManagerImpl::CDAPInvokeIdManagerImpl() {
}
CDAPInvokeIdManagerImpl::~CDAPInvokeIdManagerImpl() throw () {
  used_invoke_sent_ids_.clear();
  used_invoke_recv_ids_.clear();
}
void CDAPInvokeIdManagerImpl::freeInvokeId(int invoke_id, bool sent) {
  lock();
  if (!sent)
    used_invoke_sent_ids_.remove(invoke_id);
  else
    used_invoke_recv_ids_.remove(invoke_id);
  unlock();
}
int CDAPInvokeIdManagerImpl::newInvokeId(bool sent) {
  lock();
  int candidate = 1;
  std::list<int> * invoke_ids;
  if (sent)
    invoke_ids = &used_invoke_sent_ids_;
  else
    invoke_ids = &used_invoke_recv_ids_;
  while (std::find(invoke_ids->begin(), invoke_ids->end(), candidate)
      != invoke_ids->end()) {
    candidate = candidate + 1;
  }
  invoke_ids->push_back(candidate);
  unlock();
  return candidate;
}
void CDAPInvokeIdManagerImpl::reserveInvokeId(int invoke_id, bool sent) {
  lock();
  if (sent)
    used_invoke_sent_ids_.push_back(invoke_id);
  else
    used_invoke_recv_ids_.push_back(invoke_id);
  unlock();
}

// CLASS CDAPSessionImpl
CDAPSessionImpl::CDAPSessionImpl(
    CDAPSessionManager *cdap_session_manager, long timeout,
    WireMessageProviderInterface *wire_message_provider,
    CDAPInvokeIdManagerImpl *invoke_id_manager) {
  cdap_session_manager_ = cdap_session_manager;
  connection_state_machine_ = new ConnectionStateMachine(this, timeout);
  wire_message_provider_ = wire_message_provider;
  invoke_id_manager_ = invoke_id_manager;
  session_descriptor_ = 0;
}

CDAPSessionImpl::~CDAPSessionImpl() throw () {
  delete connection_state_machine_;
  connection_state_machine_ = 0;
  delete session_descriptor_;
  session_descriptor_ = 0;
  for (std::map<int, CDAPOperationState*>::iterator iter =
      pending_messages_sent_.begin(); iter != pending_messages_sent_.end();
      ++iter) {
    delete iter->second;
  }
  pending_messages_sent_.clear();
  for (std::map<int, CDAPOperationState*>::iterator iter =
      pending_messages_recv_.begin(); iter != pending_messages_recv_.end();
      ++iter) {
    delete iter->second;
  }
  pending_messages_recv_.clear();
  for (std::map<int, CDAPOperationState*>::iterator iter =
      cancel_read_pending_messages_.begin();
      iter != cancel_read_pending_messages_.end(); ++iter) {
    delete iter->second;
    iter->second = 0;
  }
  cancel_read_pending_messages_.clear();
}
const SerializedObject* CDAPSessionImpl::encodeNextMessageToBeSent(
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
      check_can_send_or_receive_messages();
      checkInvokeIdNotExists(cdap_message, true);
      break;
    case CDAPMessage::M_CREATE_R:
      check_can_send_or_receive_messages();
      checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_CREATE, true);
      break;
    case CDAPMessage::M_DELETE:
      check_can_send_or_receive_messages();
      checkInvokeIdNotExists(cdap_message, true);
      break;
    case CDAPMessage::M_DELETE_R:
      check_can_send_or_receive_messages();
      checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_DELETE, true);
      break;
    case CDAPMessage::M_START:
      check_can_send_or_receive_messages();
      checkInvokeIdNotExists(cdap_message, true);
      break;
    case CDAPMessage::M_START_R:
      check_can_send_or_receive_messages();
      checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_START, true);
      break;
    case CDAPMessage::M_STOP:
      check_can_send_or_receive_messages();
      checkInvokeIdNotExists(cdap_message, true);
      break;
    case CDAPMessage::M_STOP_R:
      check_can_send_or_receive_messages();
      checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_STOP, true);
      break;
    case CDAPMessage::M_WRITE:
      check_can_send_or_receive_messages();
      checkInvokeIdNotExists(cdap_message, true);
      break;
    case CDAPMessage::M_WRITE_R:
      check_can_send_or_receive_messages();
      checkCanSendOrReceiveResponse(cdap_message, CDAPMessage::M_WRITE, true);
      break;
    case CDAPMessage::M_READ:
      check_can_send_or_receive_messages();
      checkInvokeIdNotExists(cdap_message, true);
      break;
    case CDAPMessage::M_READ_R:
      check_can_send_or_receive_messages();
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
const CDAPMessage* CDAPSessionImpl::messageReceived(
    const SerializedObject &message) {
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

bool CDAPSessionImpl::is_closed() const
{
	if (connection_state_machine_->get_state() ==
			CDAPSessionInterface::SESSION_STATE_NONE) {
		return true;
	}
	return false;
}

std::string CDAPSessionImpl::get_session_state() const
{
	return connection_state_machine_->get_state();

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
      break;
    case CDAPMessage::M_CREATE:
      requestMessageSentOrReceived(cdap_message, CDAPMessage::M_CREATE, sent);
      break;
    case CDAPMessage::M_CREATE_R:
      responseMessageSentOrReceived(cdap_message, CDAPMessage::M_CREATE, sent);
      break;
    case CDAPMessage::M_DELETE:
      requestMessageSentOrReceived(cdap_message, CDAPMessage::M_DELETE, sent);
      break;
    case CDAPMessage::M_DELETE_R:
      responseMessageSentOrReceived(cdap_message, CDAPMessage::M_DELETE, sent);
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
  freeOrReserveInvokeId(cdap_message, sent);
}
void CDAPSessionImpl::freeOrReserveInvokeId(const CDAPMessage &cdap_message,
                                            bool sent) {
  CDAPMessage::Opcode op_code = cdap_message.get_op_code();
  if (op_code == CDAPMessage::M_CONNECT_R || op_code == CDAPMessage::M_RELEASE_R
      || op_code == CDAPMessage::M_CREATE_R
      || op_code == CDAPMessage::M_DELETE_R || op_code == CDAPMessage::M_START_R
      || op_code == CDAPMessage::M_STOP_R || op_code == CDAPMessage::M_WRITE_R
      || op_code == CDAPMessage::M_CANCELREAD_R
      || (op_code == CDAPMessage::M_READ_R
          && cdap_message.get_flags() == CDAPMessage::NONE_FLAGS)
      || cdap_message.get_flags() != CDAPMessage::F_RD_INCOMPLETE) {
    invoke_id_manager_->freeInvokeId(cdap_message.invoke_id_, sent);
  }

  if (cdap_message.get_invoke_id() != 0) {
    if (op_code == CDAPMessage::M_CONNECT || op_code == CDAPMessage::M_RELEASE
        || op_code == CDAPMessage::M_CREATE || op_code == CDAPMessage::M_DELETE
        || op_code == CDAPMessage::M_START || op_code == CDAPMessage::M_STOP
        || op_code == CDAPMessage::M_WRITE
        || op_code == CDAPMessage::M_CANCELREAD
        || op_code == CDAPMessage::M_READ) {
      invoke_id_manager_->reserveInvokeId(cdap_message.get_invoke_id(), sent);
    }
  }
}
void CDAPSessionImpl::checkIsConnected() const {
  if (!connection_state_machine_->is_connected()) {
    throw CDAPException(
        "Cannot send a message because the CDAP session is not in CONNECTED state");
  }
}

void CDAPSessionImpl::check_can_send_or_receive_messages() const
{
	if (!connection_state_machine_->can_send_or_receive_messages()) {
		throw CDAPException("The CDAP session is not in CONN or AWAITCON state");
	}
}

void CDAPSessionImpl::checkInvokeIdNotExists(const CDAPMessage &cdap_message,
                                             bool sent) const {
  const std::map<int, CDAPOperationState*>* pending_messages;
  if (sent)
    pending_messages = &pending_messages_sent_;
  else
    pending_messages = &pending_messages_recv_;

  if (pending_messages->find(cdap_message.get_invoke_id())
      != pending_messages->end()) {
    std::stringstream ss;
    ss << cdap_message.get_invoke_id();
    throw CDAPException("The invokeid " + ss.str() + " already exists");
  }
}
void CDAPSessionImpl::checkCanSendOrReceiveCancelReadRequest(
    const CDAPMessage &cdap_message, bool sent) const {
  bool validationFailed = false;
  const std::map<int, CDAPOperationState*>* pending_messages;
  if (sent)
    pending_messages = &pending_messages_sent_;
  else
    pending_messages = &pending_messages_recv_;

  if (pending_messages->find(cdap_message.get_invoke_id())
      != pending_messages->end()) {
    CDAPOperationState *state = pending_messages->find(
        cdap_message.get_invoke_id())->second;
    if (state->get_op_code() == CDAPMessage::M_READ) {
      validationFailed = true;
    }
    if (sent && !state->is_sender()) {
      validationFailed = true;
    }
    if (!sent && state->is_sender()) {
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
    const CDAPMessage &cdap_message, CDAPMessage::Opcode op_code, bool sent)
{
	check_can_send_or_receive_messages();
	checkInvokeIdNotExists(cdap_message, sent);

	std::map<int, CDAPOperationState*>* pending_messages;
	if (sent)
		pending_messages = &pending_messages_sent_;
	else
		pending_messages = &pending_messages_recv_;

	if (cdap_message.get_invoke_id() != 0) {
		CDAPOperationState *new_operation_state =
				new CDAPOperationState(op_code, sent);
		pending_messages->insert(
				std::pair<int, CDAPOperationState*>(cdap_message.get_invoke_id(),
								    new_operation_state));
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
    bool sender) const {
  bool validation_failed = false;
  const std::map<int, CDAPOperationState*>* pending_messages;
  if (!sender)
    pending_messages = &pending_messages_sent_;
  else
    pending_messages = &pending_messages_recv_;

  std::map<int, CDAPOperationState*>::const_iterator iterator;
  iterator = pending_messages->find(cdap_message.invoke_id_);
  if (iterator == pending_messages->end()) {
    std::stringstream ss;
    ss << "Cannot send a response for the " << op_code
       << " operation with invokeId " << cdap_message.get_invoke_id()
       << std::endl;
    ss << "There are " << pending_messages->size() << " entries";
    throw CDAPException(ss.str());
  }
  CDAPOperationState* state = iterator->second;
  if (state->get_op_code() != op_code) {
    validation_failed = true;
  }
  if (sender && state->is_sender()) {
    validation_failed = true;
  }
  if (!sender && !state->is_sender()) {
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
    const CDAPMessage &cdap_message, CDAPMessage::Opcode op_code, bool sent)
{
	check_can_send_or_receive_messages();
	checkCanSendOrReceiveResponse(cdap_message, op_code, sent);

	bool operation_complete = true;
	std::map<int, CDAPOperationState*>* pending_messages;
	if (!sent)
		pending_messages = &pending_messages_sent_;
	else
		pending_messages = &pending_messages_recv_;

	if (op_code != CDAPMessage::M_READ) {
		CDAPMessage::Flags flags = cdap_message.get_flags();
		if (flags != CDAPMessage::NONE_FLAGS
				&& flags != CDAPMessage::F_RD_INCOMPLETE) {
			operation_complete = false;
		}
	}

	if (operation_complete) {
		std::map<int, CDAPOperationState*>::iterator it =
				pending_messages->find(cdap_message.get_invoke_id());
		delete it->second;
		pending_messages->erase(it);
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
  if (sent)
    pending_messages_sent_.erase(cdap_message.get_invoke_id());
  else
    pending_messages_recv_.erase(cdap_message.get_invoke_id());
}
const SerializedObject* CDAPSessionImpl::serializeMessage(
    const CDAPMessage &cdap_message) const {
  return wire_message_provider_->serializeMessage(cdap_message);
}
const CDAPMessage* CDAPSessionImpl::deserializeMessage(
    const SerializedObject &message) const {
  return wire_message_provider_->deserializeMessage(message);
}
void CDAPSessionImpl::populateSessionDescriptor(const CDAPMessage &cdap_message,
                                                bool send)
{
  session_descriptor_->set_abs_syntax(cdap_message.get_abs_syntax());
  session_descriptor_->set_auth_policy(cdap_message.get_auth_policy());

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
    session_descriptor_->set_src_ap_inst(&cdap_message.get_dest_ap_inst());
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
  wire_message_provider_factory_ = arg0;
  timeout_ = DEFAULT_TIMEOUT_IN_MS;
  wire_message_provider_ = wire_message_provider_factory_
      ->createWireMessageProvider();
  invoke_id_manager_ = new CDAPInvokeIdManagerImpl();
}

CDAPSessionManager::CDAPSessionManager(WireMessageProviderFactory *arg0,
                                       long arg1) {
  wire_message_provider_factory_ = arg0;
  timeout_ = arg1;
  wire_message_provider_ = wire_message_provider_factory_
      ->createWireMessageProvider();
  invoke_id_manager_ = new CDAPInvokeIdManagerImpl();
}

CDAPSessionImpl* CDAPSessionManager::createCDAPSession(int port_id)
{
	WriteScopedLock g(lock);
	if (cdap_sessions_.find(port_id) != cdap_sessions_.end()) {
		return cdap_sessions_.find(port_id)->second;
	} else {
		CDAPSessionImpl *cdap_session =
				new CDAPSessionImpl(this, timeout_,
                                                    wire_message_provider_,
                                                    invoke_id_manager_);
		CDAPSessionDescriptor *descriptor = new CDAPSessionDescriptor(port_id);
		cdap_session->set_session_descriptor(descriptor);
		cdap_sessions_.insert(
				std::pair<int, CDAPSessionImpl*>(port_id, cdap_session));
		return cdap_session;
	}
}

CDAPSessionManager::~CDAPSessionManager() throw ()
{
  delete invoke_id_manager_;
  invoke_id_manager_ = 0;
  for (std::map<int, CDAPSessionImpl*>::iterator iter = cdap_sessions_.begin();
      iter != cdap_sessions_.end(); ++iter) {
    delete iter->second;
    iter->second = 0;
  }
  cdap_sessions_.clear();
  delete wire_message_provider_;
  wire_message_provider_ = 0;
}
void CDAPSessionManager::getAllCDAPSessionIds(std::vector<int> &vector)
{
	ReadScopedLock g(lock);
	vector.clear();
	for (std::map<int, CDAPSessionImpl*>::iterator it = cdap_sessions_.begin();
			it != cdap_sessions_.end(); ++it) {
		vector.push_back(it->first);
	}
}

void CDAPSessionManager::getAllCDAPSessions(
    std::vector<CDAPSessionInterface*> &vector)
{
	ReadScopedLock g(lock);
 	for (std::vector<CDAPSessionInterface*>::iterator iter = vector.begin();
 			iter != vector.end(); ++iter) {
 		delete *iter;
 	}
 	vector.clear();
 	for (std::map<int, CDAPSessionImpl*>::iterator it = cdap_sessions_.begin();
 			it != cdap_sessions_.end(); ++it) {
 		vector.push_back(it->second);
 	}
}

CDAPSessionImpl* CDAPSessionManager::get_cdap_session(int port_id)
{
	ReadScopedLock g(lock);
	std::map<int, CDAPSessionImpl*>::iterator itr = cdap_sessions_.find(port_id);
	if (itr != cdap_sessions_.end())
		return cdap_sessions_.find(port_id)->second;
	else
		return 0;
}

const SerializedObject* CDAPSessionManager::encodeCDAPMessage(
    const CDAPMessage &cdap_message) {
  return wire_message_provider_->serializeMessage(cdap_message);
}
const CDAPMessage* CDAPSessionManager::decodeCDAPMessage(
    const SerializedObject &cdap_message) {
  return wire_message_provider_->deserializeMessage(cdap_message);
}

void CDAPSessionManager::removeCDAPSession(int port_id)
{
	WriteScopedLock g(lock);
	std::map<int, CDAPSessionImpl*>::iterator itr = cdap_sessions_.find(port_id);
	if (itr == cdap_sessions_.end()) {
		return;
	}

	cdap_sessions_.erase(itr);
	delete itr->second;
	itr->second = 0;
}

const SerializedObject* CDAPSessionManager::encodeNextMessageToBeSent(
    const CDAPMessage &cdap_message, int port_id)
{
	std::map<int, CDAPSessionImpl*>::iterator it = cdap_sessions_.find(port_id);
	CDAPSessionInterface* cdap_session;

	if (it == cdap_sessions_.end()) {
		if (cdap_message.get_op_code() == CDAPMessage::M_CONNECT) {
			cdap_session = createCDAPSession(port_id);
		} else {
			std::stringstream ss;
			ss << "There are no open CDAP sessions associated to the flow identified by "
					<< port_id << " right now";
			throw CDAPException(ss.str());
		}
	} else {
		cdap_session = it->second;
	}

	return cdap_session->encodeNextMessageToBeSent(cdap_message);
}

const CDAPMessage* CDAPSessionManager::messageReceived(
    const SerializedObject &encoded_cdap_message, int port_id)
{
  const CDAPMessage *cdap_message = decodeCDAPMessage(encoded_cdap_message);
  CDAPSessionImpl *cdap_session = get_cdap_session(port_id);
  switch (cdap_message->get_op_code()) {
    case CDAPMessage::M_CONNECT:
      if (cdap_session == 0) {
        cdap_session = createCDAPSession(port_id);
        cdap_session->messageReceived(*cdap_message);
        LOG_DBG("Created a new CDAP session for port %d", port_id);
      } else {
        std::stringstream ss;
	ss<< "M_CONNECT received on an already open CDAP Session, over flow "
		<< port_id;
        throw CDAPException(ss.str());
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
      if (cdap_message->get_op_code() == CDAPMessage::M_RELEASE_R) {
        removeCDAPSession(port_id);
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
    ss << "There are no open CDAP sessions associated to the flow identified by "
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

CDAPMessage* CDAPSessionManager::getOpenConnectionRequestMessage(
    int port_id, const AuthPolicy& auth_policy,
    const std::string &dest_ae_inst, const std::string &dest_ae_name,
    const std::string &dest_ap_inst, const std::string &dest_ap_name,
    const std::string &src_ae_inst, const std::string &src_ae_name,
    const std::string &src_ap_inst, const std::string &src_ap_name) {
  CDAPSessionImpl *cdap_session = get_cdap_session(port_id);
  if (cdap_session == 0) {
    cdap_session = createCDAPSession(port_id);
  }
  return CDAPMessage::getOpenConnectionRequestMessage(
      auth_policy, dest_ae_inst, dest_ae_name, dest_ap_inst,
      dest_ap_name, src_ae_inst, src_ae_name, src_ap_inst, src_ap_name,
      cdap_session->get_invoke_id_manager()->newInvokeId(true));
}

CDAPMessage* CDAPSessionManager::getOpenConnectionResponseMessage(
    const AuthPolicy &auth_policy,
    const std::string &dest_ae_inst, const std::string &dest_ae_name,
    const std::string &dest_ap_inst, const std::string &dest_ap_name,
    int result, const std::string &result_reason,
    const std::string &src_ae_inst, const std::string &src_ae_name,
    const std::string &src_ap_inst, const std::string &src_ap_name,
    int invoke_id) {
  return CDAPMessage::getOpenConnectionResponseMessage(auth_policy,
                                                       dest_ae_inst,
                                                       dest_ae_name,
                                                       dest_ap_inst,
                                                       dest_ap_name, result,
                                                       result_reason,
                                                       src_ae_inst, src_ae_name,
                                                       src_ap_inst, src_ap_name,
                                                       invoke_id);
}

CDAPMessage* CDAPSessionManager::getReleaseConnectionRequestMessage(
    int port_id, CDAPMessage::Flags flags, bool invoke_id) {
  CDAPMessage *cdap_message = CDAPMessage::getReleaseConnectionRequestMessage(
      flags);
  assignInvokeId(*cdap_message, invoke_id, port_id, true);
  return cdap_message;
}

CDAPMessage* CDAPSessionManager::getReleaseConnectionResponseMessage(
    CDAPMessage::Flags flags, int result, const std::string &result_reason,
    int invoke_id) {
  return CDAPMessage::getReleaseConnectionResponseMessage(flags, result,
                                                          result_reason,
                                                          invoke_id);
}
CDAPMessage* CDAPSessionManager::getCreateObjectRequestMessage(
    int port_id, char * filter, CDAPMessage::Flags flags,
    const std::string &obj_class, long obj_inst, const std::string &obj_name,
    int scope, bool invoke_id) {
  CDAPMessage *cdap_message = CDAPMessage::getCreateObjectRequestMessage(
      filter, flags, obj_class, obj_inst, obj_name, scope);
  assignInvokeId(*cdap_message, invoke_id, port_id, true);
  return cdap_message;

}

CDAPMessage* CDAPSessionManager::getCreateObjectResponseMessage(
    CDAPMessage::Flags flags, const std::string &obj_class, long obj_inst,
    const std::string &obj_name, int result, const std::string &result_reason,
    int invoke_id) {
  return CDAPMessage::getCreateObjectResponseMessage(flags, obj_class, obj_inst,
                                                     obj_name, result,
                                                     result_reason, invoke_id);
}

CDAPMessage* CDAPSessionManager::getDeleteObjectRequestMessage(
    int port_id, char* filter, CDAPMessage::Flags flags,
    const std::string &obj_class, long obj_inst, const std::string &obj_name,
    int scope, bool invoke_id) {
  CDAPMessage *cdap_message = CDAPMessage::getDeleteObjectRequestMessage(
      filter, flags, obj_class, obj_inst, obj_name, scope);
  assignInvokeId(*cdap_message, invoke_id, port_id, true);
  return cdap_message;
}

CDAPMessage* CDAPSessionManager::getDeleteObjectResponseMessage(
    CDAPMessage::Flags flags, const std::string &obj_class, long obj_inst,
    const std::string &obj_name, int result, const std::string &result_reason,
    int invoke_id) {
  return CDAPMessage::getDeleteObjectResponseMessage(flags, obj_class, obj_inst,
                                                     obj_name, result,
                                                     result_reason, invoke_id);
}

CDAPMessage* CDAPSessionManager::getStartObjectRequestMessage(
    int port_id, char * filter, CDAPMessage::Flags flags,
    const std::string &obj_class, long obj_inst, const std::string &obj_name,
    int scope, bool invoke_id) {
  CDAPMessage *cdap_message = CDAPMessage::getStartObjectRequestMessage(
      filter, flags, obj_class, obj_inst, obj_name, scope);
  assignInvokeId(*cdap_message, invoke_id, port_id, true);
  return cdap_message;
}

CDAPMessage* CDAPSessionManager::getStartObjectResponseMessage(
    CDAPMessage::Flags flags, int result, const std::string &result_reason,
    int invoke_id) {
  return CDAPMessage::getStartObjectResponseMessage(flags, result,
                                                    result_reason, invoke_id);
}

CDAPMessage* CDAPSessionManager::getStartObjectResponseMessage(
    CDAPMessage::Flags flags, const std::string &obj_class, long obj_inst,
    const std::string &obj_name, int result, const std::string &result_reason,
    int invoke_id) {
  return CDAPMessage::getStartObjectResponseMessage(flags, obj_class, obj_inst,
                                                    obj_name, result,
                                                    result_reason, invoke_id);
}

CDAPMessage* CDAPSessionManager::getStopObjectRequestMessage(
    int port_id, char* filter, CDAPMessage::Flags flags,
    const std::string &obj_class, long obj_inst, const std::string &obj_name,
    int scope, bool invoke_id) {
  CDAPMessage *cdap_message = CDAPMessage::getStopObjectRequestMessage(
      filter, flags, obj_class, obj_inst, obj_name, scope);
  assignInvokeId(*cdap_message, invoke_id, port_id, true);
  return cdap_message;
}

CDAPMessage* CDAPSessionManager::getStopObjectResponseMessage(
    CDAPMessage::Flags flags, int result, const std::string &result_reason,
    int invoke_id) {
  return CDAPMessage::getStopObjectResponseMessage(flags, result, result_reason,
                                                   invoke_id);
}

CDAPMessage* getStopObjectResponseMessage(CDAPMessage::Flags flags, int result,
                                          const std::string &result_reason,
                                          int invoke_id) {
  return CDAPMessage::getStopObjectResponseMessage(flags, result, result_reason,
                                                   invoke_id);
}

CDAPMessage* CDAPSessionManager::getReadObjectRequestMessage(
    int port_id, char * filter, CDAPMessage::Flags flags,
    const std::string &obj_class, long obj_inst, const std::string &obj_name,
    int scope, bool invoke_id) {
  CDAPMessage *cdap_message = CDAPMessage::getReadObjectRequestMessage(
      filter, flags, obj_class, obj_inst, obj_name, scope);
  assignInvokeId(*cdap_message, invoke_id, port_id, true);
  return cdap_message;
}

CDAPMessage* CDAPSessionManager::getReadObjectResponseMessage(
    CDAPMessage::Flags flags, const std::string &obj_class, long obj_inst,
    const std::string &obj_name, int result, const std::string &result_reason,
    int invoke_id) {
  return CDAPMessage::getReadObjectResponseMessage(flags, obj_class, obj_inst,
                                                   obj_name, result,
                                                   result_reason, invoke_id);
}

CDAPMessage* CDAPSessionManager::getWriteObjectRequestMessage(
    int port_id, char * filter, CDAPMessage::Flags flags,
    const std::string &obj_class, long obj_inst, const std::string &obj_name,
    int scope, bool invoke_id) {
  CDAPMessage *cdap_message = CDAPMessage::getWriteObjectRequestMessage(
      filter, flags, obj_class, obj_inst, obj_name, scope);
  assignInvokeId(*cdap_message, invoke_id, port_id, true);
  return cdap_message;
}

CDAPMessage* CDAPSessionManager::getWriteObjectResponseMessage(
    CDAPMessage::Flags flags, int result, const std::string &result_reason,
    int invoke_id) {
  return CDAPMessage::getWriteObjectResponseMessage(flags, result, invoke_id,
                                                    result_reason);
}

CDAPMessage* CDAPSessionManager::getCancelReadRequestMessage(
    CDAPMessage::Flags flags, int invoke_id) {
  return CDAPMessage::getCancelReadRequestMessage(flags, invoke_id);
}

CDAPMessage* CDAPSessionManager::getCancelReadResponseMessage(
    CDAPMessage::Flags flags, int invoke_id, int result,
    const std::string &result_reason) {
  return CDAPMessage::getCancelReadResponseMessage(flags, invoke_id, result,
                                                   result_reason);
}

CDAPMessage* CDAPSessionManager::getRequestMessage(int port_id,
			CDAPMessage::Opcode opcode, char * filter,
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name,
			int scope, bool invoke_id)
{
	CDAPMessage *cdap_message = CDAPMessage::getRequestMessage(opcode,
			filter, flags, obj_class, obj_inst, obj_name, scope);
	assignInvokeId(*cdap_message, invoke_id, port_id, true);
	return cdap_message;
}

CDAPMessage* CDAPSessionManager::getResponseMessage(CDAPMessage::Opcode opcode,
		CDAPMessage::Flags flags, const std::string &obj_class,
		long obj_inst, const std::string &obj_name,
		int result, const std::string &result_reason, int invoke_id)
{
	  return CDAPMessage::getResponseMessage(opcode, flags, obj_class, obj_inst,
			  obj_name, result, result_reason, invoke_id);
}

CDAPInvokeIdManagerInterface * CDAPSessionManager::get_invoke_id_manager()
{
	return invoke_id_manager_;
}

void CDAPSessionManager::assignInvokeId(CDAPMessage &cdap_message,
                                        bool invoke_id, int port_id,
                                        bool sent) {
  CDAPSessionImpl *cdap_session = 0;

  if (invoke_id) {
    cdap_session = get_cdap_session(port_id);
    if (cdap_session) {
      cdap_message.set_invoke_id(
          cdap_session->get_invoke_id_manager()->newInvokeId(sent));
    }
  }
}

// CLASS GPBWireMessageProvider
const CDAPMessage* GPBWireMessageProvider::deserializeMessage(
    const SerializedObject &message) {
  cdap::impl::googleprotobuf::CDAPMessage gpfCDAPMessage;
  CDAPMessage *cdapMessage = new CDAPMessage;

  gpfCDAPMessage.ParseFromArray(message.message_, message.size_);
  cdapMessage->set_abs_syntax(gpfCDAPMessage.abssyntax());

  AuthPolicy auth_policy;
  auth_policy.name_ = gpfCDAPMessage.authpolicy().name();
  for(int i=0; i<gpfCDAPMessage.authpolicy().versions_size(); i++) {
  	auth_policy.versions_.push_back(
  			gpfCDAPMessage.authpolicy().versions(i));
  }
  if (gpfCDAPMessage.authpolicy().has_options()) {
	  char *val = new char[gpfCDAPMessage.authpolicy().options().size()];
	  memcpy(val,
	         gpfCDAPMessage.authpolicy().options().data(),
	         gpfCDAPMessage.authpolicy().options().size());
	  SerializedObject sobj(val,
			  	gpfCDAPMessage.authpolicy().options().size());
	  auth_policy.options_ = sobj;
  }
  cdapMessage->auth_policy_ = auth_policy;

  if (gpfCDAPMessage.has_destaeinst())
    cdapMessage->set_dest_ae_inst(gpfCDAPMessage.destaeinst());
  if (gpfCDAPMessage.has_destaename())
    cdapMessage->set_dest_ae_name(gpfCDAPMessage.destaename());
  if (gpfCDAPMessage.has_destapinst())
    cdapMessage->set_dest_ap_inst(gpfCDAPMessage.destapinst());
  if (gpfCDAPMessage.has_destapname())
    cdapMessage->set_dest_ap_name(gpfCDAPMessage.destapname());
  if (gpfCDAPMessage.has_filter()) {
    char *filter = new char[gpfCDAPMessage.filter().size() + 1];
    strcpy(filter, gpfCDAPMessage.filter().c_str());
    ;
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
  if (obj_val_t.has_intval()) {
    cdapMessage->set_obj_value(new IntObjectValue(obj_val_t.intval()));
  }
  if (obj_val_t.has_sintval()) {
    cdapMessage->set_obj_value(new SIntObjectValue(obj_val_t.sintval()));
  }
  if (obj_val_t.has_int64val()) {
    cdapMessage->set_obj_value(new LongObjectValue(obj_val_t.int64val()));
  }
  if (obj_val_t.has_sint64val()) {
    cdapMessage->set_obj_value(new SLongObjectValue(obj_val_t.sint64val()));
  }
  if (obj_val_t.has_strval()) {
    cdapMessage->set_obj_value(new StringObjectValue(obj_val_t.strval()));
  }
  if (obj_val_t.has_floatval()) {
    cdapMessage->set_obj_value(new FloatObjectValue(obj_val_t.floatval()));
  }
  if (obj_val_t.has_doubleval()) {
    cdapMessage->set_obj_value(new DoubleObjectValue(obj_val_t.doubleval()));
  }
  if (obj_val_t.has_boolval()) {
    cdapMessage->set_obj_value(new BooleanObjectValue(obj_val_t.boolval()));
  }
  if (obj_val_t.has_byteval()) {
    char *byte_val = new char[obj_val_t.byteval().size()];
    SerializedObject sr_message(byte_val, obj_val_t.byteval().size());
    memcpy(byte_val, obj_val_t.byteval().data(), obj_val_t.byteval().size());
    cdapMessage->set_obj_value(new ByteArrayObjectValue(sr_message));
  }

  int opcode_val = gpfCDAPMessage.opcode();
  CDAPMessage::Opcode opcode = static_cast<CDAPMessage::Opcode>(opcode_val);
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
const SerializedObject* GPBWireMessageProvider::serializeMessage(
    const CDAPMessage &cdapMessage) {
  cdap::impl::googleprotobuf::CDAPMessage gpfCDAPMessage;
  gpfCDAPMessage.set_abssyntax(cdapMessage.get_abs_syntax());

  cdap::impl::googleprotobuf::authPolicy_t *gpb_auth_policy =
                  new cdap::impl::googleprotobuf::authPolicy_t();
  AuthPolicy auth_policy = cdapMessage.auth_policy_;
  gpb_auth_policy->set_name(auth_policy.name_);
  for(std::list<std::string>::iterator it = auth_policy.versions_.begin();
  		it != auth_policy.versions_.end(); ++it) {
  	gpb_auth_policy->add_versions(*it);
  }
  if (auth_policy.options_.size_ > 0) {
  	gpb_auth_policy->set_options(auth_policy.options_.message_,
  				     auth_policy.options_.size_);
  }
  gpfCDAPMessage.set_allocated_authpolicy(gpb_auth_policy);

  gpfCDAPMessage.set_destaeinst(cdapMessage.get_dest_ae_inst());
  gpfCDAPMessage.set_destaename(cdapMessage.get_dest_ae_name());
  gpfCDAPMessage.set_destapinst(cdapMessage.get_dest_ap_inst());
  gpfCDAPMessage.set_destapname(cdapMessage.get_dest_ap_name());
  if (cdapMessage.get_filter() != 0) {
    gpfCDAPMessage.set_filter(cdapMessage.get_filter());
  }
  gpfCDAPMessage.set_invokeid(cdapMessage.get_invoke_id());
  gpfCDAPMessage.set_objclass(cdapMessage.get_obj_class());
  gpfCDAPMessage.set_objinst(cdapMessage.get_obj_inst());
  gpfCDAPMessage.set_objname(cdapMessage.get_obj_name());

  if (cdapMessage.get_obj_value() != 0
      && !cdapMessage.get_obj_value()->is_empty()) {
    cdap::impl::googleprotobuf::objVal_t *gpb_obj_val =
        new cdap::impl::googleprotobuf::objVal_t();
    switch (cdapMessage.get_obj_value()->isType()) {
      case ObjectValueInterface::inttype:
        gpb_obj_val->set_intval(
            *(int*) cdapMessage.get_obj_value()->get_value());
        break;
      case ObjectValueInterface::sinttype:
        gpb_obj_val->set_sintval(
            *(short int*) cdapMessage.get_obj_value()->get_value());
        break;
      case ObjectValueInterface::longtype:
        gpb_obj_val->set_int64val(
            *(long long*) cdapMessage.get_obj_value()->get_value());
        break;
      case ObjectValueInterface::slongtype:
        gpb_obj_val->set_sint64val(
            *(long*) cdapMessage.get_obj_value()->get_value());
        break;
      case ObjectValueInterface::stringtype:
        gpb_obj_val->set_strval(
            *(std::string*) cdapMessage.get_obj_value()->get_value());
        break;
      case ObjectValueInterface::bytetype: {
        SerializedObject * serialized_object = (SerializedObject*) cdapMessage
            .get_obj_value()->get_value();
        gpb_obj_val->set_byteval(serialized_object->message_,
                                 serialized_object->size_);
        break;
      }
      case ObjectValueInterface::floattype:
        gpb_obj_val->set_floatval(
            *(float*) cdapMessage.get_obj_value()->get_value());
        break;
      case ObjectValueInterface::doubletype:
        gpb_obj_val->set_doubleval(
            *(double*) cdapMessage.get_obj_value()->get_value());
        break;
      case ObjectValueInterface::booltype:
        gpb_obj_val->set_boolval(
            *(bool*) cdapMessage.get_obj_value()->get_value());
        break;
    }
    gpfCDAPMessage.set_allocated_objvalue(gpb_obj_val);
  }

  if (!cdap::impl::googleprotobuf::opCode_t_IsValid(
      cdapMessage.get_op_code())) {
    throw CDAPException("Serializing Message: Not a valid OpCode");
  }

  gpfCDAPMessage.set_opcode(
      (cdap::impl::googleprotobuf::opCode_t) cdapMessage.get_op_code());
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
  gpfCDAPMessage.SerializeToArray(serialized_message, size);
  SerializedObject *serialized_class = new SerializedObject(serialized_message,
                                                            size);

  return serialized_class;
}
}
