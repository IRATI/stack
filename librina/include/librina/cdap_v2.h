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

#include <librina/common.h>
#include <librina/concurrency.h>
#include "cdap_rib_structures.h"

namespace rina {
namespace cdap {

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

//TODO: remove this => convert to a pure struct or use the class directly
typedef CDAPMessage cdap_m_t;

class CDAPMessageFactory
{
 public:
	static void getOpenConnectionRequestMessage(cdap_m_t & msg,
						    const cdap_rib::con_handle_t &con,
						    int invoke_id);
	static void getOpenConnectionResponseMessage(cdap_m_t & msg,
						     const cdap_rib::con_handle_t &con,
						     const cdap_rib::res_info_t &res,
						     int invoke_id);
	static void getReleaseConnectionRequestMessage(cdap_m_t & msg,
						       const cdap_rib::flags_t &flags);
	static void getReleaseConnectionResponseMessage(cdap_m_t & msg,
						        const cdap_rib::flags_t &flags,
						        const cdap_rib::res_info_t &res,
						        int invoke_id);
	static void getCreateObjectRequestMessage(cdap_m_t & msg,
						  const cdap_rib::filt_info_t &filt,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::obj_info_t &obj);
	static void getCreateObjectResponseMessage(cdap_m_t & msg,
						   const cdap_rib::flags_t &flags,
						   const cdap_rib::obj_info_t &obj,
						   const cdap_rib::res_info_t &res,
						   int invoke_id);
	static void getDeleteObjectRequestMessage(cdap_m_t & msg,
						  const cdap_rib::filt_info_t &filt,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::obj_info_t &obj);
	static void getDeleteObjectResponseMessage(cdap_m_t & msg,
						   const cdap_rib::flags_t &flags,
						   const cdap_rib::obj_info_t &obj,
						   const cdap_rib::res_info_t &res,
						   int invoke_id);
	static void getStartObjectRequestMessage(cdap_m_t & msg,
						 const cdap_rib::filt_info_t &filt,
						 const cdap_rib::flags_t &flags,
						 const cdap_rib::obj_info_t &obj);
	static void getStartObjectResponseMessage(cdap_m_t & msg,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::res_info_t &res,
						  int invoke_id);
	static void getStartObjectResponseMessage(cdap_m_t & msg,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::obj_info_t &obj,
						  const cdap_rib::res_info_t &res, int invoke_id);
	static void getStopObjectRequestMessage(cdap_m_t & msg,
						const cdap_rib::filt_info_t &filt,
						const cdap_rib::flags_t &flags,
						const cdap_rib::obj_info_t &obj);
	static void getStopObjectResponseMessage(cdap_m_t & msg,
						 const cdap_rib::flags_t &flags,
						 const cdap_rib::res_info_t &res,
						 int invoke_id);
	static void getReadObjectRequestMessage(cdap_m_t & msg,
						const cdap_rib::filt_info_t &filt,
						const cdap_rib::flags_t &flags,
						const cdap_rib::obj_info_t &obj);
	static void getReadObjectResponseMessage(cdap_m_t & msg,
						 const cdap_rib::flags_t &flags,
						 const cdap_rib::obj_info_t &obj,
						 const cdap_rib::res_info_t &res,
						 int invoke_id);
	static void getWriteObjectRequestMessage(cdap_m_t & msg,
						 const cdap_rib::filt_info_t &filt,
						 const cdap_rib::flags_t &flags,
						 const cdap_rib::obj_info_t &obj);
	static void getWriteObjectResponseMessage(cdap_m_t & msg,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::res_info_t &res,
						  int invoke_id);
	static void getCancelReadRequestMessage(cdap_m_t & msg,
						const cdap_rib::flags_t &flags,
						int invoke_id);
	static void getCancelReadResponseMessage(cdap_m_t & msg,
						 const cdap_rib::flags_t &flags,
						 const cdap_rib::res_info_t &res,
						 int invoke_id);
 private:
	static const int ABSTRACT_SYNTAX_VERSION;
};

///Object exchanged between applications processes that
///contains the source and destination addresses of the processes
///and optional authentication information, as well as an
///encoded CDAP Message. It is used to exchange CDAP messages
///between APs without having a CDAP session previously established
///(it can be seen as a one message session)
class ADataObject {
public:
	static const std::string A_DATA;
	static const std::string A_DATA_OBJECT_CLASS;
	static const std::string A_DATA_OBJECT_NAME;

	ADataObject();
	ADataObject(unsigned int source_address,
		    unsigned int dest_address);
	~ADataObject();

	//The address of the source AP (or IPCP)
	unsigned int source_address_;

	//The address of the destination AP (or IPCP)
	unsigned int dest_address_;

	ser_obj_t encoded_cdap_message_;
};

class CDAPCallbackInterface
{
 public:
	virtual ~CDAPCallbackInterface();
	//
	// Remote operation results
	//
	virtual void remote_open_connection_result(cdap_rib::con_handle_t &con,
						   const cdap::CDAPMessage& message);
	virtual void remote_close_connection_result(const cdap_rib::con_handle_t &con,
						    const cdap_rib::result_info &res);
	virtual void remote_create_result(const cdap_rib::con_handle_t &con,
					  const cdap_rib::obj_info_t &obj,
					  const cdap_rib::res_info_t &res,
					  const int invoke_id);
	virtual void remote_delete_result(const cdap_rib::con_handle_t &con,
					  const cdap_rib::obj_info_t &obj,
					  const cdap_rib::res_info_t &res,
					  const int invoke_id);
	virtual void remote_read_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::res_info_t &res,
					const cdap_rib::flags_t &flags,
					const int invoke_id);
	virtual void remote_cancel_read_result(
					const cdap_rib::con_handle_t &con,
					const cdap_rib::res_info_t &res,
					const int invoke_id);
	virtual void remote_write_result(const cdap_rib::con_handle_t &con,
					 const cdap_rib::obj_info_t &obj,
					 const cdap_rib::res_info_t &res,
					 const int invoke_id);
	virtual void remote_start_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::res_info_t &res,
					const int invoke_id);
	virtual void remote_stop_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::res_info_t &res,
					const int invoke_id);

	//
	// Requests coming from the peer to our RIB
	//
	virtual void open_connection(cdap_rib::con_handle_t &con,
				     const cdap::CDAPMessage& message);
	virtual void close_connection(const cdap_rib::con_handle_t &con,
				      const cdap_rib::flags_t &flags,
				      const int invoke_id);
	virtual void process_authentication_message(const cdap::CDAPMessage& message,
						    const cdap_rib::con_handle_t &con);
	virtual void create_request(const cdap_rib::con_handle_t &con,
				    const cdap_rib::obj_info_t &obj,
				    const cdap_rib::filt_info_t &filt,
				    const cdap_rib::auth_policy_t& auth,
				    const int invoke_id);
	virtual void delete_request(const cdap_rib::con_handle_t &con,
				    const cdap_rib::obj_info_t &obj,
				    const cdap_rib::filt_info_t &filt,
				    const cdap_rib::auth_policy_t& auth,
				    int invoke_id);
	virtual void read_request(const cdap_rib::con_handle_t &con,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::filt_info_t &filt,
				  const cdap_rib::flags_t &flags,
				  const cdap_rib::auth_policy_t& auth,
				  const int invoke_id);
	virtual void cancel_read_request(const cdap_rib::con_handle_t &con,
					 const cdap_rib::obj_info_t &obj,
					 const cdap_rib::filt_info_t &filt,
					 const cdap_rib::auth_policy_t& auth,
					 const int invoke_id);
	virtual void write_request(const cdap_rib::con_handle_t &con,
				   const cdap_rib::obj_info_t &obj,
				   const cdap_rib::filt_info_t &filt,
				   const cdap_rib::auth_policy_t& auth,
				   const int invoke_id);
	virtual void start_request(const cdap_rib::con_handle_t &con,
				   const cdap_rib::obj_info_t &obj,
				   const cdap_rib::filt_info_t &filt,
				   const cdap_rib::auth_policy_t& auth,
				   const int invoke_id);
	virtual void stop_request(const cdap_rib::con_handle_t &con,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::filt_info_t &filt,
				  const cdap_rib::auth_policy_t& auth,
				  const int invoke_id);
};

class CDAPIOHandler;
class CDAPSessionManagerInterface;

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
	virtual cdap_rib::con_handle_t remote_open_connection(const cdap_rib::vers_info_t &ver,
							      const cdap_rib::ep_info_t &src,
							      const cdap_rib::ep_info_t &dest,
							      const cdap_rib::auth_policy &auth,
							      int port) = 0;

	///
	/// Close a CDAP connection to a remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_close_connection(unsigned int port,
					    bool need_reply) = 0;

	///
	/// Perform a create operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_create(const cdap_rib::con_handle_t &con,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::flags_t &flags,
				  const cdap_rib::filt_info_t &filt,
				  const cdap_rib::auth_policy &auth,
				  const int invoke_id = -1) = 0;

	///
	/// Perform a delete operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_delete(const cdap_rib::con_handle_t &con,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::flags_t &flags,
				  const cdap_rib::filt_info_t &filt,
				  const cdap_rib::auth_policy &auth,
				  const int invoke_id = -1) = 0;

	///
	/// Perform a read operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_read(const cdap_rib::con_handle_t &con,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt,
				const cdap_rib::auth_policy &auth,
				const int invoke_id = -1)= 0;
	///
	/// Perform a cancel read operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_cancel_read(const cdap_rib::con_handle_t &con,
				       const cdap_rib::flags_t &flags,
				       const cdap_rib::auth_policy &auth,
				       const int invoke_id = -1) = 0;

	///
	/// Perform a write operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_write(const cdap_rib::con_handle_t &con,
				 const cdap_rib::obj_info_t &obj,
				 const cdap_rib::flags_t &flags,
				 const cdap_rib::filt_info_t &filt,
				 const cdap_rib::auth_policy &auth,
				 const int invoke_id = -1) = 0;

	///
	/// Perform a start operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_start(const cdap_rib::con_handle_t &con,
				 const cdap_rib::obj_info_t &obj,
				 const cdap_rib::flags_t &flags,
				 const cdap_rib::filt_info_t &filt,
				 const cdap_rib::auth_policy &auth,
				 const int invoke_id = -1) = 0;

	///
	/// Perform a stop operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	virtual int remote_stop(const cdap_rib::con_handle_t &con,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt,
				const cdap_rib::auth_policy &auth,
				const int invoke_id = -1) = 0;

	//
	// Local operations results
	//
	virtual void send_open_connection_result(const cdap_rib::con_handle_t &con,
						 const cdap_rib::res_info_t &res,
						 int invoke_id) = 0;
	virtual void send_open_connection_result(const cdap_rib::con_handle_t &con,
						 const cdap_rib::res_info_t &res,
						 const cdap_rib::auth_policy_t &auth,
						 int invoke_id) = 0;
	virtual void send_close_connection_result(unsigned int port,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::res_info_t &res,
						  int invoke_id) = 0;
	virtual void send_create_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id) = 0;
	virtual void send_delete_result(const cdap_rib::con_handle_t &con,
					const cdap_rib::obj_info_t &obj,
					const cdap_rib::flags_t &flags,
					const cdap_rib::res_info_t &res,
					int invoke_id) = 0;
	virtual void send_read_result(const cdap_rib::con_handle_t &con,
				      const cdap_rib::obj_info_t &obj,
				      const cdap_rib::flags_t &flags,
				      const cdap_rib::res_info_t &res,
				      int invoke_id) = 0;
	virtual void send_cancel_read_result(const cdap_rib::con_handle_t &con,
					     const cdap_rib::flags_t &flags,
					     const cdap_rib::res_info_t &res,
					     int invoke_id) = 0;
	virtual void send_write_result(const cdap_rib::con_handle_t &con,
				       const cdap_rib::flags_t &flags,
				       const cdap_rib::res_info_t &res,
				       int invoke_id) = 0;
	virtual void send_start_result(const cdap_rib::con_handle_t &con,
				       const cdap_rib::obj_info_t &obj,
				       const cdap_rib::flags_t &flags,
				       const cdap_rib::res_info_t &res,
				       int invoke_id) = 0;
	virtual void send_stop_result(const cdap_rib::con_handle_t &con,
				      const cdap_rib::flags_t &flags,
				      const cdap_rib::res_info_t &res,
				      int invoke_id) = 0;
	/// Send a cdap message previously generated
	virtual void send_cdap_result(const cdap_rib::con_handle_t &con, cdap::cdap_m_t *cdap_m) = 0;

	///
	/// Process an incoming CDAP message
	///
	virtual void process_message(ser_obj_t &message,
				     unsigned int port,
				     cdap_rib::cdap_dest_t cdap_dest = cdap_rib::CDAP_DEST_PORT) = 0;

	virtual void set_cdap_io_handler(CDAPIOHandler * handler) = 0;

	virtual CDAPIOHandler * get_cdap_io_handler() = 0;

	virtual CDAPSessionManagerInterface * get_session_manager() = 0;

	virtual void destroy_session(int port) = 0;
};

class CDAPInvokeIdManager
{
 public:
	CDAPInvokeIdManager() {};
	virtual ~CDAPInvokeIdManager() throw () {};
	virtual void freeInvokeId(int invoke_id, bool sent) = 0;
	virtual int newInvokeId(bool sent) = 0;
	virtual void reserveInvokeId(int invoke_id, bool sent) = 0;
};

class CDAPSession;
/// Implements a CDAP session manager.
class CDAPSessionManagerInterface
{
 public:
	CDAPSessionManagerInterface() { };
	virtual ~CDAPSessionManagerInterface() throw () { };
	virtual void set_timeout(long timeout) = 0;
	virtual CDAPSession* createCDAPSession(int port_id) = 0;
	virtual void getAllCDAPSessionIds(std::vector<int> &vector) = 0;
	virtual CDAPSession* get_cdap_session(int port_id) = 0;
	cdap_rib::connection_handler & get_con_handler(int port_id);
	virtual void encodeCDAPMessage(const cdap_m_t& cdap_message,
				       ser_obj_t& result) = 0;
	virtual void decodeCDAPMessage(const ser_obj_t &cdap_message,
			               cdap_m_t& result) = 0;
	virtual void removeCDAPSession(int portId) = 0;
	virtual bool session_in_await_con_state(int portId) = 0;
	virtual void encodeNextMessageToBeSent(const cdap_m_t &cdap_message,
			                       ser_obj_t& result,
			                       int port_id) = 0;
	virtual void messageReceived(const ser_obj_t &encodedcdap_m_t,
				     cdap_m_t& result,
				     int portId) = 0;
	virtual void messageSent(const cdap_m_t &cdap_message,
				 int port_id) = 0;
	virtual int get_port_id(std::string destination_application_process_name) = 0;
	virtual void
		getOpenConnectionRequestMessage(cdap_m_t & msg,
						const cdap_rib::con_handle_t &con) = 0;
	virtual void
		getOpenConnectionResponseMessage(cdap_m_t & msg,
						 const cdap_rib::con_handle_t &con,
						 const cdap_rib::res_info_t &res,
						 int invoke_id) = 0;
	virtual void
		getReleaseConnectionRequestMessage(cdap_m_t & msg,
						   const cdap_rib::flags_t &flags,
						   bool invoke_id) = 0;
	virtual void
		getReleaseConnectionResponseMessage(cdap_m_t & msg,
						    const cdap_rib::flags_t &flags,
						    const cdap_rib::res_info_t &res,
						    int invoke_id) = 0;
	virtual void getCreateObjectRequestMessage(cdap_m_t & msg,
						   const cdap_rib::filt_info_t &filt,
						   const cdap_rib::flags_t &flags,
						   const cdap_rib::obj_info_t &obj,
						   bool invoke_id) = 0;
	virtual void getCreateObjectResponseMessage(cdap_m_t & msg,
						    const cdap_rib::flags_t &flags,
						    const cdap_rib::obj_info_t &obj,
						    const cdap_rib::res_info_t &res,
						    int invoke_id) = 0;
	virtual void getDeleteObjectRequestMessage(cdap_m_t & msg,
						   const cdap_rib::filt_info_t &filt,
						   const cdap_rib::flags_t &flags,
						   const cdap_rib::obj_info_t &obj,
						   bool invoke_id) = 0;
	virtual void getDeleteObjectResponseMessage(cdap_m_t & msg,
						    const cdap_rib::flags_t &flags,
						    const cdap_rib::obj_info_t &obj,
						    const cdap_rib::res_info_t &res,
						    int invoke_id) = 0;
	virtual void getStartObjectRequestMessage(cdap_m_t & msg,
						  const cdap_rib::filt_info_t &filt,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::obj_info_t &obj,
						  bool invoke_id) = 0;
	virtual void getStartObjectResponseMessage(cdap_m_t & msg,
						   const cdap_rib::flags_t &flags,
						   const cdap_rib::res_info_t &res,
						   int invoke_id) = 0;
	virtual void getStartObjectResponseMessage(cdap_m_t & msg,
						   const cdap_rib::flags_t &flags,
						   const cdap_rib::obj_info_t &obj,
						   const cdap_rib::res_info_t &res,
						   int invoke_id) = 0;
	virtual void getStopObjectRequestMessage(cdap_m_t & msg,
						 const cdap_rib::filt_info_t &filt,
						 const cdap_rib::flags_t &flags,
						 const cdap_rib::obj_info_t &obj,
						 bool invoke_id) = 0;
	virtual void getStopObjectResponseMessage(cdap_m_t & msg,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::res_info_t &res,
						  int invoke_id) = 0;
	virtual void getReadObjectRequestMessage(cdap_m_t & msg,
						 const cdap_rib::filt_info_t &filt,
						 const cdap_rib::flags_t &flags,
						 const cdap_rib::obj_info_t &obj,
						 bool invoke_id) = 0;
	virtual void getReadObjectResponseMessage(cdap_m_t & msg,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::obj_info_t &obj,
						  const cdap_rib::res_info_t &res,
						  int invoke_id) = 0;
	virtual void getWriteObjectRequestMessage(cdap_m_t & msg,
						  const cdap_rib::filt_info_t &filt,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::obj_info_t &obj,
						  bool invoke_id) = 0;
	virtual void getWriteObjectResponseMessage(cdap_m_t & msg,
						   const cdap_rib::flags_t &flags,
						   const cdap_rib::res_info_t &res,
						   int invoke_id) = 0;
	virtual void getCancelReadRequestMessage(cdap_m_t & msg,
						 const cdap_rib::flags_t &flags,
						 int invoke_id) = 0;
	virtual void getCancelReadResponseMessage(cdap_m_t & msg,
						  const cdap_rib::flags_t &flags,
						  const cdap_rib::res_info_t &res,
						  int invoke_id) = 0;
	virtual CDAPInvokeIdManager * get_invoke_id_manager() = 0;
	virtual cdap_rib::con_handle_t & get_con_handle(int port_id) = 0;
};

//Applies SDU Protection
class SDUProtectionHandler {
public:
	SDUProtectionHandler(){};
	virtual ~SDUProtectionHandler(){};

	virtual void protect_sdu(ser_obj_t& sdu, int port_id) {};
	virtual void unprotect_sdu(ser_obj_t& sdu, int port_id) {};
};

///Implements handling of CDAP send and receive message operations
class CDAPIOHandler {
public:
	CDAPIOHandler();
	virtual ~CDAPIOHandler();
	virtual void process_message(ser_obj_t &message,
				     unsigned int port,
				     cdap_rib::cdap_dest_t cdap_dest) = 0;
	virtual void send(const cdap_m_t & m_sent,
			  const cdap_rib::con_handle_t &con) = 0;
	virtual void set_sdu_protection_handler(SDUProtectionHandler * handler);
	virtual void add_fd_to_port_id_mapping(int fd, unsigned int port_id);
	virtual void remove_fd_to_port_id_mapping(unsigned int port_id);

	CDAPSessionManagerInterface * manager_;
	CDAPCallbackInterface * callback_;
	SDUProtectionHandler * sdup_;
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
	cdap_rib::auth_policy_t auth_policy_;


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
	ser_obj_t obj_value_;

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

	bool is_request_message() const;
	std::string to_string() const;

private:
	std::string opcodeToString() const;
};

/// Provides a wire format for CDAP messages
class SerializerInterface{

public:
	virtual ~SerializerInterface(){};

	/// Convert from wire format to CDAPMessage
	/// @param message
	/// @return
	/// @throws CDAPException
	virtual void deserializeMessage(const ser_obj_t &message,
					cdap_m_t& result) = 0;
	/// Convert from CDAP messages to wire format
	/// @param cdapMessage
	/// @return
	/// @throws CDAPException
	virtual void serializeMessage(const cdap_m_t &cdapMessage,
				      ser_obj_t& result) = 0;
};

///
/// Initialize the CDAP provider
///
/// Should be only called once.
///
/// @warning This function is NOT thread safe
///
extern void init(cdap::CDAPCallbackInterface *callback,
		 cdap_rib::concrete_syntax_t& syntax,
		 bool ipcp);


/// Override the CDAP IO handler (required for IPCP)
extern void set_cdap_io_handler(cdap::CDAPIOHandler *handler);

/// Override the default SDU Protection Handler (which does nothing)
extern void set_sdu_protection_handler(SDUProtectionHandler * handler);

/// Associate a file descriptor to a port-id
extern void add_fd_to_port_id_mapping(int fd, unsigned int port_id);

/// Remove the association of a file descriptor to a port-id
extern void remove_fd_to_port_id_mapping(unsigned int port_id);

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

/// Encoder of Integers
class CDAPMessageEncoder: public Encoder<cdap_m_t>{
public:
	CDAPMessageEncoder(cdap_rib::concrete_syntax_t& syntax);
	~CDAPMessageEncoder();
	void encode(const cdap_m_t &obj, ser_obj_t& serobj);
	void decode(const ser_obj_t &serobj, cdap_m_t &des_obj);

private:
	SerializerInterface * serializer;
};

/// String encoder
class StringEncoder : public Encoder<std::string>{
public:
	void encode(const std::string &obj, ser_obj_t& serobj);
	void decode(const ser_obj_t &serobj, std::string &des_obj);

	std::string get_type() const { return "string"; };
};

/// Encoder of Integers
class IntEncoder: public Encoder<int>{
public:
	void encode(const int &obj, ser_obj_t& serobj);
	void decode(const ser_obj_t &serobj, int &des_obj);
};


} //namespace cdap
} //namespace rina

#endif /* CDAP_PROVIDER_H_ */
