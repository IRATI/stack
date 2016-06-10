/*
 * Catalog for policy plugins
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

Catalog::~Catalog()
{
	for (map<string, map<unsigned int, CatalogResource *> >::iterator
			it = resources.begin(); it != resources.end(); it++) {
		for (map<unsigned int, CatalogResource *>::iterator
				rmit = it->second.begin();
					rmit != it->second.end(); rmit++) {
			delete rmit->second;
		}
	}

	for (map<string, map< string, CatalogPsInfo * > >::iterator
			it = policy_sets.begin(); it != policy_sets.end(); it++) {
		for (map<string, CatalogPsInfo *>::iterator
				cmit = it->second.begin();
				cmit != it->second.end(); cmit++) {
			delete cmit->second;
		}
	}

	for (map<string, CatalogPlugin *>::iterator
			it = plugins.begin(); it != plugins.end(); it++) {
		delete it->second;
	}
}

CatalogPsInfo::CatalogPsInfo(const rina::PsInfo& psinfo,
			     CatalogPlugin *pl) : PsInfo(psinfo)
{
	plugin = pl;
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

	plugins[plugin_name] = new CatalogPlugin(plugin_name, plugin_path);

	for (list<rina::PsInfo>::const_iterator ps = new_policy_sets.begin();
					ps != new_policy_sets.end(); ps++) {
		map<string, CatalogPsInfo*>& cpsets =
					policy_sets[ps->app_entity];
		if (cpsets.count(ps->name)) {
			// Policy set already in the catalog for the
			// specified component;
			continue;
		}

		cpsets[ps->name] = new CatalogPsInfo(*ps, plugins[plugin_name]);
	}
}

// Helper function used by load_by_template()
void Catalog::psinfo_from_psconfig(list< rina::PsInfo >& psinfo_list,
				   const string& component,
				   const rina::PolicyConfig& pconfig)
{
	std::string ps_name = pconfig.name_;
	std::string ps_version = pconfig.version_;

	if (ps_name == string()) {
		ps_name = "default";
		ps_version = "";
	}

	psinfo_list.push_back(rina::PsInfo(ps_name, component, ps_version));
}

int Catalog::load(Addon *addon, unsigned int ipcp_id,
			      const rina::DIFConfiguration& t)
{
	list< rina::PsInfo > required_policy_sets;

	// Collect into a list all the policy sets specified
	// by the template
	psinfo_from_psconfig(required_policy_sets, "rmt",
			     t.rmt_configuration_.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "pff",
			     t.rmt_configuration_.pft_conf_.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "enrollment-task",
			     t.et_configuration_.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "security-manager",
			     t.sm_configuration_.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "flow-allocator",
			     t.fa_configuration_.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "namespace-manager",
			     t.nsm_configuration_.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "resource-allocator",
			     t.ra_configuration_.pduftg_conf_.policy_set_);

	psinfo_from_psconfig(required_policy_sets, "routing",
			     t.routing_configuration_.policy_set_);

	for (list<rina::QoSCube*>::const_iterator qit =
			t.efcp_configuration_.qos_cubes_.begin();
			qit != t.efcp_configuration_.qos_cubes_.end(); qit++) {
		psinfo_from_psconfig(required_policy_sets, "dtp",
				     (*qit)->dtp_config_.dtp_policy_set_);
		psinfo_from_psconfig(required_policy_sets, "dtcp",
				     (*qit)->dtcp_config_.dtcp_policy_set_);
	}

	psinfo_from_psconfig(required_policy_sets, "security-manager",
			t.sm_configuration_.default_auth_profile.authPolicy);
	psinfo_from_psconfig(required_policy_sets, "crypto",
			t.sm_configuration_.default_auth_profile.encryptPolicy);
	psinfo_from_psconfig(required_policy_sets, "errc",
			t.sm_configuration_.default_auth_profile.crcPolicy);
	psinfo_from_psconfig(required_policy_sets, "ttl",
			t.sm_configuration_.default_auth_profile.ttlPolicy);

	for (map<std::string, rina::AuthSDUProtectionProfile>::const_iterator
			pit = t.sm_configuration_.specific_auth_profiles.begin();
				pit !=t.sm_configuration_.specific_auth_profiles.end();
					pit++) {
		psinfo_from_psconfig(required_policy_sets, "security-manager",
				 pit->second.authPolicy);
		psinfo_from_psconfig(required_policy_sets, "crypto",
				 pit->second.encryptPolicy);
		psinfo_from_psconfig(required_policy_sets, "errc",
				 pit->second.crcPolicy);
		psinfo_from_psconfig(required_policy_sets, "ttl",
				 pit->second.ttlPolicy);
	}

	// Load all the policy sets in the list
	for (list<rina::PsInfo>::iterator i=required_policy_sets.begin();
			i != required_policy_sets.end(); i++) {
		int ret = load_policy_set(addon, ipcp_id, *i);

		if (ret) {
			LOG_WARN("Failed to load policy-set %s",
				 i->toString().c_str());
		}

                // Record that this IPCP has selected this policy set
                // (TODO move this in the assign_to_dif response).
                // This must not be done for DTCP and DTP policy sets,
                // since they associated to a flow and not to an IPCP.
                if (!ret && i->app_entity != "dtcp"
                         && i->app_entity != "dtp"
                         && i->app_entity != "errc"
                         && i->app_entity != "crypto"
                         && i->app_entity != "ttl") {
			policy_set_selected(*i, ipcp_id);
		}
	}

	return 0;
}

/* To be called under catalog lock. */
CatalogPsInfo *
Catalog::ps_lookup(const rina::PsInfo& psinfo)
{
	if (policy_sets.count(psinfo.app_entity) == 0) {
		LOG_WARN("Catalog does not contain any policy-set "
				"for component %s",
				psinfo.app_entity.c_str());
		return NULL;
	}

	map<string, CatalogPsInfo*>& cmap = policy_sets[psinfo.app_entity];

	if (cmap.count(psinfo.name) == 0) {
		LOG_WARN("Catalog does not contain a policy-set "
				"called %s for component %s",
				psinfo.name.c_str(),
				psinfo.app_entity.c_str());
		return NULL;
	}

	return cmap[psinfo.name];
}

int Catalog::plugin_loaded(const string& plugin_name, unsigned int ipcp_id,
			   bool load)
{
	// Lookup again (the plugin or policy set may have disappeared
	// in the meanwhile) under the write lock, and update the
	// catalog data structure.
	rina::WriteScopedLock wlock(rwlock);

	if (plugins.count(plugin_name) == 0) {
		LOG_WARN("Plugin %s it's not in the catalog",
			 plugin_name.c_str());
		return -1;
	}

	// Mark the plugin as (un)loaded for the specified IPCP
	if (load) {
		plugins[plugin_name]->loaded.insert(ipcp_id);
	} else {
		plugins[plugin_name]->loaded.erase(ipcp_id);
	}
}

CatalogResource *
Catalog::rsrc_lookup(const std::string& component, unsigned int id)
{
	map<string, map<unsigned int, CatalogResource *> >::iterator mit;
	map<unsigned int, CatalogResource *>::iterator rmit;

	mit = resources.find(component);
	if (mit == resources.end()) {
		return NULL;
	}

	rmit = mit->second.find(id);
	if (rmit == mit->second.end()) {
		return NULL;
	}

	return rmit->second;
}

int
Catalog::add_resource(const std::string& component, unsigned int id,
		      CatalogPsInfo *cpsinfo)
{
	if (resources.count(component) == 0) {
		resources[component] = map<unsigned int, CatalogResource *>();
	}

	if (resources[component].count(id)) {
		LOG_ERR("Resource with id %u for component %s "
			"already exists", id, component.c_str());
	}

	resources[component][id] = new CatalogResource(id, cpsinfo);

	return 0;
}

int Catalog::policy_set_selected(const rina::PsInfo& ps_info,
			         unsigned int id)
{
	rina::WriteScopedLock wlock(rwlock);
	CatalogPsInfo *cpsinfo;
	CatalogResource *rsrc;

	cpsinfo = ps_lookup(ps_info);
	if (!cpsinfo) {
		return -1;
	}

	rsrc = rsrc_lookup(ps_info.app_entity, id);
	if (rsrc) {
		rsrc->ps->selected.erase(id);
		rsrc->ps = cpsinfo;
	} else {
		add_resource(ps_info.app_entity, id, cpsinfo);
	}

	cpsinfo->selected.insert(id);

	return 0;
}

int Catalog::load_policy_set(Addon *addon, unsigned int ipcp_id,
			     const rina::PsInfo& ps_info)
{
	string plugin_name;
	Promise promise;

	{
		// Perform a first lookup (under read lock) to see if we
		// can find a suitable plugin to load
		rina::ReadScopedLock wlock(rwlock);
		CatalogPsInfo *cpsinfo;

		cpsinfo = ps_lookup(ps_info);
		if (!cpsinfo) {
			return -1;
		}

		if (cpsinfo->plugin->loaded.count(ipcp_id)) {
			// Nothing to do, we have already loaded the
			// plugin for that IPCP.
			return 0;
		}

		plugin_name = cpsinfo->plugin->name;
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

	for (std::map<std::string, CatalogPlugin *>::iterator
			plit = plugins.begin(); plit != plugins.end();
								plit++) {
		plit->second->loaded.erase(ipcp_id);
	}
}

string Catalog::toString(const CatalogPsInfo *cps) const
{
	stringstream ss;

	ss << "    ===================================="
	   << endl << "    ps name: " << cps->name
	   << endl << "    ps component: " << cps->app_entity
	   << endl << "    ps version: " << cps->version
	   << endl << "    plugin: " << cps->plugin->path
	   << "/" << cps->plugin->name
	   << endl << "    loaded for ipcps [ ";
	for (set<unsigned int>::iterator
		ii = cps->plugin->loaded.begin();
			ii != cps->plugin->loaded.end(); ii++) {
		ss << *ii << " ";
	}
	ss << "]" << endl << "    selected for resources [ ";
	for (set<unsigned int>::iterator
		ii = cps->selected.begin();
			ii != cps->selected.end(); ii++) {
		ss << *ii << " ";
	}
	ss << "]" << endl;

	return ss.str();
}

string Catalog::toString() const
{
	map<string, map< string, CatalogPsInfo * > >::const_iterator mit;
	map<string, CatalogPsInfo *>::const_iterator cmit;
	rina::ReadScopedLock rlock(const_cast<Catalog *>(this)->rwlock);
	stringstream ss;

	ss << "Catalog of plugins and policy sets:" << endl;

	for (mit = policy_sets.begin(); mit != policy_sets.end(); mit++) {
		for (cmit = mit->second.begin();
				cmit != mit->second.end(); cmit++) {
			const CatalogPsInfo *cps = cmit->second;

			ss << toString(cps);
		}
	}

	ss << "      ====================================" << endl;

	return ss.str();
}

void Catalog::print() const
{
	LOG_INFO("%s", toString().c_str());
}

string Catalog::toString(const string& component) const
{
	rina::ReadScopedLock rlock(const_cast<Catalog *>(this)->rwlock);
	map<string, map< string, CatalogPsInfo * > >::const_iterator mit;
	map<string, CatalogPsInfo *>::const_iterator cmit;
	stringstream ss;

	mit = policy_sets.find(component);
	if (mit == policy_sets.end()) {
		return string();
	}

	for (cmit = mit->second.begin(); cmit != mit->second.end(); cmit++) {
		const CatalogPsInfo *cps = cmit->second;

		ss << toString(cps);
	}

	ss << "      ====================================" << endl;

	return ss.str();
}

} // namespace rinad
