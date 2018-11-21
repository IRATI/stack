/*
 * UNIX console
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#ifndef __UNIX_CONSOLE_H__
#define __UNIX_CONSOLE_H__

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sstream>

#include "concurrency.h"
#include "common.h"

namespace rina {

class UNIXConsole;

class ConsoleCmdInfo {
public:
	const char *usage;
	UNIXConsole * console;

	ConsoleCmdInfo() : usage(NULL), console(NULL) { }
	virtual ~ConsoleCmdInfo(){};
	ConsoleCmdInfo(const char *u, UNIXConsole * con)
	: usage(u), console(con) { }

	virtual int execute(std::vector<std::string>& args);
};

class UNIXConsole {
	struct Connection;

	static const unsigned int CMDBUFSIZE = 120;

	rina::Thread *worker;
	std::string socket_path;
	std::string prompt;

	int init(void);
	int process_command(Connection *conn, char *cmdbuf, int size);
	int plugin_load_unload(std::vector<std::string>& args,
			bool load);
	bool cleanup_filesystem_socket();

public:
	UNIXConsole(const std::string& socket_path,
		    const std::string& prompt);
	void body();
	virtual ~UNIXConsole() throw();
	bool keep_on_running;

	static const int CMDRETCONT = 0;
	static const int CMDRETSTOP = 1;
	std::ostringstream outstream;
	std::map<std::string, ConsoleCmdInfo *> commands_map;
};

}
#endif  /* __IPCM_CONSOLE_H__ */
