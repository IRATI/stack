/*
 * CDAP implementation
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This library is free software{} you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation{} either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY{} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library{} if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */
#include <algorithm>

#define RINA_PREFIX "cdap"

#include "librina/logs.h"
#include "librina/application.h"
#include "librina/timer.h"
#include "librina/cdap_v2.h"
#include "librina/exceptions.h"

#include "CDAP.pb.h"

namespace rina {
namespace cdap {

// CLASS CDAPException
CDAPException::CDAPException() :
Exception("CDAP message caused an Exception") {
	result_ = OTHER;
}

CDAPException::CDAPException(std::string result_reason) :
Exception(result_reason.c_str()) {
	result_ = OTHER;
}
CDAPException::CDAPException(ErrorCode result, std::string error_message) :
Exception(error_message.c_str()) {
	result_ = result;
}
CDAPException::ErrorCode CDAPException::get_result() const {
	return result_;
}

// STRUCT CDAPMessage
CDAPMessage::CDAPMessage()
{
	abs_syntax_ = 0;
	filter_ = 0;
	flags_ = cdap_rib::flags_t::NONE_FLAGS;
	invoke_id_ = 0;
	obj_inst_ = 0;
	obj_value_.size_ = 0;
	op_code_ = NONE_OPCODE;
	result_ = 0;
	scope_ = 0;
	version_ = 0;
}

bool CDAPMessage::is_request_message() const
{
	if (op_code_ == M_CONNECT || op_code_ == M_RELEASE ||
			op_code_ == M_CREATE || op_code_ == M_DELETE ||
			op_code_ == M_READ || op_code_ == M_WRITE ||
			op_code_ == M_START || op_code_ == M_STOP ||
			op_code_ == M_CANCELREAD) {
		return true;
	} else {
		return false;
	}
}

std::string CDAPMessage::opcodeToString() const
{
	std::string result;

	switch(op_code_) {
	case M_CONNECT:
		result = "0_M_CONNECT";
		break;
	case M_CONNECT_R:
		result = "1_M_CONNECT_R";
		break;
	case M_RELEASE:
		result = "2_M_RELEASE";
		break;
	case M_RELEASE_R:
		result = "3_M_RELEASER";
		break;
	case M_CREATE:
		result = "4_M_CREATE";
		break;
	case M_CREATE_R:
		result = "5_M_CREATE_R";
		break;
	case M_DELETE:
		result = "6_M_DELETE";
		break;
	case M_DELETE_R:
		result = "7_M_DELETE_R";
		break;
	case M_READ:
		result = "8_M_READ";
		break;
	case M_READ_R:
		result = "9_M_READ_R";
		break;
	case M_CANCELREAD:
		result = "10_M_CANCELREAD";
		break;
	case M_CANCELREAD_R:
		result = "11_M_CANCELREAD_R";
		break;
	case M_WRITE:
		result = "12_M_WRITE";
		break;
	case M_WRITE_R:
		result = "13_M_WRITE_R";
		break;
	case M_START:
		result = "14_M_START";
		break;
	case M_START_R:
		result = "15_M_START_r";
		break;
	case M_STOP:
		result = "16_M_STOP";
		break;
	case M_STOP_R:
		result = "17_M_STOP_R";
		break;
	case NONE_OPCODE:
		result = "18_NON_OPCODE";
		break;
	default:
		result = "Wrong operation code";
	}

	return result;
}

std::string CDAPMessage::to_string() const
{
	std::stringstream ss;
	ss << "Opcode: "<< opcodeToString() << std::endl;
	if (op_code_ == CDAPMessage::M_CONNECT
			|| op_code_ == CDAPMessage::M_CONNECT_R) {
		if (abs_syntax_ != 0)
			ss << "Abstract syntax: " << abs_syntax_ << std::endl;
		ss << "Authentication policy: " << auth_policy_.to_string() << std::endl;
		if (!src_ap_name_.empty())
			ss << "Source AP name: " << src_ap_name_ << std::endl;
		if (!src_ap_inst_.empty())
			ss << "Source AP instance: " << src_ap_inst_ << std::endl;
		if (!src_ae_name_.empty())
			ss << "Source AE name: " << src_ae_name_ << std::endl;
		if (!src_ae_inst_.empty())
			ss << "Source AE instance: " << src_ae_inst_ << std::endl;
		if (!dest_ap_name_.empty())
			ss << "Destination AP name: " << dest_ap_name_ << std::endl;
		if (!dest_ap_inst_.empty())
			ss << "Destination AP instance: " << dest_ap_inst_<< std::endl;
		if (!dest_ae_name_.empty())
			ss << "Destination AE name: " << dest_ae_name_ << std::endl;
		if (!dest_ae_inst_.empty())
			ss << "Destination AE instance: " << dest_ae_inst_ << std::endl;
	}
	if (filter_ != 0)
		ss << "Filter: " << filter_ << std::endl;
	if (flags_ != cdap_rib::flags_t::NONE_FLAGS)
		ss << "Flags: " << flags_ << std::endl;
	if (invoke_id_ != 0)
		ss << "Invoke id: " << invoke_id_ << std::endl;
	if (!obj_class_.empty())
		ss << "Object class: " << obj_class_ << std::endl;
	if (!obj_name_.empty())
		ss << "Object name: " << obj_name_ << std::endl;
	if (obj_inst_ != 0)
		ss << "Object instance: " << obj_inst_ << std::endl;
	if (op_code_ == CDAPMessage::M_CONNECT_R
			|| op_code_ == CDAPMessage::M_RELEASE_R
			|| op_code_ == CDAPMessage::M_READ_R
			|| op_code_ == CDAPMessage::M_WRITE_R
			|| op_code_ == CDAPMessage::M_CANCELREAD_R
			|| op_code_ == CDAPMessage::M_START_R
			|| op_code_ == CDAPMessage::M_STOP_R
			|| op_code_ == CDAPMessage::M_CREATE_R
			|| op_code_ == CDAPMessage::M_DELETE_R) {
		ss << "Result: " << result_ << std::endl;
		if (!result_reason_.empty())
			ss << "Result Reason: " << result_reason_ << std::endl;
	}
	if (op_code_ == CDAPMessage::M_READ
			|| op_code_ == CDAPMessage::M_WRITE
			|| op_code_ == CDAPMessage::M_CANCELREAD
			|| op_code_ == CDAPMessage::M_START
			|| op_code_ == CDAPMessage::M_STOP
			|| op_code_ == CDAPMessage::M_CREATE
			|| op_code_ == CDAPMessage::M_DELETE) {
		ss << "Scope: " << scope_ << std::endl;
	}
	if (version_ != 0)
		ss << "Version: " << version_ << std::endl;
	return ss.str();
}

const int CDAPMessageFactory::ABSTRACT_SYNTAX_VERSION = 0x0073;

class ResetStablishmentTimerTask;
class ReleaseConnectionTimerTask;
class CDAPSession;
/// Implements the CDAP connection state machine
class ConnectionStateMachine : public rina::Lockable
{
 public:
	ConnectionStateMachine(CDAPSession* cdap_session, long timeout);
	~ConnectionStateMachine() throw ();
	bool is_connected() const;
	bool is_await_conn() const;
	/// Checks if a the CDAP connection can be opened (i.e. an M_CONNECT message can be sent)
	/// @throws CDAPException
	void checkConnect();
	void connectSentOrReceived(bool sent);
	/// Checks if the CDAP M_CONNECT_R message can be sent
	/// @throws CDAPException
	void checkConnectResponse();
	void connectResponseSentOrReceived(bool sent);
	/// Checks if the CDAP M_RELEASE message can be sent
	/// @throws CDAPException
	void checkRelease();
	void releaseSentOrReceived(const cdap_m_t &cdap_message, bool sent);
	/// Checks if the CDAP M_RELEASE_R message can be sent
	/// @throws CDAPException
	void checkReleaseResponse();
	void releaseResponseSentOrReceived(bool sent);
	bool can_send_or_receive_messages();
 private:
	enum ConnectionState
	{
		NONE,
		AWAITCON,
		CONNECTED,
		AWAITCLOSE
	};
	void resetConnection();
	void noConnectionResponse();
	/// The AE has sent an M_CONNECT message
	/// @throws CDAPException
	void connect();
	/// An M_CONNECT message has been received, update the state
	/// @param message
	void connectReceived();
	/// The AE has sent an M_CONNECT_R  message
	/// @param openConnectionResponseMessage
	/// @throws CDAPException
	void connectResponse();
	/// An M_CONNECT_R message has been received
	/// @param message
	void connectResponseReceived();
	/// The AE has sent an M_RELEASE message
	/// @param releaseConnectionRequestMessage
	/// @throws CDAPException
	void release(const cdap_m_t &cdap_m_t);
	/// An M_RELEASE message has been received
	/// @param message
	void releaseReceived(const cdap_m_t &message);
	/// The AE has called the close response operation
	/// @param releaseConnectionRequestMessage
	/// @throws CDAPException
	void releaseResponse();
	/// An M_RELEASE_R message has been received
	/// @param message
	void releaseResponseReceived();
	/// The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	long timeout_;
	/// The flow that this CDAP connection operates over
	CDAPSession *cdap_session_;
	/// The state of the CDAP connection, drives the CDAP connection
	/// state machine
	ConnectionState connection_state_;
	Timer timer;
	TimerTask * last_timer_task;
	friend class ResetStablishmentTimerTask;
	friend class ReleaseConnectionTimerTask;
};

/// It will always try to use short invokeIds (as close to 1 as possible)
class CDAPInvokeIdManagerImpl : public CDAPInvokeIdManager, rina::Lockable
{
 public:
	CDAPInvokeIdManagerImpl();
	~CDAPInvokeIdManagerImpl() throw ();
	void freeInvokeId(int invoke_id, bool sent);
	int newInvokeId(bool sent);
	void reserveInvokeId(int invoke_id, bool sent);
 private:
	std::list<int> used_invoke_sent_ids_;
	std::list<int> used_invoke_recv_ids_;
};

/// Encapsulates an operation state
class CDAPOperationState
{
 public:
	CDAPOperationState(cdap_m_t::Opcode op_code, bool sender);
	~CDAPOperationState();
	cdap_m_t::Opcode get_op_code() const;
	bool is_sender() const;
 private:
	/// The opcode of the message
	cdap_m_t::Opcode op_code_;
	/// Tells if sender or receiver
	bool sender_;
};

// TODO: make these two classes one
class ResetStablishmentTimerTask : public rina::TimerTask
{
 public:
	ResetStablishmentTimerTask(ConnectionStateMachine *con_state_machine);
	void run();
 private:
	ConnectionStateMachine *con_state_machine_;
};

class ReleaseConnectionTimerTask : public rina::TimerTask
{
 public:
	ReleaseConnectionTimerTask(ConnectionStateMachine *con_state_machine);
	void run();
 private:
	ConnectionStateMachine *con_state_machine_;
};

/// Validates that a CDAP message is well formed
class CDAPMessageValidator
{
 public:
	/// Validates a CDAP message
	/// @param message
	/// @throws CDAPException thrown if the CDAP message is not valid, indicating the reason
	static void validate(const cdap_m_t &message);
 private:
	static void validateAbsSyntax(const cdap_m_t &message);
	static void validateDestAEInst(const cdap_m_t &message);
	static void validateDestAEName(const cdap_m_t &message);
	static void validateDestApInst(const cdap_m_t &message);
	static void validateDestApName(const cdap_m_t &message);
	static void validateFilter(const cdap_m_t &message);
	static void validateInvokeID(const cdap_m_t &message);
	static void validateObjClass(const cdap_m_t &message);
	static void validateObjInst(const cdap_m_t &message);
	static void validateObjName(const cdap_m_t &message);
	static void validateObjValue(const cdap_m_t &message);
	static void validateOpcode(const cdap_m_t &message);
	static void validateResult(const cdap_m_t &message);
	static void validateResultReason(const cdap_m_t &message);
	static void validateScope(const cdap_m_t &message);
	static void validateSrcAEInst(const cdap_m_t &message);
	static void validateSrcAEName(const cdap_m_t &message);
	static void validateSrcApInst(const cdap_m_t &message);
	static void validateSrcApName(const cdap_m_t &message);
	static void validateVersion(const cdap_m_t &message);
};

/// Implements a CDAP session. Has the necessary logic to ensure that a
/// the operation of the CDAP protocol is consistent (correct states and proper
/// invocation of operations)
class CDAPSessionManager;
class CDAPSession
{
 public:
	CDAPSession(CDAPSessionManager *cdap_session_manager, long timeout,
		    CDAPMessageEncoder *encoder,
		    CDAPInvokeIdManagerImpl *invoke_id_manager);
	~CDAPSession() throw ();
	//const CDAPSessionInvokeIdManagerInterface* getInvokeIdManager();
	//bool isConnected() const;
	void encodeNextMessageToBeSent(const cdap_m_t &cdap_message,
				       ser_obj_t& result);
	void messageSent(const cdap_m_t &cdap_message);
	void messageReceived(const ser_obj_t& message,
			     cdap_m_t& result);
	void messageReceived(const cdap_m_t &cdap_message);
	int get_port_id() const;
	CDAPInvokeIdManagerImpl* get_invoke_id_manager() const;
	void stopConnection();
	bool is_in_await_con_state();
	const cdap_rib::con_handle_t& get_con_handle();
	void set_port_id(int port_id);
	void check_can_send_or_receive_messages() const;
 private:
	void messageSentOrReceived(const cdap_m_t &cdap_message, bool sent);
	void freeOrReserveInvokeId(const cdap_m_t &cdap_message, bool sent);
	void checkIsConnected() const;
	void checkInvokeIdNotExists(int invoke_id,
				    bool sent) const;
	void checkCanSendOrReceiveCancelReadRequest(int invoke_id,
						    bool sent) const;
	void requestMessageSentOrReceived(const cdap_m_t &cdap_message,
					  cdap_m_t::Opcode op_code,
					  bool sent);
	void cancelReadMessageSentOrReceived(const cdap_m_t &cdap_message,
					     bool sender);
	void checkCanSendOrReceiveResponse(int invoke_id,
					   cdap_m_t::Opcode op_code,
					   bool sender) const;
	void checkCanSendOrReceiveCancelReadResponse(int invoke_id,
						     bool send) const;
	void responseMessageSentOrReceived(const cdap_m_t &cdap_message,
					   cdap_m_t::Opcode op_code,
					   bool sent);
	void cancelReadResponseMessageSentOrReceived(const cdap_m_t &cdap_message,
						     bool sent);
	void serializeMessage(const cdap_m_t &cdap_message,
			      ser_obj_t& result) const;
	void deserializeMessage(const ser_obj_t &message,
			        cdap_m_t& result) const;
	void populate_con_handle(const cdap_m_t &cdap_message, bool send);
	/// This map contains the invokeIds of the messages that
	/// have requested a response, except for the M_CANCELREADs
	std::map<int, CDAPOperationState*> pending_messages_sent_;
	std::map<int, CDAPOperationState*> pending_messages_recv_;
	std::map<int, CDAPOperationState*> cancel_read_pending_messages_;
	/// Deals with the connection establishment and deletion messages and states
	ConnectionStateMachine *connection_state_machine_;
	/// This map contains the invokeIds of the messages that
	/// have requested a response, except for the M_CANCELREADs
	//std::map<int, CDAPOperationState> pending_messages_;
	//std::map<int, CDAPOperationState> cancel_read_pending_messages_;
	CDAPMessageEncoder *encoder;
	cdap_rib::con_handle_t con_handle;
	CDAPSessionManager *cdap_session_manager_;
	CDAPInvokeIdManagerImpl *invoke_id_manager_;
};

/// Implements a CDAP session manager.
class CDAPSessionManager : public CDAPSessionManagerInterface
{
 public:
	static const long DEFAULT_TIMEOUT_IN_MS = 10000;
	CDAPSessionManager();
	CDAPSessionManager(cdap_rib::concrete_syntax_t& syntax);
	CDAPSessionManager(cdap_rib::concrete_syntax_t& syntax,
			   long timeout);
	~CDAPSessionManager() throw ();
	void set_timeout(long timeout);
	CDAPSession* createCDAPSession(int port_id);
	void getAllCDAPSessionIds(std::vector<int> &vector);
	CDAPSession* get_cdap_session(int port_id);
	void encodeCDAPMessage(const cdap_m_t &cdap_message,
			       ser_obj_t& result);
	void decodeCDAPMessage(const ser_obj_t &cdap_message,
			       cdap_m_t& result);
	void removeCDAPSession(int portId);
	bool session_in_await_con_state(int portId);
	void encodeNextMessageToBeSent(const cdap_m_t &cdap_message,
				       ser_obj_t& result,
				       int port_id);
	void messageReceived(const ser_obj_t &encodedcdap_m_t,
			     cdap_m_t& result,
			     int portId);
	void messageSent(const cdap_m_t &cdap_message,
			 int port_id);
	int get_port_id(std::string destination_application_process_name);
	void getOpenConnectionRequestMessage(cdap_m_t & msg,
				    	     const cdap_rib::con_handle_t &con);
	void getOpenConnectionResponseMessage(cdap_m_t & msg,
					      const cdap_rib::con_handle_t &con,
					      const cdap_rib::res_info_t &res,
					      int invoke_id);
	void getReleaseConnectionRequestMessage(cdap_m_t & msg,
						const cdap_rib::flags_t &flags,
						bool invoke_id);
	void getReleaseConnectionResponseMessage(cdap_m_t & msg,
						 const cdap_rib::flags_t &flags,
						 const cdap_rib::res_info_t &res,
						 int invoke_id);
	void getCreateObjectRequestMessage(cdap_m_t & msg,
					   const cdap_rib::filt_info_t &filt,
					   const cdap_rib::flags_t &flags,
					   const cdap_rib::obj_info_t &obj,
					   bool invoke_id);
	void getCreateObjectResponseMessage(cdap_m_t & msg,
					    const cdap_rib::flags_t &flags,
					    const cdap_rib::obj_info_t &obj,
					    const cdap_rib::res_info_t &res,
					    int invoke_id);
	void getDeleteObjectRequestMessage(cdap_m_t & msg,
					   const cdap_rib::filt_info_t &filt,
					   const cdap_rib::flags_t &flags,
					   const cdap_rib::obj_info_t &obj,
					   bool invoke_id);
	void getDeleteObjectResponseMessage(cdap_m_t & msg,
					    const cdap_rib::flags_t &flags,
					    const cdap_rib::obj_info_t &obj,
					    const cdap_rib::res_info_t &res,
					    int invoke_id);
	void getStartObjectRequestMessage(cdap_m_t & msg,
					  const cdap_rib::filt_info_t &filt,
					  const cdap_rib::flags_t &flags,
					  const cdap_rib::obj_info_t &obj,
					  bool invoke_id);
	void getStartObjectResponseMessage(cdap_m_t & msg,
					   const cdap_rib::flags_t &flags,
					   const cdap_rib::res_info_t &res,
					   int invoke_id);
	void getStartObjectResponseMessage(cdap_m_t & msg,
					   const cdap_rib::flags_t &flags,
					   const cdap_rib::obj_info_t &obj,
					   const cdap_rib::res_info_t &res,
					   int invoke_id);
	void getStopObjectRequestMessage(cdap_m_t & msg,
					 const cdap_rib::filt_info_t &filt,
					 const cdap_rib::flags_t &flags,
					 const cdap_rib::obj_info_t &obj,
					 bool invoke_id);
	void getStopObjectResponseMessage(cdap_m_t & msg,
					  const cdap_rib::flags_t &flags,
					  const cdap_rib::res_info_t &res,
					  int invoke_id);
	void getReadObjectRequestMessage(cdap_m_t & msg,
					 const cdap_rib::filt_info_t &filt,
					 const cdap_rib::flags_t &flags,
					 const cdap_rib::obj_info_t &obj,
					 bool invoke_id);
	void getReadObjectResponseMessage(cdap_m_t & msg,
					  const cdap_rib::flags_t &flags,
					  const cdap_rib::obj_info_t &obj,
					  const cdap_rib::res_info_t &res,
					  int invoke_id);
	void getWriteObjectRequestMessage(cdap_m_t & msg,
					  const cdap_rib::filt_info_t &filt,
					  const cdap_rib::flags_t &flags,
					  const cdap_rib::obj_info_t &obj,
					  bool invoke_id);
	void getWriteObjectResponseMessage(cdap_m_t & msg,
					   const cdap_rib::flags_t &flags,
					   const cdap_rib::res_info_t &res,
					   int invoke_id);
	void getCancelReadRequestMessage(cdap_m_t & msg,
					 const cdap_rib::flags_t &flags,
					 int invoke_id);
	void getCancelReadResponseMessage(cdap_m_t & msg,
					  const cdap_rib::flags_t &flags,
					  const cdap_rib::res_info_t &res,
					  int invoke_id);
	CDAPInvokeIdManager * get_invoke_id_manager();
	const cdap_rib::con_handle_t& get_con_handle(int port_id);

 private:
	std::map<int, CDAPSession*> cdap_sessions_;
	/// The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	CDAPMessageEncoder * encoder;
	long timeout_;
	CDAPInvokeIdManagerImpl *invoke_id_manager_;
};

class AppCDAPIOHandler : public CDAPIOHandler
{
 public:
	AppCDAPIOHandler(){};
	void send(const cdap_m_t & m_sent,
		  const cdap_rib::con_handle_t &con);
	void process_message(const ser_obj_t &message,
			     unsigned int handle,
			     cdap_rib::cdap_dest_t cdap_dest);

 private:
        // Lock to control that when sending a message requiring
	// a reply the CDAP Session manager has been updated before
	// receiving the response message
        rina::Lockable atomic_send_lock_;
};

/// Google Protocol Buffers Wire Message Provider
class GPBSerializer : public SerializerInterface
{
 public:
	void deserializeMessage(const ser_obj_t &message,
				cdap_m_t& result);
	void serializeMessage(const cdap_m_t &cdapMessage,
			      ser_obj_t& result);
};

// CLASS CDAPMessageFactory
void CDAPMessageFactory::getOpenConnectionRequestMessage(cdap_m_t & msg,
							 const cdap_rib::con_handle_t &con,
							 int invoke_id)
{
	msg.abs_syntax_ = ABSTRACT_SYNTAX_VERSION;
	msg.op_code_ = cdap_m_t::M_CONNECT;
	msg.auth_policy_.name = con.auth_.name;
	msg.auth_policy_.versions = con.auth_.versions;
	if (con.auth_.options.size_ > 0)
		msg.auth_policy_.options = con.auth_.options;
	msg.dest_ae_inst_ = con.dest_.ae_inst_;
	msg.dest_ae_name_ = con.dest_.ae_name_;
	msg.dest_ap_inst_ = con.dest_.ap_inst_;
	msg.dest_ap_name_ = con.dest_.ap_name_;
	msg.invoke_id_ = invoke_id;
	msg.src_ae_inst_ = con.src_.ae_inst_;
	msg.src_ae_name_ = con.src_.ae_name_;
	msg.src_ap_inst_ = con.src_.ap_inst_;
	msg.src_ap_name_ = con.src_.ap_name_;
	msg.version_ = 1;
}

void CDAPMessageFactory::getOpenConnectionResponseMessage(cdap_m_t & msg,
							  const cdap_rib::con_handle_t &con,
							  const cdap_rib::res_info_t &res,
							  int invoke_id)
{
	msg.abs_syntax_ = ABSTRACT_SYNTAX_VERSION;
	msg.op_code_ = cdap_m_t::M_CONNECT_R;
	if (con.auth_.options.size_ > 0)
		msg.auth_policy_.options = con.auth_.options;
	msg.dest_ae_inst_ = con.dest_.ae_inst_;
	msg.dest_ae_name_ = con.dest_.ae_name_;
	msg.dest_ap_inst_ = con.dest_.ap_inst_;
	msg.dest_ap_name_ = con.dest_.ap_name_;
	msg.invoke_id_ = invoke_id;
	msg.src_ae_inst_ = con.src_.ae_inst_;
	msg.src_ae_name_ = con.src_.ae_name_;
	msg.src_ap_inst_ = con.src_.ap_inst_;
	msg.src_ap_name_ = con.src_.ap_name_;
	msg.version_ = 1;
	msg.result_ = res.code_;
	msg.result_reason_ = res.reason_;
}

void CDAPMessageFactory::getReleaseConnectionRequestMessage(cdap_m_t & msg,
							    const cdap_rib::flags_t &flags)
{
	msg.op_code_ = cdap_m_t::M_RELEASE;
	msg.flags_ = flags.flags_;
}

void CDAPMessageFactory::getReleaseConnectionResponseMessage(cdap_m_t & msg,
							     const cdap_rib::flags_t &flags,
							     const cdap_rib::res_info_t &res,
							     int invoke_id)
{
	msg.op_code_ = cdap_m_t::M_RELEASE_R;

	msg.flags_ = flags.flags_;
	msg.invoke_id_ = invoke_id;
	msg.result_ = res.code_;
	msg.result_reason_ = res.reason_;
}

void CDAPMessageFactory::getCreateObjectRequestMessage(cdap_m_t & msg,
						       const cdap_rib::filt_info_t &filt,
						       const cdap_rib::flags_t &flags,
						       const cdap_rib::obj_info_t &obj)
{
	msg.op_code_ = cdap_m_t::M_CREATE;
	msg.filter_ = filt.filter_;
	msg.scope_ = filt.scope_;
	msg.flags_ = flags.flags_;
	msg.obj_class_ = obj.class_;
	msg.obj_inst_ = obj.inst_;
	msg.obj_name_ = obj.name_;
	msg.obj_value_ = obj.value_;
}

void CDAPMessageFactory::getCreateObjectResponseMessage(cdap_m_t & msg,
							const cdap_rib::flags_t &flags,
							const cdap_rib::obj_info_t &obj,
							const cdap_rib::res_info_t &res,
							int invoke_id)
{
	msg.op_code_ = cdap_m_t::M_CREATE_R;
	msg.flags_ = flags.flags_;
	msg.invoke_id_ = invoke_id;
	msg.obj_class_ = obj.class_;
	msg.obj_inst_ = obj.inst_;
	msg.obj_name_ = obj.name_;
	msg.obj_value_ = obj.value_;
	msg.result_ = res.code_;
	msg.result_reason_ = res.reason_;
}

void CDAPMessageFactory::getDeleteObjectRequestMessage(cdap_m_t & msg,
						       const cdap_rib::filt_info_t &filt,
						       const cdap_rib::flags_t &flags,
						       const cdap_rib::obj_info_t &obj)
{
	msg.op_code_ = cdap_m_t::M_DELETE;
	msg.filter_ = filt.filter_;
	msg.scope_ = filt.scope_;
	msg.flags_ = flags.flags_;
	msg.obj_class_ = obj.class_;
	msg.obj_inst_ = obj.inst_;
	msg.obj_name_ = obj.name_;
}

void CDAPMessageFactory::getDeleteObjectResponseMessage(cdap_m_t & msg,
							const cdap_rib::flags_t &flags,
							const cdap_rib::obj_info_t &obj,
							const cdap_rib::res_info_t &res,
							int invoke_id)
{
	msg.op_code_ = cdap_m_t::M_DELETE_R;
	msg.flags_ = flags.flags_;
	msg.invoke_id_ = invoke_id;
	msg.obj_class_ = obj.class_;
	msg.obj_inst_ = obj.inst_;
	msg.obj_name_ = obj.name_;
	msg.result_ = res.code_;
	msg.result_reason_ = res.reason_;
}

void CDAPMessageFactory::getStartObjectRequestMessage(cdap_m_t & msg,
						      const cdap_rib::filt_info_t &filt,
						      const cdap_rib::flags_t &flags,
						      const cdap_rib::obj_info_t &obj)
{
	msg.op_code_ = cdap_m_t::M_START;
	msg.filter_ = filt.filter_;
	msg.scope_ = filt.scope_;
	msg.flags_ = flags.flags_;
	msg.obj_class_ = obj.class_;
	msg.obj_inst_ = obj.inst_;
	msg.obj_name_ = obj.name_;
	msg.obj_value_ = obj.value_;
}

void CDAPMessageFactory::getStartObjectResponseMessage(cdap_m_t & msg,
						       const cdap_rib::flags_t &flags,
						       const cdap_rib::res_info_t &res,
						       int invoke_id)
{
	msg.op_code_ = cdap_m_t::M_START_R;
	msg.flags_ = flags.flags_;
	msg.invoke_id_ = invoke_id;
	msg.result_ = res.code_;
	msg.result_reason_ = res.reason_;
}

void CDAPMessageFactory::getStartObjectResponseMessage(cdap_m_t & msg,
						       const cdap_rib::flags_t &flags,
						       const cdap_rib::obj_info_t &obj,
						       const cdap_rib::res_info_t &res,
						       int invoke_id)
{
	msg.op_code_ = cdap_m_t::M_START_R;
	msg.flags_ = flags.flags_;
	msg.invoke_id_ = invoke_id;
	msg.result_ = res.code_;
	msg.result_reason_ = res.reason_;
	msg.obj_class_ = obj.class_;
	msg.obj_inst_ = obj.inst_;
	msg.obj_name_ = obj.name_;
	msg.obj_value_ = obj.value_;
}

void CDAPMessageFactory::getStopObjectRequestMessage(cdap_m_t & msg,
						     const cdap_rib::filt_info_t &filt,
						     const cdap_rib::flags_t &flags,
						     const cdap_rib::obj_info_t &obj)
{
	msg.op_code_ = cdap_m_t::M_STOP;
	msg.filter_ = filt.filter_;
	msg.scope_ = filt.scope_;
	msg.flags_ = flags.flags_;
	msg.obj_class_ = obj.class_;
	msg.obj_inst_ = obj.inst_;
	msg.obj_name_ = obj.name_;
	msg.obj_value_ = obj.value_;
}

void CDAPMessageFactory::getStopObjectResponseMessage(cdap_m_t & msg,
						      const cdap_rib::flags_t &flags,
						      const cdap_rib::res_info_t &res,
						      int invoke_id)
{
	msg.op_code_ = cdap_m_t::M_STOP_R;
	msg.flags_ = flags.flags_;
	msg.invoke_id_ = invoke_id;
	msg.result_ = res.code_;
	msg.result_reason_ = res.reason_;
}

void CDAPMessageFactory::getReadObjectRequestMessage(cdap_m_t & msg,
						     const cdap_rib::filt_info_t &filt,
						     const cdap_rib::flags_t &flags,
						     const cdap_rib::obj_info_t &obj)
{
	msg.op_code_ = cdap_m_t::M_READ;
	msg.filter_ = filt.filter_;
	msg.scope_ = filt.scope_;
	msg.flags_ = flags.flags_;
	msg.obj_class_ = obj.class_;
	msg.obj_inst_ = obj.inst_;
	msg.obj_name_ = obj.name_;
}

void CDAPMessageFactory::getReadObjectResponseMessage(cdap_m_t & msg,
						      const cdap_rib::flags_t &flags,
						      const cdap_rib::obj_info_t &obj,
						      const cdap_rib::res_info_t &res,
						      int invoke_id)
{
	msg.op_code_ = cdap_m_t::M_READ_R;
	msg.flags_ = flags.flags_;
	msg.invoke_id_ = invoke_id;
	msg.obj_class_ = obj.class_;
	msg.obj_inst_ = obj.inst_;
	msg.obj_name_ = obj.name_;
	msg.obj_value_ = obj.value_;
	msg.result_ = res.code_;
	msg.result_reason_ = res.reason_;
}

void CDAPMessageFactory::getWriteObjectRequestMessage(cdap_m_t & msg,
						      const cdap_rib::filt_info_t &filt,
						      const cdap_rib::flags_t &flags,
						      const cdap_rib::obj_info_t &obj)
{
	msg.op_code_ = cdap_m_t::M_WRITE;
	msg.filter_ = filt.filter_;
	msg.scope_ = filt.scope_;
	msg.flags_ = flags.flags_;
	msg.obj_class_ = obj.class_;
	msg.obj_inst_ = obj.inst_;
	msg.obj_name_ = obj.name_;
	msg.obj_value_ = obj.value_;
}

void CDAPMessageFactory::getWriteObjectResponseMessage(cdap_m_t & msg,
						       const cdap_rib::flags_t &flags,
						       const cdap_rib::res_info_t &res,
						       int invoke_id)
{
	msg.op_code_ = cdap_m_t::M_WRITE_R;
	msg.flags_ = flags.flags_;
	msg.invoke_id_ = invoke_id;
	msg.result_ = res.code_;
	msg.result_reason_ = res.reason_;
}

void CDAPMessageFactory::getCancelReadRequestMessage(cdap_m_t & msg,
						     const cdap_rib::flags_t &flags,
						     int invoke_id)
{
	msg.op_code_ = cdap_m_t::M_CANCELREAD;
	msg.flags_ = flags.flags_;
	msg.invoke_id_ = invoke_id;
}

void CDAPMessageFactory::getCancelReadResponseMessage(cdap_m_t & msg,
						      const cdap_rib::flags_t &flags,
						      const cdap_rib::res_info_t &res,
						      int invoke_id)
{
	msg.op_code_ = cdap_m_t::M_CANCELREAD_R;
	msg.flags_ = flags.flags_;
	msg.invoke_id_ = invoke_id;
	msg.result_ = res.code_;
	msg.result_reason_ = res.reason_;
}

// CLASS CDAPSessionInvokeIdManagerImpl
CDAPInvokeIdManagerImpl::CDAPInvokeIdManagerImpl()
{
}
CDAPInvokeIdManagerImpl::~CDAPInvokeIdManagerImpl() throw ()
{
	used_invoke_sent_ids_.clear();
	used_invoke_recv_ids_.clear();
}
void CDAPInvokeIdManagerImpl::freeInvokeId(int invoke_id, bool sent)
{
	lock();
	if (!sent)
		used_invoke_sent_ids_.remove(invoke_id);
	else
		used_invoke_recv_ids_.remove(invoke_id);
	unlock();
}
int CDAPInvokeIdManagerImpl::newInvokeId(bool sent)
{
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
void CDAPInvokeIdManagerImpl::reserveInvokeId(int invoke_id, bool sent)
{
	lock();
	if (sent)
		used_invoke_sent_ids_.push_back(invoke_id);
	else
		used_invoke_recv_ids_.push_back(invoke_id);
	unlock();
}

// CLASS CDAPOperationState
CDAPOperationState::CDAPOperationState(cdap_m_t::Opcode op_code, bool sender)
{
	op_code_ = op_code;
	sender_ = sender;
}
CDAPOperationState::~CDAPOperationState()
{
}
cdap_m_t::Opcode CDAPOperationState::get_op_code() const
{
	return op_code_;
}
bool CDAPOperationState::is_sender() const
{
	return sender_;
}

// CLASS ConnectionStateMachine
ConnectionStateMachine::ConnectionStateMachine(CDAPSession *cdap_session,
					       long timeout)
{
	cdap_session_ = cdap_session;
	timeout_ = timeout;
	last_timer_task = 0;
	lock();
	connection_state_ = NONE;
	unlock();;
}
ConnectionStateMachine::~ConnectionStateMachine() throw ()
{
	if (last_timer_task)
		timer.cancelTask(last_timer_task);

	last_timer_task = 0;
}

bool ConnectionStateMachine::is_connected() const
{
	return connection_state_ == CONNECTED;
}
bool ConnectionStateMachine::is_await_conn() const
{
	return connection_state_ == AWAITCON;
}
void ConnectionStateMachine::checkConnect()
{
	lock();
	if (connection_state_ != NONE) {
		std::stringstream ss;
		ss << "Cannot open a new connection because "
		   << "this CDAP session is currently in " << connection_state_
		   << " state";
		unlock();
		throw CDAPException(ss.str());
	}
	unlock();
}
void ConnectionStateMachine::connectSentOrReceived(bool sent)
{
	if (sent) {
		connect();
	} else {
		connectReceived();
	}
}
void ConnectionStateMachine::checkConnectResponse()
{
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
void ConnectionStateMachine::connectResponseSentOrReceived(bool sent)
{
	if (sent) {
		connectResponse();
	} else {
		connectResponseReceived();
	}
}
void ConnectionStateMachine::checkRelease()
{
	lock();
	if (connection_state_ != CONNECTED) {
		std::stringstream ss;
		ss << "Cannot close a connection because "
		   << "this CDAP session is " << "currently in "
		   << connection_state_ << " state";
		unlock();
		throw CDAPException(ss.str());
	}
	unlock();
}
void ConnectionStateMachine::releaseSentOrReceived(const cdap_m_t &cdap_message,
						   bool sent)
{
	if (sent) {
		release(cdap_message);
	} else {
		releaseReceived(cdap_message);
	}
}
void ConnectionStateMachine::checkReleaseResponse()
{
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
void ConnectionStateMachine::releaseResponseSentOrReceived(bool sent)
{
	if (sent) {
		releaseResponse();
	} else {
		releaseResponseReceived();
	}
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
void ConnectionStateMachine::resetConnection()
{
	connection_state_ = NONE;
	unlock();
	cdap_session_->stopConnection();
}
void ConnectionStateMachine::connect()
{
	checkConnect();
	lock();
	connection_state_ = AWAITCON;
	unlock();
	LOG_DBG("Waiting timeout %d to receive a connection response",
		timeout_);
	last_timer_task = new ResetStablishmentTimerTask(this);
	timer.scheduleTask(last_timer_task, timeout_);
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
void ConnectionStateMachine::connectResponse()
{
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
	if (last_timer_task) {
		timer.cancelTask(last_timer_task);
		last_timer_task = 0;
	}
	connection_state_ = CONNECTED;
	unlock();
}
void ConnectionStateMachine::release(const cdap_m_t &cdap_message)
{
	checkRelease();
	lock();
	connection_state_ = AWAITCLOSE;
	unlock();
	if (cdap_message.invoke_id_ != 0) {
		last_timer_task = new ReleaseConnectionTimerTask(this);
		LOG_DBG("Waiting timeout %d to receive a release response",
			timeout_);
		timer.scheduleTask(last_timer_task, timeout_);
	}

}
void ConnectionStateMachine::releaseReceived(const cdap_m_t &message)
{
	lock();
	if (connection_state_ != CONNECTED && connection_state_ != AWAITCLOSE) {
		std::stringstream ss;
		ss << "Cannot close the connection because this CDAP session is currently in "
		   << connection_state_ << " state";
		unlock();
		throw CDAPException(ss.str());
	}
	if (message.invoke_id_ != 0 && connection_state_ != AWAITCLOSE) {
		connection_state_ = AWAITCLOSE;
	} else {
		connection_state_ = NONE;
		cdap_session_->stopConnection();
	}
	unlock();
}
void ConnectionStateMachine::releaseResponse()
{
	checkReleaseResponse();
	lock();
	connection_state_ = NONE;
	unlock();
}
void ConnectionStateMachine::releaseResponseReceived()
{
	if (connection_state_ != AWAITCLOSE) {
		std::stringstream ss;
		ss << "Received an M_RELEASE_R message, but this CDAP session is currently in "
		   << connection_state_ << " state";
		throw CDAPException(ss.str());
	}
	LOG_DBG("Release response received");
	if (last_timer_task) {
		timer.cancelTask(last_timer_task);
		last_timer_task = 0;
	}
}

// CLASS ResetStablishmentTimerTask
ResetStablishmentTimerTask::ResetStablishmentTimerTask(
		ConnectionStateMachine *con_state_machine)
{
	con_state_machine_ = con_state_machine;
}
void ResetStablishmentTimerTask::run()
{
	con_state_machine_->lock();
	if (con_state_machine_->connection_state_
			== ConnectionStateMachine::AWAITCON) {
		LOG_ERR( "M_CONNECT_R message not received within %d ms. Reseting the connection",
			con_state_machine_->timeout_);
		con_state_machine_->resetConnection();
	} else
		con_state_machine_->unlock();
}

// CLASS ReleaseConnectionTimerTask
ReleaseConnectionTimerTask::ReleaseConnectionTimerTask(
		ConnectionStateMachine *con_state_machine)
{
	con_state_machine_ = con_state_machine;
}
void ReleaseConnectionTimerTask::run()
{
	con_state_machine_->lock();
	if (con_state_machine_->connection_state_
			== ConnectionStateMachine::AWAITCLOSE) {
		LOG_ERR( "M_RELEASE_R message not received within %d ms. Reseting the connection",
			con_state_machine_->timeout_);
		con_state_machine_->resetConnection();
	} else
		con_state_machine_->unlock();
}

/* CLASS CDAPMessageValidator */
void CDAPMessageValidator::validate(const cdap_m_t &message)
{
	validateAbsSyntax(message);
	validateDestAEInst(message);
	validateDestAEName(message);
	validateDestApInst(message);
	validateDestApName(message);
	validateFilter(message);
	validateInvokeID(message);
	validateObjClass(message);
	validateObjInst(message);
	validateObjName(message);
	validateObjValue(message);
	validateOpcode(message);
	validateResult(message);
	validateResultReason(message);
	validateScope(message);
	validateSrcAEInst(message);
	validateSrcAEName(message);
	validateSrcApInst(message);
	validateSrcApName(message);
	validateVersion(message);
}

void CDAPMessageValidator::validateAbsSyntax(const cdap_m_t &message)
{
	if (message.abs_syntax_ == 0) {
		if (message.op_code_ == cdap_m_t::M_CONNECT
				|| message.op_code_ == cdap_m_t::M_CONNECT_R) {
			throw CDAPException(
					"AbsSyntax must be set for M_CONNECT and M_CONNECT_R messages");
		}
	} else {
		if ((message.op_code_ != cdap_m_t::M_CONNECT)
				&& (message.op_code_ != cdap_m_t::M_CONNECT_R)) {
			throw CDAPException(
					"AbsSyntax can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestAEInst(const cdap_m_t &message)
{
	if (!message.dest_ae_inst_.empty()) {
		if ((message.op_code_ != cdap_m_t::M_CONNECT)
				&& (message.op_code_ != cdap_m_t::M_CONNECT_R)) {
			throw CDAPException(
					"dest_ae_inst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestAEName(const cdap_m_t &message)
{
	if (!message.dest_ae_name_.empty()) {
		if ((message.op_code_ != cdap_m_t::M_CONNECT)
				&& (message.op_code_ != cdap_m_t::M_CONNECT_R)) {
			throw CDAPException(
					"DestAEName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestApInst(const cdap_m_t &message)
{
	if (!message.dest_ap_inst_.empty()) {
		if (message.op_code_ != cdap_m_t::M_CONNECT
				&& message.op_code_ != cdap_m_t::M_CONNECT_R) {
			throw CDAPException(
					"DestApInst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestApName(const cdap_m_t &message)
{
	if (message.dest_ap_name_.empty()) {
		if (message.op_code_ == cdap_m_t::M_CONNECT) {
			throw CDAPException(
					"DestApName must be set for the M_CONNECT message");
		} else if (message.op_code_ == cdap_m_t::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message.op_code_ != cdap_m_t::M_CONNECT
				&& message.op_code_ != cdap_m_t::M_CONNECT_R) {
			throw CDAPException(
					"DestApName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateFilter(const cdap_m_t &message)
{
	if (message.filter_ != 0) {
		if (message.op_code_ != cdap_m_t::M_CREATE
				&& message.op_code_ != cdap_m_t::M_DELETE
				&& message.op_code_ != cdap_m_t::M_READ
				&& message.op_code_ != cdap_m_t::M_WRITE
				&& message.op_code_ != cdap_m_t::M_START
				&& message.op_code_ != cdap_m_t::M_STOP) {
			throw CDAPException(
					"The filter parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages");
		}
	}
}

void CDAPMessageValidator::validateInvokeID(const cdap_m_t &message)
{
	if (message.invoke_id_ == 0) {
		if (message.op_code_ == cdap_m_t::M_CONNECT
				|| message.op_code_ == cdap_m_t::M_CONNECT_R
				|| message.op_code_ == cdap_m_t::M_RELEASE_R
				|| message.op_code_ == cdap_m_t::M_CREATE_R
				|| message.op_code_ == cdap_m_t::M_DELETE_R
				|| message.op_code_ == cdap_m_t::M_READ_R
				|| message.op_code_ == cdap_m_t::M_CANCELREAD
				|| message.op_code_ == cdap_m_t::M_CANCELREAD_R
				|| message.op_code_ == cdap_m_t::M_WRITE_R
				|| message.op_code_ == cdap_m_t::M_START_R
				|| message.op_code_ == cdap_m_t::M_STOP_R) {
			throw CDAPException(
					"The invoke id parameter cannot be 0");
		}
	}
}

void CDAPMessageValidator::validateObjClass(const cdap_m_t &message)
{
	if (!message.obj_class_.empty()) {
		if (message.obj_name_.empty()) {
			throw CDAPException(
					"If the objClass parameter is set, the objName parameter also has to be set");
		}
		if (message.op_code_ != cdap_m_t::M_CREATE
				&& message.op_code_ != cdap_m_t::M_CREATE_R
				&& message.op_code_ != cdap_m_t::M_DELETE
				&& message.op_code_ != cdap_m_t::M_DELETE_R
				&& message.op_code_ != cdap_m_t::M_READ
				&& message.op_code_ != cdap_m_t::M_READ_R
				&& message.op_code_ != cdap_m_t::M_WRITE
				&& message.op_code_ != cdap_m_t::M_WRITE_R
				&& message.op_code_ != cdap_m_t::M_START
				&& message.op_code_ != cdap_m_t::M_STOP
				&& message.op_code_ != cdap_m_t::M_START_R
				&& message.op_code_ != cdap_m_t::M_STOP_R) {
			throw CDAPException(
					"The objClass parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R, M_STOP_R messages");
		}
	}
}

void CDAPMessageValidator::validateObjInst(const cdap_m_t &message)
{
	if (message.obj_inst_ != 0) {
		if (message.op_code_ != cdap_m_t::M_CREATE
				&& message.op_code_ != cdap_m_t::M_CREATE_R
				&& message.op_code_ != cdap_m_t::M_DELETE
				&& message.op_code_ != cdap_m_t::M_DELETE_R
				&& message.op_code_ != cdap_m_t::M_READ
				&& message.op_code_ != cdap_m_t::M_READ_R
				&& message.op_code_ != cdap_m_t::M_WRITE
				&& message.op_code_ != cdap_m_t::M_WRITE_R
				&& message.op_code_ != cdap_m_t::M_START
				&& message.op_code_ != cdap_m_t::M_STOP
				&& message.op_code_ != cdap_m_t::M_START_R
				&& message.op_code_ != cdap_m_t::M_STOP_R) {
			throw CDAPException(
					"The objInst parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_START_R, M_STOP and M_STOP_R messages");
		}
	}
}

void CDAPMessageValidator::validateObjName(const cdap_m_t &message)
{
	if (!message.obj_name_.empty()) {
		if (message.obj_class_.empty()) {
			throw new CDAPException(
					"If the objName parameter is set, the objClass parameter also has to be set");
		}
		if (message.op_code_ != cdap_m_t::M_CREATE
				&& message.op_code_ != cdap_m_t::M_CREATE_R
				&& message.op_code_ != cdap_m_t::M_DELETE
				&& message.op_code_ != cdap_m_t::M_DELETE_R
				&& message.op_code_ != cdap_m_t::M_READ
				&& message.op_code_ != cdap_m_t::M_READ_R
				&& message.op_code_ != cdap_m_t::M_WRITE
				&& message.op_code_ != cdap_m_t::M_WRITE_R
				&& message.op_code_ != cdap_m_t::M_START
				&& message.op_code_ != cdap_m_t::M_STOP
				&& message.op_code_ != cdap_m_t::M_START_R
				&& message.op_code_ != cdap_m_t::M_STOP_R) {
			throw CDAPException(
					"The objName parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R and M_STOP_R messages");
		}
	}
}

void CDAPMessageValidator::validateObjValue(const cdap_m_t &message)
{
	if (message.obj_value_.size_ == 0) {
		if (message.op_code_ == cdap_m_t::M_WRITE) {
			throw CDAPException(
					"The objValue parameter must be set for M_WRITE messages");
		}
	} else {
		if (message.op_code_ != cdap_m_t::M_CREATE
				&& message.op_code_ != cdap_m_t::M_CREATE_R
				&& message.op_code_ != cdap_m_t::M_READ_R
				&& message.op_code_ != cdap_m_t::M_WRITE
				&& message.op_code_ != cdap_m_t::M_START
				&& message.op_code_ != cdap_m_t::M_STOP
				&& message.op_code_ != cdap_m_t::M_START_R
				&& message.op_code_ != cdap_m_t::M_STOP_R
				&& message.op_code_ != cdap_m_t::M_WRITE_R
				&& message.op_code_ != cdap_m_t::M_DELETE
				&& message.op_code_ != cdap_m_t::M_READ) {
			throw CDAPException(
					"The objValue parameter can only be set for M_CREATE, M_DELETE, M_CREATE_R, M_READ_R, M_WRITE, M_START, M_START_R, M_STOP, M_STOP_R and M_WRITE_R messages");
		}
	}
}

void CDAPMessageValidator::validateOpcode(const cdap_m_t &message)
{
	if (message.op_code_ == cdap_m_t::NONE_OPCODE) {
		throw CDAPException(
				"The opcode must be set for all the messages");
	}
}

void CDAPMessageValidator::validateResult(const cdap_m_t &message)
{
	/* FIXME: Do something with sense */
}

void CDAPMessageValidator::validateResultReason(const cdap_m_t &message)
{
	if (!message.result_reason_.empty()) {
		if (message.op_code_ != cdap_m_t::M_CREATE_R
				&& message.op_code_ != cdap_m_t::M_DELETE_R
				&& message.op_code_ != cdap_m_t::M_READ_R
				&& message.op_code_ != cdap_m_t::M_WRITE_R
				&& message.op_code_ != cdap_m_t::M_CONNECT_R
				&& message.op_code_ != cdap_m_t::M_RELEASE_R
				&& message.op_code_ != cdap_m_t::M_CANCELREAD
				&& message.op_code_ != cdap_m_t::M_CANCELREAD_R
				&& message.op_code_ != cdap_m_t::M_START_R
				&& message.op_code_ != cdap_m_t::M_STOP_R) {
			throw CDAPException(
					"The resultReason parameter can only be set for M_CREATE_R, M_DELETE_R, M_READ_R, M_WRITE_R, M_START_R, M_STOP_R, M_CONNECT_R, M_RELEASE_R, M_CANCELREAD and M_CANCELREAD_R messages");
		}
	}
}

void CDAPMessageValidator::validateScope(const cdap_m_t &message)
{
	if (message.scope_ != 0) {
		if (message.op_code_ != cdap_m_t::M_CREATE
				&& message.op_code_ != cdap_m_t::M_DELETE
				&& message.op_code_ != cdap_m_t::M_READ
				&& message.op_code_ != cdap_m_t::M_WRITE
				&& message.op_code_ != cdap_m_t::M_START
				&& message.op_code_ != cdap_m_t::M_STOP) {
			throw CDAPException(
					"The scope parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages");
		}
	}
}

void CDAPMessageValidator::validateSrcAEInst(const cdap_m_t &message)
{
	if (!message.src_ae_inst_.empty()) {
		if (message.op_code_ != cdap_m_t::M_CONNECT
				&& message.op_code_ != cdap_m_t::M_CONNECT_R) {
			throw CDAPException(
					"SrcAEInst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateSrcAEName(const cdap_m_t &message)
{
	if (!message.src_ae_name_.empty()) {
		if (message.op_code_ != cdap_m_t::M_CONNECT
				&& message.op_code_ != cdap_m_t::M_CONNECT_R) {
			throw CDAPException(
					"SrcAEName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateSrcApInst(const cdap_m_t &message)
{
	if (!message.src_ap_inst_.empty()) {
		if (message.op_code_ != cdap_m_t::M_CONNECT
				&& message.op_code_ != cdap_m_t::M_CONNECT_R) {
			throw CDAPException(
					"SrcApInst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateSrcApName(const cdap_m_t &message)
{
	if (message.src_ap_name_.empty()) {
		if (message.op_code_ == cdap_m_t::M_CONNECT) {
			throw CDAPException(
					"SrcApName must be set for the M_CONNECT message");
		} else if (message.op_code_ == cdap_m_t::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message.op_code_ != cdap_m_t::M_CONNECT
				&& message.op_code_ != cdap_m_t::M_CONNECT_R) {
			throw CDAPException(
					"SrcApName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateVersion(const cdap_m_t &message)
{
	if (message.version_ == 0) {
		if (message.op_code_ == cdap_m_t::M_CONNECT
				|| message.op_code_ == cdap_m_t::M_CONNECT_R) {
			throw CDAPException(
					"Version must be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

// CLASS CDAPSession
CDAPSession::CDAPSession(CDAPSessionManager *cdap_session_manager,
			 long timeout,
			 CDAPMessageEncoder *enc,
			 CDAPInvokeIdManagerImpl *invoke_id_manager)
{
	cdap_session_manager_ = cdap_session_manager;
	connection_state_machine_ = new ConnectionStateMachine(this, timeout);
	encoder = enc;
	invoke_id_manager_ = invoke_id_manager;
}

CDAPSession::~CDAPSession() throw ()
{
	delete connection_state_machine_;
	connection_state_machine_ = 0;
	for (std::map<int, CDAPOperationState*>::iterator iter =
			pending_messages_sent_.begin();
			iter != pending_messages_sent_.end(); ++iter) {
		delete iter->second;
	}
	pending_messages_sent_.clear();
	for (std::map<int, CDAPOperationState*>::iterator iter =
			pending_messages_recv_.begin();
			iter != pending_messages_recv_.end(); ++iter) {
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

void CDAPSession::encodeNextMessageToBeSent(const cdap_m_t &cdap_message,
					    ser_obj_t& result)
{
	CDAPMessageValidator::validate(cdap_message);

	switch (cdap_message.op_code_) {
		case cdap_m_t::M_CONNECT:
			connection_state_machine_->checkConnect();
			break;
		case cdap_m_t::M_CONNECT_R:
			connection_state_machine_->checkConnectResponse();
			break;
		case cdap_m_t::M_RELEASE:
			connection_state_machine_->checkRelease();
			break;
		case cdap_m_t::M_RELEASE_R:
			connection_state_machine_->checkReleaseResponse();
			break;
		case cdap_m_t::M_CREATE:
			check_can_send_or_receive_messages();
			checkInvokeIdNotExists(cdap_message.invoke_id_,
					       true);
			break;
		case cdap_m_t::M_CREATE_R:
			check_can_send_or_receive_messages();
			checkCanSendOrReceiveResponse(cdap_message.invoke_id_,
						      cdap_m_t::M_CREATE,
						      true);
			break;
		case cdap_m_t::M_DELETE:
			check_can_send_or_receive_messages();
			checkInvokeIdNotExists(cdap_message.invoke_id_,
					       true);
			break;
		case cdap_m_t::M_DELETE_R:
			check_can_send_or_receive_messages();
			checkCanSendOrReceiveResponse(cdap_message.invoke_id_,
						      cdap_m_t::M_DELETE,
						      true);
			break;
		case cdap_m_t::M_START:
			check_can_send_or_receive_messages();
			checkInvokeIdNotExists(cdap_message.invoke_id_,
					       true);
			break;
		case cdap_m_t::M_START_R:
			check_can_send_or_receive_messages();
			checkCanSendOrReceiveResponse(cdap_message.invoke_id_,
						      cdap_m_t::M_START,
						      true);
			break;
		case cdap_m_t::M_STOP:
			check_can_send_or_receive_messages();
			checkInvokeIdNotExists(cdap_message.invoke_id_,
					       true);
			break;
		case cdap_m_t::M_STOP_R:
			check_can_send_or_receive_messages();
			checkCanSendOrReceiveResponse(cdap_message.invoke_id_,
						      cdap_m_t::M_STOP, true);
			break;
		case cdap_m_t::M_WRITE:
			check_can_send_or_receive_messages();
			checkInvokeIdNotExists(cdap_message.invoke_id_,
					       true);
			break;
		case cdap_m_t::M_WRITE_R:
			check_can_send_or_receive_messages();
			checkCanSendOrReceiveResponse(cdap_message.invoke_id_,
						      cdap_m_t::M_WRITE,
						      true);
			break;
		case cdap_m_t::M_READ:
			check_can_send_or_receive_messages();
			checkInvokeIdNotExists(cdap_message.invoke_id_,
					       true);
			break;
		case cdap_m_t::M_READ_R:
			check_can_send_or_receive_messages();
			checkCanSendOrReceiveResponse(cdap_message.invoke_id_,
						      cdap_m_t::M_READ,
						      true);
			break;
		case cdap_m_t::M_CANCELREAD:
			check_can_send_or_receive_messages();
			checkCanSendOrReceiveCancelReadRequest(cdap_message.invoke_id_,
							       true);
			break;
		case cdap_m_t::M_CANCELREAD_R:
			check_can_send_or_receive_messages();
			checkCanSendOrReceiveCancelReadResponse(cdap_message.invoke_id_,
								true);
			break;
		default:
			std::stringstream ss;
			ss << "Unrecognized operation code: "
			   << cdap_message.op_code_;
			throw CDAPException(ss.str());
	}

	serializeMessage(cdap_message, result);
}

bool CDAPSession::is_in_await_con_state()
{
	return connection_state_machine_->is_await_conn();
}

const cdap_rib::con_handle_t& CDAPSession::get_con_handle()
{
	return con_handle;
}

void CDAPSession::set_port_id(int port_id)
{
	con_handle.port_id = port_id;
}

void CDAPSession::check_can_send_or_receive_messages() const
{
	if (!connection_state_machine_->can_send_or_receive_messages()) {
		throw CDAPException("The CDAP session is not in CONN or AWAITCON state");
	}
}

void CDAPSession::messageSent(const cdap_m_t &cdap_message)
{
	messageSentOrReceived(cdap_message, true);
}

void CDAPSession::messageReceived(const ser_obj_t &message,
				  cdap_m_t& result)
{
	deserializeMessage(message, result);
	messageSentOrReceived(result, false);
}

void CDAPSession::messageReceived(const cdap_m_t &cdap_message)
{
	messageSentOrReceived(cdap_message, false);
}

int CDAPSession::get_port_id() const
{
	return con_handle.port_id;
}

CDAPInvokeIdManagerImpl* CDAPSession::get_invoke_id_manager() const
{
	return invoke_id_manager_;
}
void CDAPSession::stopConnection()
{
	cdap_session_manager_->removeCDAPSession(get_port_id());
}
void CDAPSession::messageSentOrReceived(const cdap_m_t &cdap_message, bool sent)
{
	switch (cdap_message.op_code_) {
		case cdap_m_t::M_CONNECT:
			connection_state_machine_->connectSentOrReceived(sent);
			populate_con_handle(cdap_message, sent);
			break;
		case cdap_m_t::M_CONNECT_R:
			connection_state_machine_->connectResponseSentOrReceived(sent);
			break;
		case cdap_m_t::M_RELEASE:
			connection_state_machine_->releaseSentOrReceived(cdap_message,
									 sent);
			break;
		case cdap_m_t::M_RELEASE_R:
			connection_state_machine_->releaseResponseSentOrReceived(sent);
			break;
		case cdap_m_t::M_CREATE:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_CREATE,
						     sent);
			break;
		case cdap_m_t::M_CREATE_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_CREATE,
						      sent);
			break;
		case cdap_m_t::M_DELETE:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_DELETE,
						     sent);
			break;
		case cdap_m_t::M_DELETE_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_DELETE,
						      sent);
			break;
		case cdap_m_t::M_START:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_START,
						     sent);
			break;
		case cdap_m_t::M_START_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_START,
						      sent);
			break;
		case cdap_m_t::M_STOP:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_STOP,
						     sent);
			break;
		case cdap_m_t::M_STOP_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_STOP,
						      sent);
			break;
		case cdap_m_t::M_WRITE:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_WRITE, sent);
			break;
		case cdap_m_t::M_WRITE_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_WRITE,
						      sent);
			break;
		case cdap_m_t::M_READ:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_READ,
						     sent);
			break;
		case cdap_m_t::M_READ_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_READ,
						      sent);
			break;
		case cdap_m_t::M_CANCELREAD:
			cancelReadMessageSentOrReceived(cdap_message,
					sent);
			break;
		case cdap_m_t::M_CANCELREAD_R:
			cancelReadResponseMessageSentOrReceived(cdap_message,
								sent);
			break;
		default:
			std::stringstream ss;
			ss << "Unrecognized operation code: "
			   << cdap_message.op_code_;
			throw CDAPException(ss.str());
	}
	freeOrReserveInvokeId(cdap_message, sent);
}
void CDAPSession::freeOrReserveInvokeId(const cdap_m_t &cdap_message, bool sent)
{
	if (cdap_message.op_code_ == cdap_m_t::M_CONNECT_R || cdap_message.op_code_ == cdap_m_t::M_RELEASE_R
			|| cdap_message.op_code_ == cdap_m_t::M_CREATE_R
			|| cdap_message.op_code_ == cdap_m_t::M_DELETE_R
			|| cdap_message.op_code_ == cdap_m_t::M_START_R
			|| cdap_message.op_code_ == cdap_m_t::M_STOP_R
			|| cdap_message.op_code_ == cdap_m_t::M_WRITE_R
			|| cdap_message.op_code_ == cdap_m_t::M_CANCELREAD_R
			|| (cdap_message.op_code_ == cdap_m_t::M_READ_R
					&& cdap_message.flags_
							== cdap_rib::flags_t::NONE_FLAGS)
			|| cdap_message.flags_
					!= cdap_rib::flags_t::F_RD_INCOMPLETE) {
		invoke_id_manager_->freeInvokeId(cdap_message.invoke_id_, sent);
	}

	if (cdap_message.invoke_id_ != 0) {
		if (cdap_message.op_code_ == cdap_m_t::M_CONNECT
				|| cdap_message.op_code_ == cdap_m_t::M_RELEASE
				|| cdap_message.op_code_ == cdap_m_t::M_CREATE
				|| cdap_message.op_code_ == cdap_m_t::M_DELETE
				|| cdap_message.op_code_ == cdap_m_t::M_START
				|| cdap_message.op_code_ == cdap_m_t::M_STOP
				|| cdap_message.op_code_ == cdap_m_t::M_WRITE
				|| cdap_message.op_code_ == cdap_m_t::M_CANCELREAD
				|| cdap_message.op_code_ == cdap_m_t::M_READ) {
			invoke_id_manager_->reserveInvokeId(
					cdap_message.invoke_id_, sent);
		}
	}
}
void CDAPSession::checkIsConnected() const
{
	if (!connection_state_machine_->is_connected()) {
		throw CDAPException(
				"Cannot send a message because the CDAP session is not in CONNECTED state");
	}
}
void CDAPSession::checkInvokeIdNotExists(int invoke_id,
					 bool sent) const
{
	const std::map<int, CDAPOperationState*>* pending_messages;
	if (sent)
		pending_messages = &pending_messages_sent_;
	else
		pending_messages = &pending_messages_recv_;

	if (pending_messages->find(invoke_id)
			!= pending_messages->end()) {
		std::stringstream ss;
		ss << invoke_id;
		throw CDAPException(
				"The invokeid " + ss.str() + " already exists");
	}
}
void CDAPSession::checkCanSendOrReceiveCancelReadRequest(int invoke_id,
							 bool sent) const
{
	bool validationFailed = false;
	const std::map<int, CDAPOperationState*>* pending_messages;
	if (sent)
		pending_messages = &pending_messages_sent_;
	else
		pending_messages = &pending_messages_recv_;

	if (pending_messages->find(invoke_id)
			!= pending_messages->end()) {
		CDAPOperationState *state = pending_messages->find(invoke_id)->second;
		if (state->get_op_code() == cdap_m_t::M_READ) {
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
			ss << invoke_id;
			throw CDAPException(
					"Cannot set an M_CANCELREAD message because there is no READ transaction associated to the invoke id "
							+ ss.str());
		}
	} else {
		std::stringstream ss;
		ss << invoke_id;
		throw CDAPException(
				"Cannot set an M_CANCELREAD message because there is no READ transaction associated to the invoke id "
						+ ss.str());
	}
}
void CDAPSession::requestMessageSentOrReceived(const cdap_m_t &cdap_message,
					       cdap_m_t::Opcode op_code,
					       bool sent)
{
	check_can_send_or_receive_messages();
	checkInvokeIdNotExists(cdap_message.invoke_id_,
			       sent);

	std::map<int, CDAPOperationState*>* pending_messages;
	if (sent)
		pending_messages = &pending_messages_sent_;
	else
		pending_messages = &pending_messages_recv_;

	if (cdap_message.invoke_id_ != 0) {
		CDAPOperationState *new_operation_state =
				new CDAPOperationState(op_code, sent);
		pending_messages->insert(
				std::pair<int, CDAPOperationState*>(
						cdap_message.invoke_id_,
						new_operation_state));
	}
}
void CDAPSession::cancelReadMessageSentOrReceived(const cdap_m_t &cdap_message,
						  bool sender)
{
	checkCanSendOrReceiveCancelReadRequest(cdap_message.invoke_id_,
					       sender);
	CDAPOperationState *new_operation_state = new CDAPOperationState(cdap_m_t::M_CANCELREAD,
									 sender);
	cancel_read_pending_messages_.insert(
			std::pair<int, CDAPOperationState*>(cdap_message.invoke_id_,
							    new_operation_state));
}
void CDAPSession::checkCanSendOrReceiveResponse(int invoke_id,
						cdap_m_t::Opcode op_code,
						bool sender) const
{
	bool validation_failed = false;
	const std::map<int, CDAPOperationState*>* pending_messages;
	if (!sender)
		pending_messages = &pending_messages_sent_;
	else
		pending_messages = &pending_messages_recv_;

	std::map<int, CDAPOperationState*>::const_iterator iterator;
	iterator = pending_messages->find(invoke_id);
	if (iterator == pending_messages->end()) {
		std::stringstream ss;
		ss << "Cannot send a response for the " << op_code
		   << " operation with invokeId " << invoke_id
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
		   << " operation with invokeId " << invoke_id;
		throw CDAPException(ss.str());
	}
}
void CDAPSession::checkCanSendOrReceiveCancelReadResponse(int invoke_id,
							  bool send) const
{
	bool validation_failed = false;

	if (cancel_read_pending_messages_.find(invoke_id)
			== cancel_read_pending_messages_.end()) {
		std::stringstream ss;
		ss << "Cannot send a response for the "
		   << cdap_m_t::M_CANCELREAD << " operation with invokeId "
		   << invoke_id;
		throw CDAPException(ss.str());
	}
	CDAPOperationState *state = cancel_read_pending_messages_.find(invoke_id)->second;
	if (state->get_op_code() != cdap_m_t::M_CANCELREAD) {
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
		ss << "Cannot send a response for the "
		   << cdap_m_t::M_CANCELREAD << " operation with invokeId "
		   << invoke_id;
		throw CDAPException(ss.str());
	}
}
void CDAPSession::responseMessageSentOrReceived(const cdap_m_t &cdap_message,
						cdap_m_t::Opcode op_code,
						bool sent)
{
	check_can_send_or_receive_messages();
	checkCanSendOrReceiveResponse(cdap_message.invoke_id_,
				      op_code,
				      sent);
	bool operation_complete = true;
	std::map<int, CDAPOperationState*>* pending_messages;
	if (!sent)
		pending_messages = &pending_messages_sent_;
	else
		pending_messages = &pending_messages_recv_;

	if (op_code == cdap_m_t::M_READ_R) {
		if (cdap_message.flags_ == cdap_rib::flags_t::F_RD_INCOMPLETE) {
			operation_complete = false;
		}
	}

	if (operation_complete) {
		std::map<int, CDAPOperationState*>::iterator it =
				pending_messages->find(cdap_message.invoke_id_);
		delete it->second;
		pending_messages->erase(it);
	}
	// check for M_READ_R and M_CANCELREAD race condition
	if (!sent) {
		if (op_code != cdap_m_t::M_READ) {
			cancel_read_pending_messages_.erase(
					cdap_message.invoke_id_);
		}
	}
}
void CDAPSession::cancelReadResponseMessageSentOrReceived(const cdap_m_t &cdap_message,
							  bool sent)
{
	checkIsConnected();
	checkCanSendOrReceiveCancelReadResponse(cdap_message.invoke_id_,
						sent);
	cancel_read_pending_messages_.erase(cdap_message.invoke_id_);
	if (sent)
		pending_messages_sent_.erase(cdap_message.invoke_id_);
	else
		pending_messages_recv_.erase(cdap_message.invoke_id_);
}

void CDAPSession::serializeMessage(const cdap_m_t &cdap_message,
				   ser_obj_t& result) const
{
	encoder->encode(cdap_message, result);
}

void CDAPSession::deserializeMessage(const ser_obj_t &message,
				     cdap_m_t& result) const
{
	encoder->decode(message, result);
}

void CDAPSession::populate_con_handle(const cdap_m_t &cdap_message,
				      bool send)
{
	con_handle.abs_syntax = cdap_message.abs_syntax_;
	con_handle.auth_.name = cdap_message.auth_policy_.name;
	con_handle.auth_.versions = cdap_message.auth_policy_.versions;
	if (cdap_message.auth_policy_.options.size_ > 0)
		con_handle.auth_.options = cdap_message.auth_policy_.options;

	if (send) {
		con_handle.dest_.ae_inst_ = cdap_message.dest_ae_inst_;
		con_handle.dest_.ae_name_ = cdap_message.dest_ae_name_;
		con_handle.dest_.ap_inst_ = cdap_message.dest_ap_inst_;
		con_handle.dest_.ap_name_ = cdap_message.dest_ap_name_;
		con_handle.src_.ae_inst_ = cdap_message.src_ae_inst_;
		con_handle.src_.ae_name_ = cdap_message.src_ae_name_;
		con_handle.src_.ap_inst_ = cdap_message.src_ap_inst_;
		con_handle.src_.ap_name_ = cdap_message.src_ap_name_;
	} else {
		con_handle.dest_.ae_inst_ = cdap_message.src_ae_inst_;
		con_handle.dest_.ae_name_ = cdap_message.src_ae_name_;
		con_handle.dest_.ap_inst_ = cdap_message.src_ap_inst_;
		con_handle.dest_.ap_name_ = cdap_message.src_ap_name_;
		con_handle.src_.ae_inst_ = cdap_message.dest_ae_inst_;
		con_handle.src_.ae_name_ = cdap_message.dest_ae_name_;
		con_handle.src_.ap_inst_ = cdap_message.dest_ap_inst_;
		con_handle.src_.ap_name_ = cdap_message.dest_ap_name_;
	}

	con_handle.version_.version_ = cdap_message.version_;
}

// CLASS CDAPSessionManager
CDAPSessionManager::CDAPSessionManager()
{
	throw CDAPException(
			"Not allowed default constructor of CDAPSessionManager has been called.");
}

CDAPSessionManager::CDAPSessionManager(cdap_rib::concrete_syntax_t& syntax)
{
	timeout_ = DEFAULT_TIMEOUT_IN_MS;
	encoder = new CDAPMessageEncoder(syntax);
	invoke_id_manager_ = new CDAPInvokeIdManagerImpl();
}

CDAPSessionManager::CDAPSessionManager(cdap_rib::concrete_syntax_t& syntax,
				       long timeout)
{
	timeout_ = timeout;
	encoder = new CDAPMessageEncoder(syntax);
	invoke_id_manager_ = new CDAPInvokeIdManagerImpl();
}

CDAPSession* CDAPSessionManager::createCDAPSession(int port_id)
{
	if (cdap_sessions_.find(port_id) != cdap_sessions_.end()) {
		return cdap_sessions_.find(port_id)->second;
	} else {
		CDAPSession *cdap_session = new CDAPSession(this, timeout_,
							    encoder,
							    invoke_id_manager_);
		cdap_session->set_port_id(port_id);
		cdap_sessions_.insert(
				std::pair<int, CDAPSession*>(port_id,
							     cdap_session));
		return cdap_session;
	}
}
CDAPSessionManager::~CDAPSessionManager() throw ()
{
	delete invoke_id_manager_;
	for (std::map<int, CDAPSession*>::iterator iter =
			cdap_sessions_.begin(); iter != cdap_sessions_.end();
			++iter) {
		delete iter->second;
		iter->second = 0;
	}
	cdap_sessions_.clear();
	delete encoder;
}
void CDAPSessionManager::set_timeout(long timeout)
{
	timeout_ = timeout;
}
void CDAPSessionManager::getAllCDAPSessionIds(std::vector<int> &vector)
{
	vector.clear();
	for (std::map<int, CDAPSession*>::iterator it = cdap_sessions_.begin();
			it != cdap_sessions_.end(); ++it) {
		vector.push_back(it->first);
	}
}
CDAPSession* CDAPSessionManager::get_cdap_session(int port_id)
{
	std::map<int, CDAPSession*>::iterator itr = cdap_sessions_.find(
			port_id);
	if (itr != cdap_sessions_.end())
		return cdap_sessions_.find(port_id)->second;
	else
		return 0;
}

void CDAPSessionManager::encodeCDAPMessage(const cdap_m_t& cdap_message,
					   ser_obj_t& result)
{
	encoder->encode(cdap_message, result);
}

void CDAPSessionManager::decodeCDAPMessage(const ser_obj_t &cdap_message,
					   cdap_m_t& result)
{
	encoder->decode(cdap_message, result);
}

void CDAPSessionManager::removeCDAPSession(int port_id)
{
	std::map<int, CDAPSession*>::iterator itr = cdap_sessions_.find(
		port_id);
	if (itr != cdap_sessions_.end()){
		delete itr->second;
		itr->second = 0;
		cdap_sessions_.erase(itr);
	}
}

bool CDAPSessionManager::session_in_await_con_state(int portId)
{
	CDAPSession * cdap_session = get_cdap_session(portId);
	if (!cdap_session)
		return false;

	return cdap_session->is_in_await_con_state();
}

void CDAPSessionManager::encodeNextMessageToBeSent(const CDAPMessage &cdap_message,
					    	   ser_obj_t& result,
					    	   int port_id)
{
	std::map<int, CDAPSession*>::iterator it = cdap_sessions_.find(port_id);
	CDAPSession *cdap_session;

	if (it == cdap_sessions_.end()) {
		if (cdap_message.op_code_ == CDAPMessage::M_CONNECT) {
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

	cdap_session->encodeNextMessageToBeSent(cdap_message, result);
}

void CDAPSessionManager::messageReceived(const ser_obj_t &encoded_cdap_message,
					 cdap_m_t& result,
					 int port_id)
{
	decodeCDAPMessage(encoded_cdap_message, result);
	CDAPSession *cdap_session = get_cdap_session(port_id);
	switch (result.op_code_) {
		case CDAPMessage::M_CONNECT:
			if (cdap_session == 0) {
				cdap_session = createCDAPSession(port_id);
				cdap_session->messageReceived(result);
				LOG_DBG("Created a new CDAP session for port %d",
					port_id);
			} else {
				std::stringstream ss;
				ss << "M_CONNECT received on an already open CDAP Session, over flow "
				   << port_id;
				throw CDAPException(ss.str());
			}
			break;
		default:
			if (cdap_session != 0) {
				cdap_session->messageReceived(result);
			} else {
				std::stringstream ss;
				ss << "Receive a "
				   << result.op_code_
				   << " CDAP message on a CDAP session that is not open, over flow "
				   << port_id;
				throw CDAPException(ss.str());
			}
			break;
	}
}
void CDAPSessionManager::messageSent(const CDAPMessage &cdap_message,
				     int port_id)
{
	CDAPSession *cdap_session = get_cdap_session(port_id);
	if (cdap_session == 0
			&& cdap_message.op_code_ == CDAPMessage::M_CONNECT) {
		cdap_session = createCDAPSession(port_id);
	} else if (cdap_session == 0
			&& cdap_message.op_code_ != CDAPMessage::M_CONNECT) {
		std::stringstream ss;
		ss << "There are no open CDAP sessions associated to the flow identified by "
		   << port_id << " right now";
		throw CDAPException(ss.str());
	}
	cdap_session->messageSent(cdap_message);
}
int CDAPSessionManager::get_port_id(
		std::string destination_application_process_name)
{
	std::map<int, CDAPSession*>::iterator itr = cdap_sessions_.begin();
	CDAPSession *current_session;
	while (itr != cdap_sessions_.end()) {
		current_session = itr->second;
		if (current_session->get_con_handle().dest_.ap_name_
				== (destination_application_process_name)) {
			return current_session->get_port_id();
		}
	}
	std::stringstream ss;
	ss << "Don't have a running CDAP sesion to "
	   << destination_application_process_name;
	throw CDAPException(ss.str());
}

void CDAPSessionManager::getOpenConnectionRequestMessage(cdap_m_t & msg,
							 const cdap_rib::con_handle_t &con)
{
	CDAPSession *cdap_session = get_cdap_session(con.port_id);
	if (cdap_session == 0) {
		cdap_session = createCDAPSession(con.port_id);
	}

	CDAPMessageFactory::getOpenConnectionRequestMessage(msg,
							    con,
							    invoke_id_manager_->newInvokeId(true));
}

void CDAPSessionManager::getOpenConnectionResponseMessage(cdap_m_t & msg,
							  const cdap_rib::con_handle_t &con,
							  const cdap_rib::res_info_t &res,
							  int invoke_id)
{
	CDAPMessageFactory::getOpenConnectionResponseMessage(msg,
							     con,
							     res,
							     invoke_id);
}

void CDAPSessionManager::getReleaseConnectionRequestMessage(cdap_m_t & msg,
							    const cdap_rib::flags_t &flags,
							    bool invoke_id)
{
	CDAPMessageFactory::getReleaseConnectionRequestMessage(msg, flags);
	if (invoke_id)
		msg.invoke_id_ = invoke_id_manager_->newInvokeId(true);
}

void CDAPSessionManager::getReleaseConnectionResponseMessage(cdap_m_t & msg,
							     const cdap_rib::flags_t &flags,
							     const cdap_rib::res_info_t &res,
							     int invoke_id)
{
	CDAPMessageFactory::getReleaseConnectionResponseMessage(msg,
								flags,
								res,
								invoke_id);
}

void CDAPSessionManager::getCreateObjectRequestMessage(cdap_m_t & msg,
						       const cdap_rib::filt_info_t &filt,
						       const cdap_rib::flags_t &flags,
						       const cdap_rib::obj_info_t &obj,
						       bool invoke_id)
{

	CDAPMessageFactory::getCreateObjectRequestMessage(msg,
							  filt,
							  flags,
							  obj);
	if (invoke_id)
		msg.invoke_id_ = invoke_id_manager_->newInvokeId(true);

}

void CDAPSessionManager::getCreateObjectResponseMessage(cdap_m_t & msg,
							const cdap_rib::flags_t &flags,
							const cdap_rib::obj_info_t &obj,
							const cdap_rib::res_info_t &res,
							int invoke_id)
{
	CDAPMessageFactory::getCreateObjectResponseMessage(msg,
							   flags,
							   obj,
							   res,
							   invoke_id);
}

void CDAPSessionManager::getDeleteObjectRequestMessage(cdap_m_t & msg,
						       const cdap_rib::filt_info_t &filt,
						       const cdap_rib::flags_t &flags,
						       const cdap_rib::obj_info_t &obj,
						       bool invoke_id)
{
	CDAPMessageFactory::getDeleteObjectRequestMessage(msg,
							  filt,
							  flags,
							  obj);
	if (invoke_id)
		msg.invoke_id_ = invoke_id_manager_->newInvokeId(true);
}

void CDAPSessionManager::getDeleteObjectResponseMessage(cdap_m_t & msg,
							const cdap_rib::flags_t &flags,
							const cdap_rib::obj_info_t &obj,
							const cdap_rib::res_info_t &res,
							int invoke_id)
{
	CDAPMessageFactory::getDeleteObjectResponseMessage(msg,
							   flags,
							   obj,
							   res,
							   invoke_id);
}

void CDAPSessionManager::getStartObjectRequestMessage(cdap_m_t & msg,
						      const cdap_rib::filt_info_t &filt,
						      const cdap_rib::flags_t &flags,
						      const cdap_rib::obj_info_t &obj,
						      bool invoke_id)
{
	CDAPMessageFactory::getStartObjectRequestMessage(msg,
							 filt,
							 flags,
							 obj);
	if (invoke_id)
		msg.invoke_id_ = invoke_id_manager_->newInvokeId(true);
}

void CDAPSessionManager::getStartObjectResponseMessage(cdap_m_t & msg,
						       const cdap_rib::flags_t &flags,
						       const cdap_rib::res_info_t &res,
						       int invoke_id)
{
	CDAPMessageFactory::getStartObjectResponseMessage(msg,
							  flags,
							  res,
							  invoke_id);
}

void CDAPSessionManager::getStartObjectResponseMessage(cdap_m_t & msg,
						       const cdap_rib::flags_t &flags,
						       const cdap_rib::obj_info_t &obj,
						       const cdap_rib::res_info_t &res,
						       int invoke_id)
{
	CDAPMessageFactory::getStartObjectResponseMessage(msg,
							  flags,
							  obj,
							  res,
							  invoke_id);
}

void CDAPSessionManager::getStopObjectRequestMessage(cdap_m_t & msg,
						     const cdap_rib::filt_info_t &filt,
						     const cdap_rib::flags_t &flags,
						     const cdap_rib::obj_info_t &obj,
						     bool invoke_id)
{
	CDAPMessageFactory::getStopObjectRequestMessage(msg,
							filt,
							flags,
							obj);
	if (invoke_id)
		msg.invoke_id_ = invoke_id_manager_->newInvokeId(true);
}

void CDAPSessionManager::getStopObjectResponseMessage(cdap_m_t & msg,
						      const cdap_rib::flags_t &flags,
						      const cdap_rib::res_info_t &res,
						      int invoke_id)
{
	CDAPMessageFactory::getStopObjectResponseMessage(msg,
							 flags,
							 res,
							 invoke_id);
}

void CDAPSessionManager::getReadObjectRequestMessage(cdap_m_t & msg,
						     const cdap_rib::filt_info_t &filt,
						     const cdap_rib::flags_t &flags,
						     const cdap_rib::obj_info_t &obj,
						     bool invoke_id)
{
	CDAPMessageFactory::getReadObjectRequestMessage(msg,
							filt,
							flags,
							obj);
	if (invoke_id)
		msg.invoke_id_ = invoke_id_manager_->newInvokeId(true);
}

void CDAPSessionManager::getReadObjectResponseMessage(cdap_m_t & msg,
						      const cdap_rib::flags_t &flags,
						      const cdap_rib::obj_info_t &obj,
						      const cdap_rib::res_info_t &res,
						      int invoke_id)
{
	CDAPMessageFactory::getReadObjectResponseMessage(msg,
							 flags,
							 obj,
							 res,
							 invoke_id);
}

void CDAPSessionManager::getWriteObjectRequestMessage(cdap_m_t & msg,
						      const cdap_rib::filt_info_t &filt,
						      const cdap_rib::flags_t &flags,
						      const cdap_rib::obj_info_t &obj,
						      bool invoke_id)
{
	CDAPMessageFactory::getWriteObjectRequestMessage(msg,
							 filt,
							 flags,
							 obj);
	if (invoke_id)
		msg.invoke_id_ = invoke_id_manager_->newInvokeId(true);
}

void CDAPSessionManager::getWriteObjectResponseMessage(cdap_m_t & msg,
						       const cdap_rib::flags_t &flags,
						       const cdap_rib::res_info_t &res,
						       int invoke_id)
{
	CDAPMessageFactory::getWriteObjectResponseMessage(msg,
							  flags,
							  res,
							  invoke_id);
}

void CDAPSessionManager::getCancelReadRequestMessage(cdap_m_t & msg,
						     const cdap_rib::flags_t &flags,
						     int invoke_id)
{
	CDAPMessageFactory::getCancelReadRequestMessage(msg,
							flags,
							invoke_id);
}

void CDAPSessionManager::getCancelReadResponseMessage(cdap_m_t & msg,
						      const cdap_rib::flags_t &flags,
						      const cdap_rib::res_info_t &res,
						      int invoke_id)
{
	CDAPMessageFactory::getCancelReadResponseMessage(msg,
							 flags,
							 res,
							 invoke_id);
}

CDAPInvokeIdManager * CDAPSessionManager::get_invoke_id_manager()
{
	return invoke_id_manager_;
}

const cdap_rib::con_handle_t& CDAPSessionManager::get_con_handle(int port_id)
{
	CDAPSession *cdap_session = get_cdap_session(port_id);
	if (!cdap_session) {
		throw Exception("Could not find CDAP session associated to port-id");
	}

	return cdap_session->get_con_handle();
}

// CLASS GPBWireMessageProvider
void GPBSerializer::deserializeMessage(const ser_obj_t &message,
				       cdap_m_t& result)
{
	messages::CDAPMessage gpfCDAPMessage;

	gpfCDAPMessage.ParseFromArray(message.message_, message.size_);
	// ABS_SYNTAX
	if (gpfCDAPMessage.has_abssyntax())
		result.abs_syntax_ = gpfCDAPMessage.abssyntax();
	// AUTH_POLICY
	result.auth_policy_.name = gpfCDAPMessage.authpolicy().name();
	for(int i=0; i<gpfCDAPMessage.authpolicy().versions_size(); i++) {
		result.auth_policy_.versions.push_back(
				gpfCDAPMessage.authpolicy().versions(i));
	}
	if (gpfCDAPMessage.authpolicy().has_options()) {
		result.auth_policy_.options.message_ = new unsigned char[gpfCDAPMessage.authpolicy().options().size()];
		result.auth_policy_.options.size_ = gpfCDAPMessage.authpolicy().options().size();
		memcpy(result.auth_policy_.options.message_,
		       gpfCDAPMessage.authpolicy().options().data(),
		       gpfCDAPMessage.authpolicy().options().size());
	}
	// DEST_AE_INST
	if (gpfCDAPMessage.has_destaeinst())
		result.dest_ae_inst_ = gpfCDAPMessage.destaeinst();
	// DEST_AE_NAME
	if (gpfCDAPMessage.has_destaename())
		result.dest_ae_name_ = gpfCDAPMessage.destaename();
	// DEST_AP_INST
	if (gpfCDAPMessage.has_destapinst())
		result.dest_ap_inst_ = gpfCDAPMessage.destapinst();
	// DEST_AP_NAME
	if (gpfCDAPMessage.has_destapname())
		result.dest_ap_name_ = gpfCDAPMessage.destapname();
	// FILTER
	if (gpfCDAPMessage.has_filter()) {
		char *filter = new char[gpfCDAPMessage.filter().size() + 1];
		strcpy(filter, gpfCDAPMessage.filter().c_str());
		result.filter_ = filter;
	}
	// FLAGS
	if (gpfCDAPMessage.has_flags()) {
		int flag_value = gpfCDAPMessage.flags();
		cdap_rib::flags_t::Flags flags =
				static_cast<cdap_rib::flags_t::Flags>(flag_value);
		result.flags_ = flags;
	}
	// INVOKE_ID
	if (gpfCDAPMessage.has_invokeid())
		result.invoke_id_ = gpfCDAPMessage.invokeid();
	// OBJECT_CLASS
	if (gpfCDAPMessage.has_objclass())
		result.obj_class_ = gpfCDAPMessage.objclass();
	// OBJ_INSTANCE
	if (gpfCDAPMessage.has_objinst())
		result.obj_inst_ = gpfCDAPMessage.objinst();
	// OBJ_NAME
	if (gpfCDAPMessage.has_objname())
		result.obj_name_ = gpfCDAPMessage.objname();
	// OBJ_VALUE
	if (gpfCDAPMessage.has_objvalue()) {
		messages::objVal_t obj_val_t = gpfCDAPMessage.objvalue();
		char unsigned *byte_val = new
				unsigned char[obj_val_t.byteval().size()];
		memcpy(byte_val, obj_val_t.byteval().data(),
		       obj_val_t.byteval().size());
		result.obj_value_.message_ = byte_val;
		result.obj_value_.size_ = obj_val_t.byteval().size();
	}
	// OP_CODE
	if (gpfCDAPMessage.has_opcode()) {
		int opcode_val = gpfCDAPMessage.opcode();
		CDAPMessage::Opcode opcode =
				static_cast<CDAPMessage::Opcode>(opcode_val);
		result.op_code_ = opcode;
	}
	// RESULT
	if (gpfCDAPMessage.has_result())
		result.result_ = gpfCDAPMessage.result();
	// RESULT_REASON
	if (gpfCDAPMessage.has_resultreason())
		result.result_reason_ = gpfCDAPMessage.resultreason();
	// SCOPE
	if (gpfCDAPMessage.has_scope())
		result.scope_ = gpfCDAPMessage.scope();
	// SRC_AE_INST
	if (gpfCDAPMessage.has_srcaeinst())
		result.src_ae_inst_ = gpfCDAPMessage.srcaeinst();
	// SRC_AE_NAME
	if (gpfCDAPMessage.has_srcaename())
		result.src_ae_name_ = gpfCDAPMessage.srcaename();
	// SRC_AP_INST
	if (gpfCDAPMessage.has_srcapinst())
		result.src_ap_inst_ = gpfCDAPMessage.srcapinst();
	// SRC_AP_NAME
	if (gpfCDAPMessage.has_srcapname())
		result.src_ap_name_ = gpfCDAPMessage.srcapname();
	// VERISON
	if (gpfCDAPMessage.has_version())
		result.version_ = gpfCDAPMessage.version();
}
// FIXME: check existanc of fields before seting
void GPBSerializer::serializeMessage(const cdap_m_t &cdapMessage,
				     ser_obj_t& result)
{
	messages::CDAPMessage gpfCDAPMessage;
	// ABS_SYNTAX
	gpfCDAPMessage.set_abssyntax(cdapMessage.abs_syntax_);
	// AUTH_POLICY
	messages::authPolicy_t *gpb_auth_policy = new messages::authPolicy_t();
	gpb_auth_policy->set_name(cdapMessage.auth_policy_.name);
	std::list<std::string> versions = cdapMessage.auth_policy_.versions;
	for(std::list<std::string>::iterator it = versions.begin();
		it != versions.end(); ++it) {
		gpb_auth_policy->add_versions(*it);
	}
	if (cdapMessage.auth_policy_.options.size_ > 0) {
		char * gpb_opts = new char[cdapMessage.auth_policy_.options.size_];
		memcpy(gpb_opts,
		       cdapMessage.auth_policy_.options.message_,
		       cdapMessage.auth_policy_.options.size_);
		gpb_auth_policy->set_options(gpb_opts,
					     cdapMessage.auth_policy_.options.size_);
	}
	gpfCDAPMessage.set_allocated_authpolicy(gpb_auth_policy);
	// DEST_AE_INST
	gpfCDAPMessage.set_destaeinst(cdapMessage.dest_ae_inst_);
	// DEST_AE_NAME
	gpfCDAPMessage.set_destaename(cdapMessage.dest_ae_name_);
	// DEST_AP_INST
	gpfCDAPMessage.set_destapinst(cdapMessage.dest_ap_inst_);
	// DEST_AP_NAME
	gpfCDAPMessage.set_destapname(cdapMessage.dest_ap_name_);
	// FILTER
	if (cdapMessage.filter_ != 0) {
		gpfCDAPMessage.set_filter(cdapMessage.filter_);
	}
	// INVOKE_ID
	gpfCDAPMessage.set_invokeid(cdapMessage.invoke_id_);
	// OBJ_CLASS
	gpfCDAPMessage.set_objclass(cdapMessage.obj_class_);
	// OBJ_INST
	gpfCDAPMessage.set_objinst(cdapMessage.obj_inst_);
	// OBJ_NAME
	gpfCDAPMessage.set_objname(cdapMessage.obj_name_);
	// OBJ_VALUE
	if (cdapMessage.obj_value_.size_ > 0) {
		messages::objVal_t *gpb_obj_val = new messages::objVal_t();
		char * gpb_val = new char[cdapMessage.obj_value_.size_];
		memcpy(gpb_val,
		       cdapMessage.obj_value_.message_,
		       cdapMessage.obj_value_.size_);
		gpb_obj_val->set_byteval(gpb_val,
					 cdapMessage.obj_value_.size_);
		gpfCDAPMessage.set_allocated_objvalue(gpb_obj_val);
	}
	// OP_CODE
	if (!messages::opCode_t_IsValid(cdapMessage.op_code_)) {
		throw CDAPException("Serializing Message: Not a valid OpCode");
	}
	gpfCDAPMessage.set_opcode((messages::opCode_t) cdapMessage.op_code_);
	// RESULT
	gpfCDAPMessage.set_result(cdapMessage.result_);
	// RESULT_REASON
	gpfCDAPMessage.set_resultreason(cdapMessage.result_reason_);
	// SCOPE
	gpfCDAPMessage.set_scope(cdapMessage.scope_);
	// SRC_AE_INST
	gpfCDAPMessage.set_srcaeinst(cdapMessage.src_ae_inst_);
	// SRC_AE_NAME
	gpfCDAPMessage.set_srcaename(cdapMessage.src_ae_name_);
	// SRC_AP_NAME
	gpfCDAPMessage.set_srcapname(cdapMessage.src_ap_name_);
	// SRC_AP_INST
	gpfCDAPMessage.set_srcapinst(cdapMessage.src_ap_inst_);
	// VERSION
	gpfCDAPMessage.set_version(cdapMessage.version_);

	int size = gpfCDAPMessage.ByteSize();
	result.message_ = new unsigned char[size];
	result.size_ = size;
	gpfCDAPMessage.SerializeToArray(result.message_, size);
}

class CDAPProvider : public CDAPProviderInterface
{
 public:
	CDAPProvider(cdap::CDAPCallbackInterface *callback,
		     CDAPSessionManager *manager);
	~CDAPProvider();

	//Remote

	cdap_rib::con_handle_t remote_open_connection(const cdap_rib::vers_info_t &ver,
						      const cdap_rib::ep_info_t &src,
						      const cdap_rib::ep_info_t &dest,
						      const cdap_rib::auth_policy_t &auth,
						      int port);
	int remote_close_connection(unsigned int port);
	int remote_create(const cdap_rib::con_handle_t &con,
			  const cdap_rib::obj_info_t &obj,
			  const cdap_rib::flags_t &flags,
			  const cdap_rib::filt_info_t &filt,
			  const int invoke_id = -1);
	int remote_delete(const cdap_rib::con_handle_t &con,
			  const cdap_rib::obj_info_t &obj,
			  const cdap_rib::flags_t &flags,
			  const cdap_rib::filt_info_t &filt,
			  const int invoke_id = -1);
	int remote_read(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::flags_t &flags,
			const cdap_rib::filt_info_t &filt,
			const int invoke_id = -1);
	int remote_cancel_read(const cdap_rib::con_handle_t &con,
			       const cdap_rib::flags_t &flags,
			       const int invoke_id = -1);
	int remote_write(const cdap_rib::con_handle_t &con,
			 const cdap_rib::obj_info_t &obj,
			 const cdap_rib::flags_t &flags,
			 const cdap_rib::filt_info_t &filt,
			 const int invoke_id = -1);
	int remote_start(const cdap_rib::con_handle_t &con,
			 const cdap_rib::obj_info_t &obj,
			 const cdap_rib::flags_t &flags,
			 const cdap_rib::filt_info_t &filt,
			 const int invoke_id = -1);
	int remote_stop(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::flags_t &flags,
			const cdap_rib::filt_info_t &filt,
			const int invoke_id = -1);

	//Local

	void send_open_connection_result(const cdap_rib::con_handle_t &con,
				         const cdap_rib::res_info_t &res,
				         int invoke_id);
	void send_close_connection_result(unsigned int port,
				       	  const cdap_rib::flags_t &flags,
				       	  const cdap_rib::res_info_t &res,
				       	  int invoke_id);
	void send_create_result(const cdap_rib::con_handle_t &con,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::res_info_t &res,
				int invoke_id);
	void send_delete_result(const cdap_rib::con_handle_t &con,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::res_info_t &res,
				int invoke_id);
	void send_read_result(const cdap_rib::con_handle_t &con,
			      const cdap_rib::obj_info_t &obj,
			      const cdap_rib::flags_t &flags,
			      const cdap_rib::res_info_t &res,
			      int invoke_id);
	void send_cancel_read_result(const cdap_rib::con_handle_t &con,
				     const cdap_rib::flags_t &flags,
				     const cdap_rib::res_info_t &res,
				     int invoke_id);
	void send_write_result(const cdap_rib::con_handle_t &con,
			       const cdap_rib::flags_t &flags,
			       const cdap_rib::res_info_t &res,
			       int invoke_id);
	void send_start_result(const cdap_rib::con_handle_t &con,
			       const cdap_rib::obj_info_t &obj,
			       const cdap_rib::flags_t &flags,
			       const cdap_rib::res_info_t &res,
			       int invoke_id);
	void send_stop_result(const cdap_rib::con_handle_t &con,
			      const cdap_rib::flags_t &flags,
			      const cdap_rib::res_info_t &res,
			      int invoke_id);

	// Process and incoming CDAP message
	void process_message(const ser_obj_t &message,
			     unsigned int port,
			     cdap_rib::cdap_dest_t cdap_dest = cdap_rib::CDAP_DEST_PORT);

	void set_cdap_io_handler(CDAPIOHandler * handler);

	CDAPSessionManagerInterface * get_session_manager();

 protected:
	CDAPSessionManager *manager_;
	CDAPCallbackInterface *callback_;
	CDAPIOHandler *io_handler_;

 private:
	void send(const cdap_m_t & m_sent,
		  const cdap_rib::con_handle_t &con);
};

// CLASS CDAPProvider
CDAPProvider::CDAPProvider(cdap::CDAPCallbackInterface *callback,
			   CDAPSessionManager *manager)
{
	callback_ = callback;
	manager_ = manager;
	io_handler_ = 0;
}

CDAPProvider::~CDAPProvider()
{
}

cdap_rib::con_handle_t
	CDAPProvider::remote_open_connection(const cdap_rib::vers_info_t &ver,
					     const cdap_rib::ep_info_t &src,
					     const cdap_rib::ep_info_t &dest,
					     const cdap_rib::auth_policy_t &auth,
					     int port)
{
	cdap_m_t m_sent;
	cdap_rib::con_handle_t con;

	con.port_id = port;
	con.version_ = ver;
	con.src_ = src;
	con.dest_ = dest;
	con.auth_ = auth;

	manager_->getOpenConnectionRequestMessage(m_sent, con);
	send(m_sent, con);

	return con;
}

int CDAPProvider::remote_close_connection(unsigned int port)
{
	int invoke_id;
	cdap_m_t m_sent;
	cdap_rib::con_handle_t con;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	cdap_rib::flags_t flags;
	flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;
	manager_->getReleaseConnectionRequestMessage(m_sent,
						     flags,
						     true);
	invoke_id = m_sent.invoke_id_;
	con.port_id = port;
	con.cdap_dest = cdap_rib::CDAP_DEST_PORT;
	send(m_sent, con);

	return invoke_id;
}

int CDAPProvider::remote_create(const cdap_rib::con_handle_t &con,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id)
{
	cdap_m_t m_sent;
	int inv_id;

	if (invoke_id < 0) {
		manager_->getCreateObjectRequestMessage(m_sent,
						        filt,
						        flags,
						        obj,
						        true);
		inv_id = m_sent.invoke_id_;
	} else {
		manager_->getCreateObjectRequestMessage(m_sent,
						        filt,
						        flags,
						        obj,
						        false);
		m_sent.invoke_id_ = invoke_id;
		inv_id = invoke_id;
	}

	send(m_sent, con);
	return inv_id;
}

int CDAPProvider::remote_delete(const cdap_rib::con_handle_t &con,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id)
{
	cdap_m_t m_sent;
	int inv_id;

	if (invoke_id < 0) {
		manager_->getDeleteObjectRequestMessage(m_sent,
							filt,
							flags,
							obj,
							true);
		inv_id = m_sent.invoke_id_;
	} else {
		manager_->getDeleteObjectRequestMessage(m_sent,
							filt,
							flags,
							obj,
							false);
		m_sent.invoke_id_ = invoke_id;
		inv_id = invoke_id;
	}

	send(m_sent, con);
	return inv_id;
}

int CDAPProvider::remote_read(const cdap_rib::con_handle_t &con,
			      const cdap_rib::obj_info_t &obj,
			      const cdap_rib::flags_t &flags,
			      const cdap_rib::filt_info_t &filt,
			      const int invoke_id)
{
	cdap_m_t m_sent;
	int inv_id;

	if (invoke_id < 0) {
		manager_->getReadObjectRequestMessage(m_sent,
						      filt,
						      flags,
						      obj,
						      true);
		inv_id = m_sent.invoke_id_;
	} else {
		manager_->getReadObjectRequestMessage(m_sent,
						      filt,
						      flags,
						      obj,
						      false);
		inv_id = m_sent.invoke_id_;
		m_sent.invoke_id_ = invoke_id;
		inv_id = invoke_id;
	}

	send(m_sent, con);
	return inv_id;
}

int CDAPProvider::remote_cancel_read(const cdap_rib::con_handle_t &con,
				     const cdap_rib::flags_t &flags,
				     int invoke_id)
{
	cdap_m_t m_sent;

	manager_->getCancelReadRequestMessage(m_sent,
					      flags,
					      invoke_id);
	send(m_sent, con);

	return invoke_id;
}

int CDAPProvider::remote_write(const cdap_rib::con_handle_t &con,
			       const cdap_rib::obj_info_t &obj,
			       const cdap_rib::flags_t &flags,
			       const cdap_rib::filt_info_t &filt,
			       const int invoke_id)
{
	cdap_m_t m_sent;
	int inv_id;

	if (invoke_id < 0) {
		manager_->getWriteObjectRequestMessage(m_sent,
					       	       filt,
					       	       flags,
					       	       obj,
					       	       true);
		inv_id = m_sent.invoke_id_;
	} else {
		manager_->getWriteObjectRequestMessage(m_sent,
					       	      filt,
					       	      flags,
					       	      obj,
					       	      false);
		m_sent.invoke_id_ = invoke_id;
		inv_id = invoke_id;
	}
	send(m_sent, con);
	return inv_id;
}

int CDAPProvider::remote_start(const cdap_rib::con_handle_t &con,
			       const cdap_rib::obj_info_t &obj,
			       const cdap_rib::flags_t &flags,
			       const cdap_rib::filt_info_t &filt,
			       const int invoke_id)
{
	cdap_m_t m_sent;
	int inv_id;

	if (invoke_id < 0) {
		manager_->getStartObjectRequestMessage(m_sent,
					       	       filt,
					       	       flags,
					       	       obj,
					       	       true);
		inv_id = m_sent.invoke_id_;
	} else {
		manager_->getStartObjectRequestMessage(m_sent,
					       	       filt,
					       	       flags,
					       	       obj,
					       	       false);
		m_sent.invoke_id_ = invoke_id;
		inv_id = invoke_id;
	}

	send(m_sent, con);
	return inv_id;
}

int CDAPProvider::remote_stop(const cdap_rib::con_handle_t &con,
			      const cdap_rib::obj_info_t &obj,
			      const cdap_rib::flags_t &flags,
			      const cdap_rib::filt_info_t &filt,
			      const int invoke_id)
{
	cdap_m_t m_sent;
	int inv_id;

	if (invoke_id < 0) {
		manager_->getStopObjectRequestMessage(m_sent,
						      filt,
						      flags,
						      obj,
						      true);
		inv_id = m_sent.invoke_id_;
	} else {
		manager_->getStopObjectRequestMessage(m_sent,
						      filt,
						      flags,
						      obj,
						      false);

		m_sent.invoke_id_ = invoke_id;
		inv_id = invoke_id;
	}

	send(m_sent, con);
	return inv_id;
}

void CDAPProvider::send_open_connection_result(const cdap_rib::con_handle_t &con,
					       const cdap_rib::res_info_t &res,
					       int invoke_id)
{
	cdap_m_t m_sent;

	manager_->getOpenConnectionResponseMessage(m_sent,
						   con,
						   res,
						   invoke_id);
	send(m_sent, con);
}

void CDAPProvider::send_close_connection_result(unsigned int port,
					        const cdap_rib::flags_t &flags,
					        const cdap_rib::res_info_t &res,
					        int invoke_id)
{
	cdap_m_t m_sent;
	rina::cdap_rib::con_handle_t con;

	//FIXME: change flags
	manager_->getReleaseConnectionResponseMessage(m_sent,
						      flags,
						      res,
						      invoke_id);

	con.port_id = port;
	con.cdap_dest = cdap_rib::CDAP_DEST_PORT;
	send(m_sent, con);
}

void CDAPProvider::send_create_result(const cdap_rib::con_handle_t &con,
				      const cdap_rib::obj_info_t &obj,
				      const cdap_rib::flags_t &flags,
				      const cdap_rib::res_info_t &res,
				      int invoke_id)
{
	cdap_m_t m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	manager_->getCreateObjectResponseMessage(m_sent,
						 flags,
						 obj,
						 res,
						 invoke_id);
	send(m_sent, con);
}

void CDAPProvider::send_delete_result(const cdap_rib::con_handle_t &con,
				      const cdap_rib::obj_info_t &obj,
				      const cdap_rib::flags_t &flags,
				      const cdap_rib::res_info_t &res,
				      int invoke_id)
{
	cdap_m_t m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	manager_->getDeleteObjectResponseMessage(m_sent,
						 flags,
						 obj,
						 res,
						 invoke_id);
	send(m_sent, con);
}

void CDAPProvider::send_read_result(const cdap_rib::con_handle_t &con,
				    const cdap_rib::obj_info_t &obj,
				    const cdap_rib::flags_t &flags,
				    const cdap_rib::res_info_t &res,
				    int invoke_id)
{
	cdap_m_t m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	manager_->getReadObjectResponseMessage(m_sent,
					       flags,
					       obj,
					       res,
					       invoke_id);
	send(m_sent, con);
}

void CDAPProvider::send_cancel_read_result(const cdap_rib::con_handle_t &con,
					   const cdap_rib::flags_t &flags,
					   const cdap_rib::res_info_t &res,
					   int invoke_id)
{
	cdap_m_t m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	manager_->getCancelReadResponseMessage(m_sent,
					       flags,
					       res,
					       invoke_id);
	send(m_sent, con);
}

void CDAPProvider::send_write_result(const cdap_rib::con_handle_t &con,
				     const cdap_rib::flags_t &flags,
				     const cdap_rib::res_info_t &res,
				     int invoke_id)
{
	cdap_m_t m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	manager_->getWriteObjectResponseMessage(m_sent,
						flags,
						res,
						invoke_id);
	send(m_sent, con);
}

void CDAPProvider::send_start_result(const cdap_rib::con_handle_t &con,
				     const cdap_rib::obj_info_t &obj,
				     const cdap_rib::flags_t &flags,
				     const cdap_rib::res_info_t &res,
				     int invoke_id)
{
	cdap_m_t m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	manager_->getStartObjectResponseMessage(m_sent,
						flags,
						obj,
						res,
						invoke_id);
	send(m_sent, con);
}

void CDAPProvider::send_stop_result(const cdap_rib::con_handle_t &con,
				    const cdap_rib::flags_t &flags,
				    const cdap_rib::res_info_t &res,
				    int invoke_id)
{
	cdap_m_t m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	manager_->getStopObjectResponseMessage(m_sent,
					       flags,
					       res,
					       invoke_id);
	send(m_sent, con);
}

void CDAPProvider::process_message(const ser_obj_t &message,
				   unsigned int port,
				   cdap_rib::cdap_dest_t cdap_dest)
{
	if (!io_handler_)
		throw Exception("CDAP IO Handler not set!");

	io_handler_->process_message(message, port, cdap_dest);
}

void CDAPProvider::send(const cdap_m_t & m_sent,
			const cdap_rib::con_handle_t &con)
{
	if (!io_handler_)
		throw Exception("CDAP IO Handler not set!");

	io_handler_->send(m_sent, con);
}

void CDAPProvider::set_cdap_io_handler(CDAPIOHandler * handler)
{
	if (io_handler_)
		throw Exception("CDAP IO Handler already set!");

	io_handler_ = handler;
	io_handler_->callback_ = callback_;
	io_handler_->manager_ = manager_;
}

CDAPSessionManagerInterface * CDAPProvider::get_session_manager()
{
	return manager_;
}

void AppCDAPIOHandler::process_message(const ser_obj_t &message,
				       unsigned int port,
				       cdap_rib::cdap_dest_t cdap_dest)
{
	(void) cdap_dest;
	cdap_m_t m_rcv;

	atomic_send_lock_.lock();
	try {
		manager_->messageReceived(message, m_rcv, port);
	} catch (rina::Exception &e) {
		atomic_send_lock_.unlock();
		throw e;
	}
	atomic_send_lock_.unlock();

	// Fill structures
	cdap_rib::con_handle_t con = manager_->get_con_handle(port);
	// Flags
	cdap_rib::flags_t flags;
	flags.flags_ = m_rcv.flags_;
	// Object
	cdap_rib::obj_info_t obj;
	obj.class_ = m_rcv.obj_class_;
	obj.inst_ = m_rcv.obj_inst_;
	obj.name_ = m_rcv.obj_name_;
	obj.value_.size_ = m_rcv.obj_value_.size_;
	obj.value_.message_ = new unsigned char[obj.value_.size_];
	memcpy(obj.value_.message_, m_rcv.obj_value_.message_,
	       m_rcv.obj_value_.size_);
	// Filter
	cdap_rib::filt_info_t filt;
	filt.filter_ = m_rcv.filter_;
	filt.scope_ = m_rcv.scope_;
	// Invoke id
	int invoke_id = m_rcv.invoke_id_;
	// Result
	cdap_rib::res_info_t res;
	//FIXME: do not typecast when the codes are an enum in the GPB
	res.code_ = static_cast<cdap_rib::res_code_t>(m_rcv.result_);
	res.reason_ = m_rcv.result_reason_;

	switch (m_rcv.op_code_) {

		//Local
		case cdap_m_t::M_CONNECT:
			callback_->open_connection(con,
						   m_rcv);
			break;
		case cdap_m_t::M_RELEASE:
			callback_->close_connection(con,
						    flags,
						    invoke_id);
			break;
		case cdap_m_t::M_DELETE:
			callback_->delete_request(con,
						  obj,
						  filt,
						  invoke_id);
			break;
		case cdap_m_t::M_CREATE:
			callback_->create_request(con,
						  obj,
						  filt,
						  invoke_id);
			break;
		case cdap_m_t::M_READ:
			callback_->read_request(con,
						obj,
						filt,
						invoke_id);
			break;
		case cdap_m_t::M_CANCELREAD:
			callback_->cancel_read_request(con,
						       obj,
						       filt,
						       invoke_id);
			break;
		case cdap_m_t::M_WRITE:
			callback_->write_request(con,
						 obj,
						 filt,
						 invoke_id);
			break;
		case cdap_m_t::M_START:
			callback_->start_request(con,
						 obj,
						 filt,
						 invoke_id);
			break;
		case cdap_m_t::M_STOP:
			callback_->stop_request(con,
						obj,
						filt,
						invoke_id);
			break;

		//Remote
		case cdap_m_t::M_CONNECT_R:
			callback_->remote_open_connection_result(con,
								 res);
			break;
		case cdap_m_t::M_RELEASE_R:
			callback_->remote_close_connection_result(con,
								  res);
			break;
		case cdap_m_t::M_CREATE_R:
			callback_->remote_create_result(con,
							obj,
							res,
							invoke_id);
			break;
		case cdap_m_t::M_DELETE_R:
			callback_->remote_delete_result(con,
							res,
							invoke_id);
			break;
		case cdap_m_t::M_READ_R:
			callback_->remote_read_result(con,
						      obj,
						      res,
						      flags,
						      invoke_id);
			break;
		case cdap_m_t::M_CANCELREAD_R:
			callback_->remote_cancel_read_result(con,
							     res,
							     invoke_id);
			break;
		case cdap_m_t::M_WRITE_R:
			callback_->remote_write_result(con,
						       obj,
						       res,
						       invoke_id);
			break;
		case cdap_m_t::M_START_R:
			callback_->remote_start_result(con,
						       obj,
						       res,
						       invoke_id);
			break;
		case cdap_m_t::M_STOP_R:
			callback_->remote_stop_result(con,
						      obj,
						      res,
						      invoke_id);
			break;
		default:
			LOG_ERR("Operation not recognized");
			break;
	}
}

void AppCDAPIOHandler::send(const cdap_m_t & m_sent,
			    const cdap_rib::con_handle_t &con)
{
	ser_obj_t ser_sent_m;

	manager_->encodeNextMessageToBeSent(m_sent,
					    ser_sent_m,
					    con.port_id);

	rina::ScopedLock slock(atomic_send_lock_);

	try{
		rina::ipcManager->writeSDU(con.port_id,
					   ser_sent_m.message_,
					   ser_sent_m.size_);
		manager_->messageSent(m_sent, con.port_id);
	} catch (rina::Exception &e) {

		if (m_sent.invoke_id_ != 0 && m_sent.is_request_message())
			manager_->get_invoke_id_manager()->freeInvokeId(m_sent.invoke_id_,
								       false);

		std::string reason = std::string(e.what());
		if (reason.compare("Flow closed") == 0)
			manager_->removeCDAPSession(con.port_id);

		throw e;
	}
}

//
// Default empty callbacks
//
CDAPCallbackInterface::~CDAPCallbackInterface()
{
}
void CDAPCallbackInterface::remote_open_connection_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::result_info &res)
{
	LOG_INFO("Callback open_connection_result operation not implemented");
}
void CDAPCallbackInterface::open_connection(const cdap_rib::con_handle_t &con,
					    const cdap::CDAPMessage& message)
{
	LOG_INFO("Callback open_connection operation not implemented");
}
void CDAPCallbackInterface::remote_close_connection_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::result_info &res)
{
	LOG_INFO("Callback close_connection_result operation not implemented");

}
void CDAPCallbackInterface::close_connection(const cdap_rib::con_handle_t &con,
		const cdap_rib::flags_t &flags,
		int invoke_id)
{
	LOG_INFO("Callback close_connection operation not implemented");
}

void CDAPCallbackInterface::remote_create_result(const cdap_rib::con_handle_t &con,
						 const cdap_rib::obj_info_t &obj,
						 const cdap_rib::res_info_t &res,
						 const int invoke_id)
{
	LOG_INFO("Callback create_result operation not implemented");
}

void CDAPCallbackInterface::remote_delete_result(const cdap_rib::con_handle_t &con,
						 const cdap_rib::res_info_t &res,
						 const int invoke_id)
{
	LOG_INFO("Callback delete_result operation not implemented");
}

void CDAPCallbackInterface::remote_read_result(const cdap_rib::con_handle_t &con,
					       const cdap_rib::obj_info_t &obj,
					       const cdap_rib::res_info_t &res,
					       const cdap_rib::flags_t &flags,
					       const int invoke_id)
{
	LOG_INFO("Callback read_result operation not implemented");
}

void CDAPCallbackInterface::remote_cancel_read_result(const cdap_rib::con_handle_t &con,
						      const cdap_rib::res_info_t &res,
						      const int invoke_id)
{
	LOG_INFO("Callback cancel_read_result operation not implemented");
}

void CDAPCallbackInterface::remote_write_result(const cdap_rib::con_handle_t &con,
						const cdap_rib::obj_info_t &obj,
						const cdap_rib::res_info_t &res,
						const int invoke_id)
{
	LOG_INFO("Callback write_result operation not implemented");
}

void CDAPCallbackInterface::remote_start_result(const cdap_rib::con_handle_t &con,
						const cdap_rib::obj_info_t &obj,
						const cdap_rib::res_info_t &res,
						const int invoke_id)
{
	LOG_INFO("Callback start_result operation not implemented");
}

void CDAPCallbackInterface::remote_stop_result(const cdap_rib::con_handle_t &con,
					       const cdap_rib::obj_info_t &obj,
					       const cdap_rib::res_info_t &res,
					       const int invoke_id)
{
	LOG_INFO("Callback stop_result operation not implemented");
}

void CDAPCallbackInterface::create_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int invoke_id)
{
	LOG_INFO("Callback create_request operation not implemented");
}
void CDAPCallbackInterface::delete_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int invoke_id)
{
	LOG_INFO("Callback delete_request operation not implemented");
}
void CDAPCallbackInterface::read_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int invoke_id)
{
	LOG_INFO("Callback read_request operation not implemented");
}
void CDAPCallbackInterface::cancel_read_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int invoke_id)
{
	LOG_INFO("Callback cancel_read_request operation not implemented");
}
void CDAPCallbackInterface::write_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int invoke_id)
{
	LOG_INFO("Callback write_request operation not implemented");
}
void CDAPCallbackInterface::start_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int invoke_id)
{
	LOG_INFO("Callback start_request operation not implemented");
}
void CDAPCallbackInterface::stop_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int invoke_id)
{
	LOG_INFO("Callback stop_request operation not implemented");
}

void CDAPCallbackInterface::process_authentication_message(const cdap::CDAPMessage& message,
					    	    	   const cdap_rib::con_handle_t &con)
{
	LOG_INFO("Callback process_authentication_message operation not implemented");
}

/// Class ADataObject
const std::string ADataObject::A_DATA_OBJECT_CLASS = "a_data";
const std::string ADataObject::A_DATA = "a_data";
const std::string ADataObject::A_DATA_OBJECT_NAME = A_DATA;

ADataObject::ADataObject()
{
	source_address_ = 0;
	dest_address_ = 0;
}

ADataObject::ADataObject(unsigned int source_address,
			 unsigned int dest_address)
{
	source_address_ = source_address;
	dest_address_ = dest_address;
}

ADataObject::~ADataObject()
{
}

//
// CDAP Provider
//

//Static elements
static bool inited = false;
static CDAPSessionManager *manager = NULL;
static CDAPProviderInterface* iface = NULL;

CDAPProviderInterface* getProvider(){
	return iface;
}

void init(cdap::CDAPCallbackInterface *callback,
	  cdap_rib::concrete_syntax_t &syntax,
	  bool is_IPCP)
{
	//First check the flag
	if(inited){
		LOG_ERR("Double call to rina::cdap::init()");
		throw Exception("Double call to rina::cdap::init()");
	}

	//Initialize subcomponents
	inited = true;
	manager = new CDAPSessionManager(syntax);
	iface = new CDAPProvider(callback, manager);

	if (!is_IPCP)
		iface->set_cdap_io_handler(new AppCDAPIOHandler());
}

void set_cdap_io_handler(cdap::CDAPIOHandler *handler)
{
	if (!inited) {
		LOG_ERR("CDAP has not been initialized yet");
		throw Exception("CDAP has not been initialized yet");

	}

	iface->set_cdap_io_handler(handler);
}

void destroy(int port){
	manager->removeCDAPSession(port);
}

void fini(){
	delete manager;
	delete iface;
}

CDAPMessageEncoder::CDAPMessageEncoder(cdap_rib::concrete_syntax_t& cdap_syntax)
{
	if (cdap_syntax.syntax == cdap_rib::concrete_syntax_t::GPB)
		serializer = new GPBSerializer();
	else
		throw CDAPException("Unsupported concrete syntax");
}

CDAPMessageEncoder::~CDAPMessageEncoder()
{
	if (serializer) {
		delete serializer;
		serializer = 0;
	}
}

void CDAPMessageEncoder::encode(const cdap_m_t &obj,
				ser_obj_t& serobj)
{
	serializer->serializeMessage(obj, serobj);
}

void CDAPMessageEncoder::decode(const ser_obj_t &serobj,
				cdap_m_t &des_obj)
{
	serializer->deserializeMessage(serobj, des_obj);
}

void StringEncoder::encode(const std::string& obj, ser_obj_t& serobj)
{
	messages::string_t s;
	s.set_value(obj);

	//Allocate memory
	serobj.size_ = s.ByteSize();
	serobj.message_ = new unsigned char[serobj.size_];

	if (!serobj.message_)
		throw rina::Exception("out of memory");  //TODO improve this

	s.SerializeToArray(serobj.message_, serobj.size_);
}

void StringEncoder::decode(const ser_obj_t& ser_obj, std::string& obj)
{
	messages::string_t s;
	s.ParseFromArray(ser_obj.message_, ser_obj.size_);

	obj = s.value();
}

// Class IntEncoder
void IntEncoder::encode(const int &obj, ser_obj_t& serobj)
{
	messages::int_t gpb;

	gpb.set_value(obj);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new unsigned char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void IntEncoder::decode(const ser_obj_t &serobj, int &des_obj)
{
	messages::int_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	des_obj = gpb.value();
}

} //namespace cdap
} //namespace rina
