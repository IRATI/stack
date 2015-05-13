/*
 * DIF template monitor (monitors DIF templates)
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Marc Sune             <marc.sune (at) bisdn.de>
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

#ifndef __DIF_TEMPLATE_MONITOR_H__
#define __DIF_TEMPLATE_MONITOR_H__

#include <map>

#include <librina/concurrency.h>

#include "rina-configuration.h"

namespace rinad {

/// Monitors the folder of the DIF templates
class DIFTemplateMonitor: public rina::SimpleThread {
public:
	DIFTemplateMonitor(rina::ThreadAttributes * thread_attrs,
			   const std::string& folder);
	~DIFTemplateMonitor() throw();
	void do_stop();
	int run();

private:
	bool has_to_stop();
	void process_events(int fd);

	std::string folder_name;
	bool stop;
	rina::Lockable lock;
};

class DIFTemplateManager {
public:
	DIFTemplateManager(const std::string& folder);
	virtual ~DIFTemplateManager(void);
	rinad::DIFProperties * get_dif_template(const std::string& name);
	void add_dif_template(const std::string& name, rinad::DIFProperties * dif_template);
	void remove_dif_template(const std::string& name);

private:
	void internal_remove_dif_template(const std::string& name);

	//The folder containing the DIF templates to monitor
	std::string folder_name;

	//The current templates
	std::map<std::string, rinad::DIFProperties *> dif_templates;

	rina::ReadWriteLockable templates_lock;
	DIFTemplateMonitor * template_monitor;
};

} //namespace rinad

#endif  /* __DIF_TEMPLATE_MONITOR_H__ */
