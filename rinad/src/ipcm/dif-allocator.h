/*
 * DIF Allocator (resolves application names to DIFs)
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef __DIF_ALLOCATOR_H__
#define __DIF_ALLOCATOR_H__

#include <map>

#include <librina/concurrency.h>

#include "rina-configuration.h"

namespace rinad {

class DIFAllocator;

/// Monitors the file with the application to DIF mappings
class DIFDirectoryMonitor: public rina::SimpleThread {
public:
	DIFDirectoryMonitor(rina::ThreadAttributes * thread_attrs,
			   const std::string& file,
			   DIFAllocator * da);
	~DIFDirectoryMonitor() throw();
	void do_stop();
	int run();

private:
	bool has_to_stop();
	void process_events(int fd);

	DIFAllocator * dif_allocator;
	std::string folder;
	bool stop;
	rina::Lockable lock;
};

class DIFAllocator {
public:
	static const std::string DIF_DIRECTORY_FILE_NAME;

	DIFAllocator(const std::string& folder);
	virtual ~DIFAllocator(void);
        bool lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
        			       rina::ApplicationProcessNamingInformation& result);
        void update_directory_contents();

private:
        std::string folder_name;
        std::string fq_file_name;

	//The current DIF Directory
	std::map<std::string, rina::ApplicationProcessNamingInformation> dif_directory;

	rina::ReadWriteLockable directory_lock;
	DIFDirectoryMonitor * directory_monitor;
};

} //namespace rinad

#endif  /* __DIF_TEMPLATE_MONITOR_H__ */
