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

#include <net/netlink.h>
#include <net/genetlink.h>
#include <linux/export.h>

#define RINA_PREFIX "netlink-utils"

#include "logs.h"
#include "common.h"
#include "debug.h"
#include "utils.h"
#include "netlink.h"
#include "netlink-utils.h"

/*
 * FIXME: I suppose these functions are internal (at least for the time being)
 *        therefore have been "statified" to avoid polluting the common name
 *        space (while allowing the compiler to present us "unused functions"
 *        warning messages (which would be unpossible if declared not-static)
 *
 * Francesco
 */

/*
 * FIXME: If some of them remain 'static', parameters checking has to be
 *        trasformed into ASSERT() calls (since msg is checked in the caller)
 *
 * NOTE: that functionalities exported to "shims" should prevent "evoking"
 *       ASSERT() here ...
 *
 * Francesco
 */

/*
 * FIXME: Destination is usually at the end of the prototype, not at the
 * beginning (e.g. msg and name)
 */


#define BUILD_ERR_STRING(X)                                     \
        "Netlink message does not contain " X ", bailing out"

#define BUILD_ERR_STRING_BY_MSG_TYPE(X)                 \
        "Could not parse Netlink message of type "X

static int rnl_check_attr_policy(struct nlmsghdr   * nlh, 
				 int 		   max_attr,
                                 struct nla_policy * attr_policy)
{
	struct nlattr *attrs[max_attr + 1];
	LOG_DBG("Entering rnl_check_attr_policy...");
        if (nlmsg_parse(nlh,
                        /* FIXME: Check if this is correct */
                        sizeof(struct genlmsghdr) +
                        sizeof(struct rina_msg_hdr),
                        attrs,
                        max_attr,
                        attr_policy) < 0){
		LOG_ERR("Could not validate nl message policy");
                return -1;
	}
	LOG_DBG("Leaving rnl_check_attr_policy...");
        return 0;
}

static int parse_flow_spec(struct nlattr * fspec_attr,
                           struct flow_spec * fspec_struct)
{
        struct nla_policy attr_policy[FSPEC_ATTR_MAX + 1];
        struct nlattr *attrs[FSPEC_ATTR_MAX + 1];

        attr_policy[FSPEC_ATTR_AVG_BWITH].type = NLA_U32;
        attr_policy[FSPEC_ATTR_AVG_SDU_BWITH].type = NLA_U32;
        attr_policy[FSPEC_ATTR_DELAY].type = NLA_U32;
        attr_policy[FSPEC_ATTR_JITTER].type = NLA_U32;
        attr_policy[FSPEC_ATTR_MAX_GAP].type = NLA_U32;
        attr_policy[FSPEC_ATTR_MAX_SDU_SIZE].type = NLA_U32;
        attr_policy[FSPEC_ATTR_IN_ORD_DELIVERY].type = NLA_FLAG;
        attr_policy[FSPEC_ATTR_PART_DELIVERY].type = NLA_FLAG;
        attr_policy[FSPEC_ATTR_PEAK_BWITH_DURATION].type = NLA_U32;
        attr_policy[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION].type = NLA_U32;
        attr_policy[FSPEC_ATTR_UNDETECTED_BER].type = NLA_U32;

        if (nla_parse_nested(attrs, FSPEC_ATTR_MAX, fspec_attr, attr_policy) < 0)
        	return -1;

        if (attrs[FSPEC_ATTR_AVG_BWITH])
                /* FIXME: min = max in uint_range */
                fspec_struct->average_bandwidth->min = \
                        fspec_struct->average_bandwidth->max = \
                        nla_get_u32(attrs[FSPEC_ATTR_AVG_BWITH]);

        if (attrs[FSPEC_ATTR_AVG_SDU_BWITH])
                fspec_struct->average_sdu_bandwidth->min = \
                        fspec_struct->average_sdu_bandwidth->max = \
                        nla_get_u32(attrs[FSPEC_ATTR_AVG_SDU_BWITH]);

        if (attrs[FSPEC_ATTR_PEAK_BWITH_DURATION])
                fspec_struct->peak_bandwidth_duration->min = \
                        fspec_struct->peak_bandwidth_duration->max = \
                        nla_get_u32(attrs[FSPEC_ATTR_PEAK_BWITH_DURATION]);

        if (attrs[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION])
                fspec_struct->peak_sdu_bandwidth_duration->min = \
                        fspec_struct->peak_sdu_bandwidth_duration->max = \
                        nla_get_u32(attrs[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION]);

        if (attrs[FSPEC_ATTR_UNDETECTED_BER])
                fspec_struct->undetected_bit_error_rate = \
                        nla_get_u32(attrs[FSPEC_ATTR_UNDETECTED_BER]);

        if (attrs[FSPEC_ATTR_UNDETECTED_BER])
                fspec_struct->undetected_bit_error_rate = \
                        nla_get_u32(attrs[FSPEC_ATTR_UNDETECTED_BER]);

        if (attrs[FSPEC_ATTR_PART_DELIVERY])
                fspec_struct->partial_delivery = \
                        nla_get_flag(attrs[FSPEC_ATTR_PART_DELIVERY]);

        if (attrs[FSPEC_ATTR_IN_ORD_DELIVERY])
                fspec_struct->ordered_delivery = \
                        nla_get_flag(attrs[FSPEC_ATTR_IN_ORD_DELIVERY]);

        if (attrs[FSPEC_ATTR_MAX_GAP])
                fspec_struct->max_allowable_gap = \
                        (int) nla_get_u32(attrs[FSPEC_ATTR_MAX_GAP]);

        if (attrs[FSPEC_ATTR_DELAY])
                fspec_struct->delay = \
                        nla_get_u32(attrs[FSPEC_ATTR_DELAY]);

        if (attrs[FSPEC_ATTR_MAX_SDU_SIZE])
                fspec_struct->max_sdu_size = \
                        nla_get_u32(attrs[FSPEC_ATTR_MAX_SDU_SIZE]);

        return 0;

}

static int parse_app_name_info(struct nlattr * name_attr,
                               struct name * name_struct)
{
        struct nla_policy attr_policy[APNI_ATTR_MAX + 1];
        struct nlattr *attrs[APNI_ATTR_MAX + 1];

        attr_policy[APNI_ATTR_PROCESS_NAME].type = NLA_STRING;
        attr_policy[APNI_ATTR_PROCESS_INSTANCE].type = NLA_STRING;
        attr_policy[APNI_ATTR_ENTITY_NAME].type = NLA_STRING;
        attr_policy[APNI_ATTR_ENTITY_INSTANCE].type = NLA_STRING;

        if (nla_parse_nested(attrs, APNI_ATTR_MAX, name_attr, attr_policy) < 0)
                return -1;

        if (attrs[APNI_ATTR_PROCESS_NAME])
                nla_strlcpy(name_struct->process_name,
                            attrs[APNI_ATTR_PROCESS_NAME],
                            sizeof(attrs[APNI_ATTR_PROCESS_NAME]));

        if (attrs[APNI_ATTR_PROCESS_INSTANCE])
                nla_strlcpy(name_struct->process_instance,
                            attrs[APNI_ATTR_PROCESS_INSTANCE],
                            sizeof(attrs[APNI_ATTR_PROCESS_INSTANCE]));
        if (attrs[APNI_ATTR_ENTITY_NAME])
                nla_strlcpy(name_struct->entity_name,
                            attrs[APNI_ATTR_ENTITY_NAME],
                            sizeof(attrs[APNI_ATTR_ENTITY_NAME]));
        if (attrs[APNI_ATTR_ENTITY_INSTANCE])
                nla_strlcpy(name_struct->entity_instance,
                            attrs[APNI_ATTR_ENTITY_INSTANCE],
                            sizeof(attrs[APNI_ATTR_ENTITY_INSTANCE]));
        return 0;
}

static int parse_dif_config(struct nlattr      * dif_config_attr,
                            struct dif_config  * dif_config_struct)
{
        struct nla_policy attr_policy[DCONF_ATTR_MAX + 1];
        struct nlattr *attrs[DCONF_ATTR_MAX + 1];

        attr_policy[DCONF_ATTR_DIF_TYPE].type = NLA_STRING;
        attr_policy[DCONF_ATTR_DIF_NAME].type = NLA_NESTED;

        if (nla_parse_nested(attrs, 
			     DCONF_ATTR_MAX, 
			     dif_config_attr, 
			     attr_policy) < 0)
                goto parse_fail;

        if (attrs[DCONF_ATTR_DIF_TYPE])
                nla_strlcpy(dif_config_struct->type,
                            attrs[DCONF_ATTR_DIF_TYPE],
                            sizeof(attrs[DCONF_ATTR_DIF_TYPE]));

	if (parse_app_name_info(attrs[DCONF_ATTR_DIF_NAME], 
				dif_config_struct->dif_name) <0)
		goto parse_fail;

	return 0;

	parse_fail:
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("dif config attribute"));
        	return -1;

		
}

static int parse_rib_object(struct nlattr     * rib_obj_attr,
                            struct rib_object * rib_obj_struct)
{
        struct nla_policy attr_policy[RIBO_ATTR_MAX + 1];
        struct nlattr *attrs[RIBO_ATTR_MAX + 1];

        attr_policy[RIBO_ATTR_OBJECT_CLASS].type = NLA_U32;
        attr_policy[RIBO_ATTR_OBJECT_NAME].type = NLA_STRING;
        attr_policy[RIBO_ATTR_OBJECT_INSTANCE].type = NLA_U32;

        if (nla_parse_nested(attrs, RIBO_ATTR_MAX, rib_obj_attr, attr_policy) < 0)
                return -1;

        if (attrs[RIBO_ATTR_OBJECT_CLASS])
                rib_obj_struct->rib_obj_class =\
                            nla_get_u32(&rib_obj_attr[RIBO_ATTR_OBJECT_CLASS]);

        if (attrs[RIBO_ATTR_OBJECT_NAME])
                nla_strlcpy(rib_obj_struct->rib_obj_name,
                            attrs[RIBO_ATTR_OBJECT_NAME],
                            sizeof(attrs[RIBO_ATTR_OBJECT_NAME]));
        if (attrs[RIBO_ATTR_OBJECT_INSTANCE])
                rib_obj_struct->rib_obj_instance =\
                            nla_get_u32(&rib_obj_attr[RIBO_ATTR_OBJECT_INSTANCE]);
        return 0;
}

static int parse_rib_objects_list(struct nlattr     * rib_objs_attr,
				  uint_t            count,
                                  struct rib_object * rib_objs_struct)
{
	int i;
	for (i=0; i < count; i++){
		if (parse_rib_object(&rib_objs_attr[i], &rib_objs_struct[i]) < 0){
			LOG_ERR("Could not parse rib_objs_list attribute");
			return -1;
		}
	}
	return 0;
}

static int rnl_parse_generic_u32_param_msg (struct genl_info * info,
					    uint_t           param_var,
					    uint_t	     param_name,
					    uint_t	     max_params,
					    string_t         * msg_name)
{
	struct nla_policy attr_policy[max_params + 1];

	LOG_DBG("rnl_parse_generic_u32_param_msg started...");
	
	attr_policy[param_name].type = NLA_U32;

	if(rnl_check_attr_policy(info->nlhdr, max_params, attr_policy) < 0){
		LOG_ERR("Could not parse Netlink message of type %s", msg_name);
		return -1;
	}

        if (info->attrs[param_name])
                param_var = nla_get_u32(info->attrs[param_name]);

	return 0;

}

static int rnl_parse_ipcm_assign_to_dif_req_msg(struct genl_info * info,
		struct rnl_ipcm_assign_to_dif_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IATDR_ATTR_MAX + 1];

        attr_policy[IATDR_ATTR_DIF_CONFIGURATION].type = NLA_NESTED;
	
        if (rnl_check_attr_policy(info->nlhdr, IATDR_ATTR_MAX, attr_policy) < 0 || 
	    parse_dif_config(info->attrs[IATDR_ATTR_DIF_CONFIGURATION],
                             msg_attrs->dif_config) < 0) {
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST"));
        	return -1;
	}

	return 0;
}


static int rnl_parse_ipcm_assign_to_dif_resp_msg(struct genl_info * info,
	  struct rnl_ipcm_assign_to_dif_resp_msg_attrs * msg_attrs)
{
	LOG_DBG("rnl_parse_ipcm_assign_to_dif_resp_msg started...");
	return rnl_parse_generic_u32_param_msg(info,
					msg_attrs->result,
					IATDRE_ATTR_RESULT,
					IATDRE_ATTR_MAX,
					"RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE");
}

static int rnl_parse_ipcm_ipcp_dif_reg_noti_msg(struct genl_info * info,
		struct rnl_ipcm_ipcp_dif_reg_noti_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IDRN_ATTR_MAX + 1];

        attr_policy[IDRN_ATTR_IPC_PROCESS_NAME].type = NLA_NESTED;
        attr_policy[IDRN_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[IDRN_ATTR_REGISTRATION].type = NLA_FLAG;

        if (rnl_check_attr_policy(info->nlhdr, IDRN_ATTR_MAX, attr_policy) < 0 ||
            parse_app_name_info(info->attrs[IDRN_ATTR_IPC_PROCESS_NAME],
                                msg_attrs->ipcp_name) < 0		||
            parse_app_name_info(info->attrs[IDRN_ATTR_DIF_NAME], 
	    			msg_attrs->dif_name) < 0) {
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE(\
		"RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION"));
	        return -1;

 	}
	
	if (info->attrs[IDRN_ATTR_REGISTRATION])
		msg_attrs->is_registered = \
			nla_get_flag(info->attrs[IDRN_ATTR_REGISTRATION]);

        return 0;

}

static int rnl_parse_ipcm_ipcp_dif_unreg_noti_msg(struct genl_info * info,
	  struct rnl_ipcm_ipcp_dif_unreg_noti_msg_attrs * msg_attrs)
{
	return rnl_parse_generic_u32_param_msg(info,
					msg_attrs->result,
					IDUN_ATTR_RESULT,
					IDUN_ATTR_MAX,
					"RINA_C_IPCM_IPC_PROCESS_UNREGISTRATION_NOTIFICATION");
}

static int rnl_parse_ipcm_enroll_to_dif_req_msg(struct genl_info * info,
	  struct rnl_ipcm_enroll_to_dif_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IEDR_ATTR_MAX + 1];

        attr_policy[IEDR_ATTR_DIF_NAME].type = NLA_NESTED;

        if (rnl_check_attr_policy(info->nlhdr, IEDR_ATTR_MAX, attr_policy) < 0 ||
            parse_app_name_info(info->attrs[IEDR_ATTR_DIF_NAME], 
	    			msg_attrs->dif_name) < 0) {
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE(\
		"RINA_C_IPCM_ENROLL_TO_DIF_REQUEST:"));
	        return -1;

 	}
	return 0;
}

static int rnl_parse_ipcm_enroll_to_dif_resp_msg(struct genl_info * info,
	  struct rnl_ipcm_enroll_to_dif_resp_msg_attrs * msg_attrs)
{
	return rnl_parse_generic_u32_param_msg(info,
					msg_attrs->result,
					IEDRE_ATTR_RESULT,
					IEDRE_ATTR_MAX,
					"RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE");
}

static int rnl_parse_ipcm_disconn_neighbor_req_msg(struct genl_info * info,
          struct rnl_ipcm_disconn_neighbor_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IDNR_ATTR_MAX + 1];

        attr_policy[IDNR_ATTR_NEIGHBOR_NAME].type = NLA_NESTED;

        if (rnl_check_attr_policy(info->nlhdr, IDNR_ATTR_MAX, attr_policy) < 0 ||
            parse_app_name_info(info->attrs[IDNR_ATTR_NEIGHBOR_NAME], 
	    			msg_attrs->neighbor_name) < 0) {
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE(\
		"RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST:"));
	        return -1;

 	}
	return 0;
}

static int rnl_parse_ipcm_disconn_neighbor_resp_msg(struct genl_info * info,
	  struct rnl_ipcm_disconn_neighbor_resp_msg_attrs * msg_attrs)
{
	return rnl_parse_generic_u32_param_msg(info,
					msg_attrs->result,
					IATDRE_ATTR_RESULT,
					IATDRE_ATTR_MAX,
					"RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE");
}

static int rnl_parse_ipcm_alloc_flow_req_msg(struct genl_info * info,
                                             struct rnl_ipcm_alloc_flow_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRM_ATTR_MAX + 1];

        attr_policy[IAFRM_ATTR_SOURCE_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRM_ATTR_DEST_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRM_ATTR_FLOW_SPEC].type = NLA_NESTED;
        attr_policy[IAFRM_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[IAFRM_ATTR_PORT_ID].type = NLA_U32;

        if (rnl_check_attr_policy(info->nlhdr, IAFRM_ATTR_MAX, attr_policy) < 0)
                goto format_fail;

        if (parse_app_name_info(info->attrs[IAFRM_ATTR_SOURCE_APP_NAME],
                                msg_attrs->source) < 0)
                goto format_fail;

        if (parse_app_name_info(info->attrs[IAFRM_ATTR_DEST_APP_NAME],
                                msg_attrs->dest) < 0)
                goto format_fail;

        if (parse_flow_spec(info->attrs[IAFRM_ATTR_FLOW_SPEC],
                            msg_attrs->fspec) < 0);
        	goto format_fail;

        if (info->attrs[IAFRM_ATTR_PORT_ID])
                msg_attrs->id = \
                        (port_id_t) nla_get_u32(info->attrs[IAFRM_ATTR_PORT_ID]);

        if (parse_app_name_info(info->attrs[IAFRM_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0)
                goto format_fail;

        return 0;

 	format_fail:
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_ALLOCATE_FLOW_REQUEST"));
	        return -1;
}


static int rnl_parse_ipcm_alloc_flow_req_arrived_msg(struct genl_info * info,
                struct rnl_ipcm_alloc_flow_req_arrived_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRA_ATTR_MAX + 1];

        attr_policy[IAFRA_ATTR_SOURCE_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_DEST_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_FLOW_SPEC].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_DIF_NAME].type = NLA_NESTED;

        if (rnl_check_attr_policy(info->nlhdr, IAFRA_ATTR_MAX, attr_policy) < 0 ||
            parse_app_name_info(info->attrs[IAFRA_ATTR_SOURCE_APP_NAME],
                                 msg_attrs->source) < 0		 ||
            parse_app_name_info(info->attrs[IAFRA_ATTR_DEST_APP_NAME],
                                msg_attrs->dest) < 0			 ||
            parse_flow_spec(info->attrs[IAFRA_ATTR_FLOW_SPEC],
                            msg_attrs->fspec) < 0			 ||
            parse_app_name_info(info->attrs[IAFRA_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0){
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED"));
	        return -1;
	}

	return 0;
}

static int rnl_parse_ipcm_alloc_flow_req_result_msg(struct genl_info * info,
	struct rnl_ipcm_alloc_flow_req_result_msg_attrs * msg_attrs)
{
	return rnl_parse_generic_u32_param_msg(info,
					msg_attrs->result,
					IAFRRM_ATTR_RESULT,
					IAFRRM_ATTR_MAX,
					"RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT");
}

static int rnl_parse_ipcm_alloc_flow_resp_msg(struct genl_info * info,
                     struct rnl_alloc_flow_resp_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRE_ATTR_MAX + 1];

        /* FIXME: nla_policy struct is different from user-space. No min/max
         * attrs. len not specified */
        attr_policy[IAFRE_ATTR_RESULT].type = NLA_U32;
        attr_policy[IAFRE_ATTR_NOTIFY_SOURCE].type = NLA_FLAG;
        attr_policy[IAFRE_ATTR_PORT_ID].type = NLA_U32;

        /* Any other comprobations could be done in addition to nlmsg_parse()
         * done by rnl_check_attr_policy */

        if (rnl_check_attr_policy(info->nlhdr, IAFRE_ATTR_MAX, attr_policy) < 0){
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_ALLOCATE_FLOW_RESPONSE"));
        	return -1;
	}

        if (info->attrs[IAFRE_ATTR_RESULT])
                msg_attrs->result       = \
                        nla_get_u32(info->attrs[IAFRE_ATTR_RESULT]);
        if (info->attrs[IAFRE_ATTR_NOTIFY_SOURCE])
                msg_attrs->notify_src  = \
                        nla_get_flag(info->attrs[IAFRE_ATTR_NOTIFY_SOURCE]);
        if (info->attrs[IAFRE_ATTR_PORT_ID])
                msg_attrs->id       = \
                        nla_get_u32(info->attrs[IAFRE_ATTR_PORT_ID]);
        return 0;

}

static int rnl_parse_ipcm_dealloc_flow_req_msg(struct genl_info * info,
                struct rnl_ipcm_dealloc_flow_req_msg_attrs * msg_attrs)
{
	return rnl_parse_generic_u32_param_msg(info,
					msg_attrs->id,
					IDFRT_ATTR_PORT_ID,
					IDFRT_ATTR_MAX,
					"RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST");

}


static int rnl_parse_ipcm_dealloc_flow_resp_msg(struct genl_info * info,
                struct rnl_ipcm_dealloc_flow_resp_msg_attrs * msg_attrs)
{
	return rnl_parse_generic_u32_param_msg(info,
					msg_attrs->result,
					IDFRE_ATTR_RESULT,
					IDFRE_ATTR_MAX,
					"RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE");
}

static int rnl_parse_ipcm_flow_dealloc_noti_msg(struct genl_info * info,
	  struct rnl_ipcm_flow_dealloc_noti_msg_attrs * msg_attrs)
{
	struct nla_policy attr_policy[IFDN_ATTR_MAX + 1];

	attr_policy[IFDN_ATTR_PORT_ID].type = NLA_U32;
	attr_policy[IFDN_ATTR_CODE].type = NLA_U32;

	if(rnl_check_attr_policy(info->nlhdr, IFDN_ATTR_MAX, attr_policy) < 0){
		LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION"));
		return -1;
	}

        if (info->attrs[IFDN_ATTR_PORT_ID])
                msg_attrs->id = nla_get_u32(info->attrs[IFDN_ATTR_PORT_ID]);
        if (info->attrs[IFDN_ATTR_CODE])
                msg_attrs->code = nla_get_u32(info->attrs[IFDN_ATTR_CODE]);
	return 0;
}

static int rnl_parse_ipcm_reg_app_req_msg(struct genl_info * info,
		struct rnl_ipcm_reg_app_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IRAR_ATTR_MAX + 1];

        attr_policy[IRAR_ATTR_APP_NAME].type = NLA_NESTED;
        attr_policy[IRAR_ATTR_DIF_NAME].type = NLA_NESTED;

        if (rnl_check_attr_policy(info->nlhdr, IRAR_ATTR_MAX, attr_policy) < 0 ||
            parse_app_name_info(info->attrs[IRAR_ATTR_APP_NAME],
                                 msg_attrs->app_name) < 0		         ||
            parse_app_name_info(info->attrs[IRAR_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0){
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_REGISTER_APPLICATION_REQUEST"));
	        return -1;
	}

	return 0;
}

static int rnl_parse_ipcm_reg_app_resp_msg(struct genl_info * info,
	  struct rnl_ipcm_reg_app_resp_msg_attrs * msg_attrs)
{
	struct nla_policy attr_policy[IRARE_ATTR_MAX + 1];

        attr_policy[IRAR_ATTR_APP_NAME].type = NLA_NESTED;
	attr_policy[IRARE_ATTR_RESULT].type = NLA_U32;

	if(rnl_check_attr_policy(info->nlhdr, IRARE_ATTR_MAX, attr_policy) < 0 ||
           parse_app_name_info(info->attrs[IRARE_ATTR_APP_NAME],
                                 msg_attrs->app_name) < 0) {
		LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE"));
		return -1;
	}

        if (info->attrs[IRARE_ATTR_RESULT])
                msg_attrs->result = nla_get_u32(info->attrs[IRARE_ATTR_RESULT]);
	return 0;
}

static int rnl_parse_ipcm_unreg_app_req_msg(struct genl_info * info,
	  struct rnl_ipcm_unreg_app_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IUAR_ATTR_MAX + 1];

        attr_policy[IUAR_ATTR_APP_NAME].type = NLA_NESTED;
        attr_policy[IUAR_ATTR_DIF_NAME].type = NLA_NESTED;

        if (rnl_check_attr_policy(info->nlhdr, IUAR_ATTR_MAX, attr_policy) < 0 ||
            parse_app_name_info(info->attrs[IUAR_ATTR_APP_NAME],
                                 msg_attrs->app_name) < 0		         ||
            parse_app_name_info(info->attrs[IUAR_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0){
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST"));
	        return -1;
	}

	return 0;
}

static int rnl_parse_ipcm_unreg_app_resp_msg(struct genl_info * info,
	  struct rnl_ipcm_unreg_app_resp_msg_attrs * msg_attrs)
{
	return rnl_parse_generic_u32_param_msg(info,
					msg_attrs->result,
					IUARE_ATTR_RESULT,
					IUARE_ATTR_MAX,
					"RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE");
}

static int rnl_parse_ipcm_query_rib_req_msg(struct genl_info * info,
	  struct rnl_ipcm_query_rib_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IDQR_ATTR_MAX + 1];

        attr_policy[IDQR_ATTR_OBJECT].type = NLA_NESTED;
        attr_policy[IDQR_ATTR_SCOPE].type = NLA_U32;
        attr_policy[IDQR_ATTR_FILTER].type = NLA_STRING;

        if (rnl_check_attr_policy(info->nlhdr, IDQR_ATTR_MAX, attr_policy) < 0 ||
            parse_rib_object(info->attrs[IDQR_ATTR_OBJECT],
                             msg_attrs->rib_obj) < 0) {                
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_QUERY_RIB_REQUEST"));
	        return -1;
	}
        if (info->attrs[IDQR_ATTR_SCOPE])
                msg_attrs->scope = \
                        nla_get_u32(info->attrs[IDQR_ATTR_SCOPE]);

        if (info->attrs[IDQR_ATTR_FILTER])
                nla_strlcpy(msg_attrs->filter,
                            info->attrs[IDQR_ATTR_FILTER],
                            sizeof(info->attrs[IDQR_ATTR_FILTER]));
        return 0;
}

/* FIXME: Check all RIB objects parsing functions. Not sure they are correct */
static int rnl_parse_ipcm_query_rib_resp_msg(struct genl_info * info,
	  struct rnl_ipcm_query_rib_resp_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IDQR_ATTR_MAX + 1];

        attr_policy[IDQRE_ATTR_RESULT].type = NLA_U32;
        attr_policy[IDQRE_ATTR_COUNT].type = NLA_U32;
        attr_policy[IDQRE_ATTR_RIB_OBJECTS].type = NLA_NESTED;

        if (rnl_check_attr_policy(info->nlhdr, IDQRE_ATTR_MAX, attr_policy) < 0)
		goto parse_fail;

	if (info->attrs[IDQRE_ATTR_RESULT])
                msg_attrs->result = \
                        nla_get_u32(info->attrs[IDQRE_ATTR_RESULT]);

        if (info->attrs[IDQRE_ATTR_COUNT])
                msg_attrs->count = \
                        nla_get_u32(info->attrs[IDQRE_ATTR_COUNT]);

	if(parse_rib_objects_list(info->attrs[IDQRE_ATTR_RIB_OBJECTS],
	    		 	   msg_attrs->count,
                                   msg_attrs->rib_objs) < 0)
		goto parse_fail;
	
	return 0;

	parse_fail:
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_QUERY_RIB_RESPONSE"));
	        return -1;
}

static int rnl_parse_rmt_add_fte_req_msg(struct genl_info * info,
	  struct rnl_rmt_add_fte_req_msg_attrs * msg_attrs)
{
	return 0;
}

static int rnl_parse_rmt_del_fte_req_msg(struct genl_info * info,
	  struct rnl_rmt_del_fte_req_msg_attrs * msg_attrs)
{
	return 0;
}

static int rnl_parse_rmt_dump_ft_req_msg(struct genl_info * info,
	   struct rnl_rmt_dump_ft_req_msg_attrs * msg_attrs)
{
	return 0;
}

static int rnl_parse_rmt_dump_ft_reply_msg(struct genl_info * info,
	  struct rnl_rmt_dump_ft_reply_msg_attrs * msg_attrs)
{
	return 0;
}

int rnl_parse_msg(struct genl_info      * info,
                  struct rnl_msg        * msg)
{

	LOG_DBG("RINA Netlink parser started...");

        /* harcoded  */
        /* msg->family                  = "rina";*/
        msg->src_port                   = info->snd_portid;
        /* dst_port can not be parsed */
        msg->dst_port                   = 0;
        msg->seq_num                    = info->snd_seq;
        msg->op_code                    = (msg_id) info->genlhdr->cmd;
#if 0
        msg->req_msg_flag               =
                msg->resp_msg_flag              =
                msg->notification_msg_flag      =
#endif
        msg->rina_hdr           = info->userhdr;

	LOG_DBG("Parsed Netlink message header:\n"
		"msg->src_port: %d "
		"msg->dst_port: %d "
		"msg->seq_num:  %d "
		"msg->op_code:  %d "
		"msg->rina_hdr->src_ipc_id: %d "
		"msg->rina_hdr->dst_ipc_id: %d",msg->src_port,msg->dst_port,
				   		msg->seq_num,msg->op_code,
						msg->rina_hdr->src_ipc_id,
						msg->rina_hdr->dst_ipc_id);
	LOG_DBG("[LDBG] msg is at %pK", msg);
	LOG_DBG("[LDBG] msg->rina_hdr is at %pK and size is: %d", msg->rina_hdr, sizeof(msg->rina_hdr));
	LOG_DBG("[LDBG] msg->attrs is at %pK", msg->attrs);

	switch(info->genlhdr->cmd){
        case RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST:
		if (rnl_parse_ipcm_assign_to_dif_req_msg(info,
							 msg->attrs) < 0)
                	goto fail;
                break;
        case RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE:
		if (rnl_parse_ipcm_assign_to_dif_resp_msg(info,
							  msg->attrs) <0)
			goto fail;
                break;
        case RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION:
		if (rnl_parse_ipcm_ipcp_dif_reg_noti_msg(info,
							 msg->attrs) < 0)
			goto fail;
                break;
        case RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION:
		if (rnl_parse_ipcm_ipcp_dif_unreg_noti_msg(info, 
							   msg->attrs) < 0)
			goto fail;
                break;
        case RINA_C_IPCM_ENROLL_TO_DIF_REQUEST:
		if (rnl_parse_ipcm_enroll_to_dif_req_msg(info,
							 msg->attrs) < 0)
			goto fail;
                break;
        case RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE:
		if (rnl_parse_ipcm_enroll_to_dif_resp_msg(info,
							  msg->attrs) < 0)
			goto fail;
                break;
        case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST:
		if (rnl_parse_ipcm_disconn_neighbor_req_msg(info,
							    msg->attrs) < 0)
			goto fail;
                break;
        case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE:
		if (rnl_parse_ipcm_disconn_neighbor_resp_msg(info,
							     msg->attrs) < 0)
			goto fail;
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST:
                if (rnl_parse_ipcm_alloc_flow_req_msg(info, 
						      msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED:
                if (rnl_parse_ipcm_alloc_flow_req_arrived_msg(info, 
							      msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
                if (rnl_parse_ipcm_alloc_flow_req_result_msg(info, 
							     msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE:
                if (rnl_parse_ipcm_alloc_flow_resp_msg(info, 
						       msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST:
                if (rnl_parse_ipcm_dealloc_flow_req_msg(info, 
							msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION:
                if (rnl_parse_ipcm_flow_dealloc_noti_msg(info, 
							 msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE:
                if (rnl_parse_ipcm_dealloc_flow_resp_msg(info, 
							 msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_REGISTER_APPLICATION_REQUEST:
                if (rnl_parse_ipcm_reg_app_req_msg(info, 
						   msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE:
                if (rnl_parse_ipcm_reg_app_resp_msg(info, 
					 	    msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST:
                if (rnl_parse_ipcm_unreg_app_req_msg(info, 
					 	     msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE:
                if (rnl_parse_ipcm_unreg_app_resp_msg(info, 
				 	 	      msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_QUERY_RIB_REQUEST:
                if (rnl_parse_ipcm_query_rib_req_msg(info, 
				 	 	     msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_QUERY_RIB_RESPONSE:
                if (rnl_parse_ipcm_query_rib_resp_msg(info, 
				 	 	      msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_RMT_ADD_FTE_REQUEST:
                if (rnl_parse_rmt_add_fte_req_msg(info, 
				 	 	  msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_RMT_DELETE_FTE_REQUEST:
                if (rnl_parse_rmt_del_fte_req_msg(info, 
				 	 	  msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_RMT_DUMP_FT_REQUEST:
                if (rnl_parse_rmt_dump_ft_req_msg(info, 
				 	 	  msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_RMT_DUMP_FT_REPLY:
                if (rnl_parse_rmt_dump_ft_reply_msg(info, 
				 	 	    msg->attrs) < 0)
                        goto fail;
                break;
        default:
                goto fail;
                break;
        }
        return 0;

 fail:
        LOG_ERR("Could not parse netlink message of type: %d", info->genlhdr->cmd);
        return -1;
}
EXPORT_SYMBOL(rnl_parse_msg);


/* FORMATTING */


static int format_app_name_info(const struct name * name,
                                struct sk_buff * msg)
{
        if (!msg) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        /*
         * Components might be missing (and nla_put_string wonna have NUL
         * terminated strings, otherwise kernel panics are on the way).
         * Would we like to fallback here or simply return an error if (at
         * least) one of them is missing ?
         */

        if (name->process_name)
                if (nla_put_string(msg,
                                   APNI_ATTR_PROCESS_NAME,
                                   name->process_name))
                        return -1;
        if (name->process_instance)
                if (nla_put_string(msg,
                                   APNI_ATTR_PROCESS_INSTANCE,
                                   name->process_instance))
                        return -1;
        if (name->entity_name)
                if (nla_put_string(msg,
                                   APNI_ATTR_ENTITY_NAME,
                                   name->entity_name))
                        return -1;
        if (name->entity_instance)
                if (nla_put_string(msg,
                                   APNI_ATTR_ENTITY_INSTANCE,
                                   name->entity_instance))
                        return -1;

        return 0;
}

static int format_flow_spec(const struct flow_spec * fspec,
                            struct sk_buff   * msg)
{
        if (!msg) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        /*
         * FIXME: only->max or min attributes are taken from
         *  uint_range types
         */
        /* FIXME: ??? only max is accessed, what do you mean ? */
        /* FIXME: librina does not define ranges for these attributes, just
         * unique values. So far I seleced only the max or min value depending
         * on the most restrincting (in this case all max).
         * Leo */

        if (fspec->average_bandwidth->max > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_AVG_BWITH,
                                fspec->average_bandwidth->max))
                        return -1;
        if (fspec->average_sdu_bandwidth->max > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_AVG_SDU_BWITH,
                                fspec->average_sdu_bandwidth->max))
                        return -1;
        if (fspec->delay > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_DELAY,
                                fspec->delay))
                        return -1;
        if (fspec->jitter > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_JITTER,
                                fspec->jitter))
                        return -1;
        if (fspec->max_allowable_gap > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_MAX_GAP,
                                fspec->max_allowable_gap))
                        return -1;
        if (fspec->max_sdu_size > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_MAX_SDU_SIZE,
                                fspec->max_sdu_size))
                        return -1;
        if (fspec->ordered_delivery > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_IN_ORD_DELIVERY,
                                fspec->ordered_delivery))
                        return -1;
        if (fspec->partial_delivery > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_PART_DELIVERY,
                                fspec->partial_delivery))
                        return -1;
        if (fspec->peak_bandwidth_duration->max > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_PEAK_BWITH_DURATION,
                                fspec->peak_bandwidth_duration->max))
                        return -1;
        if (fspec->peak_sdu_bandwidth_duration->max > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_PEAK_SDU_BWITH_DURATION,
                                fspec->peak_sdu_bandwidth_duration->max))
                        return -1;
        if (fspec->undetected_bit_error_rate > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_UNDETECTED_BER,
                                fspec->undetected_bit_error_rate))
                        return -1;
        return 0;
}

static int format_nested_app_name_info_attr(struct nlattr     * msg_attr,
			      		    uint_t            attr_name,
                              		    const struct name * name,
                              		    struct sk_buff    * skb_out)
{
        if (!(msg_attr =
                nla_nest_start(skb_out, attr_name))) {
                nla_nest_cancel(skb_out, msg_attr);
                LOG_ERR(BUILD_ERR_STRING("Name attribute"));
                return -1;
        }
        if (format_app_name_info(name, skb_out) < 0)
                return -1;
        nla_nest_end(skb_out, msg_attr);
        return 0;
}

static int format_dif_config(const struct dif_config * config,
			     struct sk_buff          * msg)
{
	struct nlattr * msg_dif_name = NULL;

	format_nested_app_name_info_attr(msg_dif_name,
			                 DCONF_ATTR_DIF_NAME,
			                 config->dif_name,
			                 msg);


        if (!(msg_dif_name =
                nla_nest_start(msg, DCONF_ATTR_DIF_NAME))) {
                nla_nest_cancel(msg, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("dif name attribute"));
                return -1;
        }
        if (format_app_name_info(config->dif_name, msg) < 0)
		return -1;
	nla_nest_end(msg, msg_dif_name);

	if (nla_put_string(msg, 
			  DCONF_ATTR_DIF_TYPE,
			  config->type) < 0)
                LOG_ERR(BUILD_ERR_STRING("dif type attribute"));
 		return -1;

	return 0;

}

static int rnl_format_generic_u32_param_msg(uint_t          param_var,
					    uint_t 	    param_name,
					    string_t 	    * msg_name,
                                            struct sk_buff  * skb_out)
{
	LOG_DBG("Entring rnl_format_generic_u32_param_msg...");
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }
	
	if (nla_put_u32(skb_out, param_name, param_var) < 0) {
                LOG_ERR("Could not format %s message correctly", msg_name);
                return -1;
	}
        return 0;
}

int rnl_format_ipcm_assign_to_dif_req_msg(const struct dif_config  * config,
                                          struct sk_buff           * skb_out)
{
        struct nlattr * msg_config;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_config =
                nla_nest_start(skb_out, IATDR_ATTR_DIF_CONFIGURATION))) {
                nla_nest_cancel(skb_out, msg_config);
                LOG_ERR(BUILD_ERR_STRING("dif configuration attribute"));
                goto format_fail;
        }
        if (format_dif_config(config, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_config);

        return 0;

	format_fail:
                LOG_ERR("Could not format "
                        "rnl_ipcm_assign_to_dif_req_msg "
                        "message correctly");
                return -1;

}
EXPORT_SYMBOL(rnl_format_ipcm_assign_to_dif_req_msg);



int rnl_format_ipcm_assign_to_dif_resp_msg(uint_t          result,
                                           struct sk_buff  * skb_out)
{
	return rnl_format_generic_u32_param_msg(result,
					     IAFRRM_ATTR_RESULT, 
					     "rnl_ipcm_assign_to_dif_resp_msg",
					     skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_assign_to_dif_resp_msg);

int rnl_format_ipcm_ipcp_dif_reg_noti_msg(const struct name * ipcp_name,
                                          const struct name * dif_name,
                                          bool		    is_registered,
                                          struct sk_buff    * skb_out)
{
        struct nlattr * msg_ipcp_name, * msg_dif_name;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                goto format_fail;
        }

        /* name-formating might be moved into its own function (and reused) */
        if (!(msg_ipcp_name =
                nla_nest_start(skb_out, IDRN_ATTR_IPC_PROCESS_NAME))) {
                nla_nest_cancel(skb_out, msg_ipcp_name);
                LOG_ERR(BUILD_ERR_STRING("ipcp name attribute"));
                goto format_fail;
        }
	if (format_app_name_info(ipcp_name, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_ipcp_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IDRN_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("dif name attribute"));
                goto format_fail;
        }
	if (format_app_name_info(dif_name, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_dif_name);

	/* FIXME: I think the flag value must be specified so nla_put_flag
	* can not be used. Check in US cause it is using it */
        if (nla_put(skb_out, IDRN_ATTR_REGISTRATION, is_registered, NULL) < 0) {
                LOG_ERR(BUILD_ERR_STRING("is_registered attribute"));
		goto format_fail;
	}

	return 0;

	format_fail:
                LOG_ERR("Could not format "
                        "rnl_ipcm_ipcp_dif_reg_noti_msg "
                        "message correctly");
                return -1;

}
EXPORT_SYMBOL(rnl_format_ipcm_ipcp_dif_reg_noti_msg);

/*  FIXME: It does not exist in user space */
int rnl_format_ipcm_ipcp_dif_unreg_noti_msg(uint_t          result,
                                            struct sk_buff  * skb_out)
{
	return rnl_format_generic_u32_param_msg(result,
					     IDUN_ATTR_RESULT, 
					     "rnl_ipcm_ipcp_to_dif_unreg_noti_msg",
					     skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_ipcp_dif_unreg_noti_msg);

int rnl_format_ipcm_enroll_to_dif_req_msg(const struct name    * dif_name,
                                          struct sk_buff       *  skb_out)
{
        struct nlattr * msg_dif_name;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                goto format_fail;
        }

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IEDR_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("dif name attribute"));
                goto format_fail;
        }
	if (format_app_name_info(dif_name, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_dif_name);

	return 0;

	format_fail:
                LOG_ERR("Could not format "
                        "rnl_ipcm_enroll_to_dif_req_msg "
                        "message correctly");
                return -1;
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_enroll_to_dif_req_msg);

int rnl_format_ipcm_enroll_to_dif_resp_msg(uint_t         result,
                                           struct sk_buff * skb_out)
{
	return rnl_format_generic_u32_param_msg(result,
					     IEDRE_ATTR_RESULT, 
					     "rnl_ipcm_enroll_to_dif_resp_msg",
					     skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_enroll_to_dif_resp_msg);

int rnl_format_ipcm_disconn_neighbor_req_msg(const struct name    * neighbor_name,
                                             struct sk_buff * skb_out)
{
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_disconn_neighbor_req_msg);

int rnl_format_ipcm_disconn_neighbor_resp_msg(uint_t         result,
                                              struct sk_buff * skb_out)
{
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_disconn_neighbor_resp_msg);

int rnl_format_ipcm_alloc_flow_req_msg(const struct name      * source,
                                       const struct name      * dest,
                                       const struct flow_spec * fspec,
                                       port_id_t	      id,
                                       const struct name      * dif_name,
                                       struct sk_buff	      * skb_out)
{
        struct nlattr * msg_src_name, * msg_dst_name;
        struct nlattr * msg_fspec,    * msg_dif_name;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        /* name-formating might be moved into its own function (and reused) */
        if (!(msg_src_name =
                nla_nest_start(skb_out, IAFRM_ATTR_SOURCE_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_src_name);
                LOG_ERR(BUILD_ERR_STRING("source application name attribute"));
                goto format_fail;
        }
	if (format_app_name_info(source, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_src_name);

        if (!(msg_dst_name =
              nla_nest_start(skb_out, IAFRM_ATTR_DEST_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_dst_name);
                LOG_ERR(BUILD_ERR_STRING("destination app name attribute"));
                goto format_fail;
        }
	if (format_app_name_info(dest, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_dst_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IAFRM_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                goto format_fail;
        }
	if (format_app_name_info(dif_name, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_dif_name);

        if (!(msg_fspec =
              nla_nest_start(skb_out, IAFRM_ATTR_FLOW_SPEC))) {
                nla_nest_cancel(skb_out, msg_fspec);
                LOG_ERR(BUILD_ERR_STRING("flow spec attribute"));
                goto format_fail;
        }
	if (format_flow_spec(fspec, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_fspec);

        if (nla_put_u32(skb_out, IAFRM_ATTR_PORT_ID, id)) 
		goto format_fail;

        return 0;

	format_fail:
                LOG_ERR("Could not format "
                        "rnl_ipcm_alloc_flow_req_msg "
                        "message correctly");
                return -1;

}
EXPORT_SYMBOL(rnl_format_ipcm_alloc_flow_req_msg);

int rnl_format_ipcm_alloc_flow_req_arrived_msg(const struct name      * source,
                                               const struct name      * dest,
                                               const struct flow_spec * fspec,
                                               const struct name      * dif_name,
                                               struct sk_buff   * skb_out)
{
        struct nlattr * msg_src_name, * msg_dst_name;
        struct nlattr * msg_fspec,    * msg_dif_name;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                goto format_fail;
        }

        /* name-formating might be moved into its own function (and reused) */
        if (!(msg_src_name =
              nla_nest_start(skb_out, IAFRA_ATTR_SOURCE_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_src_name);
                LOG_ERR(BUILD_ERR_STRING("source application name attribute"));
                goto format_fail;
        }
        if (format_app_name_info(source, skb_out) < 0)        
		goto format_fail;
	nla_nest_end(skb_out, msg_src_name);

        if (!(msg_dst_name =
              nla_nest_start(skb_out, IAFRA_ATTR_DEST_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_dst_name);
                LOG_ERR(BUILD_ERR_STRING("destination app name attribute"));
                goto format_fail;
        }
        if (format_app_name_info(dest, skb_out) < 0)        
		goto format_fail;
	nla_nest_end(skb_out, msg_dst_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IAFRA_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                goto format_fail;
        }
        if (format_app_name_info(dif_name, skb_out) < 0)        
		goto format_fail;
	nla_nest_end(skb_out, msg_dif_name);

        if (!(msg_fspec =
              nla_nest_start(skb_out, IAFRA_ATTR_FLOW_SPEC))) {
                nla_nest_cancel(skb_out, msg_fspec);
                LOG_ERR(BUILD_ERR_STRING("flow spec attribute"));
                goto format_fail;
        }
        if (format_flow_spec(fspec, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_fspec);

	return 0;

	format_fail:
                LOG_ERR("Could not format "
                        "rnl_ipcm_alloc_flow_req_arrived_msg "
                        "message correctly");
                return -1;
        
}
EXPORT_SYMBOL(rnl_format_ipcm_alloc_flow_req_arrived_msg);

int rnl_format_ipcm_alloc_flow_req_result_msg(uint_t          result,
                                              struct sk_buff  * skb_out)
{
	LOG_DBG("Entring rnl_format_ipcm_alloc_flow_req_result_msg");
	return rnl_format_generic_u32_param_msg(result,
					     IAFRRM_ATTR_RESULT, 
					     "rnl_ipcm_alloc_flow_req_result_msg",
					     skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_alloc_flow_req_result_msg);

int rnl_format_ipcm_alloc_flow_resp_msg(uint_t          result,
                                        bool            notify_src,
                                        port_id_t       id,
                                        struct sk_buff  * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IAFRE_ATTR_RESULT, result)		  ||
	    /* FIXME: I think the flag value must be specified so nla_put_flag
	     * can not be used. Check in US cause it is using it */
            nla_put(skb_out, IAFRE_ATTR_NOTIFY_SOURCE, notify_src, NULL)  ||
            nla_put_u32(skb_out, IAFRE_ATTR_PORT_ID, id )) {
                LOG_ERR("Could not format "
                        "rnl_ipcm_alloc_flow_resp_msg "
                        "message correctly");
                return -1;
        }
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_alloc_flow_resp_msg);

int rnl_format_ipcm_dealloc_flow_req_msg(port_id_t       id,
                                         struct sk_buff  * skb_out)
{
	return rnl_format_generic_u32_param_msg(id,
					     IDFRT_ATTR_PORT_ID, 
					     "rnl_ipcm_dealloc_flow_req_msg",
					     skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_dealloc_flow_req_msg);

int rnl_format_ipcm_dealloc_flow_resp_msg(uint_t          result,
                                          struct sk_buff  * skb_out)
{
	return rnl_format_generic_u32_param_msg(result,
					     IDFRE_ATTR_RESULT, 
					     "rnl_ipcm_dealloc_flow_resp_msg",
					     skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_dealloc_flow_resp_msg);

int rnl_format_ipcm_flow_dealloc_noti_msg(port_id_t       id,
                                          uint_t          code,
                                          struct sk_buff  * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

	if (nla_put_u32(skb_out, IFDN_ATTR_PORT_ID, id)	 ||
            nla_put_u32(skb_out, IFDN_ATTR_CODE, code )) {
                LOG_ERR("Could not format "
                        "rnl_ipcm_dealloc_flow_resp_msg "
                        "message correctly");
                return -1;
        }
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_flow_dealloc_noti_msg);

int rnl_format_ipcm_reg_app_req_msg(const struct name     * app_name,
                                    const struct name     * dif_name,
                                    struct sk_buff  * skb_out)
{
        struct nlattr * msg_src_name, * msg_dif_name;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_src_name =
              nla_nest_start(skb_out, IRAR_ATTR_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_src_name);
                LOG_ERR(BUILD_ERR_STRING("source application name attribute"));
                goto format_fail;
        }
        if (format_app_name_info(app_name, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_src_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IRAR_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                goto format_fail;
        }
        if (format_app_name_info(dif_name, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_dif_name);

        return 0;

	format_fail:
                LOG_ERR("Could not format "
                        "rnl_ipcm_reg_app_req_msg "
                        "message correctly");
                return -1;

}
EXPORT_SYMBOL(rnl_format_ipcm_reg_app_req_msg);

/* FIXME: Not sure if app_name is needed here */
int rnl_format_ipcm_reg_app_resp_msg(const struct name * app_name,
				     uint_t            result,
                                     struct sk_buff    * skb_out)
{
        struct nlattr * msg_src_name;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_src_name =
              nla_nest_start(skb_out, IRARE_ATTR_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_src_name);
                LOG_ERR(BUILD_ERR_STRING("source application name attribute"));
                goto format_fail;
        }
        if (format_app_name_info(app_name, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_src_name);

	return 0;
	
	format_fail:
                LOG_ERR("Could not format "
                        "rnl_ipcm_reg_app_resp_msg "
                        "message correctly");
                return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_reg_app_resp_msg);

int rnl_format_ipcm_unreg_app_req_msg(const struct name     * app_name,
                                      const struct name     * dif_name,
                                      struct sk_buff  * skb_out)
{
        struct nlattr * msg_src_name, * msg_dif_name;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_src_name =
              nla_nest_start(skb_out, IUAR_ATTR_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_src_name);
                LOG_ERR(BUILD_ERR_STRING("source application name attribute"));
                goto format_fail;
        }
        if (format_app_name_info(app_name, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_src_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IUAR_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                goto format_fail;
        }
        if (format_app_name_info(dif_name, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_dif_name);

        return 0;

	format_fail:
                LOG_ERR("Could not format "
                        "rnl_ipcm_unreg_app_req_msg "
                        "message correctly");
                return -1;

}
EXPORT_SYMBOL(rnl_format_ipcm_unreg_app_req_msg);

int rnl_format_ipcm_unreg_app_resp_msg(uint_t          result,
                                       struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IUARE_ATTR_RESULT, result) < 0){
                LOG_ERR("Could not format "
                        "rnl_ipcm_unreg_app_resp_msg "
                        "message correctly");
                return -1;
        }
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_unreg_app_resp_msg);

static int format_rib_object(const struct rib_object * obj, 
			     struct sk_buff          * msg)
{
        if (!msg) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (obj->rib_obj_class)
                if (nla_put_u32(msg,
                                RIBO_ATTR_OBJECT_CLASS,
                                obj->rib_obj_class))
                        return -1;
        if (obj->rib_obj_name)
                if (nla_put_string(msg,
                                   RIBO_ATTR_OBJECT_NAME,
                                   obj->rib_obj_name))
                        return -1;
        if (obj->rib_obj_instance)
                if (nla_put_u32(msg,
                                RIBO_ATTR_OBJECT_INSTANCE,
                                obj->rib_obj_instance))
                        return -1;

        return 0;
}

static int format_rib_objects_list(const struct rib_object ** objs,
				  uint_t                  count,
				  struct sk_buff	  * msg)
{
	int i;
	struct nlattr * msg_obj;
	
	for (i=0; i< count; i++){
        	if (!(msg_obj =
        		nla_nest_start(msg, i))) {
        	        nla_nest_cancel(msg, msg_obj);
        	        LOG_ERR(BUILD_ERR_STRING("rib object attribute"));
        	        return -1;
        	}
		if(format_rib_object(objs[i], msg) < 0)
			return -1;
		nla_nest_end(msg, msg_obj);
	}
	
	return 0;

}

int rnl_format_ipcm_query_rib_req_msg(const struct rib_object * obj,
                                      uint_t                  scope,
                                      const const regex_t     * filter,
                                      struct sk_buff          * skb_out)
{
	struct nlattr * msg_obj;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_obj =
        	nla_nest_start(skb_out, IDQR_ATTR_OBJECT))) {
                nla_nest_cancel(skb_out, msg_obj);
                LOG_ERR(BUILD_ERR_STRING("rib object attribute"));
                goto format_fail;
        }
        if (format_rib_object(obj, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_obj);

        if (nla_put_u32(skb_out, IDQR_ATTR_SCOPE, scope)      ||
	    nla_put_string(skb_out, IDQR_ATTR_FILTER, filter)) 
	    	goto format_fail;

	return 0;

	format_fail:
                LOG_ERR("Could not format "
                        "rnl_ipcm_query_rib_req_msg "
                        "message correctly");
                return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_query_rib_req_msg);

int rnl_format_ipcm_query_rib_resp_msg(uint_t                  result,
				       uint_t		       count,
				       const struct rib_object ** objs,
                                       struct sk_buff          * skb_out)
{
	struct nlattr * msg_objs;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_objs =
        	nla_nest_start(skb_out, IDQRE_ATTR_RIB_OBJECTS))) {
                nla_nest_cancel(skb_out, msg_objs);
                LOG_ERR(BUILD_ERR_STRING("rib object list attribute"));
                goto format_fail;
        }
        if (format_rib_objects_list(objs, count, skb_out) < 0)
		goto format_fail;
	nla_nest_end(skb_out, msg_objs);
	

        if (nla_put_u32(skb_out, IDQRE_ATTR_RESULT, result) ||
	    nla_put_u32(skb_out, IDQRE_ATTR_COUNT, count))
		goto format_fail;

	return 0;

	format_fail:
                LOG_ERR("Could not format "
                        "rnl_ipcm_query_rib_resp_msg "
                        "message correctly");
                return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_query_rib_resp_msg);

int rnl_format_rmt_add_fte_req_msg(const struct pdu_ft_entry * entry,
                                   struct sk_buff      * skb_out)
{
        return 0;
}
EXPORT_SYMBOL(rnl_format_rmt_add_fte_req_msg);

int rnl_format_rmt_del_fte_req_msg(const struct pdu_ft_entry *entry,
                                   struct sk_buff      * skb_out)
{
        return 0;
}
EXPORT_SYMBOL(rnl_format_rmt_del_fte_req_msg);

