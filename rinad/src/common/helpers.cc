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


/* Returns an IPC process assigned to the DIF specified by @dif_name,
 * if any.
 */
rina::IPCProcess *
select_ipcp_by_dif(const rina::ApplicationProcessNamingInformation& dif_name)
{
        const vector<rina::IPCProcess *>& ipcps =
                rina::ipcProcessFactory->listIPCProcesses();

        for (unsigned int i = 0; i < ipcps.size(); i++) {
                rina::DIFInformation dif_info = ipcps[i]->getDIFInformation();

                if (dif_info.dif_name_ == dif_name) {
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
