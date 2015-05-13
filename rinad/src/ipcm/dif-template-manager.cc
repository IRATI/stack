/*
 * DIF Template Manager
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

#include <errno.h>
#include <poll.h>
#include <sys/inotify.h>

#define RINA_PREFIX     "ipcm.dif-template-manager"
#include <librina/logs.h>

#include "dif-template-manager.h"

using namespace std;


namespace rinad {

//Class DIF Template Monitor
DIFTemplateMonitor::DIFTemplateMonitor(rina::ThreadAttributes * thread_attrs,
		   	   	       const std::string& folder) :
		   	   			       rina::SimpleThread(thread_attrs)
{
	folder_name = folder;
	stop = false;
}

DIFTemplateMonitor::~DIFTemplateMonitor() throw()
{
}

void DIFTemplateMonitor::do_stop()
{
	rina::ScopedLock g(lock);
	stop = true;
}

bool DIFTemplateMonitor::has_to_stop()
{
	rina::ScopedLock g(lock);
	return stop;
}

void DIFTemplateMonitor::process_events(int fd)
{
        char buf[4096]
            __attribute__ ((aligned(__alignof__(struct inotify_event))));
        const struct inotify_event *event;
        int i;
        ssize_t len;
        char *ptr;

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
                	std::string file_name = std::string(event->name);

                	//ignore *.swp and *.swx
                	if (file_name.find(".swx") != std::string::npos) {
                		continue;
                	}

                	if (file_name.find(".swp") != std::string::npos) {
                		continue;
                	}

                	if (file_name.find("~") != std::string::npos) {
                		continue;
                	}

                	//TODO
                	if (event->mask & IN_CLOSE_WRITE)
                		LOG_DBG("IN_CLOSE_WRITE: ");
                	if (event->mask & IN_DELETE)
                		LOG_DBG("IN_DELETE: ");

                	/* Print the name of the file */

                	if (event->len)
                		LOG_DBG("%s", event->name);
                }
        }
}

int DIFTemplateMonitor::run()
{
	int fd;
	int wd;
	int pollnum;
	struct pollfd fds[1];

	LOG_DBG("DIF Template monitor started, monitoring folder %s",
			folder_name.c_str());

	fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) {
		LOG_ERR("Error initializing inotify, stopping DIF template monitor");
		return -1;
	}

	wd = inotify_add_watch(fd, folder_name.c_str(), IN_CLOSE_WRITE | IN_DELETE);
	if (wd == -1) {
		LOG_ERR("Error adding a watch, stopping DIF template monitor");
		close(fd);
		return -1;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (!has_to_stop()) {
		pollnum = poll(fds, 1, 1000);

		if (pollnum == EINVAL) {
			LOG_ERR("Poll returned EINVAL, stopping DIF template monitor");
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

        LOG_DBG("DIF Template Manager stopped");

	return 0;
}

//Class DIF Template Manager
DIFTemplateManager::DIFTemplateManager(const std::string& folder)
{
	folder_name = folder;

	//TODO load current templates from template folder

	//Create a thread that monitors the DIF template folder when required
	rina::ThreadAttributes thread_attrs;
	thread_attrs.setJoinable();
	template_monitor = new DIFTemplateMonitor(&thread_attrs, folder);
}

DIFTemplateManager::~DIFTemplateManager()
{
	void * status;

	if (template_monitor) {
		template_monitor->do_stop();
		template_monitor->join(&status);

		delete template_monitor;
	}
}

rinad::DIFProperties * DIFTemplateManager::get_dif_template(const std::string& name)
{
	rina::ReadScopedLock g(templates_lock);

	std::map<std::string, rinad::DIFProperties*>::iterator it = dif_templates.find(name);
	if (it == dif_templates.end()) {
		return 0;
	} else {
		return it->second;
	}
}

void DIFTemplateManager::add_dif_template(const std::string& name,
					  rinad::DIFProperties * dif_template)
{
	if (!dif_template) {
		LOG_ERR("Cannot add a bogus dif_template");
		return;
	}

	rina::WriteScopedLock g(templates_lock);

	//If the template already exists destroy the old version
	internal_remove_dif_template(name);
	dif_templates[name] = dif_template;

	LOG_INFO("Added DIF template called: %s", name.c_str());
}

void DIFTemplateManager::remove_dif_template(const std::string& name)
{
	rina::WriteScopedLock g(templates_lock);
	internal_remove_dif_template(name);
}

void DIFTemplateManager::internal_remove_dif_template(const std::string& name)
{
	std::map<std::string, rinad::DIFProperties*>::iterator it = dif_templates.find(name);
	if (it != dif_templates.end()) {
		dif_templates.erase(name);
		delete it->second;
		LOG_INFO("Removed DIF template called: %s", name.c_str());
	}
}

} //namespace rinad
