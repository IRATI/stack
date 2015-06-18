/*
 * CDAP
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Bernat Gast—n <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef LIBRINA_CDAP_H
#define LIBRINA_CDAP_H

#ifdef __cplusplus

#include <sstream>
#include "exceptions.h"
#include "common.h"

namespace rina {

class CDAPErrorCodes {
public:
	static const int CONNECTION_REJECTED_ERROR;
};

/// Encapsulates the data of an AuthValue
class AuthPolicy {
public:
	AuthPolicy() { };
	std::string to_string() const;

	/// Authentication policy name
	std::string name_;
	/// Supported versions of the policy
	std::list<std::string> versions_;
	/// Policy specific information, encoded as a byte array
	SerializedObject options_;
};

/// Encapsulates the data to set an object value
class ObjectValueInterface {
public:
	enum types{
		inttype,
		sinttype,
		longtype,
		slongtype,
		stringtype,
		bytetype,
		floattype,
		doubletype,
		booltype,
	};
	virtual ~ObjectValueInterface() {
	}
	;
	virtual bool is_empty() const = 0;
	virtual const void* get_value() const = 0;
	virtual types isType() const = 0;
};

template<typename T>
class AbstractObjectValue: public ObjectValueInterface {
public:
	AbstractObjectValue() {
                empty_ = true;
        }
	AbstractObjectValue(T &value) {
                value_ = value;
                empty_ = false;
        }
	virtual ~AbstractObjectValue() { }

	const void* get_value() const
        { return &value_; }

	bool is_empty() const
        { return empty_; }

	virtual types isType() const = 0;
        virtual bool operator==(const AbstractObjectValue<T> &other) = 0;
protected:
	T value_;
	bool empty_;
};

class IntObjectValue: public AbstractObjectValue<int> {
public:
	IntObjectValue();
	IntObjectValue(int value);
	bool operator==(const AbstractObjectValue<int> &other);
	types isType() const;
};

class SIntObjectValue: public AbstractObjectValue<short int> {
public:
	SIntObjectValue();
	SIntObjectValue(short int value);
	bool operator==(const AbstractObjectValue<short int> &other);
	types isType() const;
};

class LongObjectValue: public AbstractObjectValue<long long> {
public:
	LongObjectValue();
	LongObjectValue(long long value);
	bool operator==(const AbstractObjectValue<long long> &other);
	types isType() const;
};

class SLongObjectValue: public AbstractObjectValue<long> {
public:
	SLongObjectValue();
	SLongObjectValue(long value);
	bool operator==(const AbstractObjectValue<long> &other);
	types isType() const;
};

class StringObjectValue: public AbstractObjectValue<std::string> {
public:
	StringObjectValue();
	StringObjectValue(std::string value);
	bool operator==(const AbstractObjectValue<std::string> &other);
	types isType() const;
};

class ByteArrayObjectValue: public AbstractObjectValue<SerializedObject> {
public:
	ByteArrayObjectValue();
	~ByteArrayObjectValue();
	ByteArrayObjectValue(SerializedObject value);
	bool operator==(const AbstractObjectValue<SerializedObject> &other);
	types isType() const;
};

class FloatObjectValue: public AbstractObjectValue<float> {
public:
	FloatObjectValue();
	FloatObjectValue(float value);
	bool operator==(const AbstractObjectValue<float> &other);
	types isType() const;
};

class DoubleObjectValue: public AbstractObjectValue<double> {
public:
	DoubleObjectValue();
	DoubleObjectValue(double value);
	bool operator==(const AbstractObjectValue<double> &other);
	types isType() const;
};

class BooleanObjectValue: public AbstractObjectValue<bool> {
public:
	BooleanObjectValue();
	BooleanObjectValue(bool value);
	bool operator==(const AbstractObjectValue<bool> &other);
	types isType() const;
};

/// Exception produced in the CDAP
class CDAPMessage;
class CDAPException: public Exception {
public:
	enum ErrorCode {
		RELEASE_CONNECITON,
		OTHER
	};
	CDAPException();
	CDAPException(std::string result_reason);
	CDAPException(ErrorCode result, std::string error_message);
	virtual ~CDAPException() throw () {};
	ErrorCode get_result() const;
private:
	/// Operation result code
	ErrorCode result_;
};

/// Validates that a CDAP message is well formed
class CDAPMessageValidator {
public:
	/// Validates a CDAP message
	/// @param message
	/// @throws CDAPException thrown if the CDAP message is not valid, indicating the reason
	static void validate(const CDAPMessage *message);
private:
	static void validateAbsSyntax(const CDAPMessage *message);
	static void validateDestAEInst(const CDAPMessage *message);
	static void validateDestAEName(const CDAPMessage *message);
	static void validateDestApInst(const CDAPMessage *message);
	static void validateDestApName(const CDAPMessage *message);
	static void validateFilter(const CDAPMessage *message);
	static void validateInvokeID(const CDAPMessage *message);
	static void validateObjClass(const CDAPMessage *message);
	static void validateObjInst(const CDAPMessage *message);
	static void validateObjName(const CDAPMessage *message);
	static void validateObjValue(const CDAPMessage *message);
	static void validateOpcode(const CDAPMessage *message);
	static void validateResult(const CDAPMessage *message);
	static void validateResultReason(const CDAPMessage *message);
	static void validateScope(const CDAPMessage *message);
	static void validateSrcAEInst(const CDAPMessage *message);
	static void validateSrcAEName(const CDAPMessage *message);
	static void validateSrcApInst(const CDAPMessage *message);
	static void validateSrcApName(const CDAPMessage *message);
	static void validateVersion(const CDAPMessage *message);
};

/// CDAP message. Depending on the opcode, the following messages are possible:
/// M_CONNECT -> Common Connect Request. Initiate a connection from a source application to a destination application
/// M_CONNECT_R -> Common Connect Response. Response to M_CONNECT, carries connection information or an error indication
/// M_RELEASE -> Common Release Request. Orderly close of a connection
/// M_RELEASE_R -> Common Release Response. Response to M_RELEASE, carries final resolution of close operation
/// M_CREATE -> Create Request. Create an application object
/// M_CREATE_R -> Create Response. Response to M_CREATE, carries result of create request, including identification of the created object
/// M_DELETE -> Delete Request. Delete a specified application object
/// M_DELETE_R -> Delete Response. Response to M_DELETE, carries result of a deletion attempt
/// M_READ -> Read Request. Read the value of a specified application object
/// M_READ_R -> Read Response. Response to M_READ, carries part of all of object value, or error indication
/// M_CANCELREAD -> Cancel Read Request. Cancel a prior read issued using the M_READ for which a value has not been completely returned
/// M_CANCELREAD_R -> Cancel Read Response. Response to M_CANCELREAD, indicates outcome of cancellation
/// M_WRITE -> Write Request. Write a specified value to a specified application object
/// M_WRITE_R -> Write Response. Response to M_WRITE, carries result of write operation
/// M_START -> Start Request. Start the operation of a specified application object, used when the object has operational and non operational states
/// M_START_R -> Start Response. Response to M_START, indicates the result of the operation
/// M_STOP -> Stop Request. Stop the operation of a specified application object, used when the object has operational an non operational states
/// M_STOP_R -> Stop Response. Response to M_STOP, indicates the result of the operation.
class CDAPMessage {

public:
	enum Opcode {
		M_CONNECT,
		M_CONNECT_R,
		M_RELEASE,
		M_RELEASE_R,
		M_CREATE,
		M_CREATE_R,
		M_DELETE,
		M_DELETE_R,
		M_READ,
		M_READ_R,
		M_CANCELREAD,
		M_CANCELREAD_R,
		M_WRITE,
		M_WRITE_R,
		M_START,
		M_START_R,
		M_STOP,
		M_STOP_R,
		NONE_OPCODE
	};
	enum Flags {
		NONE_FLAGS, F_SYNC, F_RD_INCOMPLETE
	};
	CDAPMessage();
	~CDAPMessage();
	static CDAPMessage* getOpenConnectionRequestMessage(
			const AuthPolicy& auth_policy,
			const std::string &dest_ae_inst, const std::string &dest_ae_name,
			const std::string &dest_ap_inst, const std::string &dest_ap_name,
			const std::string &src_ae_inst, const std::string &src_ae_name,
			const std::string &src_ap_inst, const std::string &src_ap_name,
			int invoke_id);
	static CDAPMessage* getOpenConnectionResponseMessage(
			const AuthPolicy& auth_policy,
			const std::string &dest_ae_inst, const std::string &dest_ae_name,
			const std::string &dest_ap_inst, const std::string &dest_ap_name,
			int result, const std::string &result_reason,
			const std::string &src_ae_inst, const std::string &src_ae_name,
			const std::string &src_ap_inst, const std::string &src_ap_name,
			int invoke_id);
	static CDAPMessage* getReleaseConnectionRequestMessage(Flags flags);
	static CDAPMessage* getReleaseConnectionResponseMessage(Flags flags,
			int result, const std::string &result_reason, int invoke_id);
	static CDAPMessage* getCreateObjectRequestMessage(char * filter,
			Flags flags, const std::string &obj_class, long obj_inst,
			const std::string &obj_name, int scope);
	static CDAPMessage* getCreateObjectResponseMessage(Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name,
			int result, const std::string &result_reason, int invoke_id);
	static CDAPMessage* getDeleteObjectRequestMessage(char * filter,
			Flags flags, const std::string &obj_class, long obj_inst,
			const std::string &obj_name, int scope);
	static CDAPMessage* getDeleteObjectResponseMessage(Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name, int result,
			const std::string &result_reason, int invoke_id);
	static CDAPMessage* getStartObjectRequestMessage(char * filter, Flags flags,
			const std::string &obj_class,
			long obj_inst, const std::string &obj_name, int scope);
	static CDAPMessage* getStartObjectResponseMessage(Flags flags,
			int result, const std::string &result_reason, int invoke_id);
	static CDAPMessage* getStartObjectResponseMessage(Flags flags,
			const std::string &objectClass,
			long obj_inst,
			const std::string &obj_name, int result,
			const std::string &result_reason, int invoke_id);
	static CDAPMessage* getStopObjectRequestMessage(char * filter, Flags flags,
			const std::string &obj_class,
			long obj_inst, const std::string &obj_name, int scope);
	static CDAPMessage* getStopObjectResponseMessage(Flags flags,
			int result, const std::string &result_reason, int invoke_id);
	static CDAPMessage* getReadObjectRequestMessage(char * filter, Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name, int scope);
	static CDAPMessage* getReadObjectResponseMessage(Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name,
			int result, const std::string &result_reason, int invoke_id);
	static CDAPMessage* getWriteObjectRequestMessage(char * filter, Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name,
			int scope);
	static CDAPMessage* getWriteObjectResponseMessage(Flags flags,
			int result, int invoke_id, const std::string &result_reason);
	static CDAPMessage* getCancelReadRequestMessage(Flags flags,
			int invoke_id);
	static CDAPMessage* getCancelReadResponseMessage(Flags flags,
			int invoke_id, int result, const std::string &result_reason);
	static CDAPMessage* getRequestMessage(Opcode opcode, char * filter, Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name, int scope);
	static CDAPMessage* getResponseMessage(Opcode opcode, Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name, int result,
			const std::string &result_reason, int invoke_id);
	std::string to_string() const;
	bool is_request_message() const;
	/// Returns a reply message from the request message, copying all the fields except for: Opcode (it will be the
	/// request message counterpart), result (it will be 0) and result_reason (it will be null)
	/// @param requestMessage
	/// @return
	/// @throws CDAPException
#ifndef SWIG
	CDAPMessage getReplyMessage();
	int get_abs_syntax() const;
	void set_abs_syntax(int abs_syntax);
	const AuthPolicy& get_auth_policy() const;
	void set_auth_policy(const AuthPolicy& auth_policy);
	const std::string& get_dest_ae_inst() const;
	void set_dest_ae_inst(const std::string &dest_ae_inst);
	const std::string& get_dest_ae_name() const;
	void set_dest_ae_name(const std::string &dest_ae_name);
	const std::string& get_dest_ap_inst() const;
	void set_dest_ap_inst(const std::string &dest_ap_inst);
	const std::string& get_dest_ap_name() const;
	void set_dest_ap_name(const std::string &dest_ap_name);
	const char* get_filter() const;
	void set_filter(char * filter);
	Flags get_flags() const;
	void set_flags(Flags flags);
	int get_invoke_id() const;
	void set_invoke_id(int invoke_id);
	const std::string& get_obj_class() const;
	void set_obj_class(const std::string &obj_class);
	long get_obj_inst() const;
	void set_obj_inst(long obj_inst);
	const std::string& get_obj_name() const;
	void set_obj_name(const std::string &obj_name);
	const ObjectValueInterface* get_obj_value() const;
	void set_obj_value(ObjectValueInterface *obj_value);
	Opcode get_op_code() const;
	void set_op_code(Opcode op_code);
	int get_result() const;
	void set_result(int result);
	const std::string& get_result_reason() const;
	void set_result_reason(const std::string &result_reason);
	int get_scope() const;
	void set_scope(int scope);
	const std::string& get_src_ae_inst() const;
	void set_src_ae_inst(const std::string &src_ae_inst);
	const std::string& get_src_ae_name() const;
	void set_src_ae_name(const std::string &src_ae_name);
	const std::string& get_src_ap_inst() const;
	void set_src_ap_inst(const std::string &src_ap_inst);
	const std::string& get_src_ap_name() const;
	void set_src_ap_name(const std::string &src_ap_name);
	long get_version() const;
	void set_version(long version);
#endif

	static const int ABSTRACT_SYNTAX_VERSION;
	/// AbstractSyntaxID (int32), mandatory. The specific version of the
	/// CDAP protocol message declarations that the message conforms to
	int abs_syntax_;
	/// Authentication policy parameters
	AuthPolicy auth_policy_;
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
	/// Filter (bytes). Filter predicate function to be used to determine whether an operation
	/// is to be applied to the designated object (s).
	char* filter_;
	/// flags (enm, int32), conditional, may be required by CDAP.
	/// set_ of Boolean values that modify the meaning of a
	/// message in a uniform way when true.
	Flags flags_;
	/// InvokeID, (int32). Unique identifier provided to identify a request, used to
	/// identify subsequent associated messages.
	int invoke_id_;
	/// ObjectClass (string). Identifies the object class definition of the
	/// addressed object.
	std::string obj_class_;
	/// ObjectInstance (int64). Object instance uniquely identifies a single object
	/// with a specific ObjectClass and ObjectName in an application's RIB. Either
	/// the ObjectClass and ObjectName or this value may be used, if the ObjectInstance
	/// value is known. If a class and name are supplied in an operation,
	/// an ObjectInstance value may be returned, and that may be used in future operations
	/// in lieu of obj_class and obj_name for the duration of this connection.
	long obj_inst_;
	/// ObjectName (string). Identifies a named object that the operation is
	/// to be applied to. Object names identify a unique object of the designated
	/// ObjectClass within an application.
	std::string obj_name_;
	/// ObjectValueInterface (ObjectValueInterface). The value of the object.
	ObjectValueInterface *obj_value_;
	/// Opcode (enum, int32), mandatory.
	/// Message type of this message.
	Opcode op_code_;
	/// Result (int32). Mandatory in the responses, forbidden in the requests
	/// The result of an operation, indicating its success (which has the value zero,
	/// the default for this field), partial success in the case of
	/// synchronized operations, or reason for failure
	int result_;
	/// Result-Reason (string), optional in the responses, forbidden in the requests
	/// Additional explanation of the result_
	std::string result_reason_;
	/// Scope (int32). Specifies the depth of the object tree at
	/// the destination application to which an operation (subject to filtering)
	/// is to apply (if missing or present and having the value 0, the default,
	/// only the target_ed application's objects are addressed)
	int scope_;
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

private:
	static const std::string opcodeToString(Opcode opcode);
};

///Describes a CDAPSession, by identifying the source and destination application processes.
///Note that "source" and "destination" are just placeholders, as both parties in a CDAP exchange have
///the same role, because the CDAP session is bidirectional.
class CDAPSessionDescriptor {
public:
	CDAPSessionDescriptor();
	CDAPSessionDescriptor(int port_id);
	CDAPSessionDescriptor(int abs_syntax, const AuthPolicy& auth_policy);
	virtual ~CDAPSessionDescriptor();
	/// The source naming information is always the naming information of the local Application process
	const ApplicationProcessNamingInformation get_source_application_process_naming_info();
	/// The destination naming information is always the naming information of the remote application process
	const ApplicationProcessNamingInformation get_destination_application_process_naming_info();
#ifndef SWIG
	void set_abs_syntax(const int abs_syntax);
	void set_auth_policy(const AuthPolicy& auth_policy);
	void set_dest_ae_inst(const std::string *dest_ae_inst);
	void set_dest_ae_name(const std::string *dest_ae_name);
	void set_dest_ap_inst(const std::string *dest_ap_inst);
	void set_dest_ap_name(const std::string *dest_ap_name);
	void set_src_ae_inst(const std::string *src_ae_inst);
	void set_src_ae_name(const std::string *src_ae_name);
	void set_src_ap_inst(const std::string *src_ap_inst);
	void set_src_ap_name(const std::string *src_ap_name);
	void set_version(const long version);
	void set_ap_naming_info(
			const ApplicationProcessNamingInformation* ap_naming_info);
	int get_port_id() const;
	std::string get_dest_ap_name() const;
	const ApplicationProcessNamingInformation& get_ap_naming_info() const;
#endif

	/// AbstractSyntaxID (int32), mandatory. The specific version of the
	/// CDAP protocol message declarations that the message conforms to
	///
	int abs_syntax_;
	/// Authenticatio Policy information
	AuthPolicy auth_policy_;
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

	ApplicationProcessNamingInformation ap_naming_info_;
};

/// Manages the invoke ids of a session.
class CDAPInvokeIdManagerInterface {
public:
	virtual ~CDAPInvokeIdManagerInterface() {
	}
	;
	/// Obtains a valid invoke id for this session
	/// @return
	virtual int newInvokeId(bool sent) = 0;
	/// Allows an invoke id to be reused for this session
	/// @param invoke_id
	virtual void freeInvokeId(int invoke_id, bool sent) = 0;
	/// Mark an invoke_id as reserved (don't use it)
	/// @param invoke_id
	virtual void reserveInvokeId(int invoke_id, bool sent) = 0;
};

/// Represents a CDAP session. Clients of the library are the ones managing the invoke ids. Application entities must
/// use the CDAP library this way:
///  1) when sending a message:
///     a) create the CDAPMessage
///     b) call serializeNextMessageToBeSent()
///     c) if it is successful, send the byte[] through the underlying transport connection
///     d) if successful, update the CDAPSession state machine by calling messageSent()
///  2) when receiving a message:
///     a) call the messageReceived operation
///     b) if successful, you can already use the cdap message; if not, look at the exception
class CDAPSessionInterface {
public:
	static const std::string SESSION_STATE_NONE;
	static const std::string SESSION_STATE_AWAIT_CON;
	static const std::string SESSION_STATE_CON;
	static const std::string SESSION_STATE_AWAIT_CLOSE;

	virtual ~CDAPSessionInterface() throw () {
	}
	;
	/**
	 * Encodes the next CDAP message to be sent, and checks against the
	 * CDAP state machine that this is a valid message to be sent
	 * @param message
	 * @return the serialized request message, ready to be sent to a flow
	 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
	 */
	virtual const SerializedObject* encodeNextMessageToBeSent(const CDAPMessage &message) = 0;
	/**
	 * Tell the CDAP state machine that we've just sent the cdap Message,
	 * so the internal state machine will be updated
	 * @param message
	 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
	 */
	virtual void messageSent(const CDAPMessage &message) = 0;
	/**
	 * Tell the CDAP state machine that we've received a message, and get
	 * the deserialized CDAP message. The state of the CDAP state machine will be updated
	 * @param cdap_message
	 * @return
	 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
	 */
	virtual const CDAPMessage* messageReceived(const SerializedObject &cdap_message) = 0;
	/**
	 * Tell the CDAP state machine that we've received a message. The state of the CDAP state machine will be updated
	 * @param cdap_message
	 * @return
	 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
	 */
	virtual void messageReceived(const CDAPMessage &cdap_message) = 0;

	///Return the descriptor of this session
	virtual CDAPSessionDescriptor* get_session_descriptor() const = 0;

	//Return the state of this session (NONE, AWAIT_CON, CON or AWAIT_CLOSE)
	virtual std::string get_session_state() const = 0;

	/// True if this CDAP session is closed, false otherwise
	virtual bool is_closed() const = 0;
};

/// Manages the creation/deletion of CDAP sessions within an IPC process
class CDAPSessionManagerInterface {
public:
	virtual ~CDAPSessionManagerInterface() throw () {
	}
	;
	/// Depending on the message received, it will create a new CDAP state machine (CDAP Session), or update
	/// an existing one, or terminate one.
	/// @param encodedCDAPMessage
	/// @param port_id
	/// @return Decoded CDAP Message
	/// @throws CDAPException if the message is not consistent with the appropriate CDAP state machine
	virtual const CDAPMessage* messageReceived(const SerializedObject &encoded_cdap_message,
			int port_id) = 0;
	/// Encodes the next CDAP message to be sent, and checks against the
	/// CDAP state machine that this is a valid message to be sent
	/// @param cdap_message The cdap message to be serialized
	/// @param port_id
	/// @return encoded version of the CDAP Message
	///  @throws CDAPException
	virtual const SerializedObject* encodeNextMessageToBeSent(
			const CDAPMessage &cdap_message, int port_id) = 0;
	/// Creates a CDAPSession
	/// @param port_id the port associated to this CDAP session
	/// @return the CDAPSessionInterface corresponding to the CDAPSession
	virtual CDAPSessionInterface* createCDAPSession(int port_id) = 0;
	/// Update the CDAP state machine because we've sent a message through the
	/// flow identified by port_id
	/// @param cdap_message The CDAP message to be serialized
	/// @param port_id
	/// @return encoded version of the CDAP Message
	/// @throws CDAPException
	virtual void messageSent(const CDAPMessage &cdap_message, int port_id) = 0;
	/// Called by the CDAPSession state machine when the cdap session is terminated
	/// @param port_id
	virtual void removeCDAPSession(int port_id) = 0;
	/// Get a CDAP session that matches the port_id
	/// @param port_id
	/// @return
	virtual const CDAPSessionInterface* get_cdap_session(int port_id) = 0;
	/// Get the identifiers of all the CDAP sessions
	/// @return
	virtual void getAllCDAPSessionIds(std::vector<int> &vector) = 0;

	/// Encodes a CDAP message. It just converts a CDAP message into a byte
	/// array, without caring about what session this CDAP message belongs to (and
	/// therefore it doesn't update any CDAP session state machine). Called by
	/// functions that have to relay CDAP messages, and need to
	/// encode its contents to make the relay decision and maybe modify some
	/// message values.
	/// @param cdap_message
	/// @return
	/// @throws CDAPException
	virtual const SerializedObject* encodeCDAPMessage(const CDAPMessage &cdap_message)
	= 0;
	/// Decodes a CDAP message. It just converts the byte array into a CDAP
	/// message, without caring about what session this CDAP message belongs to (and
	/// therefore it doesn't update any CDAP session state machine). Called by
	/// functions that have to relay CDAP messages, and need to serialize/
	/// decode its contents to make the relay decision and maybe modify some
	/// @param cdap_message
	/// @return
	/// @throws CDAPException
	virtual const CDAPMessage* decodeCDAPMessage(const SerializedObject &cdap_message)
	= 0;
	/// Return the port_id of the (N-1) Flow that supports the CDAP Session
	/// with the IPC process identified by destinationApplicationProcessName and destinationApplicationProcessInstance
	/// @param destinationApplicationProcessName
	/// @throws CDAPException
	virtual int get_port_id(std::string destination_application_process_name)
	= 0;
	/// Create a M_CONNECT CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param auth_mech
	/// @param auth_value
	/// @param dest_ae_inst
	/// @param dest_ae_name
	/// @param dest_ap_inst
	/// @param dest_ap_name
	/// @param src_ae_inst
	/// @param src_ae_name
	/// @param src_ap_inst
	/// @param src_ap_name
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getOpenConnectionRequestMessage(int port_id,
			const AuthPolicy& auth_policy,
			const std::string &dest_ae_inst, const std::string &dest_ae_name,
			const std::string &dest_ap_inst, const std::string &dest_ap_name,
			const std::string &src_ae_inst, const std::string &src_ae_name,
			const std::string &src_ap_inst, const std::string &src_ap_name) = 0;
	/// Create a M_CONNECT_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param auth_mech
	/// @param auth_value
	/// @param dest_ae_inst
	/// @param dest_ae_name
	/// @param dest_ap_inst
	/// @param dest_ap_name
	/// @param result
	/// @param result_reason
	/// @param src_ae_inst
	/// @param src_ae_name
	/// @param src_ap_inst
	/// @param src_ap_name
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getOpenConnectionResponseMessage(
			const AuthPolicy& auth_policy,
			const std::string &dest_ae_inst, const std::string &dest_ae_name,
			const std::string &dest_ap_inst, const std::string &dest_ap_name,
			int result, const std::string &result_reason,
			const std::string &src_ae_inst, const std::string &src_ae_name,
			const std::string &src_ap_inst, const std::string &src_ap_name,
			int invoke_id) = 0;
	/// Create an M_RELEASE CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getReleaseConnectionRequestMessage(int port_id,
			CDAPMessage::Flags flags, bool invoke_id) = 0;
	/// Create a M_RELEASE_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getReleaseConnectionResponseMessage(
			CDAPMessage::Flags flags, int result,
			const std::string &result_reason, int invoke_id) = 0;
	/// Create a M_CREATE CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param obj_class
	/// @param obj_inst
	/// @param obj_name
	/// @param obj_value
	/// @param scope
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getCreateObjectRequestMessage(int port_id,
			char * filter, CDAPMessage::Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name,
			int scope, bool invoke_id) = 0;
	/// Crate a M_CREATE_R CDAP Message
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
	virtual CDAPMessage* getCreateObjectResponseMessage(
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name,
			int result,
			const std::string &result_reason, int invoke_id) = 0;
	/// Create a M_DELETE CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param filter
	/// @param flags
	/// @param obj_class
	/// @param obj_inst
	/// @param obj_name
	/// @param object_value
	/// @param scope
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getDeleteObjectRequestMessage(int port_id,
			char* filter, CDAPMessage::Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name,
			int scope,
			bool invoke_id)= 0;
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
	virtual CDAPMessage* getDeleteObjectResponseMessage(
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name, int result,
			const std::string &result_reason, int invoke_id) = 0;
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
	virtual CDAPMessage* getStartObjectRequestMessage(int port_id,
			char * filter, CDAPMessage::Flags flags,
			const std::string &obj_class,
			long obj_inst, const std::string &obj_name, int scope,
			bool invoke_id) = 0;
	/// Create a M_START_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getStartObjectResponseMessage(
			CDAPMessage::Flags flags, int result,
			const std::string &result_reason, int invoke_id) = 0;
	/// Create a M_START_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getStartObjectResponseMessage(
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst,
			const std::string &obj_name, int result,
			const std::string &result_reason, int invoke_id) = 0;
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
	virtual CDAPMessage* getStopObjectRequestMessage(int port_id,
			char* filter, CDAPMessage::Flags flags,
			const std::string &obj_class,
			long obj_inst, const std::string &obj_name, int scope,
			bool invoke_id) = 0;
	/// Create a M_STOP_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getStopObjectResponseMessage(
			CDAPMessage::Flags flags, int result,
			const std::string &result_reason, int invoke_id) = 0;
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
	virtual CDAPMessage* getReadObjectRequestMessage(int port_id,
			char * filter, CDAPMessage::Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name, int scope, bool invoke_id)
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
	virtual CDAPMessage* getReadObjectResponseMessage(
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name,
			int result,
			const std::string &result_reason, int invoke_id) = 0;
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
	virtual CDAPMessage* getWriteObjectRequestMessage(int port_id,
			char * filter, CDAPMessage::Flags flags,
			const std::string &obj_class, long obj_inst,
			const std::string &obj_name,
			int scope, bool invoke_id) = 0;
	/// Create a M_WRITE_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param result
	/// @param result_reason
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getWriteObjectResponseMessage(
			CDAPMessage::Flags flags, int result,
			const std::string &result_reason, int invoke_id) = 0;
	/// Create a M_CANCELREAD CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param invoke_id
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getCancelReadRequestMessage(
			CDAPMessage::Flags flags, int invoke_id) = 0;
	/// Create a M_CANCELREAD_R CDAP Message
	/// @param port_id identifies the CDAP Session that this message is part of
	/// @param flags
	/// @param invoke_id
	/// @param result
	/// @param result_reason
	/// @return
	/// @throws CDAPException
	virtual CDAPMessage* getCancelReadResponseMessage(
			CDAPMessage::Flags flags, int invoke_id, int result,
			const std::string &result_reason) = 0;

	virtual CDAPMessage* getRequestMessage(int port_id,
			CDAPMessage::Opcode opcode, char * filter,
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name,
			int scope, bool invoke_id) = 0;

	virtual CDAPMessage* getResponseMessage(CDAPMessage::Opcode opcode,
			CDAPMessage::Flags flags, const std::string &obj_class,
			long obj_inst, const std::string &obj_name,
			int result,
			const std::string &result_reason, int invoke_id) = 0;

	virtual CDAPInvokeIdManagerInterface * get_invoke_id_manager() = 0;
};

/// Provides a wire format for CDAP messages
class WireMessageProviderInterface {
public:
	virtual ~WireMessageProviderInterface() throw () {
	}
	;
	/// Convert from wire format to CDAPMessage
	/// @param message
	/// @return
	/// @throws CDAPException
	virtual const CDAPMessage* deserializeMessage(const SerializedObject &message) = 0;
	/// Convert from CDAP messages to wire format
	/// @param cdapMessage
	/// @return
	/// @throws CDAPException
	virtual const SerializedObject* serializeMessage(const CDAPMessage &cdapMessage) = 0;
};

/// Factory to return a WireMessageProvider
class WireMessageProviderFactory{
public:
	WireMessageProviderInterface* createWireMessageProvider();
};

class CDAPSessionManagerFactory {
public:
	CDAPSessionManagerInterface* createCDAPSessionManager(
			WireMessageProviderFactory *wire_message_provider_factory,
			long timeout);
};

class CACEPHandler {
public:
        virtual ~CACEPHandler(){};

        /// A remote IPC process Connect request has been received.
        /// @param invoke_id the id of the connect message
        /// @param session_descriptor
        virtual void connect(const CDAPMessage& cdap_message,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;

        /// A remote IPC process Connect response has been received.
        /// @param result
        /// @param result_reason
        /// @param session_descriptor
        virtual void connectResponse(int result, const std::string& result_reason,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;

        /// A remote IPC process Release request has been received.
        /// @param invoke_id the id of the release message
        /// @param session_descriptor
        virtual void release(int invoke_id,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;

        /// A remote IPC process Release response has been received.
        /// @param result
        /// @param result_reason
        /// @param session_descriptor
        virtual void releaseResponse(int result, const std::string& result_reason,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;

        /// Process an authentication message
        virtual void process_authentication_message(const CDAPMessage& message,
        		rina::CDAPSessionDescriptor * session_descriptor) = 0;
};

/// Interface of classes that handle CDAP response message.
class ICDAPResponseMessageHandler {
public:
        virtual ~ICDAPResponseMessageHandler(){};
        virtual void createResponse(int result, const std::string& result_reason,
                        void * object_value, rina::CDAPSessionDescriptor * session_descriptor) = 0;
        virtual void deleteResponse(int result, const std::string& result_reason,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;
        virtual void readResponse(int result, const std::string& result_reason,
                        void * object_value, const std::string& object_name,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;
        virtual void cancelReadResponse(int result, const std::string& result_reason,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;
        virtual void writeResponse(int result, const std::string& result_reason,
                        void * object_value, rina::CDAPSessionDescriptor * session_descriptor) = 0;
        virtual void startResponse(int result, const std::string& result_reason,
                        void * object_value, rina::CDAPSessionDescriptor * session_descriptor) = 0;
        virtual void stopResponse(int result, const std::string& result_reason,
                        void * object_value, rina::CDAPSessionDescriptor * session_descriptor) = 0;
};

class BaseCDAPResponseMessageHandler: public ICDAPResponseMessageHandler {
public:
        virtual void createResponse(int result, const std::string& result_reason,
                        void * object_value, CDAPSessionDescriptor * session_descriptor) {
                (void) result; // Stop compiler barfs
                (void) result_reason; //Stop compiler barfs
                (void) object_value; //Stop compiler barfs
                (void) session_descriptor; // Stop compiler barfs
        }
        virtual void deleteResponse(int result, const std::string& result_reason,
                        CDAPSessionDescriptor * session_descriptor) {
                                (void) result; // Stop compiler barfs
                                (void) result_reason; //Stop compiler barfs
                                (void) session_descriptor; // Stop compiler barfs
        }
        virtual void readResponse(int result, const std::string& result_reason,
                        void * object_value, const std::string& object_name,
                        CDAPSessionDescriptor * session_descriptor) {
                                (void) result; // Stop compiler barfs
                                (void) result_reason; //Stop compiler barfs
                                (void) object_value; //Stop compiler barfs
                                (void) object_name; //Stop compiler barfs
                                (void) session_descriptor; // Stop compiler barfs
        }
        virtual void cancelReadResponse(int result, const std::string& result_reason,
                        CDAPSessionDescriptor * cdapSessionDescriptor) {
                                (void) result; // Stop compiler barfs
                                (void) result_reason; //Stop compiler barfs
                (void) cdapSessionDescriptor; // Stop compiler barfs
        }
        virtual void writeResponse(int result, const std::string& result_reason,
                        void * object_value, CDAPSessionDescriptor * session_descriptor) {
                (void) result; // Stop compiler barfs
                (void) result_reason; // Stop compiler barfs
                (void) object_value; // Stop compiler barfs
                (void) session_descriptor; // Stop compiler barfs
        }
        virtual void startResponse(int result, const std::string& result_reason,
                        void * object_value, CDAPSessionDescriptor * session_descriptor) {
                                (void) result; // Stop compiler barfs
                                (void) result_reason; // Stop compiler barfs
                                (void) object_value; // Stop compiler barfs
                                (void) session_descriptor; // Stop compiler barfs
        }
        virtual void stopResponse(int result, const std::string& result_reason,
                        void * object_value, CDAPSessionDescriptor * session_descriptor) {
                                (void) result; // Stop compiler barfs
                                (void) result_reason; // Stop compiler barfs
                                (void) object_value; // Stop compiler barfs
                                (void) session_descriptor; // Stop compiler barfs
        }
};

}

#endif

#endif
