//
// CDAP connector worker classes
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


#include <unistd.h>
#include <iostream>
#include <exception>
#define RINA_PREFIX     "dms-worker"
#include <librina/logs.h>
#include <encoders/CDAP.pb.h> // For CDAPMessage
#include <encoders/ApplicationProcessNamingInfoMessage.pb.h>
#include <encoders/MA-IPCP.pb.h>
#include <encoders/NeighborMessage.pb.h>

#include "connector.h"
#include "eventsystem.h"  // Various constants used
#include "eventfilters.h"  // Event filtering
#include "json_format.h"  // JSON formatting

using namespace easywsclient;
using namespace std;
using namespace google::protobuf;
using namespace rina::messages;

//
// Constructor
DMSWorker::DMSWorker(rina::ThreadAttributes * threadAttributes,
		const std::string& address,
		unsigned int max_size,
		Server * serv)
: ServerWorker(threadAttributes, serv)
{
	max_sdu_size = max_size;
	ws_address = address;
	ws = NULL;

	ES_EventFilterBuilder builder;
	// Create a shutdown filter
	builder.ofType(ESD_Types::ES_SHUTDOWN)
		   .fromSource(ESD_Sources::SYSTEM)
		   .inLanguage(EDSL_Language::ESEVENT);
	// We ignore versions for now.
	shutdown_filter = builder.build();
	builder.reset();

	// Create a CDAP command / response filter
	builder.ofType(CDAP_Types::ES_CDAP)
		    .fromSource(CDAP_Sources::DMS_MANAGER)
		    .inLanguage(EDSL_Language::CDAP);
	cdap_filter = builder.build();

	LOG_DBG("New DMS Worker created");
}

// Wait                
void DMSWorker::connect_blocking() {
	connectLatch.lock();
	if (ws == nullptr) {
		connectLatch.doWait();
	}
	connectLatch.unlock();
}

// Do the internal work necessary
int DMSWorker::internal_run() {
	int retries = 30;
	unsigned int nap_time = 200; // in millisecs

	// Loop attempting to make a connection
	while (ws == NULL) {
		ws = WebSocket::from_url(ws_address);
		if (ws == NULL) {
			usleep(nap_time * 1000);
			if (retries > 0) {
				LOG_ERR("Error: No connection made to DMS@%s, retrying ...",
					ws_address.c_str());
				retries--;
			} else {
				// bail out
				return 0;
			}
		}
	}

	// Okay now start processing the messages
	LOG_INFO("Connection made to DMS@%s: %d",
			ws_address.c_str(),
			Thread::self()->getThreadType() );
	{
		connectLatch.lock();
		connectLatch.signal();
		connectLatch.unlock();
	}

	// Complete handshake with server
	ws->send("hello dms");
	while (ws->getReadyState() != WebSocket::CLOSED) {
		ws->poll(2000);
		auto myself = this;
		ws->dispatch ( [myself](const std::string& m) { myself->process_message(m); });
	}
	delete ws;
	ws = NULL;
	// Okay now start processing the messages
	LOG_INFO("Disconnected from DMS@%s",
		  ws_address.c_str());

	return 0;
}

// Process the incoming JSON message
void DMSWorker::process_message(const std::string & message)
{
	// Were we asked to shutdown
	if (shutdown_filter->match(message)) {
		// Give a reason
		size_t pstart = message.find("\"reason\":\"");
		if (pstart != string::npos) {
			size_t pend = message.find("\",", pstart+10);
			if (pend != string::npos) {
				size_t len = pend - pstart - 10;
				LOG_DBG("ESD >>> %s", message.substr(pstart+10,len).c_str());
			} else {
				LOG_DBG("ESD >>> Disconnect received");
			}
		} else {
			LOG_DBG("ESD >>> Disconnect received");
		}
		ws->close();
		// Is it CDAP
	} else if (cdap_filter->match(message)) {
		LOG_DBG("CDAP >>> %s", message.c_str());
		// Attempt some parsing
		rina::messages::CDAPMessage cdap_message;
		if (JsonFormat::ParseFromString(message, &cdap_message)) {
			// We got a valid CDAP message
			log_short_message(cdap_message, CDAP_Sources::DMS_AGENT);
			// Convert object value
			process_value(cdap_message);

			// Encode as a byte array
			int size = cdap_message.ByteSize();
			unsigned char* buf = new unsigned char[size];
			if (cdap_message.SerializeToArray(buf, size)) {
				// Extract destination
				rina::ApplicationProcessNamingInformation dest;
				if (cdap_message.has_destapname()) {
					if (cdap_message.has_destapinst()) {
						dest = rina::ApplicationProcessNamingInformation(
								cdap_message.destapname(), cdap_message.destapinst());
					}  else {
						dest = rina::ApplicationProcessNamingInformation(
								cdap_message.destapname(), std::string()
						);
					}
					LOG_DBG("Message is %d for %s",
						cdap_message.IsInitialized(),
						dest.getProcessNamePlusInstance().c_str());
					// Now dispatch to the MA
					send_message(buf, size, dest.getProcessNamePlusInstance());
				} else {
					LOG_WARN("missing destination on message! ");
				}
			}
		} else {
			LOG_WARN("failed to encode message!");
		}
	} else {
		// echo the message
		//std::cout << ">>> Not for us" << std::endl;
	}

}


//
// Process the object value if present
//
void DMSWorker::process_value(CDAPMessage & cdap_message) {
	// Figure out if we need to do anything
	if (cdap_message.has_objvalue()) {
		objVal_t value = cdap_message.objvalue();
		if (value.has_jsonval() && value.has_typeval()) {
			// We might have something to do
			string type = value.typeval();
			LOG_DBG("Type is:[%s]", type.c_str());
			//const DescriptorPool* dp = DescriptorPool::generated_pool();
			const Descriptor* d  = ipcp_config_t::descriptor(); // dp->FindMessageTypeByName(type);
			if (d != nullptr) {
				MessageFactory* mf = MessageFactory::generated_factory();
				const Message* prototype = mf->GetPrototype(d);
				if (prototype != nullptr) {
					Message* m = prototype->New();
					if (m != nullptr) {
						// Attempt type conversion
						JsonFormat::ParseFromString(value.jsonval(), m);
						int size = m->ByteSize();
						auto buf = new unsigned char[size];
						if(m->SerializeToArray(buf, size)) {
							// Update the value
							objVal_t* newvalue = new objVal_t();
							newvalue->set_byteval(buf, size);
							newvalue->set_typeval(type); // Pass along the type
							cdap_message.set_allocated_objvalue(newvalue);
						}
					}
				}
			} else {
				LOG_WARN("No descriptor for %s", type.c_str());
			}
		} else {
			LOG_WARN("Missing type info [json=%s ,type=]",
				  value.has_jsonval(),
				  value.has_typeval());
		}
	}
}

/*
 * Handover interface
 */

// Send outgoing message to DMS
void DMSWorker::send_message(const std::string & message) {
	if (ws == nullptr) {
		LOG_WARN("No DMS found: message has been discarded.");
		return;
	}

	try {
		// Write the next JSON message
		ws->send(message);
	} catch (exception &e) {
		LOG_WARN("Cant send message to DMS: message has been discarded. Exception : %s",
				e.what());
	}
}

// Send outgoing message to MA
void DMSWorker::send_message(const void* message, int size, 
		const std::string& destination) {
	Connector* c = reinterpret_cast<Connector*>(server);
	assert(c != nullptr);
	MAWorker* ma = c->find_ma_worker();
	if (ma != nullptr) {
		try {
			// Found MA instance so just send it
			ma->send_message(message, size, destination);
		} catch (std::exception& e) {
			LOG_ERR("ERROR: Exception: %s", e.what());
		} catch (...) {
			LOG_ERR("ERROR: Exception : Unknown");
		}
	} else {
		LOG_ERR("INFO: No MA!");
	}
}

//
// Log a short, but informative message
//
void DMSWorker::log_short_message(const CDAPMessage& message, const char * direction) const {
	string b;

	b.append("CDAP ");
	opCode_t code = message.opcode();
	b.append(opCode_t_Name(code));
	switch(code) {
	// operations on an object
	case M_CREATE:
	case M_DELETE:
	case M_READ:
	case M_WRITE:
	case M_CANCELREAD:
	case M_START:
	case M_STOP:
		b.append(" of ");
		b.append(message.objclass());
		// missing break is intentional
	case M_CONNECT:
	case M_RELEASE:
		b.append(". ");
		break;
		// Responses on an object
	case M_CREATE_R:
	case M_DELETE_R:
	case M_READ_R:
	case M_CANCELREAD_R:
	case M_WRITE_R:
	case M_START_R:
	case M_STOP_R:
		b.append(" of ");
		b.append(message.objclass());
		// missing break is intentional
	case M_CONNECT_R:
	case M_RELEASE_R:
		if (message.result() != 0) {
			b.append(" FAILED ");
			if (message.has_resultreason()) {
				b.append("(");
				b.append(message.resultreason());
				b.append(") ");
			}
		} else {
			b.append(" SUCCESSFUL ");
		}
		break;
	default:
		break;
	}
	b.append("Sending to ");
	b.append(direction);
	b.append("[");
	b.append(message.destapname());
	if (message.has_destapinst()) {
		b.append("-");
		b.append(message.destapinst());
	}
	b.append("].");
	LOG_INFO("%s",b.c_str());
}
