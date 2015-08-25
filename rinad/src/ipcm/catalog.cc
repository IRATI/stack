/*
 * Catalog for policy plugins
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
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/plugin-info.h>

#define RINA_PREFIX "ipcm-catalog"
#include <librina/logs.h>
#include <debug.h>

#include "ipcm.h"
#include "catalog.h"

using namespace std;


namespace rinad {

CatalogPsInfo::CatalogPsInfo(const rina::PsInfo& psinfo,
			     map<string, CatalogPlugin>::iterator plit)
				: PsInfo(psinfo)
{
	plugin = plit;
}

static bool endswith(const string& s, const string& suffix)
{
	if (s.size() < suffix.size()) {
		return false;
	}

	return suffix == s.substr(s.size() - suffix.size(), suffix.size());
}

#define MANIFEST_SUFFIX ".manifest"

void Catalog::import()
{
	const rinad::RINAConfiguration& config = IPCManager->getConfig();

	for (list<string>::const_iterator lit =
		config.local.pluginsPaths.begin();
			lit != config.local.pluginsPaths.end(); lit++) {
		DIR *dir = opendir(lit->c_str());
		struct dirent *ent;

		if (dir == NULL) {
			LOG_WARN("Failed to open plugins directory: '%s'",
				 lit->c_str());
			continue;
		}

		while ((ent = readdir(dir)) != NULL) {
			string plugin_name(ent->d_name);

			if (!endswith(plugin_name, MANIFEST_SUFFIX)) {
				continue;
			}

			LOG_INFO("Catalog: found manifest %s",
					plugin_name.c_str());

			// Remove the MANIFEST_SUFFIX suffix
			plugin_name = plugin_name.substr(0,
						plugin_name.size() -
						string(MANIFEST_SUFFIX).size());

			add_plugin(plugin_name, *lit);
		}

		closedir(dir);
	}
}

void Catalog::add_plugin(const string& plugin_name, const string& plugin_path)
{
	map<string, CatalogPlugin>::iterator plit;
	list<rina::PsInfo> new_policy_sets;
	rina::WriteScopedLock wlock(rwlock);
	int ret;

	if (plugins.count(plugin_name)) {
		// Plugin already in the catalog
		return;
	}

	ret = rina::plugin_get_info(plugin_name, plugin_path, new_policy_sets);
	if (ret) {
		LOG_WARN("Failed to load manifest file for plugin '%s'",
			 plugin_name.c_str());
		return;
	}

	plugins[plugin_name] = CatalogPlugin(plugin_name, plugin_path);
	plit = plugins.find(plugin_name);

	for (list<rina::PsInfo>::const_iterator ps = new_policy_sets.begin();
					ps != new_policy_sets.end(); ps++) {
		map<string, CatalogPsInfo>& cpsets =
					policy_sets[ps->app_entity];
		if (cpsets.count(ps->name)) {
			// Policy set already in the catalog for the
			// specified component;
			continue;
		}

		cpsets[ps->name] = CatalogPsInfo(*ps, plit);
	}
}

// Helper function used by load_by_template()
void Catalog::psinfo_from_psconfig(list< rina::PsInfo >& psinfo_list,
				   const string& component,
				   const rina::PolicyConfig& pconfig)
{
	if (pconfig.name_ != string()) {
		psinfo_list.push_back(rina::PsInfo(pconfig.name_,
						   component,
						   pconfig.version_));
	}
}

int Catalog::load_by_template(Addon *addon, unsigned int ipcp_id,
			      const rinad::DIFTemplate *t)
{
	list< rina::PsInfo > required_policy_sets;

	// Collect into a list all the policy sets specified
	// by the template
	psinfo_from_psconfig(required_policy_sets, "rmt",
			     t->rmtConfiguration.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "pff",
			     t->rmtConfiguration.pft_conf_.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "enrollment-task",
			     t->etConfiguration.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "security-manager",
			     t->secManConfiguration.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "flow-allocator",
			     t->faConfiguration.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "namespace-manager",
			     t->nsmConfiguration.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "resource-allocator",
			     t->raConfiguration.pduftg_conf_.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "routing",
			     t->routingConfiguration.policy_set_);

	// Load all the policy sets in the list
	for (list<rina::PsInfo>::iterator i=required_policy_sets.begin();
			i != required_policy_sets.end(); i++) {
		int ret = load_policy_set(addon, ipcp_id, *i);

		if (ret) {
			LOG_WARN("Failed to load policy-set %s/%s",
				  i->app_entity.c_str(), i->name.c_str());
		}
	}

	return 0;
}

int Catalog::plugin_loaded(const string& plugin_name, unsigned int ipcp_id)
{
	// Lookup again (the plugin or policy set may have disappeared
	// in the meanwhile) under the write lock, and update the
	// catalog data structure.
	rina::WriteScopedLock wlock(rwlock);

	if (plugins.count(plugin_name) == 0) {
		LOG_WARN("Plugin %s disappeared while "
				"loading", plugin_name.c_str());
		return -1;
	}

	// Mark the plugin as loaded for the specified IPCP
	plugins[plugin_name].loaded.insert(ipcp_id);
}

int Catalog::load_policy_set(Addon *addon, unsigned int ipcp_id,
			     const rina::PsInfo& psinfo)
{
	string plugin_name;
	Promise promise;

	{
		// Perform a first lookup (under read lock) to see if we
		// can find a suitable plugin to load
		rina::ReadScopedLock wlock(rwlock);

		if (policy_sets.count(psinfo.app_entity) == 0) {
			LOG_WARN("Catalog does not contain any policy-set "
				 "for component %s",
				 psinfo.app_entity.c_str());
			return -1;
		}

		map<string, CatalogPsInfo>& cmap = policy_sets[psinfo.app_entity];

		if (cmap.count(psinfo.name) == 0) {
			LOG_WARN("Catalog does not contain a policy-set "
				 "called %s for component %s",
				 psinfo.name.c_str(),
				 psinfo.app_entity.c_str());
			return -1;
		}

		if (cmap[psinfo.name].plugin->second.loaded.count(ipcp_id)) {
			// Nothing to do, we have already loaded the
			// plugin for that IPCP.
			return 0;
		}

		plugin_name = cmap[psinfo.name].plugin->second.name;
	}

	// Perform the plugin loading - a blocking and possibly
	// asynchronous operation - out of the catalog lock
	if (IPCManager->plugin_load(addon, &promise, ipcp_id,
			plugin_name, true) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Error occurred while loading plugin '%s'",
			 plugin_name.c_str());
		return -1;
	}

	LOG_INFO("Plugin '%s' successfully loaded",
			plugin_name.c_str());

	return 0;
}

void Catalog::ipcp_destroyed(unsigned int ipcp_id)
{
	rina::WriteScopedLock wlock(rwlock);

	for (std::map<std::string, CatalogPlugin>::iterator
			plit = plugins.begin(); plit != plugins.end();
								plit++) {
		plit->second.loaded.erase(ipcp_id);
	}
}

string Catalog::toString() const
{
	map<string, map< string, CatalogPsInfo > >::const_iterator mit;
	map<string, CatalogPsInfo>::const_iterator cmit;
	rina::ReadScopedLock rlock(const_cast<Catalog *>(this)->rwlock);
	stringstream ss;

	ss << "Catalog of plugins and policy sets:" << endl;

	for (mit = policy_sets.begin(); mit != policy_sets.end(); mit++) {
		for (cmit = mit->second.begin();
				cmit != mit->second.end(); cmit++) {
			const CatalogPsInfo& cps = cmit->second;

			ss << "    ===================================="
			   << endl << "    ps name: " << cps.name
			   << endl << "    ps component: " << cps.app_entity
			   << endl << "    ps version: " << cps.version
			   << endl << "    plugin: " << cps.plugin->second.path
			   << "/" << cps.plugin->second.name
			   << ", loaded for ipcps [ ";
			for (set<unsigned int>::iterator
				ii = cps.plugin->second.loaded.begin();
					ii != cps.plugin->second.loaded.end();
									ii++) {
				ss << *ii << " ";
			}
			ss << "]" << endl;
		}
	}

	ss << "      ====================================" << endl;

	return ss.str();
}

void Catalog::print() const
{
	LOG_INFO("%s", toString().c_str());
}

} // namespace rinad
