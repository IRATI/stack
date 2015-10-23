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

#include <librina/cdap_v2.h>
#include <librina/common.h>
#include <librina/rib_v2.h>
#include "rib-daemon.h"

namespace rinad {

//Class ManagementSDUReader data
ManagementSDUReaderData::ManagementSDUReaderData(unsigned int max_sdu_size)
{
	max_sdu_size_ = max_sdu_size;
}

void * doManagementSDUReaderWork(void* arg)
{
	ManagementSDUReaderData * data = (ManagementSDUReaderData *) arg;
	rina::ser_obj_t message;
	char* buffer = new char[data->max_sdu_size_];

	rina::ReadManagementSDUResult result;
	LOG_IPCP_INFO("Starting Management SDU reader ...");
	while (true) {
		try {
			result = rina::kernelIPCProcess->readManagementSDU(buffer,
									   data->max_sdu_size_);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems reading management SDU: %s", e.what());
			continue;
		}

		message.message_ = buffer;
		message.size_ = result.bytesRead;

		//Instruct CDAP provider to process the CACEP message
		try{
			rina::cdap::getProvider()->process_message(message,
								   result.portId);
		}catch(rina::WriteSDUException &e){
			LOG_ERR("Cannot write to flow with port id: %u anymore",
				result.portId);
		}
	}

	delete buffer;

	return 0;
}

class IPCPCDAPIOHandler : public rina::cdap::CDAPIOHandler
{
 public:
	IPCPCDAPIOHandler(){};
	void send(const rina::cdap::cdap_m_t *m_sent,
		  unsigned int handle,
		  rina::cdap_rib::cdap_dest_t cdap_dest);
	void process_message(rina::ser_obj_t &message,
			     unsigned int handle,
			     rina::cdap_rib::cdap_dest_t cdap_dest);

 private:
        // Lock to control that when sending a message requiring
	// a reply the CDAP Session manager has been updated before
	// receiving the response message
        rina::Lockable atomic_send_lock_;
};

void IPCPCDAPIOHandler::send(const rina::cdap::cdap_m_t *m_sent,
			     unsigned int handle,
			     rina::cdap_rib::cdap_dest_t cdap_dest)
{
	//TODO
}

void IPCPCDAPIOHandler::process_message(rina::ser_obj_t &message,
		     	     	        unsigned int handle,
		     	     	        rina::cdap_rib::cdap_dest_t cdap_dest)
{
	const rina::cdap::cdap_m_t *m_rcv;
	rina::cdap_rib::con_handle_t con_handle;
	bool is_auth_message = false;

	if (cdap_dest == rina::cdap_rib::CDAP_DEST_IPCM) {
		try {
			m_rcv = manager_->decodeCDAPMessage(message);
			con_handle.cdap_dest = rina::cdap_rib::CDAP_DEST_IPCM;

			LOG_IPCP_DBG("Received delegated CDAP message from IPCM : %s",
					m_rcv->to_string().c_str());
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Error decoding CDAP message : %s", e.what());
			return;
		}
	} else {

		//1 Decode the message and obtain the CDAP session descriptor
		atomic_send_lock_.lock();
		try {
			m_rcv = manager_->messageReceived(message, handle);
		} catch (rina::Exception &e) {
			atomic_send_lock_.unlock();
			LOG_IPCP_ERR("Error decoding CDAP message: %s", e.what());
			return;
		}
		atomic_send_lock_.unlock();

		//2 If it is an A-Data PDU extract the real message
		if (m_rcv->obj_name_ == rina::ADataObject::A_DATA_OBJECT_NAME) {
			try {
				ADataObjectEncoder encoder;
				rina::ADataObject a_data_obj;
				rina::cdap::cdap_m_t * old_msg = m_rcv;

				encoder.decode(m_rcv->obj_value_, a_data_obj);

				m_rcv = manager_->decodeCDAPMessage(*(a_data_obj.encoded_cdap_message_));
				if (m_rcv->invoke_id_ != 0) {
					if (m_rcv->is_request_message()){
						manager_->get_invoke_id_manager()->reserveInvokeId(m_rcv->invoke_id_,
								false);
					} else {
						manager_->get_invoke_id_manager()->freeInvokeId(m_rcv->invoke_id_,
								false);
					}
				}

				LOG_IPCP_DBG("Received A-Data CDAP message from address %u : %s",
						a_data_obj.source_address_,
						m_rcv->to_string().c_str());

				con_handle.cdap_dest = rina::cdap_rib::CDAP_DEST_ADDRESS;
				delete old_msg;
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Error processing A-data message: %s", e.what());
				return;
			}
		} else {
			rina::cdap::CDAPSession * cdap_session;

			LOG_IPCP_DBG("Received CDAP message from N-1 port %u : %s",
					handle, m_rcv->to_string().c_str());
			if (manager_->session_in_await_con_state(handle) &&
					m_rcv->op_code_ != rina::cdap::cdap_m_t::M_CONNECT)
				is_auth_message = true;
		}
	}

	// Fill structures
	// Auth
	con_handle.auth_.name = m_rcv->auth_policy_.name_;
	con_handle.auth_.versions = m_rcv->auth_policy_.versions_;
	if (con_handle.auth_.options.size_ > 0) {
		con_handle.auth_.options.size_ = m_rcv->auth_policy_.options_.size_;
		con_handle.auth_.options.message_ = new char[con_handle.auth_.options.size_];
		memcpy(con_handle.auth_.options.message_,
		       m_rcv->auth_policy_.options_.message_,
		       m_rcv->auth_policy_.options_.size_);
	}
	// Src
	con_handle.src_.ae_name_ = m_rcv->src_ae_name_;
	con_handle.src_.ae_inst_ = m_rcv->src_ae_inst_;
	con_handle.src_.ap_name_ = m_rcv->src_ap_name_;
	con_handle.src_.ae_inst_ = m_rcv->src_ae_inst_;
	// Dest
	con_handle.dest_.ae_name_ = m_rcv->dest_ae_name_;
	con_handle.dest_.ae_inst_ = m_rcv->dest_ae_inst_;
	con_handle.dest_.ap_name_ = m_rcv->dest_ap_name_;
	con_handle.dest_.ae_inst_ = m_rcv->dest_ae_inst_;
	// Handle
	con_handle.handle_ = handle;
	// Version
	con_handle.version_.version_ = m_rcv->version_;
	// Flags
	rina::cdap_rib::flags_t flags;
	flags.flags_ = m_rcv->flags_;
	// Object
	rina::cdap_rib::obj_info_t obj;
	obj.class_ = m_rcv->obj_class_;
	obj.inst_ = m_rcv->obj_inst_;
	obj.name_ = m_rcv->obj_name_;
	obj.value_.size_ = m_rcv->obj_value_.size_;
	obj.value_.message_ = new char[obj.value_.size_];
	memcpy(obj.value_.message_, m_rcv->obj_value_.message_,
			m_rcv->obj_value_.size_);
	// Filter
	rina::cdap_rib::filt_info_t filt;
	filt.filter_ = m_rcv->filter_;
	filt.scope_ = m_rcv->scope_;
	// Invoke id
	int invoke_id = m_rcv->invoke_id_;
	// Result
	rina::cdap_rib::res_info_t res;
	//FIXME: do not typecast when the codes are an enum in the GPB
	res.code_ = static_cast<rina::cdap_rib::res_code_t>(m_rcv->result_);
	res.reason_ = m_rcv->result_reason_;

	// If authentication-related message, process here
	if (is_auth_message) {
		callback_->process_authentication_message(*m_rcv, con_handle);
		delete m_rcv;
		return;
	}

	// Process all other messages
	switch (m_rcv->op_code_) {
		//Local
		case rina::cdap::cdap_m_t::M_CONNECT:
			callback_->open_connection(con_handle, flags, invoke_id);
			break;
		case rina::cdap::cdap_m_t::M_RELEASE:
			callback_->close_connection(con_handle, flags, invoke_id);
			break;
		case rina::cdap::cdap_m_t::M_DELETE:
			callback_->delete_request(con_handle, obj, filt, invoke_id);
			break;
		case rina::cdap::cdap_m_t::M_CREATE:
			callback_->create_request(con_handle, obj, filt, invoke_id);
			break;
		case rina::cdap::cdap_m_t::M_READ:
			callback_->read_request(con_handle, obj, filt, invoke_id);
			break;
		case rina::cdap::cdap_m_t::M_CANCELREAD:
			callback_->cancel_read_request(con_handle, obj, filt, invoke_id);
			break;
		case rina::cdap::cdap_m_t::M_WRITE:
			callback_->write_request(con_handle, obj, filt, invoke_id);
			break;
		case rina::cdap::cdap_m_t::M_START:
			callback_->start_request(con_handle, obj, filt, invoke_id);
			break;
		case rina::cdap::cdap_m_t::M_STOP:
			callback_->stop_request(con_handle, obj, filt, invoke_id);
			break;

		//Remote
		case rina::cdap::cdap_m_t::M_CONNECT_R:
			callback_->remote_open_connection_result(con_handle, res);
			break;
		case rina::cdap::cdap_m_t::M_RELEASE_R:
			callback_->remote_close_connection_result(con_handle, res);
			break;
		case rina::cdap::cdap_m_t::M_CREATE_R:
			callback_->remote_create_result(con_handle, obj, res);
			break;
		case rina::cdap::cdap_m_t::M_DELETE_R:
			callback_->remote_delete_result(con_handle, res);
			break;
		case rina::cdap::cdap_m_t::M_READ_R:
			callback_->remote_read_result(con_handle, obj, res);
			break;
		case rina::cdap::cdap_m_t::M_CANCELREAD_R:
			callback_->remote_cancel_read_result(con_handle, res);
			break;
		case rina::cdap::cdap_m_t::M_WRITE_R:
			callback_->remote_write_result(con_handle, obj, res);
			break;
		case rina::cdap::cdap_m_t::M_START_R:
			callback_->remote_start_result(con_handle, obj, res);
			break;
		case rina::cdap::cdap_m_t::M_STOP_R:
			callback_->remote_stop_result(con_handle, obj, res);
			break;
		default:
			LOG_ERR("Operation not recognized");
			break;
	}

	delete m_rcv;
}

///Class RIBDaemon
IPCPRIBDaemonImpl::IPCPRIBDaemonImpl(rina::cacep::AppConHandlerInterface *app_con_callback)
{
	management_sdu_reader_ = 0;
	n_minus_one_flow_manager_ = 0;
	initialize_rib_daemon(app_con_callback);
}

void IPCPRIBDaemonImpl::initialize_rib_daemon(rina::cacep::AppConHandlerInterface *app_con_callback)
{
	rina::cdap_rib::cdap_params params;
	rina::cdap_rib::vers_info_t vers;
	rina::rib::RIBObj* robj;

	//Initialize the RIB library and cdap
	params.is_IPCP_ = true;
	rina::rib::init(app_con_callback, 0, params);
	rina::cdap::set_cdap_io_handler(new IPCPCDAPIOHandler());
	getProxy();

	//Create schema
	vers.version_ = 0x1ULL;
	ribd->createSchema(vers);
	//TODO create callbacks

	//Create RIB
	rib = ribd->createRIB(vers);

	//TODO populate RIB with some objects
	try {
		robj = new rina::rib::RIBObj("DIFManagement");
		ribd->addObjRIB(rib, "/difmanagement", &robj);

		robj = new rina::rib::RIBObj("DataTransfer");
		ribd->addObjRIB(rib, "/dt", &robj);

		robj = new rina::rib::RIBObj("FlowAllocator");
		ribd->addObjRIB(rib, "/fa", &robj);

		robj = new rina::rib::RIBObj("IPCManagement");
		ribd->addObjRIB(rib, "/ipcmanagement", &robj);

		robj = new rina::rib::RIBObj("ResourceAllocator");
		ribd->addObjRIB(rib, "/resalloc", &robj);

		robj = new rina::rib::RIBObj("RIBDaemon");
		ribd->addObjRIB(rib, "/ribdaemon", &robj);

		robj = new rina::rib::RIBObj("RMT");
		ribd->addObjRIB(rib, "/rmt", &robj);

		robj = new rina::rib::RIBObj("SDUDelimiting");
		ribd->addObjRIB(rib, "/sdudel", &robj);
	} catch (rina::Exception &e1) {
		LOG_ERR("RIB basic objects were not created because %s",
				e1.what());
	}
}

void IPCPRIBDaemonImpl::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
		return;

	ipcp = dynamic_cast<IPCProcess*>(ap);
	if (!ipcp) {
		LOG_IPCP_ERR("Bogus instance of IPCP passed, return");
		return;
	}


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
	std::list<rina::rib::RIBObj *> ribObjects = ribd->get_rib_objects(rib);
	std::list<rina::rib::RIBObjectData> result;

	std::list<rina::rib::RIBObj*>::iterator it;
	for (it = ribObjects.begin(); it != ribObjects.end(); ++it) {
		LOG_IPCP_DBG("Object name: %s", (*it)->fqn.c_str());
		result.push_back((*it)->get_object_data());
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
