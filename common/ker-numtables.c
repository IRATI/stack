/*
 * Serialization tables for kernel control messages.
 *
 * Copyright (C) 2015-2016 Nextworks
 * Author: Vincenzo Maffione <v.maffione@gmail.com>
 *
 * This file is part of rlite.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "irati/serdes-utils.h"
#include "irati/kernel-msg.h"

struct irati_msg_layout irati_ker_numtables[] = {
    [RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_assign_to_dif)
		   - sizeof(char *) - sizeof(struct name *)
		   - sizeof(struct dif_config *),
	.names = 1,
        .strings = 1,
	.dif_configs = 1,
    },
    [RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_update_config)
		   - sizeof(struct dif_config *),
	.dif_configs = 1,
    },
    [RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_dif_reg_not)
	           - 2 * sizeof(struct name *),
	.names = 2,
    },
    [RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_dif_reg_not)
	           - 2 * sizeof(struct name *),
	.names = 2,
    },
    [RINA_C_IPCM_ALLOCATE_FLOW_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_allocate_flow) -
                    3 * sizeof(struct name *) - sizeof(struct flow_spec *),
        .names = 3,
	.flow_specs = 1,
    },
    [RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_allocate_flow) -
                    3 * sizeof(struct name *) - sizeof(struct flow_spec *),
        .names = 3,
	.flow_specs = 1,
    },
    [RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_allocate_flow_resp),
    },
    [RINA_C_IPCM_REGISTER_APPLICATION_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_reg_app) -
                    3 * sizeof(struct name *),
        .names = 3,
    },
    [RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_unreg_app) -
                    2 * sizeof(struct name *),
        .names = 2,
    },
    [RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCM_QUERY_RIB_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_query_rib)
		   - 3 * sizeof(char *),
        .strings = 3,
    },
    [RINA_C_IPCM_QUERY_RIB_RESPONSE] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_query_rib_resp) -
                    sizeof(struct query_rib_resp *),
        .query_rib_resps = 1,
    },
    [RINA_C_RMT_DUMP_FT_REQUEST] = {
        .copylen = sizeof(struct irati_msg_base),
    },
    [RINA_C_RMT_MODIFY_FTE_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_rmt_dump_ft)
	           - sizeof(struct pff_entry_list *),
	.pff_entry_lists = 1,
    },
    [RINA_C_RMT_DUMP_FT_REPLY] = {
        .copylen = sizeof(struct irati_kmsg_rmt_dump_ft)
	           - sizeof(struct pff_entry_list *),
	.pff_entry_lists = 1,
    },
    [RINA_C_IPCP_CONN_CREATE_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_conn_create_arrived)
		   - sizeof(struct dtp_config *) - sizeof(struct dtcp_config *),
	.dtp_configs = 1,
	.dtcp_configs = 1,
    },
    [RINA_C_IPCP_CONN_CREATE_ARRIVED] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_conn_create_arrived)
		   - sizeof(struct dtp_config *) - sizeof(struct dtcp_config *),
	.dtp_configs = 1,
	.dtcp_configs = 1,
    },
    [RINA_C_IPCP_CONN_CREATE_RESPONSE] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_conn_update),
    },
    [RINA_C_IPCP_CONN_CREATE_RESULT] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_conn_update),
    },
    [RINA_C_IPCP_CONN_UPDATE_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_conn_update),
    },
    [RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_select_ps_param)
		   - 3 * sizeof(char *),
	.strings = 3,
    },
    [RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCP_SELECT_POLICY_SET_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_select_ps)
		   - 2 * sizeof(char *),
	.strings = 2,
    },
    [RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCP_UPDATE_CRYPTO_STATE_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_update_crypto_state)
	           - sizeof(struct sdup_crypto_state *),
	.sdup_crypto_states = 1,
    },
    [RINA_C_IPCP_ADDRESS_CHANGE_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_address_change),
    },
    [RINA_C_IPCP_ALLOCATE_PORT_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_allocate_port)
	           - sizeof(struct name *),
	.names = 1,
    },
    [RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT] = {
        .copylen = sizeof(struct irati_kmsg_multi_msg),
    },
    [RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_multi_msg),
    },
    [RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION] = {
        .copylen = sizeof(struct irati_kmsg_multi_msg),
    },
    [RINA_C_IPCP_CONN_UPDATE_RESULT] = {
        .copylen = sizeof(struct irati_kmsg_multi_msg),
    },
    [RINA_C_IPCP_CONN_DESTROY_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_multi_msg),
    },
    [RINA_C_IPCP_CONN_DESTROY_RESULT] = {
        .copylen = sizeof(struct irati_kmsg_multi_msg),
    },
    [RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE] = {
        .copylen = sizeof(struct irati_kmsg_multi_msg),
    },
    [RINA_C_IPCP_ALLOCATE_PORT_RESPONSE] = {
        .copylen = sizeof(struct irati_kmsg_multi_msg),
    },
    [RINA_C_IPCP_DEALLOCATE_PORT_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_multi_msg),
    },
    [RINA_C_IPCP_DEALLOCATE_PORT_RESPONSE] = {
        .copylen = sizeof(struct irati_kmsg_multi_msg),
    },
    [RINA_C_IPCP_MANAGEMENT_SDU_WRITE_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_mgmt_sdu) -
		   sizeof(struct buffer *),
	.buffers = 1,
    },
    [RINA_C_IPCP_MANAGEMENT_SDU_READ_NOTIF] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_mgmt_sdu) -
		   sizeof(struct buffer *),
	.buffers = 1,
    },
    [RINA_C_IPCP_MANAGEMENT_SDU_WRITE_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCM_CREATE_IPCP_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_create_ipcp)
	           - sizeof(struct name *) - sizeof(char *),
	.names = 1,
	.strings = 1,
    },
    [RINA_C_IPCM_CREATE_IPCP_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCM_DESTROY_IPCP_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_destroy_ipcp),
    },
    [RINA_C_IPCM_DESTROY_IPCP_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCM_ENROLL_TO_DIF_REQUEST] = {
        .copylen = sizeof(struct irati_msg_ipcm_enroll_to_dif)
		  - 4 * sizeof(struct name *),
	.names = 4,
    },
    [RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_ipcm_enroll_to_dif_resp)
		   - sizeof(struct name *) - sizeof(char *)
		   - sizeof(struct dif_config *) - sizeof(struct ipcp_neigh_list *),
	.names = 1,
	.strings = 1,
	.dif_configs = 1,
	.ipcp_neigh_lists = 1,
    },
    [RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST] = {
        .copylen = sizeof(struct irati_msg_with_name)
		   - sizeof(struct name *),
	.names = 1,
    },
    [RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCM_IPC_PROCESS_INITIALIZED] = {
        .copylen = sizeof(struct irati_msg_with_name)
		   - sizeof(struct name *),
	.names = 1,
    },
    [RINA_C_APP_ALLOCATE_FLOW_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcm_allocate_flow)
		   - 3 * sizeof(struct name *) - sizeof(struct flow_spec *),
	.names = 3,
	.flow_specs = 1,
    },
    [RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT] = {
        .copylen = sizeof(struct irati_msg_app_alloc_flow_result)
		   - 2 * sizeof(struct name *) - sizeof (char *),
	.names = 2,
	.strings = 1,
    },
    [RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED] = {
	.copylen = sizeof(struct irati_kmsg_ipcm_allocate_flow)
		   - 3 * sizeof(struct name *) - sizeof(struct flow_spec *),
	.names = 3,
	.flow_specs = 1,
    },
    [RINA_C_APP_ALLOCATE_FLOW_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_app_alloc_flow_response),
    },
    [RINA_C_APP_DEALLOCATE_FLOW_REQUEST] = {
        .copylen = sizeof(struct irati_msg_app_dealloc_flow),
    },
    [RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION] = {
	.copylen = sizeof(struct irati_msg_app_dealloc_flow),
    },
    [RINA_C_APP_REGISTER_APPLICATION_REQUEST] = {
        .copylen = sizeof(struct irati_msg_app_reg_app)
		   - 3 * sizeof(struct name *),
	.names = 3,
    },
    [RINA_C_APP_REGISTER_APPLICATION_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_app_reg_app_resp)
		   - 2 * sizeof(struct name *),
	.names = 2,
    },
    [RINA_C_APP_UNREGISTER_APPLICATION_REQUEST] = {
	.copylen = sizeof(struct irati_msg_app_reg_app_resp)
		   - 2 * sizeof(struct name *),
	.names = 2,
    },
    [RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE] = {
	.copylen = sizeof(struct irati_msg_app_reg_app_resp)
		   - 2 * sizeof(struct name *),
	.names = 2,
    },
    [RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION] = {
        .copylen = sizeof(struct irati_msg_app_reg_cancel)
		   - 2 * sizeof(struct name *) - sizeof(char *),
	.names = 2,
	.strings = 1,
    },
    [RINA_C_APP_GET_DIF_PROPERTIES_REQUEST] = {
	.copylen = sizeof(struct irati_msg_app_reg_app_resp)
		   - 2 * sizeof(struct name *),
	.names = 2,
    },
    [RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_get_dif_prop)
		   - 2 * sizeof(struct name *) - sizeof(struct get_dif_prop_resp *),
	.names = 2,
	.dif_properties = 1,
    },
    [RINA_C_IPCM_PLUGIN_LOAD_REQUEST] = {
        .copylen = sizeof(struct irati_msg_ipcm_plugin_load)
		   - sizeof(char *),
	.strings = 1,
    },
    [RINA_C_IPCM_PLUGIN_LOAD_RESPONSE] = {
        .copylen = sizeof(struct irati_msg_base_resp),
    },
    [RINA_C_IPCM_FWD_CDAP_MSG_REQUEST] = {
        .copylen = sizeof(struct irati_msg_ipcm_fwd_cdap_msg)
		   - sizeof(struct buffer *),
	.buffers = 1,
    },
    [RINA_C_IPCM_FWD_CDAP_MSG_RESPONSE] = {
	.copylen = sizeof(struct irati_msg_ipcm_fwd_cdap_msg)
		   - sizeof(struct buffer *),
	.buffers = 1,
    },
    [RINA_C_IPCM_MEDIA_REPORT] = {
        .copylen = sizeof(struct irati_msg_ipcm_media_report)
		   - sizeof(struct media_report *),
	.media_reports = 1,
    },
    [RINA_C_IPCM_FINALIZE_REQUEST] = {
        .copylen = sizeof(struct irati_msg_base),
    },
    [RINA_C_IPCP_CONN_MODIFY_REQUEST] = {
        .copylen = sizeof(struct irati_kmsg_ipcp_conn_update),
    },
    [RINA_C_IPCM_SCAN_MEDIA_REQUEST] = {
        .copylen = sizeof(struct irati_msg_base),
    },
    [RINA_C_MAX] = {
        .copylen = 0,
        .names = 0,
        .strings = 0,
	.dif_configs = 0,
	.dtp_configs = 0,
	.dtcp_configs = 0,
	.pff_entry_lists = 0,
	.query_rib_resps = 0,
	.sdup_crypto_states = 0,
	.dif_properties = 0,
	.ipcp_neigh_lists = 0,
	.media_reports = 0,
	.buffers = 0,
    },
};
