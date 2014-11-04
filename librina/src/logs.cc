//
// Logging facilities
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#include "librina/logs.h"

LOG_LEVEL        logLevel         = DBG;
FILE *           logOutputStream  = stdout;
pthread_rwlock_t outputStreamLock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t logLevelLock     = PTHREAD_RWLOCK_INITIALIZER;

void setLogLevel(const std::string& newLogLevel)
{
        LOG_DBG("New log level: %s", newLogLevel.c_str());

	pthread_rwlock_wrlock(&logLevelLock);

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

	pthread_rwlock_unlock(&logLevelLock);
}

int setLogFile(const std::string& pathToFile)
{
	int result = 0;

	if (pathToFile.compare("") == 0) {
		return result;
	}

	pthread_rwlock_wrlock(&outputStreamLock);
	if (logOutputStream != stdout) {
	        result = -1;
	} else {
	        logOutputStream = fopen(pathToFile.c_str(), "w");
	        if (!logOutputStream) {
	                logOutputStream = stdout;
	                result = -1;
	        }
	}
	pthread_rwlock_unlock(&outputStreamLock);

	return result;
}

int processId = -1;

int getProcessId()
{
	if (processId == -1){
		processId = getpid();
	}

	return processId;
}

static bool shouldLog(LOG_LEVEL level)
{
	switch (level) {
	case EMERG:
		return true;
	case ALERT:
		if (logLevel == EMERG) {
			return false;
		} else {
			return true;
		}
	case CRIT:
		if (logLevel == EMERG || logLevel == ALERT) {
			return false;
		} else {
			return true;
		}
	case ERR:
		if (logLevel == EMERG ||
                    logLevel == ALERT ||
                    logLevel == CRIT) {
			return false;
		} else {
			return true;
		}
	case WARN:
		if (logLevel == EMERG ||
                    logLevel == ALERT ||
                    logLevel == CRIT
                    || logLevel == ERR) {
			return false;
		} else {
			return true;
		}
	case NOTE:
		if (logLevel == NOTE || logLevel == INFO || logLevel == DBG) {
			return true;
		} else {
			return false;
		}
	case INFO:
		if (logLevel == INFO || logLevel == DBG) {
			return true;
		} else {
			return false;
		}
	case DBG:
		if (logLevel == DBG) {
			return true;
		} else {
			return false;
		}
	default:
		return false;
	}
}

void log(LOG_LEVEL level, const char * fmt, ...)
{
	bool goon;
	pthread_rwlock_rdlock(&logLevelLock);
	goon = shouldLog(level);
	pthread_rwlock_unlock(&logLevelLock);

	if (!goon)
		return;

	va_list args;

	va_start(args, fmt);

	pthread_rwlock_rdlock(&outputStreamLock);
	time_t now= time(0);
	if (logOutputStream != stdout) {
			fprintf(logOutputStream, "%d(%ld)", getProcessId(), now);
			vfprintf(logOutputStream, fmt, args);
	}
	fprintf(stdout, "%d(%ld)", getProcessId(), now);
	vfprintf(stdout, fmt, args);

	pthread_rwlock_unlock(&outputStreamLock);

	va_end(args);
}
