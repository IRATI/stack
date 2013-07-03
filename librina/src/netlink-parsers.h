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
 * librina-netlink-parsers.h
 *
 *  Created on: 14/06/2013
 *      Author: eduardgrasa
 */

#ifndef LIBRINA_NETLINK_PARSERS_H_
#define LIBRINA_NETLINK_PARSERS_H_

#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>

#include "netlink-messages.h"

namespace rina {

int putBaseNetlinkMessage(nl_msg* netlinkMessage, BaseNetlinkMessage * message);

BaseNetlinkMessage * parseBaseNetlinkMessage(nlmsghdr* netlinkMesasgeHeader);

/* APPLICATION PROCESS NAMING INFORMATION CLASS */
enum ApplicationProcessNamingInformationAttributes {
	APNI_ATTR_PROCESS_NAME = 1,
	APNI_ATTR_PROCESS_INSTANCE,
	APNI_ATTR_ENTITY_NAME,
	APNI_ATTR_ENTITY_INSTANCE,
	__APNI_ATTR_MAX,
};

#define APNI_ATTR_MAX (__APNI_ATTR_MAX -1)

int putApplicationProcessNamingInformationObject(nl_msg* netlinkMessage,
		const ApplicationProcessNamingInformation& object);

ApplicationProcessNamingInformation *
parseApplicationProcessNamingInformationObject(nlattr *nested);

/* AppAllocateFlowRequestMessage CLASS*/
enum AppAllocateFlowRequestAttributes {
	AAFR_ATTR_SOURCE_APP_NAME = 1,
	AAFR_ATTR_DEST_APP_NAME,
	AAFR_ATTR_FLOW_SPEC,
	__AAFR_ATTR_MAX,
};

#define AAFR_ATTR_MAX (__AAFR_ATTR_MAX -1)

int putAppAllocateFlowRequestMessageObject(nl_msg* netlinkMessage,
		const AppAllocateFlowRequestMessage& object);

AppAllocateFlowRequestMessage * parseAppAllocateFlowRequestMessage(
		nlmsghdr *hdr);

/* FLOW SPECIFICATION CLASS */
enum FlowSpecificationAttributes {
	FSPEC_ATTR_AVG_BWITH = 1,
	FSPEC_ATTR_AVG_SDU_BWITH,
	FSPEC_ATTR_DELAY,
	FSPEC_ATTR_JITTER,
	FSPEC_ATTR_MAX_GAP,
	FSPEC_ATTR_MAX_SDU_SIZE,
	FSPEC_ATTR_IN_ORD_DELIVERY,
	FSPEC_ATTR_PART_DELIVERY,
	FSPEC_ATTR_PEAK_BWITH_DURATION,
	FSPEC_ATTR_PEAK_SDU_BWITH_DURATION,
	FSPEC_ATTR_UNDETECTED_BER,
	__FSPEC_ATTR_MAX,
};

#define FSPEC_ATTR_MAX (__FSPEC_ATTR_MAX -1)

int putFlowSpecificationObject(nl_msg* netlinkMessage,
		const FlowSpecification& object);

FlowSpecification * parseFlowSpecificationObject(nlattr *nested);

/* AppAllocateFlowRequestResultMessage CLASS*/
enum AppAllocateFlowRequestResultAttributes {
	AAFRR_ATTR_SOURCE_APP_NAME = 1,
	AAFRR_ATTR_PORT_ID,
	AAFRR_ATTR_ERROR_DESCRIPTION,
	AAFRR_ATTR_DIF_NAME,
	AAFRR_ATTR_IPC_PROCESS_PORT_ID,
	AAFRR_ATTR_IPC_PROCESS_ID,
	__AAFRR_ATTR_MAX,
};

#define AAFRR_ATTR_MAX (__AAFRR_ATTR_MAX -1)

int putAppAllocateFlowRequestResultMessageObject(nl_msg* netlinkMessage,
		const AppAllocateFlowRequestResultMessage& object);

AppAllocateFlowRequestResultMessage * parseAppAllocateFlowRequestResultMessage(
		nlmsghdr *hdr);

/* AppAllocateFlowRequestMessage CLASS*/
enum AppAllocateFlowRequestArrivedAttributes {
	AAFRA_ATTR_SOURCE_APP_NAME = 1,
	AAFRA_ATTR_DEST_APP_NAME,
	AAFRA_ATTR_FLOW_SPEC,
	AAFRA_ATTR_PORT_ID,
	AAFRA_ATTR_DIF_NAME,
	__AAFRA_ATTR_MAX,
};

#define AAFRA_ATTR_MAX (__AAFRA_ATTR_MAX -1)

int putAppAllocateFlowRequestArrivedMessageObject(nl_msg* netlinkMessage,
		const AppAllocateFlowRequestArrivedMessage& object);

AppAllocateFlowRequestArrivedMessage * parseAppAllocateFlowRequestArrivedMessage(
		nlmsghdr *hdr);

/* AppAllocateFlowResponseMessage CLASS*/
enum AppAllocateFlowResponseAttributes {
	AAFRE_ATTR_DIF_NAME = 1,
	AAFRE_ATTR_ACCEPT,
	AAFRE_ATTR_DENY_REASON,
	AAFRE_ATTR_NOTIFY_SOURCE,
	__AAFRE_ATTR_MAX,
};

#define AAFRE_ATTR_MAX (__AAFRE_ATTR_MAX -1)

int putAppAllocateFlowResponseMessageObject(nl_msg* netlinkMessage,
		const AppAllocateFlowResponseMessage& object);

AppAllocateFlowResponseMessage * parseAppAllocateFlowResponseMessage(
		nlmsghdr *hdr);

/* AppDeallocateFlowRequestMessage CLASS*/
enum AppDeallocateFlowRequestMessageAttributes {
	ADFRT_ATTR_PORT_ID = 1,
	ADFRT_ATTR_DIF_NAME,
	ADFRT_ATTR_APP_NAME,
	__ADFRT_ATTR_MAX,
};

#define ADFRT_ATTR_MAX (__ADFRT_ATTR_MAX -1)

int putAppDeallocateFlowRequestMessageObject(nl_msg* netlinkMessage,
		const AppDeallocateFlowRequestMessage& object);

AppDeallocateFlowRequestMessage * parseAppDeallocateFlowRequestMessage(
		nlmsghdr *hdr);

/* AppDeallocateFlowResponseMessage CLASS*/
enum AppDeallocateFlowResponseMessageAttributes {
	ADFRE_ATTR_RESULT = 1,
	ADFRE_ATTR_ERROR_DESCRIPTION,
	ADFRE_ATTR_APP_NAME,
	__ADFRE_ATTR_MAX,
};

#define ADFRE_ATTR_MAX (__ADFRE_ATTR_MAX -1)

int putAppDeallocateFlowResponseMessageObject(nl_msg* netlinkMessage,
		const AppDeallocateFlowResponseMessage& object);

AppDeallocateFlowResponseMessage * parseAppDeallocateFlowResponseMessage(
		nlmsghdr *hdr);

/* AppFlowDeallocatedNotificationMessage CLASS*/
enum AppFlowDeallocatedNotificationMessageAttributes {
	AFDN_ATTR_PORT_ID = 1,
	AFDN_ATTR_CODE,
	AFDN_ATTR_REASON,
	AFDN_ATTR_APP_NAME,
	AFDN_ATTR_DIF_NAME,
	__AFDN_ATTR_MAX,
};

#define AFDN_ATTR_MAX (__AFDN_ATTR_MAX -1)

int putAppFlowDeallocatedNotificationMessageObject(nl_msg* netlinkMessage,
		const AppFlowDeallocatedNotificationMessage& object);

AppFlowDeallocatedNotificationMessage * parseAppFlowDeallocatedNotificationMessage(
		nlmsghdr *hdr);


/* AppRegisterApplicationRequestMessage CLASS*/
enum AppRegisterApplicationRequestMessageAttributes {
	ARAR_ATTR_APP_NAME = 1,
	ARAR_ATTR_DIF_NAME,
	__ARAR_ATTR_MAX,
};

#define ARAR_ATTR_MAX (__ARAR_ATTR_MAX -1)

int putAppRegisterApplicationRequestMessageObject(nl_msg* netlinkMessage,
		const AppRegisterApplicationRequestMessage& object);

AppRegisterApplicationRequestMessage * parseAppRegisterApplicationRequestMessage(
		nlmsghdr *hdr);

/* AppRegisterApplicationResponseMessage CLASS*/
enum AppRegisterApplicationResponseMessageAttributes {
	ARARE_ATTR_APP_NAME = 1,
	ARARE_ATTR_RESULT,
	ARARE_ATTR_ERROR_DESCRIPTION,
	ARARE_ATTR_DIF_NAME,
	ARARE_ATTR_PROCESS_PORT_ID,
	ARARE_ATTR_PROCESS_IPC_PROCESS_ID,
	__ARARE_ATTR_MAX,
};

#define ARARE_ATTR_MAX (__ARARE_ATTR_MAX -1)

int putAppRegisterApplicationResponseMessageObject(nl_msg* netlinkMessage,
		const AppRegisterApplicationResponseMessage& object);

AppRegisterApplicationResponseMessage *
	parseAppRegisterApplicationResponseMessage(nlmsghdr *hdr);

/* IpcmRegisterApplicationRequestMessage CLASS*/
enum IpcmRegisterApplicationRequestMessageAttributes {
	IRAR_ATTR_APP_NAME = 1,
	IRAR_ATTR_DIF_NAME,
	IRAR_ATTR_APP_PORT_ID,
	__IRAR_ATTR_MAX,
};

#define IRAR_ATTR_MAX (__IRAR_ATTR_MAX -1)

int putIpcmRegisterApplicationRequestMessageObject(nl_msg* netlinkMessage,
		const IpcmRegisterApplicationRequestMessage& object);

IpcmRegisterApplicationRequestMessage *
	parseIpcmRegisterApplicationRequestMessage(nlmsghdr *hdr);

/* IpcmRegisterApplicationResponseMessage CLASS*/
enum IpcmRegisterApplicationResponseMessageAttributes {
	IRARE_ATTR_APP_NAME = 1,
	IRARE_ATTR_RESULT,
	IRARE_ATTR_ERROR_DESCRIPTION,
	IRARE_ATTR_DIF_NAME,
	__IRARE_ATTR_MAX,
};

#define IRARE_ATTR_MAX (__IRARE_ATTR_MAX -1)

int putIpcmRegisterApplicationResponseMessageObject(nl_msg* netlinkMessage,
		const IpcmRegisterApplicationResponseMessage& object);

IpcmRegisterApplicationResponseMessage *
	parseIpcmRegisterApplicationResponseMessage(nlmsghdr *hdr);

}

#endif /* LIBRINA_NETLINK_PARSERS_H_ */
