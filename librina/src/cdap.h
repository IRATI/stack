/* cdap.h
 * Created on: May 20, 2014
 * Author: bernat
 */

#ifndef CDAP_H_
#define CDAP_H_

#ifdef __cplusplus

#include <memory>
#include <map>
#include "librina-cdap.h"

namespace rina {

/// Implements the CDAP connection state machine
class CDAPSessionImpl;
class ConnectionStateMachine {
public:
	ConnectionStateMachine(CDAPSessionImpl *cdapSession, long timeout);
	/// Checks if a the CDAP connection can be opened (i.e. an M_CONNECT message can be sent)
	/// @throws CDAPException
	// FIXME: synchronized
	void checkConnect() throw (CDAPException);
	void connectSentOrReceived(CDAPMessage cdapMessage, bool sent) throw (CDAPException);
	/// Checks if the CDAP M_CONNECT_R message can be sent
	/// @throws CDAPException
	// FIXME: synchronized
	void checkConnectResponse() throw (CDAPException);
	// FIXME: synchronized
	void connectResponseSentOrReceived(CDAPMessage cdapMessage, bool sent) throw (CDAPException);
	/// Checks if the CDAP M_RELEASE message can be sent
	/// @throws CDAPException
	// FIXME: synchronized
	void checkRelease() throw (CDAPException);
	// FIXME: synchronized
	void releaseSentOrReceived(CDAPMessage cdapMessage, bool sent) throw (CDAPException);
	/// Checks if the CDAP M_RELEASE_R message can be sent
	/// @throws CDAPException
	// FIXME: synchronized
	void checkReleaseResponse() throw (CDAPException);
	// FIXME: synchronized
	void releaseResponseSentOrReceived(CDAPMessage cdapMessage, bool sent) throw (CDAPException);
private:
	enum ConnectionState {NONE, AWAITCON, CONNECTED, AWAITCLOSE};
	/// The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	long timeout;
	/// The flow that this CDAP connection operates over
	CDAPSessionInterface *cdap_session;
	/// The state of the CDAP connection, drives the CDAP connection
	/// state machine
	ConnectionState connection_state;
	/// The AE has sent an M_CONNECT message
	/// @throws CDAPException
	// FIXME: synchronized
	void connect() throw (CDAPException);
	/// An M_CONNECT message has been received, update the state
	/// @param message
	// FIXME: synchronized
	void connectReceived(CDAPMessage message) throw (CDAPException);
	/// The AE has sent an M_CONNECT_R  message
	/// @param openConnectionResponseMessage
	/// @throws CDAPException
	void connectResponse() throw (CDAPException);
	/// An M_CONNECT_R message has been received
	/// @param message
	void connectResponseReceived(CDAPMessage message);
	/// The AE has sent an M_RELEASE message
	/// @param releaseConnectionRequestMessage
	/// @throws CDAPException
	void release(CDAPMessage cdapMessage) throw (CDAPException);
	/// An M_RELEASE message has been received
	/// @param message
	void releaseReceived(CDAPMessage message) throw (CDAPException);
	/// The AE has called the close response operation
	/// @param releaseConnectionRequestMessage
	/// @throws CDAPException
	void releaseResponse() throw (CDAPException);
	/// An M_RELEASE_R message has been received
	/// @param message
	void releaseResponseReceived() throw (CDAPException);
};

/// Implements a CDAP session. Has the necessary logic to ensure that a
/// the operation of the CDAP protocol is consistent (correct states and proper
/// invocation of operations)
class CDAPSessionManager;
class CDAPSessionImpl: public CDAPSession{
public:
	CDAPSessionImpl(CDAPSessionManager *cdap_session_manager, long timeout, WireMessageProviderInterface *wire_message_provider);
	~CDAPSessionImpl() throw();
	CDAPSessionInvokeIdManager getInvokeIdManager();
	bool isConnected();
	char* encodeNextMessageToBeSent(CDAPMessage cdap_message) throw (CDAPException);
	void messageSent(CDAPMessage cdap_message) throw (CDAPException);
	CDAPMessage messageReceived(char message[]) throw (CDAPException);
	CDAPMessage messageReceived(CDAPMessage cdap_message) throw (CDAPException);
	void setSessionDescriptor(CDAPSessionDescriptor sessionDescriptor);
	int getPortId();
protected:
	void stopConnection();
private:
	CDAPMessage messageSentOrReceived(CDAPMessage cdap_message, bool sent) throw (CDAPException);
	void freeOrReserveInvokeId(CDAPMessage cdap_message);
	void checkIsConnected() throw (CDAPException);
	void checkInvokeIdNotExists(CDAPMessage cdap_message) throw (CDAPException);
	void checkCanSendOrReceiveCancelReadRequest(CDAPMessage cdap_message, bool sender) throw (CDAPException);
	void requestMessageSentOrReceived(CDAPMessage cdap_message, CDAPMessage::Opcode op_code, bool sent) throw (CDAPException);
	void cancelReadMessageSentOrReceived(CDAPMessage cdap_message, bool sender) throw (CDAPException);
	void checkCanSendOrReceiveResponse(CDAPMessage cdap_message, CDAPMessage::Opcode op_code, bool send) throw (CDAPException);
	void checkCanSendOrReceiveCancelReadResponse(CDAPMessage cdap_message, bool send) throw (CDAPException);
	void responseMessageSentOrReceived(CDAPMessage cdap_message, CDAPMessage::Opcode op_code, bool sent) throw (CDAPException);
	void cancelReadResponseMessageSentOrReceived(CDAPMessage cdap_message, bool sent) throw (CDAPException);
	char* serializeMessage(CDAPMessage cdap_message) throw (CDAPException);
	CDAPMessage deserializeMessage(char message[]) throw (CDAPException);
	void populateSessionDescriptor(CDAPMessage cdap_message, bool send);
	void emptySessionDescriptor();
	/// Deals with the connection establishment and deletion messages and states
	ConnectionStateMachine connection_state_machine_;
	/// This map contains the invokeIds of the messages that
	/// have requested a response, except for the M_CANCELREADs
	//std::map<int, CDAPOperationState> pending_messages_;
	//std::map<int, CDAPOperationState> cancel_read_pending_messages_;
	WireMessageProviderInterface *wire_message_provider_;
	CDAPSessionDescriptor session_descriptor_;
	CDAPSessionManager *cdap_session_manager_;
	CDAPSessionInvokeIdManager invoke_id_manager_;
};

/// Implements a CDAP session manager.
class CDAPSessionManager: public CDAPSessionManagerInterface {
public:
	static const long DEFAULT_TIMEOUT_IN_MS = 10000;
	CDAPSessionManager();
	CDAPSessionManager(
			WireMessageProviderFactoryInterface &wire_message_provider_factory);
	CDAPSessionManager(
			WireMessageProviderFactoryInterface &wire_message_provider_factory, long timeout);
	CDAPSession* createCDAPSession(int port_id);
	~CDAPSessionManager() throw();
	/// Get the identifiers of all the CDAP sessions
	/// @return
	void getAllCDAPSessionIds(std::vector<int> &vector);
	void getAllCDAPSessions(std::vector<CDAPSession*> &vector);
	CDAPSessionInterface* getCDAPSession(int port_id);
	///  Encodes a CDAP message. It just converts a CDAP message into a byte
	///  array, without caring about what session this CDAP message belongs to (and
	///  therefore it doesn't update any CDAP session state machine). Called by
	///  functions that have to relay CDAP messages, and need to encode/
	///  decode its contents to make the relay decision and maybe modify some
	///  message values.
	///  @param cdap_message
	///  @return
	///  @throws CDAPException
	char* encodeCDAPMessage(CDAPMessage cdap_message) throw (CDAPException);
	/// Decodes a CDAP message. It just converts the byte array into a CDAP
	/// message, without caring about what session this CDAP message belongs to (and
	/// therefore it doesn't update any CDAP session state machine). Called by
	/// functions that have to relay CDAP messages, and need to encode/
	/// decode its contents to make the relay decision and maybe modify some
	/// @param cdap_message
	/// @return
	/// @throws CDAPException
	CDAPMessage* decodeCDAPMessage(char cdap_message[]) throw (CDAPException);
	/// Called by the CDAPSession state machine when the cdap session is terminated
	/// @param portId
	void removeCDAPSession(int portId);
	/// Encodes the next CDAP message to be sent, and checks against the
	/// CDAP state machine that this is a valid message to be sent
	/// @param cdap_message The cdap message to be serialized
	/// @param portId
	/// @return encoded version of the CDAP Message
	/// @throws CDAPException
	char* encodeNextMessageToBeSent(CDAPMessage cdap_message, int portId)
			throw (CDAPException);
	/// Depending on the message received, it will create a new CDAP state machine (CDAP Session), or update
	/// an existing one, or terminate one.
	/// @param encodedCDAPMessage
	/// @param portId
	/// @return Decoded CDAP Message
	/// @throws CDAPException if the message is not consistent with the appropriate CDAP state machine
	CDAPMessage* messageReceived(char encodedCDAPMessage[], int portId)
			throw (CDAPException);
	/// Update the CDAP state machine because we've sent a message through the
	/// flow identified by portId
	/// @param cdap_message The CDAP message to be serialized
	/// @param portId
	/// @return encoded version of the CDAP Message
	/// @throws CDAPException
	void messageSent(CDAPMessage cdap_message, int portId) throw (CDAPException);
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
	CDAPMessage* getOpenConnectionRequestMessage(int portId,
			CDAPMessage::AuthTypes authMech, AuthValue authValue,
			std::string destAEInst, std::string destAEName,
			std::string destApInst, std::string destApName,
			std::string srcAEInst, std::string srcAEName, std::string srcApInst,
			std::string srcApName) throw (CDAPException);
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
	CDAPMessage* getOpenConnectionResponseMessage(int portId,
			CDAPMessage::AuthTypes authMech, AuthValue authValue,
			std::string destAEInst, std::string destAEName,
			std::string destApInst, std::string destApName, int result,
			std::string resultReason, std::string srcAEInst,
			std::string srcAEName, std::string srcApInst, std::string srcApName,
			int invokeId) throw (CDAPException);
	/// Create an M_RELEASE CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param invokeID
	/// @return
	/// @throws CDAPException
	CDAPMessage* getReleaseConnectionRequestMessage(int portId,
			CDAPMessage::Flags flags, bool invokeID) throw (CDAPException);
	/// Create a M_RELEASE_R CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param resultReason
	/// @param invokeId
	/// @return
	/// @throws CDAPException

	CDAPMessage* getReleaseConnectionResponseMessage(int portId,
			CDAPMessage::Flags flags, int result, std::string resultReason,
			int invokeId) throw (CDAPException);
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
	CDAPMessage* getCreateObjectRequestMessage(int portId, char filter[],
			CDAPMessage::Flags flags, std::string objClass, long objInst,
			std::string objName, ObjectValueInterface &objValue, int scope,
			bool invokeId) throw (CDAPException);
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
	CDAPMessage* getCreateObjectResponseMessage(int portId,
			CDAPMessage::Flags flags, std::string objClass, long objInst,
			std::string objName, ObjectValueInterface &objValue, int result,
			std::string resultReason, int invokeId) throw (CDAPException);
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
	CDAPMessage* getDeleteObjectRequestMessage(int portId, char filter[],
			CDAPMessage::Flags flags, std::string objClass, long objInst,
			std::string objName, ObjectValueInterface &objectValue, int scope,
			bool invokeId) throw (CDAPException);
	/// Create a M_DELETE_R CDAP MESSAGE
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param objClass
	/// @param objInst
	/// @param objName
	/// @param result
	/// @param resultReason
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	CDAPMessage* getDeleteObjectResponseMessage(int portId,
			CDAPMessage::Flags flags, std::string objClass, long objInst,
			std::string objName, int result, std::string resultReason,
			int invokeId) throw (CDAPException);
	/// Create a M_START CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param objClass
	/// @param objValue
	/// @param objInst
	/// @param objName
	/// @param scope
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	CDAPMessage* getStartObjectRequestMessage(int portId, char filter[],
			CDAPMessage::Flags flags, std::string objClass,
			ObjectValueInterface &objValue, long objInst, std::string objName,
			int scope, bool invokeId) throw (CDAPException);
	/// Create a M_START_R CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param resultReason
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	CDAPMessage* getStartObjectResponseMessage(int portId,
			CDAPMessage::Flags flags, int result, std::string resultReason,
			int invokeId) throw (CDAPException);
	/// Create a M_START_R CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param resultReason
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	CDAPMessage* getStartObjectResponseMessage(int portId,
			CDAPMessage::Flags flags, std::string objClass,
			ObjectValueInterface &objValue, long objInst, std::string objName,
			int result, std::string resultReason, int invokeId)
					throw (CDAPException);
	/// Create a M_STOP CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param objClass
	/// @param objValue
	/// @param objInst
	/// @param objName
	/// @param scope
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	CDAPMessage* getStopObjectRequestMessage(int portId, char filter[],
			CDAPMessage::Flags flags, std::string objClass,
			ObjectValueInterface &objValue, long objInst, std::string objName,
			int scope, bool invokeId) throw (CDAPException);
	/// Create a M_STOP_R CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param resultReason
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	CDAPMessage* getStopObjectResponseMessage(int portId,
			CDAPMessage::Flags flags, int result, std::string resultReason,
			int invokeId) throw (CDAPException);
	/// Create a M_READ CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param objClass
	/// @param objInst
	/// @param objName
	/// @param scope
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	CDAPMessage* getReadObjectRequestMessage(int portId, char filter[],
			CDAPMessage::Flags flags, std::string objClass, long objInst,
			std::string objName, int scope, bool invokeId) throw (CDAPException);
	/// Crate a M_READ_R CDAP Message
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
	CDAPMessage* getReadObjectResponseMessage(int portId,
			CDAPMessage::Flags flags, std::string objClass, long objInst,
			std::string objName, ObjectValueInterface &objValue, int result,
			std::string resultReason, int invokeId) throw (CDAPException);
	/// Create a M_WRITE CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param objClass
	/// @param objInst
	/// @param objValue
	/// @param objName
	/// @param scope
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	CDAPMessage* getWriteObjectRequestMessage(int portId, char filter[],
			CDAPMessage::Flags flags, std::string objClass, long objInst,
			ObjectValueInterface &objValue, std::string objName, int scope,
			bool invokeId) throw (CDAPException);
	/// Create a M_WRITE_R CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param resultReason
	/// @param invokeId
	/// @return
	/// @throws CDAPException
	CDAPMessage* getWriteObjectResponseMessage(int portId,
			CDAPMessage::Flags flags, int result, std::string resultReason,
			int invokeId) throw (CDAPException);
	/// Create a M_CANCELREAD CDAP Message
	/// @param portId identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param invokeID
	/// @return
	/// @throws CDAPException
	CDAPMessage* getCancelReadRequestMessage(int portId,
			CDAPMessage::Flags flags, int invokeID) throw (CDAPException);
	 /// Create a M_CANCELREAD_R CDAP Message
	 /// @param portId identifies the CDAP Session that this message is part of
	 /// @param flags
	 /// @param invokeID
	 /// @param result
	 /// @param resultReason
	 /// @return
	 /// @throws CDAPException
	CDAPMessage* getCancelReadResponseMessage(int portId,
			CDAPMessage::Flags flags, int invokeID, int result,
			std::string resultReason) throw (CDAPException);
private:
	WireMessageProviderFactoryInterface* wire_message_provider_factory_;
	std::map<int, CDAPSession*> cdap_sessions_;
	/// Used by the serialize and unserialize operations
	WireMessageProviderInterface& wire_message_provider_;
	/// The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	long timeout_;
	void assignInvokeId(CDAPMessage cdap_message, bool invoke_id, int port_id);
};
}
#endif
#endif // CDAP_H_
