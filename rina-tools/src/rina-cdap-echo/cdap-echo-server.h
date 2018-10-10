//
// Echo CDAP Server
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Bernat Gastón <bernat.gaston@i2cat.net>e>
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

#ifndef EC_SERVER_HPP
#define EC_SERVER_HPP

#include <librina/librina.h>
#include <time.h>
#include <signal.h>
#include <librina/cdap_v2.h>

#include "server.h"

class ConnectionCallback : public rina::cdap::CDAPCallbackInterface {

public:
	ConnectionCallback(bool *keep_serving);
	void open_connection(rina::cdap_rib::con_handle_t &con,
			     const rina::cdap::CDAPMessage& m);
	void remote_read_request(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::filt_info_t &filt,
			int message_id);
	void close_connection(const rina::cdap_rib::con_handle_t &con,
			      const rina::cdap_rib::flags_t &flags, int message_id);

	rina::cdap::CDAPProviderInterface* get_provider(){
		if(!prov_)
			prov_ = rina::cdap::getProvider();
		return prov_;
	}

private:
	bool *keep_serving_;
	rina::cdap::CDAPProviderInterface *prov_;
};

class CDAPEchoWorker : public ServerWorker {
public:
	CDAPEchoWorker(int port, int fd,
			     unsigned int max_sdu_size,
			     Server * serv);
	~CDAPEchoWorker() throw() { };
	int internal_run();

private:
	void serveEchoFlow();

	int port_id;
        int fd;
	unsigned int max_sdu_size;
	rina::Sleep sleep_wrapper;
};

class CDAPEchoServer : public Server {
public:
	CDAPEchoServer(const std::list<std::string>& dif_names,
		       const std::string& app_name,
		       const std::string& app_instance,
		       const int dealloc_wait);

protected:
	ServerWorker * internal_start_worker(rina::FlowInformation flow);

private:
	int dw;
	static const unsigned int max_sdu_size_in_bytes;
};

#endif
