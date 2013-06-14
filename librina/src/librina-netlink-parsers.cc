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
 * librina-netlink-parsers.cc
 *
 *  Created on: 14/06/2013
 *      Author: eduardgrasa
 */

#include "librina-netlink-parsers.h"
#define RINA_PREFIX "netlink-parsers"
#include "logs.h"

namespace rina{

int putApplicationProcessNamingInformationObject(nl_msg* netlinkMessage,
		const ApplicationProcessNamingInformation& object){
	NLA_PUT_STRING(netlinkMessage, APNI_ATTR_PROCESS_NAME,
			object.getProcessName().c_str());
	NLA_PUT_STRING(netlinkMessage, APNI_ATTR_PROCESS_INSTANCE,
			object.getProcessInstance().c_str());
	NLA_PUT_STRING(netlinkMessage, APNI_ATTR_ENTITY_NAME,
			object.getEntityName().c_str());
	NLA_PUT_STRING(netlinkMessage, APNI_ATTR_ENTITY_INSTANCE,
			object.getEntityInstance().c_str());

	return 0;

	nla_put_failure: 	LOG_WARN("Error building ApplicationProcessNamingInformation Netlink object");
						return -1;
}

ApplicationProcessNamingInformation * parseApplicationProcessNamingInformationObject(nlattr *nested){
	struct nla_policy attr_policy[APNI_ATTR_MAX + 1];
	attr_policy[APNI_ATTR_PROCESS_NAME].type = NLA_STRING;
	attr_policy[APNI_ATTR_PROCESS_NAME].minlen = 0;
	attr_policy[APNI_ATTR_PROCESS_NAME].maxlen = 65535;
	attr_policy[APNI_ATTR_PROCESS_INSTANCE].type = NLA_STRING;
	attr_policy[APNI_ATTR_PROCESS_INSTANCE].minlen = 0;
	attr_policy[APNI_ATTR_PROCESS_INSTANCE].maxlen = 65535;
	attr_policy[APNI_ATTR_ENTITY_NAME].type = NLA_STRING;
	attr_policy[APNI_ATTR_ENTITY_NAME].minlen = 0;
	attr_policy[APNI_ATTR_ENTITY_NAME].maxlen = 65535;
	attr_policy[APNI_ATTR_ENTITY_INSTANCE].type = NLA_STRING;
	attr_policy[APNI_ATTR_ENTITY_INSTANCE].minlen = 0;
	attr_policy[APNI_ATTR_ENTITY_INSTANCE].maxlen = 65535;
	struct nlattr *attrs[APNI_ATTR_MAX + 1];

	int err = nla_parse_nested(attrs, APNI_ATTR_MAX, nested, attr_policy);
	if (err < 0){
		LOG_WARN("Error parsing ApplicationProcessNaming information from Netlink message: %d", err);
		return NULL;
	}

	ApplicationProcessNamingInformation * result = new ApplicationProcessNamingInformation();
	if(attrs[APNI_ATTR_PROCESS_NAME]){
		result->setProcessName(nla_get_string(attrs[APNI_ATTR_PROCESS_NAME]));
	}
	if(attrs[APNI_ATTR_PROCESS_INSTANCE]){
		result->setProcessInstance(nla_get_string(attrs[APNI_ATTR_PROCESS_INSTANCE]));
	}
	if(attrs[APNI_ATTR_ENTITY_NAME]){
		result->setEntityName(nla_get_string(attrs[APNI_ATTR_ENTITY_NAME]));
	}
	if(attrs[APNI_ATTR_ENTITY_INSTANCE]){
		result->setEntityInstance(nla_get_string(attrs[APNI_ATTR_ENTITY_INSTANCE]));
	}

	return result;
}

int putAppAllocateFlowRequestMessageObject(nl_msg* netlinkMessage,
		const AppAllocateFlowRequestMessage& object){
	struct nlattr *sourceAppName, *destinationAppName;

	if (!(sourceAppName = nla_nest_start(netlinkMessage, AAFR_ATTR_SOURCE_APP_NAME))){
		goto nla_put_failure;
	}
	if (putApplicationProcessNamingInformationObject(netlinkMessage, object.getSourceAppName()) < 0){
		goto nla_put_failure;
	}
	nla_nest_end(netlinkMessage, sourceAppName);

	if (!(destinationAppName = nla_nest_start(netlinkMessage, AAFR_ATTR_DEST_APP_NAME))){
		goto nla_put_failure;
	}
	if (putApplicationProcessNamingInformationObject(netlinkMessage, object.getDestAppName()) <0){
		goto nla_put_failure;
	}
	nla_nest_end(netlinkMessage, destinationAppName);

	nla_put_failure: 	LOG_WARN("Error building AppAllocateFlowRequestMessage Netlink object");
						return -1;
}

AppAllocateFlowRequestMessage * parseAppAllocateFlowRequestMessage(nlmsghdr *hdr){
	struct nla_policy attr_policy[AAFR_ATTR_MAX+1];
	attr_policy[AAFR_ATTR_SOURCE_APP_NAME].type = NLA_NESTED;
	attr_policy[AAFR_ATTR_SOURCE_APP_NAME].minlen = 0;
	attr_policy[AAFR_ATTR_SOURCE_APP_NAME].maxlen = 65535;
	attr_policy[AAFR_ATTR_DEST_APP_NAME].type = NLA_NESTED;
	attr_policy[AAFR_ATTR_DEST_APP_NAME].minlen = 0;
	attr_policy[AAFR_ATTR_DEST_APP_NAME].maxlen = 65535;
	struct nlattr *attrs[AAFR_ATTR_MAX+1];

	/*
	 * The nlmsg_parse() function will make sure that the message contains
	 * enough payload to hold the header (struct my_hdr), validates any
	 * attributes attached to the messages and stores a pointer to each
	 * attribute in the attrs[] array accessable by attribute type.
	 */
	int err = nlmsg_parse(hdr, 0, attrs, AAFR_ATTR_MAX, attr_policy);
	if (err < 0){
		LOG_WARN("Error parsing AppAllocateFlowRequestMessage information from Netlink message: %d", err);
		return NULL;
	}

	AppAllocateFlowRequestMessage * result = new AppAllocateFlowRequestMessage();
	ApplicationProcessNamingInformation * sourceName;
	ApplicationProcessNamingInformation * destName;

	if (attrs[AAFR_ATTR_SOURCE_APP_NAME]) {
		sourceName = parseApplicationProcessNamingInformationObject(attrs[AAFR_ATTR_SOURCE_APP_NAME]);
		if (sourceName == NULL){
			delete result;
			return NULL;
		}else{
			result->setSourceAppName(*sourceName);
		}
	}

	if (attrs[AAFR_ATTR_DEST_APP_NAME]) {
		destName = parseApplicationProcessNamingInformationObject(attrs[AAFR_ATTR_DEST_APP_NAME]);
		if (destName == NULL){
			delete result;
			return NULL;
		}else{
			result->setDestAppName(*destName);
		}
	}

	return result;
}

}
