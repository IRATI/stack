/*
 * Catalog for policy plugins
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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
	std::map<std::string, CatalogPlugin>::iterator plugin;

	CatalogPsInfo() : PsInfo() { }
	CatalogPsInfo(const rina::PsInfo& psinfo,
		      std::map<std::string, CatalogPlugin>::iterator plit);
};

class Catalog {
public:
	Catalog() { }

	void import();
	void add_plugin(const std::string& plugin_name,
		        const std::string& plugin_path);
	int load_by_template(Addon *addon, unsigned int ipcp_id,
			     const rinad::DIFTemplate *dif_template);

	int load_policy_set(Addon *addon, unsigned int ipcp_id,
			    const rina::PsInfo& psinfo);

	void print() const;
	std::string toString() const;

private:
	void psinfo_from_psconfig(std::list< rina::PsInfo >& psinfo_list,
				  const std::string& component,
				  const rina::PolicyConfig& pconfig);

	std::map<std::string,
		 std::map<std::string, CatalogPsInfo>
		> policy_sets;

	std::map<std::string, CatalogPlugin> plugins;

	rina::ReadWriteLockable rwlock;
};

} // namespace rinad

#endif  /*__CATALOG_H__ */
