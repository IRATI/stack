//
// CDAP connector worker classes
//
// Micheal Crotty <mcrotty AT tssg DOT org>
// Copyright (c) 2016, PRISTINE project, Waterford Institute of Technology. 
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
#include <encoders/CDAP.pb.h> // For CDAPMessage
#define RINA_PREFIX     "cdap-connect"
#include <librina/logs.h>

#include "connector.h"
#include "eventsystem.h"  // Various constants used
#include "json_format.h"  // JSON formatting

using namespace std;
using namespace rina;
using namespace google::protobuf;
using namespace rina::messages;

// Allow construction given flow informaiton
ConnectionInfo::ConnectionInfo(rina::FlowInformation& flow_info) {
  flow = flow_info;
  // Generate an id
  ApplicationProcessNamingInformation remote = flow.remoteAppName;
  id = string(remote.getProcessNamePlusInstance());
}



MAWorker::MAWorker(rina::ThreadAttributes * threadAttributes,
                             rina::FlowInformation flow, unsigned int max_size,
                             Server * serv)
        : ServerWorker(threadAttributes, serv)
{
    flow_ = flow;
    max_sdu_size = max_size;
    //cdap_prov_ = 0;
    //connections
    //connections = 
}

int MAWorker::internal_run()
{
  std::list<ConnectionInfo*> list = connections.getEntries();
  unsigned char buffer[max_sdu_size_in_bytes];
  while(true) {
    // Run through the open connections
    if (update) {  // performance improvement
      list = connections.getEntries();
      update=false;      
    }
    for (auto p=list.begin();p != list.end();p++) {
      try {
        // Read the next SDU
        int bytes_read = rina::ipcManager->readSDU((*p)->get_port(), buffer,
                                                   max_sdu_size_in_bytes);
        // rina::ser_obj_t message;
        // message.message_ = buffer;
        // message.size_ = bytes_read;
        if (bytes_read > 0) {
          std::cout << "Read " << bytes_read << " bytes into the sdu buffer" << std::endl;
          (*p)->ok();
          process_message(buffer, bytes_read);            
        }
      } catch (rina::ReadSDUException &e)
      {
        if ((*p)->failed()) {
          // notify manager of failed connection
          LOG_WARN("Three failed attempts made to read an SDU, closing flow");
          notify_connection_gone(*p);
        }
        LOG_WARN("ReadSDUException in readSDU: %s", e.what());
      } catch (rina::Exception &e)
      {
        // These exceptions are assumed fatal
        LOG_WARN("Exception in readSDU: %s, closing flow", e.what());
        // notify manager of failed connection
        notify_connection_gone(*p);
      }
    }
  }
  return 0;
}

// process the incoming message
void MAWorker::process_message(const void* message, int size) {
  
  rina::messages::CDAPMessage m;
  if (m.ParseFromArray(message, size)) {
    // We have parse correctly
    
    if (m.opcode() == rina::messages::opCode_t::M_CONNECT) {
      //cout << "Received a M_CONNECT" << endl;
      open_connection(m);
    }
    
    // TODO: Fixup the value
    //process_value(m);
    
    // Encode as JSON
    string encoded;
    if (JsonFormat::PrintToString(m, get_dialect(m), &encoded)) {
      // Pass on to manager
      send_message(encoded);      
    } else {
      LOG_WARN("Encoding to JSON message failed. Message discarded");      
    }
  } else {
    LOG_WARN("Invalid CDAP message received. Message discarded.");
  }
}

// process the incoming value
void MAWorker::process_value(CDAPMessage & cdap_message) {
  // Figure out if we need to do anything
  if (cdap_message.has_objvalue()) {
    objVal_t value = cdap_message.objvalue();
    if (value.has_byteval() && value.has_typeval()) {
      // We might have something to do
      string type = value.typeval();
      const DescriptorPool* dp = DescriptorPool::generated_pool();
      const Descriptor* d  = dp->FindMessageTypeByName(type);
      if (d != nullptr) {
        MessageFactory* mf = MessageFactory::generated_factory();
        const Message* prototype = mf->GetPrototype(d);
        if (prototype != nullptr) {
          Message* m = prototype->New();
          if (m != nullptr) {
            // Attempt type conversion
            if (m->ParseFromString(value.byteval())) {
              string noheader;
              string encoded;
              if (JsonFormat::PrintToString((*m), noheader, &encoded)) {
                // Update the value
                objVal_t* newvalue = new objVal_t();
                newvalue->set_jsonval(encoded);
                newvalue->set_typeval(type); // Pass along the type
                cdap_message.set_allocated_objvalue(newvalue);
              }              
            }
          }
        }
      }
    }
  }
}

//
// New flow has been accepted.
// 
void MAWorker::newFlowRequest(rina::FlowInformation& flow) {
  ConnectionInfo* ci = new ConnectionInfo(flow);  
  // add to connnections
  ci->get_id();
  connections.put(ci->get_id(),ci);
  update = true;
}

// A flow has been deallocated. 
void MAWorker::closeFlowRequest(rina::FlowInformation& flow) {
  // At this point we are unsure as to whether the CDAP Release has been sent.
  ConnectionInfo ci(flow);  
  // Renotify just in case
  notify_connection_gone(&ci);
  update = true;
}

// Notify the manager the connection is no longer present
void MAWorker::notify_connection_gone(ConnectionInfo* connection) {
  static long notify_ids = 8000;

  // When a connection is terminated notify manager
  rina::messages::CDAPMessage m;
  m.set_opcode(rina::messages::opCode_t::M_RELEASE);
  m.set_invokeid(notify_ids++);
  // CONNECTs and RELEASEs should have the CDAP protocol version
  m.set_abssyntax(0); // TODO: Fix hardcoding.
  m.set_version(0);   // TODO: Fix hardcoding
  // Source the message as coming from the MA
  ApplicationProcessNamingInformation remote = connection->flow.remoteAppName;
  m.set_srcapname(remote.processName);
  m.set_srcapinst(remote.processInstance);
  if (!remote.entityName.empty()) {
    m.set_srcaename(remote.entityName);
    m.set_srcaeinst(remote.entityInstance);
  }
  // Going to the manager
  ApplicationProcessNamingInformation local = connection->flow.localAppName;
  m.set_destapname(local.processName);
  m.set_destapinst(local.processInstance);
  if (!local.entityName.empty()) {
    m.set_destaename(local.entityName);
    m.set_destaeinst(local.entityInstance);    
  }

  // Encode as JSON
  string encoded;
  if (JsonFormat::PrintToString(m, get_dialect(m), &encoded)) {
    // Pass on to manager
    send_message(encoded);      
  } else {
    LOG_WARN("Encoding to JSON message failed. Manager not notified.");      
  }
  
  // Do local cleanup
  connection_gone(connection);
}

// clean up failed connection
void MAWorker::connection_gone(ConnectionInfo* connection) {
  //std::unique_ptr<ConnectionInfo> connection(con);
  try {
    // Log the action
    LOG_INFO("Connection unexpectedly closed. [port-id=%d].", connection->get_port());
    // Notify IPC manager
    rina::ipcManager->flowDeallocated(connection->get_port());
  } catch (rina::FlowDeallocationException fde) {
    // Flow no longer exists, so no further IPCManager cleanup needed.    
    //cleanAndCloseFlow(p->get_port());  
  } catch (rina::IPCException ie) {
    // Inconsistency in the RINA API
    LOG_INFO("Inconsistency in the RINA API: Caught a IPCException:%s.", ie.what());
    // Flow no longer exists, so no further IPCManager cleanup needed.    
  }
  // Remove from active connections
  ConnectionInfo* p =connections.erase(connection->get_id());
  delete p;
}


// Get the dialect from the message
const string& MAWorker::get_dialect(const rina::messages::CDAPMessage& m) const {
  static const string REQ(CDAP_Dialects::CDAP_REQ);
  static const string RES(CDAP_Dialects::CDAP_RES);
  
  return ((m.opcode() & 0x0001) == 0) ? REQ : RES;
}



/*
 * Handover interface
 */
 
// Process outgoing message to DMS
void MAWorker::send_message(const string& message) {
  if (server != nullptr) {
    Connector* c = reinterpret_cast<Connector*>(server);
    Handover* dms = reinterpret_cast<Handover*>(c->find_dms_worker());
    if (dms != nullptr) {
      // Found DMS instance so just send it
      dms->send_message(message);
    } else {
      cout << "No DMS yet!" << endl;
    }    
  }
}

// Process outgoing message to MA
void MAWorker::send_message(const void* message, int size, const std::string & destination) {
  //assert(false); // wrong API call
  
  // Find appropriate connection information  
  ConnectionInfo* c = connections.find(destination);
  if (c == nullptr) {
    LOG_INFO("WARNING: No MA[%s] found: message has been discarded.",
      destination.c_str());
    return;    
  }
  
  try {
    // Write the next SDU
    // the const_cast is necessary as the RINA IPC API is defined as a
    // non const ie. void * only.
    int bytes_written = rina::ipcManager->writeSDU(c->get_port(), 
          const_cast<void*>(message), size);
    cout << "Sent message to MA[" << c->get_id() << "]: " 
      << bytes_written << " bytes sent." << endl;
    c->ok();
  } catch (rina::WriteSDUException &e) {
    if (c->failed()) {
      // notify manager of failed connection
      LOG_WARN("Three failed attempts made to write an SDU, closing flow[%d].",
        c->get_port());
      notify_connection_gone(c);
    }
    LOG_WARN("WriteSDUException in writeSDU: %s", e.what());
  } catch (rina::Exception &e) {
    // These exceptions are assumed fatal
    LOG_WARN("Exception in writeSDU: %s, closing flow[%d]", e.what(), 
      c->get_port());
    // notify manager of failed connection
    notify_connection_gone(c);
  }
}

// Allow testing of the connenction management functionality
int MAWorker::count_flows() {
  return connections.getEntries().size();
}

bool MAWorker::check_flow(string ma) {
  return (connections.find(ma) != nullptr);
}


void MAWorker::open_connection(rina::messages::CDAPMessage& message) {
  //info("Open connection requested [port-id=" + port_id + "].");
  int abstract_syntax = message.abssyntax();
  int version = message.version();
  long message_id = message.invokeid();

  // info("Message from MA [" + message.getSrcApName() + ":"
  //     + message.getSrcApInst() + ":" + message.getSrcAEName() + ":"
  //     + message.getSrcAEInst() + "]");
  // info("             to [" + message.getDestApName() + ":"
  //     + message.getDestApInst() + ":" + message.getDestAEName() + ":"
  //     + message.getDestAEInst() + "]");
  CDAPMessage* builder = CDAPMessage::default_instance().New();
  builder->set_abssyntax(abstract_syntax);
  builder->set_version(version);
  builder->set_invokeid(message_id);
  // Set opcode
  builder->set_opcode(opCode_t::M_CONNECT_R);
  builder->set_flags(F_NO_FLAGS);

  // Source bits
  builder->set_srcapname(message.destapname());
  builder->set_srcapinst(message.destapinst());
  if (message.has_destaename()) {
    builder->set_srcaename(message.destaename());
    builder->set_srcaeinst(message.destaeinst());
  }
  // Destination bits
  builder->set_destapname(message.srcapname());
  builder->set_destapinst(message.srcapinst());
  if (message.has_srcaename()) {
    builder->set_destaename(message.srcaename());
    builder->set_destaeinst(message.srcaeinst());
  }
  // Reply bits
  builder->set_result(0);
  builder->set_resultreason(string("OK"));

  if (message.has_destaename()) {
    LOG_INFO("CDAP OPEN: Received from [%s:%s:%s:%s].", 
        builder->destapname().c_str(), builder->destapinst().c_str(), 
        builder->destaename().c_str(), builder->destaeinst().c_str());
  } else {
    LOG_INFO("CDAP OPEN: Received from [%s:%s].", builder->destapname().c_str(),
        builder->destapinst().c_str());
  }  
  int size = builder->ByteSize();
  void* buf = new unsigned char[size];
  if (builder->SerializeToArray(buf,size)) {
    rina::ApplicationProcessNamingInformation 
      destination(builder->destapname(), builder->destapinst());
    send_message(buf, size, destination.getProcessNamePlusInstance());
    LOG_INFO("CDAP OPEN: Accepting response issued (%d,v%d)", abstract_syntax, 
    version);
  } else {
    LOG_INFO("CDAP OPEN: No response made, encoding failed");     
  }
}
