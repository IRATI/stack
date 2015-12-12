/*
 * Loggin wrapping for the IPCP
 *
 *    Marc Sune  <marc.sune (at) bisdn.de>
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

#ifndef IPCP_LOGGING_HH
#define IPCP_LOGGING_HH

#include <string.h>
#define RINA_PREFIX "ipcp"
#include <librina/logs.h>

//fwd decl
extern int ipcp_id;

#ifndef IPCP_MODULE
	#define IPCP_PREFIX RINA_PREFIX "[%d]"
#else
	#define IPCP_PREFIX RINA_PREFIX "[%d]." IPCP_MODULE
#endif

//Wrap them
#define LOG_IPCP_EMERG(FMT, ARGS...) __LOG(IPCP_PREFIX, EMERG, FMT, ipcp_id, ##ARGS)
#define LOG_IPCP_ALERT(FMT, ARGS...) __LOG(IPCP_PREFIX, ALERT, FMT, ipcp_id, ##ARGS)
#define LOG_IPCP_CRIT(FMT,  ARGS...) __LOG(IPCP_PREFIX, CRIT,  FMT, ipcp_id, ##ARGS)
#define LOG_IPCP_ERR(FMT,   ARGS...) __LOG(IPCP_PREFIX, ERR,   FMT, ipcp_id, ##ARGS)
#define LOG_IPCP_WARN(FMT,  ARGS...) __LOG(IPCP_PREFIX, WARN,  FMT, ipcp_id, ##ARGS)
#define LOG_IPCP_NOTE(FMT,  ARGS...) __LOG(IPCP_PREFIX, NOTE,  FMT, ipcp_id, ##ARGS)
#define LOG_IPCP_INFO(FMT,  ARGS...) __LOG(IPCP_PREFIX, INFO,  FMT, ipcp_id, ##ARGS)
#define LOG_IPCP_DBG(FMT,   ARGS...) __LOG(IPCP_PREFIX, DBG,   FMT, ipcp_id, ##ARGS)

#endif //IPCP_LOGGING_HH
