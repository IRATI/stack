/*
 * RNL utilities
 *
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_RNL_UTILS_H
#define RINA_RNL_UTILS_H

#include "connection.h"

/*
 * FIXME:
 *   These definitions must be moved in the user-space exported headers or
 *   hidden completely, part of them are used internally, msg and msg_attrs
 *   structs no ...
 */

enum app_name_info_attrs_list {
        APNI_ATTR_PROCESS_NAME = 1,
        APNI_ATTR_PROCESS_INSTANCE,
        APNI_ATTR_ENTITY_NAME,
        APNI_ATTR_ENTITY_INSTANCE,
        __APNI_ATTR_MAX,
};
#define APNI_ATTR_MAX (__APNI_ATTR_MAX - 1)

enum port_id_altlist_attrs_list {
        PIA_ATTR_PORTIDS = 1,
        __PIA_ATTR_MAX,
};
#define PIA_ATTR_MAX (__PIA_ATTR_MAX - 1)

enum mod_pff_entry_attrs_list {
        PFFELE_ATTR_ADDRESS = 1,
        PFFELE_ATTR_QOSID,
        PFFELE_ATTR_PORT_ID_ALTLISTS,
        __PFFELE_ATTR_MAX,
};
#define PFFELE_ATTR_MAX (__PFFELE_ATTR_MAX - 1)

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

enum ipcp_config_entry_attrs_list {
        IPCP_CONFIG_ENTRY_ATTR_NAME = 1,
        IPCP_CONFIG_ENTRY_ATTR_VALUE,
        __IPCP_CONFIG_ENTRY_ATTR_MAX,
};
#define IPCP_CONFIG_ENTRY_ATTR_MAX (__IPCP_CONFIG_ENTRY_ATTR_MAX - 1)

enum policy_param_attrs_list {
        PPA_ATTR_NAME = 1,
        PPA_ATTR_VALUE,
        __PPA_ATTR_MAX,
};
#define PPA_ATTR_MAX (__PPA_ATTR_MAX - 1)

enum policy_attrs_list {
        PA_ATTR_NAME = 1,
        PA_ATTR_VERSION,
        PA_ATTR_PARAMETERS,
        __PA_ATTR_MAX,
};
#define PA_ATTR_MAX (__PA_ATTR_MAX - 1)

enum dtcp_wb_fctrl_config_attrs_list {
        DWFCC_ATTR_MAX_CLOSED_WINDOW_Q_LENGTH = 1,
        DWFCC_ATTR_INITIAL_CREDIT,
        DWFCC_ATTR_RCVR_FLOW_CTRL_POLICY,
        DWFCC_ATTR_TX_CTRL_POLICY,
        __DWFCC_ATTR_MAX,
};
#define DWFCC_ATTR_MAX (__DWFCC_ATTR_MAX -1)

enum dtcp_rb_fctrl_config_attrs_list {
        DRFCC_ATTR_SEND_RATE = 1,
        DRFCC_ATTR_TIME_PERIOD,
        DRFCC_ATTR_NO_RATE_SDOWN_POLICY,
        DRFCC_ATTR_NO_OVERR_DEF_PEAK_POLICY,
        DRFCC_ATTR_RATE_REDUC_POLICY,
        __DRFCC_ATTR_MAX,
};
#define DRFCC_ATTR_MAX (__DRFCC_ATTR_MAX -1)

enum dtcp_fctrl_config_attrs_lists {
        DFCC_ATTR_WINDOW_BASED = 1,
        DFCC_ATTR_WINDOW_BASED_CONFIG,
        DFCC_ATTR_RATE_BASED,
        DFCC_ATTR_RATE_BASED_CONFIG,
        DFCC_ATTR_SBYTES_THRES,
        DFCC_ATTR_SBYTES_PER_THRES,
        DFCC_ATTR_SBUFFER_THRES,
        DFCC_ATTR_RBYTES_THRES,
        DFCC_ATTR_RBYTES_PER_THRES,
        DFCC_ATTR_RBUFFER_THRES,
        DFCC_ATTR_CLOSED_WINDOW_POLICY,
        DFCC_ATTR_RECON_FLOW_CTRL_POLICY,
        DFCC_ATTR_RCVING_FLOW_CTRL_POLICY,
        __DFCC_ATTR_MAX,
};
#define DFCC_ATTR_MAX (__DFCC_ATTR_MAX -1)

enum dtcp_rctrl_config_attrs_list {
        DRCC_ATTR_MAX_TIME_TO_RETRY = 1,
        DRCC_ATTR_DATA_RXMSN_MAX,
        DRCC_ATTR_INIT_TR,
        DRCC_ATTR_RTX_TIME_EXP_POLICY,
        DRCC_ATTR_SACK_POLICY,
        DRCC_ATTR_RACK_LIST_POLICY,
        DRCC_ATTR_RACK_POLICY,
        DRCC_ATTR_SDING_ACK_POLICY,
        DRCC_ATTR_RCONTROL_ACK_POLICY,
        __DRCC_ATTR_MAX,
};
#define DRCC_ATTR_MAX (__DRCC_ATTR_MAX -1)

enum dtcp_config_params_attrs_list {
        DCA_ATTR_FLOW_CONTROL = 1,
        DCA_ATTR_FLOW_CONTROL_CONFIG,
        DCA_ATTR_RETX_CONTROL,
        DCA_ATTR_RETX_CONTROL_CONFIG,
        DCA_ATTR_DTCP_POLICY_SET,
        DCA_ATTR_LOST_CONTROL_PDU_POLICY,
        DCA_ATTR_RTT_EST_POLICY,
        __DCA_ATTR_MAX,
};
#define DCA_ATTR_MAX (__DCA_ATTR_MAX - 1)

enum dtp_config_params_attrs_list {
        DTPCA_ATTR_DTCP_PRESENT = 1,
        DTPCA_ATTR_SEQ_NUM_ROLLOVER,
        DTPCA_ATTR_INIT_A_TIMER,
        DTPCA_ATTR_PARTIAL_DELIVERY,
        DTPCA_ATTR_INCOMPLETE_DELIVERY,
        DTPCA_ATTR_IN_ORDER_DELIVERY,
        DTPCA_ATTR_MAX_SDU_GAP,
        DTPCA_ATTR_DTP_POLICY_SET,
        __DTPCA_ATTR_MAX,
};
#define DTPCA_ATTR_MAX (__DTPCA_ATTR_MAX - 1)

/*enum conn_policies_params_attrs_list {
        CPP_ATTR_DTCP_PRESENT = 1,
        CPP_ATTR_DTCP_CONFIG,
        CPP_ATTR_DTP_POLICY_SET,
        CPP_ATTR_RCVR_TIMER_INAC_POLICY,
        CPP_ATTR_SNDR_TIMER_INAC_POLICY,
        CPP_ATTR_INIT_SEQ_NUM_POLICY,
        CPP_ATTR_SEQ_NUM_ROLLOVER,
        CPP_ATTR_INIT_A_TIMER,
        CPP_ATTR_PARTIAL_DELIVERY,
        CPP_ATTR_INCOMPLETE_DELIVERY,
        CPP_ATTR_IN_ORDER_DELIVERY,
        CPP_ATTR_MAX_SDU_GAP,
        __CPP_ATTR_MAX,
};
*/

/* FIXME: in user space these are called without _NAME */
enum ipcm_alloc_flow_req_attrs_list {
        IAFRM_ATTR_SOURCE_APP_NAME = 1,
        IAFRM_ATTR_DEST_APP_NAME,
        IAFRM_ATTR_FLOW_SPEC,
        IAFRM_ATTR_DIF_NAME,
        __IAFRM_ATTR_MAX,
};
#define IAFRM_ATTR_MAX (__IAFRM_ATTR_MAX -1)

enum ipcm_alloc_flow_req_arrived_attrs_list {
        IAFRA_ATTR_SOURCE_APP_NAME = 1,
        IAFRA_ATTR_DEST_APP_NAME,
        IAFRA_ATTR_FLOW_SPEC,
        IAFRA_ATTR_DIF_NAME,
        IAFRA_ATTR_PORT_ID,
        __IAFRA_ATTR_MAX,
};
#define IAFRA_ATTR_MAX (__IAFRA_ATTR_MAX -1)

enum ipcm_alloc_flow_resp_attrs_list {
        IAFRE_ATTR_RESULT = 1,
        IAFRE_ATTR_NOTIFY_SOURCE,
        __IAFRE_ATTR_MAX,
};
#define IAFRE_ATTR_MAX (__IAFRE_ATTR_MAX -1)

/*
 * FIXME: Need to specify the possible values of result to map with deny
 * reasons strings in US
 */
#define ALLOC_RESP_DENY_REASON_1 "FAILED"

enum ipcm_alloc_flow_req_result_attrs_list {
        IAFRRM_ATTR_RESULT = 1,
        IAFRRM_ATTR_PORT_ID,
        __IAFRRM_ATTR_MAX,
};
#define IAFRRM_ATTR_MAX (__IAFRRM_ATTR_MAX -1)

/*
 * FIXME: Need to specify the possible values of result to map with error
 * descriptions strings in US
 */
#define ALLOC_RESP_ERR_DESC_1 "FAILED"

enum ipcm_dealloc_flow_req_attrs_list {
        IDFRT_ATTR_PORT_ID = 1,
        __IDFRT_ATTR_MAX,
};
#define IDFRT_ATTR_MAX (__IDFRT_ATTR_MAX -1)

enum ipcm_dealloc_flow_resp_attrs_list {
        IDFRE_ATTR_RESULT = 1,
        __IDFRE_ATTR_MAX,
};
#define IDFRE_ATTR_MAX (__IDFRE_ATTR_MAX -1)

/*
 * FIXME: Need to specify the possible values of result to map with error
 * descriptions strings in US
 */
#define DEALLOC_RESP_ERR_DESC_1 "FAILED"

enum ipcm_flow_dealloc_noti_attrs_list {
        IFDN_ATTR_PORT_ID = 1,
        IFDN_ATTR_CODE,
        __IFDN_ATTR_MAX,
};
#define IFDN_ATTR_MAX (__IFDN_ATTR_MAX -1)

enum ipcm_conn_create_req_attrs_list {
        ICCRQ_ATTR_PORT_ID = 1,
        ICCRQ_ATTR_SOURCE_ADDR,
        ICCRQ_ATTR_DEST_ADDR,
        ICCRQ_ATTR_QOS_ID,
        ICCRQ_ATTR_DTP_CONFIG,
        ICCRQ_ATTR_DTCP_CONFIG,
        __ICCRQ_ATTR_MAX,
};
#define ICCRQ_ATTR_MAX (__ICCRQ_ATTR_MAX - 1)

enum ipcm_conn_create_resp_attrs_list {
        ICCRE_ATTR_PORT_ID = 1,
        ICCRE_ATTR_SOURCE_CEP_ID,
        __ICCRE_ATTR_MAX,
};
#define ICCRE_ATTR_MAX (__ICCRE_ATTR_MAX - 1)

enum ipcm_conn_create_arrived_attrs_list {
        ICCA_ATTR_PORT_ID = 1,
        ICCA_ATTR_SOURCE_ADDR,
        ICCA_ATTR_DEST_ADDR,
        ICCA_ATTR_DEST_CEP_ID,
        ICCA_ATTR_QOS_ID,
        ICCA_ATTR_FLOW_USER_IPCP_ID,
        ICCA_ATTR_DTP_CONFIG,
        ICCA_ATTR_DTCP_CONFIG,
        __ICCA_ATTR_MAX,
};
#define ICCA_ATTR_MAX (__ICCA_ATTR_MAX - 1)

enum ipcm_conn_create_result_attrs_list {
        ICCRS_ATTR_PORT_ID = 1,
        ICCRS_ATTR_SOURCE_CEP_ID,
        ICCRS_ATTR_DEST_CEP_ID,
        __ICCRS_ATTR_MAX,
};
#define ICCRS_ATTR_MAX (__ICCRS_ATTR_MAX - 1)

enum ipcm_conn_update_req_attrs_list {
        ICURQ_ATTR_PORT_ID = 1,
        ICURQ_ATTR_SOURCE_CEP_ID,
        ICURQ_ATTR_DEST_CEP_ID,
        ICURQ_ATTR_FLOW_USER_IPCP_ID,
        __ICURQ_ATTR_MAX,
};
#define ICURQ_ATTR_MAX (__ICURQ_ATTR_MAX - 1)

enum ipcm_conn_update_result_attrs_list {
        ICURS_ATTR_PORT_ID = 1,
        ICURS_ATTR_RESULT,
        __ICURS_ATTR_MAX,
};
#define ICURS_ATTR_MAX (__ICURS_ATTR_MAX - 1)

enum ipcm_conn_destroy_req_attrs_list {
        ICDR_ATTR_PORT_ID = 1,
        ICDR_ATTR_SOURCE_CEP_ID,
        __ICDR_ATTR_MAX,
};
#define ICDR_ATTR_MAX (__ICDR_ATTR_MAX - 1)

enum ipcm_conn_destroy_result_attrs_list {
        ICDRS_ATTR_PORT_ID = 1,
        ICDRS_ATTR_RESULT,
        __ICDRS_ATTR_MAX,
};
#define ICDRS_ATTR_MAX (__ICDRS_ATTR_MAX - 1)

enum ipcm_reg_app_req_attrs_list {
        IRAR_ATTR_APP_NAME = 1,
        IRAR_ATTR_DIF_NAME,
        IRAR_ATTR_REG_IPCP_ID,
        __IRAR_ATTR_MAX,
};
#define IRAR_ATTR_MAX (__IRAR_ATTR_MAX -1)

enum ipcm_reg_app_resp_attrs_list {
        IRARE_ATTR_RESULT=1,
        __IRARE_ATTR_MAX,
};
#define IRARE_ATTR_MAX (__IRARE_ATTR_MAX -1)

/*
 * FIXME: Need to specify the possible values of result to map with error
 * descriptions strings in US
 */
#define REG_APP_RESP_ERR_DESC_1 "FAILED"

enum ipcm_unreg_app_req_attrs_list {
        IUAR_ATTR_APP_NAME = 1,
        IUAR_ATTR_DIF_NAME,
        __IUAR_ATTR_MAX,
};
#define IUAR_ATTR_MAX (__IUAR_ATTR_MAX -1)

enum ipcm_unreg_app_resp_attrs_list {
        IUARE_ATTR_RESULT = 1,
        __IUARE_ATTR_MAX,
};
#define IUARE_ATTR_MAX (__IUARE_ATTR_MAX -1)

/*
 * FIXME: Need to specify the possible values of result to map with error
 * descriptions strings in US
 */
#define UNREG_APP_RESP_ERR_DESC_1 "FAILED"

enum ipcm_query_rib_req_attrs_list {
	IDQR_ATTR_OBJECT_CLASS = 1,
	IDQR_ATTR_OBJECT_NAME,
	IDQR_ATTR_OBJECT_INSTANCE,
	IDQR_ATTR_SCOPE,
	IDQR_ATTR_FILTER,
        __IDQR_ATTR_MAX,
};
#define IDQR_ATTR_MAX (__IDQR_ATTR_MAX -1)

enum rib_object_attrs_list {
	RIBO_ATTR_OBJECT_CLASS = 1,
	RIBO_ATTR_OBJECT_NAME,
	RIBO_ATTR_OBJECT_INSTANCE,
	RIBO_ATTR_OBJECT_DISPLAY_VALUE,
	__RIBO_ATTR_MAX,
};
#define RIBO_ATTR_MAX (__RIBO_ATTR_MAX -1)

enum ipcm_query_rib_resp_attrs_list {
	IDQRE_ATTR_RESULT = 1,
	IDQRE_ATTR_RIB_OBJECTS,
	__IDQRE_ATTR_MAX,
};
#define IDQRE_ATTR_MAX (__IDQRE_ATTR_MAX -1)

enum ipcm_assign_to_dif_req_attrs_list {
        IATDR_ATTR_DIF_INFORMATION = 1,
        __IATDR_ATTR_MAX,
};
#define IATDR_ATTR_MAX (__IATDR_ATTR_MAX -1)

enum ipcm_update_dif_config_req_attrs_list {
        IUDCR_ATTR_DIF_CONFIGURATION = 1,
        __IUDCR_ATTR_MAX,
};
#define IUDCR_ATTR_MAX (__IUDCR_ATTR_MAX -1)

enum dif_info_attrs_list {
        DINFO_ATTR_DIF_TYPE = 1,
        DINFO_ATTR_DIF_NAME,
        DINFO_ATTR_CONFIG,
        __DINFO_ATTR_MAX,
};
#define DINFO_ATTR_MAX (__DINFO_ATTR_MAX -1)

enum data_transfer_cons_attrs_list {
        DTC_ATTR_QOS_ID = 1,
        DTC_ATTR_PORT_ID,
        DTC_ATTR_CEP_ID,
        DTC_ATTR_SEQ_NUM,
        DTC_ATTR_CTRL_SEQ_NUM,
        DTC_ATTR_ADDRESS,
        DTC_ATTR_LENGTH,
        DTC_ATTR_MAX_PDU_SIZE,
        DTC_ATTR_MAX_PDU_LIFE,
        DTC_ATTR_RATE,
        DTC_ATTR_FRAME,
        DTC_ATTR_DIF_INTEGRITY,
        __DTC_ATTR_MAX,
};
#define DTC_ATTR_MAX (__DTC_ATTR_MAX -1)

enum efcp_config_attrs_list {
        EFCPC_ATTR_DATA_TRANS_CONS = 1,
        EFCPC_ATTR_QOS_CUBES,
        EFCPC_ATTR_UNKNOWN_FLOW_POLICY,
        __EFCPC_ATTR_MAX,
};
#define EFCPC_ATTR_MAX (__EFCPC_ATTR_MAX -1)

enum dup_config_attrs_list {
	AUTHP_AUTH_POLICY = 1,
	AUTHP_ENCRYPT_POLICY,
	AUTHP_TTL_POLICY,
	AUTHP_ERROR_CHECK_POLICY,
	__AUTHP_ATTR_MAX,
};
#define AUTHP_ATTR_MAX (__AUTHP_ATTR_MAX -1)

enum spec_sdup_config_attrs_list {
    SAUTHP_UNDER_DIF = 1,
    SAUTHP_AUTH_PROFILE,
    __SAUTHP_ATTR_MAX,
};
#define SAUTHP_ATTR_MAX (__SAUTHP_ATTR_MAX -1)

enum secman_config_attrs_list {
	SECMANC_POLICY_SET = 1,
	SECMANC_DEFAULT_AUTH_SDUP_POLICY,
	SECMANC_SPECIFIC_AUTH_SDUP_POLICIES,
	__SECMANC_ATTR_MAX,
};
#define SECMANC_ATTR_MAX (__SECMANC_ATTR_MAX -1)

enum pff_config_attrs_list {
        PFFC_ATTR_POLICY_SET = 1,
        __PFFC_ATTR_MAX,
};
#define PFFC_ATTR_MAX (__PFFC_ATTR_MAX -1)

enum rmt_config_attrs_list {
        RMTC_ATTR_POLICY_SET = 1,
        RMTC_ATTR_PFF_CONFIG,
        __RMTC_ATTR_MAX,
};
#define RMTC_ATTR_MAX (__RMTC_ATTR_MAX -1)

enum dif_config_attrs_list {
        DCONF_ATTR_IPCP_CONFIG_ENTRIES = 1,
        DCONF_ATTR_ADDRESS,
        DCONF_ATTR_EFCPC,
        DCONF_ATTR_RMTC,
        DCONF_ATTR_SECMANC,
        /* From here not used in kernel */
	DCONF_ATTR_FAC,
	DCONF_ATTR_ETC,
	DCONF_ATTR_NSMC,
	DCONF_ATTR_RAC,
	DCONF_ATTR_ROUTINGC,
        __DCONF_ATTR_MAX,
};
#define DCONF_ATTR_MAX (__DCONF_ATTR_MAX -1)

enum ipcm_assign_to_dif_resp_attrs_list {
        IATDRE_ATTR_RESULT = 1,
        __IATDRE_ATTR_MAX,
};
#define IATDRE_ATTR_MAX (__IATDRE_ATTR_MAX -1)

/*
 * FIXME: Need to specify the possible values of result to map with error
 * descriptions strings in US
 */
#define ASSIGN_TO_DIF_RESP_ERR_DESC_1 "FAILED"

enum ipcm_ipcp_dif_reg_noti_attrs_list {
        IDRN_ATTR_IPC_PROCESS_NAME = 1,
        IDRN_ATTR_DIF_NAME,
        IDRN_ATTR_REGISTRATION,
        __IDRN_ATTR_MAX,
};
#define IDRN_ATTR_MAX (__IDRN_ATTR_MAX -1)

/* FIXME: It does not exist in user space */
enum ipcm_ipcp_dif_unreg_noti_attrs_list {
        IDUN_ATTR_RESULT = 1,
        __IDUN_ATTR_MAX,
};
#define IDUN_ATTR_MAX (__IDUN_ATTR_MAX -1)

enum ipcm_ipcp_enroll_to_dif_req_msg_attr_list {
        IEDR_ATTR_DIF_NAME = 1,
        __IEDR_ATTR_MAX,
};
#define IEDR_ATTR_MAX (__IEDR_ATTR_MAX -1)

enum ipcm_ipcp_enroll_to_dif_resp_msg_attr_list {
        IEDRE_ATTR_RESULT = 1,
        __IEDRE_ATTR_MAX,
};
#define IEDRE_ATTR_MAX (__IEDRE_ATTR_MAX -1)

enum ipcm_disconn_neighbor_req_msg_attr_list {
        IDNR_ATTR_NEIGHBOR_NAME = 1,
        __IDNR_ATTR_MAX,
};
#define IDNR_ATTR_MAX (__IDNR_ATTR_MAX -1)

enum ipcm_disconn_neighbor_resp_msg_attr_list {
        IDNRE_ATTR_RESULT = 1,
        __IDNRE_ATTR_MAX,
};
#define IDNRE_ATTR_MAX (__IDNRE_ATTR_MAX -1)

enum socket_closed_notification_msg_attr_list {
        ISCN_ATTR_PORT = 1,
        __ISCN_ATTR_MAX,
};
#define ISCN_ATTR_MAX (__ISCN_ATTR_MAX -1)

enum rmt_mod_pdu_fte_entry_req {
        RMPFE_ATTR_ENTRIES = 1,
        RMPFE_ATTR_MODE,
        __RMPFE_ATTR_MAX,
};
#define RMPFE_ATTR_MAX (__RMPFE_ATTR_MAX -1)

enum rmt_pff_dump_resp {
        RPFD_ATTR_RESULT = 1,
        RPFD_ATTR_ENTRIES,
        __RPFD_ATTR_MAX,
};
#define RPFD_ATTR_MAX (__RPFD_ATTR_MAX -1)

enum ipcp_set_policy_set_param_req_attrs_list {
        ISPSP_ATTR_PATH = 1,
        ISPSP_ATTR_NAME,
        ISPSP_ATTR_VALUE,
        __ISPSP_ATTR_MAX,
};
#define ISPSP_ATTR_MAX (__ISPSP_ATTR_MAX -1)

enum ipcp_select_policy_set_req_attrs_list {
        ISPS_ATTR_PATH = 1,
        ISPS_ATTR_NAME,
        __ISPS_ATTR_MAX,
};
#define ISPS_ATTR_MAX (__ISPS_ATTR_MAX -1)

enum ipcm_set_policy_set_param_req_result_attrs_list {
        ISPSPR_ATTR_RESULT = 1,
        __ISPSPR_ATTR_MAX,
};
#define ISPSPR_ATTR_MAX (__ISPSPR_ATTR_MAX -1)

enum ipcm_select_policy_set_req_result_attrs_list {
        ISPSR_ATTR_RESULT = 1,
        __ISPSR_ATTR_MAX,
};
#define ISPSR_ATTR_MAX (__ISPSR_ATTR_MAX -1)

enum ipcp_update_crypto_state_req_attrs_list {
	IUCSR_ATTR_N_1_PORT = 1,
	IUCSR_ATTR_CRYPT_STATE,
        __IUCSR_ATTR_MAX,
};
#define IUCSR_ATTR_MAX (__IUCSR_ATTR_MAX -1)

enum ipcp_crypto_state_attrs_list {
	ICSTATE_ENABLE_CRYPTO_TX = 1,
	ICSTATE_ENABLE_CRYPTO_RX,
	ICSTATE_MAC_KEY_TX,
	ICSTATE_MAC_KEY_RX,
	ICSTATE_ENCRYPT_KEY_TX,
	ICSTATE_ENCRYPT_KEY_RX,
	ICSTATE_IV_TX,
	ICSTATE_IV_RX,
        __ICSTATE_ATTR_MAX,
};
#define ICSTATE_ATTR_MAX (__ICSTATE_ATTR_MAX -1)

enum ipcp_update_crypto_state_resp_attrs_list {
        IUCSRE_ATTR_RESULT = 1,
        IUCSRE_ATTR_N_1_PORT,
        __IUCSRE_ATTR_MAX,
};
#define IUCSRE_ATTR_MAX (__IUCSRE_ATTR_MAX -1)

/* FIXME: Should be hidden by the API !!! */
struct rina_msg_hdr {
        unsigned short src_ipc_id;
        unsigned short dst_ipc_id;
};

enum rnl_msg_attr_type {
        RNL_MSG_ATTRS_ALLOCATE_FLOW_REQUEST,
        RNL_MSG_ATTRS_ALLOCATE_FLOW_RESPONSE,
        RNL_MSG_ATTRS_DEALLOCATE_FLOW_REQUEST,
        RNL_MSG_ATTRS_ASSIGN_TO_DIF_REQUEST,
        RNL_MSG_ATTRS_UPDATE_DIF_CONFIG_REQUEST,
        RNL_MSG_ATTRS_REG_UNREG_REQUEST,
        RNL_MSG_ATTRS_CONN_CREATE_REQUEST,
        RNL_MSG_ATTRS_CONN_CREATE_ARRIVED,
        RNL_MSG_ATTRS_CONN_UPDATE_REQUEST,
        RNL_MSG_ATTRS_CONN_DESTROY_REQUEST,
        RNL_MSG_ATTRS_RMT_PFFE_MODIFY_REQUEST,
        RNL_MSG_ATTRS_RMT_PFF_DUMP_REQUEST,
        RNL_MSG_ATTRS_QUERY_RIB_REQUEST,
        RNL_MSG_ATTRS_SET_POLICY_SET_PARAM_REQUEST,
        RNL_MSG_ATTRS_SELECT_POLICY_SET_REQUEST,
        RNL_MSG_ATTRS_UPDATE_CRYPTO_STATE_REQUEST
};

struct rnl_msg {
        /* Generic RINA Netlink family identifier */
        int                    family;

        /* source nl port id */
        unsigned int           src_port;

        /* destination nl port id */
        unsigned int           dst_port;

        /* The message sequence number */
        rnl_sn_t               seq_num;

        /* The operation code */
        msg_type_t             op_code;

        /* True if this is a request message */
        bool                   req_msg_flag;

        /* True if this is a response message */
        bool                   resp_msg_flag;

        /* True if this is a notification message */
        bool                   notification_msg_flag;

        /* RINA header containing IPC processes ids */
        struct rina_msg_hdr    header;

        enum rnl_msg_attr_type attr_type;

        /* Specific message attributes */
        void *                 attrs;
};

struct rnl_msg * rnl_msg_create(enum rnl_msg_attr_type type);
int              rnl_msg_destroy(struct rnl_msg * msg);

struct rnl_ipcm_assign_to_dif_req_msg_attrs {
        struct dif_info * dif_info;
};

struct rnl_ipcm_update_dif_config_req_msg_attrs {
        struct dif_config * dif_config;
};

struct rnl_ipcm_assign_to_dif_resp_msg_attrs {
        uint_t result;
};

struct rnl_ipcm_ipcp_dif_reg_noti_msg_attrs {
        struct name * ipcp_name;
        struct name * dif_name;
        bool          is_registered;
};

struct rnl_ipcm_ipcp_dif_unreg_noti_msg_attrs {
        uint_t result;
};

struct rnl_ipcm_enroll_to_dif_req_msg_attrs {
        struct name * dif_name;
};

struct rnl_ipcm_enroll_to_dif_resp_msg_attrs {
        uint_t result;
};

struct rnl_ipcm_disconn_neighbor_req_msg_attrs {
        struct name * neighbor_name;
};

struct rnl_ipcm_disconn_neighbor_resp_msg_attrs {
        uint_t result;
};

/*
 * FIXME: all the alloc flow structs are the same we can use only a generic one
 */
struct rnl_ipcm_alloc_flow_req_msg_attrs {
        struct name      * source;
        struct name      * dest;
        struct flow_spec * fspec;
        struct name      * dif_name;
};

struct rnl_ipcm_alloc_flow_req_arrived_msg_attrs {
        struct name      * source;
        struct name      * dest;
        struct flow_spec * fspec;
        struct name      * dif_name;
        port_id_t          id;
};

struct rnl_ipcm_alloc_flow_req_result_msg_attrs {
        int       result;
        port_id_t pid;
};

struct rnl_alloc_flow_resp_msg_attrs {
        uint_t    result;
        bool      notify_src;
        port_id_t id;
};

struct rnl_ipcm_dealloc_flow_req_msg_attrs {
        port_id_t id;
};

struct rnl_ipcm_dealloc_flow_resp_msg_attrs {
        uint_t result;
};

struct rnl_ipcm_flow_dealloc_noti_msg_attrs {
        port_id_t id;
        uint_t    code;
};

/*  FIXME: policies should not be int */
struct rnl_ipcp_conn_create_req_msg_attrs {
        port_id_t            port_id;
        address_t            src_addr;
        address_t            dst_addr;
        qos_id_t             qos_id;
        struct dtp_config *  dtp_cfg;
        struct dtcp_config * dtcp_cfg;
};

struct rnl_ipcp_conn_create_resp_msg_attrs {
        port_id_t port_id;
        cep_id_t  src_cep;
};

struct rnl_ipcp_conn_create_arrived_msg_attrs {
        port_id_t            port_id;
        address_t            src_addr;
        address_t            dst_addr;
        cep_id_t             dst_cep;
        qos_id_t             qos_id;
        ipc_process_id_t     flow_user_ipc_process_id;
        struct dtp_config *  dtp_cfg;
        struct dtcp_config * dtcp_cfg;
};

struct rnl_ipcp_conn_create_result_msg_attrs {
        port_id_t port_id;
        cep_id_t  src_cep;
        cep_id_t  dst_cep;
};

struct rnl_ipcp_conn_update_req_msg_attrs {
        port_id_t        port_id;
        cep_id_t         src_cep;
        cep_id_t         dst_cep;
        ipc_process_id_t flow_user_ipc_process_id;
};

struct rnl_ipcp_conn_update_result_msg_attrs {
        port_id_t port_id;
        uint_t    result;
};

struct rnl_ipcp_conn_destroy_req_msg_attrs {
        port_id_t port_id;
        cep_id_t  src_cep;
};

struct rnl_ipcp_conn_destroy_result_msg_attrs {
        port_id_t port_id;
        uint_t    result;
};

struct rnl_ipcm_reg_app_req_msg_attrs {
        struct name * app_name;
        struct name * dif_name;
};

struct rnl_ipcm_reg_app_resp_msg_attrs {
        uint_t      result;
};

struct rnl_ipcm_unreg_app_req_msg_attrs {
        struct name * app_name;
        struct name * dif_name;
};

struct rnl_ipcm_unreg_app_resp_msg_attrs {
        uint_t result;
};

struct rnl_rmt_mod_pffe_msg_attrs {
        int32_t   mode;
        struct list_head pff_entries;
};

struct rnl_rmt_dump_ft_reply_msg_attrs {
        uint_t           result;
        struct list_head pff_entries;
};

struct rnl_ipcm_query_rib_msg_attrs {
        string_t * object_class;
        string_t * object_name;
        uint64_t   object_instance;
        uint32_t   scope;
        string_t * filter;
};

struct rnl_ipcp_set_policy_set_param_req_msg_attrs {
        string_t * path;
        string_t * name;
        string_t * value;
};

struct rnl_ipcp_select_policy_set_req_msg_attrs {
        string_t * path;
        string_t * name;
};

struct rnl_ipcp_update_crypto_state_req_msg_attrs {
	struct sdup_crypto_state * state;
	port_id_t 	port_id;
};

int rnl_parse_msg(struct genl_info * info,
                  struct rnl_msg *   msg);

int rnl_assign_dif_response(ipc_process_id_t id,
                            uint_t           res,
                            rnl_sn_t         seq_num,
                            u32              nl_port_id);

int rnl_update_dif_config_response(ipc_process_id_t id,
                                   uint_t           res,
                                   rnl_sn_t         seq_num,
                                   u32              nl_port_id);

int rnl_app_alloc_flow_req_arrived_msg(ipc_process_id_t         ipc_id,
                                       const struct name *      dif_name,
                                       const struct name *      source,
                                       const struct name *      dest,
                                       const struct flow_spec * fspec,
                                       rnl_sn_t                 seq_num,
                                       u32                      nl_port_id,
                                       port_id_t                pid);

int rnl_app_alloc_flow_result_msg(ipc_process_id_t ipc_id,
                                  uint_t           res,
                                  port_id_t        pid,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id);


int rnl_app_register_unregister_response_msg(ipc_process_id_t ipc_id,
                                             uint_t           res,
                                             rnl_sn_t         seq_num,
                                             u32              nl_port_id,
                                             bool             isRegister);


int rnl_app_dealloc_flow_resp_msg(ipc_process_id_t ipc_id,
                                  uint_t           res,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id);


int rnl_flow_dealloc_not_msg(ipc_process_id_t ipc_id,
                             uint_t           code,
                             port_id_t        port_id,
                             u32              nl_port_id);

int rnl_ipcp_conn_create_resp_msg(ipc_process_id_t ipc_id,
                                  port_id_t        pid,
                                  cep_id_t         src_cep,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id);

int rnl_ipcp_conn_create_result_msg(ipc_process_id_t ipc_id,
                                    port_id_t        pid,
                                    cep_id_t         src_cep,
                                    cep_id_t         dst_cep,
                                    rnl_sn_t         seq_num,
                                    u32              nl_port_id);

int rnl_ipcp_conn_update_result_msg(ipc_process_id_t ipc_id,
                                    port_id_t        pid,
                                    uint_t           result,
                                    rnl_sn_t         seq_num,
                                    u32              nl_port_id);

int rnl_ipcp_conn_destroy_result_msg(ipc_process_id_t ipc_id,
                                     port_id_t        pid,
                                     uint_t           result,
                                     rnl_sn_t         seq_num,
                                     u32              nl_port_id);

int rnl_ipcm_sock_closed_notif_msg(u32 closed_port, u32 dest_port);

int rnl_ipcp_pff_dump_resp_msg(ipc_process_id_t   ipc_id,
                               int                result,
                               struct list_head * entries,
                               rnl_sn_t           seq_num,
                               u32                nl_port_id);

int rnl_ipcm_query_rib_resp_msg(ipc_process_id_t   ipc_id,
                                int                result,
                                struct list_head * entries,
                                rnl_sn_t           seq_num,
                                u32                nl_port_id);

int rnl_set_policy_set_param_response(ipc_process_id_t id,
                                      uint_t           res,
                                      rnl_sn_t         seq_num,
                                      u32              nl_port_id);

int rnl_select_policy_set_response(ipc_process_id_t id,
                                   uint_t           res,
                                   rnl_sn_t         seq_num,
                                   u32              nl_port_id);

int rnl_update_crypto_state_response(ipc_process_id_t id,
                                     uint_t           res,
                                     rnl_sn_t         seq_num,
                                     port_id_t	     n_1_port,
                                     u32              nl_port_id);
#endif
