/*
 * NetLink related utilities
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef RINA_NETLINK_UTILS_H
#define RINA_NETLINK_UTILS_H

#include "rmt.h"

/* These definitions must be moved in the user-space exported headers */
/* Leo: not now since it is only used internally, but msg and msg_attrs structs
 * do */

enum app_name_info_attrs_list {
        APNI_ATTR_PROCESS_NAME = 1,
        APNI_ATTR_PROCESS_INSTANCE,
        APNI_ATTR_ENTITY_NAME,
        APNI_ATTR_ENTITY_INSTANCE,
        __APNI_ATTR_MAX,
};

#define APNI_ATTR_MAX (__APNI_ATTR_MAX - 1)

enum flow_spec_attrs_list {
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

#define FSPEC_ATTR_MAX (__FSPEC_ATTR_MAX - 1)

enum ipcm_alloc_flow_req_resp_attrs_list {
        AAFRE_ATTR_DIF_NAME = 1,
        AAFRE_ATTR_ACCEPT,
        AAFRE_ATTR_DENY_REASON,
        AAFRE_ATTR_NOTIFY_SOURCE,
        __AAFRE_ATTR_MAX,
};

#define AAFRE_ATTR_MAX (__AAFRE_ATTR_MAX -1)

enum ipcm_alloc_flow_req_attrs_list {
        IAFRM_ATTR_SOURCE_APP = 1,
        IAFRM_ATTR_DEST_APP,
        IAFRM_ATTR_FLOW_SPEC,
        IAFRM_ATTR_DIF_NAME,
        IAFRM_ATTR_PORT_ID,
        IAFRM_ATTR_APP_PORT,
        __IAFRM_ATTR_MAX,
};

#define IAFRM_ATTR_MAX (__IAFRM_ATTR_MAX -1)

enum ipcm_alloc_flow_req_arrived_attrs_list {
	AAFRA_ATTR_SOURCE_APP_NAME = 1,
	AAFRA_ATTR_DEST_APP_NAME,
	AAFRA_ATTR_FLOW_SPEC,
	AAFRA_ATTR_PORT_ID,
	AAFRA_ATTR_DIF_NAME,
	__AAFRA_ATTR_MAX,
};

#define AAFRA_ATTR_MAX (__AAFRA_ATTR_MAX -1)

enum ipcm_dealloc_flow_req_msg_attrs_list {
        ADFRT_ATTR_PORT_ID = 1,
        ADFRT_ATTR_DIF_NAME,
        ADFRT_ATTR_APP_NAME,
        __ADFRT_ATTR_MAX,
};

#define ADFRT_ATTR_MAX (__ADFRT_ATTR_MAX -1)

enum ipcm_dealloc_flow_resp_attrs_list {
        ADFRE_ATTR_RESULT = 1,
        ADFRE_ATTR_ERROR_DESCRIPTION,
        ADFRE_ATTR_APP_NAME,
        __ADFRE_ATTR_MAX,
};

#define ADFRE_ATTR_MAX (__ADFRE_ATTR_MAX -1)


struct rina_msg_hdr {
	unsigned int src_ipc_id;
	unsigned int dst_ipc_id;
};

struct rnl_msg {
            
        /* Generic RINA Netlink family identifier */
        int family;

        /* source nl port id */
        unsigned int src_port;

        /* destination nl port id */
        unsigned int dst_port;

        /* The message sequence number */
        unsigned int seq_num;

        /* The operation code */
        msg_id op_code;

        /* True if this is a request message */
        bool req_msg_flag;

        /* True if this is a response message */
        bool resp_msg_flag;

        /* True if this is a notification message */
        bool notification_msg_flag;

        /* RINA header containing IPC processes ids */
        struct rina_msg_hdr * rina_hdr;

        /* Specific message attributes */
        void * attrs;
};

/* FIXME: all the alloc flow structs are the same
 * we can use only a generic one */
struct rnl_ipcm_alloc_flow_req_msg_attrs {
	struct name 	 source;
	struct name 	 dest;
	struct flow_spec fspec;
	port_id_t        id;
	struct name	 dif_name;
};

struct rnl_ipcm_alloc_flow_req_arrived_msg_attrs {
	struct name 	 source;
	struct name 	 dest;
	struct flow_spec fspec;
	port_id_t        id;
	struct name	 dif_name;
};

struct rnl_alloc_flow_resp_msg_attrs {
	struct name dif_name;
	bool 	    accept;
	string_t *  deny_reason;
	bool	    notify_src;	
};

struct rnl_ipcm_dealloc_flow_req_msg_attrs {
	port_id_t	id;
	struct name	dif_name;
	struct name	app_name;
};

struct rnl_ipcm_dealloc_flow_resp_msg_attrs {
	uint_t		result;
	string_t 	* err_desc;
	struct name	app_name;
};

int rnl_parse_msg(struct genl_info  * info,
                  struct rnl_msg    * msg);

/* FIXME: dif_config does not exist */
struct dif_config;

int rnl_format_ipcm_assign_to_dif_req_msg(struct dif_config  * config,
				          struct sk_buff  * skb_out);

int rnl_format_ipcm_assign_to_dif_resp_msg(uint_t	   result,
				           struct sk_buff  * skb_out);

int rnl_format_ipcm_ipcp_dif_reg_noti_msg(struct name     * ipcp_name,
					  struct name	  * dif_name,
					  bool		  is_registered,
				          struct sk_buff  * skb_out);

int rnl_format_ipcm_ipcp_dif_unreg_noti_msg(uint_t	    result,
				            struct sk_buff  * skb_out);

int rnl_format_ipcm_enroll_to_dif_req_msg(struct name    * dif_name,
					  struct sk_buff * skb_out);

int rnl_format_ipcm_enroll_to_dif_resp_msg(uint_t         result,
				           struct sk_buff * skb_out);

int rnl_format_ipcm_disconn_neighbor_req_msg(struct name    * neighbor_name,
				             struct sk_buff * skb_out);

int rnl_format_ipcm_disconn_neighbor_resp_msg(uint_t         result,
				              struct sk_buff * skb_out);


int rnl_format_ipcm_alloc_flow_req_msg(struct name 	* source,
				       struct name 	* dest,
				       struct flow_spec	* fspec,
				       port_id_t        id,
				       struct name	* dif_name,
				       struct sk_buff   * skb_out);

int rnl_format_ipcm_alloc_flow_req_arrived_msg(struct name      * source,
					       struct name 	* dest,
					       struct flow_spec	* fspec,
					       port_id_t        id,
					       struct name	* dif_name,
					       struct sk_buff   * skb_out);

int rnl_format_ipcm_alloc_flow_req_result_msg(uint_t	      result,
				              struct sk_buff  * skb_out);

int rnl_format_ipcm_alloc_flow_resp_msg(uint_t		result,
				        struct sk_buff  * skb_out);

int rnl_format_ipcm_dealloc_flow_req_msg(port_id_t	 id, 
				         struct sk_buff  * skb_out);

int rnl_format_ipcm_dealloc_flow_resp_msg(uint_t          result,
				          struct sk_buff  * skb_out);

int rnl_format_ipcm_flow_dealloc_noti_msg(port_id_t	  id,
					  uint_t	  code,
				          struct sk_buff  * skb_out);

int rnl_format_ipcm_reg_app_req_msg(struct name     * app_name,
				    struct name     * dif_name,
			            struct sk_buff  * skb_out);

int rnl_format_ipcm_reg_app_resp_msg(uint_t	     result,
				     struct sk_buff  * skb_out);

int rnl_format_ipcm_unreg_app_req_msg(struct name     * app_name,
				      struct name     * dif_name,
			              struct sk_buff  * skb_out);

int rnl_format_ipcm_unreg_app_resp_msg(uint_t	       result,
				       struct sk_buff * skb_out);

int rnl_format_ipcm_query_rib_req_msg(enum rib_object_t	rib_obj_class,
				      string_t		* obj_name,
				      long unsigned int obj_instance,
				      uint_t		scope,
				      const regex_t	* filter,
				      struct sk_buff    * skb_out);

int rnl_format_ipcm_query_rib_resp_msg(uint_t	       result,
				       struct sk_buff  * skb_out);

int rnl_format_rmt_add_fte_req_msg(struct pdu_ft_entry * entry,
			           struct sk_buff      * skb_out);

int rnl_format_rmt_del_fte_req_msg(struct pdu_ft_entry *entry,
			           struct sk_buff      * skb_out);

int rnl_format_rmt_dump_ft_req_msg(struct sk_buff  * skb_out);

int rnl_format_rmt_dump_ft_reply_msg(size_t	         count,
				     struct pdu_ft_entry ** entries,
			             struct sk_buff      * skb_out);
#endif
