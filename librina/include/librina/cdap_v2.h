/*
 * CDAP North bound API
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Bernat Gaston <bernat.gaston@i2cat.net>
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

#ifndef CDAP_PROVIDER_H_
#define CDAP_PROVIDER_H_
#include <string>
#include <librina/concurrency.h>
#include "cdap_rib_structures.h"
#include "cdap.h"

namespace rina {
namespace cdap {

///
/// TODO 
///

class CDAPCallbackInterface
{
 public:
	virtual ~CDAPCallbackInterface();
	//
	// Remote operation results
	//
	virtual void remote_open_connection_result(
					const cdap_rib::con_handle_t &con,
					const cdap_rib::result_info &res);
	virtual void remote_close_connection_result(
					const cdap_rib::con_handle_t &con,
					const cdap_rib::result_info &res);
	virtual void remote_create_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::res_info_t &res);
	virtual void remote_delete_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::res_info_t &res);
	virtual void remote_read_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::res_info_t &res);
	virtual void remote_cancel_read_result(
					const cdap_rib::con_handle_t &con,
					const cdap_rib::res_info_t &res);
	virtual void remote_write_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::res_info_t &res);
	virtual void remote_start_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::res_info_t &res);
	virtual void remote_stop_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::res_info_t &res);

	//
	// Requests coming from the peer to our RIB
	//
	virtual void open_connection(const cdap_rib::con_handle_t &con,
				const cdap_rib::flags_t &flags,
				const int invoke_id);
	virtual void close_connection(const cdap_rib::con_handle_t &con,
				const cdap_rib::flags_t &flags,
				const int invoke_id);
	virtual void create_request(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id);
	virtual void delete_request(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::filt_info_t &filt,
					int invoke_id);
	virtual void read_request(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id);
	virtual void cancel_read_request(
					const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id);
	virtual void write_request(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id);
	virtual void start_request(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id);
	virtual void stop_request(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id);
};

class CDAPProviderInterface {

public:
	virtual ~CDAPProviderInterface(){};

	//
	// Remote operations
	//

	///
	/// Establish a CDAP connection to a remote RIB
	///
	/// @param ver RIB version
	/// @param src Application source information
	/// @param dst Application dst information
	/// @param auth CDAP Authentication context
	/// @param port_id Flow port id to be used
	/// @ret A CDAP connection handle
	///
	virtual cdap_rib::con_handle_t remote_open_connection(
			const cdap_rib::vers_info_t &ver,
			const cdap_rib::ep_info_t &src,
			const cdap_rib::ep_info_t &dest,
			const cdap_rib::auth_policy &auth, int port) = 0;

	///
	/// Close a CDAP connection to a remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_close_connection(unsigned int port) = 0;

	///
	/// Perform a create operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_create(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt) = 0;

	///
	/// Perform a delete operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_delete(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt) = 0;

	///
	/// Perform a read operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_read(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt)= 0;
	///
	/// Perform a cancel read operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_cancel_read(unsigned int port,
				const cdap_rib::flags_t &flags,
				int invoke_id) = 0;

	///
	/// Perform a write operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_write(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt) = 0;

	///
	/// Perform a start operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_start(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt) = 0;

	///
	/// Perform a stop operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_stop(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt) = 0;

	//
	// Local operations results
	//
	virtual void send_open_connection_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::res_info_t &res,
					int invoke_id) = 0;
	virtual void send_close_connection_result(unsigned int port,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id) = 0;
	virtual void send_create_result(unsigned int port,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id) = 0;
	virtual void send_delete_result(unsigned int port,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id) = 0;
	virtual void send_read_result(unsigned int port,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id) = 0;
	virtual void send_cancel_read_result(
			unsigned int port, const cdap_rib::flags_t &flags,
			const cdap_rib::res_info_t &res, int invoke_id) = 0;
	virtual void send_write_result(unsigned int port,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id) = 0;
	virtual void send_start_result(unsigned int port,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id) = 0;
	virtual void send_stop_result(unsigned int port,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id) = 0;

	///
	/// Process an incoming CDAP message
	///
	virtual void process_message(cdap_rib::SerializedObject &message,
				unsigned int port) = 0;

	virtual void destroy_session(int port){ (void)port; /*FIXME*/ };


};

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

	/// AbstractSyntaxID (int32), mandatory. The specific version of the
	/// CDAP protocol message declarations that the message conforms to
	int abs_syntax_;

	/// Authentication Policy information
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

	/// Filter (bytes). Filter predicate function to be used to determine whether an operation
	/// is to be applied to the designated object (s).
	char* filter_;

	/// flags (enm, int32), conditional, may be required by CDAP.
	/// set_ of Boolean values that modify the meaning of a
	/// message in a uniform way when true.
	cdap_rib::flags_t::Flags flags_;

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
	cdap_rib::SerializedObject obj_value_;

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

	//Constructor
	CDAPMessage();
};

//TODO: remove this => convert to a pure struct or use the class directly
typedef CDAPMessage cdap_m_t;

/// Provides a wire format for CDAP messages
class SerializerInterface{

public:
	virtual ~SerializerInterface(){};

	/// Convert from wire format to CDAPMessage
	/// @param message
	/// @return
	/// @throws CDAPException
	virtual const cdap_m_t* deserializeMessage(
			const cdap_rib::SerializedObject &message) = 0;
	/// Convert from CDAP messages to wire format
	/// @param cdapMessage
	/// @return
	/// @throws CDAPException
	virtual const cdap_rib::SerializedObject* serializeMessage(
			const cdap_m_t &cdapMessage) = 0;
};


///
/// Initialize the CDAP provider
///
/// Should be only called once.
///
/// @warning This function is NOT thread safe
///
extern void init(cdap::CDAPCallbackInterface *callback, bool is_IPCP);

///
/// Get the CDAProvider interface object
///
/// @ret a pointer to a valid CDAPProviderInterface object or NULL
///
extern CDAPProviderInterface* getProvider(void);

///
/// Finializes the CDAP provider
///
extern void fini(void);

//TODO remove
extern void destroy(int port);


} //namespace cdap
} //namespace rina

#endif /* CDAP_PROVIDER_H_ */
