//
// Echo CDAP Client
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Bernat Gastón <bernat.gaston@i2cat.net>
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

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <librina/librina.h>
#include <librina/cdap_v2.h>

#include "application.h"

class Client;

class Client : public Application, public rina::cdap::CDAPCallbackInterface
{
	friend class APPcallback;
 public:
	Client(const std::list<std::string>& dif_names, const std::string& apn,
	       const std::string& api, const std::string& server_apn,
	       const std::string& server_api, bool quiet, unsigned long count,
	       bool registration, unsigned int wait, int g, int dw);
	void run();
	~Client();
	void open_connection_result(const rina::cdap_rib::con_handle_t &con,
			            const rina::cdap_rib::result_info &res);
	void close_connection_result(const rina::cdap_rib::con_handle_t &con,
			             const rina::cdap_rib::result_info &res);
	void remote_read_result(const rina::cdap_rib::con_handle_t &con,
                   const rina::cdap_rib::obj_info_t &obj,
                   const rina::cdap_rib::res_info_t &res,
                   const rina::cdap_rib::flags_t &flags,
                   const int invoke_id);
 protected:
	void createFlow();
	void cacep();
	void sendReadRMessage();
	void release();
	void destroyFlow();

 private:
	std::string server_name;
	std::string server_instance;
	bool quiet;
	unsigned long echo_times;  // -1 is infinite
	bool client_app_reg;
	unsigned int wait;
	int gap;
	int dealloc_wait;
	rina::FlowInformation flow_;
	rina::cdap::CDAPProviderInterface* cdap_prov_;
	rina::cdap_rib::con_handle_t con_;
	unsigned long count_;
	bool keep_running_;
	rina::Sleep sleep_wrapper;
};

#endif//CLIENT_HPP
