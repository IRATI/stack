/*
 * Logging facilities
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef LIBRINA_LOGS_H
#define LIBRINA_LOGS_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>


#ifndef RINA_PREFIX
#error You must define RINA_PREFIX before including this file
#endif

/**
 * The log levels
 */
enum LOG_LEVEL {
	EMERG,
	ALERT,
	CRIT,
	ERR,
	WARN,
	NOTE,
	INFO,
	DBG
};

/**
 * Global log level, default is INFO
 */
extern enum LOG_LEVEL logLevel;

/**
 * The stream where to print the log. If none is provided,
 * it will still be printed to stdout
 */
extern FILE * logOutputStream;



//Extern C
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**
 * Set the log level
 *
 * @param logLevel the new log level
 */
void setLogLevel(const char* logLevel);

/**
 * Set the output stream.
 *
 * @param pathToFile Path to the log file
 * @returns 0 if successful, -1 if there is an error
 */
int setLogFile(const char* pathToFile);

/**
 * Prints a log statement to the output stream, in case it can be done
 * according to the log level
 * @param level the log level
 * @param fmt a string with the print statement
 * @param ... multiple arguments for the string
 */
void logFunc(enum LOG_LEVEL level, const char * fmt, ...);

//Extern C
#ifdef __cplusplus
}
#endif //__cplusplus


#define __STRINGIZE(x) #x

#define __LOG(PREFIX, LEVEL, FMT, ARGS...)                                    \
        do {                                                                  \
		logFunc(LEVEL,                                                    \
                    "%d(%ld)#" PREFIX " (" __STRINGIZE(LEVEL) "): " FMT "\n", \
                    getpid(), time(0), ##ARGS);                               \
	} while (0)

#define LOG_EMERG(FMT, ARGS...) __LOG(RINA_PREFIX, EMERG, FMT, ##ARGS)
#define LOG_ALERT(FMT, ARGS...) __LOG(RINA_PREFIX, ALERT, FMT, ##ARGS)
#define LOG_CRIT(FMT,  ARGS...) __LOG(RINA_PREFIX, CRIT,  FMT, ##ARGS)
#define LOG_ERR(FMT,   ARGS...) __LOG(RINA_PREFIX, ERR,   FMT, ##ARGS)
#define LOG_WARN(FMT,  ARGS...) __LOG(RINA_PREFIX, WARN,  FMT, ##ARGS)
#define LOG_NOTE(FMT,  ARGS...) __LOG(RINA_PREFIX, NOTE,  FMT, ##ARGS)
#define LOG_INFO(FMT,  ARGS...) __LOG(RINA_PREFIX, INFO,  FMT, ##ARGS)
#define LOG_DBG(FMT,   ARGS...) __LOG(RINA_PREFIX, DBG,   FMT, ##ARGS)

#define __LOGF(PREFIX, LEVEL, FMT, ARGS...)                                       \
        do {                                                                      \
		logFunc(LEVEL,                                                        \
                    "%d(%ld)#" PREFIX " (" __STRINGIZE(LEVEL) ")[%s]: " FMT "\n", \
                    getpid(), time(0), __func__, ##ARGS);                         \
	} while (0)

#define LOGF_EMERG(FMT, ARGS...) __LOGF(RINA_PREFIX, EMERG, FMT, ##ARGS)
#define LOGF_ALERT(FMT, ARGS...) __LOGF(RINA_PREFIX, ALERT, FMT, ##ARGS)
#define LOGF_CRIT(FMT,  ARGS...) __LOGF(RINA_PREFIX, CRIT,  FMT, ##ARGS)
#define LOGF_ERR(FMT,   ARGS...) __LOGF(RINA_PREFIX, ERR,   FMT, ##ARGS)
#define LOGF_WARN(FMT,  ARGS...) __LOGF(RINA_PREFIX, WARN,  FMT, ##ARGS)
#define LOGF_NOTE(FMT,  ARGS...) __LOGF(RINA_PREFIX, NOTE,  FMT, ##ARGS)
#define LOGF_INFO(FMT,  ARGS...) __LOGF(RINA_PREFIX, INFO,  FMT, ##ARGS)
#define LOGF_DBG(FMT,   ARGS...) __LOGF(RINA_PREFIX, DBG,   FMT, ##ARGS)

#endif //LIBRINA_LOGS_H
