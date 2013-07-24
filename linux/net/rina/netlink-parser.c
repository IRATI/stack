/*
 * NetLink support
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

#define RINA_PREFIX "netlink-parser"

#include "logs.h"
#include "common.h"
#include "debug.h"
#include "netlink.h"
#include "netlink-parser.h"

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
 * Francesco
 */

/*
 * FIXME: Destination is usually at the end of the prototype, not at the
 * beginning (e.g. msg and name)
 */
static int craft_app_name_info(struct sk_buff * msg,
                               struct name      name)
{
        if (!msg) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        /* FIXME: Always look for terminators here, the might be missing ... */
        /*
         * FIXME: Does nla_put_string returns anything ? In the affermative
         *        case: it must be checked
         */
        nla_put_string(msg, APNI_ATTR_PROCESS_NAME,     name.process_name);
        nla_put_string(msg, APNI_ATTR_PROCESS_INSTANCE, name.process_instance);
        nla_put_string(msg, APNI_ATTR_ENTITY_NAME,      name.entity_name);
        nla_put_string(msg, APNI_ATTR_ENTITY_INSTANCE,  name.entity_instance);

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
        if (fspec.average_bandwidth->max > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_AVG_BWITH,
                            fspec.average_bandwidth->max);
        if (fspec.average_sdu_bandwidth->max > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_AVG_SDU_BWITH,
                            fspec.average_sdu_bandwidth->max);
        if (fspec.delay > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_DELAY,
                            fspec.delay);
        if (fspec.jitter > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_JITTER,
                            fspec.jitter);
        if (fspec.max_allowable_gap > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_MAX_GAP,
                            fspec.max_allowable_gap);
        if (fspec.max_sdu_size > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_MAX_SDU_SIZE,
                            fspec.max_sdu_size);
        if (fspec.ordered_delivery > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_IN_ORD_DELIVERY,
                            fspec.ordered_delivery);
        if (fspec.partial_delivery > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_PART_DELIVERY,
                            fspec.partial_delivery);
        if (fspec.peak_bandwidth_duration->max > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_PEAK_BWITH_DURATION,
                            fspec.peak_bandwidth_duration->max);
        if (fspec.peak_sdu_bandwidth_duration->max > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_PEAK_SDU_BWITH_DURATION,
                            fspec.peak_sdu_bandwidth_duration->max);
        if (fspec.peak_sdu_bandwidth_duration->max > 0)
                nla_put_u32(msg,
                            FSPEC_ATTR_UNDETECTED_BER,
                            fspec.undetected_bit_error_rate);

        return 0;
}

#define BUILD_ERR_STRING(X)                                     \
	"Netlink message does not contain " X ", bailing out"

static int craft_app_alloc_flow_req_arrived_msg(struct sk_buff * msg,
                                                struct name      source,
                                                struct name      dest,
                                                struct flow_spec fspec,
                                                port_id_t        id,
                                                struct name      dif_name)
{
        /* FIXME: What's the use of the following variables ? */
        struct nlattr * msg_src_name, * msg_dst_name;
        struct nlattr * msg_fspec,    * msg_dif_name;

        if (!msg) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_src_name =
              nla_nest_start(msg, AAFRA_ATTR_SOURCE_APP_NAME))) {
                nla_nest_cancel(msg, msg_src_name);
                LOG_ERR(BUILD_ERR_STRING("source application name attribute"));
                goto craft_fail;
        }

        if (!(msg_dst_name =
              nla_nest_start(msg, AAFRA_ATTR_DEST_APP_NAME))) {
                nla_nest_cancel(msg, msg_dst_name);
                LOG_ERR(BUILD_ERR_STRING("destination app name attribute"));
                goto craft_fail;
        }

        if (!(msg_dif_name =
              nla_nest_start(msg, AAFRA_ATTR_DIF_NAME))) {
                nla_nest_cancel(msg, msg_dif_name);
                LOG_ERR(BUILD_ERR_STRING("DIF name attribute"));
                goto craft_fail;
        }

        if (!(msg_fspec =
              nla_nest_start(msg, AAFRA_ATTR_FLOW_SPEC))) {
                nla_nest_cancel(msg, msg_fspec);
                LOG_ERR(BUILD_ERR_STRING("flow spec attribute"));
                goto craft_fail;
        }

        if (craft_app_name_info(msg, source) < 0)
                goto craft_fail;

        if (craft_app_name_info(msg, dest) < 0)
                goto craft_fail;

        if (craft_flow_spec(msg, fspec) < 0)
                goto craft_fail;

        nla_put_u32(msg, AAFRA_ATTR_PORT_ID, id);

        if (craft_app_name_info(msg, dif_name) < 0)
                goto craft_fail;

        return 0;

 craft_fail:
        LOG_ERR("Could not craft _app_alloc_flow_req_arrived_msg message");
        return -1;
}
