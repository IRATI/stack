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

#define RINA_PREFIX ""

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

void LOG(std::string prefix, LOG_LEVEL level, std::string logLevelString, const char* fmt, ...){
	if (!shouldLog(level)){
		return;
	}

	std::string headerString = std::string("librina-") + prefix + "("+logLevelString +"): ";

	va_list args;
	va_start (args, fmt);
	if (logOutputStream == NULL){
		printf(headerString.c_str());
		vprintf(fmt, args);
		printf("\n");
	}else{
		fprintf(logOutputStream, headerString.c_str());
		vfprintf(logOutputStream, fmt, args);
		fprintf(logOutputStream, "\n");
	}
	va_end(args);
}
