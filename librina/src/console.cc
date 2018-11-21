/*
 * UNIX console
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Marc Sune             <marc.sune (at) bisdn.de>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#include <sys/time.h>
#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sstream>

#define RINA_PREFIX "unix.console"
#include <librina/common.h>
#include <librina/console.h>
#include <librina/logs.h>
#include <librina/timer.h>

using namespace std;

namespace rina {

int ConsoleCmdInfo::execute(std::vector<std::string>& args) {
	return UNIXConsole::CMDRETCONT;
}

class HelpConsoleCmd: public ConsoleCmdInfo {
public:
	HelpConsoleCmd(UNIXConsole * console) :
		ConsoleCmdInfo("USAGE: help [<command>]", console) {};

	int execute(std::vector<std::string>& args) {
		if (args.size() < 2) {
			console->outstream << "Available commands:" << endl;
			for (map<string, ConsoleCmdInfo *>::iterator mit =
					console->commands_map.begin();
						mit != console->commands_map.end(); mit++) {
				console->outstream << "    " << mit->first << endl;
			}
		} else {
			map<string, ConsoleCmdInfo *>::iterator mit =
					console->commands_map.find(args[1]);

			if (mit == console->commands_map.end()) {
				console->outstream << "Unknown command '" << args[1]
						<< "'" << endl;
			} else {
				console->outstream << mit->second->usage << endl;
			}
		}

		return UNIXConsole::CMDRETCONT;
	}
};

class QuitConsoleCmd: public ConsoleCmdInfo {
public:
	QuitConsoleCmd(UNIXConsole * console) :
		ConsoleCmdInfo("USAGE: quit or exit", console) {};

	int execute(std::vector<std::string>& args) {
		return UNIXConsole::CMDRETSTOP;
	}
};

void *
console_function(void *opaque)
{
	UNIXConsole *console = static_cast<UNIXConsole *>(opaque);

	console->body();

	return NULL;
}

UNIXConsole::UNIXConsole(const std::string& socket_path_,
			 const std::string& prompt_) :
		socket_path(socket_path_), prompt(prompt_)
{
	commands_map["help"] = new HelpConsoleCmd(this);
	commands_map["quit"] = new QuitConsoleCmd(this);
	commands_map["exit"] = new QuitConsoleCmd(this);

	keep_on_running = true;
	worker = new rina::Thread(console_function, this,
			std::string("unix-console"), false);
	worker->start();
}

UNIXConsole::~UNIXConsole() throw()
{
	void *status;
	std::map<std::string, ConsoleCmdInfo *>::iterator it;

	this->keep_on_running = false;
	worker->join(&status);
	if (socket_path[0] != '@') {
		cleanup_filesystem_socket();
	}
	delete worker;

	for(it = commands_map.begin(); it != commands_map.end(); ++it) {
		delete it->second;
		it->second = NULL;
	}
	commands_map.clear();
}

bool
UNIXConsole::cleanup_filesystem_socket()
{
	struct stat stat;
	const char *path = socket_path.c_str();
	// Ignore if it's not a socket to avoid data loss;
	// bind() will fail and we'll output a log there.
	return lstat(path, &stat) || !S_ISSOCK(stat.st_mode) || !unlink(path);
}

int
UNIXConsole::init()
{
	struct sockaddr_un sa_un;
	char fill[108] = "";
	memcpy(sa_un.sun_path, fill, 108);
	int sfd = -1;
	unsigned len = socket_path.size();

	if (len >= sizeof sa_un.sun_path) {
		LOG_ERR("AF_UNIX path too long");
		return -1;
	}

	socket_path.copy(sa_un.sun_path, len);
	if (socket_path[0] == '@') {
		sa_un.sun_path[0] = 0; // abstract unix socket
	} else if (!cleanup_filesystem_socket()) {
		LOG_ERR("Error [%i] unlinking %s", errno, socket_path.c_str());
		return -1;
	}
	len += offsetof(struct sockaddr_un, sun_path);

	sa_un.sun_family = AF_UNIX;
	try {
		sfd = socket(sa_un.sun_family, SOCK_STREAM, 0);
		if (sfd < 0) {
			throw rina::Exception("socket");
		}
		if (bind(sfd, reinterpret_cast<struct sockaddr *>(&sa_un),
				len)) {
			throw rina::Exception("bind");
		}
		if (listen(sfd, 5)) {
			throw rina::Exception("listen");
		}
	} catch (rina::Exception& e) {
		LOG_ERR("Error [%i] calling %s()", errno, e.what());
		if (sfd >= 0) {
			close(sfd);
			sfd = -1;
		}
	}

	return sfd;
}

struct UNIXConsole::Connection {
	char *cmdbuf;
	unsigned tail, size;
	int fd;
	std::string prompt_str;

	Connection(const std::string prompt_) :
		cmdbuf(0), tail(0), size(0), fd(-1) {
		std::stringstream ss;

		ss << prompt_ << " >>> ";
		prompt_str = ss.str();
	}

	~Connection() {
		free(cmdbuf);
		if (fd >= 0)
			close(fd);
	}

	bool write(const string &str) {
		if (::write(fd, str.data(), str.size()) >= 0) {
			return true;
		}
		if (errno == EPIPE) {
			LOG_ERR("Console client disconnected\n");
		} else {
			LOG_ERR("Error [%i] calling write()\n", errno);
		}
		return false;
	}

	bool prompt() {
		return write(prompt_str);
	}

	bool accept(int sfd) {
		fd = ::accept(sfd, NULL, NULL);
		if (fd < 0) {
			LOG_ERR("Error [%i] calling accept()\n", errno);
			return false;
		}
		return prompt();
	}

	bool readable(UNIXConsole *console) {
		char *p, *q;
		int n = size - tail;
		if (n < 128) {
			size = tail + (n = 128);
			p = (char*) realloc(cmdbuf, size);
			if (!p) {
				return false;
			}
			cmdbuf = p;
		}
		n = read(fd, p = cmdbuf + tail, n);
		if (n <= 0) {
			if (n < 0) {
				LOG_ERR("Error [%i] calling read()\n", errno);
			}
			return false;
		}
		tail += n;
		if (!(q = (char*) memchr(p, '\n', n))) {
			return true;
		}
		p = cmdbuf;
		do {
			int cmdret = console->process_command(this, p, q - p);
			if (cmdret == console->CMDRETSTOP) {
				return false;
			}
			p = q + 1;
			n = cmdbuf + tail - p;
		} while ((q = (char*) memchr(p, '\n', n)));
		memmove(cmdbuf, p, tail = n);
		return prompt();
	}
};

void UNIXConsole::body()
{
	list<Connection> conns;
	list<Connection>::iterator it;
	int sfd = init();

	if (sfd < 0) {
		return;
	}

	LOG_DBG("Console starts [fd=%d]\n", sfd);

	while (this->keep_on_running) {
		struct timeval timeout = {2,0};
		fd_set readset;
		int maxfd = sfd;
		FD_ZERO(&readset);
		FD_SET(sfd, &readset);
		for (it = conns.begin(); it != conns.end(); it++) {
			FD_SET(it->fd, &readset);
			if (maxfd < it->fd) {
				maxfd = it->fd;
			}
		}

		int res = select(maxfd+1, &readset, NULL, NULL, &timeout);
		if (res <= 0) {
			if (!res || errno == EINTR) {
				continue;
			}
			LOG_ERR("Error [%i] calling select()\n", errno);
			break;
		}

		for (it = conns.begin(); it != conns.end(); ) {
			if (FD_ISSET(it->fd, &readset) && !it->readable(this)) {
				it = conns.erase(it);
			} else {
				it++;
			}
		}

		if (FD_ISSET(sfd, &readset)) {
			conns.push_back(Connection(prompt));
			if (!conns.back().accept(sfd)) {
				conns.pop_back();
			}
		}
	}

	close(sfd);
	LOG_DBG("Console stops\n");
}

int
UNIXConsole::process_command(Connection *conn, char *cmdbuf, int size)
{
	map<string, ConsoleCmdInfo *>::iterator mit;
	istringstream iss(string(cmdbuf, size));
	vector<string> args;
	string token;
	int ret;

	while (iss >> token) {
		args.push_back(token);
	}

	if (args.size() < 1) {
		return 0;
	}

	mit = commands_map.find(args[0]);
	if (mit == commands_map.end()) {
		outstream << "Unknown command '" << args[0] << "'";
		ret = 0;
	} else {
		ret = mit->second->execute(args);
	}

	outstream << endl;
	if (!conn->write(outstream.str())) {
		ret = CMDRETSTOP;
	}

	// Make the stringstream empty
	outstream.str(string());
	return ret;
}

}//namespace rina
