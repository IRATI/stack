//
// CDAP Connector
//
// Micheal Crotty <mcrotty@tssg.org>
// Copyright (C) 2016, PRISTINE, Waterford Insitute of Technology
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
#include <iostream>

#include <cassert>
#define RINA_PREFIX     "cdap-connect"
#include <librina/logs.h>
//#include <librina/cdap_v2.h>
#include <librina/common.h>
#include <rinad/common/configuration.h>
//#include <rinad/common/encoder.h>

#include "connector.h"
#include "workers.h"



Connector::Connector(const std::list<std::string>& dif_names,
		const std::string& apn,
		const std::string& api,
		const std::string& ws)
: Server(dif_names, apn, api)
{
	client_app_reg_ = false;
	ws_address_ = ws;
	dms_worker_ = nullptr;
	ma_worker_ = nullptr;
}

void Connector::run()
{
	// Attempt to start the DMS worker
	ws_run();

	// Attempt to start the RINA server
	rina_run();

	while (true) {
		LOG_INFO("Doing RINA stuff");
		sleep(100);
	}
}

void Connector::ws_run() {
	// have we something for the DMS worker to do
	if (!(ws_address_.empty())) {
		ServerWorker * worker = internal_start_worker(ws_address_);
		if (worker != NULL) {
			// register it with the base class
			worker_started(worker);
			LOG_INFO("New DMS worker allocated");

			// Prod the DMS thread so a connection is made.
			DMSWorker* dms = find_dms_worker();
			dms->connect_blocking();
			dms->send_message(std::string("Establishing connnection."));
		}
	} else {
		LOG_INFO("No DMS worker started.");
	}
}

void Connector::rina_run() {

	if (app_name.empty()) {
		// No App name means skip this
		LOG_INFO("No MA connection started.");
		return;
	}


	rina::initialize("INFO", "");

	rina::FlowInformation flow;
	// Variables used for casting
	rina::RegisterApplicationResponseEvent*   r = nullptr;
	rina::UnregisterApplicationResponseEvent* ur = nullptr;
	rina::DeallocateFlowResponseEvent*        dfr = nullptr;

	ServerWorker * worker = internal_start_worker(flow);
	if (worker != NULL) {
		// register it with the base class
		worker_started(worker);
		LOG_INFO("New MA worker allocated");
	} else {
		LOG_INFO("No MA worker started.");
	}

	applicationRegister();

	for (;;)
	{
		rina::IPCEvent* event = rina::ipcEventProducer->eventTimedWait(1,0);
		int port_id = 0;

		if (event == nullptr) {
			LOG_INFO("Processing events");
			continue;
		}

		switch (event->eventType) {

		case rina::REGISTER_APPLICATION_RESPONSE_EVENT:
			r = dynamic_cast<rina::RegisterApplicationResponseEvent*>(event);
			if (r->result == 0) {
				rina::ipcManager->commitPendingRegistration(
						event->sequenceNumber,
						r->DIFName);
				LOG_INFO("Application has been registered at DIF[%s]",
						r->DIFName.toString().c_str());
				// registered = true;
			} else {
				rina::ipcManager->withdrawPendingRegistration(
						event->sequenceNumber);
				LOG_WARN("Failed to register application [%s] at DIF[%s].",
						r->applicationName.toString().c_str(), r->DIFName.toString().c_str());
				// registered = false
			}
			break;

		case rina::UNREGISTER_APPLICATION_RESPONSE_EVENT:
			rina::ipcManager->appUnregistrationResult(
					event->sequenceNumber,
					dynamic_cast<rina::UnregisterApplicationResponseEvent*>(event)
					->result == 0);
			ur = dynamic_cast<rina::UnregisterApplicationResponseEvent*>(event);
			if (ur->result == 0) {
				LOG_INFO("Application has been unregistered at DIF[%s]",
						ur->DIFName.toString().c_str());
			} else {
				LOG_WARN("Failed to unregister application [%s] at DIF [%s].",
						ur->applicationName.toString().c_str(), ur->DIFName.toString().c_str());
			}
			break;

		case rina::FLOW_ALLOCATION_REQUESTED_EVENT:
			flow = rina::ipcManager->allocateFlowResponse(
					*dynamic_cast<rina::FlowRequestEvent*>(event), 0, true);
			port_id = flow.portId;
			// We want non-blocking operation
			rina::ipcManager->setFlowOptsBlocking(port_id, false);
			LOG_INFO("New flow allocated [port-id = %d]", port_id);
			// Notify worker
			if (ma_worker_ != nullptr) {
				ma_worker_->newFlowRequest(flow);
			} else {
				LOG_INFO("No MA worker available");
			}
			break;

		case rina::FLOW_DEALLOCATED_EVENT:
			port_id = dynamic_cast<rina::FlowDeallocatedEvent*>(event)
			->portId;
			flow = rina::ipcManager->getFlowInformation(port_id);
			rina::ipcManager->flowDeallocated(port_id);
			LOG_INFO("Flow torn down remotely [port-id = %d]", port_id);
			// Notify worker
			if (ma_worker_ != nullptr) {
				ma_worker_->closeFlowRequest(flow);
			} else {
				LOG_INFO("No MA");
			}
			break;

		case rina::DEALLOCATE_FLOW_RESPONSE_EVENT:
			LOG_INFO("Destroying the flow after time-out");
			dfr = dynamic_cast<rina::DeallocateFlowResponseEvent*>(event);
			port_id = dfr->portId;
			flow = rina::ipcManager->getFlowInformation(port_id);

			rina::ipcManager->flowDeallocationResult(port_id,
					dfr->result == 0);
			if (ma_worker_ != nullptr) {
				ma_worker_->closeFlowRequest(flow);
			} else {
				LOG_INFO("No MA");
			}
			break;
		default:
			LOG_INFO("Server got event of type %d", event->eventType);
			break;
		}
	}
}

void Connector::applicationRegister() {
	rina::ApplicationRegistrationInformation ari;
	//rina::RegisterApplicationResponseEvent *resp;
	//unsigned int seqnum;
	// IPCEvent *event;
	// int successful_regs = 0;

	ari.ipcProcessId = 0;  // This is an application, not an IPC process
	ari.appName = rina::ApplicationProcessNamingInformation(app_name,
			app_instance);

	for (std::list<std::string>::iterator it = dif_names.begin();
			it != dif_names.end(); ++ it)
	{
		if (it->compare("") == 0) {
			ari.applicationRegistrationType = rina::APPLICATION_REGISTRATION_ANY_DIF;
		} else {
			ari.applicationRegistrationType = rina::APPLICATION_REGISTRATION_SINGLE_DIF;
			ari.difName = rina::ApplicationProcessNamingInformation(*it, std::string());
		}

		LOG_INFO("Registering [%s:%s] at [%s]", app_name.c_str(), app_instance.c_str(), it->c_str());
		// Request the registration
		//rina::ipcManager->reque
		//seqnum =
		try {
			rina::ipcManager->requestApplicationRegistration(ari);
		} catch (rina::IPCException ie) {
			LOG_INFO("Failed registering [%s:%s] at [%s]", app_name.c_str(),
					app_instance.c_str(), it->c_str());
			LOG_INFO("Exception:%s", ie.what());
		}
	}
}


// Create the appropriate MA worker
ServerWorker * Connector::internal_start_worker(rina::FlowInformation flow)
{
	rina::ThreadAttributes threadAttributes;
	MAWorker * worker = new MAWorker(&threadAttributes, flow,
			max_sdu_size_in_bytes, this);
	worker->start();
	worker->detach();

	pthread_t tid = (reinterpret_cast<rina::Thread*>(worker))->getThreadType();
	pthread_setname_np(tid, "maworker");

	ma_worker_ = worker;
	return worker;
}

// Create the appropriate DMS worker
ServerWorker * Connector::internal_start_worker(const std::string& ws_address)
{
	rina::ThreadAttributes threadAttributes;
	DMSWorker * worker = new DMSWorker(&threadAttributes, ws_address,
			max_sdu_size_in_bytes, this);
	worker->start();
	worker->detach();
	pthread_t tid = (reinterpret_cast<rina::Thread*>(worker))->getThreadType();
	pthread_setname_np(tid, "dmsworker");
	dms_worker_ = worker;
	return worker;
}
