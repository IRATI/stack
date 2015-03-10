/*
 * CDAP North bound API
 *
 *    Bernat Gastón <bernat.gaston@i2cat.net>
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

#include "cdap_provider.h"
#include <librina/cdap.h>
#include <librina/application.h>

namespace cdap {

class CDAPProvider : public CDAPProviderInterface
{
 public:
  CDAPProvider(rina::WireMessageProviderFactory wire_provider_factory,
               long timeout);
  cdap_rib::con_handle_t open_connection(const cdap_rib::vers_info_t ver,
                                         const cdap_rib::src_info_t &src,
                                         const cdap_rib::dest_info_t &dest,
                                         const cdap_rib::auth_info &auth,
                                         int port);
  int close_connection(cdap_rib::con_handle_t &con);
  int remote_create(const cdap_rib::con_handle_t &con,
                    const cdap_rib::obj_info_t &obj,
                    const cdap_rib::flags_t &flags,
                    const cdap_rib::filt_info_t &filt);
  int remote_delete(const cdap_rib::con_handle_t &con,
                    const cdap_rib::obj_info_t &obj,
                    const cdap_rib::flags_t &flags,
                    const cdap_rib::filt_info_t &filt);
  int remote_read(const cdap_rib::con_handle_t &con,
                  const cdap_rib::obj_info_t &obj,
                  const cdap_rib::flags_t &flags,
                  const cdap_rib::filt_info_t &filt);
  int remote_cancel_read(const cdap_rib::con_handle_t &con,
                         const cdap_rib::flags_t &flags, int invoke_id);
  int remote_write(const cdap_rib::con_handle_t &con,
                   const cdap_rib::obj_info_t &obj,
                   const cdap_rib::flags_t &flags,
                   const cdap_rib::filt_info_t &filt);
  int remote_start(const cdap_rib::con_handle_t &con,
                   const cdap_rib::obj_info_t &obj,
                   const cdap_rib::flags_t &flags,
                   const cdap_rib::filt_info_t &filt);
  int remote_stop(const cdap_rib::con_handle_t &con,
                  const cdap_rib::obj_info_t &obj,
                  const cdap_rib::flags_t &flags,
                  const cdap_rib::filt_info_t &filt);
  void remote_create_response(const cdap_rib::con_handle_t &con,
                              const cdap_rib::obj_info_t &obj,
                              const cdap_rib::flags_t &flags,
                              const cdap_rib::res_info_t &res, int message_id);
  void remote_delete_response(const cdap_rib::con_handle_t &con,
                              const cdap_rib::obj_info_t &obj,
                              const cdap_rib::flags_t &flags,
                              const cdap_rib::res_info_t &res, int message_id);
  void remote_read_response(const cdap_rib::con_handle_t &con,
                            const cdap_rib::obj_info_t &obj,
                            const cdap_rib::flags_t &flags,
                            const cdap_rib::res_info_t &res, int message_id);
  void remote_cancel_read_response(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::flags_t &flags,
                                   const cdap_rib::res_info_t &res,
                                   int message_id);
  void remote_write_response(const cdap_rib::con_handle_t &con,
                             const cdap_rib::flags_t &flags,
                             const cdap_rib::res_info_t &res, int message_id);
  void remote_start_response(const cdap_rib::con_handle_t &con,
                             const cdap_rib::obj_info_t &obj,
                             const cdap_rib::flags_t &flags,
                             const cdap_rib::res_info_t &res, int message_id);
  void remote_stop_response(const cdap_rib::con_handle_t &con,
                            const cdap_rib::flags_t &flags,
                            const cdap_rib::res_info_t &res, int message_id);
 protected:
  rina::CDAPSessionManagerInterface *manager_;
 private:
  virtual void send(const rina::CDAPMessage *m_sent, int port) = 0;
};

CDAPProvider::CDAPProvider(
    rina::WireMessageProviderFactory wire_provider_factory, long timeout)
{
  rina::CDAPSessionManagerFactory cdap_factory;
  manager_ = cdap_factory.createCDAPSessionManager(&wire_provider_factory,
                                                   timeout);
}

cdap_rib::con_handle_t CDAPProvider::open_connection(
    const cdap_rib::vers_info_t ver, const cdap_rib::src_info_t &src,
    const cdap_rib::dest_info_t &dest, const cdap_rib::auth_info &auth,
    int port)
{
  const rina::CDAPMessage *m_sent;
  cdap_rib::con_handle_t con;
  rina::AuthValue *value;

  con.port_ = port;
  con.version_ = ver;
  con.src_ = src;
  con.dest_ = dest;
  con.auth_ = auth;

  switch (auth.auth_mech_) {
    case cdap_rib::auth_info::AUTH_PASSWD:
    case cdap_rib::auth_info::AUTH_SSHDSA:
    case cdap_rib::auth_info::AUTH_SSHRSA:
      value = new rina::AuthValue(auth.auth_name_, auth.auth_password_,
                                  auth.auth_other_);
      break;
    case cdap_rib::auth_info::AUTH_NONE:
    default:
      value = new rina::AuthValue();
      break;
  }
  // FIXME erase auth and flags from CDAPMessage
  m_sent = manager_->getOpenConnectionRequestMessage(
      port, rina::CDAPMessage::AUTH_NONE, *value, dest.dest_ae_inst_,
      dest.dest_ae_name_, dest.dest_ap_inst_, dest.dest_ap_name_,
      src.src_ae_inst_, src.src_ae_name_, src.src_ap_inst_, src.src_ap_name_);
  send(m_sent, con.port_);

  delete value;

  return con;
}

int CDAPProvider::close_connection(cdap_rib::con_handle_t &con)
{
  int invoke_id;
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  m_sent = manager_->getReleaseConnectionRequestMessage(
      con.port_, rina::CDAPMessage::NONE_FLAGS, true);
  invoke_id = m_sent->get_invoke_id();
  send(m_sent, con.port_);

  delete m_sent;

  return invoke_id;
}
int CDAPProvider::remote_create(const cdap_rib::con_handle_t &con,
                                const cdap_rib::obj_info_t &obj,
                                const cdap_rib::flags_t &flags,
                                const cdap_rib::filt_info_t &filt)
{
  int invoke_id;
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getCreateObjectRequestMessage(
      con.port_, filt.filter_, rina::CDAPMessage::NONE_FLAGS, obj.class_,
      obj.inst_, obj.name_, filt.scope_, true);
  invoke_id = m_sent->get_invoke_id();
  send(m_sent, con.port_);

  delete m_sent;

  return invoke_id;
}
int CDAPProvider::remote_delete(const cdap_rib::con_handle_t &con,
                                const cdap_rib::obj_info_t &obj,
                                const cdap_rib::flags_t &flags,
                                const cdap_rib::filt_info_t &filt)
{
  int invoke_id;
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getDeleteObjectRequestMessage(
      con.port_, filt.filter_, rina::CDAPMessage::NONE_FLAGS, obj.class_,
      obj.inst_, obj.name_, filt.scope_, true);
  invoke_id = m_sent->get_invoke_id();
  send(m_sent, con.port_);

  delete m_sent;

  return invoke_id;
}
int CDAPProvider::remote_read(const cdap_rib::con_handle_t &con,
                              const cdap_rib::obj_info_t &obj,
                              const cdap_rib::flags_t &flags,
                              const cdap_rib::filt_info_t &filt)
{
  int invoke_id;
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getReadObjectRequestMessage(con.port_, filt.filter_,
                                                 rina::CDAPMessage::NONE_FLAGS,
                                                 obj.class_, obj.inst_,
                                                 obj.name_, filt.scope_, true);
  invoke_id = m_sent->get_invoke_id();
  send(m_sent, con.port_);

  delete m_sent;

  return invoke_id;
}
int CDAPProvider::remote_cancel_read(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::flags_t &flags,
                                     int invoke_id)
{
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getCancelReadRequestMessage(rina::CDAPMessage::NONE_FLAGS,
                                                 invoke_id);
  send(m_sent, con.port_);

  delete m_sent;

  return invoke_id;
}
int CDAPProvider::remote_write(const cdap_rib::con_handle_t &con,
                               const cdap_rib::obj_info_t &obj,
                               const cdap_rib::flags_t &flags,
                               const cdap_rib::filt_info_t &filt)
{
  int invoke_id;
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getWriteObjectRequestMessage(con.port_, filt.filter_,
                                                  rina::CDAPMessage::NONE_FLAGS,
                                                  obj.class_, obj.inst_,
                                                  obj.name_, filt.scope_, true);
  invoke_id = m_sent->get_invoke_id();
  send(m_sent, con.port_);

  delete m_sent;

  return invoke_id;
}
int CDAPProvider::remote_start(const cdap_rib::con_handle_t &con,
                               const cdap_rib::obj_info_t &obj,
                               const cdap_rib::flags_t &flags,
                               const cdap_rib::filt_info_t &filt)
{
  int invoke_id;
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getStartObjectRequestMessage(con.port_, filt.filter_,
                                                  rina::CDAPMessage::NONE_FLAGS,
                                                  obj.class_, obj.inst_,
                                                  obj.name_, filt.scope_, true);
  invoke_id = m_sent->get_invoke_id();
  send(m_sent, con.port_);

  delete m_sent;

  return invoke_id;
}
int CDAPProvider::remote_stop(const cdap_rib::con_handle_t &con,
                              const cdap_rib::obj_info_t &obj,
                              const cdap_rib::flags_t &flags,
                              const cdap_rib::filt_info_t &filt)
{
  int invoke_id;
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getStopObjectRequestMessage(con.port_, filt.filter_,
                                                 rina::CDAPMessage::NONE_FLAGS,
                                                 obj.class_, obj.inst_,
                                                 obj.name_, filt.scope_, true);
  invoke_id = m_sent->get_invoke_id();
  send(m_sent, con.port_);

  delete m_sent;

  return invoke_id;
}
void CDAPProvider::remote_create_response(const cdap_rib::con_handle_t &con,
                                          const cdap_rib::obj_info_t &obj,
                                          const cdap_rib::flags_t &flags,
                                          const cdap_rib::res_info_t &res,
                                          int message_id)
{
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getCreateObjectResponseMessage(
      rina::CDAPMessage::NONE_FLAGS, obj.class_, obj.inst_, obj.name_,
      res.result_, res.result_reason_, message_id);
  send(m_sent, con.port_);

  delete m_sent;
}
void CDAPProvider::remote_delete_response(const cdap_rib::con_handle_t &con,
                                          const cdap_rib::obj_info_t &obj,
                                          const cdap_rib::flags_t &flags,
                                          const cdap_rib::res_info_t &res,
                                          int message_id)
{
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getDeleteObjectResponseMessage(
      rina::CDAPMessage::NONE_FLAGS, obj.class_, obj.inst_, obj.name_,
      res.result_, res.result_reason_, message_id);
  send(m_sent, con.port_);

  delete m_sent;
}
void CDAPProvider::remote_read_response(const cdap_rib::con_handle_t &con,
                                        const cdap_rib::obj_info_t &obj,
                                        const cdap_rib::flags_t &flags,
                                        const cdap_rib::res_info_t &res,
                                        int message_id)
{
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getReadObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
                                                  obj.class_, obj.inst_,
                                                  obj.name_, res.result_,
                                                  res.result_reason_,
                                                  message_id);
  send(m_sent, con.port_);

  delete m_sent;
}
void CDAPProvider::remote_cancel_read_response(
    const cdap_rib::con_handle_t &con, const cdap_rib::flags_t &flags,
    const cdap_rib::res_info_t &res, int message_id)
{
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getCancelReadResponseMessage(rina::CDAPMessage::NONE_FLAGS,
                                                  message_id, res.result_,
                                                  res.result_reason_);
  send(m_sent, con.port_);

  delete m_sent;
}
void CDAPProvider::remote_write_response(const cdap_rib::con_handle_t &con,
                                         const cdap_rib::flags_t &flags,
                                         const cdap_rib::res_info_t &res,
                                         int message_id)
{
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getWriteObjectResponseMessage(
      rina::CDAPMessage::NONE_FLAGS, res.result_, res.result_reason_,
      message_id);
  send(m_sent, con.port_);

  delete m_sent;
}
void CDAPProvider::remote_start_response(const cdap_rib::con_handle_t &con,
                                         const cdap_rib::obj_info_t &obj,
                                         const cdap_rib::flags_t &flags,
                                         const cdap_rib::res_info_t &res,
                                         int message_id)
{
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getStartObjectResponseMessage(
      rina::CDAPMessage::NONE_FLAGS, obj.class_, obj.inst_, obj.name_,
      res.result_, res.result_reason_, message_id);
  send(m_sent, con.port_);

  delete m_sent;
}
void CDAPProvider::remote_stop_response(const cdap_rib::con_handle_t &con,
                                        const cdap_rib::flags_t &flags,
                                        const cdap_rib::res_info_t &res,
                                        int message_id)
{
  const rina::CDAPMessage *m_sent;

  // FIXME change CDAPMessage::NONE_FLAGS
  (void) flags;
  m_sent = manager_->getStopObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
                                                  res.result_,
                                                  res.result_reason_,
                                                  message_id);
  send(m_sent, con.port_);

  delete m_sent;
}

class AppCDAPProvider : public CDAPProvider
{
 public:
  AppCDAPProvider(rina::WireMessageProviderFactory wire_provider_factory,
                  long timeout);
 private:
  void send(const rina::CDAPMessage *m_sent, int port);
};

AppCDAPProvider::AppCDAPProvider(
    rina::WireMessageProviderFactory wire_provider_factory, long timeout)
    : CDAPProvider(wire_provider_factory, timeout)
{
}

void AppCDAPProvider::send(const rina::CDAPMessage *m_sent, int port)
{
  const rina::SerializedObject *ser_sent_m =
      manager_->encodeNextMessageToBeSent(*m_sent, port);
  manager_->messageSent(*m_sent, port);
  rina::ipcManager->getAllocatedFlow(port)->writeSDU(ser_sent_m->message_,
                                                     ser_sent_m->size_);
  delete ser_sent_m;
}

class IPCPCDAPProvider : public CDAPProvider
{
 public:
  IPCPCDAPProvider(rina::WireMessageProviderFactory wire_provider_factory,
                   long timeout);
 private:
  void send(const rina::CDAPMessage *m_sent, int port);
};

IPCPCDAPProvider::IPCPCDAPProvider(
    rina::WireMessageProviderFactory wire_provider_factory, long timeout)
    : CDAPProvider(wire_provider_factory, timeout)
{
}

void IPCPCDAPProvider::send(const rina::CDAPMessage *m_sent, int port)
{
  const rina::SerializedObject *ser_sent_m =
      manager_->encodeNextMessageToBeSent(*m_sent, port);
  manager_->messageSent(*m_sent, port);
  rina::kernelIPCProcess->writeMgmgtSDUToPortId(ser_sent_m->message_,
                                                ser_sent_m->size_, port);
  delete ser_sent_m;
}

CDAPProviderInterface* CDAPProviderFactory::create(
    const std::string &comm_protocol, long timeout, bool is_IPCP)
{
  (void) comm_protocol;
  // FIXME: call wire_provider_factory with a std::string and make a switch inside
  rina::WireMessageProviderFactory wire_provider_factory;
  // FIXME: remove wire_provider_factory as a member variable of CDAPManager
  if (is_IPCP)
    return new IPCPCDAPProvider(wire_provider_factory, timeout);
  else
    return new AppCDAPProvider(wire_provider_factory, timeout);
}

}
