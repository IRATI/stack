/*
 * NetLink support
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *
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

#define RINA_PREFIX "netlink"

#include "logs.h"
#include "netlink.h"

int rina_netlink_init(void)
{
	int ret;
	
	LOG_FBEGN;
	
	ret = genl_register_family(&nl_rina_family);
	if (ret != 0){
		LOG_DBG("Not able to register nl_rina_family");
		return -1;
	}
	
	ret = genl_register_ops(&nl_rina_family, &nl_rina_ops_echo);
	if (ret != 0){
		LOG_DBG("Not able to register nl_rina_ops");
		genl_unrigester_family(&nl_rina_family);
		return -2;
	}
	LOG_FEXIT;
	return 0;
}

void rina_netlink_exit(void)
{
        LOG_FBEGN;
	
	int ret;

	/*unregister the functions*/
	ret = genl_unregister_ops(&nl_rina_family, nl_rina_ops_echo);

	if(ret != 0){
		LOG_DBG("unregister ops: %i\n",ret);
		return;
	}
	
	/*unregister the family*/
	ret = genl_unregister_family(&nl_rina_family);
	
	if(ret !=0){
		LOG_DBG("unregister family %i\n",ret);
	}
	LOG_FEXIT;
}

/* handler */
static int nl_rina_echo(struct sk_buff *skb, struct genl_info *info)
{
	/* message handling code goes here; return 0 on success, negative
	* values on failure */
	LOG_DBG("ECHOING MESSAGE");
}

