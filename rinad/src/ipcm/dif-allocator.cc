/*
 * DIF Allocator
 *
 *    Eduard Grasa     <eduard.grasa@i2cat.net>
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

//Class DIFDirectoryMonitor
DIFDirectoryMonitor::DIFDirectoryMonitor(rina::ThreadAttributes * thread_attrs,
		   	   	         const std::string& folder_,
		   	   	         DIFAllocator * da_) :
		   	   			       rina::SimpleThread(thread_attrs)
{
	std::stringstream ss;
	folder = folder_;
	stop = false;
	dif_allocator = da_;
}

DIFDirectoryMonitor::~DIFDirectoryMonitor() throw()
{
}

void DIFDirectoryMonitor::do_stop()
{
	rina::ScopedLock g(lock);
	stop = true;
}

bool DIFDirectoryMonitor::has_to_stop()
{
	rina::ScopedLock g(lock);
	return stop;
}

static bool str_ends_with(const std::string& str, const std::string& suffix)
{
	if (suffix.size() > str.size()) {
		return false;
	}

	return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

void DIFDirectoryMonitor::process_events(int fd)
{
        char buf[4096]
            __attribute__ ((aligned(__alignof__(struct inotify_event))));
        const struct inotify_event *event;
        int i;
        ssize_t len;
        char *ptr;
        std::stringstream ss;
        DIFTemplate * dif_template;

        for (;;) {
                len = read(fd, buf, sizeof buf);
                if (len == -1 && errno != EAGAIN) {
                    LOG_ERR("Problems reading inotify file descriptor");
                    return;
                }

                if (len <= 0)
                    return;

                for (ptr = buf; ptr < buf + len;
                		ptr += sizeof(struct inotify_event) + event->len) {
                	event = (const struct inotify_event *) ptr;
                	std::string name = std::string(event->name);

			if (! str_ends_with(name, DIFAllocator::DIF_DIRECTORY_FILE_NAME)) {
				continue;
			}

                	dif_allocator->update_directory_contents();
                }
        }
}

int DIFDirectoryMonitor::run()
{
	int fd;
	int wd;
	int pollnum;
	struct pollfd fds[1];

	LOG_DBG("DIF Directory monitor started, monitoring folder %s",
			folder.c_str());

	fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) {
		LOG_ERR("Error initializing inotify, stopping DIF Directory monitor");
		return -1;
	}

	wd = inotify_add_watch(fd, folder.c_str(), IN_CLOSE_WRITE | IN_DELETE);
	if (wd == -1) {
		LOG_ERR("Error adding a watch, stopping DIF Directory monitor");
		close(fd);
		return -1;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (!has_to_stop()) {
		pollnum = poll(fds, 1, 1000);

		if (pollnum == EINVAL) {
			LOG_ERR("Poll returned EINVAL, stopping DIF Directory monitor");
			return -1;
		}

		if (pollnum <= 0) {
			//No changes during this period or error that can be ignored
			continue;
		}

		if (fds[0].revents & POLLIN) {
			process_events(fd);
		}
	}

        /* Close inotify file descriptor */
        close(fd);

        LOG_DBG("DIF Directory Monitor stopped");

	return 0;
}

//Class DIF Template Manager
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

	//load current templates from template folder
	if (!parse_app_to_dif_mappings(fq_file_name, dif_directory)) {
		LOG_ERR("Problems loading initial directory");
	}

	//Create a thread that monitors the DIF template folder when required
	rina::ThreadAttributes thread_attrs;
	thread_attrs.setJoinable();
	directory_monitor = new DIFDirectoryMonitor(&thread_attrs, folder_name, this);
	directory_monitor->start();
}

DIFAllocator::~DIFAllocator()
{
	void * status;

	if (directory_monitor) {
		directory_monitor->do_stop();
		directory_monitor->join(&status);

		delete directory_monitor;
	}

	dif_directory.clear();
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
	parse_app_to_dif_mappings(fq_file_name, dif_directory);
	LOG_DBG("DIF Allocator Directory updated!");
}

} //namespace rinad
