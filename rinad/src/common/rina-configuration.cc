/*
 * RINA configuration tree
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

#define RINA_PREFIX "rina-configuration"

#include <librina/common.h>
#include <librina/logs.h>
#include "rina-configuration.h"

#include <iostream>
#include <string>
#include <list>
#include <map>


using namespace std;

namespace rinad {

// Return the configuration of the DIF named "difName" if it is known
// @param difName
// @result
bool RINAConfiguration::lookup_dif_properties(
                const rina::ApplicationProcessNamingInformation& dif_name,
                DIFProperties& result) const
{
        for (list<DIFProperties>::const_iterator it = difConfigurations.begin();
                                        it != difConfigurations.end(); it++) {
                if (it->difName == dif_name) {
                        result = *it;
                        return true;
                }
        }

        return false;
}


// Return the DIF type by supplying the difName
// @param dif_name
// @result
bool RINAConfiguration::lookup_type_by_dif(
                const rina::ApplicationProcessNamingInformation& dif_name,
                std::string& result) const
{
        DIFProperties dif_props;

        for (list<DIFProperties>::const_iterator it = difConfigurations.begin();
                                        it != difConfigurations.end(); it++) {
                if (it->difName == dif_name) {
                        result = it->difType;
                        return true;
                }
        }

        return false;
}


#if 0
// Return the address of the IPC process named "name" if it is known,
// 0 otherwise
// @param dif_name
// @param ipcp_name
// @return
bool RINAConfiguration::lookup_ipcp_address(const string dif_name,
                const rina::ApplicationProcessNamingInformation& ipcp_name,
                unsigned int& result)
{
        DIFProperties dif_props;
        bool found;

        found = lookup_dif_properties(dif_name, dif_props);
        if (!found) {
                return false;
        }

        for (list<KnownIPCProcessAddress>::const_iterator
                it = dif_props.knownIPCProcessAddresses.begin();
                        it != dif_props.knownIPCProcessAddresses.end(); it++) {
                        if (it->name == ipcp_name) {
                                result = it->address;
                                return true;
                        }
        }

        return false;
}
#endif

string LocalConfiguration::toString() const
{
        ostringstream ss;

        ss << "Local Configuration" << endl;
        ss << "\tInstallation path: " << installationPath << endl;
        ss << "\tLibrary path: " << libraryPath << endl;
        ss << "\tLog path: " << logPath << endl;
        ss << "\tConsole port: " << consolePort << endl;

        return ss.str();
}

static void
dump_map(stringstream& ss, const map<string, string>& m,
         unsigned int level)
{
        for (map<string, string>::const_iterator mit = m.begin();
                                mit != m.end(); mit++) {
                for (unsigned int i = 0; i < level; i++) {
                        ss << "\t";
                }
                ss << mit->first << ": " << mit->second << endl;
        }
}

string RINAConfiguration::toString() const
{
        stringstream ss;

        for (list<IPCProcessToCreate>::const_iterator it =
                        ipcProcessesToCreate.begin();
                                it != ipcProcessesToCreate.end(); it++) {
                ss << "IPC process to create:" << endl;

                ss << "\tName: " << it->name.toString() << endl;
                ss << "\tDIF Name: " << it->difName.toString() << endl;

                for (list<NeighborData>::const_iterator nit =
                                it->neighbors.begin();
                                        nit != it->neighbors.end(); nit++) {
                        ss << "\tNeighbor:" << endl;
                        ss << "\t\tName: " << nit->apName.toString() << endl;
                        ss << "\t\tDIF: " << nit->difName.toString() << endl;
                        ss << "\t\tSupporting DIF: " <<
                                nit->supportingDifName.toString() << endl;
                }

                ss << "\tDIFs to register at:" << endl;
                for (list<rina::ApplicationProcessNamingInformation>::
                        const_iterator nit = it->difsToRegisterAt.begin();
                                nit != it->difsToRegisterAt.end(); nit++) {
                        ss << "\t\t" << nit->toString() << endl;
                }
                ss << "\tHost name: " << it->hostname << endl;
                ss << "\tSDU protection options:" << endl;
                dump_map(ss, it->sduProtectionOptions, 2);
                ss << "\tParameters:" << endl;
                dump_map(ss, it->parameters, 2);
        }

        for (list<DIFProperties>::const_iterator it = difConfigurations.begin();
                                it != difConfigurations.end(); it++) {
                ss << "DIF configuration/properties:" << endl;
                ss << "\tDIF name: " << it->difName.toString() << endl;
                ss << "\tDIF type: " << it->difType << endl;
                // TODO complete
                ss << "\tParameters: " << endl;
                for (map<string, string>::const_iterator pit =
                        it->configParameters.begin();
                                pit != it->configParameters.end(); pit++) {
                        ss << "\t\t" << pit->first << ":" << pit->second << endl;
                }
        }

        ss << "Application --> DIF mappings:" << endl;
        for (map<string, rina::ApplicationProcessNamingInformation>::
                        const_iterator mit = applicationToDIFMappings.begin();
                                mit != applicationToDIFMappings.end(); mit++) {
                ss << "\t" << mit->first << ": " << mit->second.toString()
                        << endl;
        }

        return local.toString() + ss.str();
}

bool DIFProperties::lookup_ipcp_address(
                const rina::ApplicationProcessNamingInformation& ipcp_name,
                unsigned int& result)
{
        for (list<KnownIPCProcessAddress>::const_iterator
                it = knownIPCProcessAddresses.begin();
                        it != knownIPCProcessAddresses.end(); it++) {
                        if (it->name.processName.compare(ipcp_name.processName) == 0 &&
                        		it->name.processInstance.compare(ipcp_name.processInstance) == 0) {
                                result = it->address;
                                return true;
                        }
        }

        return false;
}

bool RINAConfiguration::lookup_dif_by_application(
                const rina::ApplicationProcessNamingInformation& app_name,
                rina::ApplicationProcessNamingInformation& result)
{
        string encoded_name = app_name.getEncodedString();

        map<string, rina::ApplicationProcessNamingInformation>::iterator it =
                applicationToDIFMappings.find(encoded_name);

        if (it != applicationToDIFMappings.end()) {
                result = it->second;
                return true;
        }

        return false;
}

}
