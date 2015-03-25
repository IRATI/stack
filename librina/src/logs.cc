//
// Logging facilities
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//    Marc Sune             <marc.sune (at) bisdn.de>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define RINA_PREFIX "logs"

#include "librina/likely.h"
#include "librina/logs.h"

LOG_LEVEL logLevel = DBG;
FILE* logStream = stdout;

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void setLogLevel(const std::string& newLogLevel)
{
	LOG_DBG("New log level: %s", newLogLevel.c_str());

	if (LOG_LEVEL_DBG.compare(newLogLevel) == 0) {
		logLevel = DBG;
	} else if (LOG_LEVEL_INFO.compare(newLogLevel) == 0) {
		logLevel = INFO;
	} else if (LOG_LEVEL_NOTE.compare(newLogLevel) == 0) {
		logLevel = NOTE;
	} else if (LOG_LEVEL_WARN.compare(newLogLevel) == 0) {
		logLevel = WARN;
	} else if (LOG_LEVEL_ERR.compare(newLogLevel) == 0) {
		logLevel = ERR;
	} else if (LOG_LEVEL_CRIT.compare(newLogLevel) == 0) {
		logLevel = CRIT;
	} else if (LOG_LEVEL_ALERT.compare(newLogLevel) == 0) {
		logLevel = ALERT;
	} else if (LOG_LEVEL_EMERG.compare(newLogLevel) == 0) {
		logLevel = EMERG;
	}
}

int setLogFile(const std::string& pathToFile)
{
	int result = 0;

	if (pathToFile.compare("") == 0) {
		return result;
	}

	pthread_mutex_lock(&log_mutex);

	if (logStream != stdout) {
		result = -1;
	} else {
		logStream = fopen(pathToFile.c_str(), "w");
		if (!logStream) {
			logStream = stdout;
			result = -1;
		}
	}

	pthread_mutex_unlock(&log_mutex);

	return result;
}

//Process Id
static int processId = -1;

static inline int getProcessId()
{
	if(unlikely(processId == -1)){
		processId = getpid();
	}

	return processId;
}

void log(LOG_LEVEL level, const char * fmt, ...)
{
	//Avoid to use locking
	FILE* stream = logStream;

	if(level > logLevel)
		return;

	va_list args;
	time_t now = time(0);

	fprintf(stream, "%d(%ld)", getProcessId(), now);

	va_start(args, fmt);
	vfprintf(stream, fmt, args);
	va_end(args);

	fflush(stream);
}
