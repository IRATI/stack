/*
 * DIF Allocator
 *
 *    Eduard Grasa     <eduard.grasa@i2cat.net>
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

#include <dirent.h>
#include <errno.h>
#include <poll.h>

#define RINA_PREFIX     "ipcm.dif-allocator"
#include <librina/logs.h>

#include "configuration.h"
#include "dif-allocator.h"

using namespace std;


namespace rinad {

// Class DIF Allocator
DIFAllocator * DIFAllocator::create_instance(const DIFAllocatorConfig& da_config,
					     rina::ApplicationProcessNamingInformation& da_name)
{
	DIFAllocator * result;

	if (da_config.type == StaticDIFAllocator::TYPE) {
		result = new StaticDIFAllocator();
		result->set_config(da_config, da_name);
	} else if (da_config.type == DynamicDIFAllocator::TYPE) {
		result = new DynamicDIFAllocator();
		result->set_config(da_config, da_name);
	} else {
		result = NULL;
	}

	return result;
}

//Class Static DIF Allocator
const std::string StaticDIFAllocator::DIF_DIRECTORY_FILE_NAME = "da.map";
const std::string StaticDIFAllocator::TYPE = "static-dif-allocator";

StaticDIFAllocator::StaticDIFAllocator() : DIFAllocator()
{
}

StaticDIFAllocator::~StaticDIFAllocator()
{
}

int StaticDIFAllocator::set_config(const DIFAllocatorConfig& da_config,
				   rina::ApplicationProcessNamingInformation& da_name)
{
	std::string folder_name;
	rina::Parameter folder_name_parm;
	stringstream ss;
	std::string::size_type pos;

	if (da_config.parameters.size() == 0) {
		LOG_ERR("Could not find config folder name, wrong config");
		return -1;
	}

	folder_name_parm = da_config.parameters.front();
	pos = folder_name_parm.value.rfind("/");
	if (pos == std::string::npos) {
		ss << ".";
	} else {
		ss << folder_name_parm.value.substr(0, pos);
	}
	folder_name = ss.str();
	ss << "/" << DIF_DIRECTORY_FILE_NAME;
	fq_file_name = ss.str();

	LOG_INFO("DIF Directory file: %s", fq_file_name.c_str());

	//load current mappings
	if (!parse_app_to_dif_mappings(fq_file_name, dif_directory)) {
		LOG_ERR("Problems loading initial directory");
		return -1;
	}

	print_directory_contents();

	return 0;
}

void StaticDIFAllocator::local_app_registered(const rina::ApplicationProcessNamingInformation& local_app_name,
		          	  	      const rina::ApplicationProcessNamingInformation& dif_name)
{
	//Ignore
}

void StaticDIFAllocator::local_app_unregistered(const rina::ApplicationProcessNamingInformation& local_app_name,
		            	    	    	const rina::ApplicationProcessNamingInformation& dif_name)
{
	//Ignore
}

da_res_t StaticDIFAllocator::lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
                			     	       rina::ApplicationProcessNamingInformation& result,
						       const std::list<std::string>& supported_difs)
{
	rina::ReadScopedLock g(directory_lock);
        string encoded_name = app_name.getEncodedString();
        std::list< std::pair<std::string, std::string> >::iterator it;
        std::list<std::string>::const_iterator jt;

        for (it = dif_directory.begin(); it != dif_directory.end(); ++it) {
        	if (it->first == encoded_name) {
        		for (jt = supported_difs.begin(); jt != supported_difs.end(); ++jt) {
        			if (it->second == *jt) {
        				result.processName = it->second;
        				return DA_SUCCESS;
        			}
        		}
        	}
        }

        return DA_FAILURE;
}

void StaticDIFAllocator::update_directory_contents()
{
	rina::WriteScopedLock g(directory_lock);
	dif_directory.clear();
	if (!parse_app_to_dif_mappings(fq_file_name, dif_directory)) {
	    LOG_ERR("Problems while updating DIF Allocator Directory!");
        } else {
	    LOG_DBG("DIF Allocator Directory updated!");
	    print_directory_contents();
        }
}

void StaticDIFAllocator::print_directory_contents()
{
	std::list< std::pair<std::string, std::string> >::iterator it;
	std::stringstream ss;

	ss << "Application to DIF mappings" << std::endl;
	for (it = dif_directory.begin(); it != dif_directory.end(); ++it) {
		ss << "Application name: " << it->first
		   << "; DIF name: " << it->second << std::endl;
	}

	LOG_DBG("%s", ss.str().c_str());
}

//Class Dynamic DIF Allocator
const std::string DynamicDIFAllocator::TYPE = "dynamic-dif-allocator";

DynamicDIFAllocator::DynamicDIFAllocator() : DIFAllocator()
{
}

DynamicDIFAllocator::~DynamicDIFAllocator()
{
}

int DynamicDIFAllocator::set_config(const DIFAllocatorConfig& da_config,
				    rina::ApplicationProcessNamingInformation& da_name)
{
	//TODO parse config
	daf_name = da_config.daf_name;
	dap_name = da_config.dap_name;

	da_name.processName = da_config.daf_name.processName;
	da_name.processInstance = da_config.dap_name.processName;
}

void DynamicDIFAllocator::local_app_registered(const rina::ApplicationProcessNamingInformation& local_app_name,
		          	  	       const rina::ApplicationProcessNamingInformation& dif_name)
{
	//TODO
}

void DynamicDIFAllocator::local_app_unregistered(const rina::ApplicationProcessNamingInformation& local_app_name,
		            	    	    	 const rina::ApplicationProcessNamingInformation& dif_name)
{
	//Ignore registrations from the DIF Allocator itself
	if (local_app_name.processName == daf_name.processName &&
			local_app_name.processInstance == dap_name.processName) {
		return;
	}

	//TODO
}

da_res_t DynamicDIFAllocator::lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
			       	   	   	        rina::ApplicationProcessNamingInformation& result,
							const std::list<std::string>& supported_difs)
{
	//TODO
}

void DynamicDIFAllocator::update_directory_contents()
{
	//Ignore
}

} //namespace rinad
