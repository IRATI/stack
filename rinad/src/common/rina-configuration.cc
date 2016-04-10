/*
 * RINA configuration tree
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#define RINA_PREFIX "rinad.rina-configuration"

#include <librina/common.h>
#include <librina/logs.h>
#include "rina-configuration.h"

#include <iostream>
#include <string>
#include <list>
#include <map>


using namespace std;

namespace rinad {

// Return the name of the template of the DIF named "difName" if it is known
// @param difName
// @result
bool RINAConfiguration::lookup_dif_template_mappings(
                const rina::ApplicationProcessNamingInformation& dif_name,
                DIFTemplateMapping& result) const
{
        for (list<DIFTemplateMapping>::const_iterator it = difConfigurations.begin();
                                        it != difConfigurations.end(); it++) {
                if (it->dif_name == dif_name) {
                        result = *it;
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
        ss << "\tConsole socket: " << consoleSocket << endl;

	ss << "\tPlugins paths:" <<endl;
	for (list<string>::const_iterator lit = pluginsPaths.begin();
					lit != pluginsPaths.end(); lit++) {
		ss << "\t\t" << *lit << endl;
	}

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

        for (list<DIFTemplateMapping>::const_iterator it = difConfigurations.begin();
                                it != difConfigurations.end(); it++) {
                ss << "\tDIF name: " << it->dif_name.toString() << endl;
                ss << "\tTemplate name: " << it->template_name << endl;
        }

        return local.toString() + ss.str();
}

bool DIFTemplate::lookup_ipcp_address(
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

std::string DIFTemplate::toString()
{
	std::stringstream ss;
	ss << "* TEMPLATE " << templateName << std::endl;
	ss << std::endl;
	ss << "** DIF type: " << difType << std::endl;
	ss << std::endl;
	if (difType == rina::NORMAL_IPC_PROCESS) {
		ss << "** DATA TRANSFER CONSTANTS **"<<std::endl;
		ss << dataTransferConstants.toString()<<std::endl;
		ss << std::endl;

		if (qosCubes.size() != 0) {
			ss << "** QOS CUBES **" << std::endl;
			ss << std::endl;
			std::list<rina::QoSCube>::iterator it;
			for (it = qosCubes.begin(); it != qosCubes.end(); ++ it) {
				ss << "*** QOS CUBE " << it->name_ << " ***" <<std::endl;
				ss << it->toString() << std::endl;
				ss << std::endl;
			}
		}

		ss << "** ENROLLMENT TASK **"<<std::endl;
		ss << etConfiguration.toString();
		ss << std::endl;

		ss << "** RELAYING AND MULTIPLEXING TASK ** "<<std::endl;
		ss <<rmtConfiguration.toString();
		ss << std::endl;

		ss << "** FLOW ALLOCATOR ** "<<std::endl;
		ss <<faConfiguration.toString();
		ss << std::endl;

		ss << "** ROUTING ** "<<std::endl;
		ss <<routingConfiguration.toString();
		ss << std::endl;

		ss << "** RESOURCE ALLOCATOR ** "<<std::endl;
		ss <<raConfiguration.toString();
		ss << std::endl;

		ss << "** NAMESPACE MANAGER ** "<<std::endl;
		ss <<nsmConfiguration.toString();
		ss << std::endl;

		if (knownIPCProcessAddresses.size() > 0) {
			ss << "** KNOWN IPCP ADDRESSES **" <<std::endl;
			std::list<KnownIPCProcessAddress>::iterator it;
			for (it = knownIPCProcessAddresses.begin();
					it != knownIPCProcessAddresses.end(); ++it) {
				ss << "   IPCP name: " << it->name.getProcessNamePlusInstance();
				ss << " address: " << it->address << std::endl;
			}
			ss << std::endl;
		}

		if (addressPrefixes.size() > 0) {
			ss << "** ADDRESS PREFIXES **" <<std::endl;
			std::list<AddressPrefixConfiguration>::iterator it;
			for (it = addressPrefixes.begin();
					it != addressPrefixes.end(); ++it) {
				ss << "   Organization: " << it->organization;
				ss << " address prefix: " << it->addressPrefix << std::endl;
			}
			ss << std::endl;
		}

		ss << "** SECURITY MANAGER **"<< std::endl;
		ss << secManConfiguration.toString() <<std::endl;
	}

	if (configParameters.size() != 0) {
		ss << "** CONFIG PARAMETERS **" << std::endl;
		std::map<std::string, std::string>::iterator it;
		for (it = configParameters.begin(); it != configParameters.end(); ++ it) {
			ss << "   Name: " << it->first;
			ss << "   Value: " << it->second << std::endl;
		}
		ss << std::endl;
	}

	ss << std::endl;

	return ss.str();
}

}
