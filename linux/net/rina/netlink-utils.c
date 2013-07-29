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

struct rina_msg_hdr{
        unsigned int src_ipc_id;
        unsigned int dst_ipc_id;
};

#if 0
struct rnl_msg{
            
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
#endif

static int craft_app_name_info(struct sk_buff * msg,
                               struct name      name)
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

        if (name.process_name)
                if (nla_put_string(msg,
                                   APNI_ATTR_PROCESS_NAME,
                                   name.process_name))
                        return -1;
        if (name.process_instance)
                if (nla_put_string(msg,
                                   APNI_ATTR_PROCESS_INSTANCE,
                                   name.process_instance))
                        return -1;
        if (name.entity_name)
                if (nla_put_string(msg,
                                   APNI_ATTR_ENTITY_NAME,
                                   name.entity_name))
                        return -1;
        if (name.entity_instance)
                if (nla_put_string(msg,
                                   APNI_ATTR_ENTITY_INSTANCE,
                                   name.entity_instance))
                        return -1;
        
        return 0;
}

static int craft_flow_spec(struct sk_buff * msg,
                           struct flow_spec fspec)
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

        if (fspec.average_bandwidth->max > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_AVG_BWITH,
                                fspec.average_bandwidth->max))
                        return -1;
        if (fspec.average_sdu_bandwidth->max > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_AVG_SDU_BWITH,
                                fspec.average_sdu_bandwidth->max))
                        return -1;
        if (fspec.delay > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_DELAY,
                                fspec.delay))
                        return -1;
        if (fspec.jitter > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_JITTER,
                                fspec.jitter))
                        return -1;
        if (fspec.max_allowable_gap > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_MAX_GAP,
                                fspec.max_allowable_gap))
                        return -1;
        if (fspec.max_sdu_size > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_MAX_SDU_SIZE,
                                fspec.max_sdu_size))
                        return -1;
        if (fspec.ordered_delivery > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_IN_ORD_DELIVERY,
                                fspec.ordered_delivery))
                        return -1;
        if (fspec.partial_delivery > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_PART_DELIVERY,
                                fspec.partial_delivery))
                        return -1;
        if (fspec.peak_bandwidth_duration->max > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_PEAK_BWITH_DURATION,
                                fspec.peak_bandwidth_duration->max))
                        return -1;
        if (fspec.peak_sdu_bandwidth_duration->max > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_PEAK_SDU_BWITH_DURATION,
                                fspec.peak_sdu_bandwidth_duration->max))
                        return -1;
        if (fspec.undetected_bit_error_rate > 0)
                if (nla_put_u32(msg,
                            FSPEC_ATTR_UNDETECTED_BER,
                            fspec.undetected_bit_error_rate))
                        return -1;
//        if (fspec.undetected_bit_error_rate > 0)
//                if (nla_put(msg,
//                            FSPEC_ATTR_UNDETECTED_BER,
//                            sizeof(fspec.undetected_bit_error_rate),
//                            &fspec.undetected_bit_error_rate))
//                        return -1;

        return 0;
}

#define BUILD_ERR_STRING(X)                                     \
	"Netlink message does not contain " X ", bailing out"

int rnl_format_app_alloc_flow_req_arrived(struct sk_buff * msg,
                                          struct name      source,
                                          struct name      dest,
                                          struct flow_spec fspec,
                                          port_id_t        id,
                                          struct name      dif_name)
{
        /* FIXME: What's the use of the following variables ? */
	/* FIXME: they are the placeholder of the nest attr returned by
	 * nla_nest_start, if not used, then nla_nest_start should be called
	 * twice each time */
        struct nlattr * msg_src_name, * msg_dst_name;
        struct nlattr * msg_fspec,    * msg_dif_name;

        if (!msg) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        /* name-crafting might be moved into its own function (and reused) */
        if (!(msg_src_name =
              nla_nest_start(msg, AAFRA_ATTR_SOURCE_APP_NAME))) {
                nla_nest_cancel(msg, msg_src_name);
                LOG_ERR(BUILD_ERR_STRING("source application name attribute"));
                return -1;
        }

        if (!(msg_dst_name =
              nla_nest_start(msg, AAFRA_ATTR_DEST_APP_NAME))) {
                nla_nest_cancel(msg, msg_dst_name);
                LOG_ERR(BUILD_ERR_STRING("destination app name attribute"));
                return -1;
        }

        if (!(msg_dif_name =
              nla_nest_start(msg, AAFRA_ATTR_DIF_NAME))) {
                nla_nest_cancel(msg, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                return -1;
        }

        if (!(msg_fspec =
              nla_nest_start(msg, AAFRA_ATTR_FLOW_SPEC))) {
                nla_nest_cancel(msg, msg_fspec);
                LOG_ERR(BUILD_ERR_STRING("flow spec attribute"));
                return -1;
        }

        /* Boolean shortcuiting here */
        if (craft_app_name_info(msg, source)         ||
            craft_app_name_info(msg, dest)           ||
            craft_flow_spec(msg,     fspec)          ||
            nla_put_u32(msg, AAFRA_ATTR_PORT_ID, id) ||
            craft_app_name_info(msg, dif_name)) {
                LOG_ERR("Could not craft "
                        "_app_alloc_flow_req_arrived_msg "
                        "message correctly");
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rnl_format_app_alloc_flow_req_arrived);

#define BUILD_ERR_STRING_BY_MSG_TYPE(X)                                     \
         "Could not parse Netlink message of type "X

int rnl_parse_msg(struct genl_info * info, int max_attr, struct nla_policy * attr_policy)
{
	int err;

	err = nlmsg_parse(info->nlhdr, 
	      sizeof(struct genlmsghdr) + sizeof(struct rina_msg_hdr), 
	      info->attrs,
              max_attr, 
	      attr_policy);
	if(err < 0){
		LOG_ERR(BUILD_ERR_STRING_BY_MSG_TYPE("RINA_C_APP_ALLOCATE_FLOW_RESPONSE"));
		return -1;
	}
	return 0;
}

int rnl_parse_app_alloc_flow_resp(struct sk_buff * skb_in, struct genl_info * info)
{
        struct nla_policy attr_policy[AAFRE_ATTR_MAX + 1];
        
	/* FIXME: nla_policy struct is different from user-space. No min/max
	 * attrs. len not specified */
	attr_policy[AAFRE_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[AAFRE_ATTR_ACCEPT].type = NLA_FLAG;
        attr_policy[AAFRE_ATTR_DENY_REASON].type = NLA_STRING;
        attr_policy[AAFRE_ATTR_NOTIFY_SOURCE].type = NLA_FLAG;

	/* Any other comprobations could be done in addition to nlmsg_parse()
	 * done by rnl_parse_msg */

	return rnl_parse_msg(info, AAFRA_ATTR_MAX, attr_policy);

}
EXPORT_SYMBOL(rnl_parse_app_alloc_flow_resp);
