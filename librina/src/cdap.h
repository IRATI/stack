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
#include "librina-cdap.h"
#define RINA_PREFIX "cdap-manager"
#include "logs.h"


namespace rina {

/// Implements the CDAP connection state machine
class CDAPSessionImpl;
class ConnectionStateMachine {
public:
	ConnectionStateMachine(CDAPSessionInterface *cdapSession, long timeout);
	// FIXME: synchronized
	bool is_connected() const;
	/// Checks if a the CDAP connection can be opened (i.e. an M_CONNECT message can be sent)
	/// @throws CDAPException
	// FIXME: synchronized
	void checkConnect();
	void connectSentOrReceived(const CDAPMessage &cdapMessage, bool sent);
	/// Checks if the CDAP M_CONNECT_R message can be sent
	/// @throws CDAPException
	// FIXME: synchronized
	void checkConnectResponse();
	// FIXME: synchronized
	void connectResponseSentOrReceived(const CDAPMessage &cdapMessage, bool sent);
	/// Checks if the CDAP M_RELEASE message can be sent
	/// @throws CDAPException
	// FIXME: synchronized
	void checkRelease();
	// FIXME: synchronized
	void releaseSentOrReceived(const CDAPMessage &cdapMessage, bool sent);
	/// Checks if the CDAP M_RELEASE_R message can be sent
	/// @throws CDAPException
	// FIXME: synchronized
	void checkReleaseResponse();
	// FIXME: synchronized
	void releaseResponseSentOrReceived(const CDAPMessage &cdapMessage, bool sent);
private:
	enum ConnectionState {NONE, AWAITCON, CONNECTED, AWAITCLOSE};
	/// The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	long timeout_;
	/// The flow that this CDAP connection operates over
	CDAPSessionInterface *cdap_session_;
	/// The state of the CDAP connection, drives the CDAP connection
	/// state machine
	ConnectionState connection_state_;
	/// The AE has sent an M_CONNECT message
	/// @throws CDAPException
	// FIXME: synchronized
	void connect();
	/// An M_CONNECT message has been received, update the state
	/// @param message
	// FIXME: synchronized
	void connectReceived(CDAPMessage message);
	/// The AE has sent an M_CONNECT_R  message
	/// @param openConnectionResponseMessage
	/// @throws CDAPException
	void connectResponse();
	/// An M_CONNECT_R message has been received
	/// @param message
	void connectResponseReceived(CDAPMessage message);
	/// The AE has sent an M_RELEASE message
	/// @param releaseConnectionRequestMessage
	/// @throws CDAPException
	void release(CDAPMessage cdapMessage);
	/// An M_RELEASE message has been received
	/// @param message
	void releaseReceived(CDAPMessage message);
	/// The AE has called the close response operation
	/// @param releaseConnectionRequestMessage
	/// @throws CDAPException
	void releaseResponse();
	/// An M_RELEASE_R message has been received
	/// @param message
	void releaseResponseReceived();
};

/// Encapsulates an operation state
class CDAPOperationState {
public:
	CDAPOperationState(CDAPMessage::Opcode opcode, bool sender);
	CDAPMessage::Opcode get_op_code() const;
	bool is_sender() const;
private:
	/// The opcode of the message
	CDAPMessage::Opcode op_code;
	/// Tells if sender or receiver
	bool sender;
};

/// It will always try to use short invokeIds (as close to 1 as possible)
class CDAPSessionInvokeIdManagerImpl: public CDAPSessionInvokeIdManagerInterface{
public:
	// synchronized
	void freeInvokeId(int invoke_id);
	//synchronized
	int newInvokeId();
	// synchronized
	void reserveInvokeId(int invoke_id);
private:
	std::list<int> used_invoke_ids_;
};

/// Implements a CDAP session. Has the necessary logic to ensure that a
/// the operation of the CDAP protocol is consistent (correct states and proper
/// invocation of operations)
class CDAPSessionManager;
class CDAPSessionImpl: public CDAPSessionInterface{
public:
	CDAPSessionImpl(CDAPSessionManager *cdap_session_manager, long timeout,
			WireMessageProviderInterface *wire_message_provider);
	~CDAPSessionImpl() throw();
	//const CDAPSessionInvokeIdManagerInterface* getInvokeIdManager();
	//bool isConnected() const;
	const char* encodeNextMessageToBeSent(const CDAPMessage &cdap_message);
	void messageSent(CDAPMessage &cdap_message);
	const CDAPMessage* messageReceived(const char message[]);
	void messageReceived(CDAPMessage &cdap_message);
	void set_session_descriptor(CDAPSessionDescriptor *session_descriptor);
	//int getPortId() const;
	CDAPSessionDescriptor* get_session_descriptor() const;
protected:
	void stopConnection();
private:
	void messageSentOrReceived(const CDAPMessage &cdap_message, bool sent);
	void freeOrReserveInvokeId(const CDAPMessage &cdap_message);
	void checkIsConnected() const;
	void checkInvokeIdNotExists(const CDAPMessage &cdap_message) const;
	void checkCanSendOrReceiveCancelReadRequest(const CDAPMessage &cdap_message, bool sender) const;
	void requestMessageSentOrReceived(const CDAPMessage &cdap_message, CDAPMessage::Opcode op_code, bool sent);
	void cancelReadMessageSentOrReceived(const CDAPMessage &cdap_message, bool sender);
	void checkCanSendOrReceiveResponse(const CDAPMessage &cdap_message, CDAPMessage::Opcode op_code, bool send) const;
	void checkCanSendOrReceiveCancelReadResponse(const CDAPMessage &cdap_message, bool send) const;
	void responseMessageSentOrReceived(const CDAPMessage &cdap_message, CDAPMessage::Opcode op_code, bool sent);
	void cancelReadResponseMessageSentOrReceived(const CDAPMessage &cdap_message, bool sent);
	const char* serializeMessage(const CDAPMessage &cdap_message) const;
	const CDAPMessage* deserializeMessage(const char message[]) const;
	void populateSessionDescriptor(const CDAPMessage &cdap_message, bool send);
	void emptySessionDescriptor();
	/// This map contains the invokeIds of the messages that
	/// have requested a response, except for the M_CANCELREADs
	std::map<int, CDAPOperationState*> pending_messages_;
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
	CDAPSessionInvokeIdManagerInterface *invoke_id_manager_;
};

/// Implements a CDAP session manager.
class CDAPSessionManager: public CDAPSessionManagerInterface {
public:
	static const long DEFAULT_TIMEOUT_IN_MS = 10000;
	CDAPSessionManager();
	CDAPSessionManager(
			WireMessageProviderFactoryInterface *wire_message_provider_factory);
	CDAPSessionManager(
			WireMessageProviderFactoryInterface *wire_message_provider_factory, long timeout);
	CDAPSessionInterface* createCDAPSession(int port_id);
	~CDAPSessionManager() throw();
	/// Get the identifiers of all the CDAP sessions
	void getAllCDAPSessionIds(std::vector<int> &vector);
	void getAllCDAPSessions(std::vector<CDAPSessionInterface*> &vector);
	CDAPSessionImpl* getCDAPSession(int port_id);
	///  Encodes a CDAP message. It just converts a CDAP message into a byte
	///  array, without caring about what session this CDAP message belongs to (and
	///  therefore it doesn't update any CDAP session state machine). Called by
	///  functions that have to relay CDAP messages, and need to encode/
	///  decode its contents to make the relay decision and maybe modify some
	///  message values.
	///  @param cdap_message
	///  @return
	///  @throws CDAPException
	const char* encodeCDAPMessage(const CDAPMessage &cdap_message);
	/// Decodes a CDAP message. It just converts the byte array into a CDAP
	/// message, without caring about what session this CDAP message belongs to (and
	/// therefore it doesn't update any CDAP session state machine). Called by
	/// functions that have to relay CDAP messages, and need to encode/
	/// decode its contents to make the relay decision and maybe modify some
	/// @param cdap_message
	/// @return
	/// @throws CDAPException
	const CDAPMessage* decodeCDAPMessage(char cdap_message[]);
	/// Called by the CDAPSession state machine when the cdap session is terminated
	/// @param portId
	void removeCDAPSession(int portId);
	/// Encodes the next CDAP message to be sent, and checks against the
	/// CDAP state machine that this is a valid message to be sent
	/// @param cdap_message The cdap message to be serialized
	/// @param portId
	/// @return encoded version of the CDAP Message
	/// @throws CDAPException
	const char* encodeNextMessageToBeSent(const CDAPMessage &cdap_message,
			int port_id);
	/// Depending on the message received, it will create a new CDAP state machine (CDAP Session), or update
	/// an existing one, or terminate one.
	/// @param encodedCDAPMessage
	/// @param portId
	/// @return Decoded CDAP Message
	/// @throws CDAPException if the message is not consistent with the appropriate CDAP state machine
	const CDAPMessage* messageReceived(char encodedCDAPMessage[], int portId);
	/// Update the CDAP state machine because we've sent a message through the
	/// flow identified by portId
	/// @param cdap_message The CDAP message to be serialized
	/// @param portId
	/// @return encoded version of the CDAP Message
	/// @throws CDAPException
	void messageSent(const CDAPMessage &cdap_message, int port_id);
	/// Return the portId of the (N-1) Flow that supports the CDAP Session
	/// with the IPC process identified by destinationApplicationProcessName
	/// @param destinationApplicationProcessName
	/// @throws CDAPException
	int getPortId(std::string destinationApplicationProcessName)
			throw (CDAPException);
	/// Create a M_CONNECT CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param authMech
	/// @param authValue
	/// @param destAEInst
	/// @param destAEName
	/// @param destApInst
	/// @param destApName
	/// @param srcAEInst
	/// @param srcAEName
	/// @param srcApInst
	/// @param srcApName
	/// @param invokeId true if one is requested, false otherwise
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getOpenConnectionRequestMessage(int port_id,
			CDAPMessage::AuthTypes auth_mech, const AuthValue &auth_value,
			std::string dest_ae_inst, std::string dest_ae_name,
			std::string dest_ap_inst, std::string dest_ap_name,
			std::string src_ae_inst, std::string src_ae_name,
			std::string src_ap_inst, std::string src_ap_name);
	/// Create a M_CONNECT_R CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param authMech
	/// @param authValue
	/// @param destAEInst
	/// @param destAEName
	/// @param destApInst
	/// @param destApName
	/// @param result
	/// @param resultReason
	/// @param srcAEInst
	/// @param srcAEName
	/// @param srcApInst
	/// @param srcApName
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getOpenConnectionResponseMessage(int port_id,
			CDAPMessage::AuthTypes auth_mech, const AuthValue &auth_value,
			std::string dest_ae_inst, std::string dest_ae_name,
			std::string dest_ap_inst, std::string dest_ap_name, int result,
			std::string result_reason, std::string src_ae_inst,
			std::string src_ae_name, std::string src_ap_inst,
			std::string src_ap_name, int invoke_id);
	/// Create an M_RELEASE CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param invokeID
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getReleaseConnectionRequestMessage(int portId,
			CDAPMessage::Flags flags, bool invokeID);
	/// Create a M_RELEASE_R CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param resultReason
	/// @param invokeId
	/// @return
	/// @throws CDAPException

	const CDAPMessage* getReleaseConnectionResponseMessage(int portId,
			CDAPMessage::Flags flags, int result, std::string resultReason,
			int invokeId);
	/// Create a M_CREATE CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param objClass
	/// @param objInst
	/// @param objName
	/// @param objValue
	/// @param scope
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getCreateObjectRequestMessage(int port_id,
			char filter[], CDAPMessage::Flags flags, std::string obj_class,
			long obj_inst, std::string obj_name,
			const ObjectValueInterface &obj_value, int scope, bool invoke_id);
	/// Crate a M_CREATE_R CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param objClass
	/// @param objInst
	/// @param objName
	/// @param objValue
	/// @param result
	/// @param resultReason
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getCreateObjectResponseMessage(int port_id,
			CDAPMessage::Flags flags, std::string obj_class, long obj_inst,
			std::string obj_name, const ObjectValueInterface &obj_value,
			int result, std::string result_reason, int invoke_id);
	/// Create a M_DELETE CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param objClass
	/// @param objInst
	/// @param objName
	/// @param objectValue
	/// @param scope
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getDeleteObjectRequestMessage(int port_id,
			char* filter, CDAPMessage::Flags flags, std::string obj_class,
			long obj_inst, std::string obj_name,
			const ObjectValueInterface &object_value, int scope, bool invoke_id)
			= 0;
	/// Create a M_DELETE_R CDAP MESSAGE
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param obj_class
	/// @param obj_inst
	/// @param obj_name
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getDeleteObjectResponseMessage(int port_id,
			CDAPMessage::Flags flags, std::string obj_class, long obj_inst,
			std::string obj_name, int result, std::string result_reason,
			int invoke_id);
	/// Create a M_START CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param obj_class
	/// @param obj_value
	/// @param obj_inst
	/// @param obj_name
	/// @param scope
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getStartObjectRequestMessage(int port_id,
			char filter[], CDAPMessage::Flags flags, std::string obj_class,
			const ObjectValueInterface &obj_value, long obj_inst,
			std::string obj_name, int scope, bool invoke_id)
			= 0;
	/// Create a M_START_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getStartObjectResponseMessage(int port_id,
			CDAPMessage::Flags flags, int result, std::string result_reason,
			int invoke_id);
	/// Create a M_START_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getStartObjectResponseMessage(int port_id,
			CDAPMessage::Flags flags, std::string obj_class,
			const ObjectValueInterface &obj_value, long obj_inst,
			std::string obj_name, int result, std::string result_reason,
			int invoke_id);
	/// Create a M_STOP CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param obj_class
	/// @param obj_value
	/// @param obj_inst
	/// @param obj_name
	/// @param scope
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getStopObjectRequestMessage(int port_id,
			char* filter, CDAPMessage::Flags flags, std::string obj_class,
			const ObjectValueInterface &obj_value, long obj_inst,
			std::string obj_name, int scope, bool invoke_id)
			= 0;
	/// Create a M_STOP_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getStopObjectResponseMessage(int port_id,
			CDAPMessage::Flags flags, int result, std::string result_reason,
			int invoke_id);
	/// Create a M_READ CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param obj_class
	/// @param obj_inst
	/// @param obj_name
	/// @param scope
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getReadObjectRequestMessage(int port_id,
			char filter[], CDAPMessage::Flags flags, std::string obj_class,
			long obj_inst, std::string obj_name, int scope, bool invoke_id)
			= 0;
	/// Crate a M_READ_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param obj_class
	/// @param obj_inst
	/// @param obj_name
	/// @param obj_value
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getReadObjectResponseMessage(int port_id,
			CDAPMessage::Flags flags, std::string obj_class, long obj_inst,
			std::string obj_name, const ObjectValueInterface &obj_value,
			int result, std::string result_reason, int invoke_id)
			= 0;
	/// Create a M_WRITE CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param obj_class
	/// @param obj_inst
	/// @param obj_value
	/// @param obj_name
	/// @param scope
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getWriteObjectRequestMessage(int port_id,
			char filter[], CDAPMessage::Flags flags, std::string obj_class,
			long obj_inst, const ObjectValueInterface &obj_value,
			std::string obj_name, int scope, bool invoke_id);
	/// Create a M_WRITE_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getWriteObjectResponseMessage(int port_id,
			CDAPMessage::Flags flags, int result, std::string result_reason,
			int invoke_id);
	/// Create a M_CANCELREAD CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getCancelReadRequestMessage(int port_id,
			CDAPMessage::Flags flags, int invoke_id);
	/// Create a M_CANCELREAD_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param invoke_id
	/// @param result
	/// @param result_reason
	/// @return
	/// @throws CDAPException
	const CDAPMessage* getCancelReadResponseMessage(int port_id,
			CDAPMessage::Flags flags, int invoke_id, int result,
			std::string result_reason);
private:
	void assignInvokeId(const CDAPMessage &cdap_message, bool invoke_id, int port_id);
	WireMessageProviderFactoryInterface* wire_message_provider_factory_;
	std::map<int, CDAPSessionImpl*> cdap_sessions_;
	/// Used by the serialize and unserialize operations
	WireMessageProviderInterface *wire_message_provider_;
	/// The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	long timeout_;
};
}
#endif
#endif // CDAP_H_
