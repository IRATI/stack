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
#include <sys/inotify.h>

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
	}
}

DIFAllocator::~DIFAllocator()
{
}

bool DIFAllocator::lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
                			     rina::ApplicationProcessNamingInformation& result)
{
	rina::ReadScopedLock g(directory_lock);
        string encoded_name = app_name.getEncodedString();

        map<string, rina::ApplicationProcessNamingInformation>::iterator it = dif_directory.find(encoded_name);

        if (it != dif_directory.end()) {
                result = it->second;
                return true;
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
        }
}

} //namespace rinad
