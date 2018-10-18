/*
 * Configuration manager
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
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

#ifndef __MA_CONFIGURATION_H__
#define __MA_CONFIGURATION_H__

#include <assert.h>
#include <string>
#include <list>
#include <librina/application.h>
#include <librina/common.h>
#include <rina-configuration.h>

/**
* @file configuration.h
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief Management agent Configuration Manager module
*
* TODO: this module might be dropped in the future.
*/

namespace rinad{
namespace mad{

//fwd decl
class ManagementAgent;

/**
* @brief MAD configuration manager component
*/
class ConfManager{

public:
	/**
	* Initialize running state
	*/
	ConfManager(const rinad::RINAConfiguration& config);

	/**
	* Destroy the running state
	*/
	~ConfManager(void);

	/**
	* Reads the configuration source (e.g. a config file) and configures
	* the rest of the modules
	*/
	void configure(ManagementAgent& agent);

private:

        struct ManagerConnInfo {
                rina::ApplicationProcessNamingInformation manager_name;
                std::string manager_dif;
                std::string auth_policy_name;
        };

        /**
        * Internal variables containing MAD configuration
        */
        rina::ApplicationProcessNamingInformation app_name;
        std::list<ManagerConnInfo> manager_connections;
        ManagerConnInfo key_manager_connection;
};


}; //namespace mad
}; //namespace rinad

#endif  /* __MA_CONFIGURATION_H__ */
