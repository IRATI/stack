/*
 * Configuration manager
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
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

#ifndef __MA_CONFIGURATION_H__
#define __MA_CONFIGURATION_H__

#include <assert.h>
#include <string>
#include <list>
#include <librina/application.h>
#include <librina/common.h>

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
	ConfManager(const std::string& conf_file);

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
        };

        /**
        * Internal variables containing MAD configuration
        */
        rina::ApplicationProcessNamingInformation app_name;
        std::list<std::string> nms_difs;
        std::list<ManagerConnInfo> manager_connections;
};


}; //namespace mad
}; //namespace rinad

#endif  /* __MA_CONFIGURATION_H__ */
