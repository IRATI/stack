/*
 * DIF template monitor (monitors DIF templates)
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Marc Sune             <marc.sune (at) bisdn.de>
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

#ifndef __DIF_TEMPLATE_MONITOR_H__
#define __DIF_TEMPLATE_MONITOR_H__

#include <map>

#include <librina/concurrency.h>

#include "rina-configuration.h"

namespace rinad {

class DIFTemplateManager;
class DIFAllocator;

/// Monitors the folder of the DIF templates
class DIFConfigFolderMonitor: public rina::SimpleThread {
public:
	DIFConfigFolderMonitor(const std::string& folder,
			       DIFTemplateManager * dtm,
			       DIFAllocator * da);
	~DIFConfigFolderMonitor() throw();
	void do_stop();
	int run();

private:
	bool has_to_stop();
	void process_events(int fd);

	DIFTemplateManager * dif_template_manager;
	DIFAllocator * dif_allocator;
	std::string folder_name;
	bool stop;
	rina::Lockable lock;
};

class DIFTemplateManager {
public:
	static const std::string DEFAULT_TEMPLATE_NAME;

	DIFTemplateManager(const std::string& folder,
			   DIFAllocator * dif_allocator);
	virtual ~DIFTemplateManager(void);
	int get_dif_template(const std::string& name, rinad::DIFTemplate& dif_template);
	void add_dif_template(const std::string& name, rinad::DIFTemplate * dif_template);
	void remove_dif_template(const std::string& name);
	void get_all_dif_templates(std::list<rinad::DIFTemplate>& dif_templates);

private:
	int load_initial_dif_templates();
	void internal_remove_dif_template(const std::string& name);
	void augment_dif_template(rinad::DIFTemplate * dif_template);

	//The folder containing the DIF templates to monitor
	std::string folder_name;

	//The current templates
	std::map<std::string, rinad::DIFTemplate *> dif_templates;

	rina::ReadWriteLockable templates_lock;
	DIFConfigFolderMonitor * monitor;
	rinad::DIFTemplate * default_template;
};

} //namespace rinad

#endif  /* __DIF_TEMPLATE_MONITOR_H__ */
