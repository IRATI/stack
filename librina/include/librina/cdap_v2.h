/*
 * CDAP North bound API
 *
 *    Bernat Gastón <bernat.gaston@i2cat.net>
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

namespace rina{
namespace cdap {

// Interface of the RIB to be used from the communication protocol
class CDAPCallbackInterface
{
 public:
  virtual ~CDAPCallbackInterface();
  virtual void open_connection_result(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::result_info &res);
  virtual void open_connection(const cdap_rib::con_handle_t &con,
                               const cdap_rib::flags_t &flags,
                               int message_id);
  virtual void close_connection_result(const cdap_rib::con_handle_t &con,
                                       const cdap_rib::result_info &res);
  virtual void close_connection(const cdap_rib::con_handle_t &con,
                                const cdap_rib::flags_t &flags,
                                int message_id);

  virtual void remote_create_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::res_info_t &res);
  virtual void remote_delete_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::res_info_t &res);
  virtual void remote_read_result(const cdap_rib::con_handle_t &con,
                                  const cdap_rib::res_info_t &res);
  virtual void remote_cancel_read_result(const cdap_rib::con_handle_t &con,
                                         const cdap_rib::res_info_t &res);
  virtual void remote_write_result(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::res_info_t &res);
  virtual void remote_start_result(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::res_info_t &res);
  virtual void remote_stop_result(const cdap_rib::con_handle_t &con,
                                  const cdap_rib::res_info_t &res);

  virtual void remote_create_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt,
                                     int message_id);
  virtual void remote_delete_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt,
                                     int message_id);
  virtual void remote_read_request(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::obj_info_t &obj,
                                   const cdap_rib::filt_info_t &filt,
                                   int message_id);
  virtual void remote_cancel_read_request(const cdap_rib::con_handle_t &con,
                                          const cdap_rib::obj_info_t &obj,
                                          const cdap_rib::filt_info_t &filt,
                                          int message_id);
  virtual void remote_write_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt,
                                    int message_id);
  virtual void remote_start_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt,
                                    int message_id);
  virtual void remote_stop_request(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::obj_info_t &obj,
                                   const cdap_rib::filt_info_t &filt,
                                   int message_id);
};

class CDAPProviderInterface
{
 public:
  virtual ~CDAPProviderInterface()
  {
  }
  ;
  virtual cdap_rib::con_handle_t remote_open_connection(
      const cdap_rib::vers_info_t &ver, const cdap_rib::src_info_t &src,
      const cdap_rib::dest_info_t &dest, const cdap_rib::auth_info &auth,
      int port) = 0;
  virtual int remote_close_connection(int port_id) = 0;
  virtual int remote_create(const cdap_rib::con_handle_t &con,
                            const cdap_rib::obj_info_t &obj,
                            const cdap_rib::flags_t &flags,
                            const cdap_rib::filt_info_t &filt) = 0;
  virtual int remote_delete(const cdap_rib::con_handle_t &con,
                            const cdap_rib::obj_info_t &obj,
                            const cdap_rib::flags_t &flags,
                            const cdap_rib::filt_info_t &filt) = 0;
  virtual int remote_read(const cdap_rib::con_handle_t &con,
                          const cdap_rib::obj_info_t &obj,
                          const cdap_rib::flags_t &flags,
                          const cdap_rib::filt_info_t &filt)= 0;
  virtual int remote_cancel_read(const cdap_rib::con_handle_t &con,
                                 const cdap_rib::flags_t &flags,
                                 int invoke_id) = 0;
  virtual int remote_write(const cdap_rib::con_handle_t &con,
                           const cdap_rib::obj_info_t &obj,
                           const cdap_rib::flags_t &flags,
                           const cdap_rib::filt_info_t &filt) = 0;
  virtual int remote_start(const cdap_rib::con_handle_t &con,
                           const cdap_rib::obj_info_t &obj,
                           const cdap_rib::flags_t &flags,
                           const cdap_rib::filt_info_t &filt) = 0;
  virtual int remote_stop(const cdap_rib::con_handle_t &con,
                          const cdap_rib::obj_info_t &obj,
                          const cdap_rib::flags_t &flags,
                          const cdap_rib::filt_info_t &filt) = 0;
  virtual void open_connection_response(const cdap_rib::con_handle_t &con,
                                        const cdap_rib::res_info_t &res,
                                        int message_id) = 0;
  virtual void close_connection_response(const cdap_rib::con_handle_t &con,
                                         const cdap_rib::flags_t &flags,
                                         const cdap_rib::res_info_t &res,
                                         int message_id) = 0;
  virtual void remote_create_response(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::obj_info_t &obj,
                                      const cdap_rib::flags_t &flags,
                                      const cdap_rib::res_info_t &res,
                                      int message_id) = 0;
  virtual void remote_delete_response(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::obj_info_t &obj,
                                      const cdap_rib::flags_t &flags,
                                      const cdap_rib::res_info_t &res,
                                      int message_id) = 0;
  virtual void remote_read_response(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::flags_t &flags,
                                    const cdap_rib::res_info_t &res,
                                    int message_id) = 0;
  virtual void remote_cancel_read_response(const cdap_rib::con_handle_t &con,
                                           const cdap_rib::flags_t &flags,
                                           const cdap_rib::res_info_t &res,
                                           int message_id) = 0;
  virtual void remote_write_response(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::flags_t &flags,
                                     const cdap_rib::res_info_t &res,
                                     int message_id) = 0;
  virtual void remote_start_response(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::flags_t &flags,
                                     const cdap_rib::res_info_t &res,
                                     int message_id) = 0;
  virtual void remote_stop_response(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::flags_t &flags,
                                    const cdap_rib::res_info_t &res,
                                    int message_id) = 0;
  virtual void process_message(cdap_rib::SerializedObject &message, int port) = 0;
};

typedef struct CDAPMessage
{
 public:
  enum Opcode
  {
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
  /// AuthenticationMechanismName (authtypes), optional, not validated by CDAP.
  /// Identification of the method to be used by the destination application to
  /// authenticate the source application
  cdap_rib::auth_info_t::AuthTypes auth_mech_;
  /// AuthenticationValue (authvalue), optional, not validated by CDAP.
  /// Authentication information accompanying auth_mech, format and value
  /// appropiate to the selected auth_mech
  rina::AuthValue auth_value_;
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
  CDAPMessage();
} cdap_m_t;


/// Provides a wire format for CDAP messages
class SerializerInterface {
public:
  virtual ~SerializerInterface() {
  }
  ;
  /// Convert from wire format to CDAPMessage
  /// @param message
  /// @return
  /// @throws CDAPException
  virtual const cdap_m_t* deserializeMessage(const cdap_rib::SerializedObject &message) = 0;
  /// Convert from CDAP messages to wire format
  /// @param cdapMessage
  /// @return
  /// @throws CDAPException
  virtual const cdap_rib::SerializedObject* serializeMessage(const cdap_m_t &cdapMessage) = 0;
};

namespace CDAPProviderFactory{

extern void init(long timeout);
extern CDAPProviderInterface* create(bool is_IPCP, cdap::CDAPCallbackInterface *callback);
extern void destroy(int port);
extern void finit();
}

}
}
#endif /* CDAP_PROVIDER_H_ */
