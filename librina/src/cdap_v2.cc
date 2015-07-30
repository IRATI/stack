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

#define RINA_PREFIX "cdap"
#include "librina/logs.h"
#include "librina/cdap.h"
#include "librina/application.h"
#include "librina/ipc-process.h"
#include "librina/timer.h"
#include "librina/cdap_v2.h"

#include <algorithm>
#include "CDAP.pb.h"

namespace rina {
namespace cdap {

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

class CDAPMessageFactory
{
 public:
	static cdap_m_t* getOpenConnectionRequestMessage(
			const cdap_rib::con_handle_t &con, int invoke_id);
	static cdap_m_t* getOpenConnectionResponseMessage(
			const cdap_rib::con_handle_t &con,
			const cdap_rib::res_info_t &res, int invoke_id);
	static cdap_m_t* getReleaseConnectionRequestMessage(
			const cdap_rib::flags_t &flags);
	static cdap_m_t* getReleaseConnectionResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::res_info_t &res, int invoke_id);
	static cdap_m_t* getCreateObjectRequestMessage(
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj);
	static cdap_m_t* getCreateObjectResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res, int invoke_id);
	static cdap_m_t* getDeleteObjectRequestMessage(
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj);
	static cdap_m_t* getDeleteObjectResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res, int invoke_id);
	static cdap_m_t* getStartObjectRequestMessage(
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj);
	static cdap_m_t* getStartObjectResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::res_info_t &res, int invoke_id);
	static cdap_m_t* getStartObjectResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res, int invoke_id);
	static cdap_m_t* getStopObjectRequestMessage(
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj);
	static cdap_m_t* getStopObjectResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::res_info_t &res, int invoke_id);
	static cdap_m_t* getReadObjectRequestMessage(
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj);
	static cdap_m_t* getReadObjectResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res, int invoke_id);
	static cdap_m_t* getWriteObjectRequestMessage(
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj);
	static cdap_m_t* getWriteObjectResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::res_info_t &res, int invoke_id);
	static cdap_m_t* getCancelReadRequestMessage(
			const cdap_rib::flags_t &flags, int invoke_id);
	static cdap_m_t* getCancelReadResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::res_info_t &res, int invoke_id);
 private:
	static const int ABSTRACT_SYNTAX_VERSION;
	;
};

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
	/// Checks if a the CDAP connection can be opened (i.e. an M_CONNECT message can be sent)
	/// @throws rina::CDAPException
	void checkConnect();
	void connectSentOrReceived(bool sent);
	/// Checks if the CDAP M_CONNECT_R message can be sent
	/// @throws rina::CDAPException
	void checkConnectResponse();
	void connectResponseSentOrReceived(bool sent);
	/// Checks if the CDAP M_RELEASE message can be sent
	/// @throws rina::CDAPException
	void checkRelease();
	void releaseSentOrReceived(const cdap_m_t &cdap_message, bool sent);
	/// Checks if the CDAP M_RELEASE_R message can be sent
	/// @throws rina::CDAPException
	void checkReleaseResponse();
	void releaseResponseSentOrReceived(bool sent);
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
	/// @throws rina::CDAPException
	void connect();
	/// An M_CONNECT message has been received, update the state
	/// @param message
	void connectReceived();
	/// The AE has sent an M_CONNECT_R  message
	/// @param openConnectionResponseMessage
	/// @throws rina::CDAPException
	void connectResponse();
	/// An M_CONNECT_R message has been received
	/// @param message
	void connectResponseReceived();
	/// The AE has sent an M_RELEASE message
	/// @param releaseConnectionRequestMessage
	/// @throws rina::CDAPException
	void release(const cdap_m_t &cdap_m_t);
	/// An M_RELEASE message has been received
	/// @param message
	void releaseReceived(const cdap_m_t &message);
	/// The AE has called the close response operation
	/// @param releaseConnectionRequestMessage
	/// @throws rina::CDAPException
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
	rina::Timer *open_timer_;
	rina::Timer *close_timer_;
	friend class ResetStablishmentTimerTask;
	friend class ReleaseConnectionTimerTask;
};

/// It will always try to use short invokeIds (as close to 1 as possible)
class CDAPInvokeIdManagerImpl : rina::Lockable
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
	static void validate(const cdap_m_t *message);
 private:
	static void validateAbsSyntax(const cdap_m_t *message);
	static void validateDestAEInst(const cdap_m_t *message);
	static void validateDestAEName(const cdap_m_t *message);
	static void validateDestApInst(const cdap_m_t *message);
	static void validateDestApName(const cdap_m_t *message);
	static void validateFilter(const cdap_m_t *message);
	static void validateInvokeID(const cdap_m_t *message);
	static void validateObjClass(const cdap_m_t *message);
	static void validateObjInst(const cdap_m_t *message);
	static void validateObjName(const cdap_m_t *message);
	static void validateObjValue(const cdap_m_t *message);
	static void validateOpcode(const cdap_m_t *message);
	static void validateResult(const cdap_m_t *message);
	static void validateResultReason(const cdap_m_t *message);
	static void validateScope(const cdap_m_t *message);
	static void validateSrcAEInst(const cdap_m_t *message);
	static void validateSrcAEName(const cdap_m_t *message);
	static void validateSrcApInst(const cdap_m_t *message);
	static void validateSrcApName(const cdap_m_t *message);
	static void validateVersion(const cdap_m_t *message);
};

///Describes a CDAPSession, by identifying the source and destination application processes.
///Note that "source" and "destination" are just placeholders, as both parties in a CDAP exchange have
///the same role, because the CDAP session is bidirectional.
typedef struct CDAPSessionDescriptor
{
 public:
	/// AbstractSyntaxID (int32), mandatory. The specific version of the
	/// CDAP protocol message declarations that the message conforms to
	///
	int abs_syntax_;
	/// Authentication Policy options
	rina::AuthPolicy auth_policy_;
	/// DestinationApplication-Entity-Instance-Id (string), optional, not validated by CDAP.
	/// Specific instance of the Application Entity that the source application
	/// wishes to connect to in the destination application.
	std::string dest_ae_inst_;
	/// DestinationApplication-Entity-Name (string), mandatory (optional for the response).
	/// Name of the Application Entity that the source application wishes
	/// to connect to in the destination application.
	std::string dest_ae_name_;
	/// DestinationApplication-Process-Instance-Id (string), optional, not validated by CDAP.
	/// Name of the Application Process Instance that the source wishes to
	/// connect to a the destination.
	std::string dest_ap_inst_;
	/// DestinationApplication-Process-Name (string), mandatory (optional for the response).
	/// Name of the application process that the source application wishes to connect to
	/// in the destination application
	std::string dest_ap_name_;
	/// SourceApplication-Entity-Instance-Id (string).
	/// AE instance within the application originating the message
	std::string src_ae_inst_;
	/// SourceApplication-Entity-Name (string).
	/// Name of the AE within the application originating the message
	std::string src_ae_name_;
	/// SourceApplication-Process-Instance-Id (string), optional, not validated by CDAP.
	/// Application instance originating the message
	std::string src_ap_inst_;
	/// SourceApplicatio-Process-Name (string), mandatory (optional in the response).
	/// Name of the application originating the message
	std::string src_ap_name_;
	/// Version (int32). Mandatory in connect request and response, optional otherwise.
	/// Version of RIB and object set_ to use in the conversation. Note that the
	/// abstract syntax refers to the CDAP message syntax, while version refers to the
	/// version of the AE RIB objects, their values, vocabulary of object id's, and
	/// related behaviors that are subject to change over time. See text for details
	/// of use.
	long version_;
	/// Uniquely identifies this CDAP session in this IPC process. It matches the port_id
	/// of the (N-1) flow that supports the CDAP Session
	int port_id_;

	rina::ApplicationProcessNamingInformation ap_naming_info_;
} cdap_session_t;

/// Implements a CDAP session. Has the necessary logic to ensure that a
/// the operation of the CDAP protocol is consistent (correct states and proper
/// invocation of operations)
class CDAPSessionManager;
class CDAPSession
{
 public:
	CDAPSession(CDAPSessionManager *cdap_session_manager, long timeout,
		    SerializerInterface *serializer,
		    CDAPInvokeIdManagerImpl *invoke_id_manager);
	~CDAPSession() throw ();
	//const CDAPSessionInvokeIdManagerInterface* getInvokeIdManager();
	//bool isConnected() const;
	const cdap_rib::SerializedObject* encodeNextMessageToBeSent(
			const cdap_m_t &cdap_message);
	void messageSent(const cdap_m_t &cdap_message);
	const cdap_m_t* messageReceived(
			const cdap_rib::SerializedObject &message);
	void messageReceived(const cdap_m_t &cdap_message);
	void set_session_descriptor(cdap_session_t *session_descriptor);
	int get_port_id() const;
	cdap_session_t* get_session_descriptor() const;
	CDAPInvokeIdManagerImpl* get_invoke_id_manager() const;
	void stopConnection();
 private:
	void messageSentOrReceived(const cdap_m_t &cdap_message, bool sent);
	void freeOrReserveInvokeId(const cdap_m_t &cdap_message, bool sent);
	void checkIsConnected() const;
	void checkInvokeIdNotExists(const cdap_m_t &cdap_message,
				    bool sent) const;
	void checkCanSendOrReceiveCancelReadRequest(
			const cdap_m_t &cdap_message, bool sent) const;
	void requestMessageSentOrReceived(const cdap_m_t &cdap_message,
					  cdap_m_t::Opcode op_code, bool sent);
	void cancelReadMessageSentOrReceived(const cdap_m_t &cdap_message,
					     bool sender);
	void checkCanSendOrReceiveResponse(const cdap_m_t &cdap_message,
					   cdap_m_t::Opcode op_code,
					   bool sender) const;
	void checkCanSendOrReceiveCancelReadResponse(
			const cdap_m_t &cdap_message, bool send) const;
	void responseMessageSentOrReceived(const cdap_m_t &cdap_message,
					   cdap_m_t::Opcode op_code, bool sent);
	void cancelReadResponseMessageSentOrReceived(
			const cdap_m_t &cdap_message, bool sent);
	const cdap_rib::ser_obj_t* serializeMessage(
			const cdap_m_t &cdap_message) const;
	const cdap_m_t* deserializeMessage(
			const cdap_rib::SerializedObject &message) const;
	void populateSessionDescriptor(const cdap_m_t &cdap_message, bool send);
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
	SerializerInterface *serializer_;
	cdap_session_t *session_descriptor_;
	CDAPSessionManager *cdap_session_manager_;
	CDAPInvokeIdManagerImpl *invoke_id_manager_;
};

/// Implements a CDAP session manager.
class CDAPSessionManager
{
 public:
	static const long DEFAULT_TIMEOUT_IN_MS = 10000;
	CDAPSessionManager();
	CDAPSessionManager(SerializerInterface *serializer_factory);
	CDAPSessionManager(SerializerInterface *serializer_factory,
			   long timeout);
	~CDAPSessionManager() throw ();
	void set_timeout(long timeout);
	CDAPSession* createCDAPSession(int port_id);
	void getAllCDAPSessionIds(std::vector<int> &vector);
	CDAPSession* get_cdap_session(int port_id);
	const cdap_rib::SerializedObject* encodeCDAPMessage(
			const cdap_m_t &cdap_message);
	const cdap_m_t* decodeCDAPMessage(
			const cdap_rib::SerializedObject &cdap_message);
	void removeCDAPSession(int portId);
	const cdap_rib::SerializedObject* encodeNextMessageToBeSent(
			const cdap_m_t &cdap_message, int port_id);
	const cdap_m_t* messageReceived(
			const cdap_rib::SerializedObject &encodedcdap_m_t,
			int portId);
	void messageSent(const cdap_m_t &cdap_message, int port_id);
	int get_port_id(std::string destination_application_process_name);
	cdap_m_t* getOpenConnectionRequestMessage(
			const cdap_rib::con_handle_t &con);
	cdap_m_t* getOpenConnectionResponseMessage(
			const cdap_rib::con_handle_t &con,
			const cdap_rib::res_info_t &res, int invoke_id);
	cdap_m_t* getReleaseConnectionRequestMessage(
			int port_id, const cdap_rib::flags_t &flags,
			bool invoke_id);
	cdap_m_t* getReleaseConnectionResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::res_info_t &res, int invoke_id);
	cdap_m_t* getCreateObjectRequestMessage(
			int port_id, const cdap_rib::filt_info_t &filt,
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj, bool invoke_id);
	cdap_m_t* getCreateObjectResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res, int invoke_id);
	cdap_m_t* getDeleteObjectRequestMessage(
			int port_id, const cdap_rib::filt_info_t &filt,
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj, bool invoke_id);
	cdap_m_t* getDeleteObjectResponseMessage(
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res, int invoke_id);
	cdap_m_t* getStartObjectRequestMessage(
			int port_id, const cdap_rib::filt_info_t &filt,
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj, bool invoke_id);
	cdap_m_t* getStartObjectResponseMessage(const cdap_rib::flags_t &flags,
						const cdap_rib::res_info_t &res,
						int invoke_id);
	cdap_m_t* getStartObjectResponseMessage(const cdap_rib::flags_t &flags,
						const cdap_rib::obj_info_t &obj,
						const cdap_rib::res_info_t &res,
						int invoke_id);
	cdap_m_t* getStopObjectRequestMessage(int port_id,
					      const cdap_rib::filt_info_t &filt,
					      const cdap_rib::flags_t &flags,
					      const cdap_rib::obj_info_t &obj,
					      bool invoke_id);
	cdap_m_t* getStopObjectResponseMessage(const cdap_rib::flags_t &flags,
					       const cdap_rib::res_info_t &res,
					       int invoke_id);
	cdap_m_t* getReadObjectRequestMessage(int port_id,
					      const cdap_rib::filt_info_t &filt,
					      const cdap_rib::flags_t &flags,
					      const cdap_rib::obj_info_t &obj,
					      bool invoke_id);
	cdap_m_t* getReadObjectResponseMessage(const cdap_rib::flags_t &flags,
					       const cdap_rib::obj_info_t &obj,
					       const cdap_rib::res_info_t &res,
					       int invoke_id);
	cdap_m_t* getWriteObjectRequestMessage(
			int port_id, const cdap_rib::filt_info_t &filt,
			const cdap_rib::flags_t &flags,
			const cdap_rib::obj_info_t &obj, bool invoke_id);
	cdap_m_t* getWriteObjectResponseMessage(const cdap_rib::flags_t &flags,
						const cdap_rib::res_info_t &res,
						int invoke_id);
	cdap_m_t* getCancelReadRequestMessage(const cdap_rib::flags_t &flags,
					      int invoke_id);
	cdap_m_t* getCancelReadResponseMessage(const cdap_rib::flags_t &flags,
					       const cdap_rib::res_info_t &res,
					       int invoke_id);
 private:
	void assignInvokeId(cdap_m_t &cdap_message, bool invoke_id, int port_id,
			    bool sent);
	std::map<int, CDAPSession*> cdap_sessions_;
	/// Used by the serialize and unserialize operations
	SerializerInterface *serializer_;
	/// The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	long timeout_;
	CDAPInvokeIdManagerImpl *invoke_id_manager_;
};

class CDAPProvider : public CDAPProviderInterface
{
 public:
	CDAPProvider(cdap::CDAPCallbackInterface *callback,
		     CDAPSessionManager *manager);
	~CDAPProvider();

	//Remote

	cdap_rib::con_handle_t remote_open_connection(
			const cdap_rib::vers_info_t &ver,
			const cdap_rib::ep_info_t &src,
			const cdap_rib::ep_info_t &dest,
			const cdap_rib::auth_policy_t &auth, int port);
	int remote_close_connection(unsigned int port);
	int remote_create(unsigned int port, const cdap_rib::obj_info_t &obj,
			  const cdap_rib::flags_t &flags,
			  const cdap_rib::filt_info_t &filt);
	int remote_delete(unsigned int port, const cdap_rib::obj_info_t &obj,
			  const cdap_rib::flags_t &flags,
			  const cdap_rib::filt_info_t &filt);
	int remote_read(unsigned int port, const cdap_rib::obj_info_t &obj,
			const cdap_rib::flags_t &flags,
			const cdap_rib::filt_info_t &filt);
	int remote_cancel_read(unsigned int port,
			       const cdap_rib::flags_t &flags, int invoke_id);
	int remote_write(unsigned int port, const cdap_rib::obj_info_t &obj,
			 const cdap_rib::flags_t &flags,
			 const cdap_rib::filt_info_t &filt);
	int remote_start(unsigned int port, const cdap_rib::obj_info_t &obj,
			 const cdap_rib::flags_t &flags,
			 const cdap_rib::filt_info_t &filt);
	int remote_stop(unsigned int port, const cdap_rib::obj_info_t &obj,
			const cdap_rib::flags_t &flags,
			const cdap_rib::filt_info_t &filt);

	//Local

	void send_open_connection_result(const cdap_rib::con_handle_t &con,
				      const cdap_rib::res_info_t &res,
				      int invoke_id);
	void send_close_connection_result(unsigned int port,
				       const cdap_rib::flags_t &flags,
				       const cdap_rib::res_info_t &res,
				       int invoke_id);
	void send_create_result(unsigned int port,
				    const cdap_rib::obj_info_t &obj,
				    const cdap_rib::flags_t &flags,
				    const cdap_rib::res_info_t &res,
				    int invoke_id);
	void send_delete_result(unsigned int port,
				    const cdap_rib::obj_info_t &obj,
				    const cdap_rib::flags_t &flags,
				    const cdap_rib::res_info_t &res,
				    int invoke_id);
	void send_read_result(unsigned int port,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::flags_t &flags,
				  const cdap_rib::res_info_t &res,
				  int invoke_id);
	void send_cancel_read_result(unsigned int port,
					 const cdap_rib::flags_t &flags,
					 const cdap_rib::res_info_t &res,
					 int invoke_id);
	void send_write_result(unsigned int port,
				   const cdap_rib::flags_t &flags,
				   const cdap_rib::res_info_t &res,
				   int invoke_id);
	void send_start_result(unsigned int port,
				   const cdap_rib::obj_info_t &obj,
				   const cdap_rib::flags_t &flags,
				   const cdap_rib::res_info_t &res,
				   int invoke_id);
	void send_stop_result(unsigned int port,
				  const cdap_rib::flags_t &flags,
				  const cdap_rib::res_info_t &res,
				  int invoke_id);

	// Process and incoming CDAP message

	void process_message(cdap_rib::SerializedObject &message,
			     unsigned int port);
 protected:
	CDAPSessionManager *manager_;
	cdap::CDAPCallbackInterface *callback_;
 private:
	virtual void send(const cdap_m_t *m_sent, int port) = 0;
};

class AppCDAPProvider : public CDAPProvider
{
 public:
	AppCDAPProvider(cdap::CDAPCallbackInterface *callback,
			CDAPSessionManager *manager);
 private:
	void send(const cdap_m_t *m_sent, int port);
};

class IPCPCDAPProvider : public CDAPProvider
{
 public:
	IPCPCDAPProvider(cdap::CDAPCallbackInterface *callback,
			 CDAPSessionManager *manager);
 private:
	void send(const cdap_m_t *m_sent, int port);
};

/// Google Protocol Buffers Wire Message Provider
class GPBSerializer : public SerializerInterface
{
 public:
	const cdap_m_t* deserializeMessage(
			const cdap_rib::SerializedObject &message);
	const cdap_rib::SerializedObject* serializeMessage(
			const cdap_m_t &cdapMessage);
};

// CLASS CDAPMessageFactory
cdap_m_t* CDAPMessageFactory::getOpenConnectionRequestMessage(
		const cdap_rib::con_handle_t &con, int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->abs_syntax_ = ABSTRACT_SYNTAX_VERSION;
	cdap_message->op_code_ = cdap_m_t::M_CONNECT;
	AuthPolicy auth_policy;
	auth_policy.name_ = con.auth_.name;
	auth_policy.versions_ = con.auth_.versions;
	if (con.auth_.options.size_ > 0) {
		char * val = new char[con.auth_.options.size_];
		memcpy(val, con.auth_.options.message_,
				con.auth_.options.size_);
		auth_policy.options_ = SerializedObject(val,
				con.auth_.options.size_);
	}
	cdap_message->auth_policy_ = auth_policy;
	cdap_message->dest_ae_inst_ = con.dest_.ae_inst_;
	cdap_message->dest_ae_name_ = con.dest_.ae_name_;
	cdap_message->dest_ap_inst_ = con.dest_.ap_inst_;
	cdap_message->dest_ap_name_ = con.dest_.ap_name_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->src_ae_inst_ = con.src_.ae_inst_;
	cdap_message->src_ae_name_ = con.src_.ae_name_;
	cdap_message->src_ap_inst_ = con.src_.ap_inst_;
	cdap_message->src_ap_name_ = con.src_.ap_name_;
	cdap_message->version_ = 1;
	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getOpenConnectionResponseMessage(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res, int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->abs_syntax_ = ABSTRACT_SYNTAX_VERSION;
	cdap_message->op_code_ = cdap_m_t::M_CONNECT_R;
	AuthPolicy auth_policy;
	if (con.auth_.options.size_ > 0) {
		char * val = new char[con.auth_.options.size_];
		memcpy(val, con.auth_.options.message_,
				con.auth_.options.size_);
		auth_policy.options_ = SerializedObject(val,
				con.auth_.options.size_);
	}
	cdap_message->auth_policy_ = auth_policy;
	cdap_message->dest_ae_inst_ = con.dest_.ae_inst_;
	cdap_message->dest_ae_name_ = con.dest_.ae_name_;
	cdap_message->dest_ap_inst_ = con.dest_.ap_inst_;
	cdap_message->dest_ap_name_ = con.dest_.ap_name_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->src_ae_inst_ = con.src_.ae_inst_;
	cdap_message->src_ae_name_ = con.src_.ae_name_;
	cdap_message->src_ap_inst_ = con.src_.ap_inst_;
	cdap_message->src_ap_name_ = con.src_.ap_name_;
	cdap_message->version_ = 1;
	cdap_message->result_ = res.code_;
	cdap_message->result_reason_ = res.reason_;
	return cdap_message;
}

cdap_m_t* CDAPMessageFactory::getReleaseConnectionRequestMessage(
		const cdap_rib::flags_t &flags)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_RELEASE;
	cdap_message->flags_ = flags.flags_;
	return cdap_message;
}

cdap_m_t* CDAPMessageFactory::getReleaseConnectionResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::res_info_t &res,
		int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_RELEASE_R;
	cdap_message->flags_ = flags.flags_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->result_ = res.code_;
	cdap_message->result_reason_ = res.reason_;
	return cdap_message;
}

cdap_m_t* CDAPMessageFactory::getCreateObjectRequestMessage(
		const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_CREATE;
	cdap_message->filter_ = filt.filter_;
	cdap_message->scope_ = filt.scope_;
	cdap_message->flags_ = flags.flags_;
	cdap_message->obj_class_ = obj.class_;
	cdap_message->obj_inst_ = obj.inst_;
	cdap_message->obj_name_ = obj.name_;
	cdap_message->obj_value_ = obj.value_;

	return cdap_message;
}

cdap_m_t* CDAPMessageFactory::getCreateObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res, int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_CREATE_R;
	cdap_message->flags_ = flags.flags_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->obj_class_ = obj.class_;
	cdap_message->obj_inst_ = obj.inst_;
	cdap_message->obj_name_ = obj.name_;
	cdap_message->obj_value_ = obj.value_;
	cdap_message->result_ = res.code_;
	cdap_message->result_reason_ = res.reason_;

	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getDeleteObjectRequestMessage(
		const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_DELETE;
	cdap_message->filter_ = filt.filter_;
	cdap_message->scope_ = filt.scope_;
	cdap_message->flags_ = flags.flags_;
	cdap_message->obj_class_ = obj.class_;
	cdap_message->obj_inst_ = obj.inst_;
	cdap_message->obj_name_ = obj.name_;

	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getDeleteObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res, int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_DELETE_R;
	cdap_message->flags_ = flags.flags_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->obj_class_ = obj.class_;
	cdap_message->obj_inst_ = obj.inst_;
	cdap_message->obj_name_ = obj.name_;
	cdap_message->result_ = res.code_;
	cdap_message->result_reason_ = res.reason_;
	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getStartObjectRequestMessage(
		const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_START;
	cdap_message->filter_ = filt.filter_;
	cdap_message->scope_ = filt.scope_;
	cdap_message->flags_ = flags.flags_;
	cdap_message->obj_class_ = obj.class_;
	cdap_message->obj_inst_ = obj.inst_;
	cdap_message->obj_name_ = obj.name_;
	cdap_message->obj_value_ = obj.value_;
	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getStartObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::res_info_t &res,
		int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_START_R;
	cdap_message->flags_ = flags.flags_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->result_ = res.code_;
	cdap_message->result_reason_ = res.reason_;
	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getStartObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res, int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_START_R;
	cdap_message->flags_ = flags.flags_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->result_ = res.code_;
	cdap_message->result_reason_ = res.reason_;
	cdap_message->obj_class_ = obj.class_;
	cdap_message->obj_inst_ = obj.inst_;
	cdap_message->obj_name_ = obj.name_;
	cdap_message->obj_value_ = obj.value_;
	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getStopObjectRequestMessage(
		const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_STOP;
	cdap_message->filter_ = filt.filter_;
	cdap_message->scope_ = filt.scope_;
	cdap_message->flags_ = flags.flags_;
	cdap_message->obj_class_ = obj.class_;
	cdap_message->obj_inst_ = obj.inst_;
	cdap_message->obj_name_ = obj.name_;
	cdap_message->obj_value_ = obj.value_;
	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getStopObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::res_info_t &res,
		int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_STOP_R;
	cdap_message->flags_ = flags.flags_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->result_ = res.code_;
	cdap_message->result_reason_ = res.reason_;
	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getReadObjectRequestMessage(
		const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_READ;
	cdap_message->filter_ = filt.filter_;
	cdap_message->scope_ = filt.scope_;
	cdap_message->flags_ = flags.flags_;
	cdap_message->obj_class_ = obj.class_;
	cdap_message->obj_inst_ = obj.inst_;
	cdap_message->obj_name_ = obj.name_;
	return cdap_message;
}

cdap_m_t* CDAPMessageFactory::getReadObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res, int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_READ_R;
	cdap_message->flags_ = flags.flags_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->obj_class_ = obj.class_;
	cdap_message->obj_inst_ = obj.inst_;
	cdap_message->obj_name_ = obj.name_;
	cdap_message->obj_value_ = obj.value_;
	cdap_message->result_ = res.code_;
	cdap_message->result_reason_ = res.reason_;
	return cdap_message;
}

cdap_m_t* CDAPMessageFactory::getWriteObjectRequestMessage(
		const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_WRITE;
	cdap_message->filter_ = filt.filter_;
	cdap_message->scope_ = filt.scope_;
	cdap_message->flags_ = flags.flags_;
	cdap_message->obj_class_ = obj.class_;
	cdap_message->obj_inst_ = obj.inst_;
	cdap_message->obj_name_ = obj.name_;
	cdap_message->obj_value_ = obj.value_;
	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getWriteObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::res_info_t &res,
		int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_WRITE_R;
	cdap_message->flags_ = flags.flags_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->result_ = res.code_;
	cdap_message->result_reason_ = res.reason_;
	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getCancelReadRequestMessage(
		const cdap_rib::flags_t &flags, int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_CANCELREAD;
	cdap_message->flags_ = flags.flags_;
	cdap_message->invoke_id_ = invoke_id;
	return cdap_message;
}
cdap_m_t* CDAPMessageFactory::getCancelReadResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::res_info_t &res,
		int invoke_id)
{
	cdap_m_t *cdap_message = new cdap_m_t();
	cdap_message->op_code_ = cdap_m_t::M_CANCELREAD_R;
	cdap_message->flags_ = flags.flags_;
	cdap_message->invoke_id_ = invoke_id;
	cdap_message->result_ = res.code_;
	cdap_message->result_reason_ = res.reason_;
	return cdap_message;
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
	lock();
	connection_state_ = NONE;
	unlock();
	open_timer_ = 0;
	close_timer_ = 0;
}
ConnectionStateMachine::~ConnectionStateMachine() throw ()
{
	delete open_timer_;
	open_timer_ = 0;
	delete close_timer_;
	close_timer_ = 0;
}
bool ConnectionStateMachine::is_connected() const
{
	return connection_state_ == CONNECTED;
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
		throw rina::CDAPException(ss.str());
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
		throw rina::CDAPException(ss.str());
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
		throw rina::CDAPException(ss.str());
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
		throw rina::CDAPException(ss.str());
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
	open_timer_ = new rina::Timer();
	ResetStablishmentTimerTask *reset = new ResetStablishmentTimerTask(
			this);
	open_timer_->scheduleTask(reset, timeout_);
}
void ConnectionStateMachine::connectReceived()
{
	lock();
	if (connection_state_ != NONE) {
		std::stringstream ss;
		ss << "Cannot open a new connection because this CDAP session is currently in"
		   << connection_state_ << " state";
		unlock();
		throw rina::CDAPException(ss.str());
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
		throw rina::CDAPException(ss.str());
	}
	LOG_DBG("Connection response received");
	delete open_timer_;
	open_timer_ = 0;
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
		close_timer_ = new rina::Timer();
		ReleaseConnectionTimerTask *reset =
				new ReleaseConnectionTimerTask(this);
		LOG_DBG("Waiting timeout %d to receive a release response",
			timeout_);
		close_timer_->scheduleTask(reset, timeout_);
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
		throw rina::CDAPException(ss.str());
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
		throw rina::CDAPException(ss.str());
	}
	LOG_DBG("Release response received");
	delete close_timer_;
	close_timer_ = 0;
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
void CDAPMessageValidator::validate(const cdap_m_t *message)
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

void CDAPMessageValidator::validateAbsSyntax(const cdap_m_t *message)
{
	if (message->abs_syntax_ == 0) {
		if (message->op_code_ == cdap_m_t::M_CONNECT
				|| message->op_code_ == cdap_m_t::M_CONNECT_R) {
			throw rina::CDAPException(
					"AbsSyntax must be set for M_CONNECT and M_CONNECT_R messages");
		}
	} else {
		if ((message->op_code_ != cdap_m_t::M_CONNECT)
				&& (message->op_code_ != cdap_m_t::M_CONNECT_R)) {
			throw rina::CDAPException(
					"AbsSyntax can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestAEInst(const cdap_m_t *message)
{
	if (!message->dest_ae_inst_.empty()) {
		if ((message->op_code_ != cdap_m_t::M_CONNECT)
				&& (message->op_code_ != cdap_m_t::M_CONNECT_R)) {
			throw rina::CDAPException(
					"dest_ae_inst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestAEName(const cdap_m_t *message)
{
	if (!message->dest_ae_name_.empty()) {
		if ((message->op_code_ != cdap_m_t::M_CONNECT)
				&& (message->op_code_ != cdap_m_t::M_CONNECT_R)) {
			throw rina::CDAPException(
					"DestAEName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestApInst(const cdap_m_t *message)
{
	if (!message->dest_ap_inst_.empty()) {
		if (message->op_code_ != cdap_m_t::M_CONNECT
				&& message->op_code_ != cdap_m_t::M_CONNECT_R) {
			throw rina::CDAPException(
					"DestApInst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestApName(const cdap_m_t *message)
{
	if (message->dest_ap_name_.empty()) {
		if (message->op_code_ == cdap_m_t::M_CONNECT) {
			throw rina::CDAPException(
					"DestApName must be set for the M_CONNECT message");
		} else if (message->op_code_ == cdap_m_t::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message->op_code_ != cdap_m_t::M_CONNECT
				&& message->op_code_ != cdap_m_t::M_CONNECT_R) {
			throw rina::CDAPException(
					"DestApName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateFilter(const cdap_m_t *message)
{
	if (message->filter_ != 0) {
		if (message->op_code_ != cdap_m_t::M_CREATE
				&& message->op_code_ != cdap_m_t::M_DELETE
				&& message->op_code_ != cdap_m_t::M_READ
				&& message->op_code_ != cdap_m_t::M_WRITE
				&& message->op_code_ != cdap_m_t::M_START
				&& message->op_code_ != cdap_m_t::M_STOP) {
			throw rina::CDAPException(
					"The filter parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages");
		}
	}
}

void CDAPMessageValidator::validateInvokeID(const cdap_m_t *message)
{
	if (message->invoke_id_ == 0) {
		if (message->op_code_ == cdap_m_t::M_CONNECT
				|| message->op_code_ == cdap_m_t::M_CONNECT_R
				|| message->op_code_ == cdap_m_t::M_RELEASE_R
				|| message->op_code_ == cdap_m_t::M_CREATE_R
				|| message->op_code_ == cdap_m_t::M_DELETE_R
				|| message->op_code_ == cdap_m_t::M_READ_R
				|| message->op_code_ == cdap_m_t::M_CANCELREAD
				|| message->op_code_ == cdap_m_t::M_CANCELREAD_R
				|| message->op_code_ == cdap_m_t::M_WRITE_R
				|| message->op_code_ == cdap_m_t::M_START_R
				|| message->op_code_ == cdap_m_t::M_STOP_R) {
			throw rina::CDAPException(
					"The invoke id parameter cannot be 0");
		}
	}
}

void CDAPMessageValidator::validateObjClass(const cdap_m_t *message)
{
	if (!message->obj_class_.empty()) {
		if (message->obj_name_.empty()) {
			throw rina::CDAPException(
					"If the objClass parameter is set, the objName parameter also has to be set");
		}
		if (message->op_code_ != cdap_m_t::M_CREATE
				&& message->op_code_ != cdap_m_t::M_CREATE_R
				&& message->op_code_ != cdap_m_t::M_DELETE
				&& message->op_code_ != cdap_m_t::M_DELETE_R
				&& message->op_code_ != cdap_m_t::M_READ
				&& message->op_code_ != cdap_m_t::M_READ_R
				&& message->op_code_ != cdap_m_t::M_WRITE
				&& message->op_code_ != cdap_m_t::M_WRITE_R
				&& message->op_code_ != cdap_m_t::M_START
				&& message->op_code_ != cdap_m_t::M_STOP
				&& message->op_code_ != cdap_m_t::M_START_R
				&& message->op_code_ != cdap_m_t::M_STOP_R) {
			throw rina::CDAPException(
					"The objClass parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R, M_STOP_R messages");
		}
	}
}

void CDAPMessageValidator::validateObjInst(const cdap_m_t *message)
{
	if (message->obj_inst_ != 0) {
		if (message->op_code_ != cdap_m_t::M_CREATE
				&& message->op_code_ != cdap_m_t::M_CREATE_R
				&& message->op_code_ != cdap_m_t::M_DELETE
				&& message->op_code_ != cdap_m_t::M_DELETE_R
				&& message->op_code_ != cdap_m_t::M_READ
				&& message->op_code_ != cdap_m_t::M_READ_R
				&& message->op_code_ != cdap_m_t::M_WRITE
				&& message->op_code_ != cdap_m_t::M_WRITE_R
				&& message->op_code_ != cdap_m_t::M_START
				&& message->op_code_ != cdap_m_t::M_STOP
				&& message->op_code_ != cdap_m_t::M_START_R
				&& message->op_code_ != cdap_m_t::M_STOP_R) {
			throw rina::CDAPException(
					"The objInst parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_START_R, M_STOP and M_STOP_R messages");
		}
	}
}

void CDAPMessageValidator::validateObjName(const cdap_m_t *message)
{
	if (!message->obj_name_.empty()) {
		if (message->obj_class_.empty()) {
			throw new rina::CDAPException(
					"If the objName parameter is set, the objClass parameter also has to be set");
		}
		if (message->op_code_ != cdap_m_t::M_CREATE
				&& message->op_code_ != cdap_m_t::M_CREATE_R
				&& message->op_code_ != cdap_m_t::M_DELETE
				&& message->op_code_ != cdap_m_t::M_DELETE_R
				&& message->op_code_ != cdap_m_t::M_READ
				&& message->op_code_ != cdap_m_t::M_READ_R
				&& message->op_code_ != cdap_m_t::M_WRITE
				&& message->op_code_ != cdap_m_t::M_WRITE_R
				&& message->op_code_ != cdap_m_t::M_START
				&& message->op_code_ != cdap_m_t::M_STOP
				&& message->op_code_ != cdap_m_t::M_START_R
				&& message->op_code_ != cdap_m_t::M_STOP_R) {
			throw rina::CDAPException(
					"The objName parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R and M_STOP_R messages");
		}
	}
}

void CDAPMessageValidator::validateObjValue(const cdap_m_t *message)
{
	if (message->obj_value_.size_ == 0) {
		if (message->op_code_ == cdap_m_t::M_WRITE) {
			throw rina::CDAPException(
					"The objValue parameter must be set for M_WRITE messages");
		}
	} else {
		if (message->op_code_ != cdap_m_t::M_CREATE
				&& message->op_code_ != cdap_m_t::M_CREATE_R
				&& message->op_code_ != cdap_m_t::M_READ_R
				&& message->op_code_ != cdap_m_t::M_WRITE
				&& message->op_code_ != cdap_m_t::M_START
				&& message->op_code_ != cdap_m_t::M_STOP
				&& message->op_code_ != cdap_m_t::M_START_R
				&& message->op_code_ != cdap_m_t::M_STOP_R
				&& message->op_code_ != cdap_m_t::M_WRITE_R
				&& message->op_code_ != cdap_m_t::M_DELETE
				&& message->op_code_ != cdap_m_t::M_READ) {
			throw rina::CDAPException(
					"The objValue parameter can only be set for M_CREATE, M_DELETE, M_CREATE_R, M_READ_R, M_WRITE, M_START, M_START_R, M_STOP, M_STOP_R and M_WRITE_R messages");
		}
	}
}

void CDAPMessageValidator::validateOpcode(const cdap_m_t *message)
{
	if (message->op_code_ == cdap_m_t::NONE_OPCODE) {
		throw rina::CDAPException(
				"The opcode must be set for all the messages");
	}
}

void CDAPMessageValidator::validateResult(const cdap_m_t *message)
{
	/* FIXME: Do something with sense */
}

void CDAPMessageValidator::validateResultReason(const cdap_m_t *message)
{
	if (!message->result_reason_.empty()) {
		if (message->op_code_ != cdap_m_t::M_CREATE_R
				&& message->op_code_ != cdap_m_t::M_DELETE_R
				&& message->op_code_ != cdap_m_t::M_READ_R
				&& message->op_code_ != cdap_m_t::M_WRITE_R
				&& message->op_code_ != cdap_m_t::M_CONNECT_R
				&& message->op_code_ != cdap_m_t::M_RELEASE_R
				&& message->op_code_ != cdap_m_t::M_CANCELREAD
				&& message->op_code_ != cdap_m_t::M_CANCELREAD_R
				&& message->op_code_ != cdap_m_t::M_START_R
				&& message->op_code_ != cdap_m_t::M_STOP_R) {
			throw rina::CDAPException(
					"The resultReason parameter can only be set for M_CREATE_R, M_DELETE_R, M_READ_R, M_WRITE_R, M_START_R, M_STOP_R, M_CONNECT_R, M_RELEASE_R, M_CANCELREAD and M_CANCELREAD_R messages");
		}
	}
}

void CDAPMessageValidator::validateScope(const cdap_m_t *message)
{
	if (message->scope_ != 0) {
		if (message->op_code_ != cdap_m_t::M_CREATE
				&& message->op_code_ != cdap_m_t::M_DELETE
				&& message->op_code_ != cdap_m_t::M_READ
				&& message->op_code_ != cdap_m_t::M_WRITE
				&& message->op_code_ != cdap_m_t::M_START
				&& message->op_code_ != cdap_m_t::M_STOP) {
			throw rina::CDAPException(
					"The scope parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages");
		}
	}
}

void CDAPMessageValidator::validateSrcAEInst(const cdap_m_t *message)
{
	if (!message->src_ae_inst_.empty()) {
		if (message->op_code_ != cdap_m_t::M_CONNECT
				&& message->op_code_ != cdap_m_t::M_CONNECT_R) {
			throw rina::CDAPException(
					"SrcAEInst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateSrcAEName(const cdap_m_t *message)
{
	if (!message->src_ae_name_.empty()) {
		if (message->op_code_ != cdap_m_t::M_CONNECT
				&& message->op_code_ != cdap_m_t::M_CONNECT_R) {
			throw rina::CDAPException(
					"SrcAEName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateSrcApInst(const cdap_m_t *message)
{
	if (!message->src_ap_inst_.empty()) {
		if (message->op_code_ != cdap_m_t::M_CONNECT
				&& message->op_code_ != cdap_m_t::M_CONNECT_R) {
			throw rina::CDAPException(
					"SrcApInst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateSrcApName(const cdap_m_t *message)
{
	if (message->src_ap_name_.empty()) {
		if (message->op_code_ == cdap_m_t::M_CONNECT) {
			throw rina::CDAPException(
					"SrcApName must be set for the M_CONNECT message");
		} else if (message->op_code_ == cdap_m_t::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message->op_code_ != cdap_m_t::M_CONNECT
				&& message->op_code_ != cdap_m_t::M_CONNECT_R) {
			throw rina::CDAPException(
					"SrcApName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateVersion(const cdap_m_t *message)
{
	if (message->version_ == 0) {
		if (message->op_code_ == cdap_m_t::M_CONNECT
				|| message->op_code_ == cdap_m_t::M_CONNECT_R) {
			throw rina::CDAPException(
					"Version must be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

// CLASS CDAPSession
CDAPSession::CDAPSession(CDAPSessionManager *cdap_session_manager, long timeout,
			 SerializerInterface *serializer,
			 CDAPInvokeIdManagerImpl *invoke_id_manager)
{
	cdap_session_manager_ = cdap_session_manager;
	connection_state_machine_ = new ConnectionStateMachine(this, timeout);
	serializer_ = serializer;
	invoke_id_manager_ = invoke_id_manager;
	session_descriptor_ = 0;
}

CDAPSession::~CDAPSession() throw ()
{
	delete connection_state_machine_;
	connection_state_machine_ = 0;
	delete session_descriptor_;
	session_descriptor_ = 0;
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
const cdap_rib::SerializedObject* CDAPSession::encodeNextMessageToBeSent(
		const cdap_m_t &cdap_message)
{
	CDAPMessageValidator::validate(&cdap_message);

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
			checkIsConnected();
			checkInvokeIdNotExists(cdap_message, true);
			break;
		case cdap_m_t::M_CREATE_R:
			checkIsConnected();
			checkCanSendOrReceiveResponse(cdap_message,
						      cdap_m_t::M_CREATE, true);
			break;
		case cdap_m_t::M_DELETE:
			checkIsConnected();
			checkInvokeIdNotExists(cdap_message, true);
			break;
		case cdap_m_t::M_DELETE_R:
			checkIsConnected();
			checkCanSendOrReceiveResponse(cdap_message,
						      cdap_m_t::M_DELETE, true);
			break;
		case cdap_m_t::M_START:
			checkIsConnected();
			checkInvokeIdNotExists(cdap_message, true);
			break;
		case cdap_m_t::M_START_R:
			checkIsConnected();
			checkCanSendOrReceiveResponse(cdap_message,
						      cdap_m_t::M_START, true);
			break;
		case cdap_m_t::M_STOP:
			checkIsConnected();
			checkInvokeIdNotExists(cdap_message, true);
			break;
		case cdap_m_t::M_STOP_R:
			checkIsConnected();
			checkCanSendOrReceiveResponse(cdap_message,
						      cdap_m_t::M_STOP, true);
			break;
		case cdap_m_t::M_WRITE:
			checkIsConnected();
			checkInvokeIdNotExists(cdap_message, true);
			break;
		case cdap_m_t::M_WRITE_R:
			checkIsConnected();
			checkCanSendOrReceiveResponse(cdap_message,
						      cdap_m_t::M_WRITE, true);
			break;
		case cdap_m_t::M_READ:
			checkIsConnected();
			checkInvokeIdNotExists(cdap_message, true);
			break;
		case cdap_m_t::M_READ_R:
			checkIsConnected();
			checkCanSendOrReceiveResponse(cdap_message,
						      cdap_m_t::M_READ, true);
			break;
		case cdap_m_t::M_CANCELREAD:
			checkIsConnected();
			checkCanSendOrReceiveCancelReadRequest(cdap_message,
							       true);
			break;
		case cdap_m_t::M_CANCELREAD_R:
			checkIsConnected();
			checkCanSendOrReceiveCancelReadResponse(cdap_message,
								true);
			break;
		default:
			std::stringstream ss;
			ss << "Unrecognized operation code: "
			   << cdap_message.op_code_;
			throw rina::CDAPException(ss.str());
	}

	return serializeMessage(cdap_message);
}
void CDAPSession::messageSent(const cdap_m_t &cdap_message)
{
	messageSentOrReceived(cdap_message, true);
}
const cdap_m_t* CDAPSession::messageReceived(
		const cdap_rib::SerializedObject &message)
{
	const cdap_m_t *cdap_message = deserializeMessage(message);
	messageSentOrReceived(*cdap_message, false);
	return cdap_message;
}
void CDAPSession::messageReceived(const cdap_m_t &cdap_message)
{
	messageSentOrReceived(cdap_message, false);
}
void CDAPSession::set_session_descriptor(cdap_session_t *session_descriptor)
{
	session_descriptor_ = session_descriptor;
}
int CDAPSession::get_port_id() const
{
	return session_descriptor_->port_id_;
}
cdap_session_t* CDAPSession::get_session_descriptor() const
{
	return session_descriptor_;
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
			populateSessionDescriptor(cdap_message, sent);
			break;
		case cdap_m_t::M_CONNECT_R:
			connection_state_machine_->connectResponseSentOrReceived(
					sent);
			break;
		case cdap_m_t::M_RELEASE:
			connection_state_machine_->releaseSentOrReceived(
					cdap_message, sent);
			break;
		case cdap_m_t::M_RELEASE_R:
			connection_state_machine_->releaseResponseSentOrReceived(
					sent);
			break;
		case cdap_m_t::M_CREATE:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_CREATE, sent);
			break;
		case cdap_m_t::M_CREATE_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_CREATE, sent);
			break;
		case cdap_m_t::M_DELETE:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_DELETE, sent);
			break;
		case cdap_m_t::M_DELETE_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_DELETE, sent);
			break;
		case cdap_m_t::M_START:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_START, sent);
			break;
		case cdap_m_t::M_START_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_START, sent);
			break;
		case cdap_m_t::M_STOP:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_STOP, sent);
			break;
		case cdap_m_t::M_STOP_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_STOP, sent);
			break;
		case cdap_m_t::M_WRITE:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_WRITE, sent);
			break;
		case cdap_m_t::M_WRITE_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_WRITE, sent);
			break;
		case cdap_m_t::M_READ:
			requestMessageSentOrReceived(cdap_message,
						     cdap_m_t::M_READ, sent);
			break;
		case cdap_m_t::M_READ_R:
			responseMessageSentOrReceived(cdap_message,
						      cdap_m_t::M_READ, sent);
			break;
		case cdap_m_t::M_CANCELREAD:
			cancelReadMessageSentOrReceived(cdap_message, sent);
			break;
		case cdap_m_t::M_CANCELREAD_R:
			cancelReadResponseMessageSentOrReceived(cdap_message,
								sent);
			break;
		default:
			std::stringstream ss;
			ss << "Unrecognized operation code: "
			   << cdap_message.op_code_;
			throw rina::CDAPException(ss.str());
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
		throw rina::CDAPException(
				"Cannot send a message because the CDAP session is not in CONNECTED state");
	}
}
void CDAPSession::checkInvokeIdNotExists(const cdap_m_t &cdap_message,
					 bool sent) const
{
	const std::map<int, CDAPOperationState*>* pending_messages;
	if (sent)
		pending_messages = &pending_messages_sent_;
	else
		pending_messages = &pending_messages_recv_;

	if (pending_messages->find(cdap_message.invoke_id_)
			!= pending_messages->end()) {
		std::stringstream ss;
		ss << cdap_message.invoke_id_;
		throw rina::CDAPException(
				"The invokeid " + ss.str() + " already exists");
	}
}
void CDAPSession::checkCanSendOrReceiveCancelReadRequest(
		const cdap_m_t &cdap_message, bool sent) const
{
	bool validationFailed = false;
	const std::map<int, CDAPOperationState*>* pending_messages;
	if (sent)
		pending_messages = &pending_messages_sent_;
	else
		pending_messages = &pending_messages_recv_;

	if (pending_messages->find(cdap_message.invoke_id_)
			!= pending_messages->end()) {
		CDAPOperationState *state = pending_messages->find(
				cdap_message.invoke_id_)->second;
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
			ss << cdap_message.invoke_id_;
			throw rina::CDAPException(
					"Cannot set an M_CANCELREAD message because there is no READ transaction associated to the invoke id "
							+ ss.str());
		}
	} else {
		std::stringstream ss;
		ss << cdap_message.invoke_id_;
		throw rina::CDAPException(
				"Cannot set an M_CANCELREAD message because there is no READ transaction associated to the invoke id "
						+ ss.str());
	}
}
void CDAPSession::requestMessageSentOrReceived(const cdap_m_t &cdap_message,
					       cdap_m_t::Opcode op_code,
					       bool sent)
{
	checkIsConnected();
	checkInvokeIdNotExists(cdap_message, sent);

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
	checkCanSendOrReceiveCancelReadRequest(cdap_message, sender);
	CDAPOperationState *new_operation_state = new CDAPOperationState(
			cdap_m_t::M_CANCELREAD, sender);
	cancel_read_pending_messages_.insert(
			std::pair<int, CDAPOperationState*>(
					cdap_message.invoke_id_,
					new_operation_state));
}
void CDAPSession::checkCanSendOrReceiveResponse(const cdap_m_t &cdap_message,
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
	iterator = pending_messages->find(cdap_message.invoke_id_);
	if (iterator == pending_messages->end()) {
		std::stringstream ss;
		ss << "Cannot send a response for the " << op_code
		   << " operation with invokeId " << cdap_message.invoke_id_
		   << std::endl;
		ss << "There are " << pending_messages->size() << " entries";
		throw rina::CDAPException(ss.str());
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
		   << " operation with invokeId " << cdap_message.invoke_id_;
		throw rina::CDAPException(ss.str());
	}
}
void CDAPSession::checkCanSendOrReceiveCancelReadResponse(
		const cdap_m_t &cdap_message, bool send) const
{
	bool validation_failed = false;

	if (cancel_read_pending_messages_.find(cdap_message.invoke_id_)
			== cancel_read_pending_messages_.end()) {
		std::stringstream ss;
		ss << "Cannot send a response for the "
		   << cdap_m_t::M_CANCELREAD << " operation with invokeId "
		   << cdap_message.invoke_id_;
		throw rina::CDAPException(ss.str());
	}
	CDAPOperationState *state = cancel_read_pending_messages_.find(
			cdap_message.invoke_id_)->second;
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
		   << cdap_message.invoke_id_;
		throw rina::CDAPException(ss.str());
	}
}
void CDAPSession::responseMessageSentOrReceived(const cdap_m_t &cdap_message,
						cdap_m_t::Opcode op_code,
						bool sent)
{
	checkIsConnected();
	checkCanSendOrReceiveResponse(cdap_message, op_code, sent);
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
void CDAPSession::cancelReadResponseMessageSentOrReceived(
		const cdap_m_t &cdap_message, bool sent)
{
	checkIsConnected();
	checkCanSendOrReceiveCancelReadResponse(cdap_message, sent);
	cancel_read_pending_messages_.erase(cdap_message.invoke_id_);
	if (sent)
		pending_messages_sent_.erase(cdap_message.invoke_id_);
	else
		pending_messages_recv_.erase(cdap_message.invoke_id_);
}
const cdap_rib::SerializedObject* CDAPSession::serializeMessage(
		const cdap_m_t &cdap_message) const
{
	return serializer_->serializeMessage(cdap_message);
}
const cdap_m_t* CDAPSession::deserializeMessage(
		const cdap_rib::SerializedObject &message) const
{
	return serializer_->deserializeMessage(message);
}
void CDAPSession::populateSessionDescriptor(const cdap_m_t &cdap_message,
					    bool send)
{
	session_descriptor_->abs_syntax_ = cdap_message.abs_syntax_;
	session_descriptor_->auth_policy_ = cdap_message.auth_policy_;

	if (send) {
		session_descriptor_->dest_ae_inst_ = cdap_message.dest_ae_inst_;
		session_descriptor_->dest_ae_name_ = cdap_message.dest_ae_name_;
		session_descriptor_->dest_ap_inst_ = cdap_message.dest_ap_inst_;
		session_descriptor_->dest_ap_name_ = cdap_message.dest_ap_name_;
		session_descriptor_->src_ae_inst_ = cdap_message.src_ae_inst_;
		session_descriptor_->src_ae_name_ = cdap_message.src_ae_name_;
		session_descriptor_->src_ap_inst_ = cdap_message.src_ap_inst_;
		session_descriptor_->src_ap_name_ = cdap_message.src_ap_name_;
	} else {
		session_descriptor_->dest_ae_inst_ = cdap_message.src_ae_inst_;
		session_descriptor_->dest_ae_name_ = cdap_message.src_ae_name_;
		session_descriptor_->dest_ap_inst_ = cdap_message.src_ap_inst_;
		session_descriptor_->dest_ap_name_ = cdap_message.src_ap_name_;
		session_descriptor_->src_ae_inst_ = cdap_message.dest_ae_inst_;
		session_descriptor_->src_ae_name_ = cdap_message.dest_ae_name_;
		session_descriptor_->src_ap_inst_ = cdap_message.dest_ap_inst_;
		session_descriptor_->src_ap_name_ = cdap_message.dest_ap_name_;
	}
	session_descriptor_->version_ = cdap_message.version_;
}

void CDAPSession::emptySessionDescriptor()
{
	cdap_session_t *new_session = new cdap_session_t;
	new_session->port_id_ = session_descriptor_->port_id_;
	new_session->ap_naming_info_ = session_descriptor_->ap_naming_info_;
	delete session_descriptor_;
	session_descriptor_ = 0;
	session_descriptor_ = new_session;
}

// CLASS CDAPSessionManager
CDAPSessionManager::CDAPSessionManager()
{
	throw rina::CDAPException(
			"Not allowed default constructor of CDAPSessionManager has been called.");
}

CDAPSessionManager::CDAPSessionManager(SerializerInterface *serializer)
{
	timeout_ = DEFAULT_TIMEOUT_IN_MS;
	serializer_ = serializer;
	invoke_id_manager_ = new CDAPInvokeIdManagerImpl();
}

CDAPSessionManager::CDAPSessionManager(SerializerInterface *serializer,
				       long timeout)
{
	timeout_ = timeout;
	serializer_ = serializer;
	invoke_id_manager_ = new CDAPInvokeIdManagerImpl();
}

CDAPSession* CDAPSessionManager::createCDAPSession(int port_id)
{
	if (cdap_sessions_.find(port_id) != cdap_sessions_.end()) {
		return cdap_sessions_.find(port_id)->second;
	} else {
		CDAPSession *cdap_session = new CDAPSession(this, timeout_,
							    serializer_,
							    invoke_id_manager_);
		cdap_session_t *descriptor = new cdap_session_t;
		descriptor->port_id_ = port_id;
		cdap_session->set_session_descriptor(descriptor);
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
	delete serializer_;
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
const cdap_rib::SerializedObject* CDAPSessionManager::encodeCDAPMessage(
		const cdap_m_t &cdap_message)
{
	return serializer_->serializeMessage(cdap_message);
}
const CDAPMessage* CDAPSessionManager::decodeCDAPMessage(
		const cdap_rib::SerializedObject &cdap_message)
{
	return serializer_->deserializeMessage(cdap_message);
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
const cdap_rib::SerializedObject* CDAPSessionManager::encodeNextMessageToBeSent(
		const CDAPMessage &cdap_message, int port_id)
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
			throw rina::CDAPException(ss.str());
		}
	} else {
		cdap_session = it->second;
	}

	return cdap_session->encodeNextMessageToBeSent(cdap_message);
}
const cdap_m_t* CDAPSessionManager::messageReceived(
		const cdap_rib::SerializedObject &encoded_cdap_message,
		int port_id)
{
	const CDAPMessage *cdap_message = decodeCDAPMessage(
			encoded_cdap_message);
	CDAPSession *cdap_session = get_cdap_session(port_id);
	switch (cdap_message->op_code_) {
		case CDAPMessage::M_CONNECT:
			if (cdap_session == 0) {
				cdap_session = createCDAPSession(port_id);
				cdap_session->messageReceived(*cdap_message);
				LOG_DBG("Created a new CDAP session for port %d",
					port_id);
			} else {
				std::stringstream ss;
				ss << "M_CONNECT received on an already open CDAP Session, over flow "
				   << port_id;
				throw rina::CDAPException(ss.str());
			}
			break;
		default:
			if (cdap_session != 0) {
				cdap_session->messageReceived(*cdap_message);
			} else {
				std::stringstream ss;
				ss << "Receive a "
				   << cdap_message->op_code_
				   << " CDAP message on a CDAP session that is not open, over flow "
				   << port_id;
				throw rina::CDAPException(ss.str());
			}
			break;
	}
	return cdap_message;
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
		throw rina::CDAPException(ss.str());
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
		if (current_session->get_session_descriptor()->dest_ap_name_
				== (destination_application_process_name)) {
			return current_session->get_port_id();
		}
	}
	std::stringstream ss;
	ss << "Don't have a running CDAP sesion to "
	   << destination_application_process_name;
	throw rina::CDAPException(ss.str());
}

cdap_m_t* CDAPSessionManager::getOpenConnectionRequestMessage(
		const cdap_rib::con_handle_t &con)
{
	CDAPSession *cdap_session = get_cdap_session(con.port_);
	if (cdap_session == 0) {
		cdap_session = createCDAPSession(con.port_);
	}
	return CDAPMessageFactory::getOpenConnectionRequestMessage(
			con,
			cdap_session->get_invoke_id_manager()->newInvokeId(
					true));
}

cdap_m_t* CDAPSessionManager::getOpenConnectionResponseMessage(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res, int invoke_id)
{
	return CDAPMessageFactory::getOpenConnectionResponseMessage(con, res,
								    invoke_id);
}

cdap_m_t* CDAPSessionManager::getReleaseConnectionRequestMessage(
		int port_id, const cdap_rib::flags_t &flags, bool invoke_id)
{
	cdap_m_t *cdap_message =
			CDAPMessageFactory::getReleaseConnectionRequestMessage(
					flags);
	assignInvokeId(*cdap_message, invoke_id, port_id, true);
	return cdap_message;
}

cdap_m_t* CDAPSessionManager::getReleaseConnectionResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::res_info_t &res,
		int invoke_id)
{
	return CDAPMessageFactory::getReleaseConnectionResponseMessage(
			flags, res, invoke_id);
}
cdap_m_t* CDAPSessionManager::getCreateObjectRequestMessage(
		int port_id, const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		bool invoke_id)
{

	cdap_m_t *cdap_message =
			CDAPMessageFactory::getCreateObjectRequestMessage(filt,
									  flags,
									  obj);
	assignInvokeId(*cdap_message, invoke_id, port_id, true);
	return cdap_message;

}

cdap_m_t* CDAPSessionManager::getCreateObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res, int invoke_id)
{
	return CDAPMessageFactory::getCreateObjectResponseMessage(flags, obj,
								  res,
								  invoke_id);
}

cdap_m_t* CDAPSessionManager::getDeleteObjectRequestMessage(
		int port_id, const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		bool invoke_id)
{
	cdap_m_t *cdap_message =
			CDAPMessageFactory::getDeleteObjectRequestMessage(filt,
									  flags,
									  obj);
	assignInvokeId(*cdap_message, invoke_id, port_id, true);
	return cdap_message;
}

cdap_m_t* CDAPSessionManager::getDeleteObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res, int invoke_id)
{
	return CDAPMessageFactory::getDeleteObjectResponseMessage(flags, obj,
								  res,
								  invoke_id);
}

cdap_m_t* CDAPSessionManager::getStartObjectRequestMessage(
		int port_id, const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		bool invoke_id)
{
	cdap_m_t *cdap_message =
			CDAPMessageFactory::getStartObjectRequestMessage(filt,
									 flags,
									 obj);
	assignInvokeId(*cdap_message, invoke_id, port_id, true);
	return cdap_message;
}

cdap_m_t* CDAPSessionManager::getStartObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::res_info_t &res,
		int invoke_id)
{
	return CDAPMessageFactory::getStartObjectResponseMessage(flags, res,
								 invoke_id);
}

cdap_m_t* CDAPSessionManager::getStartObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res, int invoke_id)
{
	return CDAPMessageFactory::getStartObjectResponseMessage(flags, obj,
								 res, invoke_id);
}

cdap_m_t* CDAPSessionManager::getStopObjectRequestMessage(
		int port_id, const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		bool invoke_id)
{
	cdap_m_t *cdap_message =
			CDAPMessageFactory::getStopObjectRequestMessage(filt,
									flags,
									obj);
	assignInvokeId(*cdap_message, invoke_id, port_id, true);
	return cdap_message;
}

cdap_m_t* CDAPSessionManager::getStopObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::res_info_t &res,
		int invoke_id)
{
	return CDAPMessageFactory::getStopObjectResponseMessage(flags, res,
								invoke_id);
}

cdap_m_t* CDAPSessionManager::getReadObjectRequestMessage(
		int port_id, const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		bool invoke_id)
{
	cdap_m_t *cdap_message =
			CDAPMessageFactory::getReadObjectRequestMessage(filt,
									flags,
									obj);
	assignInvokeId(*cdap_message, invoke_id, port_id, true);
	return cdap_message;
}

cdap_m_t* CDAPSessionManager::getReadObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res, int invoke_id)
{
	return CDAPMessageFactory::getReadObjectResponseMessage(flags, obj, res,
								invoke_id);
}

cdap_m_t* CDAPSessionManager::getWriteObjectRequestMessage(
		int port_id, const cdap_rib::filt_info_t &filt,
		const cdap_rib::flags_t &flags, const cdap_rib::obj_info_t &obj,
		bool invoke_id)
{
	cdap_m_t *cdap_message =
			CDAPMessageFactory::getWriteObjectRequestMessage(filt,
									 flags,
									 obj);
	assignInvokeId(*cdap_message, invoke_id, port_id, true);
	return cdap_message;
}

cdap_m_t* CDAPSessionManager::getWriteObjectResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::res_info_t &res,
		int invoke_id)
{
	return CDAPMessageFactory::getWriteObjectResponseMessage(flags, res,
								 invoke_id);
}

cdap_m_t* CDAPSessionManager::getCancelReadRequestMessage(
		const cdap_rib::flags_t &flags, int invoke_id)
{
	return CDAPMessageFactory::getCancelReadRequestMessage(flags, invoke_id);
}

cdap_m_t* CDAPSessionManager::getCancelReadResponseMessage(
		const cdap_rib::flags_t &flags, const cdap_rib::res_info_t &res,
		int invoke_id)
{
	return CDAPMessageFactory::getCancelReadResponseMessage(flags, res,
								invoke_id);
}
void CDAPSessionManager::assignInvokeId(cdap_m_t &cdap_message, bool invoke_id,
					int port_id, bool sent)
{
	CDAPSession *cdap_session = 0;

	if (invoke_id) {
		cdap_session = get_cdap_session(port_id);
		if (cdap_session) {
			cdap_message.invoke_id_ = cdap_session
					->get_invoke_id_manager()->newInvokeId(
					sent);
		}
	}
}

// CLASS GPBWireMessageProvider
const cdap_m_t* GPBSerializer::deserializeMessage(
		const cdap_rib::SerializedObject &message)
{
	cdap::impl::googleprotobuf::CDAPMessage gpfCDAPMessage;
	cdap_m_t *cdapMessage = new cdap_m_t;

	gpfCDAPMessage.ParseFromArray(message.message_, message.size_);
	// ABS_SYNTAX
	if (gpfCDAPMessage.has_abssyntax())
		cdapMessage->abs_syntax_ = gpfCDAPMessage.abssyntax();
	// AUTH_POLICY
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
	// DEST_AE_INST
	if (gpfCDAPMessage.has_destaeinst())
		cdapMessage->dest_ae_inst_ = gpfCDAPMessage.destaeinst();
	// DEST_AE_NAME
	if (gpfCDAPMessage.has_destaename())
		cdapMessage->dest_ae_name_ = gpfCDAPMessage.destaename();
	// DEST_AP_INST
	if (gpfCDAPMessage.has_destapinst())
		cdapMessage->dest_ap_inst_ = gpfCDAPMessage.destapinst();
	// DEST_AP_NAME
	if (gpfCDAPMessage.has_destapname())
		cdapMessage->dest_ap_name_ = gpfCDAPMessage.destapname();
	// FILTER
	if (gpfCDAPMessage.has_filter()) {
		char *filter = new char[gpfCDAPMessage.filter().size() + 1];
		strcpy(filter, gpfCDAPMessage.filter().c_str());
		cdapMessage->filter_ = filter;
	}
	// FLAGS
	if (gpfCDAPMessage.has_flags()) {
		int flag_value = gpfCDAPMessage.flags();
		cdap_rib::flags_t::Flags flags =
				static_cast<cdap_rib::flags_t::Flags>(flag_value);
		cdapMessage->flags_ = flags;
	}
	// INVOKE_ID
	if (gpfCDAPMessage.has_invokeid())
		cdapMessage->invoke_id_ = gpfCDAPMessage.invokeid();
	// OBJECT_CLASS
	if (gpfCDAPMessage.has_objclass())
		cdapMessage->obj_class_ = gpfCDAPMessage.objclass();
	// OBJ_INSTANCE
	if (gpfCDAPMessage.has_objinst())
		cdapMessage->obj_inst_ = gpfCDAPMessage.objinst();
	// OBJ_NAME
	if (gpfCDAPMessage.has_objname())
		cdapMessage->obj_name_ = gpfCDAPMessage.objname();
	// OBJ_VALUE
	if (gpfCDAPMessage.has_objvalue()) {
		cdap::impl::googleprotobuf::objVal_t obj_val_t = gpfCDAPMessage
				.objvalue();
		char *byte_val = new char[obj_val_t.byteval().size()];
		memcpy(byte_val, obj_val_t.byteval().data(),
		       obj_val_t.byteval().size());
		cdapMessage->obj_value_.message_ = byte_val;
		cdapMessage->obj_value_.size_ = obj_val_t.byteval().size();
	}
	// OP_CODE
	if (gpfCDAPMessage.has_opcode()) {
		int opcode_val = gpfCDAPMessage.opcode();
		CDAPMessage::Opcode opcode =
				static_cast<CDAPMessage::Opcode>(opcode_val);
		cdapMessage->op_code_ = opcode;
	}
	// RESULT
	if (gpfCDAPMessage.has_result())
		cdapMessage->result_ = gpfCDAPMessage.result();
	// RESULT_REASON
	if (gpfCDAPMessage.has_resultreason())
		cdapMessage->result_reason_ = gpfCDAPMessage.resultreason();
	// SCOPE
	if (gpfCDAPMessage.has_scope())
		cdapMessage->scope_ = gpfCDAPMessage.scope();
	// SRC_AE_INST
	if (gpfCDAPMessage.has_srcaeinst())
		cdapMessage->src_ae_inst_ = gpfCDAPMessage.srcaeinst();
	// SRC_AE_NAME
	if (gpfCDAPMessage.has_srcaename())
		cdapMessage->src_ae_name_ = gpfCDAPMessage.srcaename();
	// SRC_AP_INST
	if (gpfCDAPMessage.has_srcapinst())
		cdapMessage->src_ap_inst_ = gpfCDAPMessage.srcapinst();
	// SRC_AP_NAME
	if (gpfCDAPMessage.has_srcapname())
		cdapMessage->src_ap_name_ = gpfCDAPMessage.srcapname();
	// VERISON
	if (gpfCDAPMessage.has_version())
		cdapMessage->version_ = gpfCDAPMessage.version();

	return cdapMessage;
}
// FIXME: check existanc of fields before seting
const cdap_rib::SerializedObject* GPBSerializer::serializeMessage(
		const cdap_m_t &cdapMessage)
{
	cdap::impl::googleprotobuf::CDAPMessage gpfCDAPMessage;
	// ABS_SYNTAX
	gpfCDAPMessage.set_abssyntax(cdapMessage.abs_syntax_);
	// AUTH_POLICY
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
		cdap::impl::googleprotobuf::objVal_t *gpb_obj_val =
				new cdap::impl::googleprotobuf::objVal_t();
		gpb_obj_val->set_byteval(cdapMessage.obj_value_.message_,
					 cdapMessage.obj_value_.size_);
		gpfCDAPMessage.set_allocated_objvalue(gpb_obj_val);
	}
	// OP_CODE
	if (!cdap::impl::googleprotobuf::opCode_t_IsValid(
			cdapMessage.op_code_)) {
		throw CDAPException("Serializing Message: Not a valid OpCode");
	}
	gpfCDAPMessage.set_opcode(
			(cdap::impl::googleprotobuf::opCode_t) cdapMessage
					.op_code_);
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
	char *buffer = new char[size];
	gpfCDAPMessage.SerializeToArray(buffer, size);
	cdap_rib::ser_obj_t *serialized_message =
			new cdap_rib::SerializedObject;
	serialized_message->message_ = buffer;
	serialized_message->size_ = size;

	return serialized_message;
}

// CLASS CDAPProvider
CDAPProvider::CDAPProvider(cdap::CDAPCallbackInterface *callback,
			   CDAPSessionManager *manager)
{
	callback_ = callback;
	manager_ = manager;
}

CDAPProvider::~CDAPProvider()
{
}

cdap_rib::con_handle_t CDAPProvider::remote_open_connection(
		const cdap_rib::vers_info_t &ver,
		const cdap_rib::ep_info_t &src,
		const cdap_rib::ep_info_t &dest,
		const cdap_rib::auth_policy_t &auth, int port)
{
	const cdap_m_t *m_sent;
	cdap_rib::con_handle_t con;

	con.port_ = port;
	con.version_ = ver;
	con.src_ = dest;
	con.dest_ = src;
	con.auth_ = auth;

	m_sent = manager_->getOpenConnectionRequestMessage(con);
	send(m_sent, con.port_);

	delete m_sent;

	return con;
}

int CDAPProvider::remote_close_connection(unsigned int port)
{
	int invoke_id;
	const cdap_m_t *m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	cdap_rib::flags_t flags;
	flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;
	m_sent = manager_->getReleaseConnectionRequestMessage(port, flags,
							      true);
	invoke_id = m_sent->invoke_id_;
	send(m_sent, port);

	delete m_sent;

	return invoke_id;
}

int CDAPProvider::remote_create(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt)
{
	int invoke_id;
	const cdap_m_t *m_sent;

	m_sent = manager_->getCreateObjectRequestMessage(port, filt, flags, obj,
							 true);
	invoke_id = m_sent->invoke_id_;
	send(m_sent, port);

	delete m_sent;

	return invoke_id;
}
int CDAPProvider::remote_delete(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt)
{
	int invoke_id;
	const cdap_m_t *m_sent;

	m_sent = manager_->getDeleteObjectRequestMessage(port, filt, flags, obj,
							 true);
	invoke_id = m_sent->invoke_id_;
	send(m_sent, port);

	delete m_sent;

	return invoke_id;
}
int CDAPProvider::remote_read(unsigned int port,
			      const cdap_rib::obj_info_t &obj,
			      const cdap_rib::flags_t &flags,
			      const cdap_rib::filt_info_t &filt)
{
	int invoke_id;
	const cdap_m_t *m_sent;

	m_sent = manager_->getReadObjectRequestMessage(port, filt, flags, obj,
						       true);
	invoke_id = m_sent->invoke_id_;
	send(m_sent, port);

	delete m_sent;

	return invoke_id;
}
int CDAPProvider::remote_cancel_read(unsigned int port,
				     const cdap_rib::flags_t &flags,
				     int invoke_id)
{
	const cdap_m_t *m_sent;

	m_sent = manager_->getCancelReadRequestMessage(flags, invoke_id);
	send(m_sent, port);

	delete m_sent;

	return invoke_id;
}
int CDAPProvider::remote_write(unsigned int port,
			       const cdap_rib::obj_info_t &obj,
			       const cdap_rib::flags_t &flags,
			       const cdap_rib::filt_info_t &filt)
{
	int invoke_id;
	const cdap_m_t *m_sent;

	m_sent = manager_->getWriteObjectRequestMessage(port, filt, flags, obj,
							true);
	invoke_id = m_sent->invoke_id_;
	send(m_sent, port);

	delete m_sent;

	return invoke_id;
}
int CDAPProvider::remote_start(unsigned int port,
			       const cdap_rib::obj_info_t &obj,
			       const cdap_rib::flags_t &flags,
			       const cdap_rib::filt_info_t &filt)
{
	int invoke_id;
	const cdap_m_t *m_sent;

	m_sent = manager_->getStartObjectRequestMessage(port, filt, flags, obj,
							true);
	invoke_id = m_sent->invoke_id_;
	send(m_sent, port);

	delete m_sent;

	return invoke_id;
}
int CDAPProvider::remote_stop(unsigned int port,
			      const cdap_rib::obj_info_t &obj,
			      const cdap_rib::flags_t &flags,
			      const cdap_rib::filt_info_t &filt)
{
	int invoke_id;
	const cdap_m_t *m_sent;

	m_sent = manager_->getStopObjectRequestMessage(port, filt, flags, obj,
						       true);
	invoke_id = m_sent->invoke_id_;
	send(m_sent, port);

	delete m_sent;

	return invoke_id;
}

void CDAPProvider::send_open_connection_result(
					const cdap_rib::con_handle_t &con,
					const cdap_rib::res_info_t &res,
					int invoke_id)
{
	const cdap_m_t *m_sent;

	m_sent = manager_->getOpenConnectionResponseMessage(con, res,
							    invoke_id);
	send(m_sent, con.port_);

	delete m_sent;
}
void CDAPProvider::send_close_connection_result(unsigned int port,
					     const cdap_rib::flags_t &flags,
					     const cdap_rib::res_info_t &res,
					     int invoke_id)
{
	const cdap_m_t *m_sent;

	//FIXME: change flags
	m_sent = manager_->getReleaseConnectionResponseMessage(flags, res,
							       invoke_id);
	send(m_sent, port);

	delete m_sent;
}

void CDAPProvider::send_create_result(unsigned int port,
					  const cdap_rib::obj_info_t &obj,
					  const cdap_rib::flags_t &flags,
					  const cdap_rib::res_info_t &res,
					  int invoke_id)
{
	const cdap_m_t *m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	m_sent = manager_->getCreateObjectResponseMessage(flags, obj, res,
							  invoke_id);
	send(m_sent, port);

	delete m_sent;
}
void CDAPProvider::send_delete_result(unsigned int port,
					  const cdap_rib::obj_info_t &obj,
					  const cdap_rib::flags_t &flags,
					  const cdap_rib::res_info_t &res,
					  int invoke_id)
{
	const cdap_m_t *m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	m_sent = manager_->getDeleteObjectResponseMessage(flags, obj, res,
							  invoke_id);
	send(m_sent, port);

	delete m_sent;
}
void CDAPProvider::send_read_result(unsigned int port,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id)
{
	const cdap_m_t *m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	m_sent = manager_->getReadObjectResponseMessage(flags, obj, res,
							invoke_id);
	send(m_sent, port);

	delete m_sent;
}
void CDAPProvider::send_cancel_read_result(unsigned int port,
					       const cdap_rib::flags_t &flags,
					       const cdap_rib::res_info_t &res,
					       int invoke_id)
{
	const cdap_m_t *m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	m_sent = manager_->getCancelReadResponseMessage(flags, res, invoke_id);
	send(m_sent, port);

	delete m_sent;
}
void CDAPProvider::send_write_result(unsigned int port,
					 const cdap_rib::flags_t &flags,
					 const cdap_rib::res_info_t &res,
					 int invoke_id)
{
	const cdap_m_t *m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	m_sent = manager_->getWriteObjectResponseMessage(flags, res,
							 invoke_id);
	send(m_sent, port);

	delete m_sent;
}
void CDAPProvider::send_start_result(unsigned int port,
					 const cdap_rib::obj_info_t &obj,
					 const cdap_rib::flags_t &flags,
					 const cdap_rib::res_info_t &res,
					 int invoke_id)
{
	const cdap_m_t *m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	m_sent = manager_->getStartObjectResponseMessage(flags, obj, res,
							 invoke_id);
	send(m_sent, port);

	delete m_sent;
}
void CDAPProvider::send_stop_result(unsigned int port,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id)
{
	const cdap_m_t *m_sent;

	// FIXME change cdap_rib::flags_t::NONE_FLAGS
	m_sent = manager_->getStopObjectResponseMessage(flags, res, invoke_id);
	send(m_sent, port);

	delete m_sent;
}

void CDAPProvider::process_message(cdap_rib::SerializedObject &message,
				   unsigned int port)
{
	const cdap_m_t *m_rcv;
	m_rcv = manager_->messageReceived(message, port);

	// Fill structures
	cdap_rib::con_handle_t con;
	// Auth
	con.auth_.name = m_rcv->auth_policy_.name_;
	con.auth_.versions = m_rcv->auth_policy_.versions_;
	if (con.auth_.options.size_ > 0) {
		con.auth_.options.size_ = m_rcv->auth_policy_.options_.size_;
		con.auth_.options.message_ = new char[con.auth_.options.size_];
		memcpy(con.auth_.options.message_, m_rcv->auth_policy_.options_.message_,
				m_rcv->auth_policy_.options_.size_);
	}
	// Src
	con.src_.ae_name_ = m_rcv->src_ae_name_;
	con.src_.ae_inst_ = m_rcv->src_ae_inst_;
	con.src_.ap_name_ = m_rcv->src_ap_name_;
	con.src_.ae_inst_ = m_rcv->src_ae_inst_;
	// Dest
	con.dest_.ae_name_ = m_rcv->dest_ae_name_;
	con.dest_.ae_inst_ = m_rcv->dest_ae_inst_;
	con.dest_.ap_name_ = m_rcv->dest_ap_name_;
	con.dest_.ae_inst_ = m_rcv->dest_ae_inst_;
	// Port
	con.port_ = port;
	// Version
	con.version_.version_ = m_rcv->version_;
	// Flags
	cdap_rib::flags_t flags;
	flags.flags_ = m_rcv->flags_;
	// Object
	cdap_rib::obj_info_t obj;
	obj.class_ = m_rcv->obj_class_;
	obj.inst_ = m_rcv->obj_inst_;
	obj.name_ = m_rcv->obj_name_;
	obj.value_.size_ = m_rcv->obj_value_.size_;
	obj.value_.message_ = new char[obj.value_.size_];
	memcpy(obj.value_.message_, m_rcv->obj_value_.message_,
	       m_rcv->obj_value_.size_);
	// Filter
	cdap_rib::filt_info_t filt;
	filt.filter_ = m_rcv->filter_;
	filt.scope_ = m_rcv->scope_;
	// Invoke id
	int invoke_id = m_rcv->invoke_id_;
	// Result
	cdap_rib::res_info_t res;
	//FIXME: do not typecast when the codes are an enum in the GPB
	res.code_ = static_cast<cdap_rib::res_code_t>(m_rcv->result_);
	res.reason_ = m_rcv->result_reason_;

	switch (m_rcv->op_code_) {

		//Local
		case cdap_m_t::M_CONNECT:
			callback_->open_connection(con, flags, invoke_id);
			break;
		case cdap_m_t::M_RELEASE:
			callback_->close_connection(con, flags, invoke_id);
			break;
		case cdap_m_t::M_DELETE:
			callback_->delete_request(con, obj, filt,
							 invoke_id);
			break;
		case cdap_m_t::M_CREATE:
			callback_->create_request(con, obj, filt,
							 invoke_id);
			break;
		case cdap_m_t::M_READ:
			callback_->read_request(con, obj, filt,
						       invoke_id);
			break;
		case cdap_m_t::M_CANCELREAD:
			callback_->cancel_read_request(con, obj, filt,
							      invoke_id);
			break;
		case cdap_m_t::M_WRITE:
			callback_->write_request(con, obj, filt, invoke_id);
			break;
		case cdap_m_t::M_START:
			callback_->start_request(con, obj, filt,
							invoke_id);
			break;
		case cdap_m_t::M_STOP:
			callback_->stop_request(con, obj, filt,
						       invoke_id);
			break;

		//Remote
		case cdap_m_t::M_CONNECT_R:
			callback_->remote_open_connection_result(con, res);
			break;
		case cdap_m_t::M_RELEASE_R:
			callback_->remote_close_connection_result(con, res);
			break;
		case cdap_m_t::M_CREATE_R:
			callback_->remote_create_result(con, obj, res);
			break;
		case cdap_m_t::M_DELETE_R:
			callback_->remote_delete_result(con, res);
			break;
		case cdap_m_t::M_READ_R:
			callback_->remote_read_result(con, obj, res);
			break;
		case cdap_m_t::M_CANCELREAD_R:
			callback_->remote_cancel_read_result(con, res);
			break;
		case cdap_m_t::M_WRITE_R:
			callback_->remote_write_result(con, obj, res);
			break;
		case cdap_m_t::M_START_R:
			callback_->remote_start_result(con, obj, res);
			break;
		case cdap_m_t::M_STOP_R:
			callback_->remote_stop_result(con, obj, res);
			break;
		default:
			LOG_ERR("Operation not recognized");
			break;
	}

	delete m_rcv;
}

AppCDAPProvider::AppCDAPProvider(cdap::CDAPCallbackInterface *callback,
				 CDAPSessionManager *manager)
		: CDAPProvider(callback, manager)
{
}

void AppCDAPProvider::send(const cdap_m_t *m_sent, int port)
{
	const cdap_rib::SerializedObject *ser_sent_m = manager_
			->encodeNextMessageToBeSent(*m_sent, port);
	manager_->messageSent(*m_sent, port);
	rina::ipcManager->writeSDU(port, ser_sent_m->message_,
							   ser_sent_m->size_);
	delete[] (char*) ser_sent_m->message_;
	delete ser_sent_m;
}

IPCPCDAPProvider::IPCPCDAPProvider(cdap::CDAPCallbackInterface *callback,
				   CDAPSessionManager *manager)
		: CDAPProvider(callback, manager)
{
}

void IPCPCDAPProvider::send(const cdap_m_t *m_sent, int port)
{
	const cdap_rib::SerializedObject *ser_sent_m = manager_
			->encodeNextMessageToBeSent(*m_sent, port);
	manager_->messageSent(*m_sent, port);
	rina::kernelIPCProcess->writeMgmgtSDUToPortId(ser_sent_m->message_,
						      ser_sent_m->size_, port);
	delete[] (char*) ser_sent_m->message_;
	delete ser_sent_m;
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
		const cdap_rib::flags_t &flags,
		int invoke_id)
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

void CDAPCallbackInterface::remote_create_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res)
{
	LOG_INFO("Callback create_result operation not implemented");
}
void CDAPCallbackInterface::remote_delete_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res)
{
	LOG_INFO("Callback delete_result operation not implemented");
}
void CDAPCallbackInterface::remote_read_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res)
{
	LOG_INFO("Callback read_result operation not implemented");
}
void CDAPCallbackInterface::remote_cancel_read_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res)
{
	LOG_INFO("Callback cancel_read_result operation not implemented");
}
void CDAPCallbackInterface::remote_write_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res)
{
	LOG_INFO("Callback write_result operation not implemented");
}
void CDAPCallbackInterface::remote_start_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res)
{
	LOG_INFO("Callback start_result operation not implemented");
}
void CDAPCallbackInterface::remote_stop_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res)
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


//
// CDAP Provider
//

//Static elements
static bool inited = false;
static SerializerInterface *serializer = NULL;
static CDAPSessionManager *manager = NULL;
static CDAPProviderInterface* iface = NULL;

CDAPProviderInterface* getProvider(){
	return iface;
}

void init(cdap::CDAPCallbackInterface *callback, bool is_IPCP){

	//First check the flag
	if(inited){
		LOG_ERR("Double call to rina::cdap::init()");
		throw Exception("Double call to rina::cdap::init()");
	}

	//Initialize subcomponents
	inited = true;
	serializer = new GPBSerializer();
	manager = new CDAPSessionManager(serializer);

	if (is_IPCP)
		iface = new IPCPCDAPProvider(callback, manager);
	else
		iface = new AppCDAPProvider(callback, manager);
}

void destroy(int port){
	manager->removeCDAPSession(port);
}

void fini(){
	delete serializer;
	delete manager;
	delete iface;
}

} //namespace cdap
} //namespace rina
