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

#include "connector.h"
#include "eventsystem.h"  // Various constants used
#include "eventfilters.h"  // Event filtering

using namespace easywsclient;
using namespace std;

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
      std::cerr << "Error: No connection made to DMS@" << ws_address 
        << ", retrying ..." << std::endl;
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
    } else {
      // echo the message
      std::cout << ">>> Not for us" << std::endl;      
    }
    
}



// Parse the header fields

void DMSWorker::parse_header(const Json::Value &root, const std::string& name) {
  //root.get(name);
}

void DMSWorker::parse_contents(const Json::Value &root, const std::string& name) {
  //root.get(name);
  // Pull the contents from the JSON file and complete a CDAPMessage
  
}

/*
 * Log a short but informative message
 *
void DMSWorker::logShortMessage(Builder message) {
  stringstream b(64);
  
  b.append("CDAP ");
  b.append(message.getOpCode());
  switch(message.getOpCode()) {
  // operations on an object
  case M_CREATE:
  case M_DELETE:
  case M_READ:
  case M_WRITE:
  case M_CANCELREAD:
  case M_START:
  case M_STOP:
    b.append(" of ");
    b.append(message.getObjClass());
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
    b.append(message.getObjClass());
    // missing break is intentional
  case M_CONNECT_R:
  case M_RELEASE_R:
    if (message.getResult() != 0) {
      b.append(" FAILED ");
      if (message.hasResultReason()) {
        b.append("(");
        b.append(message.getResultReason());
        b.append(") ");
      }
    } else {
      b.append(" SUCCESSFUL ");
    }
    break;
  }
  b.append("Sending to Manager.");
  //info(b.toString());
  LOG_INFO(b.c_str());
}
*/
