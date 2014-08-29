/*
 * Helpers for the RINA daemons
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <cstdlib>
#include <iostream>
#include <vector>

#define RINA_PREFIX     "rinad-helpers"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "helpers.h"

using namespace std;


namespace rinad {

// Returns an IPC process assigned to the DIF specified by @dif_name,
// if any.
rina::IPCProcess *
select_ipcp_by_dif(const rina::ApplicationProcessNamingInformation& dif_name)
{
        const vector<rina::IPCProcess *>& ipcps =
                rina::ipcProcessFactory->listIPCProcesses();

        for (unsigned int i = 0; i < ipcps.size(); i++) {
                rina::DIFInformation dif_info = ipcps[i]->getDIFInformation();
                rina::ApplicationProcessNamingInformation ipcp_name = ipcps[i]->name;

                if (dif_info.dif_name_ == dif_name
                                /* The following OR clause is a temporary hack useful
                                 * for testing with shim dummy. TODO It will go away. */
                                || ipcp_name.processName == dif_name.processName) {
                        return ipcps[i];
                }
        }

        return NULL;
}

// Returns any IPC process in the system, giving priority to
// normal IPC processes.
rina::IPCProcess *
select_ipcp()
{
        const vector<rina::IPCProcess *>& ipcps =
                rina::ipcProcessFactory->listIPCProcesses();

        for (unsigned int i = 0; i < ipcps.size(); i++) {
                if (ipcps[i]->type == rina::NORMAL_IPC_PROCESS) {
                        return ipcps[i];
                }
        }

        for (unsigned int i = 0; i < ipcps.size(); i++) {
                return ipcps[i];
        }

        return NULL;
}

bool
application_is_registered_to_ipcp(
                const rina::ApplicationProcessNamingInformation& app_name,
                rina::IPCProcess *slave_ipcp)
{
        const list<rina::ApplicationProcessNamingInformation>&
                registered_apps = slave_ipcp->getRegisteredApplications();

        for (list<rina::ApplicationProcessNamingInformation>::const_iterator
                        it = registered_apps.begin();
                                it != registered_apps.end(); it++) {
                if (app_name == *it) {
                        return true;
                }
        }

        return false;
}

rina::IPCProcess *
lookup_ipcp_by_port(unsigned int port_id)
{
        const vector<rina::IPCProcess *>& ipcps =
                rina::ipcProcessFactory->listIPCProcesses();
        rina::FlowInformation info;

        for (unsigned int i = 0; i < ipcps.size(); i++) {
                if (ipcps[i]->getFlowInformation(port_id, info)) {
                        return ipcps[i];
                }
        }

        return NULL;
}

void
collect_flows_by_application(const rina::ApplicationProcessNamingInformation&
                             app_name, list<rina::FlowInformation>& result)
{
        const vector<rina::IPCProcess *>& ipcps =
                rina::ipcProcessFactory->listIPCProcesses();

        result.clear();

        for (unsigned int i = 0; i < ipcps.size(); i++) {
                const list<rina::FlowInformation>& flows =
                        ipcps[i]->getAllocatedFlows();

                for (list<rina::FlowInformation>::const_iterator it =
                        flows.begin(); it != flows.end(); it++) {
                        if (it->localAppName == app_name) {
                                result.push_back(*it);
                        }
                }
        }
}

rina::IPCProcess *
lookup_ipcp_by_id(unsigned int id)
{
        return rina::ipcProcessFactory->getIPCProcess(id);
}

int string2int(const string& s, int& ret)
{
        char *dummy;
        const char *cstr = s.c_str();

        ret = strtoul(cstr, &dummy, 10);
        if (!s.size() || *dummy != '\0') {
                ret = ~0U;
                return -1;
        }

        return 0;
}

}
