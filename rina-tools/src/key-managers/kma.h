//
// Local Key Management Agent
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

#ifndef KMA_HPP
#define KMA_HPP

#include <string>
#include <librina/concurrency.h>
#include <librina/timer.h>

#include "application.h"


class KeyManagementAgent: public Application {
public:
	KeyManagementAgent(const std::string& creds_folder,
			   const std::list<std::string>& dif_name,
			   const std::string& apn,
			   const std::string& api,
			   const std::string& ckm_apn,
			   const std::string& ckm_api,
			   bool  quiet);
       void run();
       int readSDU(int portId, void * sdu, int maxBytes, unsigned int timout);
       void startCancelFloodFlowTask(int port_id);
       ~KeyManagementAgent() {};
protected:
        int createFlow();
        void destroyFlow(int port_id);

private:
        std::string creds_location;
        std::string dif_name;
        std::string ckm_name;
        std::string ckm_instance;
        bool quiet;
        rina::Timer timer;
};
#endif//KMA_HPP
