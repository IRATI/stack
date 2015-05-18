/* cdap.h
 * Created on: May 20, 2014
 * Author: bernat
 */

#ifndef CDAP_H_
#define CDAP_H_

#ifdef __cplusplus

#include <memory>
#include <map>
#include <sstream>
#include <algorithm>

#include "librina/cdap.h"
#include "librina/concurrency.h"
#include "librina/timer.h"

namespace rina {

class ConnectionStateMachine;
// TODO: make these two classes one
class ResetStablishmentTimerTask : public TimerTask {
public:
	ResetStablishmentTimerTask(ConnectionStateMachine *con_state_machine);
	void run();
private:
	ConnectionStateMachine *con_state_machine_;
};

class ReleaseConnectionTimerTask : public TimerTask {
public:
	ReleaseConnectionTimerTask(ConnectionStateMachine *con_state_machine);
	void run();
private:
	ConnectionStateMachine *con_state_machine_;
};

/// Implements the CDAP connection state machine
class CDAPSessionImpl;
class ConnectionStateMachine: public Lockable {
public:
	ConnectionStateMachine(CDAPSessionImpl* cdap_session, long timeout);
	~ConnectionStateMachine() throw();
	bool is_connected();
	bool can_send_or_receive_messages();
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
	void releaseSentOrReceived(const CDAPMessage &cdap_message, bool sent);
	/// Checks if the CDAP M_RELEASE_R message can be sent
	/// @throws CDAPException
	void checkReleaseResponse();
	void releaseResponseSentOrReceived(bool sent);
	std::string get_state();
private:
	enum ConnectionState {
		NONE, AWAITCON, CONNECTED, AWAITCLOSE
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
	void release(const CDAPMessage &cdapMessage);
	/// An M_RELEASE message has been received
	/// @param message
	void releaseReceived(const CDAPMessage &message);
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
	CDAPSessionImpl *cdap_session_;
	/// The state of the CDAP connection, drives the CDAP connection
	/// state machine
	ConnectionState connection_state_;

	Timer * timer;
	TimerTask * last_timer_task_;

	friend class ResetStablishmentTimerTask;
	friend class ReleaseConnectionTimerTask;
};


/// Encapsulates an operation state
class CDAPOperationState {
public:
	CDAPOperationState(CDAPMessage::Opcode op_code, bool sender);
	~CDAPOperationState();
	CDAPMessage::Opcode get_op_code() const;
	bool is_sender() const;
private:
	/// The opcode of the message
	CDAPMessage::Opcode op_code_;
	/// Tells if sender or receiver
	bool sender_;
};

/// It will always try to use short invokeIds (as close to 1 as possible)
class CDAPInvokeIdManagerImpl: public CDAPInvokeIdManagerInterface, Lockable {
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

/// Implements a CDAP session. Has the necessary logic to ensure that a
/// the operation of the CDAP protocol is consistent (correct states and proper
/// invocation of operations)
class CDAPSessionManager;
class CDAPSessionImpl: public CDAPSessionInterface {
public:
	CDAPSessionImpl(CDAPSessionManager *cdap_session_manager, long timeout,
			WireMessageProviderInterface *wire_message_provider, CDAPInvokeIdManagerImpl *invoke_id_manager);
	~CDAPSessionImpl() throw ();
	//const CDAPSessionInvokeIdManagerInterface* getInvokeIdManager();
	//bool isConnected() const;
	const SerializedObject* encodeNextMessageToBeSent(const CDAPMessage &cdap_message);
	void messageSent(const CDAPMessage &cdap_message);
	const CDAPMessage* messageReceived(const SerializedObject &message);
	void messageReceived(const CDAPMessage &cdap_message);
	void set_session_descriptor(CDAPSessionDescriptor *session_descriptor);
	int get_port_id() const;
	CDAPSessionDescriptor* get_session_descriptor() const;
	CDAPInvokeIdManagerImpl* get_invoke_id_manager() const;
	bool is_closed() const;
	std::string get_session_state() const;
private:
	void messageSentOrReceived(const CDAPMessage &cdap_message, bool sent);
	void freeOrReserveInvokeId(const CDAPMessage &cdap_message, bool sent);
	void checkIsConnected() const;
	void check_can_send_or_receive_messages() const;
	void checkInvokeIdNotExists(const CDAPMessage &cdap_message, bool sent) const;
	void checkCanSendOrReceiveCancelReadRequest(const CDAPMessage &cdap_message,
			bool sent) const;
	void requestMessageSentOrReceived(const CDAPMessage &cdap_message,
			CDAPMessage::Opcode op_code, bool sent);
	void cancelReadMessageSentOrReceived(const CDAPMessage &cdap_message,
			bool sender);
	void checkCanSendOrReceiveResponse(const CDAPMessage &cdap_message,
			CDAPMessage::Opcode op_code, bool sender) const;
	void checkCanSendOrReceiveCancelReadResponse(
			const CDAPMessage &cdap_message, bool send) const;
	void responseMessageSentOrReceived(const CDAPMessage &cdap_message,
			CDAPMessage::Opcode op_code, bool sent);
	void cancelReadResponseMessageSentOrReceived(
			const CDAPMessage &cdap_message, bool sent);
	const SerializedObject* serializeMessage(const CDAPMessage &cdap_message) const;
	const CDAPMessage* deserializeMessage(const SerializedObject &message) const;
	void populateSessionDescriptor(const CDAPMessage &cdap_message, bool send);
	void emptySessionDescriptor();
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
	WireMessageProviderInterface *wire_message_provider_;
	CDAPSessionDescriptor *session_descriptor_;
	CDAPSessionManager *cdap_session_manager_;
	CDAPInvokeIdManagerImpl *invoke_id_manager_;
};

/// Implements a CDAP session manager.
class CDAPSessionManager: public CDAPSessionManagerInterface {
public:
	static const long DEFAULT_TIMEOUT_IN_MS = 10000;
	CDAPSessionManager();
	CDAPSessionManager(
			WireMessageProviderFactory *wire_message_provider_factory);
	CDAPSessionManager(
			WireMessageProviderFactory *wire_message_provider_factory,
			long timeout);
	~CDAPSessionManager() throw ();
	CDAPSessionImpl* createCDAPSession(int port_id);
	void getAllCDAPSessionIds(std::vector<int> &vector);
	void getAllCDAPSessions(std::vector<CDAPSessionInterface*> &vector);
	CDAPSessionImpl* get_cdap_session(int port_id);
	const SerializedObject* encodeCDAPMessage(const CDAPMessage &cdap_message);
	const CDAPMessage* decodeCDAPMessage(const SerializedObject &cdap_message);
	void removeCDAPSession(int portId);
	const SerializedObject* encodeNextMessageToBeSent(const CDAPMessage &cdap_message,
			int port_id);
	const CDAPMessage* messageReceived(const SerializedObject &encodedCDAPMessage, int portId);
	void messageSent(const CDAPMessage &cdap_message, int port_id);
	int get_port_id(std::string destination_application_process_name);
	CDAPMessage* getOpenConnectionRequestMessage(int port_id,
			const AuthPolicy& auth_policy,
			const std::string &dest_ae_inst, const std::string &dest_ae_name,
			const std::string &dest_ap_inst, const std::string &dest_ap_name,
			const std::string &src_ae_inst, const std::string &src_ae_name,
			const std::string &src_ap_inst, const std::string &src_ap_name);
	CDAPMessage* getOpenConnectionResponseMessage(
			const AuthPolicy& auth_policy,
			const std::string &dest_ae_inst, const std::string &dest_ae_name,
			const std::string &dest_ap_inst, const std::string &dest_ap_name,
			int result, const std::string &result_reason,
			const std::string &src_ae_inst, const std::string &src_ae_name,
			const std::string &src_ap_inst, const std::string &src_ap_name,
			int invoke_id);
	CDAPMessage* getReleaseConnectionRequestMessage(int port_id,
			CDAPMessage::Flags flags, bool invoke_id);
	CDAPMessage* getReleaseConnectionResponseMessage(
			CDAPMessage::Flags flags, int result,
			const std::string &result_reason, int invoke_id);
	CDAPMessage* getCreateObjectRequestMessage(int port_id, char* filter,
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name,
			int scope, bool invoke_id);
	CDAPMessage* getCreateObjectResponseMessage(CDAPMessage::Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name,
			int result, const std::string &result_reason, int invoke_id);
	CDAPMessage* getDeleteObjectRequestMessage(int port_id, char* filter,
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name, int scope,
			bool invoke_id);
	CDAPMessage* getDeleteObjectResponseMessage(CDAPMessage::Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name, int result,
			const std::string &result_reason, int invoke_id);
	CDAPMessage* getStartObjectRequestMessage(int port_id, char* filter,
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name, int scope, bool invoke_id);
	CDAPMessage* getStartObjectResponseMessage(CDAPMessage::Flags flags, int result, const std::string &result_reason, int invoke_id);
	CDAPMessage* getStartObjectResponseMessage(CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst,
			const std::string &obj_name, int result,
			const std::string &result_reason, int invoke_id);
	CDAPMessage* getStopObjectRequestMessage(int port_id,
				char* filter, CDAPMessage::Flags flags,
				const std::string &obj_class,
				long obj_inst, const std::string &obj_name, int scope,
				bool invoke_id);
	CDAPMessage* getStopObjectResponseMessage(CDAPMessage::Flags flags, int result, const std::string &result_reason, int invoke_id);
	CDAPMessage* getReadObjectRequestMessage(int port_id, char * filter,
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name, int scope,
			bool invoke_id);
	CDAPMessage* getReadObjectResponseMessage(CDAPMessage::Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name,
			int result, const std::string &result_reason, int invoke_id);
	CDAPMessage* getWriteObjectRequestMessage(int port_id, char * filter,
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst,
			const std::string &obj_name, int scope, bool invoke_id);
	CDAPMessage* getWriteObjectResponseMessage(CDAPMessage::Flags flags, int result,
			const std::string &result_reason, int invoke_id);
	CDAPMessage* getCancelReadRequestMessage(CDAPMessage::Flags flags, int invoke_id);
	CDAPMessage* getCancelReadResponseMessage(CDAPMessage::Flags flags, int invoke_id, int result,
			const std::string &result_reason);
	CDAPMessage* getRequestMessage(int port_id,
			CDAPMessage::Opcode opcode, char * filter,
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name,
			int scope, bool invoke_id);
	CDAPMessage* getResponseMessage(CDAPMessage::Opcode opcode,
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name,
			int result,
			const std::string &result_reason, int invoke_id);
	CDAPInvokeIdManagerInterface * get_invoke_id_manager();
private:
	void assignInvokeId(CDAPMessage &cdap_message, bool invoke_id, int port_id, bool sent);
	WireMessageProviderFactory* wire_message_provider_factory_;
	std::map<int, CDAPSessionImpl*> cdap_sessions_;
	/// Used by the serialize and unserialize operations
	WireMessageProviderInterface *wire_message_provider_;
	/// The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	long timeout_;
	CDAPInvokeIdManagerImpl *invoke_id_manager_;
	ReadWriteLockable lock;
};

/// Google Protocol Buffers Wire Message Provider
class GPBWireMessageProvider :  public WireMessageProviderInterface {
public:
	const CDAPMessage* deserializeMessage(const SerializedObject &message);
	const SerializedObject* serializeMessage(const CDAPMessage &cdapMessage);
};

}
#endif
#endif // CDAP_H_
