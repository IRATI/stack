//
// Manager
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

#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <string>
#include <librina/application.h>
#include <librina/cdap_v2.h>
#include "server.h"

static const unsigned int max_sdu_size_in_bytes = 10000;

class ConnectionCallback : public rina::cdap::CDAPCallbackInterface {
 public:
	void open_connection(const rina::cdap_rib::con_handle_t &con,
		     const rina::cdap::CDAPMessage& message);
	void remote_create_result(const rina::cdap_rib::con_handle_t &con,
			  const rina::cdap_rib::obj_info_t &obj,
			  const rina::cdap_rib::res_info_t &res,
			  const int invoke_id);
	void remote_read_result(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res,
			const rina::cdap_rib::flags_t &flags,
			const int invoke_id);
};

class ManagerWorker : public ServerWorker {
public:
	ManagerWorker(rina::ThreadAttributes * threadAttributes,
	              rina::FlowInformation flow,
		      unsigned int max_sdu_size,
	              Server * serv);
	~ManagerWorker() throw() { };
	int internal_run();

private:
        static const std::string IPCP_1;
        static const std::string IPCP_2;
        static const std::string IPCP_3;
        void operate(rina::FlowInformation flow);
        void cacep(int port_id);
        bool createIPCP_1(int port_id);
        bool createIPCP_2(int port_id);
        bool createIPCP_3(int port_id);
        void queryRIB(int port_id, std::string name);

        rina::FlowInformation flow_;
	unsigned int max_sdu_size;
	rina::cdap::CDAPProviderInterface *cdap_prov_;
};

class Manager : public Server {
 public:
	Manager(const std::string& dif_name,
		const std::string& apn,
		const std::string& api);
	~Manager() { };

	void run();

 private:
	static const std::string mad_name;
	static const std::string mad_instance;
	rina::cdap_rib::con_handle_t con_;
        ConnectionCallback callback;
	std::string dif_name_;
	bool client_app_reg_;

 	ServerWorker * internal_start_worker(rina::FlowInformation flow);
};
#endif//MANAGER_HPP
