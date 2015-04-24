/*
 * Plugin information retrieval from manifest manifest
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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
#include <string>
#include <fstream>

#include <librina/common.h>
#include <librina/ipc-manager.h>

#define RINA_PREFIX "ipcm"
#include <librina/logs.h>

#include "json/json.h"
#include "ipcm.h"


using namespace std;

namespace rinad {

ipcm_res_t
IPCManager_::plugin_get_info(const std::string& plugin_name)
{
        string plugin_path = IPCPPLUGINSDIR + plugin_name + ".manifest";

        // Parse config file with jsoncpp
        Json::Value  root;
        Json::Reader reader;
        Json::Value  v;
        ifstream     manifest;

        manifest.open(plugin_path.c_str());
        if (manifest.fail()) {
                LOG_ERR("Failed to open manifest file for plugin %s",
                         plugin_name.c_str());
                return IPCM_FAILURE;
        }

        if (!reader.parse(manifest, root, false)) {
                LOG_ERR("Failed to parse JSON manifest file: %s",
                        reader.getFormatedErrorMessages().c_str());

                return IPCM_FAILURE;
        }

        manifest.close();

        v = root["PluginName"];
        if (v == 0) {
                LOG_ERR("Error parsing manifest: Plugin name is missing in "
                        "manifest for plugin %s", plugin_name.c_str());
                return IPCM_FAILURE;
        }

        if (v.asString() != plugin_name) {
                LOG_ERR("Plugin name in the manifest file must match\n");
                return IPCM_FAILURE;
        }

        return IPCM_SUCCESS;
}

} //rinad namespace
