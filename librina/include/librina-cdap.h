/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef LIBRINA_CDAP_H
#define LIBRINA_CDAP_H

#ifdef __cplusplus

#include "exceptions.h"
#include "librina-common.h"

namespace rina {

/**
 * Encapsulates the data of an AuthValue
 *
 */
class AuthValue {
	protected:
		/**
		 * Authentication name
		 */
		std::string authName;
		/**
		 * Authentication password
		 */
		std::string authPassword;
		/**
		 * Additional authentication information
		 */
		char* authOther;
	/*	Constructors and Destructors	*/
	public:
		AuthValue();
	/*	Accessors	*/
	public:
		std::string getAuthName() const;
		void setAuthName(std::string authName);
		std::string getAuthPassword() const;
		void setAuthPassword(std::string authPassword);
		char* getAuthOther() const;
		void setAuthOther(char* authOther);
};

/**
 * Encapsulates the data to set an object value
 */
class ObjectValue {
	/*	Members	*/
	protected:
		int intval;
		int sintval;
		long int64val;
		long sint64val;
		std::string strval;
		char* byteval;
		int floatval;
		long doubleval;
		bool booleanval;
	/*	Constructors and Destructors	*/
	public:
		ObjectValue();
	/*	Accessors	*/
	public:
		int getIntval() const;
		void setIntval(int intval);
		int getSintval() const;
		void setSintval(int sintval);
		long getInt64val() const;
		void setInt64val(long int64val);
		long getSint64val() const;
		void setSint64val(long sint64val);
		std::string getStrval() const;
		void setStrval(std::string strval);
		char* getByteval() const;
		void setByteval(char buteval[]);
		int getFloatval() const;
		void setFloatval(int floatval);
		long getDoubleval() const;
		void setDoubleval(long doubleval);
		void setBooleanval(bool booleanval);
		bool isBooleanval() const;
};

/**
 * CDAP message. Depending on the opcode, the following messages are possible:
 * M_CONNECT -> Common Connect Request. Initiate a connection from a source application to a destination application
 * M_CONNECT_R -> Common Connect Response. Response to M_CONNECT, carries connection information or an error indication
 * M_RELEASE -> Common Release Request. Orderly close of a connection
 * M_RELEASE_R -> Common Release Response. Response to M_RELEASE, carries final resolution of close operation
 * M_CREATE -> Create Request. Create an application object
 * M_CREATE_R -> Create Response. Response to M_CREATE, carries result of create request, including identification of the created object
 * M_DELETE -> Delete Request. Delete a specified application object
 * M_DELETE_R -> Delete Response. Response to M_DELETE, carries result of a deletion attempt
 * M_READ -> Read Request. Read the value of a specified application object
 * M_READ_R -> Read Response. Response to M_READ, carries part of all of object value, or error indication
 * M_CANCELREAD -> Cancel Read Request. Cancel a prior read issued using the M_READ for which a value has not been completely returned
 * M_CANCELREAD_R -> Cancel Read Response. Response to M_CANCELREAD, indicates outcome of cancellation
 * M_WRITE -> Write Request. Write a specified value to a specified application object
 * M_WRITE_R -> Write Response. Response to M_WRITE, carries result of write operation
 * M_START -> Start Request. Start the operation of a specified application object, used when the object has operational and non operational states
 * M_START_R -> Start Response. Response to M_START, indicates the result of the operation
 * M_STOP -> Stop Request. Stop the operation of a specified application object, used when the object has operational an non operational states
 * M_STOP_R -> Stop Response. Response to M_STOP, indicates the result of the operation.
 */
class CDAPMessage {
	/*	Constants	*/
	protected:
		static const int ABSTRACT_SYNTAX_VERSION;
	public:
		enum Opcode {NONE_OPCODE, M_CONNECT, M_CONNECT_R, M_RELEASE, M_RELEASE_R, M_CREATE, M_CREATE_R,
			M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_CANCELREAD, M_CANCELREAD_R, M_WRITE,
			M_WRITE_R, M_START, M_START_R, M_STOP, M_STOP_R};
		enum AuthTypes {AUTH_NONE, AUTH_PASSWD, AUTH_SSHRSA, AUTH_SSHDSA};
		enum Flags {NONE_FLAGS, F_SYNC, F_RD_INCOMPLETE};
	/*	Members	*/
	protected:
		/**
		 * AbstractSyntaxID (int32), mandatory. The specific version of the
		 * CDAP protocol message declarations that the message conforms to
		 */
		int absSyntax;
		/**
		 * AuthenticationMechanismName (authtypes), optional, not validated by CDAP.
		 * Identification of the method to be used by the destination application to
		 * authenticate the source application
		 */
		AuthTypes authMech;
		/**
		 * AuthenticationValue (authvalue), optional, not validated by CDAP.
		 * Authentication information accompanying authMech, format and value
		 * appropiate to the selected authMech
		 */
		AuthValue authValue;
		/**
		 * DestinationApplication-Entity-Instance-Id (string), optional, not validated by CDAP.
		 * Specific instance of the Application Entity that the source application
		 * wishes to connect to in the destination application.
		 */
		std::string destAEInst;
		/**
		 * DestinationApplication-Entity-Name (string), mandatory (optional for the response).
		 * Name of the Application Entity that the source application wishes
		 * to connect to in the destination application.
		 */
		std::string destAEName;
		/**
		 * DestinationApplication-Process-Instance-Id (string), optional, not validated by CDAP.
		 * Name of the Application Process Instance that the source wishes to
		 * connect to a the destination.
		 */
		std::string destApInst;
		/**
		 * DestinationApplication-Process-Name (string), mandatory (optional for the response).
		 * Name of the application process that the source application wishes to connect to
		 * in the destination application
		 */
		std::string destApName;
		/**
		 * Filter (bytes). Filter predicate function to be used to determine whether an operation
		 * is to be applied to the designated object (s).
		 */
		char* filter;
		/**
		 * flags (enm, int32), conditional, may be required by CDAP.
		 * Set of Boolean values that modify the meaning of a
		 * message in a uniform way when true.
		 */
		Flags flags;
		/**
		 * InvokeID, (int32). Unique identifier provided to identify a request, used to
		 * identify subsequent associated messages.
		 */
		int invokeID;
		/**
		 * ObjectClass (string). Identifies the object class definition of the
		 * addressed object.
		 */
		std::string objClass;
		/**
		 * ObjectInstance (int64). Object instance uniquely identifies a single object
		 * with a specific ObjectClass and ObjectName in an application's RIB. Either
		 * the ObjectClass and ObjectName or this value may be used, if the ObjectInstance
		 * value is known. If a class and name are supplied in an operation,
		 * an ObjectInstance value may be returned, and that may be used in future operations
		 * in lieu of objClass and objName for the duration of this connection.
		 */
		long objInst;
		/**
		 * ObjectName (string). Identifies a named object that the operation is
		 * to be applied to. Object names identify a unique object of the designated
		 * ObjectClass within an application.
		 */
		std::string objName;
		/**
		 * ObjectValue (ObjectValue). The value of the object.
		 */
		ObjectValue objValue;
		/**
		 * Opcode (enum, int32), mandatory.
		 * Message type of this message.
		 */
		Opcode opCode;
		/**
		 * Result (int32). Mandatory in the responses, forbidden in the requests
		 * The result of an operation, indicating its success (which has the value zero,
		 * the default for this field), partial success in the case of
		 * synchronized operations, or reason for failure
		 */
		int result;
		/**
		 * Result-Reason (string), optional in the responses, forbidden in the requests
		 * Additional explanation of the result
		 */
		std::string resultReason;
		/**
		 * Scope (int32). Specifies the depth of the object tree at
		 * the destination application to which an operation (subject to filtering)
		 * is to apply (if missing or present and having the value 0, the default,
		 * only the targeted application's objects are addressed)
		 */
		int scope;
		/**
		 * SourceApplication-Entity-Instance-Id (string).
		 * AE instance within the application originating the message
		 */
		std::string srcAEInst;
		/**
		 * SourceApplication-Entity-Name (string).
		 * Name of the AE within the application originating the message
		 */
		std::string srcAEName;
		/**
		 * SourceApplication-Process-Instance-Id (string), optional, not validated by CDAP.
		 * Application instance originating the message
		 */
		std::string srcApInst;
		/**
		 * SourceApplicatio-Process-Name (string), mandatory (optional in the response).
		 * Name of the application originating the message
		 */
		std::string srcApName;
		/**
		 * Version (int32). Mandatory in connect request and response, optional otherwise.
		 * Version of RIB and object set to use in the conversation. Note that the
		 * abstract syntax refers to the CDAP message syntax, while version refers to the
		 * version of the AE RIB objects, their values, vocabulary of object id's, and
		 * related behaviors that are subject to change over time. See text for details
		 * of use.
		 */
		long version;
	/*	Constructors and Destructors	*/
	public:
		CDAPMessage();

		static CDAPMessage*getOpenConnectionRequestMessage(AuthTypes authMech,
			const AuthValue &authValue, std::string destAEInst, std::string destAEName, std::string destApInst,
			std::string destApName, std::string srcAEInst, std::string srcAEName, std::string srcApInst,
			std::string srcApName, int invokeID) throw (CDAPException);
		static CDAPMessage*getOpenConnectionResponseMessage(AuthTypes authMech,
			const AuthValue &authValue, std::string destAEInst, std::string destAEName, std::string destApInst,
			std::string destApName, int result, std::string resultReason, std::string srcAEInst, std::string srcAEName,
			std::string srcApInst, std::string srcApName, int invokeID) throw (CDAPException);
		static CDAPMessage*getReleaseConnectionRequestMessage(Flags flags) throw (CDAPException);
		static CDAPMessage*getReleaseConnectionResponseMessage(Flags flags, int result, std::string resultReason, int invokeID) throw (CDAPException);
		static CDAPMessage*getCreateObjectRequestMessage(char filter[], Flags flags,	std::string objClass, long objInst,
				std::string objName, const ObjectValue &objValue, int scope) throw (CDAPException);
		static CDAPMessage*getCreateObjectResponseMessage(Flags flags, std::string objClass, long objInst, std::string objName, const ObjectValue &objValue, int result,
			std::string resultReason, int invokeID) throw (CDAPException);
		static CDAPMessage*getDeleteObjectRequestMessage(char filter[], Flags flags, std::string objClass, long objInst, std::string objName,
			const ObjectValue &objectValue, int scope) throw (CDAPException);
		static CDAPMessage*getDeleteObjectResponseMessage(Flags flags, std::string objClass, long objInst, std::string objName, int result,
				std::string resultReason, int invokeID) throw (CDAPException);
		static CDAPMessage*getStartObjectRequestMessage(char filter[], Flags flags, std::string objClass, const ObjectValue &objValue, long objInst, std::string objName,
			int scope) throw (CDAPException);
		static CDAPMessage*getStartObjectResponseMessage(Flags flags, int result, std::string resultReason, int invokeID) throw (CDAPException);
		static CDAPMessage*getStartObjectResponseMessage(Flags flags, std::string objectClass, const ObjectValue &objectValue, long objInst,
			std::string objName, int result, std::string resultReason, int invokeID) throw (CDAPException);
		static CDAPMessage*getStopObjectRequestMessage(char filter[], Flags flags,
				std::string objClass, const ObjectValue &objValue, long objInst, std::string objName, int scope) throw (CDAPException);
		static CDAPMessage*getStopObjectResponseMessage(Flags flags, int result, std::string resultReason, int invokeID) throw (CDAPException);
		static CDAPMessage*getReadObjectRequestMessage(char filter[], Flags flags, std::string objClass, long objInst, std::string objName, int scope) throw (CDAPException);
		static CDAPMessage*getReadObjectResponseMessage(Flags flags, std::string objClass, long objInst, std::string objName, const ObjectValue &objValue,
			int result, std::string resultReason, int invokeID) throw (CDAPException);
		static CDAPMessage*getWriteObjectRequestMessage(char filter[], Flags flags, std::string objClass, long objInst, const ObjectValue &objValue, std::string objName,
			int scope) throw (CDAPException);
		static CDAPMessage*getWriteObjectResponseMessage(Flags flags, int result, int invokeID, std::string resultReason) throw (CDAPException);
		static CDAPMessage*getCancelReadRequestMessage(Flags flags, int invokeID) throw (CDAPException);
		static CDAPMessage*getCancelReadResponseMessage(Flags flags, int invokeID, int result, std::string resultReason) throw (CDAPException);
		/**
		 * Returns a reply message from the request message, copying all the fields except for: Opcode (it will be the
		 * request message counterpart), result (it will be 0) and resultReason (it will be null)
		 * @param requestMessage
		 * @return
		 * @throws CDAPException
		 */
		/*	Accessors	*/
	public:
                CDAPMessage getReplyMessage();
		int getAbsSyntax() const;
		void setAbsSyntax(int absSyntax);
		AuthTypes getAuthMech() const;
		void setAuthMech(AuthTypes authMech);
		const AuthValue& getAuthValue() const;
		void setAuthValue(AuthValue authValue);
		std::string getDestAEInst() const;
		void setDestAEInst(std::string destAEInst);
		std::string getDestAEName() const;
		void setDestAEName(std::string destAEName);
		std::string getDestApInst() const;
		void setDestApInst(std::string destApInst);
		std::string getDestApName() const;
		void setDestApName(std::string destApName);
		char* getFilter() const;
		void setFilter(char filter[]);
		Flags getFlags() const;
		void setFlags(Flags flags);
		int getInvokeID() const;
		void setInvokeID(int invokeID);
		std::string getObjClass() const;
		void setObjClass(std::string objClass);
		long getObjInst() const;
		void setObjInst(long objInst);
		std::string getObjName() const;
		void setObjName(std::string objName);
		const ObjectValue& getObjValue() const;
		void setObjValue(const ObjectValue &objValue);
		Opcode getOpCode() const;
		void setOpCode(Opcode opCode);
		int getResult() const;
		void setResult(int result);
		std::string getResultReason() const;
		void setResultReason(std::string resultReason);
		int getScope() const;
		void setScope(int scope);
		std::string getSrcAEInst() const;
		void setSrcAEInst(std::string srcAEInst);
		std::string getSrcAEName() const;
		void setSrcAEName(std::string srcAEName);
		std::string getSrcApInst() const;
		void setSrcApInst(std::string srcApInst);
		std::string getSrcApName() const;
		void setSrcApName(std::string srcApName);
		long getVersion() const;
		void setVersion(long version);
	/*	Methods	*/
	public:
		std::string toString();
};

/**
 * Describes a CDAPSession, by identifying the source and destination application processes.
 * Note that "source" and "destination" are just placeholders, as both parties in a CDAP exchange have
 * the same role, because the CDAP session is bidirectional.
 * @author eduardgrasa
 *
 */
class CDAPSessionDescriptor {
	/*	Members	*/
	protected:
		/**
		 * AbstractSyntaxID (int32), mandatory. The specific version of the
		 * CDAP protocol message declarations that the message conforms to
		 */
		int absSyntax;
		/**
		 * AuthenticationMechanismName (authtypes), optional, not validated by CDAP.
		 * Identification of the method to be used by the destination application to
		 * authenticate the source application
		 */
		CDAPMessage::AuthTypes authMech;
		/**
		 * AuthenticationValue (authvalue), optional, not validated by CDAP.
		 * Authentication information accompanying authMech, format and value
		 * appropiate to the selected authMech
		 */
		AuthValue *authValue;
		/**
		 * DestinationApplication-Entity-Instance-Id (string), optional, not validated by CDAP.
		 * Specific instance of the Application Entity that the source application
		 * wishes to connect to in the destination application.
		 */
		std::string destAEInst;
		/**
		 * DestinationApplication-Entity-Name (string), mandatory (optional for the response).
		 * Name of the Application Entity that the source application wishes
		 * to connect to in the destination application.
		 */
		std::string destAEName;
		/**
		 * DestinationApplication-Process-Instance-Id (string), optional, not validated by CDAP.
		 * Name of the Application Process Instance that the source wishes to
		 * connect to a the destination.
		 */
		std::string destApInst;
		/**
		 * DestinationApplication-Process-Name (string), mandatory (optional for the response).
		 * Name of the application process that the source application wishes to connect to
		 * in the destination application
		 */
		std::string destApName;
		/**
		 * SourceApplication-Entity-Instance-Id (string).
		 * AE instance within the application originating the message
		 */
		std::string srcAEInst;
		/**
		 * SourceApplication-Entity-Name (string).
		 * Name of the AE within the application originating the message
		 */
		std::string srcAEName;
		/**
		 * SourceApplication-Process-Instance-Id (string), optional, not validated by CDAP.
		 * Application instance originating the message
		 */
		std::string srcApInst;

		/**
		 * SourceApplicatio-Process-Name (string), mandatory (optional in the response).
		 * Name of the application originating the message
		 */
		std::string srcApName;
		/**
		 * Version (int32). Mandatory in connect request and response, optional otherwise.
		 * Version of RIB and object set to use in the conversation. Note that the
		 * abstract syntax refers to the CDAP message syntax, while version refers to the
		 * version of the AE RIB objects, their values, vocabulary of object id's, and
		 * related behaviors that are subject to change over time. See text for details
		 * of use.
		 */
		long version;
		/**
		 * Uniquely identifies this CDAP session in this IPC process. It matches the portId
		 * of the (N-1) flow that supports the CDAP Session
		 */
		int portId;
	/*	Accessors	*/
	public:
		int getAbsSyntax() const;
		void setAbsSyntax(int absSyntax);
		const CDAPMessage::AuthTypes& getAuthMech() const;
		void setAuthMech(const CDAPMessage::AuthTypes &authMech);
		const AuthValue& getAuthValue() const;
		void setAuthValue(const AuthValue &authValue);
		std::string getDestAEInst() const;
		void setDestAEInst(std::string destAEInst);
		std::string getDestAEName() const;
		void setDestAEName(std::string destAEName);
		std::string getDestApInst() const;
		void setDestApInst(std::string destApInst) const;
		std::string getDestApName() const;
		void setDestApName(std::string destApName);
		std::string getSrcAEInst() const;
		void setSrcAEInst(std::string srcAEInst);
		std::string getSrcAEName() const;
		void setSrcAEName(std::string srcAEName);
		std::string getSrcApInst() const;
		void setSrcApInst(std::string srcApInst);
		std::string getSrcApName() const;
		void setSrcApName(std::string srcApName);
		long getVersion() const;
		void setVersion(long version);
		int getPortId() const;
		void setPortId(int portId);
		/**
		 * The source naming information is always the naming information of the local Application process
		 * @return
		 */
		const ApplicationProcessNamingInformation& getSourceApplicationProcessNamingInfo() const;
		/**
		 * The destination naming information is always the naming information of the remote application process
		 * @return
		 */
		const ApplicationProcessNamingInformation& getDestinationApplicationProcessNamingInfo() const;
};

/**
 * Manages the invoke ids of a session.
 * @author eduardgrasa
 *
 */
class CDAPSessionInvokeIdManager {
	/*	Members	*/
	protected:
		int invokeId;
	/*	Constructors and Destructors	*/
	public:
		virtual ~CDAPSessionInvokeIdManager(){};
	/*	Accessors	*/
	public:
		/**
		 * Obtains a valid invoke id for this session
		 * @return
		 */
		virtual int getInvokeId() const = 0;

		/**
		 * Allows an invoke id to be reused for this session
		 * @param invokeId
		 */
		virtual void freeInvokeId(int invokeId) = 0;

		/**
		 * Mark an invokeId as reserved (don't use it)
		 * @param invokeId
		 */
		virtual void reserveInvokeId(int invokeId) = 0;
};


/**
 * Represents a CDAP session. Clients of the library are the ones managing the invoke ids. Application entities must
 * use the CDAP library this way:
 *  1) when sending a message:
 *     a) create the CDAPMessage
 *     b) call serializeNextMessageToBeSent()
 *     c) if it is successful, send the byte[] through the underlying transport connection
 *     d) if successful, update the CDAPSession state machine by calling messageSent()
 *  2) when receiving a message:
 *     a) call the messageReceived operation
 *     b) if successful, you can already use the cdap message; if not, look at the exception
 */
class CDAPSession {
	/*	Members	 */
	protected:
		int portId;
		CDAPSessionDescriptor cdapSessionDescriptor;
		CDAPSessionInvokeIdManager *cdapSessionInvokeIdManager;
	/*	Constructors and Destructors	*/
	public:
		virtual ~CDAPSession(){};
	/*	Accessors	*/
	public:
		/**
		 * Getter for the portId
		 * @return a String that identifies a CDAP session within an IPC process
		 */
		virtual int getPortId() = 0;
		/**
		 * Getter for the sessionDescriptor
		 * @return the SessionDescriptor, provides all the data that describes this CDAP session (src and dest naming info,
		 * authentication type, version, ...)
		 */
		virtual const CDAPSessionDescriptor& getSessionDescriptor() = 0;
		/* FIXME: CHeck const	*/
		virtual const CDAPSessionInvokeIdManager* getInvokeIdManager() = 0;
	/*	Functionalitites	*/
	public:
		/**
		 * Encodes the next CDAP message to be sent, and checks against the
		 * CDAP state machine that this is a valid message to be sent
		 * @param message
		 * @return the serialized request message, ready to be sent to a flow
		 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
		 */
		virtual char* encodeNextMessageToBeSent(const CDAPMessage &message) throw(CDAPException) = 0;
		/**
		 * Tell the CDAP state machine that we've just sent the cdap Message,
		 * so the internal state machine will be updated
		 * @param message
		 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
		 */
		virtual void messageSent(const CDAPMessage &message) throw (CDAPException) = 0;
		/**
		 * Tell the CDAP state machine that we've received a message, and get
		 * the deserialized CDAP message. The state of the CDAP state machine will be updated
		 * @param cdapMessage
		 * @return
		 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
		 */
		virtual const CDAPMessage& messageReceived(char cdapMessage[]) throw (CDAPException) = 0;
		/**
		 * Tell the CDAP state machine that we've received a message. The state of the CDAP state machine will be updated
		 * @param cdapMessage
		 * @return
		 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
		 */
		virtual const CDAPMessage& messageReceived(const CDAPMessage &cdapMessage) throw (CDAPException) = 0;

};


/**
 * Manages the creation/deletion of CDAP sessions within an IPC process
 * @author eduardgrasa
 *
 */
class CDAPSessionManager {
	/*	Constructors and Destructors	*/
	public:
		virtual ~CDAPSessionManager(){};
	/*	Functionalitites	*/
	public:
		/**
		 * Depending on the message received, it will create a new CDAP state machine (CDAP Session), or update
		 * an existing one, or terminate one.
		 * @param encodedCDAPMessage
		 * @param portId
		 * @return Decoded CDAP Message
		 * @throws CDAPException if the message is not consistent with the appropriate CDAP state machine
		 */
		virtual const CDAPMessage& messageReceived(char encodedCDAPMessage[], int portId) throw (CDAPException) = 0;
		/**
		 * Encodes the next CDAP message to be sent, and checks against the
		 * CDAP state machine that this is a valid message to be sent
		 * @param cdapMessage The cdap message to be serialized
		 * @param portId
		 * @return encoded version of the CDAP Message
		 * @throws CDAPException
		 */
		virtual char* encodeNextMessageToBeSent(const CDAPMessage &cdapMessage, int portId) throw (CDAPException) = 0;
		/**
		 * Update the CDAP state machine because we've sent a message through the
		 * flow identified by portId
		 * @param cdapMessage The CDAP message to be serialized
		 * @param portId
		 * @return encoded version of the CDAP Message
		 * @throws CDAPException
		 */
		virtual void messageSent(const CDAPMessage &cdapMessage, int portId) throw (CDAPException) = 0;
		/**
		 * Get a CDAP session that matches the portId
		 * @param portId
		 * @return
		 */
		virtual const CDAPSession& getCDAPSession(int portId) = 0;
		/**
		 * Get the identifiers of all the CDAP sessions
		 * @return
		 */
		virtual int* getAllCDAPSessionIds() = 0;
		/**
		 * Called by the CDAPSession state machine when the cdap session is terminated
		 * @param portId
		 */
		virtual void removeCDAPSession(int portId) = 0;
		/**
		 * Encodes a CDAP message. It just converts a CDAP message into a byte
		 * array, without caring about what session this CDAP message belongs to (and
		 * therefore it doesn't update any CDAP session state machine). Called by
		 * functions that have to relay CDAP messages, and need to
		 * encode its contents to make the relay decision and maybe modify some
		 * message values.
		 * @param cdapMessage
		 * @return
		 * @throws CDAPException
		 */
		virtual char* encodeCDAPMessage(const CDAPMessage &cdapMessage) throw (CDAPException) = 0;
		/**
		 * Decodes a CDAP message. It just converts the byte array into a CDAP
		 * message, without caring about what session this CDAP message belongs to (and
		 * therefore it doesn't update any CDAP session state machine). Called by
		 * functions that have to relay CDAP messages, and need to serialize/
		 * decode its contents to make the relay decision and maybe modify some
		 * @param cdapMessage
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& decodeCDAPMessage(char cdapMessage[]) throw (CDAPException) = 0;
		/**
		 * Return the portId of the (N-1) Flow that supports the CDAP Session
		 * with the IPC process identified by destinationApplicationProcessName and destinationApplicationProcessInstance
		 * @param destinationApplicationProcessName
		 * @throws CDAPException
		 */
		virtual int getPortId(std::string destinationApplicationProcessName) throw (CDAPException) = 0;
		/**
		 * Create a M_CONNECT CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param authMech
		 * @param authValue
		 * @param destAEInst
		 * @param destAEName
		 * @param destApInst
		 * @param destApName
		 * @param srcAEInst
		 * @param srcAEName
		 * @param srcApInst
		 * @param srcApName
		 * @param invokeId true if one is requested, false otherwise
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getOpenConnectionRequestMessage(int portId, CDAPMessage::AuthTypes authMech, const AuthValue &authValue, std::string destAEInst,
				std::string destAEName, std::string destApInst,	std::string destApName, std::string srcAEInst, std::string srcAEName,
				std::string srcApInst, std::string srcApName) throw (CDAPException) = 0;
		/**
		 * Create a M_CONNECT_R CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param authMech
		 * @param authValue
		 * @param destAEInst
		 * @param destAEName
		 * @param destApInst
		 * @param destApName
		 * @param result
		 * @param resultReason
		 * @param srcAEInst
		 * @param srcAEName
		 * @param srcApInst
		 * @param srcApName
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getOpenConnectionResponseMessage(int portId, CDAPMessage::AuthTypes authMech, const AuthValue &authValue, std::string destAEInst,
				std::string destAEName, std::string destApInst,	std::string destApName, int result, std::string resultReason,
				std::string srcAEInst, std::string srcAEName, std::string srcApInst, std::string srcApName, int invokeId) throw (CDAPException) = 0;
		/**
		 * Create an M_RELEASE CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param invokeID
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getReleaseConnectionRequestMessage(int portId, CDAPMessage::Flags flags, bool invokeID) throw (CDAPException) = 0;
		/**
		 * Create a M_RELEASE_R CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param result
		 * @param resultReason
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getReleaseConnectionResponseMessage(int portId, CDAPMessage::Flags flags, int result, std::string resultReason,
				int invokeId) throw (CDAPException);
		/**
		 * Create a M_CREATE CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param filter
		 * @param flags
		 * @param objClass
		 * @param objInst
		 * @param objName
		 * @param objValue
		 * @param scope
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getCreateObjectRequestMessage(int portId, char filter[], CDAPMessage::Flags flags, std::string objClass,
				long objInst, std::string objName, const ObjectValue &objValue, int scope, bool invokeId) throw (CDAPException) = 0;
		/**
		 * Crate a M_CREATE_R CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param objClass
		 * @param objInst
		 * @param objName
		 * @param objValue
		 * @param result
		 * @param resultReason
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getCreateObjectResponseMessage(int portId, CDAPMessage::Flags flags, std::string objClass, long objInst,
				std::string objName, const ObjectValue &objValue, int result, std::string resultReason, int invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_DELETE CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param filter
		 * @param flags
		 * @param objClass
		 * @param objInst
		 * @param objName
		 * @param objectValue
		 * @param scope
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getDeleteObjectRequestMessage(int portId, char* filter, CDAPMessage::Flags flags, std::string objClass, long objInst, std::string objName,
				const ObjectValue &objectValue, int scope, bool invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_DELETE_R CDAP MESSAGE
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param objClass
		 * @param objInst
		 * @param objName
		 * @param result
		 * @param resultReason
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getDeleteObjectResponseMessage(int portId, CDAPMessage::Flags flags, std::string objClass, long objInst, std::string objName,
				int result, std::string resultReason, int invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_START CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param filter
		 * @param flags
		 * @param objClass
		 * @param objValue
		 * @param objInst
		 * @param objName
		 * @param scope
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getStartObjectRequestMessage(int portId, char filter[], CDAPMessage::Flags flags, std::string objClass, const ObjectValue &objValue,
				long objInst, std::string objName, int scope, bool invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_START_R CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param result
		 * @param resultReason
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getStartObjectResponseMessage(int portId, CDAPMessage::Flags flags, int result, std::string resultReason,
				int invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_START_R CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param result
		 * @param resultReason
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getStartObjectResponseMessage(int portId, CDAPMessage::Flags flags, std::string objClass, const ObjectValue &objValue,
				long objInst, std::string objName, int result, std::string resultReason, int invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_STOP CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param filter
		 * @param flags
		 * @param objClass
		 * @param objValue
		 * @param objInst
		 * @param objName
		 * @param scope
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getStopObjectRequestMessage(int portId, char* filter, CDAPMessage::Flags flags, std::string objClass,
				const ObjectValue &objValue, long objInst, std::string objName, int scope, bool invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_STOP_R CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param result
		 * @param resultReason
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getStopObjectResponseMessage(int portId, CDAPMessage::Flags flags, int result, std::string resultReason, int invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_READ CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param filter
		 * @param flags
		 * @param objClass
		 * @param objInst
		 * @param objName
		 * @param scope
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getReadObjectRequestMessage(int portId, char filter[], CDAPMessage::Flags flags, std::string objClass, long objInst,
				std::string objName, int scope, bool invokeId) throw (CDAPException) = 0;
		/**
		 * Crate a M_READ_R CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param objClass
		 * @param objInst
		 * @param objName
		 * @param objValue
		 * @param result
		 * @param resultReason
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getReadObjectResponseMessage(int portId, CDAPMessage::Flags flags, std::string objClass, long objInst, std::string objName,
				const ObjectValue &objValue, int result, std::string resultReason, int invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_WRITE CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param filter
		 * @param flags
		 * @param objClass
		 * @param objInst
		 * @param objValue
		 * @param objName
		 * @param scope
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getWriteObjectRequestMessage(int portId, char filter[], CDAPMessage::Flags flags, std::string objClass, long objInst,
				const ObjectValue &objValue, std::string objName, int scope, bool invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_WRITE_R CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param result
		 * @param resultReason
		 * @param invokeId
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getWriteObjectResponseMessage(int portId, CDAPMessage::Flags flags, int result, std::string resultReason,
				int invokeId) throw (CDAPException) = 0;
		/**
		 * Create a M_CANCELREAD CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param invokeID
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getCancelReadRequestMessage(int portId, CDAPMessage::Flags flags, int invokeID) throw (CDAPException) = 0;
		/**
		 * Create a M_CANCELREAD_R CDAP Message
		 * @param portId identifies the CDAP Session that this message is part of
		 * @param flags
		 * @param invokeID
		 * @param result
		 * @param resultReason
		 * @return
		 * @throws CDAPException
		 */
		virtual const CDAPMessage& getCancelReadResponseMessage(int portId, CDAPMessage::Flags flags, int invokeID, int result,
				std::string resultReason) throw (CDAPException);
};

/**
 * Handles CDAP messages
 * @author eduardgrasa
 *
 */
class CDAPMessageHandler {
	/*	Constructors and Destructors	*/
	public:
		virtual ~CDAPMessageHandler(){};
	/*	Methods	*/
	public:
		virtual void createResponse(const CDAPMessage &cdapMessage, const CDAPSessionDescriptor &cdapSessionDescriptor) throw (RIBDaemonException) = 0;
		virtual void deleteResponse(const CDAPMessage &cdapMessage, const CDAPSessionDescriptor &cdapSessionDescriptor) throw (RIBDaemonException) = 0;
		virtual void readResponse(const CDAPMessage &cdapMessage, const CDAPSessionDescriptor &cdapSessionDescriptor) throw (RIBDaemonException) = 0;
		virtual void cancelReadResponse(const CDAPMessage &cdapMessage, const CDAPSessionDescriptor &cdapSessionDescriptor) throw (RIBDaemonException) = 0;
		virtual void writeResponse(const CDAPMessage &cdapMessage, const CDAPSessionDescriptor &cdapSessionDescriptor) throw (RIBDaemonException) = 0;
		virtual void startResponse(const CDAPMessage &cdapMessage, const CDAPSessionDescriptor &cdapSessionDescriptor) throw (RIBDaemonException) = 0;
		virtual void stopResponse(const CDAPMessage &cdapMessage, const CDAPSessionDescriptor &cdapSessionDescriptor) throw (RIBDaemonException) = 0;
};

}

#endif

#endif
