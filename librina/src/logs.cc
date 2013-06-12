/*
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

/*
 * logs.cpp
 *
 *  Created on: 12/06/2013
 *      Author: eduardgrasa
 */

#ifndef RINA_PREFIX
#define RINA_PREFIX " "
#endif

#include "logs.h"
#include <stdlib.h>
#include <cstdio>

LOG_LEVEL logLevel = DBG;
FILE * logOutputStream;

void setLogLevel(LOG_LEVEL newLogLevel){
	logLevel = newLogLevel;
}

int setOutputStream(FILE * newOutputStream){
	if (logOutputStream == NULL && newOutputStream != NULL){
		logOutputStream = newOutputStream;
		return 0;
	}

	return -1;
}

bool shouldLog(LOG_LEVEL level){
	switch(level){
	case EMERG:{
		return true;
	}
	case ALERT:{
		if (logLevel == EMERG){
			return false;
		}else{
			return true;
		}
	}
	case CRIT:{
		if(logLevel == EMERG || logLevel == ALERT){
			return false;
		}else{
			return true;
		}
	}
	case ERR:{
		if(logLevel == EMERG || logLevel == ALERT || logLevel == CRIT){
			return false;
		}else{
			return true;
		}
	}
	case WARN:{
		if(logLevel == EMERG || logLevel == ALERT || logLevel == CRIT || logLevel == ERR){
			return false;
		}else{
			return true;
		}
	}
	case NOTE:{
		if (logLevel == NOTE || logLevel == INFO || logLevel == DBG){
			return true;
		}else{
			return false;
		}
	}
	case INFO:{
		if (logLevel == INFO || logLevel == DBG){
			return true;
		}else{
			return false;
		}
	}
	case DBG:{
		if (logLevel  == DBG){
			return true;
		}else{
			return false;
		}
	}
	default:{
		return false;
	}
	}
}

void LOG(LOG_LEVEL level, const char* fmt, va_list args){
	if (!shouldLog(level)){
		return;
	}

	if (logOutputStream == NULL){
		printf("librina-");
		printf(RINA_PREFIX);
		printf(": ");
		vprintf(fmt, args);
		printf("\n");
	}else{
		fprintf(logOutputStream, "librina-");
		fprintf(logOutputStream, RINA_PREFIX);
		fprintf(logOutputStream, ": ");
		vfprintf(logOutputStream, fmt, args);
		fprintf(logOutputStream, "\n");
	}
}

void LOG_DBG(const char* fmt, ...){
	va_list args;
	va_start (args, fmt);
	LOG(DBG, fmt, args);
	va_end(args);
}

void LOG_INFO(const char* fmt, ...){
	va_list args;
	va_start (args, fmt);
	LOG(INFO, fmt, args);
	va_end(args);
}

void LOG_NOTE(const char* fmt, ...){
	va_list args;
	va_start (args, fmt);
	LOG(NOTE, fmt, args);
	va_end(args);
}

void LOG_WARN(const char* fmt, ...){
	va_list args;
	va_start (args, fmt);
	LOG(WARN, fmt, args);
	va_end(args);
}

void LOG_ERR(const char* fmt, ...){
	va_list args;
	va_start (args, fmt);
	LOG(ERR, fmt, args);
	va_end(args);
}

void LOG_CRIT(const char* fmt, ...){
	va_list args;
	va_start (args, fmt);
	LOG(CRIT, fmt, args);
	va_end(args);
}

void LOG_ALERT(const char* fmt, ...){
	va_list args;
	va_start (args, fmt);
	LOG(ALERT, fmt, args);
	va_end(args);
}

void LOG_EMERG(const char* fmt, ...){
	va_list args;
	va_start (args, fmt);
	LOG(EMERG, fmt, args);
	va_end(args);
}

