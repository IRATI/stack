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

//Class DIF Allocator
const std::string DIFAllocator::DIF_DIRECTORY_FILE_NAME = "da.map";

DIFAllocator::DIFAllocator(const std::string& folder)
{
	stringstream ss;

	std::string::size_type pos = folder.rfind("/");
	if (pos == std::string::npos) {
		ss << ".";
	} else {
		ss << folder.substr(0, pos);
	}
	folder_name = ss.str();
	ss << "/" << DIF_DIRECTORY_FILE_NAME;
	fq_file_name = ss.str();

	LOG_INFO("DIF Directory file: %s", fq_file_name.c_str());

	//load current mappings
	if (!parse_app_to_dif_mappings(fq_file_name, dif_directory)) {
		LOG_ERR("Problems loading initial directory");
	} else {
		print_directory_contents();
	}
}

DIFAllocator::~DIFAllocator()
{
}

bool DIFAllocator::lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
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
        				return true;
        			}
        		}
        	}
        }

        return false;
}

void DIFAllocator::update_directory_contents()
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

void DIFAllocator::print_directory_contents()
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

} //namespace rinad
