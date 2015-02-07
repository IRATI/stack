//
// RIB Daemon
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#define RINA_PREFIX "rib-daemon"

#include <librina/logs.h>
#include <librina/common.h>
#include "rib-daemon.h"

namespace rinad {

//Class ManagementSDUReader data
ManagementSDUReaderData::ManagementSDUReaderData(IPCPRIBDaemon * rib_daemon,
                                                 unsigned int max_sdu_size)
{
  rib_daemon_ = rib_daemon;
  max_sdu_size_ = max_sdu_size;
}

void * doManagementSDUReaderWork(void* arg)
{
  ManagementSDUReaderData * data = (ManagementSDUReaderData *) arg;
  char* buffer = new char[data->max_sdu_size_];
  char* sdu;

  rina::ReadManagementSDUResult result;
  LOG_INFO("Starting Management SDU reader ...");
  while (true) {
    try {
      result = rina::kernelIPCProcess->readManagementSDU(buffer,
                                                         data->max_sdu_size_);
    } catch (Exception &e) {
      LOG_ERR("Problems reading management SDU: %s", e.what());
      continue;
    }

    sdu = new char[result.getBytesRead()];
    for (int i = 0; i < result.getBytesRead(); i++) {
      sdu[i] = buffer[i];
    }

    data->rib_daemon_->cdapMessageDelivered(sdu, result.getBytesRead(),
                                            result.getPortId());
  }

  delete buffer;

  return 0;
}

// Class BaseRIBDaemon
BaseRIBDaemon::BaseRIBDaemon(const rina::RIBSchema *rib_schema)
    : IPCPRIBDaemon(rib_schema)
{
}

void BaseRIBDaemon::subscribeToEvent(const IPCProcessEventType& eventId,
                                     EventListener * eventListener)
{
  if (!eventListener)
    return;

  events_lock_.lock();

  std::map<IPCProcessEventType, std::list<EventListener*> >::iterator it =
      event_listeners_.find(eventId);
  if (it == event_listeners_.end()) {
    std::list<EventListener *> listenersList;
    listenersList.push_back(eventListener);
    event_listeners_[eventId] = listenersList;
  } else {
    std::list<EventListener *>::iterator listIterator;
    for (listIterator = it->second.begin(); listIterator != it->second.end();
        ++listIterator) {
      if (*listIterator == eventListener) {
        events_lock_.unlock();
        return;
      }
    }

    it->second.push_back(eventListener);
  }

  LOG_INFO("EventListener subscribed to event %s",
           BaseEvent::eventIdToString(eventId).c_str());
  events_lock_.unlock();
}

void BaseRIBDaemon::unsubscribeFromEvent(const IPCProcessEventType& eventId,
                                         EventListener * eventListener)
{
  if (!eventListener)
    return;

  events_lock_.lock();
  std::map<IPCProcessEventType, std::list<EventListener*> >::iterator it =
      event_listeners_.find(eventId);
  if (it == event_listeners_.end()) {
    events_lock_.unlock();
    return;
  }

  it->second.remove(eventListener);
  if (it->second.size() == 0) {
    event_listeners_.erase(it);
  }

  LOG_INFO("EventListener unsubscribed from event %s",
           BaseEvent::eventIdToString(eventId).c_str());
  events_lock_.unlock();
}

void BaseRIBDaemon::deliverEvent(Event * event)
{
  if (!event)
    return;

  LOG_INFO("Event %s has just happened. Notifying event listeners.",
           BaseEvent::eventIdToString(event->get_id()).c_str());

  events_lock_.lock();
  std::map<IPCProcessEventType, std::list<EventListener*> >::iterator it =
      event_listeners_.find(event->get_id());
  if (it == event_listeners_.end()) {
    events_lock_.unlock();
    delete event;
    return;
  }

  events_lock_.unlock();
  std::list<EventListener *>::iterator listIterator;
  for (listIterator = it->second.begin(); listIterator != it->second.end();
      ++listIterator) {
    (*listIterator)->eventHappened(event);
  }

  if (event) {
    delete event;
  }
}

///Class RIBDaemon
IPCPRIBDaemonImpl::IPCPRIBDaemonImpl(const rina::RIBSchema *rib_schema)
    : BaseRIBDaemon(rib_schema)
{
  ipc_process_ = 0;
  management_sdu_reader_ = 0;
  n_minus_one_flow_manager_ = 0;
}

void IPCPRIBDaemonImpl::set_ipc_process(IPCProcess * ipc_process)
{
  initialize(ipc_process->encoder_, ipc_process->cdap_session_manager_,
             ipc_process->enrollment_task_);
  ipc_process_ = ipc_process;
  n_minus_one_flow_manager_ = ipc_process->resource_allocator_
      ->get_n_minus_one_flow_manager();

  subscribeToEvents();

  rina::ThreadAttributes * threadAttributes = new rina::ThreadAttributes();
  threadAttributes->setJoinable();
  ManagementSDUReaderData * data = new ManagementSDUReaderData(
      this, max_sdu_size_in_bytes);
  management_sdu_reader_ = new rina::Thread(threadAttributes,
                                            &doManagementSDUReaderWork,
                                            (void *) data);
}

void IPCPRIBDaemonImpl::set_dif_configuration(
    const rina::DIFConfiguration& dif_configuration)
{
  LOG_DBG("Configuration set: %u", dif_configuration.address_);
}

void IPCPRIBDaemonImpl::subscribeToEvents()
{
  subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED, this);
  subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED, this);
}

void IPCPRIBDaemonImpl::eventHappened(Event * event)
{
  if (!event)
    return;

  if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED) {
    NMinusOneFlowDeallocatedEvent * flowEvent =
        (NMinusOneFlowDeallocatedEvent *) event;
    nMinusOneFlowDeallocated(flowEvent->port_id_);
  } else if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED) {
    NMinusOneFlowAllocatedEvent * flowEvent =
        (NMinusOneFlowAllocatedEvent *) event;
    nMinusOneFlowAllocated(flowEvent);
  }
}

void IPCPRIBDaemonImpl::nMinusOneFlowDeallocated(int portId)
{
  rina::CDAPSessionManagerInterface * cdsm = ipc_process_->cdap_session_manager_;
  cdsm->removeCDAPSession(portId);
}

void IPCPRIBDaemonImpl::nMinusOneFlowAllocated(
    NMinusOneFlowAllocatedEvent * event)
{
  if (!event)
    return;
}

void IPCPRIBDaemonImpl::processQueryRIBRequestEvent(
    const rina::QueryRIBRequestEvent& event)
{
  std::list<rina::RIBObjectData> result = getRIBObjectsData();

  std::list<rina::RIBObjectData>::iterator it;
  for (it = result.begin(); it != result.end(); ++it) {
    LOG_DBG("Object name: %s", it->name_.c_str());
  }

  try {
    rina::extendedIPCManager->queryRIBResponse(event, 0, result);
  } catch (Exception &e) {
    LOG_ERR("Problems sending query RIB response to IPC Manager: %s", e.what());
  }
}

void IPCPRIBDaemonImpl::sendMessageSpecific(
    const rina::RemoteProcessId &remote_proc,
    const rina::CDAPMessage & cdapMessage,
    rina::ICDAPResponseMessageHandler *cdapMessageHandler)
{
  const rina::SerializedObject * sdu;
  rina::CDAPSessionManagerInterface * cdsm = ipc_process_->cdap_session_manager_;

  if (!cdapMessageHandler && cdapMessage.get_invoke_id() != 0
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_CONNECT
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_CONNECT_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_RELEASE
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_RELEASE_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_CANCELREAD_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_WRITE_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_READ_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_CREATE_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_DELETE_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_START_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_STOP_R) {
    throw Exception("Requested a response message but message handler is null");
  }

  atomic_send_lock_.lock();
  sdu = 0;
  try {
    sdu = cdsm->encodeNextMessageToBeSent(cdapMessage, remote_proc.port_id_);
    if (remote_proc.use_address_) {
      rina::kernelIPCProcess->sendMgmgtSDUToAddress(sdu->get_message(),
                                                    sdu->get_size(),
                                                    remote_proc.address_);
      LOG_DBG("Sent CDAP message to address %ud: %s",
              remote_proc.address_, cdapMessage.to_string().c_str());
    } else {
      rina::kernelIPCProcess->writeMgmgtSDUToPortId(sdu->get_message(),
                                                    sdu->get_size(),
                                                    remote_proc.port_id_);
      LOG_DBG(
          "Sent CDAP message of size %d through port-id %d: %s",
          sdu->get_size(), remote_proc.port_id_, cdapMessage.to_string().c_str());
    }

    cdsm->messageSent(cdapMessage, remote_proc.port_id_);
  } catch (Exception &e) {
    if (sdu) {
      delete sdu;
    }

    std::string reason = std::string(e.what());
    if (reason.compare("Flow closed") == 0) {
      cdsm->removeCDAPSession(remote_proc.port_id_);
    }

    atomic_send_lock_.unlock();

    throw e;
  }

  delete sdu;

  if (cdapMessage.get_invoke_id() != 0
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_CONNECT
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_CONNECT_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_RELEASE
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_RELEASE_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_CANCELREAD_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_WRITE_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_READ_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_CREATE_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_DELETE_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_START_R
      && cdapMessage.get_op_code() != rina::CDAPMessage::M_STOP_R) {
    handlers_waiting_for_reply_.put(cdapMessage.get_invoke_id(),
                                    cdapMessageHandler);
  }

  atomic_send_lock_.unlock();
}

//CLASS SimplestRIBObj
SimplestRIBObj::SimplestRIBObj(rina::IRIBDaemon *rib_daemon,  const std::string& object_class, const std::string& object_name):
    rina::BaseRIBObject(rib_daemon, object_class, rina::objectInstanceGenerator->getObjectInstance(), object_name)
{}
const void* SimplestRIBObj::get_value() const
{
  return 0;
}


}
