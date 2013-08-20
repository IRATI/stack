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
#if 0
/* This is an implementation of the function before redefining the API,
   but could be useful to the the new one */

static int rnl_format_ipcm_alloc_flow_req_arrived(struct sk_buff * skb_in,
                                                  struct \
                                                  rnl_ipcm_alloc_flow_req_arrived_msg_attrs\
                                                  * msg_attrs)
{
        /* FIXME: What's the use of the following variables ? */
        /* FIXME: they are the placeholder of the nest attr returned by
         * nla_nest_start, if not used, then nla_nest_start should be called
         * twice each time */
        struct nlattr * msg_src_name, * msg_dst_name;
        struct nlattr * msg_fspec,    * msg_dif_name;

        if (!skb_in) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        /* name-formating might be moved into its own function (and reused) */
        if (!(msg_src_name =
              nla_nest_start(skb_in, IAFRAATTR_SOURCE_APP_NAME))) {
                nla_nest_cancel(skb_in, msg_src_name);
                LOG_ERR(BUILD_ERR_STRING("source application name attribute"));
                return -1;
        }

        if (!(msg_dst_name =
              nla_nest_start(skb_in, IAFRAATTR_DEST_APP_NAME))) {
                nla_nest_cancel(skb_in, msg_dst_name);
                LOG_ERR(BUILD_ERR_STRING("destination app name attribute"));
                return -1;
        }

        if (!(msg_dif_name =
              nla_nest_start(skb_in, IAFRAATTR_DIF_NAME))) {
                nla_nest_cancel(skb_in, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                return -1;
        }

        if (!(msg_fspec =
              nla_nest_start(skb_in, IAFRAATTR_FLOW_SPEC))) {
                nla_nest_cancel(skb_in, msg_fspec);
                LOG_ERR(BUILD_ERR_STRING("flow spec attribute"));
                return -1;
        }

        /* Boolean shortcuiting here */
        if (format_app_name_info(skb_in, msg_attrs->source)         ||
            format_app_name_info(skb_in, msg_attrs->dest)           ||
            format_flow_spec(skb_in,     msg_attrs->fspec)          ||
            nla_put_u32(skb_in, IAFRAATTR_PORT_ID, msg_attrs->id) ||
            format_app_name_info(skb_in, msg_attrs->dif_name)) {
                LOG_ERR("Could not format "
                        "_ipcm_alloc_flow_req_arrived_msg "
                        "message correctly");
                return -1;
        }

        return 0;
}
#endif

#define BUILD_ERR_STRING_BY_MSG_TYPE(X)                 \
        "Could not parse Netlink message of type "X

static int rnl_simple_parse_msg(struct genl_info * info, int max_attr,
                                struct nla_policy * attr_policy)
{
        int err;

        err = nlmsg_parse(info->nlhdr,
                          /* FIXME: Check if this is correct */
                          sizeof(struct genlmsghdr) +
                          sizeof(struct rina_msg_hdr),
                          info->attrs,
                          max_attr,
                          attr_policy);
        if (err < 0)
                return -1;
        return 0;
}

static int parse_flow_spec(struct nlattr * fspec_attr,
                           struct flow_spec * fspec_struct)
{
        struct nla_policy attr_policy[FSPEC_ATTR_MAX + 1];
        struct nlattr *attrs[FSPEC_ATTR_MAX + 1];
        int err;

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

        err = nla_parse_nested(attrs, FSPEC_ATTR_MAX, fspec_attr, attr_policy);
        goto fail;

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

 fail:
        return -1;
}

static int parse_app_name_info(struct nlattr * name_attr,
                               struct name * name_struct)
{
        struct nla_policy attr_policy[APNI_ATTR_MAX + 1];
        struct nlattr *attrs[APNI_ATTR_MAX + 1];
        int err;

        attr_policy[APNI_ATTR_PROCESS_NAME].type = NLA_STRING;
        attr_policy[APNI_ATTR_PROCESS_INSTANCE].type = NLA_STRING;
        attr_policy[APNI_ATTR_ENTITY_NAME].type = NLA_STRING;
        attr_policy[APNI_ATTR_ENTITY_INSTANCE].type = NLA_STRING;

        err = nla_parse_nested(attrs, APNI_ATTR_MAX, name_attr, attr_policy);
        if (err < 0)
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

static int rnl_parse_ipcm_alloc_flow_req(struct genl_info * info,
                                         struct rnl_ipcm_alloc_flow_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRM_ATTR_MAX + 1];

        attr_policy[IAFRM_ATTR_SOURCE_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRM_ATTR_DEST_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRM_ATTR_FLOW_SPEC].type = NLA_NESTED;
        attr_policy[IAFRM_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[IAFRM_ATTR_PORT_ID].type = NLA_U32;

        if (rnl_simple_parse_msg(info, IAFRM_ATTR_MAX, attr_policy) < 0)
                goto fail;

        if (parse_app_name_info(info->attrs[IAFRM_ATTR_SOURCE_APP_NAME],
                                &msg_attrs->source) < 0)
                goto fail;

        if (parse_app_name_info(info->attrs[IAFRM_ATTR_DEST_APP_NAME],
                                &msg_attrs->dest) < 0)
                goto fail;

        if (parse_flow_spec(info->attrs[IAFRM_ATTR_FLOW_SPEC],
                            &msg_attrs->fspec) < 0);
        goto fail;

        if (info->attrs[IAFRM_ATTR_PORT_ID])
                msg_attrs->id = \
                        (port_id_t) nla_get_u32(info->attrs[IAFRM_ATTR_PORT_ID]);

        if (parse_app_name_info(info->attrs[IAFRM_ATTR_DIF_NAME],
                                &msg_attrs->dif_name) < 0)
                goto fail;

        return 0;

 fail:
        LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_ALLOCATE_FLOW_REQUEST"));
        return -1;
}


static int rnl_parse_ipcm_alloc_flow_req_arrived(struct genl_info * info,
                                                 struct rnl_ipcm_alloc_flow_req_arrived_msg_attrs * msg_attrs)
{
        return 0;
}

static int rnl_parse_ipcm_alloc_flow_resp(struct genl_info * info,
                                          struct rnl_alloc_flow_resp_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRE_ATTR_MAX + 1];

        /* FIXME: nla_policy struct is different from user-space. No min/max
         * attrs. len not specified */
        attr_policy[IAFRE_ATTR_RESULT].type = NLA_U32;
        attr_policy[IAFRE_ATTR_NOTIFY_SOURCE].type = NLA_FLAG;
        attr_policy[IAFRE_ATTR_PORT_ID].type = NLA_U32;

        /* Any other comprobations could be done in addition to nlmsg_parse()
         * done by rnl_simple_parse_msg */

        if (rnl_simple_parse_msg(info, IAFRE_ATTR_MAX, attr_policy) < 0)
                goto fail;

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

 	fail:
        	LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_ALLOCATE_FLOW_RESPONSE"));
        	return -1;

}

static int rnl_parse_ipcm_dealloc_flow_req(struct genl_info * info,
                                           struct rnl_ipcm_dealloc_flow_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IDFRT_ATTR_MAX + 1];

        attr_policy[IDFRT_ATTR_PORT_ID].type = NLA_U32;

        if (rnl_simple_parse_msg(info, IDFRT_ATTR_MAX, attr_policy) < 0)
                goto fail;

        if (info->attrs[IDFRT_ATTR_PORT_ID])
                msg_attrs->id       = \
                        (port_id_t) nla_get_u32(info->attrs[IDFRT_ATTR_PORT_ID]);

        return 0;

 fail:
        LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST"));
        return -1;

}


static int rnl_parse_ipcm_dealloc_flow_resp(struct genl_info * info,
                                            struct rnl_ipcm_dealloc_flow_resp_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IDFRE_ATTR_MAX + 1];

        attr_policy[IDFRE_ATTR_RESULT].type = NLA_U32;

        if (rnl_simple_parse_msg(info, IDFRE_ATTR_MAX, attr_policy) < 0)
                goto fail;

        if (info->attrs[IDFRE_ATTR_RESULT])
                msg_attrs->result       = \
                        (port_id_t) nla_get_u32(info->attrs[IDFRE_ATTR_RESULT]);

        return 0;

 fail:
        LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE"));
        return -1;
}

#if 0
ipc_process_id_t rnl_src_ipcid_from_msg(struct genl_info * info)
{
        struct rina_msg_hdr * usr_hdr = info->userhdr;
        return usr_hdr->src_ipc_id;
}
EXPORT_SYMBOL(rnl_src_ipcid_from_msg);

ipc_process_id_t rnl_dst_ipcid_from_msg(struct genl_info * info)
{
        struct rina_msg_hdr * usr_hdr = info->userhdr;
        return usr_hdr->src_ipc_id;
}
EXPORT_SYMBOL(rnl_dst_ipcid_from_msg);
#endif

int rnl_parse_msg(struct genl_info      * info,
                  struct rnl_msg        * msg)
{
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
        switch(info->genlhdr->cmd){
        case RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST:
                msg->attrs = 0;
                break;
        case RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE:
                break;
        case RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION:
                break;
        case RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION:
                break;
        case RINA_C_IPCM_ENROLL_TO_DIF_REQUEST:
                break;
        case RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE:
                break;
        case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST:
                break;
        case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE:
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST:
                if (rnl_parse_ipcm_alloc_flow_req(info, msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED:
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE:
                if (rnl_parse_ipcm_alloc_flow_resp(info, msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST:
                if (rnl_parse_ipcm_dealloc_flow_req(info, msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE:
                if (rnl_parse_ipcm_dealloc_flow_resp(info, msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_REGISTER_APPLICATION_REQUEST:
                break;
        case RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE:
                break;
        case RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST:
                break;
        case RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE:
                break;
        case RINA_C_IPCM_QUERY_RIB_REQUEST:
                break;
        case RINA_C_IPCM_QUERY_RIB_RESPONSE:
                break;
        case RINA_C_RMT_ADD_FTE_REQUEST:
                break;
        case RINA_C_RMT_DELETE_FTE_REQUEST:
                break;
        case RINA_C_RMT_DUMP_FT_REQUEST:
                break;
        case RINA_C_RMT_DUMP_FT_REPLY:
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


int rnl_format_ipcm_assign_to_dif_req_msg(const struct dif_config  * config,
                                          struct sk_buff  * skb_out)
{
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_assign_to_dif_req_msg);

int rnl_format_ipcm_assign_to_dif_resp_msg(uint_t          result,
                                           struct sk_buff  * skb_out)
{
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_assign_to_dif_resp_msg);

int rnl_format_ipcm_ipcp_dif_reg_noti_msg(const struct name     * ipcp_name,
                                          const struct name     * dif_name,
                                          bool            is_registered,
                                          struct sk_buff  * skb_out)
{
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_ipcp_dif_reg_noti_msg);

int rnl_format_ipcm_ipcp_dif_unreg_noti_msg(uint_t          result,
                                            struct sk_buff  * skb_out)
{
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_ipcp_dif_unreg_noti_msg);

int rnl_format_ipcm_enroll_to_dif_req_msg(const struct name    * dif_name,
                                          struct sk_buff * skb_out)
{
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_enroll_to_dif_req_msg);

int rnl_format_ipcm_enroll_to_dif_resp_msg(uint_t         result,
                                           struct sk_buff * skb_out)
{
        return 0;
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
                return -1;
        }

        if (!(msg_dst_name =
              nla_nest_start(skb_out, IAFRM_ATTR_DEST_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_dst_name);
                LOG_ERR(BUILD_ERR_STRING("destination app name attribute"));
                return -1;
        }

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IAFRM_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                return -1;
        }

        if (!(msg_fspec =
              nla_nest_start(skb_out, IAFRM_ATTR_FLOW_SPEC))) {
                nla_nest_cancel(skb_out, msg_fspec);
                LOG_ERR(BUILD_ERR_STRING("flow spec attribute"));
                return -1;
        }

        /* Boolean shortcuiting here */
        if (format_app_name_info(source, skb_out)         ||
            format_app_name_info(dest, skb_out)           ||
            format_flow_spec(fspec, skb_out)              ||
            nla_put_u32(skb_out, IAFRM_ATTR_PORT_ID, id)  ||
            format_app_name_info(dif_name, skb_out)) {
                LOG_ERR("Could not format "
                        "rnl_ipcm_alloc_flow_req_msg "
                        "message correctly");
                return -1;
        }

        return 0;
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
                return -1;
        }

        /* name-formating might be moved into its own function (and reused) */
        if (!(msg_src_name =
              nla_nest_start(skb_out, IAFRA_ATTR_SOURCE_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_src_name);
                LOG_ERR(BUILD_ERR_STRING("source application name attribute"));
                return -1;
        }

        if (!(msg_dst_name =
              nla_nest_start(skb_out, IAFRA_ATTR_DEST_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_dst_name);
                LOG_ERR(BUILD_ERR_STRING("destination app name attribute"));
                return -1;
        }

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IAFRA_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                return -1;
        }

        if (!(msg_fspec =
              nla_nest_start(skb_out, IAFRA_ATTR_FLOW_SPEC))) {
                nla_nest_cancel(skb_out, msg_fspec);
                LOG_ERR(BUILD_ERR_STRING("flow spec attribute"));
                return -1;
        }

        /* Boolean shortcuiting here */
        if (format_app_name_info(source, skb_out)         ||
            format_app_name_info(dest, skb_out)           ||
            format_flow_spec(fspec, skb_out)              ||
            format_app_name_info(dif_name, skb_out)) {
                LOG_ERR("Could not format "
                        "rnl_ipcm_alloc_flow_req_arrived_msg "
                        "message correctly");
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_alloc_flow_req_arrived_msg);

int rnl_format_ipcm_alloc_flow_req_result_msg(uint_t          result,
                                              struct sk_buff  * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

	if (nla_put_u32(skb_out, IAFRRM_ATTR_RESULT, result) < 0) {
                LOG_ERR("Could not format "
                        "rnl_ipcm_alloc_flow_req_result_msg "
                        "message correctly");
                return -1;
	}
        return 0;
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
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IDFRT_ATTR_PORT_ID, id) < 0){
                LOG_ERR("Could not format "
                        "rnl_ipcm_dealloc_flow_req_msg "
                        "message correctly");
                return -1;
        }
        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_dealloc_flow_req_msg);

int rnl_format_ipcm_dealloc_flow_resp_msg(uint_t          result,
                                          struct sk_buff  * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IDFRE_ATTR_RESULT, result) < 0){
                LOG_ERR("Could not format "
                        "rnl_ipcm_dealloc_flow_resp_msg "
                        "message correctly");
                return -1;
        }
        return 0;
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
                return -1;
        }

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IRAR_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                return -1;
        }

        if (format_app_name_info(app_name, skb_out) ||
            format_app_name_info(dif_name, skb_out)) {
                LOG_ERR("Could not format "
                        "rnl_ipcm_reg_app_req_msg "
                        "message correctly");
                return -1;
        }

        return 0;
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
                return -1;
        }

        if (format_app_name_info(app_name, skb_out) ||
            nla_put_u32(skb_out, IRARE_ATTR_RESULT, result )) {
                LOG_ERR("Could not format "
                        "rnl_ipcm_reg_app_resp_msg "
                        "message correctly");
                return -1;
        }

        return 0;
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
                return -1;
        }

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IUAR_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                return -1;
        }

        if (format_app_name_info(app_name, skb_out) ||
            format_app_name_info(dif_name, skb_out)) {
                LOG_ERR("Could not format "
                        "rnl_ipcm_unreg_app_req_msg "
                        "message correctly");
                return -1;
        }

        return 0;
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

#if 0
static int format_rib_object_list(const struct rib_object ** objs,
				  uint_t                  count,
				  struct sk_buff	  * msg)
{
	int i;
	struct nlattr ** msg_objs;
	
	msg_objs = rkmalloc(count * sizeof(struct nlattr *), GFP_KERNEL);
	
	for (i=0; i< count; i++){
        	if (!(msg_objs[i] =
        		nla_nest_start(msg, i))) {
        	        nla_nest_cancel(msg, msg_objs[i]);
        	        LOG_ERR(BUILD_ERR_STRING("rib object attribute"));
        	        return -1;
        	}
	}
	for (i; i< count; i++){
		if (format_rib_object(objs[i], msg) < 0)
			return -1
	}

	return 0;

}
#endif 

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
                return -1;
        }

        if (format_rib_object(obj, skb_out)		      ||
	    nla_put_u32(skb_out, IDQR_ATTR_SCOPE, scope)      ||
	    nla_put_string(skb_out, IDQR_ATTR_FILTER, filter)) {
                LOG_ERR("Could not format "
                        "rnl_ipcm_query_rib_req_msg "
                        "message correctly");
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rnl_format_ipcm_query_rib_req_msg);

int rnl_format_ipcm_query_rib_resp_msg(uint_t                  result,
				       uint_t		       count,
				       const struct rib_object ** objs,
                                       struct sk_buff          * skb_out)
{
#if 0
	int i;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }


        if (nla_put_u32(skb_out, IDQRE_ATTR_RESULT, result) ||
	    nla_put_u32(skb_out, IDQRE_ATTR_COUNT, count)   ||
	    format_rib_objects_list(objs, skb_out)) {
                LOG_ERR("Could not format "
                        "rnl_ipcm_query_rib_resp_msg "
                        "message correctly");
                return -1;
        }
#endif

        return 0;
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

