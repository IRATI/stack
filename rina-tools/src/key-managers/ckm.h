//
// Central Key Manager, just for demo purposes (does not interact with
// Manager yet)
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

#ifndef CKM_H_
#define CKM_H_

#include "server.h"

class CKMWorker : public ServerWorker {
public:
	CKMWorker(rina::ThreadAttributes * threadAttributes,
		  const std::string& creds_folder,
	          Server * serv);
	~CKMWorker() throw() { };
	int internal_run();

private:
        std::string creds_location;
        rina::Timer timer;
        CancelFlowTimerTask * last_task;
};

class CentralKeyManager: public Server
{
public:
	CentralKeyManager(const std::list<std::string>& dif_names,
			  const std::string& app_name,
			  const std::string& app_instance,
			  const std::string& creds_folder);
        ~CentralKeyManager() { };

protected:
        ServerWorker * internal_start_worker(rina::FlowInformation flow);

private:
        std::string creds_location;
};

#endif /* CKM_H_ */
