//
// Network Manager (single standalone network manager to demo / debut / test
// interaction with Management Agents)
//
// Eduard Grasa <eduard.grasa@i2cat.net>
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

#define RINA_PREFIX "net-manager"
#include <librina/logs.h>
#include "net-manager.h"

#include <rina/api.h>

NetworkManager::NetworkManager(const std::string& app_name,
			       const std::string& app_instance) :
			       	       rina::ApplicationProcess(app_name, app_instance)
{
	std::stringstream ss;

	cfd = 0;
	ss << app_name << "|" << app_instance << "||";
	complete_name = ss.str();
}

NetworkManager::~NetworkManager()
{
	//TODO delete stuff and unregister
}

unsigned int NetworkManager::get_address() const
{
	return 0;
}

void NetworkManager::event_loop(std::list<std::string>& dif_names)
{
	std::list<std::string>::iterator it;
	int nfd;

	// 1 Open RINA descriptor
	cfd = rina_open();
	if (cfd < 0) {
		LOG_DBG("rina_open() failed");
		return;
	}

	// 2 Register to one or more DIFs
	for (it = dif_names.begin(); it != dif_names.end(); ++it) {
		if (rina_register(cfd, it->c_str(), complete_name.c_str(), 0) < 0) {
			LOG_WARN("Failed to register to DIF %s", it->c_str());
		} else {
			LOG_INFO("Registered to DIF %s", it->c_str());
		}
	}

	if (dif_names.size() == 0) {
		if (rina_register(cfd, NULL, complete_name.c_str(), 0) < 0) {
			LOG_WARN("Failed to register at DIF %s", it->c_str());
		} else {
			LOG_INFO("Registered at DIF");
		}
	}

	// Main event loop
	for (;;) {
		// Accept new flow
		nfd = rina_flow_accept(cfd, NULL, NULL, 0);
		LOG_DBG("Accepted flow from remote application, fd = %d", nfd);

		// TODO give it to a new worker, which will proceed with enrollment
	}

	return;
}
