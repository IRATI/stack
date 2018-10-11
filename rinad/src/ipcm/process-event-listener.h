/*
 * Listens for process events, to detect
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef __PROCESS_EVENT_LISTENER_H__
#define __PROCESS_EVENT_LISTENER_H__

#include <librina/concurrency.h>

namespace rinad {

/// Monitors events on OS processes
class OSProcessMonitor: public rina::SimpleThread {
public:
	OSProcessMonitor();
	~OSProcessMonitor() throw();
	void do_stop();
	int run();

private:
	bool has_to_stop();
	int process_events(void);
	int nl_connect(void);
	int set_proc_ev_listen(int nl_sock, bool enable);

	bool stop;
	rina::Lockable lock;
	int nl_sock;
};

} //namespace rinad

#endif  /* __PROCESS_EVENT_LISTENER_H__ */
