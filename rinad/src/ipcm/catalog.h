/*
 * Catalog for policy plugins
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

#ifndef __CATALOG_H__
#define __CATALOG_H__

#include <string>
#include <list>
#include <map>
#include <set>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/patterns.h>

#include "addon.h"
#include "dif-template-manager.h"


namespace rinad {

// Data structure that represent a plugin in the catalog
struct CatalogPlugin {
	// The name of the plugin
	std::string name;

	// The path of the plugin
	std::string path;

	// The IPCPs for which the plugin is already loaded ?
	std::set<unsigned int> loaded;

	CatalogPlugin() { }
	CatalogPlugin(const std::string& n, const std::string& p)
			: name(n), path(p) { }
};

struct CatalogPsInfo: public rina::PsInfo {
	// Back-reference to the plugin that published this
	// policy-set
	CatalogPlugin *plugin;

	// The resources (IPCP or port-ids of flows) for which this policy set is
	// selected
	std::set<unsigned int> selected;

	CatalogPsInfo() : PsInfo() { }
	CatalogPsInfo(const rina::PsInfo& psinfo,
		      CatalogPlugin *pl);
};

struct CatalogResource {
	// Back-reference to the policy set entry that
	// is currently selected for this resource
	CatalogPsInfo *ps;

	// Id of the resource (port-id or IPCP id)
	unsigned int id;

	CatalogResource() { }
	CatalogResource(unsigned int _id, CatalogPsInfo *_ps)
				: id(_id), ps(_ps) { }
};

class Catalog {
public:
	Catalog() { }
	~Catalog();

	void import();
	void add_plugin(const std::string& plugin_name,
		        const std::string& plugin_path);
	int load(Addon *addon, unsigned int ipcp_id,
			const rina::DIFConfiguration& t);

	int load_policy_set(Addon *addon, unsigned int ipcp_id,
			const rina::PsInfo& psinfo);

	int plugin_loaded(const std::string& plugin_name,
			 unsigned int ipcp_id, bool load);

	int policy_set_selected(const rina::PsInfo& ps_info,
			unsigned int id);

	void ipcp_destroyed(unsigned int ipcp_id);

	void print() const;
	std::string toString() const;
	std::string toString(const std::string& component) const;

private:
	void psinfo_from_psconfig(std::list< rina::PsInfo >& psinfo_list,
				  const std::string& component,
				  const rina::PolicyConfig& pconfig);

	CatalogPsInfo *ps_lookup(const rina::PsInfo& psinfo);

	CatalogResource *rsrc_lookup(const std::string& component,
				     unsigned int id);

	int add_resource(const std::string& component,
			 unsigned int id, CatalogPsInfo *);

	std::string toString(const CatalogPsInfo *cps) const;

	std::map<std::string,
		 std::map<std::string, CatalogPsInfo*>
		> policy_sets;

	std::map<std::string, CatalogPlugin*> plugins;

	std::map<std::string,
		 std::map<unsigned int, CatalogResource*>
		> resources;

	rina::ReadWriteLockable rwlock;
};

} // namespace rinad

#endif  /*__CATALOG_H__ */
