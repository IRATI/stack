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
#include <encoders/CDAP.pb.h> // For CDAPMessage

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
}
                
// Do the internal work necessary
int DMSWorker::internal_run() {
  int retries = 30;
  unsigned int nap_time = 1000; // in millisecs
  
  // Loop attempting to make a connection
  while (ws == NULL) {
    ws = WebSocket::from_url(ws_address);
    usleep(nap_time * 1000);
    if (retries > 0) {
      cerr << "Error: No connection made to DMS@" << ws_address 
        << ", retrying ..." << endl;
      retries--;
    } else {
      // bail out 
      return 0;
    }
  }

  // Okay now start processing the messages
  std::cerr << "Info: Connection made to DMS@" << ws_address << std::endl;
  
  // Complete handshake with server
  ws->send("hello dms");
  while (ws->getReadyState() != WebSocket::CLOSED) {
    ws->poll();
    auto myself = this;
    ws->dispatch ( [myself](const std::string& m) { myself->process_message(m); });
  }
  delete ws;
  ws = NULL;    
  // Okay now start processing the messages
  std::cerr << "Info: Disconnected from DMS@" << ws_address << std::endl;
  
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
          std::cout << "ESD >>> " << message.substr(pstart+10,len).c_str() << std::endl;                
        } else {
          std::cout << "ESD >>> Disconnect received" << std::endl;                          
        }
      } else {
        std::cout << "ESD >>> Disconnect received" << std::endl;                          
      }      
      ws->close();
    // Is it CDAP
    } else if (cdap_filter->match(message)) {
      std::cout << "CDAP >>> " << message.c_str() << std::endl;      
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
              cout << "Message is " << cdap_message.IsInitialized() 
                << " for " << dest.getProcessNamePlusInstance().c_str() << endl;
              // Now dispatch to the MA
              send_message(buf, size, dest.getProcessNamePlusInstance());          
          } else {
            cout << "warning: missing destination on message! " << endl;
          }
        }
      } else {
        cout << "warning: failed to encode message! " << endl;
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
      const DescriptorPool* dp = DescriptorPool::generated_pool();
      const Descriptor* d  = dp->FindMessageTypeByName(type);
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
      }
    }
  }
}



/*
 * Handover interface
 */
 

// Send outgoing message to DMS
void DMSWorker::send_message(const std::string & message) {
  if (ws == nullptr) {
    cout << "WARNING: No DMS found: " 
      << " message has been discarded." << endl;
    return;    
  }

  try {
    // Write the next JSON message
    ws->send(message);
    cout << "Sent message to DMS"  << endl;
  } catch (exception &e) {
    cout << "WARNING: Cant send message to DMS: " 
      << " message has been discarded." << endl;
    cout << "Exception : " << e.what() << endl;
  }  
}

// Send outgoing message to MA
void DMSWorker::send_message(const void* message, int size, 
    const std::string& destination) {
  Connector* c = reinterpret_cast<Connector*>(server);
  assert(c != nullptr);
  Handover* ma = reinterpret_cast<Handover*>(c->find_ma_worker());
  if (ma != nullptr) {
    // Found MA instance so just send it
    ma->send_message(message, size, destination);
  } else {
    cout << "INFO: No MA!" << endl;
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
  //LOG_INFO(b.c_str());
  cout << "INFO:" << b << endl;
}
