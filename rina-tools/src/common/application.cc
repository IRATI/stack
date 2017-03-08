/*
 * Echo Application
 *
 * Addy Bombeke <addy.bombeke@ugent.be>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <iostream>
#include <librina/librina.h>

#define RINA_PREFIX     "rina-echo-time"
#include <librina/logs.h>

#include "application.h"

using namespace std;
using namespace rina;

Application::Application(const std::list<std::string>& dif_names_,
                         const string& app_name_,
                         const string& app_instance_) :
        dif_names(dif_names_),
        app_name(app_name_),
        app_instance(app_instance_)
{ }

void Application::applicationRegister()
{
        ApplicationRegistrationInformation ari;
        RegisterApplicationResponseEvent *resp;
        unsigned int seqnum;
        IPCEvent *event;
        int successful_regs = 0;

        ari.ipcProcessId = 0;  // This is an application, not an IPC process
        ari.appName = ApplicationProcessNamingInformation(app_name,
                                                          app_instance);

        for (std::list<std::string>::iterator it = dif_names.begin();
        		it != dif_names.end(); ++ it)
        {
        	if (it->compare("") == 0) {
        		ari.applicationRegistrationType = APPLICATION_REGISTRATION_ANY_DIF;
        	} else {
        		ari.applicationRegistrationType = APPLICATION_REGISTRATION_SINGLE_DIF;
        		ari.difName = ApplicationProcessNamingInformation(*it, string());
        	}

        	// Request the registration
        	seqnum = ipcManager->requestApplicationRegistration(ari);

        	// Wait for the response to come
        	for (;;) {
        		event = ipcEventProducer->eventWait();
        		if (event && event->eventType ==
        				REGISTER_APPLICATION_RESPONSE_EVENT &&
					event->sequenceNumber == seqnum) {
        			break;
        		}
        	}

        	resp = dynamic_cast<RegisterApplicationResponseEvent*>(event);

        	// Update librina state
        	if (resp->result == 0) {
        		ipcManager->commitPendingRegistration(seqnum, resp->DIFName);
        		LOG_INFO("Application registered in DIF %s",
        			 it->c_str());
        		successful_regs++;
        	} else {
        		ipcManager->withdrawPendingRegistration(seqnum);
        		LOG_WARN("Failed to register application in DIF %s",
        			  it->c_str());
        	}
        }

        if (successful_regs == 0)
        	throw ApplicationRegistrationException("Could not register application to any DIF");
}

const uint Application::max_buffer_size = 1 << 16;
