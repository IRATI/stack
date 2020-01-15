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

#ifndef RINAD_RINA_CONFIGURATION_H
#define RINAD_RINA_CONFIGURATION_H

#ifdef __cplusplus

#include <librina/configuration.h>
#include <librina/common.h>

#include <string>
#include <sstream>
#include <list>
#include <map>


namespace rinad {

/// Represent an application name to DIF name mapping
struct AppToDIFMapping {
	rina::ApplicationProcessNamingInformation app_name;
	std::list<std::string> dif_names;
};

/*
 * Assigns an address prefix to a certain substring (the organization)
 * that is part of the application process name
 */
struct AddressPrefixConfiguration {

        static const unsigned int MAX_ADDRESSES_PER_PREFIX = 16;

        /*
         * The address prefix (it is the first valid address for the
         * given subdomain)
         */
        unsigned int addressPrefix;

        /* The organization whose addresses start by the prefix */
        std::string organization;

        AddressPrefixConfiguration() : addressPrefix(0) { }
};

struct NMinusOneFlowsConfiguration {

        /* The N-1 QoS id required by a management flow */
        int managementFlowQoSId;

        /*
         * The N-1 QoS id required by each N-1 flow  that has to be created
         * by the enrollee IPC Process after enrollment has been
         * successfully completed
         */
        std::list<int> dataFlowsQoSIds;

        NMinusOneFlowsConfiguration() : managementFlowQoSId(2) { }
};

/* The configuration of a known IPC Process */
struct KnownIPCProcessAddress {

        /* The application name of the remote IPC Process */
        rina::ApplicationProcessNamingInformation name;

        /* The address of the remote IPC Process */
        unsigned int address;

        KnownIPCProcessAddress() : address(0) { }
};

/* The configuration required to create a DIF */
struct DIFTemplate {
	std::string templateName;
        std::string difType;
        rina::DataTransferConstants dataTransferConstants;
        std::list<rina::QoSCube> qosCubes;
        rina::RMTConfiguration rmtConfiguration;
        rina::EnrollmentTaskConfiguration etConfiguration;
        rina::SecurityManagerConfiguration secManConfiguration;
        rina::FlowAllocatorConfiguration faConfiguration;
        rina::NamespaceManagerConfiguration nsmConfiguration;
        rina::ResourceAllocatorConfiguration raConfiguration;
        rina::RoutingConfiguration routingConfiguration;

        /*
         * The addresses of the known IPC Process (apname, address)
         * that can potentially be members of the DIFs I know
         */
        std::list<KnownIPCProcessAddress> knownIPCProcessAddresses;

        /* The address prefixes, assigned to different organizations */
        std::list<AddressPrefixConfiguration> addressPrefixes;

        /* Extra configuration parameters (name/value pairs) */
        std::map<std::string, std::string> configParameters;

        bool lookup_ipcp_address(
                        const rina::ApplicationProcessNamingInformation&,
                        unsigned int& result);

        std::string toString();
};

struct NeighborData {

        rina::ApplicationProcessNamingInformation apName;
        rina::ApplicationProcessNamingInformation supportingDifName;
        rina::ApplicationProcessNamingInformation difName;
};


/* The configuration required to create an IPC Process */
struct IPCProcessToCreate {
	static std::string ALL_SHIM_DIFS;

        rina::ApplicationProcessNamingInformation name;
        rina::ApplicationProcessNamingInformation difName;
        std::list<NeighborData> neighbors;
        std::list<rina::ApplicationProcessNamingInformation> difsToRegisterAt;
        std::list<std::string> n1difsPeerDiscovery;
        std::string hostname;
        /*
         * Specifies what SDU Protection Module should be used for each possible
         * N-1 DIF (being NULL the default one)
         * nMinus1DIFName: sduProtectionType
         */
        std::map<std::string, std::string> sduProtectionOptions;
        std::map<std::string, std::string> parameters;
};

/* Configuration of the local RINA Software instantiation */
struct LocalConfiguration {

        /*
         * The unix socket where the IPC Manager is listening for incoming
         * connections from administrators
         */
        std::string consoleSocket;

        /* The path to the RINA binaries installation in the system */
        std::string installationPath;

        /* The path to the RINA libraries in the system */
        std::string libraryPath;

        /* The path to the RINA log folder in the system */
        std::string logPath;

	/* The paths where to look for policy plugins. */
	std::list<std::string> pluginsPaths;

	/* The system name */
	rina::ApplicationProcessNamingInformation system_name;

        std::string toString() const;

        LocalConfiguration() { }
};

struct DIFTemplateMapping {

	// The name of the DIF
	rina::ApplicationProcessNamingInformation dif_name;

	// The name of the template
	std::string template_name;
};

/*
 * Global configuration for the RINA software
 */
class RINAConfiguration {
 public:
        /*
         * The local software configuration (console port, timeouts, ...)
         */
        LocalConfiguration local;

        /*
         * The list of IPC Process to create when the software starts
         */
        std::list<IPCProcessToCreate> ipcProcessesToCreate;

        /*
         * The configurations of zero or more DIFs
         */
        std::list<DIFTemplateMapping> difConfigurations;

	/*
	 * The path of the configuration file where the configuration
	 * comes from
	 */
	std::string configuration_file;

	/*
	 * The version of the configuration file
	 */
	std::string configuration_file_version;

        bool lookup_dif_template_mappings(
                        const rina::ApplicationProcessNamingInformation& dif_name,
                        DIFTemplateMapping& result) const;

#if 0
        bool lookup_ipcp_address(const std::string dif_name,
                const rina::ApplicationProcessNamingInformation& ipcp_name,
                unsigned int& result);
#endif
        std::string toString() const;
};

DIFTemplate * parse_dif_template(const std::string& file_name,
				 const std::string& template_name);
bool parse_app_to_dif_mappings(const std::string& file_name,
			       std::list< std::pair<std::string, std::string> >& mappings);

}

#endif

#endif
