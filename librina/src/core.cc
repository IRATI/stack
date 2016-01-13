//
// Core
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <sstream>
#include <unistd.h>

#define RINA_PREFIX "librina.core"

#include "librina/logs.h"
#include "core.h"

namespace rina {

char * stringToCharArray(std::string s)
{
  char * result = new char[s.size() + 1];
  result[s.size()] = 0;
  memcpy(result, s.c_str(), s.size());
  return result;
}

char * intToCharArray(int i)
{
  std::stringstream strs;
  strs << i;
  return stringToCharArray(strs.str());
}

/* CLASS RINA NETLINK ENDPOINT */
RINANetlinkEndpoint::RINANetlinkEndpoint()
{
  netlinkPortId = 0;
  ipcProcessId = 0;
}

RINANetlinkEndpoint::RINANetlinkEndpoint(unsigned int netlinkPortId,
                                         unsigned short ipcProcessId)
{
  this->netlinkPortId = netlinkPortId;
  this->ipcProcessId = ipcProcessId;
}

RINANetlinkEndpoint::RINANetlinkEndpoint(
    unsigned int netlinkPortId, unsigned short ipcProcessId,
    const ApplicationProcessNamingInformation& appNamingInfo)
{
  this->netlinkPortId = netlinkPortId;
  this->ipcProcessId = ipcProcessId;
  this->applicationProcessName = appNamingInfo;
}

unsigned short RINANetlinkEndpoint::getIpcProcessId() const
{
  return ipcProcessId;
}

void RINANetlinkEndpoint::setIpcProcessId(unsigned short ipcProcessId)
{
  this->ipcProcessId = ipcProcessId;
}

unsigned int RINANetlinkEndpoint::getNetlinkPortId() const
{
  return netlinkPortId;
}

void RINANetlinkEndpoint::setNetlinkPortId(unsigned int netlinkPortId)
{
  this->netlinkPortId = netlinkPortId;
}

const ApplicationProcessNamingInformation&
RINANetlinkEndpoint::getApplicationProcessName() const
{
  return applicationProcessName;
}

void RINANetlinkEndpoint::setApplicationProcessName(
    const ApplicationProcessNamingInformation& applicationProcessName)
{
  this->applicationProcessName = applicationProcessName;
}


/* CLASS NETLINK PORT ID MAP */
NetlinkPortIdMap::~NetlinkPortIdMap(){
        for (std::map<unsigned short, RINANetlinkEndpoint *>::iterator iterator
                        = ipcProcessIdMappings.begin();
                        iterator != ipcProcessIdMappings.end(); ++iterator) {
                if (iterator->second) {
                        delete iterator->second;
                }
        }
        for (std::map<std::string, RINANetlinkEndpoint*>::iterator iterator
                  = applicationNameMappings.begin();
                  iterator != applicationNameMappings.end(); ++iterator) {
                if (iterator->second) {
                        delete iterator->second;
                }
        }
}

void NetlinkPortIdMap::putIPCProcessIdToNelinkPortIdMapping(
    unsigned int netlinkPortId, unsigned short ipcProcessId)
{
  RINANetlinkEndpoint * current = ipcProcessIdMappings[ipcProcessId];
  if (current != 0) {
    current->setIpcProcessId(ipcProcessId);
    current->setNetlinkPortId(netlinkPortId);
  } else {
    ipcProcessIdMappings[ipcProcessId] = new RINANetlinkEndpoint(netlinkPortId,
                                                                 ipcProcessId);
  }
}

RINANetlinkEndpoint * NetlinkPortIdMap::getNetlinkPortIdFromIPCProcessId(
    unsigned short ipcProcessId)
{
  std::map<unsigned short, RINANetlinkEndpoint *>::iterator it =
      ipcProcessIdMappings.find(ipcProcessId);
  if (it == ipcProcessIdMappings.end()) {
    LOG_ERR("Could not find the netlink endpoint of IPC Process %d",
            ipcProcessId);
    throw NetlinkException(NetlinkException::error_fetching_netlink_port_id);
  }

  return it->second;
}

void NetlinkPortIdMap::putAPNametoNetlinkPortIdMapping(
    ApplicationProcessNamingInformation apName, unsigned int netlinkPortId,
    unsigned short ipcProcessId)
{
  RINANetlinkEndpoint * current = applicationNameMappings[apName
      .getProcessNamePlusInstance()];
  if (current != 0) {
    current->setIpcProcessId(ipcProcessId);
    current->setNetlinkPortId(netlinkPortId);
  } else {
    applicationNameMappings[apName.getProcessNamePlusInstance()] =
        new RINANetlinkEndpoint(netlinkPortId, ipcProcessId, apName);
  }
}

RINANetlinkEndpoint * NetlinkPortIdMap::getNetlinkPortIdFromAPName(
    ApplicationProcessNamingInformation apName)
{
  std::map<std::string, RINANetlinkEndpoint *>::iterator it =
      applicationNameMappings.find(apName.getProcessNamePlusInstance());
  if (it == applicationNameMappings.end()) {
    LOG_ERR("Could not find the netlink endpoint of Application %s",
            apName.toString().c_str());
    throw NetlinkException(NetlinkException::error_fetching_netlink_port_id);
  };

  return it->second;
}

unsigned int NetlinkPortIdMap::getIPCManagerPortId()
{
  return 1;
}

void NetlinkPortIdMap::updateMessageOrPortIdMap(BaseNetlinkMessage* message,
                                                bool send)
{
  switch (message->getOperationCode()) {
    case RINA_C_APP_ALLOCATE_FLOW_REQUEST: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      } else {
        AppAllocateFlowRequestMessage * specificMessage =
            dynamic_cast<AppAllocateFlowRequestMessage *>(message);
        putAPNametoNetlinkPortIdMapping(
            specificMessage->getSourceAppName(),
            specificMessage->getSourcePortId(),
            specificMessage->getSourceIpcProcessId());
      }
      break;
    }
    case RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT: {
      AppAllocateFlowRequestResultMessage * specificMessage =
          dynamic_cast<AppAllocateFlowRequestResultMessage *>(message);
      if (send) {
        RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
            specificMessage->getSourceAppName());
        specificMessage->setDestPortId(endpoint->getNetlinkPortId());
        specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
      }
      break;
    }
    case RINA_C_APP_ALLOCATE_FLOW_RESPONSE: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED: {
      AppAllocateFlowRequestArrivedMessage * specificMessage =
          dynamic_cast<AppAllocateFlowRequestArrivedMessage *>(message);
      if (send) {
        RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
            specificMessage->getDestAppName());
        specificMessage->setDestPortId(endpoint->getNetlinkPortId());
        specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
      }
      break;
    }
    case RINA_C_APP_DEALLOCATE_FLOW_REQUEST: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_APP_DEALLOCATE_FLOW_RESPONSE: {
      AppDeallocateFlowResponseMessage * specificMessage =
          dynamic_cast<AppDeallocateFlowResponseMessage *>(message);
      if (send) {
        RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
            specificMessage->getApplicationName());
        specificMessage->setDestPortId(endpoint->getNetlinkPortId());
        specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
      }
      break;
    }
    case RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION: {
      AppFlowDeallocatedNotificationMessage * specificMessage =
          dynamic_cast<AppFlowDeallocatedNotificationMessage *>(message);
      if (send) {
        RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
            specificMessage->getApplicationName());
        specificMessage->setDestPortId(endpoint->getNetlinkPortId());
        specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
      }
      break;
    }
    case RINA_C_APP_REGISTER_APPLICATION_REQUEST: {
      AppRegisterApplicationRequestMessage * specificMessage =
          dynamic_cast<AppRegisterApplicationRequestMessage *>(message);
      if (send) {
        specificMessage->setDestPortId(getIPCManagerPortId());
      } else {
        putAPNametoNetlinkPortIdMapping(
            specificMessage->getApplicationRegistrationInformation().appName,
            specificMessage->getSourcePortId(),
            specificMessage->getSourceIpcProcessId());
      }
      break;
    }
    case RINA_C_APP_REGISTER_APPLICATION_RESPONSE: {
      AppRegisterApplicationResponseMessage * specificMessage =
          dynamic_cast<AppRegisterApplicationResponseMessage *>(message);
      if (send) {
        RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
            specificMessage->getApplicationName());
        specificMessage->setDestPortId(endpoint->getNetlinkPortId());
        specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
      }
      break;
    }
    case RINA_C_APP_UNREGISTER_APPLICATION_REQUEST: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      } else {
        AppUnregisterApplicationRequestMessage * specificMessage =
            dynamic_cast<AppUnregisterApplicationRequestMessage *>(message);
        putAPNametoNetlinkPortIdMapping(
            specificMessage->getApplicationName(),
            specificMessage->getSourcePortId(),
            specificMessage->getSourceIpcProcessId());
      }
      break;
    }
    case RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE: {
      if (send) {
        AppUnregisterApplicationResponseMessage * specificMessage =
            dynamic_cast<AppUnregisterApplicationResponseMessage *>(message);
        RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
            specificMessage->getApplicationName());
        specificMessage->setDestPortId(endpoint->getNetlinkPortId());
        specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
      }
      break;
    }
    case RINA_C_APP_GET_DIF_PROPERTIES_REQUEST: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      } else {
        AppGetDIFPropertiesRequestMessage * specificMessage =
            dynamic_cast<AppGetDIFPropertiesRequestMessage *>(message);
        putAPNametoNetlinkPortIdMapping(
            specificMessage->getApplicationName(),
            specificMessage->getSourcePortId(),
            specificMessage->getSourceIpcProcessId());
      }
      break;
    }
    case RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE: {
      if (send) {
        AppGetDIFPropertiesResponseMessage * specificMessage =
            dynamic_cast<AppGetDIFPropertiesResponseMessage *>(message);
        RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
            specificMessage->getApplicationName());
        specificMessage->setDestPortId(endpoint->getNetlinkPortId());
        specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
      }
      break;
    }
      // FIXME use the same code for multiple labels
    case RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_QUERY_RIB_RESPONSE: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_SET_POLICY_SET_PARAM_RESPONSE: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_SELECT_POLICY_SET_RESPONSE: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_PLUGIN_LOAD_RESPONSE: {
      if (send) {
        message->setDestPortId(getIPCManagerPortId());
      }
      break;
    }
    case RINA_C_IPCM_IPC_PROCESS_INITIALIZED: {
      if (send) {
      } else {
        putIPCProcessIdToNelinkPortIdMapping(message->getSourcePortId(),
                                             message->getSourceIpcProcessId());
        IpcmIPCProcessInitializedMessage * specificMessage =
            dynamic_cast<IpcmIPCProcessInitializedMessage *>(message);
        putAPNametoNetlinkPortIdMapping(
            specificMessage->getName(), specificMessage->getSourcePortId(),
            specificMessage->getSourceIpcProcessId());
      }
      break;
    }
    default: {
      //Do nothing
    }
  }
}

/**
 * An OS Process has finalized. Retrieve the information associated to
 * the NL port-id (application name, IPC Process id if it is IPC process),
 * and return it in the form of an OSProcessFinalized event
 * @param nl_portid
 * @return
 */
IPCEvent * NetlinkPortIdMap::osProcessFinalized(unsigned int nl_portid)
{
  //1 Try to get application process name, if not there return 0
  ApplicationProcessNamingInformation apNamingInfo;
  std::map<std::string, RINANetlinkEndpoint*>::iterator iterator;
  std::map<unsigned short, RINANetlinkEndpoint*>::iterator iterator2;
  bool foundAppName = false;
  unsigned short ipcProcessId = 0;

  for (iterator = applicationNameMappings.begin();
      iterator != applicationNameMappings.end(); ++iterator) {
    if (iterator->second->getNetlinkPortId() == nl_portid) {
      apNamingInfo = iterator->second->getApplicationProcessName();
      foundAppName = true;
      delete iterator->second;
      applicationNameMappings.erase(iterator);
      break;
    }
  }

  if (!foundAppName) {
    return 0;
  }

  //2 Try to get IPC Process id
  for (iterator2 = ipcProcessIdMappings.begin();
      iterator2 != ipcProcessIdMappings.end(); ++iterator2) {
    if (iterator2->second->getNetlinkPortId() == nl_portid) {
      ipcProcessId = iterator2->first;
      delete iterator2->second;
      ipcProcessIdMappings.erase(iterator2);
      break;
    }
  }

  OSProcessFinalizedEvent * event = new OSProcessFinalizedEvent(apNamingInfo,
                                                                ipcProcessId,
                                                                0);
  return event;
}

/* CLASS RINA Manager */
/** main function of the Netlink message reader thread */
void * doNetlinkMessageReaderWork(void * arg)
{
  RINAManager * myRINAManager = (RINAManager *) arg;
  NetlinkManager * netlinkManager = myRINAManager->getNetlinkManager();
  BlockingFIFOQueue<IPCEvent> * eventsQueue = myRINAManager->getEventQueue();
  BaseNetlinkMessage * incomingMessage;
  IPCEvent * event;

  //Continuously try to read incoming Netlink messages
  while (true) {
    //Receive message
    try {
      LOG_DBG("Waiting for message %d", netlinkManager->getLocalPort());
      incomingMessage = netlinkManager->getMessage();
    } catch (NetlinkException &e) {
      LOG_ERR("Error receiving netlink message. %s", e.what());
      continue;
    }

    //Process the message
    if (incomingMessage->getOperationCode()
        == RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION) {
      IpcmNLSocketClosedNotificationMessage * message =
          dynamic_cast<IpcmNLSocketClosedNotificationMessage *>(incomingMessage);
      LOG_DBG("NL socket at port %d is closed", message->getPortId());

      event = myRINAManager->osProcessFinalized(message->getPortId());
      if (event) {
        eventsQueue->put(event);
        LOG_DBG(
            "Added event of type %s and sequence number %u to events queue",
            IPCEvent::eventTypeToString(event->eventType).c_str(), event->sequenceNumber);
      }

      delete message;
    } else {
      myRINAManager->netlinkMessageArrived(incomingMessage);
      event = incomingMessage->toIPCEvent();
      if (event) {
        LOG_DBG(
            "Added event of type %s and sequence number %u to events queue",
            IPCEvent::eventTypeToString(event->eventType).c_str(), event->sequenceNumber);
        eventsQueue->put(event);
      } else
        LOG_WARN("Event is null for message type %d",
                 incomingMessage->getOperationCode());

      delete incomingMessage;
    }
  }

  return (void *) 0;
}

RINAManager::RINAManager()
{
  //1 Initialize NetlinkManager
  try {
    netlinkManager = new NetlinkManager(getNelinkPortId());
  } catch (NetlinkException &e) {
    LOG_ERR("Error initializing Netlink Manager. %s", e.what());
    LOG_ERR("Program will exit now");
    exit(-1); 	//FIXME Is this too drastic?
  }
  LOG_DBG("Initialized Netlink Manager");

  initialize();
}

RINAManager::RINAManager(unsigned int netlinkPort)
{
  //1 Initialize NetlinkManager
  try {
    netlinkManager = new NetlinkManager(netlinkPort);
  } catch (NetlinkException &e) {
    LOG_ERR("Error initializing Netlink Manager. %s", e.what());
    LOG_ERR("Program will exit now");
    exit(-1); 	//FIXME Is this too drastic?
  }
  LOG_DBG("Initialized Netlink Manager");

  initialize();
}

void RINAManager::initialize()
{
  //2 Initialie events Queue
  eventQueue = new BlockingFIFOQueue<IPCEvent>();
  LOG_DBG("Initialized event queue");

  //3 Start Netlink message reader thread
  ThreadAttributes threadAttributes;
  threadAttributes.setJoinable();
  netlinkMessageReader = new Thread(&doNetlinkMessageReaderWork,
		  	  	    (void *) this,
		  	  	    &threadAttributes);
  netlinkMessageReader->start();
  LOG_DBG("Started Netlink Message reader thread");
}

RINAManager::~RINAManager()
{
  delete netlinkManager;
  delete netlinkMessageReader;
  delete eventQueue;
}

void RINAManager::sendMessage(BaseNetlinkMessage * netlinkMessage,
		bool fill_seq_num)
{
  sendReceiveLock.lock();

  try {
    netlinkPortIdMap.updateMessageOrPortIdMap(netlinkMessage, true);
    if (fill_seq_num) {
      netlinkMessage->setSequenceNumber(netlinkManager->getSequenceNumber());
    }
    netlinkManager->sendMessage(netlinkMessage);
  } catch (NetlinkException &e) {
    sendReceiveLock.unlock();
    throw e;
  }

  sendReceiveLock.unlock();
}

void RINAManager::sendMessageOfMaxSize(BaseNetlinkMessage * netlinkMessage,
                                       size_t maxSize, bool fill_seq_num)
{
  sendReceiveLock.lock();

  try {
    netlinkPortIdMap.updateMessageOrPortIdMap(netlinkMessage, true);
    if (fill_seq_num) {
      netlinkMessage->setSequenceNumber(netlinkManager->getSequenceNumber());
    }
    netlinkManager->sendMessageOfMaxSize(netlinkMessage, maxSize);
  } catch (NetlinkException &e) {
    sendReceiveLock.unlock();
    throw e;
  }

  sendReceiveLock.unlock();
}

void RINAManager::netlinkMessageArrived(BaseNetlinkMessage * message)
{
  sendReceiveLock.lock();

  //Try to update netlink port id map
  try {
    netlinkPortIdMap.updateMessageOrPortIdMap(message, false);
  } catch (NetlinkException &e) {
    LOG_WARN("Exception while trying to update netlink portId map. %s",
             e.what());
  }

  sendReceiveLock.unlock();
}

IPCEvent * RINAManager::osProcessFinalized(unsigned int nl_portid)
{
  IPCEvent * event;
  sendReceiveLock.lock();
  event = netlinkPortIdMap.osProcessFinalized(nl_portid);
  sendReceiveLock.unlock();
  return event;
}

BlockingFIFOQueue<IPCEvent>* RINAManager::getEventQueue()
{
  return eventQueue;
}

NetlinkManager* RINAManager::getNetlinkManager()
{
  return netlinkManager;
}

Singleton<RINAManager> rinaManager;

/* Get and set default Netlink port id */
unsigned int netlinkPortId = getpid();
Lockable * netlinkLock = new Lockable();

void setNetlinkPortId(unsigned int newNetlinkPortId)
{
  netlinkLock->lock();
  netlinkPortId = newNetlinkPortId;
  netlinkLock->unlock();
}

unsigned int getNelinkPortId()
{
  unsigned int result = 0;
  netlinkLock->lock();
  result = netlinkPortId;
  netlinkLock->unlock();
  return result;
}

}
