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

#include <netlink-manager.h>

namespace rina {

int putBaseNetlinkMessage(nl_msg* netlinkMessage,
		BaseNetlinkMessage * message);

BaseNetlinkMessage * parseBaseNetlinkMessage(nlmsghdr* netlinkMesasgeHeader);

/* APPLICATION PROCESS NAMING INFORMATION CLASS */
enum ApplicationProcessNamingInformationAttributes{
	APNI_ATTR_PROCESS_NAME = 1,
	APNI_ATTR_PROCESS_INSTANCE,
	APNI_ATTR_ENTITY_NAME,
	APNI_ATTR_ENTITY_INSTANCE,
	__APNI_ATTR_MAX,
};

#define APNI_ATTR_MAX (__APNI_ATTR_MAX -1)

int putApplicationProcessNamingInformationObject(nl_msg* netlinkMessage,
		const ApplicationProcessNamingInformation& object);

ApplicationProcessNamingInformation * parseApplicationProcessNamingInformationObject(nlattr *nested);

/* AppAllocateFlowRequestMessage CLASS*/
enum AppAllocateFlowRequestAttributes{
	AAFR_ATTR_SOURCE_APP_NAME = 1,
	AAFR_ATTR_DEST_APP_NAME,
	AAFR_ATTR_FLOW_SPEC,
	__AAFR_ATTR_MAX,
};

#define AAFR_ATTR_MAX (__AAFR_ATTR_MAX -1)

int putAppAllocateFlowRequestMessageObject(nl_msg* netlinkMessage,
		AppAllocateFlowRequestMessage * object);

AppAllocateFlowRequestMessage * parseAppAllocateFlowRequestMessage(nlmsghdr *hdr);

/* FLOW SPECIFICATION CLASS */
enum FlowSpecificationAttributes{
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


}
#endif /* LIBRINA_NETLINK_PARSERS_H_ */
