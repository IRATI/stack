/*
 * RNL utilities
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

#include <linux/export.h>
#include <net/netlink.h>
#include <net/genetlink.h>

#define RINA_PREFIX "rnl-utils"

#include "logs.h"
#include "common.h"
#include "debug.h"
#include "ipcp-utils.h"
#include "utils.h"
#include "rnl.h"
#include "rnl-utils.h"

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

#define BUILD_STRERROR(X)                                       \
        "Netlink message does not contain " X ", bailing out"

#define BUILD_STRERROR_BY_MTYPE(X)                      \
        "Could not parse Netlink message of type " X

extern struct genl_family rnl_nl_family;

char * nla_get_string(struct nlattr * nla)
{ return (char *) nla_data(nla); }

static int rnl_check_attr_policy(struct nlmsghdr *   nlh,
                                 int                 max_attr,
                                 struct nla_policy * attr_policy)
{
        struct nlattr * attrs[max_attr + 1];
        int             result;

        result = nlmsg_parse(nlh,
                             /* FIXME: Check if this is correct */
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             max_attr,
                             attr_policy);
        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy",
                        result);
                return -1;
        }

        return 0;
}

static int parse_flow_spec(struct nlattr * fspec_attr,
                           struct flow_spec * fspec_struct)
{
        struct nla_policy attr_policy[FSPEC_ATTR_MAX + 1];
        struct nlattr *   attrs[FSPEC_ATTR_MAX + 1];

        attr_policy[FSPEC_ATTR_AVG_BWITH].type = NLA_U32;
        attr_policy[FSPEC_ATTR_AVG_BWITH].len = 4;
        attr_policy[FSPEC_ATTR_AVG_SDU_BWITH].type = NLA_U32;
        attr_policy[FSPEC_ATTR_AVG_SDU_BWITH].len = 4;
        attr_policy[FSPEC_ATTR_DELAY].type = NLA_U32;
        attr_policy[FSPEC_ATTR_DELAY].len = 4;
        attr_policy[FSPEC_ATTR_JITTER].type = NLA_U32;
        attr_policy[FSPEC_ATTR_JITTER].len = 4;
        attr_policy[FSPEC_ATTR_MAX_GAP].type = NLA_U32;
        attr_policy[FSPEC_ATTR_MAX_GAP].len = 4;
        attr_policy[FSPEC_ATTR_MAX_SDU_SIZE].type = NLA_U32;
        attr_policy[FSPEC_ATTR_MAX_SDU_SIZE].len = 4;
        attr_policy[FSPEC_ATTR_IN_ORD_DELIVERY].type = NLA_FLAG;
        attr_policy[FSPEC_ATTR_IN_ORD_DELIVERY].len = 0;
        attr_policy[FSPEC_ATTR_PART_DELIVERY].type = NLA_FLAG;
        attr_policy[FSPEC_ATTR_PART_DELIVERY].len = 0;
        attr_policy[FSPEC_ATTR_PEAK_BWITH_DURATION].type = NLA_U32;
        attr_policy[FSPEC_ATTR_PEAK_BWITH_DURATION].len = 4;
        attr_policy[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION].type = NLA_U32;
        attr_policy[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION].len = 4;
        attr_policy[FSPEC_ATTR_UNDETECTED_BER].type = NLA_U32;
        attr_policy[FSPEC_ATTR_UNDETECTED_BER].len = 4;

        if (nla_parse_nested(attrs,
                             FSPEC_ATTR_MAX,
                             fspec_attr,
                             attr_policy) < 0)
                return -1;

        if (attrs[FSPEC_ATTR_AVG_BWITH])
                /* FIXME: min = max in uint_range */
                fspec_struct->average_bandwidth =
                        nla_get_u32(attrs[FSPEC_ATTR_AVG_BWITH]);

        if (attrs[FSPEC_ATTR_AVG_SDU_BWITH])
                fspec_struct->average_sdu_bandwidth =
                        nla_get_u32(attrs[FSPEC_ATTR_AVG_SDU_BWITH]);

        if (attrs[FSPEC_ATTR_PEAK_BWITH_DURATION])
                fspec_struct->peak_bandwidth_duration =
                        nla_get_u32(attrs[FSPEC_ATTR_PEAK_BWITH_DURATION]);

        if (attrs[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION])
                fspec_struct->peak_sdu_bandwidth_duration =
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
                               struct name *   name_struct)
{
        struct nla_policy attr_policy[APNI_ATTR_MAX + 1];
        struct nlattr *   attrs[APNI_ATTR_MAX + 1];
        string_t *        process_name;
        string_t *        process_instance;
        string_t *        entity_name;
        string_t *        entity_instance;

        if (!name_attr || !name_struct) {
                LOG_ERR("Bogus input parameters, cannot parse name app info");
                return -1;
        }

        attr_policy[APNI_ATTR_PROCESS_NAME].type = NLA_STRING;
        attr_policy[APNI_ATTR_PROCESS_NAME].len = 0;
        attr_policy[APNI_ATTR_PROCESS_INSTANCE].type = NLA_STRING;
        attr_policy[APNI_ATTR_PROCESS_INSTANCE].len = 0;
        attr_policy[APNI_ATTR_ENTITY_NAME].type = NLA_STRING;
        attr_policy[APNI_ATTR_ENTITY_NAME].len = 0;
        attr_policy[APNI_ATTR_ENTITY_INSTANCE].type = NLA_STRING;
        attr_policy[APNI_ATTR_ENTITY_INSTANCE].len = 0;

        if (nla_parse_nested(attrs, APNI_ATTR_MAX, name_attr, attr_policy) < 0)
                return -1;


        if (attrs[APNI_ATTR_PROCESS_NAME])
                process_name =
                        nla_get_string(attrs[APNI_ATTR_PROCESS_NAME]);
        else
                process_name = NULL;

        if (attrs[APNI_ATTR_PROCESS_INSTANCE])
                process_instance =
                        nla_get_string(attrs[APNI_ATTR_PROCESS_INSTANCE]);
        else
                process_instance = NULL;

        if (attrs[APNI_ATTR_ENTITY_NAME])
                entity_name =
                        nla_get_string(attrs[APNI_ATTR_ENTITY_NAME]);
        else
                entity_name = NULL;

        if (attrs[APNI_ATTR_ENTITY_INSTANCE])
                entity_instance =
                        nla_get_string(attrs[APNI_ATTR_ENTITY_INSTANCE]);
        else
                entity_instance = NULL;

        if (!name_init_from(name_struct,
                            process_name, process_instance,
                            entity_name,  entity_instance))
                return -1;

        return 0;
}

static int parse_ipcp_config_entry_value(struct nlattr *            name_attr,
                                         struct ipcp_config_entry * entry)
{
        struct nla_policy attr_policy[IPCP_CONFIG_ENTRY_ATTR_MAX + 1];
        struct nlattr *   attrs[IPCP_CONFIG_ENTRY_ATTR_MAX + 1];

        if (!name_attr) {
                LOG_ERR("Bogus attribute passed, bailing out");
                return -1;
        }

        if (!entry) {
                LOG_ERR("Bogus entry passed, bailing out");
                return -1;
        }

        attr_policy[IPCP_CONFIG_ENTRY_ATTR_NAME].type = NLA_STRING;
        attr_policy[IPCP_CONFIG_ENTRY_ATTR_NAME].len = 0;
        attr_policy[IPCP_CONFIG_ENTRY_ATTR_VALUE].type = NLA_STRING;
        attr_policy[IPCP_CONFIG_ENTRY_ATTR_VALUE].len = 0;

        if (nla_parse_nested(attrs, IPCP_CONFIG_ENTRY_ATTR_MAX,
                             name_attr, attr_policy) < 0)
                return -1;

        if (attrs[IPCP_CONFIG_ENTRY_ATTR_NAME])
                entry->name = kstrdup(nla_get_string(attrs[IPCP_CONFIG_ENTRY_ATTR_NAME]), GFP_KERNEL);

        if (attrs[IPCP_CONFIG_ENTRY_ATTR_VALUE])
                entry->value = kstrdup(nla_get_string(attrs[IPCP_CONFIG_ENTRY_ATTR_VALUE]), GFP_KERNEL);

        return 0;
}

static int parse_list_of_ipcp_config_entries(struct nlattr *     nested_attr,
                                             struct dif_config * dif_config)
{
        struct nlattr *            nla;
        struct ipcp_config_entry * entry;
        struct ipcp_config *       config;
        int                        rem = 0;
        int                        entries_with_problems = 0;
        int                        total_entries = 0;

        if (!nested_attr) {
                LOG_ERR("Bogus attribute passed, bailing out");
                return -1;
        }

        if (!dif_config) {
                LOG_ERR("Bogus dif_config passed, bailing out");
                return -1;
        }

        for (nla = (struct nlattr*) nla_data(nested_attr),
                     rem = nla_len(nested_attr);
             nla_ok(nla, rem);
             nla = nla_next(nla, &(rem))) {
                total_entries++;

                entry = rkzalloc(sizeof(*entry), GFP_KERNEL);
                if (!entry) {
                        entries_with_problems++;
                        continue;
                }

                if (parse_ipcp_config_entry_value(nla, entry) < 0) {
                        rkfree(entry);
                        entries_with_problems++;
                        continue;
                }

                config = ipcp_config_create();
                if (!config) {
                        rkfree(entry);
                        entries_with_problems++;
                        continue;
                }
                config->entry = entry;
                list_add(&config->next, &dif_config->ipcp_config_entries);
        }

        if (rem > 0) {
                LOG_WARN("Missing bits to parse");
        }

        if (entries_with_problems > 0)
                LOG_WARN("Problems parsing %d out of %d parameters",
                         entries_with_problems,
                         total_entries);

        return 0;
}

static int parse_dt_cons(struct nlattr *  attr,
                         struct dt_cons * dt_cons)
{
        struct nla_policy attr_policy[DTC_ATTR_MAX + 1];
        struct nlattr *   attrs[DTC_ATTR_MAX + 1];

        attr_policy[DTC_ATTR_QOS_ID].type = NLA_U16;
        attr_policy[DTC_ATTR_QOS_ID].len = 2;
        attr_policy[DTC_ATTR_PORT_ID].type = NLA_U16;
        attr_policy[DTC_ATTR_PORT_ID].len = 2;
        attr_policy[DTC_ATTR_CEP_ID].type = NLA_U16;
        attr_policy[DTC_ATTR_CEP_ID].len = 2;
        attr_policy[DTC_ATTR_SEQ_NUM].type = NLA_U16;
        attr_policy[DTC_ATTR_SEQ_NUM].len = 2;
        attr_policy[DTC_ATTR_ADDRESS].type = NLA_U16;
        attr_policy[DTC_ATTR_ADDRESS].len = 2;
        attr_policy[DTC_ATTR_LENGTH].type = NLA_U16;
        attr_policy[DTC_ATTR_LENGTH].len = 2;
        attr_policy[DTC_ATTR_MAX_PDU_SIZE].type = NLA_U32;
        attr_policy[DTC_ATTR_MAX_PDU_SIZE].len = 4;
        attr_policy[DTC_ATTR_MAX_PDU_LIFE].type = NLA_U32;
        attr_policy[DTC_ATTR_MAX_PDU_LIFE].len = 4;
        attr_policy[DTC_ATTR_DIF_INTEGRITY].type = NLA_FLAG;
        attr_policy[DTC_ATTR_DIF_INTEGRITY].len = 0;

        if (nla_parse_nested(attrs, DTC_ATTR_MAX, attr, attr_policy) < 0)
                return -1;

        if (attrs[DTC_ATTR_QOS_ID])
                dt_cons->qos_id_length =
                        nla_get_u16(attrs[DTC_ATTR_QOS_ID]);

        if (attrs[DTC_ATTR_PORT_ID])
                dt_cons->port_id_length =
                        nla_get_u16(attrs[DTC_ATTR_PORT_ID]);

        if (attrs[DTC_ATTR_CEP_ID])
                dt_cons->cep_id_length =
                        nla_get_u16(attrs[DTC_ATTR_CEP_ID]);

        if (attrs[DTC_ATTR_SEQ_NUM])
                dt_cons->seq_num_length =
                        nla_get_u16(attrs[DTC_ATTR_SEQ_NUM]);

        if (attrs[DTC_ATTR_ADDRESS])
                dt_cons->address_length =
                        nla_get_u16(attrs[DTC_ATTR_ADDRESS]);

        if (attrs[DTC_ATTR_LENGTH])
                dt_cons->length_length =
                        nla_get_u16(attrs[DTC_ATTR_LENGTH]);

        if (attrs[DTC_ATTR_MAX_PDU_SIZE])
                dt_cons->max_pdu_size =
                        nla_get_u32(attrs[DTC_ATTR_MAX_PDU_SIZE]);

        if (attrs[DTC_ATTR_MAX_PDU_LIFE])
                dt_cons->max_pdu_life =
                        nla_get_u32(attrs[DTC_ATTR_MAX_PDU_LIFE]);

        if (attrs[DTC_ATTR_DIF_INTEGRITY])
                dt_cons->dif_integrity = true;

        return 0;
}

static int parse_dif_config(struct nlattr * dif_config_attr,
                            struct dif_config  * dif_config)
{
        struct nla_policy attr_policy[DCONF_ATTR_MAX + 1];
        struct nlattr *   attrs[DCONF_ATTR_MAX + 1];
        struct dt_cons *  dt_cons;

        attr_policy[DCONF_ATTR_IPCP_CONFIG_ENTRIES].type = NLA_NESTED;
        attr_policy[DCONF_ATTR_IPCP_CONFIG_ENTRIES].len = 0;
        attr_policy[DCONF_ATTR_DATA_TRANS_CONS].type = NLA_NESTED;
        attr_policy[DCONF_ATTR_DATA_TRANS_CONS].len = 0;
        attr_policy[DCONF_ATTR_ADDRESS].type = NLA_U32;
        attr_policy[DCONF_ATTR_ADDRESS].len = 4;
        attr_policy[DCONF_ATTR_QOS_CUBES].type = NLA_NESTED;
        attr_policy[DCONF_ATTR_QOS_CUBES].len = 0;

        if (nla_parse_nested(attrs,
                             DCONF_ATTR_MAX,
                             dif_config_attr,
                             attr_policy) < 0)
                goto parse_fail;

        if (attrs[DCONF_ATTR_IPCP_CONFIG_ENTRIES]) {
                if (parse_list_of_ipcp_config_entries(attrs[DCONF_ATTR_IPCP_CONFIG_ENTRIES],
                                                      dif_config) < 0)
                        goto parse_fail;
        }

        if (attrs[DCONF_ATTR_DATA_TRANS_CONS]) {
                dt_cons = rkzalloc(sizeof(struct dt_cons),GFP_KERNEL);
                if (!dt_cons)
                        goto parse_fail;

                dif_config->dt_cons = dt_cons;

                if (parse_dt_cons(attrs[DCONF_ATTR_DATA_TRANS_CONS],
                                  dif_config->dt_cons) < 0) {
                        rkfree(dif_config->dt_cons);
                        goto parse_fail;
                }
        }

        if (attrs[DCONF_ATTR_ADDRESS])
                dif_config->address = nla_get_u32(attrs[DCONF_ATTR_ADDRESS]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("dif config attributes"));
        return -1;
}

static int parse_dif_info(struct nlattr *    dif_config_attr,
                          struct dif_info  * dif_info)
{
        struct nla_policy attr_policy[DINFO_ATTR_MAX + 1];
        struct nlattr *   attrs[DINFO_ATTR_MAX + 1];

        attr_policy[DINFO_ATTR_DIF_TYPE].type = NLA_STRING;
        attr_policy[DINFO_ATTR_DIF_TYPE].len  = 0;
        attr_policy[DINFO_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[DINFO_ATTR_DIF_NAME].len  = 0;
        attr_policy[DINFO_ATTR_CONFIG].type   = NLA_NESTED;
        attr_policy[DINFO_ATTR_CONFIG].len    = 0;

        if (nla_parse_nested(attrs,
                             DINFO_ATTR_MAX,
                             dif_config_attr,
                             attr_policy) < 0)
                goto parse_fail;

        if (attrs[DINFO_ATTR_DIF_TYPE])
                dif_info->type =
                        kstrdup(nla_get_string(attrs[DINFO_ATTR_DIF_TYPE]),
                                GFP_KERNEL);

        if (parse_app_name_info(attrs[DINFO_ATTR_DIF_NAME],
                                dif_info->dif_name) < 0)
                goto parse_fail;

        if (attrs[DINFO_ATTR_CONFIG])
                if (parse_dif_config(attrs[DINFO_ATTR_CONFIG],
                                     dif_info->configuration) < 0)
                        goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("dif info attribute"));
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

        if (nla_parse_nested(attrs,
                             RIBO_ATTR_MAX, rib_obj_attr, attr_policy) < 0)
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
        for (i=0; i < count; i++) {
                if (parse_rib_object(&rib_objs_attr[i],
                                     &rib_objs_struct[i]) < 0) {
                        LOG_ERR("Could not parse rib_objs_list attribute");
                        return -1;
                }
        }
        return 0;
}

static int rnl_parse_generic_u32_param_msg(struct genl_info * info,
                                           uint_t *           param_var,
                                           uint_t             param_name,
                                           uint_t             max_params,
                                           string_t *         msg_name)
{
        struct nla_policy attr_policy[max_params + 1];
        struct nlattr *   attrs[max_params + 1];

        attr_policy[param_name].type = NLA_U32;
        attr_policy[param_name].len  = 4;

        if (nlmsg_parse(info->nlhdr,
                        /* FIXME: Check if this is correct */
                        sizeof(struct genlmsghdr) +
                        sizeof(struct rina_msg_hdr),
                        attrs,
                        max_params,
                        attr_policy) < 0) {
                LOG_ERR("Could not parse Netlink message type %s", msg_name);
                return -1;
        }

        if (attrs[param_name]) {
                * param_var = nla_get_u32(attrs[param_name]);
        }

        return 0;
}

static int rnl_parse_ipcm_assign_to_dif_req_msg(struct genl_info * info,
                                                struct rnl_ipcm_assign_to_dif_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IATDR_ATTR_MAX + 1];
        struct nlattr *   attrs[IATDR_ATTR_MAX + 1];
        int               result;

        attr_policy[IATDR_ATTR_DIF_INFORMATION].type = NLA_NESTED;
        attr_policy[IATDR_ATTR_DIF_INFORMATION].len  = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IATDR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy",
                        result);
                goto parse_fail;
        }

        if (parse_dif_info(attrs[IATDR_ATTR_DIF_INFORMATION],
                           msg_attrs->dif_info) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_assign_to_dif_resp_msg(struct genl_info * info,
                                                 struct rnl_ipcm_assign_to_dif_resp_msg_attrs * msg_attrs)
{
        return rnl_parse_generic_u32_param_msg(info,
                                               &(msg_attrs->result),
                                               IATDRE_ATTR_RESULT,
                                               IATDRE_ATTR_MAX,
                                               "RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE");
}

static int rnl_parse_ipcm_update_dif_config_req_msg(struct genl_info * info,
                                                    struct rnl_ipcm_update_dif_config_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IUDCR_ATTR_MAX + 1];
        struct nlattr *attrs[IUDCR_ATTR_MAX + 1];
        int result;

        attr_policy[IUDCR_ATTR_DIF_CONFIGURATION].type = NLA_NESTED;
        attr_policy[IUDCR_ATTR_DIF_CONFIGURATION].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IUDCR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy",
                        result);
                goto parse_fail;
        }

        if (parse_dif_config(attrs[IUDCR_ATTR_DIF_CONFIGURATION],
                             msg_attrs->dif_config) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST"));
        return -1;
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
                                msg_attrs->ipcp_name) < 0               ||
            parse_app_name_info(info->attrs[IDRN_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0) {
                LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION"));
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
                                               &(msg_attrs->result),
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
                LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_ENROLL_TO_DIF_REQUEST:"));
                return -1;

        }
        return 0;
}

static int rnl_parse_ipcm_enroll_to_dif_resp_msg(struct genl_info * info,
                                                 struct rnl_ipcm_enroll_to_dif_resp_msg_attrs * msg_attrs)
{
        return rnl_parse_generic_u32_param_msg(info,
                                               &(msg_attrs->result),
                                               IEDRE_ATTR_RESULT,
                                               IEDRE_ATTR_MAX,
                                               "RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE");
}

static int rnl_parse_ipcm_disconn_neighbor_req_msg(struct genl_info * info,
                                                   struct rnl_ipcm_disconn_neighbor_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IDNR_ATTR_MAX + 1];

        attr_policy[IDNR_ATTR_NEIGHBOR_NAME].type = NLA_NESTED;

        if (rnl_check_attr_policy(info->nlhdr,
                                  IDNR_ATTR_MAX, attr_policy) < 0 ||
            parse_app_name_info(info->attrs[IDNR_ATTR_NEIGHBOR_NAME],
                                msg_attrs->neighbor_name) < 0) {
                LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST:"));
                return -1;

        }

        return 0;
}

static int rnl_parse_ipcm_disconn_neighbor_resp_msg(struct genl_info * info,
                                                    struct rnl_ipcm_disconn_neighbor_resp_msg_attrs * msg_attrs)
{
        return rnl_parse_generic_u32_param_msg(info,
                                               &(msg_attrs->result),
                                               IATDRE_ATTR_RESULT,
                                               IATDRE_ATTR_MAX,
                                               "RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE");
}

static int rnl_parse_ipcm_alloc_flow_req_msg(struct genl_info * info,
                                             struct rnl_ipcm_alloc_flow_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRM_ATTR_MAX + 1];
        struct nlattr *attrs[IAFRM_ATTR_MAX + 1];
        int result;

        attr_policy[IAFRM_ATTR_SOURCE_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRM_ATTR_SOURCE_APP_NAME].len  = 0;
        attr_policy[IAFRM_ATTR_DEST_APP_NAME].type   = NLA_NESTED;
        attr_policy[IAFRM_ATTR_DEST_APP_NAME].len    = 0;
        attr_policy[IAFRM_ATTR_FLOW_SPEC].type       = NLA_NESTED;
        attr_policy[IAFRM_ATTR_FLOW_SPEC].len        = 0;
        attr_policy[IAFRM_ATTR_DIF_NAME].type        = NLA_NESTED;
        attr_policy[IAFRM_ATTR_DIF_NAME].len         = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IAFRM_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy", result);
                goto parse_fail;
        }

        if (parse_app_name_info(attrs[IAFRM_ATTR_SOURCE_APP_NAME],
                                msg_attrs->source) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IAFRM_ATTR_DEST_APP_NAME],
                                msg_attrs->dest) < 0)
                goto parse_fail;

        if (parse_flow_spec(attrs[IAFRM_ATTR_FLOW_SPEC],
                            msg_attrs->fspec) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IAFRM_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_ALLOCATE_FLOW_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_alloc_flow_req_arrived_msg(struct genl_info * info,
                                                     struct rnl_ipcm_alloc_flow_req_arrived_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRA_ATTR_MAX + 1];
        struct nlattr *attrs[IAFRA_ATTR_MAX + 1];
        int result;

        attr_policy[IAFRA_ATTR_SOURCE_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_SOURCE_APP_NAME].len = 0;
        attr_policy[IAFRA_ATTR_DEST_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_DEST_APP_NAME].len = 0;
        attr_policy[IAFRA_ATTR_FLOW_SPEC].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_FLOW_SPEC].len = 0;
        attr_policy[IAFRA_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_DIF_NAME].len = 0;
        attr_policy[IAFRA_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[IAFRA_ATTR_PORT_ID].len = 4;


        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IAFRA_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy", result);
                goto parse_fail;
        }

        if (parse_app_name_info(attrs[IAFRA_ATTR_SOURCE_APP_NAME],
                                msg_attrs->source) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IAFRA_ATTR_DEST_APP_NAME],
                                msg_attrs->dest) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IAFRA_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0)
                goto parse_fail;

        if (parse_flow_spec(attrs[IAFRA_ATTR_FLOW_SPEC],
                            msg_attrs->fspec) < 0)
                goto parse_fail;

        if (attrs[IAFRA_ATTR_PORT_ID])
                msg_attrs->id =
                        nla_get_u32(attrs[IAFRA_ATTR_PORT_ID]);
        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED"));
        return -1;
}

static int rnl_parse_ipcm_alloc_flow_req_result_msg(struct genl_info * info,
                                                    struct rnl_ipcm_alloc_flow_req_result_msg_attrs * msg_attrs)
{
        /* FIXME: It should parse the port-id as well */
        return rnl_parse_generic_u32_param_msg(info,
                                               &(msg_attrs->result),
                                               IAFRRM_ATTR_RESULT,
                                               IAFRRM_ATTR_MAX,
                                               "RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT");
}

static int rnl_parse_ipcm_alloc_flow_resp_msg(struct genl_info * info,
                                              struct rnl_alloc_flow_resp_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRE_ATTR_MAX + 1];
        struct nlattr *attrs[IAFRE_ATTR_MAX + 1];
        int result;

        attr_policy[IAFRE_ATTR_RESULT].type = NLA_U32;
        attr_policy[IAFRE_ATTR_RESULT].len = 4;
        attr_policy[IAFRE_ATTR_NOTIFY_SOURCE].type = NLA_FLAG;
        attr_policy[IAFRE_ATTR_NOTIFY_SOURCE].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IAFRE_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy", result);
                goto parse_fail;
        }

        if (attrs[IAFRE_ATTR_RESULT])
                msg_attrs->result =
                        nla_get_u32(attrs[IAFRE_ATTR_RESULT]);

        if (attrs[IAFRE_ATTR_NOTIFY_SOURCE])
                msg_attrs->notify_src =
                        nla_get_flag(attrs[IAFRE_ATTR_NOTIFY_SOURCE]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE"));
        return -1;
}

static int rnl_parse_ipcm_dealloc_flow_req_msg(struct genl_info * info,
                                               struct rnl_ipcm_dealloc_flow_req_msg_attrs * msg_attrs)
{
        return rnl_parse_generic_u32_param_msg(info,
                                               (uint_t *) &(msg_attrs->id),
                                               IDFRT_ATTR_PORT_ID,
                                               IDFRT_ATTR_MAX,
                                               "RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST");

}

static int rnl_parse_ipcm_dealloc_flow_resp_msg(struct genl_info * info,
                                                struct rnl_ipcm_dealloc_flow_resp_msg_attrs * msg_attrs)
{
        return rnl_parse_generic_u32_param_msg(info,
                                               &(msg_attrs->result),
                                               IDFRE_ATTR_RESULT,
                                               IDFRE_ATTR_MAX,
                                               "RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE");
}

static int rnl_parse_ipcm_flow_dealloc_noti_msg(struct genl_info * info,
                                                struct rnl_ipcm_flow_dealloc_noti_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IFDN_ATTR_MAX + 1];
        struct nlattr *attrs[IFDN_ATTR_MAX + 1];
        int result;

        attr_policy[IFDN_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[IFDN_ATTR_PORT_ID].len = 4;
        attr_policy[IFDN_ATTR_CODE].type = NLA_U32;
        attr_policy[IFDN_ATTR_CODE].len = 4;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IFDN_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy", result);
                goto parse_fail;
        }

        if (attrs[IFDN_ATTR_PORT_ID])
                msg_attrs->id = nla_get_u32(attrs[IFDN_ATTR_PORT_ID]);

        if (attrs[IFDN_ATTR_CODE])
                msg_attrs->code = nla_get_u32(attrs[IFDN_ATTR_CODE]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION"));
        return -1;
}

static int rnl_parse_ipcm_conn_create_req_msg(struct genl_info * info,
                                              struct rnl_ipcp_conn_create_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[ICCRQ_ATTR_MAX + 1];
        struct nlattr *attrs[ICCRQ_ATTR_MAX + 1];
        int    result;

        attr_policy[ICCRQ_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[ICCRQ_ATTR_PORT_ID].len = 0;
        attr_policy[ICCRQ_ATTR_SOURCE_ADDR].type = NLA_U32;
        attr_policy[ICCRQ_ATTR_SOURCE_ADDR].len = 0;
        attr_policy[ICCRQ_ATTR_DEST_ADDR].type = NLA_U32;
        attr_policy[ICCRQ_ATTR_DEST_ADDR].len = 0;
        attr_policy[ICCRQ_ATTR_QOS_ID].type = NLA_U32;
        attr_policy[ICCRQ_ATTR_QOS_ID].len = 0;
        attr_policy[ICCRQ_ATTR_POLICIES].type = NLA_U32;
        attr_policy[ICCRQ_ATTR_POLICIES].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             ICCRQ_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy", result);
                goto parse_fail;
        }

        if (attrs[ICCRQ_ATTR_PORT_ID])
                msg_attrs->port_id = nla_get_u32(attrs[ICCRQ_ATTR_PORT_ID]);

        if (attrs[ICCRQ_ATTR_SOURCE_ADDR])
                msg_attrs->src_addr = nla_get_u32(attrs[ICCRQ_ATTR_SOURCE_ADDR]);

        if (attrs[ICCRQ_ATTR_DEST_ADDR])
                msg_attrs->dst_addr = nla_get_u32(attrs[ICCRQ_ATTR_DEST_ADDR]);

        if (attrs[ICCRQ_ATTR_QOS_ID])
                msg_attrs->qos_id = nla_get_u32(attrs[ICCRQ_ATTR_QOS_ID]);

        if (attrs[ICCRQ_ATTR_POLICIES])
                msg_attrs->policies = nla_get_u32(attrs[ICCRQ_ATTR_POLICIES]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCP_CONN_CREATE_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_conn_create_arrived_msg(struct genl_info * info,
                                                  struct rnl_ipcp_conn_create_arrived_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[ICCA_ATTR_MAX + 1];
        struct nlattr *attrs[ICCA_ATTR_MAX + 1];
        int    result;

        attr_policy[ICCA_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[ICCA_ATTR_PORT_ID].len = 0;
        attr_policy[ICCA_ATTR_SOURCE_ADDR].type = NLA_U32;
        attr_policy[ICCA_ATTR_SOURCE_ADDR].len = 0;
        attr_policy[ICCA_ATTR_DEST_ADDR].type = NLA_U32;
        attr_policy[ICCA_ATTR_DEST_ADDR].len = 0;
        attr_policy[ICCA_ATTR_DEST_CEP_ID].type = NLA_U32;
        attr_policy[ICCA_ATTR_DEST_CEP_ID].len = 0;
        attr_policy[ICCA_ATTR_QOS_ID].type = NLA_U32;
        attr_policy[ICCA_ATTR_QOS_ID].len = 0;
        attr_policy[ICCA_ATTR_POLICIES].type = NLA_U32;
        attr_policy[ICCA_ATTR_POLICIES].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             ICCA_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy", result);
                goto parse_fail;
        }

        if (attrs[ICCA_ATTR_PORT_ID])
                msg_attrs-> port_id= nla_get_u32(attrs[ICCA_ATTR_PORT_ID]);

        if (attrs[ICCA_ATTR_SOURCE_ADDR])
                msg_attrs->src_addr = nla_get_u32(attrs[ICCA_ATTR_SOURCE_ADDR]);

        if (attrs[ICCA_ATTR_DEST_ADDR])
                msg_attrs->dst_addr = nla_get_u32(attrs[ICCA_ATTR_DEST_ADDR]);

        if (attrs[ICCA_ATTR_DEST_CEP_ID])
                msg_attrs->dst_cep = nla_get_u32(attrs[ICCA_ATTR_DEST_CEP_ID]);

        if (attrs[ICCA_ATTR_QOS_ID])
                msg_attrs->qos_id = nla_get_u32(attrs[ICCA_ATTR_QOS_ID]);

        if (attrs[ICCA_ATTR_POLICIES])
                msg_attrs->policies = nla_get_u32(attrs[ICCA_ATTR_POLICIES]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCP_CONN_CREATE_ARRIVED"));
        return -1;
}

static int rnl_parse_ipcm_conn_update_req_msg(struct genl_info * info,
                                              struct rnl_ipcp_conn_update_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[ICURQ_ATTR_MAX + 1];
        struct nlattr *attrs[ICURQ_ATTR_MAX + 1];
        int    result;

        attr_policy[ICURQ_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[ICURQ_ATTR_PORT_ID].len = 0;
        attr_policy[ICURQ_ATTR_SOURCE_CEP_ID].type = NLA_U32;
        attr_policy[ICURQ_ATTR_SOURCE_CEP_ID].len = 0;
        attr_policy[ICURQ_ATTR_DEST_CEP_ID].type = NLA_U32;
        attr_policy[ICURQ_ATTR_DEST_CEP_ID].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             ICURQ_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy", result);
                goto parse_fail;
        }

        if (attrs[ICURQ_ATTR_PORT_ID])
                msg_attrs-> port_id= nla_get_u32(attrs[ICURQ_ATTR_PORT_ID]);

        if (attrs[ICURQ_ATTR_SOURCE_CEP_ID])
                msg_attrs->src_cep = nla_get_u32(attrs[ICURQ_ATTR_SOURCE_CEP_ID]);

        if (attrs[ICURQ_ATTR_DEST_CEP_ID])
                msg_attrs->dst_cep = nla_get_u32(attrs[ICURQ_ATTR_DEST_CEP_ID]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCP_CONN_UPDATE_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_conn_destroy_req_msg(struct genl_info * info,
                                               struct rnl_ipcp_conn_destroy_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[ICDR_ATTR_MAX + 1];
        struct nlattr *attrs[ICDR_ATTR_MAX + 1];
        int    result;

        attr_policy[ICDR_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[ICDR_ATTR_PORT_ID].len = 0;
        attr_policy[ICDR_ATTR_SOURCE_CEP_ID].type = NLA_U32;
        attr_policy[ICDR_ATTR_SOURCE_CEP_ID].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             ICDR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy", result);
                goto parse_fail;
        }

        if (attrs[ICDR_ATTR_PORT_ID])
                msg_attrs-> port_id= nla_get_u32(attrs[ICDR_ATTR_PORT_ID]);

        if (attrs[ICDR_ATTR_SOURCE_CEP_ID])
                msg_attrs->src_cep = nla_get_u32(attrs[ICDR_ATTR_SOURCE_CEP_ID]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCP_CONN_DESTROY_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_reg_app_req_msg(struct genl_info * info,
                                          struct rnl_ipcm_reg_app_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IRAR_ATTR_MAX + 1];
        struct nlattr *attrs[IRAR_ATTR_MAX + 1];
        int result;

        attr_policy[IRAR_ATTR_APP_NAME].type = NLA_NESTED;
        attr_policy[IRAR_ATTR_APP_NAME].len = 0;
        attr_policy[IRAR_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[IRAR_ATTR_DIF_NAME].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IRAR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error %d)",
                        result);
                goto parse_fail;
        }

        if (parse_app_name_info(attrs[IRAR_ATTR_APP_NAME],
                                msg_attrs->app_name) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IRAR_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_REGISTER_APPLICATION_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_reg_app_resp_msg(struct genl_info * info,
                                           struct rnl_ipcm_reg_app_resp_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IRARE_ATTR_MAX + 1];
        struct nlattr *attrs[IRARE_ATTR_MAX + 1];
        int result;

        attr_policy[IRARE_ATTR_RESULT].type = NLA_U32;
        attr_policy[IRARE_ATTR_RESULT].len = 4;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IRARE_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("ould not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (attrs[IRARE_ATTR_RESULT])
                msg_attrs->result = nla_get_u32(attrs[IRARE_ATTR_RESULT]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_REGISTER_APPLICATION_REPONSE"));
        return -1;
}

static int rnl_parse_ipcm_unreg_app_req_msg(struct genl_info * info,
                                            struct rnl_ipcm_unreg_app_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IUAR_ATTR_MAX + 1];
        struct nlattr *attrs[IUAR_ATTR_MAX + 1];
        int result;

        attr_policy[IUAR_ATTR_APP_NAME].type = NLA_NESTED;
        attr_policy[IUAR_ATTR_APP_NAME].len = 0;
        attr_policy[IUAR_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[IUAR_ATTR_DIF_NAME].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IUAR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy", result);
                goto parse_fail;
        }

        if (parse_app_name_info(attrs[IUAR_ATTR_APP_NAME],
                                msg_attrs->app_name) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IUAR_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_unreg_app_resp_msg(struct genl_info * info,
                                             struct rnl_ipcm_unreg_app_resp_msg_attrs * msg_attrs)
{
        return rnl_parse_generic_u32_param_msg(info,
                                               &(msg_attrs->result),
                                               IUARE_ATTR_RESULT,
                                               IUARE_ATTR_MAX,
                                               "RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE");
}

static int rnl_parse_ipcm_query_rib_req_msg(struct genl_info * info,
                                            struct rnl_ipcm_query_rib_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IDQR_ATTR_MAX + 1];

        attr_policy[IDQR_ATTR_OBJECT].type = NLA_NESTED;
        attr_policy[IDQR_ATTR_SCOPE].type  = NLA_U32;
        attr_policy[IDQR_ATTR_FILTER].type = NLA_STRING;

        if (rnl_check_attr_policy(info->nlhdr, IDQR_ATTR_MAX, attr_policy) < 0 ||
            parse_rib_object(info->attrs[IDQR_ATTR_OBJECT],
                             msg_attrs->rib_obj) < 0) {
                LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_QUERY_RIB_REQUEST"));
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

        if (parse_rib_objects_list(info->attrs[IDQRE_ATTR_RIB_OBJECTS],
                                   msg_attrs->count,
                                   msg_attrs->rib_objs) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_QUERY_RIB_RESPONSE"));
        return -1;
}

static int rnl_parse_rmt_add_fte_req_msg(struct genl_info * info,
                                         struct rnl_rmt_add_fte_req_msg_attrs * msg_attrs)
{ return 0; }

static int rnl_parse_rmt_del_fte_req_msg(struct genl_info * info,
                                         struct rnl_rmt_del_fte_req_msg_attrs * msg_attrs)
{ return 0; }

static int rnl_parse_rmt_dump_ft_req_msg(struct genl_info * info,
                                         struct rnl_rmt_dump_ft_req_msg_attrs * msg_attrs)
{ return 0; }

static int rnl_parse_rmt_dump_ft_reply_msg(struct genl_info * info,
                                           struct rnl_rmt_dump_ft_reply_msg_attrs * msg_attrs)
{ return 0; }

int rnl_parse_msg(struct genl_info * info,
                  struct rnl_msg   * msg)
{

        LOG_DBG("RINA Netlink parser started ...");

        if (!info) {
                LOG_ERR("Got empty info, bailing out");
                return -1;
        }

        if (!msg) {
                LOG_ERR("Got empty message, bailing out");
                return -1;
        }

        /* harcoded  */
        /* msg->family             = "rina";*/
        msg->src_port              = info->snd_portid;
        /* dst_port can not be parsed */
        msg->dst_port              = 0;
        msg->seq_num               = info->snd_seq;
        msg->op_code               = (msg_type_t) info->genlhdr->cmd;
#if 0
        msg->req_msg_flag          = 0;
        msg->resp_msg_flag         = 0;
        msg->notification_msg_flag = 0;
#endif
        msg->rina_hdr              = info->userhdr;

#if 0
        /*
         * FIXME: This is broken for 2 reasons:
         *   a) do not use the same LOG_*() for the same line (DO NOT USE \n)
         *   b) missing parameters (6 %d, 4 parameters)
         */

        LOG_DBG("Parsed Netlink message header:\n"
                "msg->src_port: %d "
                "msg->dst_port: %d "
                "msg->seq_num:  %u "
                "msg->op_code:  %d "
                "msg->rina_hdr->src_ipc_id: %d "
                "msg->rina_hdr->dst_ipc_id: %d",
                msg->src_port,msg->dst_port,
                msg->seq_num,msg->op_code,
                msg->rina_hdr->src_ipc_id,
                msg->rina_hdr->dst_ipc_id);
#endif

        LOG_DBG("msg is at %pK", msg);
        LOG_DBG("  msg->rina_hdr is at %pK and size is: %zd",
                msg->rina_hdr, sizeof(msg->rina_hdr));
        LOG_DBG("  msg->attrs is at %pK",
                msg->attrs);
        LOG_DBG("  (msg->rina_hdr)->src_ipc_id is %d",
                (msg->rina_hdr)->src_ipc_id);
        LOG_DBG("  (msg->rina_hdr)->dst_ipc_id is %d",
                (msg->rina_hdr)->dst_ipc_id);

        switch(info->genlhdr->cmd) {
        case RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST:
                if (rnl_parse_ipcm_assign_to_dif_req_msg(info,
                                                         msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE:
                if (rnl_parse_ipcm_assign_to_dif_resp_msg(info,
                                                          msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST:
                if (rnl_parse_ipcm_update_dif_config_req_msg(info,
                                                             msg->attrs) < 0)
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
        case RINA_C_IPCP_CONN_CREATE_REQUEST:
                if (rnl_parse_ipcm_conn_create_req_msg(info,
                                                       msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCP_CONN_CREATE_ARRIVED:
                if (rnl_parse_ipcm_conn_create_arrived_msg(info,
                                                           msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCP_CONN_UPDATE_REQUEST:
                if (rnl_parse_ipcm_conn_update_req_msg(info,
                                                       msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCP_CONN_DESTROY_REQUEST:
                if (rnl_parse_ipcm_conn_destroy_req_msg(info,
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
        LOG_ERR("Could not parse netlink message of type: %d",
                info->genlhdr->cmd);
        return -1;
}
EXPORT_SYMBOL(rnl_parse_msg);

/* FORMATTING */

static int format_app_name_info(const struct name * name,
                                struct sk_buff *    msg)
{
        if (!msg || !name) {
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
                            struct sk_buff *         msg)
{
        if (!fspec) {
                LOG_ERR("Cannot format flow-spec, "
                        "fspec parameter is NULL ...");
                return -1;
        }
        if (!msg) {
                LOG_ERR("Cannot format flow-spec, "
                        "message parameter is NULL ...");
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

        if (fspec->average_bandwidth > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_AVG_BWITH,
                                fspec->average_bandwidth))
                        return -1;
        if (fspec->average_sdu_bandwidth > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_AVG_SDU_BWITH,
                                fspec->average_sdu_bandwidth))
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
        if (fspec->max_allowable_gap >=0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_MAX_GAP,
                                fspec->max_allowable_gap))
                        return -1;
        if (fspec->max_sdu_size > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_MAX_SDU_SIZE,
                                fspec->max_sdu_size))
                        return -1;
        if (fspec->ordered_delivery)
                if (nla_put_flag(msg,
                                 FSPEC_ATTR_IN_ORD_DELIVERY))
                        return -1;
        if (fspec->partial_delivery)
                if (nla_put_flag(msg,
                                 FSPEC_ATTR_PART_DELIVERY))
                        return -1;
        if (fspec->peak_bandwidth_duration > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_PEAK_BWITH_DURATION,
                                fspec->peak_bandwidth_duration))
                        return -1;
        if (fspec->peak_sdu_bandwidth_duration > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_PEAK_SDU_BWITH_DURATION,
                                fspec->peak_sdu_bandwidth_duration))
                        return -1;
        if (fspec->undetected_bit_error_rate > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_UNDETECTED_BER,
                                fspec->undetected_bit_error_rate))
                        return -1;

        return 0;
}

#if 0
static int format_nested_app_name_info_attr(struct nlattr     * msg_attr,
                                            uint_t            attr_name,
                                            const struct name * name,
                                            struct sk_buff    * skb_out)
{
        if (!(msg_attr =
              nla_nest_start(skb_out, attr_name))) {
                nla_nest_cancel(skb_out, msg_attr);
                LOG_ERR(BUILD_STRERROR("Name attribute"));
                return -1;
        }
        if (format_app_name_info(name, skb_out) < 0)
                return -1;
        nla_nest_end(skb_out, msg_attr);
        return 0;
}
#endif

static int rnl_format_generic_u32_param_msg(u32             param_var,
                                            uint_t          param_name,
                                            string_t        * msg_name,
                                            struct sk_buff  * skb_out)
{
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

int rnl_format_ipcm_assign_to_dif_resp_msg(uint_t          result,
                                           struct sk_buff  * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                IAFRRM_ATTR_RESULT,
                                                "rnl_ipcm_assign_to_dif_resp_msg",
                                                skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_assign_to_dif_resp_msg);

int rnl_format_ipcm_update_dif_config_resp_msg(uint_t          result,
                                               struct sk_buff  * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                IAFRRM_ATTR_RESULT,
                                                "rnl_ipcm_update_dif_config_resp_msg",
                                                skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_update_dif_config_resp_msg);

int rnl_format_ipcm_ipcp_dif_reg_noti_msg(const struct name * ipcp_name,
                                          const struct name * dif_name,
                                          bool                is_registered,
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
                LOG_ERR(BUILD_STRERROR("ipcp name attribute"));
                goto format_fail;
        }
        if (format_app_name_info(ipcp_name, skb_out) < 0)
                goto format_fail;
        nla_nest_end(skb_out, msg_ipcp_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IDRN_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_STRERROR("dif name attribute"));
                goto format_fail;
        }
        if (format_app_name_info(dif_name, skb_out) < 0)
                goto format_fail;
        nla_nest_end(skb_out, msg_dif_name);

        /* FIXME: I think the flag value must be specified so nla_put_flag
         * can not be used. Check in US cause it is using it */
        if (nla_put(skb_out,
                    IDRN_ATTR_REGISTRATION,
                    is_registered,
                    NULL) < 0) {
                LOG_ERR(BUILD_STRERROR("is_registered attribute"));
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
int rnl_format_ipcm_ipcp_dif_unreg_noti_msg(uint_t           result,
                                            struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                IDUN_ATTR_RESULT,
                                                "rnl_ipcm_ipcp_to_dif_unreg_noti_msg",
                                                skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_ipcp_dif_unreg_noti_msg);

int rnl_format_ipcm_enroll_to_dif_req_msg(const struct name * dif_name,
                                          struct sk_buff *    skb_out)
{
        struct nlattr * msg_dif_name;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                goto format_fail;
        }

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IEDR_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_STRERROR("dif name attribute"));
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

int rnl_format_ipcm_disconn_neighbor_resp_msg(uint_t           result,
                                              struct sk_buff * skb_out)
{ return 0; }
EXPORT_SYMBOL(rnl_format_ipcm_disconn_neighbor_resp_msg);

int rnl_format_ipcm_alloc_flow_req_msg(const struct name *      source,
                                       const struct name *      dest,
                                       const struct flow_spec * fspec,
                                       port_id_t                id,
                                       const struct name *      dif_name,
                                       struct sk_buff *         skb_out)
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
                LOG_ERR(BUILD_STRERROR("source application name attribute"));
                goto format_fail;
        }

        if (format_app_name_info(source, skb_out) < 0)
                goto format_fail;
        nla_nest_end(skb_out, msg_src_name);

        if (!(msg_dst_name =
              nla_nest_start(skb_out, IAFRM_ATTR_DEST_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_dst_name);
                LOG_ERR(BUILD_STRERROR("destination app name attribute"));
                goto format_fail;
        }

        if (format_app_name_info(dest, skb_out) < 0)
                goto format_fail;
        nla_nest_end(skb_out, msg_dst_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IAFRM_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_STRERROR("DIF name attribute"));
                goto format_fail;
        }

        if (format_app_name_info(dif_name, skb_out) < 0)
                goto format_fail;
        nla_nest_end(skb_out, msg_dif_name);

        if (!(msg_fspec =
              nla_nest_start(skb_out, IAFRM_ATTR_FLOW_SPEC))) {
                nla_nest_cancel(skb_out, msg_fspec);
                LOG_ERR(BUILD_STRERROR("flow spec attribute"));
                goto format_fail;
        }

        if (format_flow_spec(fspec, skb_out) < 0)
                goto format_fail;

        nla_nest_end(skb_out, msg_fspec);

        return 0;

 format_fail:
        LOG_ERR("Could not format rnl_ipcm_alloc_flow_req_msg message");
        return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_alloc_flow_req_msg);

int rnl_format_ipcm_alloc_flow_req_arrived_msg(const struct name *      source,
                                               const struct name *      dest,
                                               const struct flow_spec * fspec,
                                               const struct name *      dif_name,
                                               port_id_t                pid,
                                               struct sk_buff *         skb_out)
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
                LOG_ERR(BUILD_STRERROR("source application name attribute"));
                goto format_fail;
        }

        if (format_app_name_info(source, skb_out) < 0)
                goto format_fail;

        nla_nest_end(skb_out, msg_src_name);

        if (!(msg_dst_name =
              nla_nest_start(skb_out, IAFRA_ATTR_DEST_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_dst_name);
                LOG_ERR(BUILD_STRERROR("destination app name attribute"));
                goto format_fail;
        }

        if (format_app_name_info(dest, skb_out) < 0)
                goto format_fail;

        nla_nest_end(skb_out, msg_dst_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IAFRA_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_STRERROR("DIF name attribute"));
                goto format_fail;
        }

        if (format_app_name_info(dif_name, skb_out) < 0)
                goto format_fail;

        nla_nest_end(skb_out, msg_dif_name);

        if (!(msg_fspec =
              nla_nest_start(skb_out, IAFRA_ATTR_FLOW_SPEC))) {
                nla_nest_cancel(skb_out, msg_fspec);
                LOG_ERR(BUILD_STRERROR("flow spec attribute"));
                goto format_fail;
        }

        if (format_flow_spec(fspec, skb_out) < 0)
                goto format_fail;

        nla_nest_end(skb_out, msg_fspec);

        if (nla_put_u32(skb_out, IAFRA_ATTR_PORT_ID, pid))
                goto format_fail;

        return 0;

 format_fail:
        LOG_ERR("Could not format "
                "rnl_ipcm_alloc_flow_req_arrived_msg "
                "message correctly");
        return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_alloc_flow_req_arrived_msg);

int rnl_format_ipcm_alloc_flow_req_result_msg(uint_t           result,
                                              port_id_t        pid,
                                              struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IAFRRM_ATTR_PORT_ID, pid))
                goto format_fail;

        if (nla_put_u32(skb_out, IAFRRM_ATTR_RESULT, result))
                goto format_fail;

        return 0;

 format_fail:
        LOG_ERR("Could not format "
                "rnl_format_ipcm_alloc_req_result_msg"
                "message correctly");
        return -1;

}
EXPORT_SYMBOL(rnl_format_ipcm_alloc_flow_req_result_msg);

int rnl_format_ipcm_alloc_flow_resp_msg(uint_t           result,
                                        bool             notify_src,
                                        struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IAFRE_ATTR_RESULT, result))
                goto format_fail;

        if (notify_src)
                if (nla_put_flag(skb_out, IAFRE_ATTR_NOTIFY_SOURCE))
                        goto format_fail;

        return 0;

 format_fail:
        LOG_ERR("Could not format "
                "rnl_ipcm_alloc_flow_resp_msg "
                "message correctly");
        return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_alloc_flow_resp_msg);

int rnl_format_ipcm_dealloc_flow_req_msg(port_id_t        id,
                                         struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(id,
                                                IDFRT_ATTR_PORT_ID,
                                                "rnl_ipcm_dealloc_flow_req_msg",
                                                skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_dealloc_flow_req_msg);

int rnl_format_ipcm_dealloc_flow_resp_msg(uint_t           result,
                                          struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                IDFRE_ATTR_RESULT,
                                                "rnl_ipcm_dealloc_flow_resp_msg",
                                                skb_out);
}
EXPORT_SYMBOL(rnl_format_ipcm_dealloc_flow_resp_msg);

int rnl_format_ipcm_flow_dealloc_noti_msg(port_id_t        id,
                                          uint_t           code,
                                          struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IFDN_ATTR_PORT_ID, id))
                goto format_fail;

        if (nla_put_u32(skb_out, IFDN_ATTR_CODE, code ))
                goto format_fail;

        return 0;

 format_fail:
        LOG_ERR("Could not format "
                "rnl_ipcm_alloc_flow_resp_msg "
                "message correctly");
        return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_flow_dealloc_noti_msg);

int rnl_format_ipcm_conn_create_resp_msg(port_id_t        id,
                                         cep_id_t         src_cep,
                                         struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICCRE_ATTR_PORT_ID, id))
                goto format_fail;

        if (nla_put_u32(skb_out, ICCRE_ATTR_SOURCE_CEP_ID, src_cep ))
                goto format_fail;

        return 0;

 format_fail:
        LOG_ERR("Could not format "
                "rnl_format_ipcm_conn_create_resp_msg"
                "message correctly");
        return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_conn_create_resp_msg);

int rnl_format_ipcm_conn_create_result_msg(port_id_t        id,
                                           cep_id_t         src_cep,
                                           cep_id_t         dst_cep,
                                           struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICCRS_ATTR_PORT_ID, id))
                goto format_fail;

        if (nla_put_u32(skb_out, ICCRS_ATTR_SOURCE_CEP_ID, src_cep ))
                goto format_fail;

        if (nla_put_u32(skb_out, ICCRS_ATTR_DEST_CEP_ID, dst_cep ))
                goto format_fail;

        return 0;

 format_fail:
        LOG_ERR("Could not format "
                "rnl_format_ipcm_conn_create_result_msg"
                "message correctly");
        return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_conn_create_result_msg);

int rnl_format_ipcm_conn_update_result_msg(port_id_t        id,
                                           uint_t           result,
                                           struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICURS_ATTR_PORT_ID, id))
                goto format_fail;

        if (nla_put_u32(skb_out, ICURS_ATTR_RESULT, result))
                goto format_fail;

        return 0;

 format_fail:
        LOG_ERR("Could not format "
                "rnl_format_ipcm_conn_update_result_msg"
                "message correctly");
        return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_conn_update_result_msg);

int rnl_format_ipcm_conn_destroy_result_msg(port_id_t        id,
                                            uint_t           result,
                                            struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICDRS_ATTR_PORT_ID, id))
                goto format_fail;

        if (nla_put_u32(skb_out, ICDRS_ATTR_RESULT, result))
                goto format_fail;

        return 0;

 format_fail:
        LOG_ERR("Could not format "
                "rnl_format_ipcm_conn_destroy_result_msg"
                "message correctly");
        return -1;
}
EXPORT_SYMBOL(rnl_format_ipcm_conn_destroy_result_msg);

int rnl_format_ipcm_reg_app_req_msg(const struct name * app_name,
                                    const struct name * dif_name,
                                    struct sk_buff *    skb_out)
{
        struct nlattr * msg_src_name, * msg_dif_name;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_src_name =
              nla_nest_start(skb_out, IRAR_ATTR_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_src_name);
                LOG_ERR(BUILD_STRERROR("source application name attribute"));
                goto format_fail;
        }

        if (format_app_name_info(app_name, skb_out) < 0)
                goto format_fail;
        nla_nest_end(skb_out, msg_src_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IRAR_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_STRERROR("DIF name attribute"));
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

int rnl_format_ipcm_reg_app_resp_msg(uint_t           result,
                                     struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IRARE_ATTR_RESULT, result ))
                goto format_fail;

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
                LOG_ERR(BUILD_STRERROR("source application name attribute"));
                goto format_fail;
        }

        if (format_app_name_info(app_name, skb_out) < 0)
                goto format_fail;
        nla_nest_end(skb_out, msg_src_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IUAR_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_STRERROR("DIF name attribute"));
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

        if (nla_put_u32(skb_out, IUARE_ATTR_RESULT, result) < 0) {
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
                                   struct sk_buff         * msg)
{
        int i;
        struct nlattr * msg_obj;

        for (i=0; i< count; i++) {
                if (!(msg_obj =
                      nla_nest_start(msg, i))) {
                        nla_nest_cancel(msg, msg_obj);
                        LOG_ERR(BUILD_STRERROR("rib object attribute"));
                        return -1;
                }
                if (format_rib_object(objs[i], msg) < 0)
                        return -1;
                nla_nest_end(msg, msg_obj);
        }

        return 0;

}

int rnl_format_ipcm_query_rib_req_msg(const struct rib_object * obj,
                                      uint_t                    scope,
                                      const regex_t *           filter,
                                      struct sk_buff *          skb_out)
{
        struct nlattr * msg_obj;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_obj =
              nla_nest_start(skb_out, IDQR_ATTR_OBJECT))) {
                nla_nest_cancel(skb_out, msg_obj);
                LOG_ERR(BUILD_STRERROR("rib object attribute"));
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
                                       uint_t                  count,
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
                LOG_ERR(BUILD_STRERROR("rib object list attribute"));
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

int rnl_format_socket_closed_notification_msg(u32              nl_port,
                                              struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(nl_port,
                                                ISCN_ATTR_PORT,
                                                "rnl_format_socket_closed_notification_msg",
                                                skb_out);
}
EXPORT_SYMBOL(rnl_format_socket_closed_notification_msg);

static int send_nl_unicast_msg(struct net *     net,
                               struct sk_buff * skb,
                               u32              portid,
                               msg_type_t       type,
                               rnl_sn_t         seq_num)
{
        int result;

        result = genlmsg_unicast(net, skb, portid);
        if (result) {
                LOG_ERR("Could not send NL unicast msg of type %d "
                        "with seq num %u to %u (result = %d)",
                        (int) type, seq_num, portid, result);
                nlmsg_free(skb);
                return -1;
        }

        LOG_DBG("Sent NL unicast msg of type %d with seq num %u to %u",
                (int) type, seq_num, portid);

        return 0;
}

int rnl_assign_dif_response(ipc_process_id_t id,
                            uint_t           res,
                            rnl_sn_t         seq_num,
                            u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = id;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_assign_to_dif_resp_msg(res, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        LOG_DBG("Gonna send NL unicast message now");
        result = send_nl_unicast_msg(&init_net,
                                     out_msg,
                                     nl_port_id,
                                     RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE,
                                     seq_num);
        LOG_DBG("NL unicast message sent (result = %d)", result);

        return result;
}
EXPORT_SYMBOL(rnl_assign_dif_response);

int rnl_update_dif_config_response(ipc_process_id_t id,
                                   uint_t           res,
                                   rnl_sn_t         seq_num,
                                   u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = id;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_update_dif_config_resp_msg(res, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }
        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_update_dif_config_response);

int rnl_app_register_unregister_response_msg(ipc_process_id_t ipc_id,
                                             uint_t           res,
                                             rnl_sn_t         seq_num,
                                             u32              nl_port_id,
                                             bool             isRegister)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        uint_t                command;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        command = isRegister                               ?
                RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE  :
                RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE;

        out_hdr = (struct rina_msg_hdr *) genlmsg_put(out_msg,
                                                      0,
                                                      seq_num,
                                                      &rnl_nl_family,
                                                      0,
                                                      command);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_reg_app_resp_msg(res, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }
        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   command,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_app_register_unregister_response_msg);

int rnl_app_alloc_flow_req_arrived_msg(ipc_process_id_t         ipc_id,
                                       const struct name *      dif_name,
                                       const struct name *      source,
                                       const struct name *      dest,
                                       const struct flow_spec * fspec,
                                       rnl_sn_t                 seq_num,
                                       u32                      nl_port_id,
                                       port_id_t                pid)
{
        struct sk_buff * msg;
        struct rina_msg_hdr * hdr;
        int result;

        msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        hdr = (struct rina_msg_hdr *)
                genlmsg_put(msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            NLM_F_REQUEST,
                            RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED);
        if (!hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(msg);
                return -1;
        }

        hdr->dst_ipc_id = 0;
        hdr->src_ipc_id = ipc_id;
        if (rnl_format_ipcm_alloc_flow_req_arrived_msg(source,
                                                       dest,
                                                       fspec,
                                                       dif_name,
                                                       pid,
                                                       msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(msg);
                return -1;
        }
        result = genlmsg_end(msg, hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   msg,
                                   nl_port_id,
                                   RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_app_alloc_flow_req_arrived_msg);

int rnl_app_alloc_flow_result_msg(ipc_process_id_t ipc_id,
                                  uint_t           res,
                                  port_id_t        pid,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_alloc_flow_req_result_msg(res, pid, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_app_alloc_flow_result_msg);

int rnl_app_dealloc_flow_resp_msg(ipc_process_id_t ipc_id,
                                  uint_t           res,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_dealloc_flow_resp_msg(res, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_app_dealloc_flow_resp_msg);

int rnl_flow_dealloc_not_msg(ipc_process_id_t ipc_id,
                             uint_t           code,
                             port_id_t        port_id,
                             u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            0,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_flow_dealloc_noti_msg(port_id, code, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION,
                                   0);
}
EXPORT_SYMBOL(rnl_flow_dealloc_not_msg);

int rnl_ipcp_conn_create_resp_msg(ipc_process_id_t ipc_id,
                                  port_id_t        pid,
                                  cep_id_t         src_cep,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int    result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCP_CONN_CREATE_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_conn_create_resp_msg(pid, src_cep, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }
        result = genlmsg_unicast(&init_net, out_msg, nl_port_id);

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCP_CONN_CREATE_RESPONSE,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_ipcp_conn_create_resp_msg);

int rnl_ipcp_conn_create_result_msg(ipc_process_id_t ipc_id,
                                    port_id_t        pid,
                                    cep_id_t         src_cep,
                                    cep_id_t         dst_cep,
                                    rnl_sn_t         seq_num,
                                    u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int    result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCP_CONN_CREATE_RESULT);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_conn_create_result_msg(pid,
                                                   src_cep, dst_cep,
                                                   out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCP_CONN_CREATE_RESULT,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_ipcp_conn_create_result_msg);

int rnl_ipcp_conn_update_result_msg(ipc_process_id_t ipc_id,
                                    port_id_t        pid,
                                    uint_t           res,
                                    rnl_sn_t         seq_num,
                                    u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int    result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCP_CONN_UPDATE_RESULT);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_conn_update_result_msg(pid, res, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCP_CONN_UPDATE_RESULT,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_ipcp_conn_update_result_msg);

int rnl_ipcp_conn_destroy_result_msg(ipc_process_id_t ipc_id,
                                     port_id_t        pid,
                                     uint_t           res,
                                     rnl_sn_t         seq_num,
                                     u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int    result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCP_CONN_UPDATE_RESULT);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_conn_destroy_result_msg(pid, res, out_msg)) {
                LOG_ERR("Could not format message...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }
        result = genlmsg_unicast(&init_net, out_msg, nl_port_id);
        if (result) {
                LOG_ERR("Could not send unicast msg: %d", result);
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rnl_ipcp_conn_destroy_result_msg);

int rnl_ipcm_sock_closed_notif_msg(u32 closed_port, u32 dest_port)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            0,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = 0;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_socket_closed_notification_msg(closed_port, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   dest_port,
                                   RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION,
                                   0);
}
EXPORT_SYMBOL(rnl_ipcm_sock_closed_notif_msg);
