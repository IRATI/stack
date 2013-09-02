/*
 * NetLink testing 
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <net/netlink.h>
#include <net/genetlink.h>
#include <linux/export.h>

#define RINA_PREFIX "netlink-testing"

#include "logs.h"
#include "common.h"
#include "debug.h"
#include "utils.h"
#include "netlink.h"
#include "netlink-utils.h"
#include "netlink-test.h"

int data;
struct rina_nl_set * set;


static int test_echo_dispatcher(void * data, 
			   	struct sk_buff * skb_in, 
			   	struct genl_info * info)
{
	
	struct rnl_msg * my_msg;
	struct rnl_ipcm_assign_to_dif_resp_msg_attrs * attrs;
	struct rina_msg_hdr * my_hdr;
	struct sk_buff * out_msg;
	struct rina_msg_hdr * out_hdr;
	int result;

	LOG_DBG("Entering the test dispatcher....");
	LOG_DBG("[LDBG] Dispatching message (skb-in=%pK, info=%pK)", skb_in, info);

	if (!info) {
		LOG_ERR("Wrong info struct in dispatcher");
		return -1;
	}
	attrs = rkzalloc(sizeof(*attrs), GFP_KERNEL);
        if (!attrs)
                return -1;
        my_msg = rkzalloc(sizeof(*my_msg), GFP_KERNEL);
        if (!my_msg) {
		LOG_ERR("Could not allocate space for my_msg struct");
                rkfree(attrs);
                return -1;
        }
        my_hdr = rkzalloc(sizeof(*my_hdr), GFP_KERNEL);
        if (!my_hdr) {
		LOG_ERR("Could not allocate space for header");
                rkfree(attrs);
                rkfree(my_msg);
                return -1;
        }
        my_msg->attrs = attrs;
        my_msg->rina_hdr = my_hdr;
	
	LOG_DBG("[LDBG] test-dispatcher before parsing OK");

	if (rnl_parse_msg(info, my_msg)){
		LOG_ERR("Could not parse message");
		rkfree(attrs);
		rkfree(my_msg);
		rkfree(my_hdr);
		return -1;
	}

	LOG_DBG("Returned value\n"
		"RESULT: %d\n",attrs->result);

	
	
	LOG_DBG("Formatting out message...");

	out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_KERNEL);
	if(!out_msg) {
		LOG_ERR("Could not allocate memory for message");
		rkfree(attrs);
		rkfree(my_msg);
		rkfree(my_hdr);
		return -1;
	}

	out_hdr = (struct rina_msg_hdr *) genlmsg_put(
				out_msg, 
				0, 
				0, 
				get_nl_family(), 
				0, 
				RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE);
	if(!out_hdr) {
		LOG_ERR("Could not use genlmsg_put");
		nlmsg_free(out_msg);
		rkfree(attrs);
		rkfree(my_msg);
		rkfree(my_hdr);
		return -1;
	}

	out_hdr->src_ipc_id = my_hdr->dst_ipc_id;
	out_hdr->dst_ipc_id = my_hdr->src_ipc_id;

	if (rnl_format_ipcm_assign_to_dif_resp_msg(attrs->result, out_msg)){
		LOG_ERR("Could not format message...");
		nlmsg_free(out_msg);
		rkfree(attrs);
		rkfree(my_msg);
		rkfree(my_hdr);
		return -1;
	}
	result = genlmsg_end(out_msg, out_hdr);

	if (result){
		LOG_DBG("Result of genlmesg_end: %d", result);
	}
	result = genlmsg_unicast(&init_net, out_msg, info->snd_portid);
	if(result) {
		LOG_ERR("Could not send unicast msg: %d", result);
		rkfree(attrs);
		rkfree(my_msg);
		rkfree(my_hdr);
		return -1;
	}

	rkfree(attrs);
	rkfree(my_msg);
	rkfree(my_hdr);
	return 0;
}


int test_register_echo_handler(void)
{

	LOG_DBG("REGISTERING TEST HANDLER...");
	set = rina_netlink_set_create(1);
	if(!set) {
		LOG_ERR("Could not create set");
		return -1;
	}

	if (rina_netlink_set_register(set)){
		LOG_ERR("Could not register set");
		return -1;
	}

	if (rina_netlink_handler_register(set,
				RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE,
				&data,
				(message_handler_cb) test_echo_dispatcher)) {
		LOG_ERR("Could not register handler");
		return -1;
	}
	LOG_DBG("test_register_echo_handler: registered handler at %p:", test_echo_dispatcher);
	return 0;
}
EXPORT_SYMBOL(test_register_echo_handler);

static int test_dispatcher(void * data, 
			   struct sk_buff skb_in, 
			   struct genl_info * info)
{
	
	struct rnl_msg * my_msg;
	struct rnl_ipcm_alloc_flow_req_result_msg_attrs * attrs;
	struct rina_msg_hdr * my_hdr;

	LOG_DBG("ENTER TEST DISPATCHER");

	if (!info) {
		LOG_ERR("Wrong info struct in dispatcher");
		return -1;
	}

	attrs = rkzalloc(sizeof(*attrs), GFP_KERNEL);
        if (!attrs)
                return -1;
        my_msg = rkzalloc(sizeof(*my_msg), GFP_KERNEL);
        if (!my_msg) {
		LOG_ERR("Could not allocate space for my_msg struct");
                rkfree(attrs);
                return -1;
        }
        my_hdr = rkzalloc(sizeof(*my_hdr), GFP_KERNEL);
        if (!my_hdr) {
		LOG_ERR("Could not allocate space for header");
                rkfree(attrs);
                rkfree(my_msg);
                return -1;
        }
        my_msg->attrs = attrs;
        my_msg->rina_hdr = my_hdr;
	if (rnl_parse_msg(info, my_msg)){
		LOG_ERR("Could not parse message");
		return -1;
	}

	LOG_DBG("Returned value\n"
		"RESULT: %d\n",attrs->result);
	return 0;	
}

int test_register_handler(void)
{

	LOG_DBG("REGISTERING TEST HANDLER...");
	set = rina_netlink_set_create(1);
	if(!set) {
		LOG_ERR("Could not create set");
		return -1;
	}

	if (rina_netlink_set_register(set)){
		LOG_ERR("Could not register set");
		return -1;
	}

	if (rina_netlink_handler_register(set,
					  RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
					  &data,
					  (message_handler_cb) test_dispatcher)) {
		LOG_ERR("Could not register handler");
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(test_register_handler);

int test_rnl_format_ipcm_alloc_flow_req_result_msg(uint_t result)
{
	struct sk_buff * msg;
	struct rina_msg_hdr * hdr;
	
	LOG_DBG("FORMATTING TEST MESSAGE...");

	msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_KERNEL);
	if(!msg) {
		LOG_ERR("Could not allocate memory for message");
		return -1;
	}

	hdr = (struct rina_msg_hdr *) genlmsg_put(
				msg, 
				0, 
				0, 
				get_nl_family(), 
				0, 
				RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT);
	if(!hdr) {
		LOG_ERR("Could not use genlmsg_put");
		nlmsg_free(msg);
		return -1;
	}

	//memcpy(hdr, &usr_hdr, sizeof(usr_hdr));
	hdr->src_ipc_id = 0;
	hdr->dst_ipc_id = 1;

	if (rnl_format_ipcm_alloc_flow_req_result_msg(result, msg)){
		LOG_ERR("Could not format message...");
		nlmsg_free(msg);
		return -1;
	}
	result = genlmsg_end(msg, hdr);

#if 0
	LOG_DBG("msg: %p\n"
		"GENL_HDRLEN: %d\n"
		"userhdr: %p\n"
		"nlmsg_data: %p", msg, GENL_HDRLEN, hdr, nlmsg_data(hdr));
#endif

	if (result){
		LOG_DBG("Result of genlmesg_end: %d", result);
	}
	result = genlmsg_unicast(&init_net, msg, 0);
	if(result) {
		LOG_ERR("Could not send unicast msg: %d", result);
		return -1;
	}
	return 0;
}


static int test_begin_generic(struct sk_buff * msg,
		       struct rina_msg_hdr * hdr,
		       char * msg_name,
		       uint_t msg_id)
{

	LOG_DBG("Testing %s formatting...", msg_name);

	msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_KERNEL);
	if(!msg) {
		LOG_ERR("Could not allocate memory for message");
		return -1;
	}

	hdr = (struct rina_msg_hdr *) genlmsg_put(
				msg, 
				0, 
				0, 
				get_nl_family(), 
				0, 
				msg_id);
	if(!hdr) {
		LOG_ERR("Could not use genlmsg_put");
		nlmsg_free(msg);
		return -1;
	}

	hdr->src_ipc_id = 0;
	hdr->dst_ipc_id = 1;

	return 0;
}

static void test_end_generic(struct sk_buff * msg, 
		     struct rina_msg_hdr * hdr,
		     char * msg_name)
{

	genlmsg_end(msg, hdr);

	nlmsg_free(msg);

	LOG_DBG("%s test ended\n\n", msg_name);

}

void populate_generic_name(string_t    * prefix,
		           struct name * name)
{
	name->process_name = strcat(prefix,"my_process_name");
	name->process_instance = strcat(prefix,"my_process_instance");
	name->entity_name = strcat(prefix,"my_entity_name");
	name->entity_instance = strcat(prefix,"my_entity_instance");
}

int test_rnl_format_ipcm_assign_to_dif_req_msg(void)
{

	struct sk_buff * msg;
	struct rina_msg_hdr * hdr;
	struct dif_config * config;
	struct name * name;

	config = rkzalloc(sizeof(*config), GFP_KERNEL);
	name = rkzalloc(sizeof(*name), GFP_KERNEL);
	if(!config){
		LOG_ERR("Could not allocate config param");
		return -1;
	}
	if(!name){
		LOG_ERR("Could not allocate name param");
		return -1;
	}
	
	populate_generic_name("dif_",name);
	config->type = "Test";
	config->dif_name = name;

	if (test_begin_generic(msg,
	  		       hdr,
			       "RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST",
			       RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST) < 0)
		return -1;

	if (rnl_format_ipcm_assign_to_dif_req_msg(config, msg)){
		LOG_ERR("Could not format message RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST...");
		nlmsg_free(msg);
		return -1;
	}

	test_end_generic(msg, hdr, "RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST");
	rkfree(config);
	rkfree(name);
	return 0;

}

int test_rnl_format_ipcm_assign_to_dif_resp_msg(void)
{
	struct sk_buff * msg;
	struct rina_msg_hdr * hdr;

	if (test_begin_generic(msg,
	  		       hdr,
			       "RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE",
			       RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE) < 0)
		return -1;

	if (rnl_format_ipcm_assign_to_dif_resp_msg(10, msg)){
		LOG_ERR("Could not format message RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE...");
		nlmsg_free(msg);
		return -1;
	}

	test_end_generic(msg, hdr, "RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE");
	return 0;
}

int test_rnl_format_ipcm_ipcp_dif_reg_noti_msg(void)
{
	struct sk_buff * msg;
	struct rina_msg_hdr * hdr;
	struct name * ipc_name;
	struct name * dif_name;

	ipc_name = rkzalloc(sizeof(*ipc_name), GFP_KERNEL);
	dif_name = rkzalloc(sizeof(*dif_name), GFP_KERNEL);
	if(!ipc_name){
		LOG_ERR("Could not allocate ipc_name param");
		return -1;
	}
	if(!dif_name){
		LOG_ERR("Could not allocate dif_name param");
		return -1;
	}
	
	populate_generic_name("ipc_",ipc_name);
	populate_generic_name("dif_",dif_name);

	if (test_begin_generic(msg,
	  	hdr,
		"RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION",
		RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION) < 0)
		return -1;

	if (rnl_format_ipcm_ipcp_dif_reg_noti_msg(ipc_name,
						       dif_name,
						       false, 
						       msg)){
		LOG_ERR("Could not format message"
		"RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION...");
		nlmsg_free(msg);
		return -1;
	}

	test_end_generic(msg, hdr, "RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION");
	rkfree(ipc_name);
	rkfree(dif_name);
	return 0;

}

int test_rnl_format_ipcm_ipcp_dif_unreg_noti_msg(void)
{
	struct sk_buff * msg;
	struct rina_msg_hdr * hdr;

	if (test_begin_generic(msg,
	  	hdr,
		"RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION",
		RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION) < 0)
		return -1;

	if (rnl_format_ipcm_ipcp_dif_unreg_noti_msg(10,
						    msg)){
		LOG_ERR("Could not format message"
		"RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION...");
		nlmsg_free(msg);
		return -1;
	}

	test_end_generic(msg, hdr, "RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION");
	return 0;

}

int test_rnl_format_ipcm_enroll_to_dif_req_msg(void)
{
	struct sk_buff * msg;
	struct rina_msg_hdr * hdr;
	struct name * dif_name;

	dif_name = rkzalloc(sizeof(*dif_name), GFP_KERNEL);
	if(!dif_name){
		LOG_ERR("Could not allocate dif_name param");
		return -1;
	}
	
	populate_generic_name("dif_",dif_name);

	if (test_begin_generic(msg,
	  	hdr,
		"RINA_C_IPCM_ENROLL_TO_DIF_REQUEST",
		RINA_C_IPCM_ENROLL_TO_DIF_REQUEST) < 0)
		return -1;

	if (rnl_format_ipcm_enroll_to_dif_req_msg(dif_name,
						   msg)){
		LOG_ERR("Could not format message"
		"RINA_C_IPCM_ENROLL_TO_DIF_REQUEST...");
		nlmsg_free(msg);
		return -1;
	}

	test_end_generic(msg, hdr, "RINA_C_IPCM_ENROLL_TO_DIF_REQUEST");
	rkfree(dif_name);
	return 0;

}

int test_rnl_format_ipcm_enroll_to_dif_resp_msg(void)
{
	struct sk_buff * msg;
	struct rina_msg_hdr * hdr;

	if (test_begin_generic(msg,
	  	hdr,
		"RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE",
		RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE) < 0)
		return -1;

	if (rnl_format_ipcm_ipcp_dif_unreg_noti_msg(10,
						    msg)){
		LOG_ERR("Could not format message"
		"RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE...");
		nlmsg_free(msg);
		return -1;
	}

	test_end_generic(msg, hdr, "RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE");
	return 0;

}

void test_formatters(void){

	int i;

	LOG_DBG("Netlink formatting tests started...");

	//for (i=RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST; i< RINA_C_MAX; i++) {
	for (i=RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST; i< RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE; i++) {

		switch(i){
        	case RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST:
			test_rnl_format_ipcm_assign_to_dif_req_msg();
        	        break;
        	case RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE:
			test_rnl_format_ipcm_assign_to_dif_resp_msg();
        	        break;
        	case RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION:
			test_rnl_format_ipcm_ipcp_dif_reg_noti_msg();
        	        break;
        	case RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION:
			test_rnl_format_ipcm_ipcp_dif_unreg_noti_msg(); 
        	        break;
        	case RINA_C_IPCM_ENROLL_TO_DIF_REQUEST:
			test_rnl_format_ipcm_enroll_to_dif_req_msg();
        	        break;
        	case RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE:
			test_rnl_format_ipcm_enroll_to_dif_resp_msg();
        	        break;
#if 0
        	case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST:
			test_rnl_format_ipcm_disconn_neighbor_req_msg();
        	        break;
        	case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE:
			test_rnl_format_ipcm_disconn_neighbor_resp_msg();
        	        break;
        	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST:
        	        test_rnl_format_ipcm_alloc_flow_req_msg(); 
        	        break;
        	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED:
        	        test_rnl_format_ipcm_alloc_flow_req_arrived_msg(); 
        	        break;
        	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
        	        test_rnl_format_ipcm_alloc_flow_req_result_msg(); 
        	        break;
        	case RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE:
        	        test_rnl_format_ipcm_alloc_flow_resp_msg(); 
        	        break;
        	case RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST:
        	        test_rnl_format_ipcm_dealloc_flow_req_msg(); 
        	        break;
        	case RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION:
        	        test_rnl_format_ipcm_flow_dealloc_noti_msg(); 
        	        break;
        	case RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE:
        	        test_rnl_format_ipcm_dealloc_flow_resp_msg(); 
        	        break;
        	case RINA_C_IPCM_REGISTER_APPLICATION_REQUEST:
        	        test_rnl_format_ipcm_reg_app_req_msg(); 
        	        break;
        	case RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE:
        	        test_rnl_format_ipcm_reg_app_resp_msg(); 
        	        break;
        	case RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST:
        	        test_rnl_format_ipcm_unreg_app_req_msg(); 
        	        break;
        	case RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE:
        	        test_rnl_format_ipcm_unreg_app_resp_msg(); 
        	        break;
        	case RINA_C_IPCM_QUERY_RIB_REQUEST:
        	        test_rnl_format_ipcm_query_rib_req_msg(); 
        	        break;
        	case RINA_C_IPCM_QUERY_RIB_RESPONSE:
        	        test_rnl_format_ipcm_query_rib_resp_msg(); 
        	        break;
        	case RINA_C_RMT_ADD_FTE_REQUEST:
        	        test_rnl_format_rmt_add_fte_req_msg(); 
        	        break;
        	case RINA_C_RMT_DELETE_FTE_REQUEST:
        	        test_rnl_format_rmt_del_fte_req_msg(); 
        	        break;
        	case RINA_C_RMT_DUMP_FT_REQUEST:
        	        test_rnl_format_rmt_dump_ft_req_msg(); 
        	        break;
        	case RINA_C_RMT_DUMP_FT_REPLY:
        	        test_rnl_format_rmt_dump_ft_reply_msg(); 
        	        break;
#endif
        	default:
        	        LOG_ERR("UNKNOWN MESSAGE TO TEST FORMAT");
        	        break;
        	}
	}

	LOG_DBG("Netlink formatters testing ended");
}
EXPORT_SYMBOL(test_formatters);
