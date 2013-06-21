/*
 *  Nl Listener (Netlink Listener)
 *
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define RINA_PREFIX "nl-listener"

int nl_listener_init(void)
{
	LOG_FBEGN;
	nl_sk=netlink_kernel_create(&init_net, NETLINK_RINA, 0, nl_recv_msg, NULL, THIS_MODULE);
	if(!nl_sk){
		LOG_ALERT("Error creating socket.\n");
		return -10;
	}
	LOG_FEXIT;
	return 0;
}

void nl_listener_exit(void)
{
	LOG_FBEGN;
	netlink_kernel_release(nl_sk);
	LOG_FEXIT;
}


static void nl_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	int pid;
	struct sk_buff *skb_out;
	int msg_size;
	char *msg="Hello from kernel";
	int res;

	LOG_DBG("Entering: %s\n", __FUNCTION__);
				        
	msg_size=strlen(msg);
					        
	nlh=(struct nlmsghdr*)skb->data;
	LOGDBG("Netlink received msg payload: %s\n",(char*)nlmsg_data(nlh));
	pid = nlh->nlmsg_pid; /*pid of sending process */

	skb_out = nlmsg_new(msg_size,0);

	if(!skb_out){
		LOG_ERROR("Failed to allocate new skb\n");
		return;
	} 

	nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);

	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
        strncpy(nlmsg_data(nlh),msg,msg_size);														    
	res=nlmsg_unicast(nl_sk,skb_out,pid);
																    
	if(res<0)
		LOG_DBG("Error while sending bak to user\n");
}

