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
#include <list>
#include <string>
#include <fstream>

#include "librina/common.h"
#include "librina/ipc-manager.h"

#define RINA_PREFIX "plugin-info"
#include "librina/logs.h"

#include "json/json.h"


using namespace std;

namespace rina {

int
plugin_get_info(const std::string& plugin_name,
		const std::string& plugins_path,
		std::list<rina::PsInfo>& result)
{
        string plugin_path = plugins_path + "/" +
                             plugin_name + ".manifest";
        ifstream     manifest;
        Json::Value  root;
        Json::Reader reader;
        Json::Value  v;

        manifest.open(plugin_path.c_str());
        if (manifest.fail()) {
                LOG_ERR("Failed to open manifest file for plugin %s",
                         plugin_name.c_str());
                return -1;
        }

        if (!reader.parse(manifest, root, false)) {
                LOG_ERR("Failed to parse JSON manifest file: %s",
                        reader.getFormatedErrorMessages().c_str());

                return -1;
        }

        manifest.close();

        v = root["PluginName"];
        if (v == 0) {
                LOG_ERR("Error parsing manifest: Plugin name is missing in "
                        "manifest for plugin %s", plugin_name.c_str());
                return -1;
        }

        if (v.asString() != plugin_name) {
                LOG_ERR("Plugin name in the manifest file must match\n");
                return -1;
        }

        v = root["Version"];
        if (v == 0) {
                LOG_WARN("Couldn't find Version for plugin %s",
                         plugin_name.c_str());
        }

        v = root["PolicySets"];
        if (v == 0) {
                LOG_WARN("Plugin %s does not declare any policy sets",
                                plugin_name.c_str());
                return 0;
        }

	result.clear();

        for (unsigned int i = 0; i < v.size(); i++) {
                string ps_name = v[i].get("Name", string()).asString();
                string ps_component = v[i].get("Component", string())
                                          .asString();
                string ps_version = v[i].get("Version", string()).asString();

		result.push_back(rina::PsInfo(ps_name, ps_component, ps_version));
        }

        return 0;
}

} //rina namespace
