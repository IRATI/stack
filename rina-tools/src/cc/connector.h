//
// CDAP Connector
//
// Micheal Crotty <mcrotty@tssg.org>
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

#ifndef CONNECTOR_HPP
#define CONNECTOR_HPP

#include <string>
#include <librina/application.h>
#include <librina/cdap_v2.h>
#include <librina/json/json.h> // JSON parsing

#include "server.h"
#include "easywsclient.h"
#include "eventfilters.h"

static const unsigned int max_sdu_size_in_bytes = 10000;


class Connection {
public:
  std::string id;
  rina::FlowInformation flow;
  long portId;
};


//
// Class that deals with the DMS Manager.
//
class DMSWorker : public ServerWorker {
public:
	DMSWorker(rina::ThreadAttributes * threadAttributes,
	              const std::string& flow,
		      unsigned int max_sdu_size,
	              Server * serv);
	~DMSWorker() throw() { };
	int internal_run();

  void process_message(const std::string & message);

protected:
  // Parse the JSON message
  void parse_header(const Json::Value &root, const std::string& name);
  void parse_contents(const Json::Value &root, const std::string& name);

private:
	unsigned int               max_sdu_size;
  std::string                ws_address;
  easywsclient::WebSocket*   ws;
  ES_EventFilter*            shutdown_filter;
  ES_EventFilter*            cdap_filter;  
};

// 
// Class that deals with the MAs
//
class MAWorker : public ServerWorker {
public:
	MAWorker(rina::ThreadAttributes * threadAttributes,
	              rina::FlowInformation flow,
		      unsigned int max_sdu_size,
	              Server * serv);
	~MAWorker() throw() { };
	int internal_run();

private:
        rina::FlowInformation flow_;
	unsigned int max_sdu_size;
	//rina::cdap::CDAPProviderInterface *cdap_prov_;
};


class Connector : public Server {
 public:
	Connector(const std::list<std::string>& dif_names,
		const std::string& apn,
		const std::string& api,
    const std::string& ws);
	~Connector() { };

  // Help find the other worker
  ServerWorker* find_worker(ServerWorker* whosAsking) const;

  void ws_run();
  void rina_run();
  

	void run();

 private:
	//static const std::string mad_name;
	//static const std::string mad_instance;
	std::string dif_name_;
	bool client_app_reg_;

  std::string ws_address_;

  //std::map<std:string,Connection> w

  ServerWorker * internal_start_worker(rina::FlowInformation flow);
  ServerWorker * internal_start_worker(const std::string& ws);
};

#endif //MANAGER_HPP
