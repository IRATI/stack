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

#define IPCP_MODULE "rib-daemon"
#include "ipcp-logging.h"

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
	LOG_IPCP_INFO("Starting Management SDU reader ...");
	while (true) {
		try {
		result = rina::kernelIPCProcess->readManagementSDU(buffer, data->max_sdu_size_);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems reading management SDU: %s", e.what());
			continue;
		}

		sdu = new char[result.getBytesRead()];
		for (int i=0; i<result.getBytesRead(); i++) {
			sdu[i] = buffer[i];
		}

		data->rib_daemon_->cdapMessageDelivered(sdu, result.getBytesRead(), result.getPortId());
	}

	delete buffer;

	return 0;
}

///Class RIBDaemon
IPCPRIBDaemonImpl::IPCPRIBDaemonImpl()
{
	management_sdu_reader_ = 0;
	n_minus_one_flow_manager_ = 0;
	wmpi = rina::WireMessageProviderFactory().createWireMessageProvider();

}

void IPCPRIBDaemonImpl::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
		return;

	app = ap;
	ipcp = dynamic_cast<IPCProcess*>(app);
	if (!ipcp) {
		LOG_IPCP_ERR("Bogus instance of IPCP passed, return");
		return;
	}

        initialize(EncoderConstants::SEPARATOR, ipcp->encoder_,
                        ipcp->cdap_session_manager_, ipcp->enrollment_task_);
        n_minus_one_flow_manager_ = ipcp->resource_allocator_->get_n_minus_one_flow_manager();

        subscribeToEvents();

        rina::ThreadAttributes * threadAttributes = new rina::ThreadAttributes();
        threadAttributes->setJoinable();
        ManagementSDUReaderData * data = new ManagementSDUReaderData(this, max_sdu_size_in_bytes);
        management_sdu_reader_ = new rina::Thread(&doManagementSDUReaderWork,
        					  (void *) data,
        					  threadAttributes);
        management_sdu_reader_->start();
}

void IPCPRIBDaemonImpl::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	LOG_IPCP_DBG("Configuration set: %u", dif_configuration.address_);
}

void IPCPRIBDaemonImpl::subscribeToEvents()
{
	ipcp->internal_event_manager_->subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED, this);
	ipcp->internal_event_manager_->subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED, this);
}

void IPCPRIBDaemonImpl::eventHappened(rina::InternalEvent * event)
{
	if (!event)
		return;

	if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED) {
		rina::NMinusOneFlowDeallocatedEvent * flowEvent =
			(rina::NMinusOneFlowDeallocatedEvent *) event;
		nMinusOneFlowDeallocated(flowEvent->port_id_);
	} else if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED) {
		rina::NMinusOneFlowAllocatedEvent * flowEvent =
			(rina::NMinusOneFlowAllocatedEvent *) event;
		nMinusOneFlowAllocated(flowEvent);
	}
}

void IPCPRIBDaemonImpl::nMinusOneFlowDeallocated(int portId)
{
        ipcp->cdap_session_manager_->removeCDAPSession(portId);
}

void IPCPRIBDaemonImpl::nMinusOneFlowAllocated(rina::NMinusOneFlowAllocatedEvent * event)
{
	if (!event)
		return;
}

void IPCPRIBDaemonImpl::processQueryRIBRequestEvent(const rina::QueryRIBRequestEvent& event)
{
	std::list<rina::BaseRIBObject *> ribObjects = RIBDaemon::getRIBObjects();
	std::list<rina::RIBObjectData> result;

	std::list<rina::BaseRIBObject*>::iterator it;
	for (it = ribObjects.begin(); it != ribObjects.end(); ++it) {
		LOG_IPCP_DBG("Object name: %s", (*it)->name_.c_str());
		result.push_back((*it)->get_data());
	}

	try {
		rina::extendedIPCManager->queryRIBResponse(event, 0, result);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending query RIB response to IPC Manager: %s",
				e.what());
	}
}

void IPCPRIBDaemonImpl::sendMessageSpecific(bool useAddress, const rina::CDAPMessage& cdapMessage, int sessionId,
			unsigned int address, rina::ICDAPResponseMessageHandler * cdapMessageHandler) {
	const rina::SerializedObject * sdu;
	rina::ADataObject adata;
	rina::CDAPMessage * adataCDAPMessage;
	rina::CDAPSessionManagerInterface * cdsm = ipcp->cdap_session_manager_;

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
		throw rina::Exception("Requested a response message but message handler is null");
	}

	atomic_send_lock_.lock();
	sdu = 0;
	try {
		if (useAddress) {
			adata.source_address_ = ipcp->get_address();
			adata.dest_address_ = address;
			adata.encoded_cdap_message_ = cdsm->encodeCDAPMessage(cdapMessage);
			adataCDAPMessage = rina::CDAPMessage::getWriteObjectRequestMessage(0,
					rina::CDAPMessage::NONE_FLAGS, rina::ADataObject::A_DATA_OBJECT_CLASS,
					0, rina::ADataObject::A_DATA_OBJECT_NAME, 0);
			ipcp->encoder_->encode(&adata, adataCDAPMessage);
			sdu = cdsm->encodeCDAPMessage(*adataCDAPMessage);
			rina::kernelIPCProcess->sendMgmgtSDUToAddress(sdu->message_, sdu->size_, address);
			LOG_IPCP_DBG("Sent A-Data CDAP message to address %u: %s", address,
					cdapMessage.to_string().c_str());
			if (cdapMessage.invoke_id_ != 0 && !cdapMessage.is_request_message()) {
				cdsm->get_invoke_id_manager()->freeInvokeId(cdapMessage.invoke_id_, true);
			}

			delete sdu;
			delete adataCDAPMessage;
		} else {
			sdu = cdsm->encodeNextMessageToBeSent(cdapMessage, sessionId);
			rina::kernelIPCProcess->writeMgmgtSDUToPortId(sdu->message_, sdu->size_, sessionId);
			LOG_IPCP_DBG("Sent CDAP message of size %d through port-id %d: %s" , sdu->size_, sessionId,
					cdapMessage.to_string().c_str());

			cdsm->messageSent(cdapMessage, sessionId);
			delete sdu;

			//Check if CDAP session was closed due to the message sent
			const rina::CDAPSessionInterface* cdap_session = cdsm->get_cdap_session(sessionId);
			if (cdap_session && cdap_session->is_closed()) {
				cdsm->removeCDAPSession(sessionId);
			}
		}
	} catch (rina::Exception &e) {
		if (sdu) {
			delete sdu;
		}

		if (cdapMessage.invoke_id_ != 0 && cdapMessage.is_request_message()) {
			cdsm->get_invoke_id_manager()->freeInvokeId(cdapMessage.invoke_id_, false);
		}

		std::string reason = std::string(e.what());
		if (reason.compare("Flow closed") == 0) {
			cdsm->removeCDAPSession(sessionId);
		}

		atomic_send_lock_.unlock();

		throw e;
	}

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
    	handlers_waiting_for_reply_.put(cdapMessage.get_invoke_id(), cdapMessageHandler);
    }

	atomic_send_lock_.unlock();
}

void IPCPRIBDaemonImpl::cdapMessageDelivered(char* message, int length, int portId) {
    const rina::CDAPMessage * cdapMessage;
    const rina::CDAPMessage * aDataCDAPMessage;
    const rina::CDAPSessionInterface * cdapSession;
    rina::CDAPSessionDescriptor * descriptor;
    rina::ADataObject * adata;

    //1 Decode the message and obtain the CDAP session descriptor
    atomic_send_lock_.lock();
    try {
            rina::SerializedObject serializedMessage = rina::SerializedObject(message, length);
            cdapMessage = cdap_session_manager_->messageReceived(serializedMessage, portId);
    } catch (rina::Exception &e) {
            atomic_send_lock_.unlock();
            LOG_IPCP_ERR("Error decoding CDAP message: %s", e.what());
            return;
    }

    //2 If it is an A-Data PDU extract the real message
    if (cdapMessage->obj_name_ == rina::ADataObject::A_DATA_OBJECT_NAME) {
    	try {
    		adata = (rina::ADataObject *) ipcp->encoder_->decode(cdapMessage);
    		if (!adata) {
    			delete cdapMessage;
    			atomic_send_lock_.unlock();
    			return;
    		}

    		aDataCDAPMessage = cdap_session_manager_->decodeCDAPMessage(*adata->encoded_cdap_message_);

    		if (aDataCDAPMessage->invoke_id_ != 0) {
    			if (aDataCDAPMessage->is_request_message()) {
    				cdap_session_manager_->get_invoke_id_manager()->reserveInvokeId(aDataCDAPMessage->invoke_id_,
    												false);
    			} else {
    				cdap_session_manager_->get_invoke_id_manager()->freeInvokeId(aDataCDAPMessage->invoke_id_,
    				    							     false);
    			}
    		}

    		descriptor = new rina::CDAPSessionDescriptor();

    		LOG_IPCP_DBG("Received A-Data CDAP message from address %u : %s", adata->source_address_,
    	    		aDataCDAPMessage->to_string().c_str());

    		atomic_send_lock_.unlock();
    		processIncomingCDAPMessage(aDataCDAPMessage,
    					   descriptor,
    					   rina::CDAPSessionInterface::SESSION_STATE_CON);
    		delete aDataCDAPMessage;
    		delete adata;
    		delete descriptor;
    		delete cdapMessage;
    		return;
    	} catch (rina::Exception &e) {
    		atomic_send_lock_.unlock();
    		LOG_IPCP_ERR("Error processing A-data message: %s", e.what());
    		return;
    	}
    }

    cdapSession = cdap_session_manager_->get_cdap_session(portId);
    if (!cdapSession) {
            atomic_send_lock_.unlock();
            LOG_IPCP_ERR("Could not find open CDAP session related to portId %d", portId);
            delete cdapMessage;
            return;
    }

    descriptor = cdapSession->get_session_descriptor();
    LOG_IPCP_DBG("Received CDAP message through portId %d: %s", portId,
                    cdapMessage->to_string().c_str());
    atomic_send_lock_.unlock();

    processIncomingCDAPMessage(cdapMessage, descriptor,
		    	       cdapSession->get_session_state());

    //Check if CDAP session has been closed due to the message received
    if (cdapSession->is_closed()) {
	    cdap_session_manager_->removeCDAPSession(portId);
    }

    delete cdapMessage;
}

void IPCPRIBDaemonImpl::generateCDAPResponse(int invoke_id,
			rina::CDAPSessionDescriptor * cdapSessDescr,
			rina::CDAPMessage::Opcode opcode,
			const std::string& obj_class,
			const std::string& obj_name,
			rina::RIBObjectValue& robject_value)
{
	IPCMCDAPSessDesc *fwdsess =
		dynamic_cast<IPCMCDAPSessDesc*>(cdapSessDescr);

	if (!fwdsess) {
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = cdapSessDescr->port_id_;

		remote_operation_response_with_value(opcode, obj_class, obj_name,
						     robject_value, 0, std::string(),
						     invoke_id, remote_id,
						     rina::CDAPMessage::NONE_FLAGS);

	} else {
		rina::CDAPMessage *rmsg = NULL;

		switch (opcode) {
		case rina::CDAPMessage::M_READ_R:
			rmsg = rina::CDAPMessage::getReadObjectResponseMessage(
					rina::CDAPMessage::NONE_FLAGS,
					obj_class, 0, obj_name, 0,
					std::string(), invoke_id);
			break;
		case rina::CDAPMessage::M_WRITE_R:
			rmsg = rina::CDAPMessage::getWriteObjectResponseMessage(
					rina::CDAPMessage::NONE_FLAGS,
					0, invoke_id, std::string());
			break;
		case rina::CDAPMessage::M_START_R:
			rmsg = rina::CDAPMessage::getStartObjectResponseMessage(
					rina::CDAPMessage::NONE_FLAGS,
					0, std::string(), invoke_id);
			break;
		case rina::CDAPMessage::M_STOP_R:
			rmsg = rina::CDAPMessage::getStopObjectResponseMessage(
					rina::CDAPMessage::NONE_FLAGS,
					0, std::string(), invoke_id);
			break;
		default:
			LOG_IPCP_WARN("Missing generateCDAPResponse support "
				      "for opcode %d", opcode);
			rmsg = 0;
			break;
		}

		if (!rmsg) {
			return;
		}

		encodeObject(robject_value, rmsg);

		// Reply to the IPC Manager, attaching the response message
		const rina::SerializedObject * so;

		so = wmpi->serializeMessage(*rmsg);
		delete rmsg;

		rina::extendedIPCManager->forwardCDAPResponse(
				fwdsess->req_seqnum, *so, 0);
		delete so;
	}
}

} //namespace rinad
