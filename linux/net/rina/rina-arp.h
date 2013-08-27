/*
 * Implementation of RFC 826 because current implementation is too
 * intertwined with IP version 4.
 *
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#ifndef RINA_ARP_H
#define RINA_ARP_H

#define ARPHRD_ETHER    1               /* Ethernet                     */

#define ARPOP_REQUEST   1               /* ARP request                  */
#define ARPOP_REPLY     2               /* ARP reply                    */

struct network_mapping {
        struct list_head list;

        /* The network address advertised */
        unsigned char *network_address;
	
	/* Hardware address */
	unsigned char hw_addr[ALIGN(MAX_ADDR_LEN, sizeof(unsigned long))];
};

struct arp_entries {
	struct list_head list;
	struct list_head temp_entries;
	struct list_head perm_entries;
};

static struct arp_data {
	struct list_head list;
	struct list_head arp_entries;
	__be16 ar_pro;
	struct net_device *dev;
};


struct arphdr {
	__be16          ar_hrd;         /* format of hardware address   */
	__be16          ar_pro;         /* format of protocol address   */
	unsigned char   ar_hln;         /* length of hardware address   */
	unsigned char   ar_pln;         /* length of protocol address   */
	__be16          ar_op;          /* ARP opcode (command)         */
 
#if 0
	/*
	 *      This bit is variable sized however...
	 *      This is an example
	 */
	unsigned char    ar_sha[ETH_ALEN];     /* sender hardware address   */
	unsigned char    ar_sip[4];            /* sender IP address         */
	unsigned char    ar_tha[ETH_ALEN];     /* target hardware address   */
	unsigned char    ar_tip[4];            /* target IP address         */
#endif
 
};

static inline struct arphdr *arp_hdr(const struct sk_buff *skb)
{
	return (struct arphdr *)skb_network_header(skb);
}

struct arp_reply_ops {
	struct list_head list;
	__be16 ar_pro;
	struct net_device *dev;
	void (*handle)(struct sk_buff *skb);
};

int             add_arp_reply_handler(struct arp_reply_ops *ops);
int             remove_arp_reply_handler(struct arp_reply_ops *ops);
int             register_network_address(__be16 ar_pro, 
					 struct net_device *dev, 
					 unsigned char *netw_addr);
int             unregister_network_address(__be16 ar_pro, 
					   struct net_device *dev, 
					   unsigned char *netw_addr);
unsigned char * lookup_network_address(__be16 ar_pro, 
				       struct net_device *dev, 
				       unsigned char *netw_addr);
int              arp_send_request(__be16 ar_pro, 
				  struct net_device *dev, 
				  unsigned char *src_netw_addr,
				  unsigned char *dest_netw_addr);
#endif
