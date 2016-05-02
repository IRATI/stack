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

#include <linux/export.h>
#include <net/netlink.h>
#include <net/genetlink.h>
#include <linux/rculist.h>

#define RINA_PREFIX "rnl-utils"

#include "logs.h"
#include "common.h"
#include "debug.h"
#include "ipcp-utils.h"
#include "utils.h"
#include "rnl.h"
#include "rnl-utils.h"
#include "dtp-conf-utils.h"
#include "dtcp-conf-utils.h"
#include "policies.h"
#include "sdup.h"

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
 *        beginning (e.g. msg and name)
 */

#define BUILD_STRERROR(X)                                       \
        "Netlink message does not contain " X ", bailing out"

#define BUILD_STRERROR_BY_MTYPE(X)                      \
        "Could not parse Netlink message of type " X

/* FIXME: These externs have to disappear from here */
extern struct genl_family rnl_nl_family;

static char * nla_get_string(struct nlattr * nla)
{ return (char *) nla_data(nla); }

static char * nla_dup_string(struct nlattr * nla, gfp_t flags)
{ return rkstrdup_gfp(nla_get_string(nla), flags); }

static struct rnl_ipcm_alloc_flow_req_msg_attrs *
rnl_ipcm_alloc_flow_req_msg_attrs_create(void)
{
        struct rnl_ipcm_alloc_flow_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->source = name_create();
        if (!tmp->source) {
                rkfree(tmp);
                return NULL;
        }

        tmp->dest = name_create();
        if (!tmp->dest) {
                name_destroy(tmp->source);
                rkfree(tmp);
                return NULL;
        }

        tmp->dif_name = name_create();
        if (!tmp->dif_name) {
                name_destroy(tmp->dest);
                name_destroy(tmp->source);
                rkfree(tmp);
                return NULL;
        }

        tmp->fspec = rkzalloc(sizeof(struct flow_spec), GFP_KERNEL);
        if (!tmp->fspec) {
                name_destroy(tmp->dif_name);
                name_destroy(tmp->dest);
                name_destroy(tmp->source);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static struct rnl_alloc_flow_resp_msg_attrs *
rnl_alloc_flow_resp_msg_attrs_create(void)
{
        struct rnl_alloc_flow_resp_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcm_dealloc_flow_req_msg_attrs *
rnl_ipcm_dealloc_flow_req_msg_attrs_create(void)
{
        struct rnl_ipcm_dealloc_flow_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcm_assign_to_dif_req_msg_attrs *
rnl_ipcm_assign_to_dif_req_msg_attrs_create(void)
{
        struct rnl_ipcm_assign_to_dif_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        tmp->dif_info = dif_info_create();
        if (!tmp->dif_info) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static struct rnl_ipcm_update_dif_config_req_msg_attrs *
rnl_ipcm_update_dif_config_req_msg_attrs_create(void)
{
        struct rnl_ipcm_update_dif_config_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        tmp->dif_config = dif_config_create();
        if (!tmp->dif_config) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static struct rnl_ipcm_reg_app_req_msg_attrs *
rnl_ipcm_reg_app_req_msg_attrs_create(void)
{
        struct rnl_ipcm_reg_app_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        tmp->app_name = name_create();
        if (!tmp->app_name) {
                rkfree(tmp);
                return NULL;
        }

        tmp->dif_name = name_create();
        if (!tmp->dif_name) {
                name_destroy(tmp->app_name);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static struct rnl_ipcp_conn_create_req_msg_attrs *
rnl_ipcp_conn_create_req_msg_attrs_create(void)
{
        struct rnl_ipcp_conn_create_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        tmp->dtp_cfg = dtp_config_create();
        if (!tmp->dtp_cfg) {
                rkfree(tmp);
                return NULL;
        }

        tmp->dtcp_cfg = dtcp_config_create();
        if (!tmp->dtcp_cfg) {
                dtp_config_destroy(tmp->dtp_cfg);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static struct rnl_ipcp_conn_create_arrived_msg_attrs *
rnl_ipcp_conn_create_arrived_msg_attrs_create(void)
{
        struct rnl_ipcp_conn_create_arrived_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        tmp->dtp_cfg = dtp_config_create();
        if (!tmp->dtp_cfg) {
                rkfree(tmp);
                return NULL;
        }

        tmp->dtcp_cfg = dtcp_config_create();
        if (!tmp->dtcp_cfg) {
                dtp_config_destroy(tmp->dtp_cfg);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static struct rnl_ipcp_conn_update_req_msg_attrs *
rnl_ipcp_conn_update_req_msg_attrs_create(void)
{
        struct rnl_ipcp_conn_update_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcp_conn_destroy_req_msg_attrs *
rnl_ipcp_conn_destroy_req_msg_attrs_create(void)
{
        struct rnl_ipcp_conn_destroy_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_rmt_mod_pffe_msg_attrs *
rnl_rmt_mod_pffe_msg_attrs_create(void)
{
        struct rnl_rmt_mod_pffe_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->pff_entries);

        return tmp;
}

static struct rnl_ipcm_query_rib_msg_attrs *
rnl_ipcm_query_rib_msg_attrs_create(void)
{
        struct rnl_ipcm_query_rib_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcp_set_policy_set_param_req_msg_attrs *
rnl_ipcp_set_policy_set_param_req_msg_attrs_create(void)
{
        struct rnl_ipcp_set_policy_set_param_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcp_select_policy_set_req_msg_attrs *
rnl_ipcp_select_policy_set_req_msg_attrs_create(void)
{
        struct rnl_ipcp_select_policy_set_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcp_update_crypto_state_req_msg_attrs *
rnl_ipcp_update_crypto_state_req_msg_attrs_create(void)
{
        struct rnl_ipcp_update_crypto_state_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        tmp->port_id = 0;
        tmp->state = NULL;

        return tmp;
}

struct rnl_msg * rnl_msg_create(enum rnl_msg_attr_type type)
{
        struct rnl_msg * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->attr_type = type;

        switch (tmp->attr_type) {
        case RNL_MSG_ATTRS_ALLOCATE_FLOW_REQUEST:
                tmp->attrs =
                        rnl_ipcm_alloc_flow_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_ALLOCATE_FLOW_RESPONSE:
                tmp->attrs =
                        rnl_alloc_flow_resp_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_DEALLOCATE_FLOW_REQUEST:
                tmp->attrs =
                        rnl_ipcm_dealloc_flow_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_ASSIGN_TO_DIF_REQUEST:
                tmp->attrs =
                        rnl_ipcm_assign_to_dif_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_UPDATE_DIF_CONFIG_REQUEST:
                tmp->attrs =
                        rnl_ipcm_update_dif_config_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_REG_UNREG_REQUEST:
                tmp->attrs =
                        rnl_ipcm_reg_app_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_CONN_CREATE_REQUEST:
                tmp->attrs =
                        rnl_ipcp_conn_create_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_CONN_CREATE_ARRIVED:
                tmp->attrs =
                        rnl_ipcp_conn_create_arrived_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_CONN_UPDATE_REQUEST:
                tmp->attrs =
                        rnl_ipcp_conn_update_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_CONN_DESTROY_REQUEST:
                tmp->attrs =
                        rnl_ipcp_conn_destroy_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_RMT_PFFE_MODIFY_REQUEST:
                tmp->attrs =
                        rnl_rmt_mod_pffe_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_RMT_PFF_DUMP_REQUEST:
                tmp->attrs = NULL;
                break;
        case RNL_MSG_ATTRS_QUERY_RIB_REQUEST:
                tmp->attrs =
                        rnl_ipcm_query_rib_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_SET_POLICY_SET_PARAM_REQUEST:
                tmp->attrs =
                        rnl_ipcp_set_policy_set_param_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_SELECT_POLICY_SET_REQUEST:
                tmp->attrs =
                        rnl_ipcp_select_policy_set_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_UPDATE_CRYPTO_STATE_REQUEST:
                tmp->attrs =
                	rnl_ipcp_update_crypto_state_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        default:
                LOG_ERR("Unknown attributes type %d", tmp->attr_type);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(rnl_msg_create);

static int
rnl_ipcm_alloc_flow_req_msg_attrs_destroy(struct rnl_ipcm_alloc_flow_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->source)   name_destroy(attrs->source);
        if (attrs->dest)     name_destroy(attrs->dest);
        if (attrs->dif_name) name_destroy(attrs->dif_name);
        if (attrs->fspec)    rkfree(attrs->fspec);
        rkfree(attrs);

        return 0;
}

static int
rnl_alloc_flow_resp_msg_attrs_destroy(struct rnl_alloc_flow_resp_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcm_dealloc_flow_req_msg_attrs_destroy(struct rnl_ipcm_dealloc_flow_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcm_assign_to_dif_req_msg_attrs_destroy(struct rnl_ipcm_assign_to_dif_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->dif_info) {
                dif_info_destroy(attrs->dif_info);
        }

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcm_update_dif_config_req_msg_attrs_destroy(struct rnl_ipcm_update_dif_config_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->dif_config) dif_config_destroy(attrs->dif_config);

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcm_reg_app_req_msg_attrs_destroy(struct rnl_ipcm_reg_app_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->app_name) name_destroy(attrs->app_name);
        if (attrs->dif_name) name_destroy(attrs->dif_name);

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcp_conn_create_req_msg_attrs_destroy(struct rnl_ipcp_conn_create_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->dtp_cfg) {
                dtp_config_destroy(attrs->dtp_cfg);
                /* FIXME: The following workaround must be removed */
                attrs->dtp_cfg = NULL;
        }

        if (attrs->dtcp_cfg) {
                dtcp_config_destroy(attrs->dtcp_cfg);
                /* FIXME: The following workaround must be removed */
                attrs->dtcp_cfg = NULL;
        }

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcp_conn_create_arrived_msg_attrs_destroy(struct rnl_ipcp_conn_create_arrived_msg_attrs * attrs)
{
        if (!attrs)
                return -1;
        if (attrs->dtp_cfg) {
                dtp_config_destroy(attrs->dtp_cfg);
                /* FIXME: The following workaround must be removed */
                attrs->dtp_cfg = NULL;
        }

        if (attrs->dtcp_cfg) {
                dtcp_config_destroy(attrs->dtcp_cfg);
                /* FIXME: The following workaround must be removed */
                attrs->dtcp_cfg = NULL;
        }

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcp_conn_update_req_msg_attrs_destroy(struct rnl_ipcp_conn_update_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcp_conn_destroy_req_msg_attrs_destroy(struct rnl_ipcp_conn_destroy_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        rkfree(attrs);
        return 0;
}

static int
rnl_rmt_mod_pffe_msg_attrs_destroy(struct rnl_rmt_mod_pffe_msg_attrs * attrs)
{
        struct mod_pff_entry * e_pos, * e_nxt;

        if (!attrs)
                return -1;

        list_for_each_entry_safe(e_pos, e_nxt,
                                 &attrs->pff_entries, next) {
		struct port_id_altlist * a_pos, * a_nxt;

		list_for_each_entry_safe(a_pos, a_nxt,
					 &e_pos->port_id_altlists, next) {
			if (a_pos->ports) {
				rkfree(a_pos->ports);
			}
			list_del(&a_pos->next);
			rkfree(a_pos);
		}

                list_del(&e_pos->next);
                rkfree(e_pos);
        }
        LOG_DBG("rnl_rmt_mod_pffe_msg_attrs destroy correctly");
        rkfree(attrs);
        return 0;
}

static int
rnl_ipcm_query_rib_msg_attrs_destroy(
                struct rnl_ipcm_query_rib_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->object_class)
                rkfree(attrs->object_class);

        if (attrs->object_name)
                rkfree(attrs->object_name);

        if (attrs->filter)
                rkfree(attrs->filter);

        LOG_DBG("rnl_ipcm_query_rib_req_msg_attrs "
                "destroyed correctly");
        rkfree(attrs);

        return 0;
}

static int
rnl_ipcp_set_policy_set_param_msg_attrs_destroy(
                struct rnl_ipcp_set_policy_set_param_req_msg_attrs * attrs)
{
        if (!attrs) {
		LOG_ERR("Bogus attributes passed, bailing out");
                return -1;
	}

        if (attrs->path)
                rkfree(attrs->path);

        if (attrs->name)
                rkfree(attrs->name);

        if (attrs->value)
                rkfree(attrs->value);

        LOG_DBG("rnl_ipcp_set_policy_set_param_req_msg_attrs "
                "destroyed correctly");
        rkfree(attrs);

        return 0;
}

static int
rnl_ipcp_select_policy_set_msg_attrs_destroy(
                struct rnl_ipcp_select_policy_set_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->path)
                rkfree(attrs->path);

        if (attrs->name)
                rkfree(attrs->name);

        LOG_DBG("rnl_ipcp_select_policy_set_req_msg_attrs destroyed correctly");
        rkfree(attrs);

        return 0;
}

static void sdup_crypto_state_destroy(struct sdup_crypto_state * state)
{
	if (!state)
		return;

	if (state->mac_key_tx)
		buffer_destroy(state->mac_key_tx);

	if (state->mac_key_rx)
		buffer_destroy(state->mac_key_rx);

	if (state->encrypt_key_tx)
		buffer_destroy(state->encrypt_key_tx);

	if (state->encrypt_key_rx)
		buffer_destroy(state->encrypt_key_rx);

	if (state->iv_tx)
		buffer_destroy(state->iv_tx);

	if (state->iv_rx)
		buffer_destroy(state->iv_rx);

	rkfree(state);
}

static int
rnl_ipcp_update_crypto_state_msg_attrs_destroy(
                struct rnl_ipcp_update_crypto_state_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->state)
        	sdup_crypto_state_destroy(attrs->state);

        rkfree(attrs);

        LOG_DBG("rnl_ipcp_update_crypto_state_req_msg_attrs destroyed correctly");

        return 0;
}

int rnl_msg_destroy(struct rnl_msg * msg)
{
        if (!msg)
                return -1;

        switch(msg->attr_type) {
        case RNL_MSG_ATTRS_ALLOCATE_FLOW_REQUEST:
                rnl_ipcm_alloc_flow_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_ALLOCATE_FLOW_RESPONSE:
                rnl_alloc_flow_resp_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_DEALLOCATE_FLOW_REQUEST:
                rnl_ipcm_dealloc_flow_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_ASSIGN_TO_DIF_REQUEST:
                rnl_ipcm_assign_to_dif_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_UPDATE_DIF_CONFIG_REQUEST:
                rnl_ipcm_update_dif_config_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_REG_UNREG_REQUEST:
                rnl_ipcm_reg_app_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_CONN_CREATE_REQUEST:
                rnl_ipcp_conn_create_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_CONN_CREATE_ARRIVED:
                rnl_ipcp_conn_create_arrived_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_CONN_UPDATE_REQUEST:
                rnl_ipcp_conn_update_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_CONN_DESTROY_REQUEST:
                rnl_ipcp_conn_destroy_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_RMT_PFFE_MODIFY_REQUEST:
                rnl_rmt_mod_pffe_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_QUERY_RIB_REQUEST:
                rnl_ipcm_query_rib_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_SET_POLICY_SET_PARAM_REQUEST:
                rnl_ipcp_set_policy_set_param_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_SELECT_POLICY_SET_REQUEST:
                rnl_ipcp_select_policy_set_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_UPDATE_CRYPTO_STATE_REQUEST:
        	rnl_ipcp_update_crypto_state_msg_attrs_destroy(msg->attrs);
                break;
        default:
                break;
        }

        rkfree(msg);

        return 0;
}
EXPORT_SYMBOL(rnl_msg_destroy);

static int parse_flow_spec(struct nlattr * fspec_attr,
                           struct flow_spec * fspec_struct)
{
        struct nla_policy attr_policy[FSPEC_ATTR_MAX + 1];
        struct nlattr *   attrs[FSPEC_ATTR_MAX + 1];

        attr_policy[FSPEC_ATTR_AVG_BWITH].type               = NLA_U32;
        attr_policy[FSPEC_ATTR_AVG_BWITH].len                = 4;
        attr_policy[FSPEC_ATTR_AVG_SDU_BWITH].type           = NLA_U32;
        attr_policy[FSPEC_ATTR_AVG_SDU_BWITH].len            = 4;
        attr_policy[FSPEC_ATTR_DELAY].type                   = NLA_U32;
        attr_policy[FSPEC_ATTR_DELAY].len                    = 4;
        attr_policy[FSPEC_ATTR_JITTER].type                  = NLA_U32;
        attr_policy[FSPEC_ATTR_JITTER].len                   = 4;
        attr_policy[FSPEC_ATTR_MAX_GAP].type                 = NLA_U32;
        attr_policy[FSPEC_ATTR_MAX_GAP].len                  = 4;
        attr_policy[FSPEC_ATTR_MAX_SDU_SIZE].type            = NLA_U32;
        attr_policy[FSPEC_ATTR_MAX_SDU_SIZE].len             = 4;
        attr_policy[FSPEC_ATTR_IN_ORD_DELIVERY].type         = NLA_FLAG;
        attr_policy[FSPEC_ATTR_IN_ORD_DELIVERY].len          = 0;
        attr_policy[FSPEC_ATTR_PART_DELIVERY].type           = NLA_FLAG;
        attr_policy[FSPEC_ATTR_PART_DELIVERY].len            = 0;
        attr_policy[FSPEC_ATTR_PEAK_BWITH_DURATION].type     = NLA_U32;
        attr_policy[FSPEC_ATTR_PEAK_BWITH_DURATION].len      = 4;
        attr_policy[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION].type = NLA_U32;
        attr_policy[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION].len  = 4;
        attr_policy[FSPEC_ATTR_UNDETECTED_BER].type          = NLA_U32;
        attr_policy[FSPEC_ATTR_UNDETECTED_BER].len           = 4;

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
                fspec_struct->undetected_bit_error_rate =
                        nla_get_u32(attrs[FSPEC_ATTR_UNDETECTED_BER]);

        if (attrs[FSPEC_ATTR_UNDETECTED_BER])
                fspec_struct->undetected_bit_error_rate =
                        nla_get_u32(attrs[FSPEC_ATTR_UNDETECTED_BER]);

        fspec_struct->partial_delivery =
                nla_get_flag(attrs[FSPEC_ATTR_PART_DELIVERY]);

        fspec_struct->ordered_delivery =
                nla_get_flag(attrs[FSPEC_ATTR_IN_ORD_DELIVERY]);

        if (attrs[FSPEC_ATTR_MAX_GAP])
                fspec_struct->max_allowable_gap =
                        (int) nla_get_u32(attrs[FSPEC_ATTR_MAX_GAP]);

        if (attrs[FSPEC_ATTR_DELAY])
                fspec_struct->delay =
                        nla_get_u32(attrs[FSPEC_ATTR_DELAY]);

        if (attrs[FSPEC_ATTR_MAX_SDU_SIZE])
                fspec_struct->max_sdu_size =
                        nla_get_u32(attrs[FSPEC_ATTR_MAX_SDU_SIZE]);

        return 0;
}

static int parse_port_id_altlist_entries(struct nlattr * nested_attr,
					 struct port_id_altlist *alts)
{
        int             rem = 0;
	int		i = 0;
        struct nlattr * nla;

        if (!nested_attr) {
                LOG_ERR("Bogus nested attribute (ports) passed, bailing out");
                return -1;
        }

        if (!alts) {
                LOG_ERR("Bogus port_id_altlist entry passed, bailing out");
                return -1;
        }

        alts->num_ports	= nla_len(nested_attr);
        alts->ports	= rkzalloc(alts->num_ports, GFP_KERNEL);
        LOG_DBG("Allocated %zd bytes in %pk", alts->num_ports, alts->ports);

        if (!alts->ports) {
                LOG_ERR("Could not allocate memory for port-ids");
                return -1;
        }

        for (nla = (struct nlattr*) nla_data(nested_attr),
                     rem = nla_len(nested_attr);
             nla_ok(nla, rem);
             nla = nla_next(nla, &rem), i++) {
                alts->ports[i] = nla_get_u32(nla);
        }

        alts->num_ports = (size_t) i;

        if (rem)
                LOG_WARN("Missing bits to pars");

        return 0;
}

static int parse_port_id_altlist(struct nlattr * attr,
			     struct port_id_altlist *alts)
{
        struct nla_policy attr_policy[PIA_ATTR_MAX + 1];
        struct nlattr *   attrs[PIA_ATTR_MAX + 1];

        attr_policy[PIA_ATTR_PORTIDS].type = NLA_NESTED;
        attr_policy[PIA_ATTR_PORTIDS].len  = 0;

        if (nla_parse_nested(attrs,
                             PIA_ATTR_MAX,
                             attr,
                             attr_policy) < 0)
                return -1;

        if (attrs[PIA_ATTR_PORTIDS]) {
                if (parse_port_id_altlist_entries(attrs[PIA_ATTR_PORTIDS],
                                                   alts))
                        return -1;

        }

        return 0;
}

static int parse_pdu_fte_altlists(struct nlattr *        nested_attr,
                                  struct mod_pff_entry * entry)
{
        int             rem = 0;
        struct nlattr * nla;
        int             entries_with_problems = 0;
        int             total_entries         = 0;

        if (!nested_attr) {
                LOG_ERR("Bogus nested attribute (ports) passed, bailing out");
                return -1;
        }

        if (!entry) {
                LOG_ERR("Bogus PFF entry passed, bailing out");
                return -1;
        }

	INIT_LIST_HEAD(&entry->port_id_altlists);

        for (nla = (struct nlattr*) nla_data(nested_attr),
                     rem = nla_len(nested_attr);
             nla_ok(nla, rem);
             nla = nla_next(nla, &rem), total_entries++) {
		struct port_id_altlist *alts;

		alts = rkzalloc(sizeof(*entry), GFP_KERNEL);
		if (!alts) {
			entries_with_problems++;
			continue;
		}

		if (parse_port_id_altlist(nla, alts)) {
			rkfree(alts);
			entries_with_problems++;
			continue;
		}

		list_add(&alts->next, &entry->port_id_altlists);
        }

        if (rem)
                LOG_WARN("Missing bits to pars");

        if (entries_with_problems > 0)
                LOG_WARN("Problems parsing %d out of %d entries",
                         entries_with_problems,
                         total_entries);

        return 0;
}

static int parse_pdu_fte_list_entry(struct nlattr *        attr,
                                    struct mod_pff_entry * mpfe)
{
        struct nla_policy attr_policy[PFFELE_ATTR_MAX + 1];
        struct nlattr *   attrs[PFFELE_ATTR_MAX + 1];

        attr_policy[PFFELE_ATTR_ADDRESS].type = NLA_U32;
        attr_policy[PFFELE_ATTR_ADDRESS].len  = 4;
        attr_policy[PFFELE_ATTR_QOSID].type   = NLA_U32;
        attr_policy[PFFELE_ATTR_QOSID].len    = 4;
        attr_policy[PFFELE_ATTR_PORT_ID_ALTLISTS].type = NLA_NESTED;
        attr_policy[PFFELE_ATTR_PORT_ID_ALTLISTS].len  = 0;

        if (nla_parse_nested(attrs,
                             PFFELE_ATTR_MAX,
                             attr,
                             attr_policy) < 0)
                return -1;

        if (attrs[PFFELE_ATTR_ADDRESS])
                mpfe->fwd_info =
                        nla_get_u32(attrs[PFFELE_ATTR_ADDRESS]);

        if (attrs[PFFELE_ATTR_QOSID])
                mpfe->qos_id =
                        nla_get_u32(attrs[PFFELE_ATTR_QOSID]);

        if (attrs[PFFELE_ATTR_PORT_ID_ALTLISTS]) {
                if (parse_pdu_fte_altlists(attrs[PFFELE_ATTR_PORT_ID_ALTLISTS],
                                           mpfe))
                        return -1;

        }

        return 0;
}

static int parse_app_name_info(const struct nlattr * name_attr,
                               struct name *         name_struct)
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

        attr_policy[APNI_ATTR_PROCESS_NAME].type     = NLA_STRING;
        attr_policy[APNI_ATTR_PROCESS_NAME].len      = 0;
        attr_policy[APNI_ATTR_PROCESS_INSTANCE].type = NLA_STRING;
        attr_policy[APNI_ATTR_PROCESS_INSTANCE].len  = 0;
        attr_policy[APNI_ATTR_ENTITY_NAME].type      = NLA_STRING;
        attr_policy[APNI_ATTR_ENTITY_NAME].len       = 0;
        attr_policy[APNI_ATTR_ENTITY_INSTANCE].type  = NLA_STRING;
        attr_policy[APNI_ATTR_ENTITY_INSTANCE].len   = 0;

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

        attr_policy[IPCP_CONFIG_ENTRY_ATTR_NAME].type  = NLA_STRING;
        attr_policy[IPCP_CONFIG_ENTRY_ATTR_NAME].len   = 0;
        attr_policy[IPCP_CONFIG_ENTRY_ATTR_VALUE].type = NLA_STRING;
        attr_policy[IPCP_CONFIG_ENTRY_ATTR_VALUE].len  = 0;

        if (nla_parse_nested(attrs, IPCP_CONFIG_ENTRY_ATTR_MAX,
                             name_attr, attr_policy) < 0)
                return -1;

        if (attrs[IPCP_CONFIG_ENTRY_ATTR_NAME])
                entry->name = nla_dup_string(attrs[IPCP_CONFIG_ENTRY_ATTR_NAME], GFP_KERNEL);

        if (attrs[IPCP_CONFIG_ENTRY_ATTR_VALUE])
                entry->value = nla_dup_string(attrs[IPCP_CONFIG_ENTRY_ATTR_VALUE], GFP_KERNEL);

        return 0;
}

static int parse_list_of_ipcp_config_entries(struct nlattr *     nested_attr,
                                             struct dif_config * dif_config)
{
        struct nlattr *            nla;
        struct ipcp_config_entry * entry;
        struct ipcp_config *       config;
        int                        rem                   = 0;
        int                        entries_with_problems = 0;
        int                        total_entries         = 0;

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

static int parse_policy_param(struct nlattr * attr, struct policy_parm * param)
{
        struct nla_policy attr_policy[PPA_ATTR_MAX + 1];
        struct nlattr *   attrs[PPA_ATTR_MAX + 1];

        if (!attr || !param) {
                LOG_ERR("Bogus input parameters, cannot parse policy info");
                return -1;
        }

        attr_policy[PPA_ATTR_NAME].type  = NLA_STRING;
        attr_policy[PPA_ATTR_NAME].len   = 0;
        attr_policy[PPA_ATTR_VALUE].type = NLA_STRING;
        attr_policy[PPA_ATTR_VALUE].len  = 0;

        if (nla_parse_nested(attrs, PPA_ATTR_MAX, attr, attr_policy))
                return -1;

        if (attrs[PPA_ATTR_NAME])
                policy_param_name_set(param,
                                      nla_dup_string(attrs[PPA_ATTR_NAME],
                                                     GFP_KERNEL));
        else
                policy_param_name_set(param, NULL);

        if (attrs[PPA_ATTR_VALUE])
                policy_param_value_set(param,
                                       nla_dup_string(attrs[PPA_ATTR_VALUE],
                                                      GFP_KERNEL));
        else
                policy_param_value_set(param, NULL);

        return 0;
}

static int parse_policy_param_list(struct nlattr * nested_attr,
                                   struct policy * p)
{
        struct nlattr *      nla;
        int                  rem                   = 0;
        int                  entries_with_problems = 0;
        int                  total_entries         = 0;

        if (!nested_attr) {
                LOG_ERR("Bogus attribute passed, bailing out");
                return -1;
        }

        if (!p) {
                LOG_ERR("Bogus policy struct passed, bailing out");
                return -1;
        }

        for (nla = (struct nlattr *) nla_data(nested_attr),
                     rem = nla_len(nested_attr);
             nla_ok(nla, rem);
             nla = nla_next(nla, &(rem))) {
                struct policy_parm * param;

                total_entries++;

                param = policy_param_create();
                if (!param) {
                	LOG_ERR("Parameter is null");
                        entries_with_problems++;
                        continue;
                }

                if (parse_policy_param(nla, param)) {
                	LOG_ERR("Problems parsing parameter");
                        policy_param_destroy(param);
                        entries_with_problems++;
                        continue;
                }

                if (policy_param_bind(p, param)) {
                	LOG_ERR("Problems binding parameter to policy");
                        policy_param_destroy(param);
                        entries_with_problems++;
                        continue;
                }
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

static int parse_policy(struct nlattr * p_attr, struct policy * p)
{
        struct nla_policy attr_policy[PA_ATTR_MAX + 1];
        struct nlattr *   attrs[PA_ATTR_MAX + 1];

        if (!p_attr || !p) {
                LOG_ERR("Bogus input parameters, cannot parse policy info");
                return -1;
        }

        attr_policy[PA_ATTR_NAME].type       = NLA_STRING;
        attr_policy[PA_ATTR_NAME].len        = 0;
        attr_policy[PA_ATTR_VERSION].type    = NLA_STRING;
        attr_policy[PA_ATTR_VERSION].len     = 0;
        attr_policy[PA_ATTR_PARAMETERS].type = NLA_NESTED;
        attr_policy[PA_ATTR_PARAMETERS].len  = 0;

        if (nla_parse_nested(attrs, PA_ATTR_MAX, p_attr, attr_policy))
                return -1;

        if (attrs[PA_ATTR_NAME])
                policy_name_set(p, nla_dup_string(attrs[PA_ATTR_NAME],
                                                  GFP_KERNEL));
        else
                policy_name_set(p, NULL);

        if (attrs[PA_ATTR_VERSION])
                policy_version_set(p, nla_dup_string(attrs[PA_ATTR_VERSION],
                                                     GFP_KERNEL));
        else
                policy_version_set(p, NULL);

        if (attrs[PA_ATTR_PARAMETERS])
                if (parse_policy_param_list(attrs[PA_ATTR_PARAMETERS], p))
                        return -1;

        return 0;
}

static int parse_dt_cons(struct nlattr *  attr,
                         struct dt_cons * dt_cons)
{
        struct nla_policy attr_policy[DTC_ATTR_MAX + 1];
        struct nlattr *   attrs[DTC_ATTR_MAX + 1];

        attr_policy[DTC_ATTR_QOS_ID].type          = NLA_U16;
        attr_policy[DTC_ATTR_QOS_ID].len           = 2;
        attr_policy[DTC_ATTR_PORT_ID].type         = NLA_U16;
        attr_policy[DTC_ATTR_PORT_ID].len          = 2;
        attr_policy[DTC_ATTR_CEP_ID].type          = NLA_U16;
        attr_policy[DTC_ATTR_CEP_ID].len           = 2;
        attr_policy[DTC_ATTR_SEQ_NUM].type         = NLA_U16;
        attr_policy[DTC_ATTR_SEQ_NUM].len          = 2;
        attr_policy[DTC_ATTR_CTRL_SEQ_NUM].type    = NLA_U16;
        attr_policy[DTC_ATTR_CTRL_SEQ_NUM].len     = 2;
        attr_policy[DTC_ATTR_ADDRESS].type         = NLA_U16;
        attr_policy[DTC_ATTR_ADDRESS].len          = 2;
        attr_policy[DTC_ATTR_LENGTH].type          = NLA_U16;
        attr_policy[DTC_ATTR_LENGTH].len           = 2;
        attr_policy[DTC_ATTR_MAX_PDU_SIZE].type    = NLA_U32;
        attr_policy[DTC_ATTR_MAX_PDU_SIZE].len     = 4;
        attr_policy[DTC_ATTR_MAX_PDU_LIFE].type    = NLA_U32;
        attr_policy[DTC_ATTR_MAX_PDU_LIFE].len     = 4;
        attr_policy[DTC_ATTR_RATE].type            = NLA_U16;
        attr_policy[DTC_ATTR_RATE].len             = 2;
        attr_policy[DTC_ATTR_FRAME].type           = NLA_U16;
        attr_policy[DTC_ATTR_FRAME].len            = 2;
        attr_policy[DTC_ATTR_DIF_INTEGRITY].type   = NLA_FLAG;
        attr_policy[DTC_ATTR_DIF_INTEGRITY].len    = 0;

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

        if (attrs[DTC_ATTR_CTRL_SEQ_NUM])
                dt_cons->ctrl_seq_num_length =
                        nla_get_u16(attrs[DTC_ATTR_CTRL_SEQ_NUM]);

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

        if (attrs[DTC_ATTR_RATE])
                dt_cons->rate_length =
                        nla_get_u16(attrs[DTC_ATTR_RATE]);

        if (attrs[DTC_ATTR_FRAME])
                dt_cons->frame_length =
                        nla_get_u16(attrs[DTC_ATTR_FRAME]);

        dt_cons->dif_integrity = nla_get_flag(attrs[DTC_ATTR_DIF_INTEGRITY]);

        return 0;
}

static int parse_efcp_config(struct nlattr *      efcp_config_attr,
                             struct efcp_config * efcp_config)
{
        struct nla_policy attr_policy[EFCPC_ATTR_MAX + 1];
        struct nlattr *   attrs[EFCPC_ATTR_MAX + 1];

        attr_policy[EFCPC_ATTR_DATA_TRANS_CONS].type     = NLA_NESTED;
        attr_policy[EFCPC_ATTR_DATA_TRANS_CONS].len      = 0;
        attr_policy[EFCPC_ATTR_QOS_CUBES].type           = NLA_NESTED;
        attr_policy[EFCPC_ATTR_QOS_CUBES].len            = 0;
        attr_policy[EFCPC_ATTR_UNKNOWN_FLOW_POLICY].type = NLA_NESTED;
        attr_policy[EFCPC_ATTR_UNKNOWN_FLOW_POLICY].len = 0;

        if (nla_parse_nested(attrs,
                             EFCPC_ATTR_MAX,
                             efcp_config_attr,
                             attr_policy) < 0)
                goto parse_fail;

        if (attrs[EFCPC_ATTR_DATA_TRANS_CONS]) {
                if (!efcp_config->dt_cons)
                        goto parse_fail;


                if (parse_dt_cons(attrs[EFCPC_ATTR_DATA_TRANS_CONS],
                                  efcp_config->dt_cons))
                        goto parse_fail;
        }

        if (attrs[EFCPC_ATTR_UNKNOWN_FLOW_POLICY]) {
                if (parse_policy(attrs[EFCPC_ATTR_UNKNOWN_FLOW_POLICY],
                                 efcp_config->unknown_flow))
                        goto parse_fail;
        }

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("efcp config attributes"));
        return -1;
}

static int parse_dup_config_entry(struct nlattr *           dup_config_entry_attr,
                                  struct dup_config_entry * dup_config_entry)
{
        struct nla_policy attr_policy[AUTHP_ATTR_MAX + 1];
        struct nlattr *   attrs[AUTHP_ATTR_MAX + 1];

        if (!dup_config_entry_attr || !dup_config_entry) {
                LOG_ERR("Bogus input parameters, cannot parse policy info");
                return -1;
        }

	attr_policy[AUTHP_AUTH_POLICY].type = NLA_NESTED;
	attr_policy[AUTHP_AUTH_POLICY].len = 0;
	attr_policy[AUTHP_ENCRYPT_POLICY].type = NLA_NESTED;
	attr_policy[AUTHP_ENCRYPT_POLICY].len = 0;
	attr_policy[AUTHP_ERROR_CHECK_POLICY].type = NLA_NESTED;
	attr_policy[AUTHP_ERROR_CHECK_POLICY].len = 0;
	attr_policy[AUTHP_TTL_POLICY].type = NLA_NESTED;
	attr_policy[AUTHP_TTL_POLICY].len = 0;

        if (nla_parse_nested(attrs,
        		     AUTHP_ATTR_MAX,
        		     dup_config_entry_attr,
                             attr_policy) < 0)
                goto parse_fail;

        if (attrs[AUTHP_ENCRYPT_POLICY]) {
        	dup_config_entry->crypto_policy = policy_create();
        	if (!dup_config_entry->crypto_policy)
        		goto parse_fail;

        	if (parse_policy(attrs[AUTHP_ENCRYPT_POLICY],
        			 dup_config_entry->crypto_policy))
        		goto parse_fail;
        }

        if (attrs[AUTHP_ERROR_CHECK_POLICY]) {
        	dup_config_entry->error_check_policy = policy_create();
        	if (!dup_config_entry->error_check_policy)
        		goto parse_fail;

        	if (parse_policy(attrs[AUTHP_ERROR_CHECK_POLICY],
        			 dup_config_entry->error_check_policy))
        		goto parse_fail;
        }

        if (attrs[AUTHP_TTL_POLICY]) {
        	dup_config_entry->ttl_policy = policy_create();
        	if (!dup_config_entry->ttl_policy)
        		goto parse_fail;

        	if (parse_policy(attrs[AUTHP_TTL_POLICY],
        			 dup_config_entry->ttl_policy))
        		goto parse_fail;
        }

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("dup config attributes"));
        return -1;
}

static int parse_s_dup_config_entry(struct nlattr *           sdup_config_entry_attr,
                                    struct dup_config_entry * dup_config_entry)
{
        struct nla_policy attr_policy[SAUTHP_ATTR_MAX + 1];
        struct nlattr *   attrs[SAUTHP_ATTR_MAX + 1];

        if (!sdup_config_entry_attr || !dup_config_entry) {
                LOG_ERR("Bogus input parameters, cannot parse policy info");
                return -1;
        }

	attr_policy[SAUTHP_UNDER_DIF].type = NLA_STRING;
	attr_policy[SAUTHP_UNDER_DIF].len = 0;
	attr_policy[SAUTHP_AUTH_PROFILE].type = NLA_NESTED;
	attr_policy[SAUTHP_AUTH_PROFILE].len = 0;

        if (nla_parse_nested(attrs,
        		     SAUTHP_ATTR_MAX,
        		     sdup_config_entry_attr,
                             attr_policy) < 0)
                goto parse_fail;

        if (attrs[SAUTHP_UNDER_DIF]) {
        	dup_config_entry->n_1_dif_name =
        			nla_dup_string(attrs[SAUTHP_UNDER_DIF], GFP_KERNEL);
        }

        if (attrs[SAUTHP_AUTH_PROFILE]) {
                if (parse_dup_config_entry(attrs[SAUTHP_AUTH_PROFILE],
                		           dup_config_entry))
                	goto parse_fail;
        }

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("specific dup config attributes"));
        return -1;
}

static int parse_list_of_dup_config_entries(struct nlattr *      nested_attr,
                             	     	    struct sdup_config * sdup_config)
{
        struct nlattr *            nla;
        struct dup_config_entry *  entry;
        struct dup_config *        config;
        int                        rem                   = 0;
        int                        entries_with_problems = 0;
        int                        total_entries         = 0;

        if (!nested_attr || !sdup_config) {
                LOG_ERR("Bogus input parameters, cannot parse policy info");
                return -1;
        }

        for (nla = (struct nlattr*) nla_data(nested_attr),
                     rem = nla_len(nested_attr);
             nla_ok(nla, rem);
             nla = nla_next(nla, &(rem))) {
                total_entries++;

                entry = dup_config_entry_create();
                if (!entry) {
                        entries_with_problems++;
                        continue;
                }

                if (parse_s_dup_config_entry(nla, entry) < 0) {
                        dup_config_entry_destroy(entry);
                        entries_with_problems++;
                        continue;
                }

                config = dup_config_create();
                if (!config) {
                	dup_config_entry_destroy(entry);
                        entries_with_problems++;
                        continue;
                }
                config->entry = entry;
                list_add(&config->next, &sdup_config->specific_dup_confs);
        }

        if (rem > 0) {
                LOG_WARN("Missing bits to parse");
        }

        if (entries_with_problems > 0)
                LOG_WARN("Problems parsing %d out of %d dup config entries",
                         entries_with_problems,
                         total_entries);

        return 0;
}

static int parse_sdup_config(struct nlattr *      sdup_config_attr,
                             struct sdup_config * sdup_config)
{
        struct nla_policy attr_policy[SECMANC_ATTR_MAX + 1];
        struct nlattr *   attrs[SECMANC_ATTR_MAX + 1];

        if (!sdup_config_attr || !sdup_config) {
                LOG_ERR("Bogus input parameters, cannot parse policy info");
                return -1;
        }

	attr_policy[SECMANC_POLICY_SET].type = NLA_NESTED;
	attr_policy[SECMANC_POLICY_SET].len = 0;
	attr_policy[SECMANC_DEFAULT_AUTH_SDUP_POLICY].type = NLA_NESTED;
	attr_policy[SECMANC_DEFAULT_AUTH_SDUP_POLICY].len = 0;
	attr_policy[SECMANC_SPECIFIC_AUTH_SDUP_POLICIES].type = NLA_NESTED;
	attr_policy[SECMANC_SPECIFIC_AUTH_SDUP_POLICIES].len = 0;

        if (nla_parse_nested(attrs,
        		     SECMANC_ATTR_MAX,
        		     sdup_config_attr,
                             attr_policy) < 0)
                goto parse_fail;

        if (attrs[SECMANC_DEFAULT_AUTH_SDUP_POLICY]) {
        	sdup_config->default_dup_conf = dup_config_entry_create();
        	if (!sdup_config->default_dup_conf)
        		goto parse_fail;

        	sdup_config->default_dup_conf->n_1_dif_name = get_zero_length_string();
        	if (parse_dup_config_entry(attrs[SECMANC_DEFAULT_AUTH_SDUP_POLICY],
        				   sdup_config->default_dup_conf))
        		goto parse_fail;
        }

        if (attrs[SECMANC_SPECIFIC_AUTH_SDUP_POLICIES]) {
                if (parse_list_of_dup_config_entries(attrs[SECMANC_SPECIFIC_AUTH_SDUP_POLICIES],
                				     sdup_config))
                	goto parse_fail;
        }

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("sdup config attributes"));
        return -1;
}

static int parse_pff_config(struct nlattr *     pff_config_attr,
                            struct pff_config * pff_config)
{
        struct nla_policy attr_policy[PFFC_ATTR_MAX + 1];
        struct nlattr *   attrs[PFFC_ATTR_MAX + 1];

        attr_policy[PFFC_ATTR_POLICY_SET].type = NLA_NESTED;
        attr_policy[PFFC_ATTR_POLICY_SET].len  = 0;

        if (nla_parse_nested(attrs,
        		     PFFC_ATTR_MAX,
                             pff_config_attr,
                             attr_policy) < 0)
                goto parse_fail;

        if (attrs[PFFC_ATTR_POLICY_SET]) {
                if (parse_policy(attrs[PFFC_ATTR_POLICY_SET],
                                 pff_config->policy_set))
                        goto parse_fail;
        }

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("pff config attributes"));
        return -1;
}

static int parse_rmt_config(struct nlattr *     rmt_config_attr,
                            struct rmt_config * rmt_config)
{
        struct nla_policy attr_policy[RMTC_ATTR_MAX + 1];
        struct nlattr *   attrs[RMTC_ATTR_MAX + 1];

        attr_policy[RMTC_ATTR_POLICY_SET].type = NLA_NESTED;
        attr_policy[RMTC_ATTR_POLICY_SET].len  = 0;
        attr_policy[RMTC_ATTR_PFF_CONFIG].type = NLA_NESTED;
        attr_policy[RMTC_ATTR_PFF_CONFIG].len  = 0;

        if (nla_parse_nested(attrs,
        		     RMTC_ATTR_MAX,
                             rmt_config_attr,
                             attr_policy) < 0)
                goto parse_fail;

        if (attrs[RMTC_ATTR_POLICY_SET]) {
                if (parse_policy(attrs[RMTC_ATTR_POLICY_SET],
                                 rmt_config->policy_set))
                        goto parse_fail;
        }
        if (attrs[RMTC_ATTR_PFF_CONFIG]) {
                if (parse_pff_config(attrs[RMTC_ATTR_PFF_CONFIG],
                                 rmt_config->pff_conf))
                        goto parse_fail;

        }

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("rmt config attributes"));
        return -1;
}

static int parse_dif_config(struct nlattr *     dif_config_attr,
                            struct dif_config * dif_config)
{
        struct nla_policy attr_policy[DCONF_ATTR_MAX + 1];
        struct nlattr *   attrs[DCONF_ATTR_MAX + 1];

        attr_policy[DCONF_ATTR_IPCP_CONFIG_ENTRIES].type = NLA_NESTED;
        attr_policy[DCONF_ATTR_IPCP_CONFIG_ENTRIES].len  = 0;
        attr_policy[DCONF_ATTR_ADDRESS].type             = NLA_U32;
        attr_policy[DCONF_ATTR_ADDRESS].len              = 4;
        attr_policy[DCONF_ATTR_EFCPC].type               = NLA_NESTED;
        attr_policy[DCONF_ATTR_EFCPC].len                = 0;
        attr_policy[DCONF_ATTR_RMTC].type                = NLA_NESTED;
        attr_policy[DCONF_ATTR_RMTC].len                 = 0;
        attr_policy[DCONF_ATTR_SECMANC].type             = NLA_NESTED;
        attr_policy[DCONF_ATTR_SECMANC].len              = 0;
        attr_policy[DCONF_ATTR_FAC].type                 = NLA_NESTED;
        attr_policy[DCONF_ATTR_FAC].len                  = 0;
        attr_policy[DCONF_ATTR_ETC].type                 = NLA_NESTED;
        attr_policy[DCONF_ATTR_ETC].len                  = 0;
        attr_policy[DCONF_ATTR_NSMC].type                = NLA_NESTED;
        attr_policy[DCONF_ATTR_NSMC].len                 = 0;
        attr_policy[DCONF_ATTR_RAC].type                 = NLA_NESTED;
        attr_policy[DCONF_ATTR_RAC].len                  = 0;
        attr_policy[DCONF_ATTR_ROUTINGC].type            = NLA_NESTED;
        attr_policy[DCONF_ATTR_ROUTINGC].len             = 0;

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

        if (attrs[DCONF_ATTR_ADDRESS])
                dif_config->address = nla_get_u32(attrs[DCONF_ATTR_ADDRESS]);

        if (attrs[DCONF_ATTR_EFCPC]) {
                dif_config->efcp_config = efcp_config_create();
                if (!dif_config->efcp_config)
                        goto parse_fail;

                if (parse_efcp_config(attrs[DCONF_ATTR_EFCPC],
                                      dif_config->efcp_config))
                        goto parse_fail;
        }

        if (attrs[DCONF_ATTR_RMTC]) {
                dif_config->rmt_config = rmt_config_create();
                if (!dif_config->rmt_config)
                	goto parse_fail;

                if (parse_rmt_config(attrs[DCONF_ATTR_RMTC],
                		     dif_config->rmt_config))
                	goto parse_fail;
        }

        if (attrs[DCONF_ATTR_SECMANC]) {
        	dif_config->sdup_config = sdup_config_create();
        	if (!dif_config->sdup_config)
        		goto parse_fail;

                if (parse_sdup_config(attrs[DCONF_ATTR_SECMANC],
                		      dif_config->sdup_config))
                	goto parse_fail;

                if (!dif_config->sdup_config->default_dup_conf) {
                	sdup_config_destroy(dif_config->sdup_config);
                	dif_config->sdup_config = NULL;
                }
        }

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("dif config attributes"));
        if (dif_config->efcp_config)
                efcp_config_destroy(dif_config->efcp_config);
        return -1;
}

static int parse_dif_info(struct nlattr *   dif_config_attr,
                          struct dif_info * dif_info)
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
                dif_info->type = nla_dup_string(attrs[DINFO_ATTR_DIF_TYPE],
                                                GFP_KERNEL);
        else
                dif_info->type = NULL;

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

static int parse_dtcp_wb_fctrl_config(struct nlattr * attr,
                                      struct dtcp_config * cfg)
{
        struct nla_policy attr_policy[DWFCC_ATTR_MAX + 1];
        struct nlattr * attrs[DWFCC_ATTR_MAX + 1];

        attr_policy[DWFCC_ATTR_MAX_CLOSED_WINDOW_Q_LENGTH].type = NLA_U32;
        attr_policy[DWFCC_ATTR_MAX_CLOSED_WINDOW_Q_LENGTH].len  = 4;
        attr_policy[DWFCC_ATTR_INITIAL_CREDIT].type             = NLA_U32;
        attr_policy[DWFCC_ATTR_INITIAL_CREDIT].len              = 4;
        attr_policy[DWFCC_ATTR_RCVR_FLOW_CTRL_POLICY].type      = NLA_NESTED;
        attr_policy[DWFCC_ATTR_RCVR_FLOW_CTRL_POLICY].len       = 0;
        attr_policy[DWFCC_ATTR_TX_CTRL_POLICY].type             = NLA_NESTED;
        attr_policy[DWFCC_ATTR_TX_CTRL_POLICY].len              = 0;

        if (nla_parse_nested(attrs,
                             DWFCC_ATTR_MAX,
                             attr, attr_policy))
                return -1;

        if (attrs[DWFCC_ATTR_MAX_CLOSED_WINDOW_Q_LENGTH])
                dtcp_max_closed_winq_length_set(cfg,
                                                nla_get_u32(attrs[DWFCC_ATTR_MAX_CLOSED_WINDOW_Q_LENGTH]));

        if (attrs[DWFCC_ATTR_INITIAL_CREDIT])
                dtcp_initial_credit_set(cfg,
                                        nla_get_u32(attrs[DWFCC_ATTR_INITIAL_CREDIT]));

        if (attrs[DWFCC_ATTR_RCVR_FLOW_CTRL_POLICY])
                if (parse_policy(attrs[DWFCC_ATTR_RCVR_FLOW_CTRL_POLICY],
                                 dtcp_rcvr_flow_control(cfg)))
                        return -1;

        if (attrs[DWFCC_ATTR_TX_CTRL_POLICY])
                if (parse_policy(attrs[DWFCC_ATTR_TX_CTRL_POLICY],
                                 dtcp_tx_control(cfg)))
                        return -1;

        return 0;
}

static int parse_dtcp_rb_fctrl_config(struct nlattr * attr,
                                      struct dtcp_config * cfg)
{
        struct nla_policy attr_policy[DRFCC_ATTR_MAX + 1];
        struct nlattr * attrs[DRFCC_ATTR_MAX + 1];

        attr_policy[DRFCC_ATTR_SEND_RATE].type                = NLA_U32;
        attr_policy[DRFCC_ATTR_SEND_RATE].len                 = 4;
        attr_policy[DRFCC_ATTR_TIME_PERIOD].type              = NLA_U32;
        attr_policy[DRFCC_ATTR_TIME_PERIOD].len               = 4;
        attr_policy[DRFCC_ATTR_NO_RATE_SDOWN_POLICY].type     = NLA_NESTED;
        attr_policy[DRFCC_ATTR_NO_RATE_SDOWN_POLICY].len      = 0;
        attr_policy[DRFCC_ATTR_NO_OVERR_DEF_PEAK_POLICY].type = NLA_NESTED;
        attr_policy[DRFCC_ATTR_NO_OVERR_DEF_PEAK_POLICY].len  = 0;
        attr_policy[DRFCC_ATTR_RATE_REDUC_POLICY].type        = NLA_NESTED;
        attr_policy[DRFCC_ATTR_RATE_REDUC_POLICY].len         = 0;

        if (nla_parse_nested(attrs,
                             DRFCC_ATTR_MAX,
                             attr, attr_policy))
                return -1;

        if (attrs[DRFCC_ATTR_SEND_RATE])
                dtcp_sending_rate_set(cfg,
                                      nla_get_u32(attrs[DRFCC_ATTR_SEND_RATE]));

        if (attrs[DRFCC_ATTR_TIME_PERIOD])
                dtcp_time_period_set(cfg,
                                     nla_get_u32(attrs[DRFCC_ATTR_TIME_PERIOD]));

        if (attrs[DRFCC_ATTR_NO_RATE_SDOWN_POLICY])
                if (parse_policy(attrs[DRFCC_ATTR_NO_RATE_SDOWN_POLICY],
                                 dtcp_no_rate_slow_down(cfg)))
                        return -1;

        if (attrs[DRFCC_ATTR_NO_OVERR_DEF_PEAK_POLICY])
                if (parse_policy(attrs[DRFCC_ATTR_NO_OVERR_DEF_PEAK_POLICY],
                                 dtcp_no_override_default_peak(cfg)))
                        return -1;

        if (attrs[DRFCC_ATTR_RATE_REDUC_POLICY])
                if (parse_policy(attrs[DRFCC_ATTR_RATE_REDUC_POLICY],
                                 dtcp_rate_reduction(cfg)))
                        return -1;

        return 0;
}

static int parse_dtcp_fctrl_config(struct nlattr * attr,
                                   struct dtcp_config * cfg)
{
        struct nla_policy attr_policy[DFCC_ATTR_MAX + 1];
        struct nlattr * attrs[DFCC_ATTR_MAX + 1];

        attr_policy[DFCC_ATTR_WINDOW_BASED].type             = NLA_FLAG;
        attr_policy[DFCC_ATTR_WINDOW_BASED].len              = 0;
        attr_policy[DFCC_ATTR_WINDOW_BASED_CONFIG].type      = NLA_NESTED;
        attr_policy[DFCC_ATTR_WINDOW_BASED_CONFIG].len       = 0;
        attr_policy[DFCC_ATTR_RATE_BASED].type               = NLA_FLAG;
        attr_policy[DFCC_ATTR_RATE_BASED].len                = 0;
        attr_policy[DFCC_ATTR_RATE_BASED_CONFIG].type        = NLA_NESTED;
        attr_policy[DFCC_ATTR_RATE_BASED_CONFIG].len         = 0;
        attr_policy[DFCC_ATTR_SBYTES_THRES].type             = NLA_U32;
        attr_policy[DFCC_ATTR_SBYTES_THRES].len              = 4;
        attr_policy[DFCC_ATTR_SBYTES_PER_THRES].type         = NLA_U32;
        attr_policy[DFCC_ATTR_SBYTES_PER_THRES].len          = 4;
        attr_policy[DFCC_ATTR_SBUFFER_THRES].type            = NLA_U32;
        attr_policy[DFCC_ATTR_SBUFFER_THRES].len             = 4;
        attr_policy[DFCC_ATTR_RBYTES_THRES].type             = NLA_U32;
        attr_policy[DFCC_ATTR_RBYTES_THRES].len              = 4;
        attr_policy[DFCC_ATTR_RBYTES_PER_THRES].type         = NLA_U32;
        attr_policy[DFCC_ATTR_RBYTES_PER_THRES].len          = 4;
        attr_policy[DFCC_ATTR_RBUFFER_THRES].type            = NLA_U32;
        attr_policy[DFCC_ATTR_RBUFFER_THRES].len             = 4;
        attr_policy[DFCC_ATTR_CLOSED_WINDOW_POLICY].type     = NLA_NESTED;
        attr_policy[DFCC_ATTR_CLOSED_WINDOW_POLICY].len      = 0;
        attr_policy[DFCC_ATTR_RECON_FLOW_CTRL_POLICY].type   = NLA_NESTED;
        attr_policy[DFCC_ATTR_RECON_FLOW_CTRL_POLICY].len    = 0;
        attr_policy[DFCC_ATTR_RCVING_FLOW_CTRL_POLICY].type  = NLA_NESTED;
        attr_policy[DFCC_ATTR_RCVING_FLOW_CTRL_POLICY].len   = 0;

        if (nla_parse_nested(attrs,
                             DFCC_ATTR_MAX,
                             attr, attr_policy))
                return -1;

        dtcp_window_based_fctrl_set(cfg, nla_get_flag(attrs[DFCC_ATTR_WINDOW_BASED]));

        if (attrs[DFCC_ATTR_WINDOW_BASED_CONFIG]) {
                if (dtcp_wfctrl_cfg_set(cfg, dtcp_window_fctrl_config_create()))
                        return -1;
                parse_dtcp_wb_fctrl_config(attrs[DFCC_ATTR_WINDOW_BASED_CONFIG],
                                           cfg);
        }

        dtcp_rate_based_fctrl_set(cfg, nla_get_flag(attrs[DFCC_ATTR_RATE_BASED]));

        if (attrs[DFCC_ATTR_RATE_BASED_CONFIG]) {
                if (dtcp_rfctrl_cfg_set(cfg, dtcp_rate_fctrl_config_create()))
                        return -1;
                parse_dtcp_rb_fctrl_config(attrs[DFCC_ATTR_RATE_BASED_CONFIG],
                                           cfg);
        }

        if (attrs[DFCC_ATTR_SBYTES_THRES])
                dtcp_sent_bytes_th_set(cfg,
                                       nla_get_u32(attrs[DFCC_ATTR_SBYTES_THRES]));

        if (attrs[DFCC_ATTR_SBYTES_PER_THRES])
                dtcp_sent_bytes_percent_th_set(cfg,
                                               nla_get_u32(attrs[DFCC_ATTR_SBYTES_PER_THRES]));

        if (attrs[DFCC_ATTR_SBUFFER_THRES])
                dtcp_sent_buffers_th_set(cfg,
                                         nla_get_u32(attrs[DFCC_ATTR_SBUFFER_THRES]));

        if (attrs[DFCC_ATTR_RBYTES_THRES])
                dtcp_rcvd_bytes_th_set(cfg,
                                       nla_get_u32(attrs[DFCC_ATTR_RBYTES_THRES]));

        if (attrs[DFCC_ATTR_RBYTES_PER_THRES])
                dtcp_rcvd_bytes_percent_th_set(cfg,
                                               nla_get_u32(attrs[DFCC_ATTR_RBYTES_PER_THRES]));

        if (attrs[DFCC_ATTR_RBUFFER_THRES])
                dtcp_rcvd_buffers_th_set(cfg,
                                         nla_get_u32(attrs[DFCC_ATTR_RBUFFER_THRES]));

        if (attrs[DFCC_ATTR_CLOSED_WINDOW_POLICY])
                if (parse_policy(attrs[DFCC_ATTR_CLOSED_WINDOW_POLICY],
                                 dtcp_closed_window(cfg)))
                        return -1;

        if (attrs[DFCC_ATTR_RECON_FLOW_CTRL_POLICY])
                if (parse_policy(attrs[DFCC_ATTR_RECON_FLOW_CTRL_POLICY],
                                 dtcp_reconcile_flow_conflict(cfg)))
                        return -1;

        if (attrs[DFCC_ATTR_RCVING_FLOW_CTRL_POLICY])
                if (parse_policy(attrs[DFCC_ATTR_RCVING_FLOW_CTRL_POLICY],
                                 dtcp_receiving_flow_control(cfg)))
                        return -1;

        return 0;
}

static int parse_dtcp_rctrl_config(struct nlattr * attr,
                                   struct dtcp_config * cfg)
{
        struct nla_policy attr_policy[DRCC_ATTR_MAX + 1];
        struct nlattr * attrs[DRCC_ATTR_MAX + 1];

        attr_policy[DRCC_ATTR_MAX_TIME_TO_RETRY].type   = NLA_U32;
        attr_policy[DRCC_ATTR_MAX_TIME_TO_RETRY].len    = 4;
        attr_policy[DRCC_ATTR_DATA_RXMSN_MAX].type      = NLA_U32;
        attr_policy[DRCC_ATTR_DATA_RXMSN_MAX].len       = 4;
        attr_policy[DRCC_ATTR_INIT_TR].type             = NLA_U32;
        attr_policy[DRCC_ATTR_INIT_TR].len              = 4;
        attr_policy[DRCC_ATTR_RTX_TIME_EXP_POLICY].type = NLA_NESTED;
        attr_policy[DRCC_ATTR_RTX_TIME_EXP_POLICY].len  = 0;
        attr_policy[DRCC_ATTR_SACK_POLICY].type         = NLA_NESTED;
        attr_policy[DRCC_ATTR_SACK_POLICY].len          = 0;
        attr_policy[DRCC_ATTR_RACK_LIST_POLICY].type    = NLA_NESTED;
        attr_policy[DRCC_ATTR_RACK_LIST_POLICY].len     = 0;
        attr_policy[DRCC_ATTR_RACK_POLICY].type         = NLA_NESTED;
        attr_policy[DRCC_ATTR_RACK_POLICY].len          = 0;
        attr_policy[DRCC_ATTR_SDING_ACK_POLICY].type    = NLA_NESTED;
        attr_policy[DRCC_ATTR_SDING_ACK_POLICY].len     = 0;
        attr_policy[DRCC_ATTR_RCONTROL_ACK_POLICY].type = NLA_NESTED;
        attr_policy[DRCC_ATTR_RCONTROL_ACK_POLICY].len  = 0;

        if (nla_parse_nested(attrs,
                             DRCC_ATTR_MAX,
                             attr, attr_policy))
                return -1;

        if (attrs[DRCC_ATTR_MAX_TIME_TO_RETRY])
                dtcp_max_time_retry_set(cfg,
                                        nla_get_u32(attrs[DRCC_ATTR_DATA_RXMSN_MAX]));

        if (attrs[DRCC_ATTR_DATA_RXMSN_MAX])
                dtcp_data_retransmit_max_set(cfg,
                                             nla_get_u32(attrs[DRCC_ATTR_DATA_RXMSN_MAX]));

        if (attrs[DRCC_ATTR_INIT_TR])
                dtcp_initial_tr_set(cfg,
                                    nla_get_u32(attrs[DRCC_ATTR_INIT_TR]));

        if (attrs[DRCC_ATTR_RTX_TIME_EXP_POLICY])
                if (parse_policy(attrs[DRCC_ATTR_RTX_TIME_EXP_POLICY],
                                 dtcp_retransmission_timer_expiry(cfg)))
                        return -1;

        if (attrs[DRCC_ATTR_SACK_POLICY])
                if (parse_policy(attrs[DRCC_ATTR_SACK_POLICY],
                                 dtcp_sender_ack(cfg)))
                        return -1;

        if (attrs[DRCC_ATTR_RACK_LIST_POLICY])
                if (parse_policy(attrs[DRCC_ATTR_RACK_LIST_POLICY],
                                 dtcp_receiving_ack_list(cfg)))
                        return -1;

        if (attrs[DRCC_ATTR_RACK_POLICY])
                if (parse_policy(attrs[DRCC_ATTR_RACK_POLICY],
                                 dtcp_rcvr_ack(cfg)))
                        return -1;

        if (attrs[DRCC_ATTR_SDING_ACK_POLICY])
                if (parse_policy(attrs[DRCC_ATTR_SDING_ACK_POLICY],
                                 dtcp_sending_ack(cfg)))
                        return -1;

        if (attrs[DRCC_ATTR_RCONTROL_ACK_POLICY])
                if (parse_policy(attrs[DRCC_ATTR_RCONTROL_ACK_POLICY],
                                 dtcp_rcvr_control_ack(cfg)))
                        return -1;

        return 0;
}

static int parse_dtcp_config(struct nlattr * attr, struct dtcp_config * cfg)
{
        struct nla_policy attr_policy[DCA_ATTR_MAX + 1];
        struct nlattr * attrs[DCA_ATTR_MAX + 1];

        attr_policy[DCA_ATTR_FLOW_CONTROL].type            = NLA_FLAG;
        attr_policy[DCA_ATTR_FLOW_CONTROL].len             = 0;
        attr_policy[DCA_ATTR_FLOW_CONTROL_CONFIG].type     = NLA_NESTED;
        attr_policy[DCA_ATTR_FLOW_CONTROL_CONFIG].len      = 0;
        attr_policy[DCA_ATTR_RETX_CONTROL].type            = NLA_FLAG;
        attr_policy[DCA_ATTR_RETX_CONTROL].len             = 0;
        attr_policy[DCA_ATTR_RETX_CONTROL_CONFIG].type     = NLA_NESTED;
        attr_policy[DCA_ATTR_RETX_CONTROL_CONFIG].len      = 0;
        attr_policy[DCA_ATTR_DTCP_POLICY_SET].type         = NLA_NESTED;
        attr_policy[DCA_ATTR_DTCP_POLICY_SET].len          = 0;
        attr_policy[DCA_ATTR_LOST_CONTROL_PDU_POLICY].type = NLA_NESTED;
        attr_policy[DCA_ATTR_LOST_CONTROL_PDU_POLICY].len  = 0;
        attr_policy[DCA_ATTR_RTT_EST_POLICY].type          = NLA_NESTED;
        attr_policy[DCA_ATTR_RTT_EST_POLICY].len           = 0;

        if (!attr || !cfg) {
                LOG_ERR("Bogus input to parse dtcp_config");
                return -1;
        }

        if (nla_parse_nested(attrs,
                             DCA_ATTR_MAX,
                             attr, attr_policy)) {
                LOG_ERR("Could not parse nested in parse_dtcp_config");
                return -1;
        }

        dtcp_flow_ctrl_set(cfg, nla_get_flag(attrs[DCA_ATTR_FLOW_CONTROL]));

        if (attrs[DCA_ATTR_FLOW_CONTROL_CONFIG]) {
                if (dtcp_fctrl_cfg_set(cfg, dtcp_fctrl_config_create()))
                        return -1;
                parse_dtcp_fctrl_config(attrs[DCA_ATTR_FLOW_CONTROL_CONFIG],
                                        cfg);
        }

        if (attrs[DCA_ATTR_RETX_CONTROL]) {
                if (dtcp_rxctrl_cfg_set(cfg, dtcp_rxctrl_config_create()))
                        return -1;
                dtcp_rtx_ctrl_set(cfg, nla_get_flag(attrs[DCA_ATTR_RETX_CONTROL]));
        }

        if (attrs[DCA_ATTR_RETX_CONTROL_CONFIG])
                if (parse_dtcp_rctrl_config(attrs[DCA_ATTR_RETX_CONTROL_CONFIG],
                                            cfg))
                        return -1;

        if (attrs[DCA_ATTR_DTCP_POLICY_SET])
                if (parse_policy(attrs[DCA_ATTR_DTCP_POLICY_SET],
                                 dtcp_ps(cfg)))
                        return -1;


        if (attrs[DCA_ATTR_LOST_CONTROL_PDU_POLICY])
                if (parse_policy(attrs[DCA_ATTR_LOST_CONTROL_PDU_POLICY],
                                 dtcp_lost_control_pdu(cfg)))
                        return -1;

        if (attrs[DCA_ATTR_RTT_EST_POLICY])
                if (parse_policy(attrs[DCA_ATTR_RTT_EST_POLICY],
                                 dtcp_rtt_estimator(cfg)))
                        return -1;

        return 0;
}

static int parse_dtp_config(struct nlattr *     attr,
                            struct dtp_config * cfg)
{
        struct nla_policy attr_policy[DTPCA_ATTR_MAX + 1];
        struct nlattr * attrs[DTPCA_ATTR_MAX + 1];

        attr_policy[DTPCA_ATTR_DTCP_PRESENT].type           = NLA_FLAG;
        attr_policy[DTPCA_ATTR_DTCP_PRESENT].len            = 0;
        attr_policy[DTPCA_ATTR_SEQ_NUM_ROLLOVER].type       = NLA_U32;
        attr_policy[DTPCA_ATTR_SEQ_NUM_ROLLOVER].len        = 4;
        attr_policy[DTPCA_ATTR_INIT_A_TIMER].type           = NLA_U32;
        attr_policy[DTPCA_ATTR_INIT_A_TIMER].len            = 4;
        attr_policy[DTPCA_ATTR_PARTIAL_DELIVERY].type       = NLA_FLAG;
        attr_policy[DTPCA_ATTR_PARTIAL_DELIVERY].len        = 0;
        attr_policy[DTPCA_ATTR_INCOMPLETE_DELIVERY].type    = NLA_FLAG;
        attr_policy[DTPCA_ATTR_INCOMPLETE_DELIVERY].len     = 0;
        attr_policy[DTPCA_ATTR_IN_ORDER_DELIVERY].type      = NLA_FLAG;
        attr_policy[DTPCA_ATTR_IN_ORDER_DELIVERY].len       = 0;
        attr_policy[DTPCA_ATTR_MAX_SDU_GAP].type            = NLA_U32;
        attr_policy[DTPCA_ATTR_MAX_SDU_GAP].len             = 4;
        attr_policy[DTPCA_ATTR_DTP_POLICY_SET].type         = NLA_NESTED;
        attr_policy[DTPCA_ATTR_DTP_POLICY_SET].len          = 0;

        if (nla_parse_nested(attrs,
                             DTPCA_ATTR_MAX,
                             attr, attr_policy) < 0)
                return -1;

        if (attrs[DTPCA_ATTR_DTCP_PRESENT])
                dtp_conf_dtcp_present_set(cfg,
                        nla_get_flag(attrs[DTPCA_ATTR_DTCP_PRESENT]));

        if (attrs[DTPCA_ATTR_SEQ_NUM_ROLLOVER])
                dtp_conf_seq_num_ro_th_set(cfg,
                        nla_get_u32(attrs[DTPCA_ATTR_SEQ_NUM_ROLLOVER]));

        if (attrs[DTPCA_ATTR_INIT_A_TIMER])
                dtp_conf_initial_a_timer_set(cfg,
                        nla_get_u32(attrs[DTPCA_ATTR_INIT_A_TIMER]));

        if (attrs[DTPCA_ATTR_PARTIAL_DELIVERY])
                dtp_conf_partial_del_set(cfg,
                        nla_get_flag(attrs[DTPCA_ATTR_PARTIAL_DELIVERY]));

        if (attrs[DTPCA_ATTR_INCOMPLETE_DELIVERY])
                dtp_conf_incomplete_del_set(cfg,
                        nla_get_flag(attrs[DTPCA_ATTR_INCOMPLETE_DELIVERY]));

        if (attrs[DTPCA_ATTR_IN_ORDER_DELIVERY])
                dtp_conf_in_order_del_set(cfg,
                        nla_get_flag(attrs[DTPCA_ATTR_IN_ORDER_DELIVERY]));

        if (attrs[DTPCA_ATTR_MAX_SDU_GAP])
                dtp_conf_max_sdu_gap_set(cfg,
                        nla_get_u32(attrs[DTPCA_ATTR_MAX_SDU_GAP]));

        if (attrs[DTPCA_ATTR_DTP_POLICY_SET]) {
                if (parse_policy(attrs[DTPCA_ATTR_DTP_POLICY_SET],
                                 dtp_conf_ps_get(cfg)))
                        return -1;
        }
        return 0;
}

static int
rnl_parse_ipcm_assign_to_dif_req_msg(struct genl_info * info,
                                     struct rnl_ipcm_assign_to_dif_req_msg_attrs * msg_attrs)
{
        if (parse_dif_info(info->attrs[IATDR_ATTR_DIF_INFORMATION],
                           msg_attrs->dif_info)) {
                LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_"
                                                "IPCM_ASSIGN_TO_DIF_REQUEST"));
                return -1;
        }

        return 0;
}

static int rnl_parse_ipcm_update_dif_config_req_msg
(struct genl_info * info,
 struct rnl_ipcm_update_dif_config_req_msg_attrs * msg_attrs)
{

        if (parse_dif_config(info->attrs[IUDCR_ATTR_DIF_CONFIGURATION],
                             msg_attrs->dif_config)) {
                LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_UPDATE_DIF_"
                                                "CONFIG_REQUEST"));
                return -1;
        }

        return 0;
}

static int
rnl_parse_ipcm_ipcp_dif_reg_noti_msg(struct genl_info * info,
                                     struct rnl_ipcm_ipcp_dif_reg_noti_msg_attrs * msg_attrs)
{
        if (info->attrs[IDRN_ATTR_IPC_PROCESS_NAME])
                if (parse_app_name_info(info->attrs[IDRN_ATTR_IPC_PROCESS_NAME],
                                        msg_attrs->ipcp_name))
                        goto parse_fail;
        if (info->attrs[IDRN_ATTR_DIF_NAME])
                if (parse_app_name_info(info->attrs[IDRN_ATTR_DIF_NAME],
                                        msg_attrs->dif_name))
                        goto parse_fail;

        msg_attrs->is_registered =
                nla_get_flag(info->attrs[IDRN_ATTR_REGISTRATION]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_IPC_"
                                        "PROCESS_DIF_REGISTRATION_NOTIF"));
        return -1;
}

static int
rnl_parse_ipcm_ipcp_dif_unreg_noti_msg(struct genl_info * info,
                                       struct rnl_ipcm_ipcp_dif_unreg_noti_msg_attrs * msg_attrs)
{

        if (info->attrs[IDUN_ATTR_RESULT])
                msg_attrs->result = nla_get_u32(info->attrs[IDUN_ATTR_RESULT]);
        return 0;
}

static int
rnl_parse_ipcm_alloc_flow_req_msg(struct genl_info * info,
                                  struct rnl_ipcm_alloc_flow_req_msg_attrs * msg_attrs)
{
        if (parse_app_name_info(info->attrs[IAFRM_ATTR_SOURCE_APP_NAME],
                                msg_attrs->source)                       ||
            parse_app_name_info(info->attrs[IAFRM_ATTR_DEST_APP_NAME],
                                msg_attrs->dest)                         ||
            parse_flow_spec(info->attrs[IAFRM_ATTR_FLOW_SPEC],
                            msg_attrs->fspec)                            ||
            parse_app_name_info(info->attrs[IAFRM_ATTR_DIF_NAME],
                                msg_attrs->dif_name)) {
                LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_"
                                                "ALLOCATE_FLOW_REQUEST"));
                return -1;
        }

        return 0;
}

static int
rnl_parse_ipcm_alloc_flow_req_arrived_msg(struct genl_info * info,
                                          struct rnl_ipcm_alloc_flow_req_arrived_msg_attrs * msg_attrs)
{
        if (parse_app_name_info(info->attrs[IAFRA_ATTR_SOURCE_APP_NAME],
                                msg_attrs->source)                      ||
            parse_app_name_info(info->attrs[IAFRA_ATTR_DEST_APP_NAME],
                                msg_attrs->dest)                        ||
            parse_app_name_info(info->attrs[IAFRA_ATTR_DIF_NAME],
                                msg_attrs->dif_name)                    ||
            parse_flow_spec(info->attrs[IAFRA_ATTR_FLOW_SPEC],
                            msg_attrs->fspec)) {
                LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_ALLOCATE_"
                                                "FLOW_REQUEST_ARRIVED"));
                return -1;
        }

        if (info->attrs[IAFRA_ATTR_PORT_ID])
                msg_attrs->id = nla_get_u32(info->attrs[IAFRA_ATTR_PORT_ID]);

        return 0;
}

static int
rnl_parse_ipcm_alloc_flow_resp_msg(struct genl_info * info,
                                   struct rnl_alloc_flow_resp_msg_attrs * msg_attrs)
{
        if (info->attrs[IAFRE_ATTR_RESULT])
                msg_attrs->result =
                        nla_get_u32(info->attrs[IAFRE_ATTR_RESULT]);

        msg_attrs->notify_src =
                nla_get_flag(info->attrs[IAFRE_ATTR_NOTIFY_SOURCE]);

        return 0;
}

static int
rnl_parse_ipcm_dealloc_flow_req_msg(struct genl_info * info,
                                    struct rnl_ipcm_dealloc_flow_req_msg_attrs * msg_attrs)
{
        if (info->attrs[IDFRT_ATTR_PORT_ID])
                msg_attrs->id =
                        nla_get_u32(info->attrs[IDFRT_ATTR_PORT_ID]);

        return 0;
}

static int rnl_parse_ipcm_flow_dealloc_noti_msg(struct genl_info * info,
                                                struct rnl_ipcm_flow_dealloc_noti_msg_attrs * msg_attrs)
{
        if (info->attrs[IFDN_ATTR_PORT_ID])
                msg_attrs->id = nla_get_u32(info->attrs[IFDN_ATTR_PORT_ID]);
        if (info->attrs[IFDN_ATTR_CODE])
                msg_attrs->code = nla_get_u32(info->attrs[IFDN_ATTR_CODE]);

        return 0;
}

static int rnl_parse_ipcm_conn_create_req_msg(struct genl_info * info,
                                              struct rnl_ipcp_conn_create_req_msg_attrs * msg_attrs)
{
        if (info->attrs[ICCRQ_ATTR_PORT_ID])
                msg_attrs->port_id  =
                        nla_get_u32(info->attrs[ICCRQ_ATTR_PORT_ID]);
        if (info->attrs[ICCRQ_ATTR_SOURCE_ADDR])
                msg_attrs->src_addr =
                        nla_get_u32(info->attrs[ICCRQ_ATTR_SOURCE_ADDR]);
        if (info->attrs[ICCRQ_ATTR_DEST_ADDR])
                msg_attrs->dst_addr =
                        nla_get_u32(info->attrs[ICCRQ_ATTR_DEST_ADDR]);
        if (info->attrs[ICCRQ_ATTR_QOS_ID])
                msg_attrs->qos_id   =
                        nla_get_u32(info->attrs[ICCRQ_ATTR_QOS_ID]);
        if (info->attrs[ICCRQ_ATTR_DTP_CONFIG]) {
                if (parse_dtp_config(info->attrs
                                     [ICCRQ_ATTR_DTP_CONFIG],
                                     msg_attrs->dtp_cfg)) {
                        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_CONNECTION"
                                                        "_CREATE_REQUEST"));
                        return -1;
                }
        }
        if (info->attrs[ICCRQ_ATTR_DTCP_CONFIG]) {
                if (parse_dtcp_config(info->attrs
                                     [ICCRQ_ATTR_DTCP_CONFIG],
                                     msg_attrs->dtcp_cfg)) {
                        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_CONNECTION"
                                                        "_CREATE_REQUEST"));
                        return -1;
                }
        }

        return 0;
}

static int
rnl_parse_ipcm_conn_create_arrived_msg(struct genl_info * info,
                                       struct rnl_ipcp_conn_create_arrived_msg_attrs * msg_attrs)
{
        if (info->attrs[ICCA_ATTR_PORT_ID])
                msg_attrs->port_id =
                        nla_get_u32(info->attrs[ICCA_ATTR_PORT_ID]);
        if (info->attrs[ICCA_ATTR_SOURCE_ADDR])
                msg_attrs->src_addr =
                        nla_get_u32(info->attrs[ICCA_ATTR_SOURCE_ADDR]);
        if (info->attrs[ICCA_ATTR_DEST_ADDR])
                msg_attrs->dst_addr =
                        nla_get_u32(info->attrs[ICCA_ATTR_DEST_ADDR]);
        if (info->attrs[ICCA_ATTR_DEST_CEP_ID])
                msg_attrs->dst_cep =
                        nla_get_u32(info->attrs[ICCA_ATTR_DEST_CEP_ID]);
        if (info->attrs[ICCA_ATTR_QOS_ID])
                msg_attrs->qos_id =
                        nla_get_u32(info->attrs[ICCA_ATTR_QOS_ID]);
        if (info->attrs[ICCA_ATTR_FLOW_USER_IPCP_ID])
                msg_attrs->flow_user_ipc_process_id =
                        nla_get_u16(info->attrs[ICCA_ATTR_FLOW_USER_IPCP_ID]);
        if (info->attrs[ICCA_ATTR_DTP_CONFIG]) {
                if (parse_dtp_config(info->attrs
                                     [ICCA_ATTR_DTP_CONFIG],
                                     msg_attrs->dtp_cfg)) {
                        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_CONNECTION"
                                                        "_CREATE_ARRIVED"));
                        return -1;
                }
        }
        if (info->attrs[ICCA_ATTR_DTCP_CONFIG]) {
                if (parse_dtcp_config(info->attrs
                                     [ICCA_ATTR_DTCP_CONFIG],
                                     msg_attrs->dtcp_cfg)) {
                        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_CONNECTION"
                                                        "_CREATE_ARRIVED"));
                        return -1;
                }
        }

        return 0;
}

static int
rnl_parse_ipcm_conn_update_req_msg(struct genl_info * info,
                                   struct rnl_ipcp_conn_update_req_msg_attrs * msg_attrs)
{
        if (info->attrs[ICURQ_ATTR_PORT_ID])
                msg_attrs->port_id =
                        nla_get_u32(info->attrs[ICURQ_ATTR_PORT_ID]);
        if (info->attrs[ICURQ_ATTR_SOURCE_CEP_ID])
                msg_attrs->src_cep =
                        nla_get_u32(info->attrs[ICURQ_ATTR_SOURCE_CEP_ID]);
        if (info->attrs[ICURQ_ATTR_DEST_CEP_ID])
                msg_attrs->dst_cep =
                        nla_get_u32(info->attrs[ICURQ_ATTR_DEST_CEP_ID]);
        if (info->attrs[ICURQ_ATTR_FLOW_USER_IPCP_ID])
                msg_attrs->flow_user_ipc_process_id =
                        nla_get_u16(info->attrs[ICURQ_ATTR_FLOW_USER_IPCP_ID]);
        return 0;
}

static int
rnl_parse_ipcm_conn_destroy_req_msg(struct genl_info * info,
                                    struct rnl_ipcp_conn_destroy_req_msg_attrs * msg_attrs)
{
        if (info->attrs[ICDR_ATTR_PORT_ID])
                msg_attrs->port_id =
                        nla_get_u32(info->attrs[ICDR_ATTR_PORT_ID]);
        if (info->attrs[ICDR_ATTR_SOURCE_CEP_ID])
                msg_attrs->src_cep =
                        nla_get_u32(info->attrs[ICDR_ATTR_SOURCE_CEP_ID]);
        return 0;
}

static int
rnl_parse_ipcm_reg_app_req_msg(struct genl_info * info,
                               struct rnl_ipcm_reg_app_req_msg_attrs * msg_attrs)
{
        if (parse_app_name_info(info->attrs[IRAR_ATTR_APP_NAME],
                                msg_attrs->app_name)             ||
            parse_app_name_info(info->attrs[IRAR_ATTR_DIF_NAME],
                                msg_attrs->dif_name)) {
                LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_REGISTER_"
                                                "APPLICATION_REQUEST"));
                return -1;
        }

        return 0;
}

static int rnl_parse_ipcm_unreg_app_req_msg(struct genl_info * info,
                                            struct rnl_ipcm_unreg_app_req_msg_attrs * msg_attrs)
{
        if (parse_app_name_info(info->attrs[IUAR_ATTR_APP_NAME],
                                msg_attrs->app_name)             ||
            parse_app_name_info(info->attrs[IUAR_ATTR_DIF_NAME],
                                msg_attrs->dif_name)) {
                LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_UNREGISTER_"
                                                "APPLICATION_REQUEST"));
                return -1;
        }
        return 0;
}

static int
rnl_parse_ipcm_query_rib_req_msg(struct genl_info * info,
                                 struct rnl_ipcm_query_rib_msg_attrs * msg_attrs)
{
    	if (info->attrs[IDQR_ATTR_OBJECT_CLASS])
    		msg_attrs->object_class = nla_dup_string(info->attrs[IDQR_ATTR_OBJECT_CLASS],
            						 GFP_KERNEL);

    	if (info->attrs[IDQR_ATTR_OBJECT_NAME])
    	        msg_attrs->object_name = nla_dup_string(info->attrs[IDQR_ATTR_OBJECT_NAME],
    	                                             	GFP_KERNEL);

        if (info->attrs[IDQR_ATTR_OBJECT_INSTANCE])
                msg_attrs->object_instance =
                        nla_get_u64(info->attrs[IDQR_ATTR_OBJECT_INSTANCE]);

        if (info->attrs[IDQR_ATTR_SCOPE])
                msg_attrs->scope =
                        nla_get_u32(info->attrs[IDQR_ATTR_SCOPE]);

        if (info->attrs[IDQR_ATTR_FILTER])
        	msg_attrs->filter = nla_dup_string(info->attrs[IDQR_ATTR_FILTER],
            	                                   GFP_KERNEL);

        return 0;
}

static int parse_list_pffe_conf_e(struct nlattr *     nested_attr,
                                  struct rnl_rmt_mod_pffe_msg_attrs * msg)
{
        struct nlattr *        nla;
        struct mod_pff_entry * entry;
        int                    rem                   = 0;
        int                    entries_with_problems = 0;
        int                    total_entries         = 0;

        if (!nested_attr) {
                LOG_ERR("Bogus attribute passed, bailing out");
                return -1;
        }

        if (!msg) {
                LOG_ERR("Bogus msg passed, bailing out");
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
                INIT_LIST_HEAD(&entry->next);

                if (parse_pdu_fte_list_entry(nla, entry) < 0) {
                        rkfree(entry);
                        entries_with_problems++;
                        continue;
                }

                list_add_tail(&entry->next, &msg->pff_entries);
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

static int
rnl_parse_rmt_modify_fte_req_msg(struct genl_info * info,
                                 struct rnl_rmt_mod_pffe_msg_attrs * msg_attrs)
{
        if (info->attrs[RMPFE_ATTR_ENTRIES]) {
                if (parse_list_pffe_conf_e(info->attrs[RMPFE_ATTR_ENTRIES],
                                           msg_attrs))
                        goto parse_fail;
        }

        if (info->attrs[RMPFE_ATTR_MODE])
                msg_attrs->mode =
                        nla_get_u32(info->attrs[RMPFE_ATTR_MODE]);
        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_RMT_MODIFY_FTE_REQ_MSG"));
        return -1;
}

static int
rnl_parse_ipcp_set_policy_set_param_req_msg(
                struct genl_info * info,
                struct rnl_ipcp_set_policy_set_param_req_msg_attrs * msg_attrs)
{
        if (info->attrs[ISPSP_ATTR_PATH])
                msg_attrs->path = nla_dup_string(info->attrs[ISPSP_ATTR_PATH],
                                                 GFP_KERNEL);

        if (info->attrs[ISPSP_ATTR_NAME])
                msg_attrs->name = nla_dup_string(info->attrs[ISPSP_ATTR_NAME],
                                                 GFP_KERNEL);

        if (info->attrs[ISPSP_ATTR_VALUE])
                msg_attrs->value = nla_dup_string(info->attrs[ISPSP_ATTR_VALUE],
                                                  GFP_KERNEL);

        return 0;
}

static int
rnl_parse_ipcp_select_policy_set_req_msg(
                struct genl_info * info,
                struct rnl_ipcp_select_policy_set_req_msg_attrs * msg_attrs)
{
        if (info->attrs[ISPS_ATTR_PATH])
                msg_attrs->path = nla_dup_string(info->attrs[ISPS_ATTR_PATH],
                                                 GFP_KERNEL);

        if (info->attrs[ISPS_ATTR_NAME])
                msg_attrs->name = nla_dup_string(info->attrs[ISPS_ATTR_NAME],
                                                 GFP_KERNEL);

        return 0;
}

static int
rnl_parse_crypto_state_info(
		struct nlattr * crypto_state_attr,
                struct sdup_crypto_state * state)
{
        struct nla_policy attr_policy[ICSTATE_ATTR_MAX + 1];
        struct nlattr *   attrs[ICSTATE_ATTR_MAX + 1];

        if (!crypto_state_attr || !state) {
                LOG_ERR("Bogus input parameters, cannot parse name app info");
                return -1;
        }

        attr_policy[ICSTATE_ENABLE_CRYPTO_TX].type = NLA_FLAG;
        attr_policy[ICSTATE_ENABLE_CRYPTO_TX].len  = 0;
        attr_policy[ICSTATE_ENABLE_CRYPTO_RX].type = NLA_FLAG;
        attr_policy[ICSTATE_ENABLE_CRYPTO_RX].len  = 0;
        attr_policy[ICSTATE_MAC_KEY_TX].type = NLA_UNSPEC;
        attr_policy[ICSTATE_MAC_KEY_TX].len = 0;
        attr_policy[ICSTATE_MAC_KEY_RX].type = NLA_UNSPEC;
        attr_policy[ICSTATE_MAC_KEY_RX].len = 0;
        attr_policy[ICSTATE_ENCRYPT_KEY_TX].type = NLA_UNSPEC;
        attr_policy[ICSTATE_ENCRYPT_KEY_TX].len = 0;
        attr_policy[ICSTATE_ENCRYPT_KEY_RX].type = NLA_UNSPEC;
        attr_policy[ICSTATE_ENCRYPT_KEY_RX].len = 0;
        attr_policy[ICSTATE_IV_TX].type = NLA_UNSPEC;
        attr_policy[ICSTATE_IV_TX].len = 0;
        attr_policy[ICSTATE_IV_RX].type = NLA_UNSPEC;
        attr_policy[ICSTATE_IV_RX].len = 0;

        if (nla_parse_nested(attrs,
        		     ICSTATE_ATTR_MAX,
        		     crypto_state_attr,
        		     attr_policy) < 0)
                return -1;

	state->enable_crypto_tx = attrs[ICSTATE_ENABLE_CRYPTO_TX];
	state->enable_crypto_rx = attrs[ICSTATE_ENABLE_CRYPTO_RX];

        if (attrs[ICSTATE_MAC_KEY_TX])
        	state->mac_key_tx = buffer_create_from(nla_data(attrs[ICSTATE_MAC_KEY_TX]),
        					       nla_len(attrs[ICSTATE_MAC_KEY_TX]));

        if (attrs[ICSTATE_MAC_KEY_RX])
        	state->mac_key_rx = buffer_create_from(nla_data(attrs[ICSTATE_MAC_KEY_RX]),
        					       nla_len(attrs[ICSTATE_MAC_KEY_RX]));

        if (attrs[ICSTATE_ENCRYPT_KEY_TX])
        	state->encrypt_key_tx = buffer_create_from(nla_data(attrs[ICSTATE_ENCRYPT_KEY_TX]),
        					           nla_len(attrs[ICSTATE_ENCRYPT_KEY_TX]));

        if (attrs[ICSTATE_ENCRYPT_KEY_RX])
        	state->encrypt_key_rx = buffer_create_from(nla_data(attrs[ICSTATE_ENCRYPT_KEY_RX]),
        					           nla_len(attrs[ICSTATE_ENCRYPT_KEY_RX]));

        if (attrs[ICSTATE_IV_TX])
        	state->iv_tx = buffer_create_from(nla_data(attrs[ICSTATE_IV_TX]),
        					  nla_len(attrs[ICSTATE_IV_TX]));

        if (attrs[ICSTATE_IV_RX])
        	state->iv_rx = buffer_create_from(nla_data(attrs[ICSTATE_IV_RX]),
        					  nla_len(attrs[ICSTATE_IV_RX]));

        return 0;
}

static struct sdup_crypto_state * sdup_crypto_state_create(void)
{
	struct sdup_crypto_state * tmp;

	tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp) {
		return NULL;
	}

	tmp->enable_crypto_tx = false;
	tmp->enable_crypto_rx = false;
	tmp->mac_key_tx = NULL;
	tmp->mac_key_rx = NULL;
	tmp->encrypt_key_tx = NULL;
	tmp->encrypt_key_rx = NULL;
	tmp->iv_tx = NULL;
	tmp->iv_rx = NULL;

	return tmp;
}

static int
rnl_parse_ipcp_update_crypto_state_req_msg(
                struct genl_info * info,
                struct rnl_ipcp_update_crypto_state_req_msg_attrs * msg_attrs)
{
        if (info->attrs[IUCSR_ATTR_N_1_PORT])
                msg_attrs->port_id =
                        nla_get_u32(info->attrs[IUCSR_ATTR_N_1_PORT]);

        if (info->attrs[IUCSR_ATTR_CRYPT_STATE]) {
        	msg_attrs->state = sdup_crypto_state_create();
        	if (!msg_attrs->state)
        		return -1;
        	rnl_parse_crypto_state_info(info->attrs[IUCSR_ATTR_CRYPT_STATE],
        				    msg_attrs->state);

        }

        return 0;
}

int rnl_parse_msg(struct genl_info * info,
                  struct rnl_msg *   msg)
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
        msg->header = *((struct rina_msg_hdr *) info->userhdr);

        LOG_DBG("msg at %pK / msg->attrs at %pK",  msg, msg->attrs);
        LOG_DBG("  src-ipc-id: %d", msg->header.src_ipc_id);
        LOG_DBG("  dst-ipc-id: %d", msg->header.dst_ipc_id);

        switch(info->genlhdr->cmd) {
        case RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST:
                if (rnl_parse_ipcm_assign_to_dif_req_msg(info,
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
        case RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST:
                if (rnl_parse_ipcm_unreg_app_req_msg(info,
                                                     msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_QUERY_RIB_REQUEST:
                if (rnl_parse_ipcm_query_rib_req_msg(info,
                                                     msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_RMT_MODIFY_FTE_REQUEST:
                if (rnl_parse_rmt_modify_fte_req_msg(info,
                                                     msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_RMT_DUMP_FT_REQUEST:
                break;
        case RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST:
                if (rnl_parse_ipcp_set_policy_set_param_req_msg(info,
                                                                msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCP_SELECT_POLICY_SET_REQUEST:
                if (rnl_parse_ipcp_select_policy_set_req_msg(info,
                                                             msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCP_UPDATE_CRYPTO_STATE_REQUEST:
                if (rnl_parse_ipcp_update_crypto_state_req_msg(info,
                                                               msg->attrs) < 0)
                        goto fail;
                break;
        default:
                goto fail;
                break;
        }
        return 0;

 fail:
        LOG_ERR("Could not parse NL message type: %d", info->genlhdr->cmd);
        return -1;
}
EXPORT_SYMBOL(rnl_parse_msg);

/* FORMATTING */
static int format_fail(char * msg_name)
{
        ASSERT(msg_name);

        LOG_ERR("Could not format %s message correctly", msg_name);

        return -1;
}

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

        /*
         * FIXME: librina does not define ranges for these attributes, just
         * unique values. So far I seleced only the max or min value depending
         * on the most restrincting (in this case all max).
         */

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
        if (fspec->max_allowable_gap >= 0)
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

static int rnl_format_generic_u32_param_msg(u32              param_var,
                                            uint_t           param_name,
                                            string_t *       msg_name,
                                            struct sk_buff * skb_out)
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

static int rnl_format_ipcm_assign_to_dif_resp_msg(uint_t           result,
                                                  struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                IAFRRM_ATTR_RESULT,
                                                "rnl_ipcm_assign_"
                                                "to_dif_resp_msg",
                                                skb_out);
}

static int rnl_format_ipcm_update_dif_config_resp_msg(uint_t           result,
                                                      struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                IAFRRM_ATTR_RESULT,
                                                "rnl_ipcm_update_dif_"
                                                "config_resp_msg",
                                                skb_out);
}

static int
rnl_format_ipcm_alloc_flow_req_arrived_msg(const struct name *      source,
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
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");
        }

        /* name-formating might be moved into its own function (and reused) */
        if (!(msg_src_name =
              nla_nest_start(skb_out, IAFRA_ATTR_SOURCE_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_src_name);
                LOG_ERR(BUILD_STRERROR("source application name attribute"));
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");
        }

        if (format_app_name_info(source, skb_out) < 0)
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");

        nla_nest_end(skb_out, msg_src_name);

        if (!(msg_dst_name =
              nla_nest_start(skb_out, IAFRA_ATTR_DEST_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_dst_name);
                LOG_ERR(BUILD_STRERROR("destination app name attribute"));
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");
        }

        if (format_app_name_info(dest, skb_out) < 0)
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");

        nla_nest_end(skb_out, msg_dst_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IAFRA_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_STRERROR("DIF name attribute"));
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");
        }

        if (format_app_name_info(dif_name, skb_out) < 0)
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");

        nla_nest_end(skb_out, msg_dif_name);

        if (!(msg_fspec =
              nla_nest_start(skb_out, IAFRA_ATTR_FLOW_SPEC))) {
                nla_nest_cancel(skb_out, msg_fspec);
                LOG_ERR(BUILD_STRERROR("flow spec attribute"));
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");
        }

        if (format_flow_spec(fspec, skb_out) < 0)
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");

        nla_nest_end(skb_out, msg_fspec);

        if (nla_put_u32(skb_out, IAFRA_ATTR_PORT_ID, pid))
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");

        return 0;
}

static int rnl_format_ipcm_alloc_flow_req_result_msg(uint_t           result,
                                                     port_id_t        pid,
                                                     struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IAFRRM_ATTR_PORT_ID, pid))
                return format_fail("rnl_format_ipcm_alloc_req_result_msg");

        if (nla_put_u32(skb_out, IAFRRM_ATTR_RESULT, result))
                return format_fail("rnl_format_ipcm_alloc_req_result_msg");

        return 0;
}

static int rnl_format_ipcm_dealloc_flow_resp_msg(uint_t           result,
                                                 struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                IDFRE_ATTR_RESULT,
                                                "rnl_ipcm_dealloc_"
                                                "flow_resp_msg",
                                                skb_out);
}

static int rnl_format_ipcm_flow_dealloc_noti_msg(port_id_t        id,
                                                 uint_t           code,
                                                 struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IFDN_ATTR_PORT_ID, id))
                return format_fail("rnl_ipcm_alloc_flow_resp_msg");

        if (nla_put_u32(skb_out, IFDN_ATTR_CODE, code))
                return format_fail("rnl_ipcm_alloc_flow_resp_msg");

        return 0;
}

static int rnl_format_ipcm_conn_create_resp_msg(port_id_t        id,
                                                cep_id_t         src_cep,
                                                struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICCRE_ATTR_PORT_ID, id))
                return format_fail("rnl_format_ipcm_conn_create_resp_msg");

        if (nla_put_u32(skb_out, ICCRE_ATTR_SOURCE_CEP_ID, src_cep))
                return format_fail("rnl_format_ipcm_conn_create_resp_msg");

        return 0;
}

static int rnl_format_ipcm_conn_create_result_msg(port_id_t        id,
                                                  cep_id_t         src_cep,
                                                  cep_id_t         dst_cep,
                                                  struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICCRS_ATTR_PORT_ID, id))
                return format_fail("rnl_format_ipcm_conn_create_result_msg");

        if (nla_put_u32(skb_out, ICCRS_ATTR_SOURCE_CEP_ID, src_cep))
                return format_fail("rnl_format_ipcm_conn_create_result_msg");

        if (nla_put_u32(skb_out, ICCRS_ATTR_DEST_CEP_ID, dst_cep))
                return format_fail("rnl_format_ipcm_conn_create_result_msg");

        return 0;
}

static int rnl_format_ipcm_conn_update_result_msg(port_id_t        id,
                                                  uint_t           result,
                                                  struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICURS_ATTR_PORT_ID, id))
                return format_fail("rnl_format_ipcm_conn_update_result_msg");

        if (nla_put_u32(skb_out, ICURS_ATTR_RESULT, result))
                return format_fail("rnl_format_ipcm_conn_update_result_msg");

        return 0;
}

static int rnl_format_ipcm_conn_destroy_result_msg(port_id_t        id,
                                                   uint_t           result,
                                                   struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICDRS_ATTR_PORT_ID, id))
                return format_fail("rnl_format_ipcm_conn_destroy_result_msg");

        if (nla_put_u32(skb_out, ICDRS_ATTR_RESULT, result))
                return format_fail("rnl_format_ipcm_conn_destroy_result_msg");

        return 0;
}

static int rnl_format_ipcm_reg_app_resp_msg(uint_t           result,
                                            struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IRARE_ATTR_RESULT, result))
                return format_fail("rnl_ipcm_reg_app_resp_msg");

        return 0;
}

static int rnl_format_socket_closed_notification_msg(u32              nl_port,
                                                     struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(nl_port,
                                                ISCN_ATTR_PORT,
                                                "rnl_format_socket_closed_"
                                                "notification_msg",
                                                skb_out);
}

static int send_nl_unicast_msg(struct net *     net,
                               struct sk_buff * skb,
                               u32              portid,
                               msg_type_t       type,
                               rnl_sn_t         seq_num)
{
        int result;

        if (!net) {
                LOG_ERR("Wrong net parameter, cannot send unicast NL message");
                return -1;
        }

        if (!skb) {
                LOG_ERR("Wrong skb parameter, cannot send unicast NL message");
                return -1;
        }

        LOG_DBG("Going to send NL unicast message "
                "(type = %d, seq-num %u, port = %u)",
                (int) type, seq_num, portid);

        result = genlmsg_unicast(net, skb, portid);
        if (result) {
                LOG_ERR("Could not send NL message "
                        "(type = %d, seq-num %u, port = %u, result = %d)",
                        (int) type, seq_num, portid, result);
                nlmsg_free(skb);
                return -1;
        }

        LOG_DBG("Unicast NL message sent "
                "(type = %d, seq-num %u, port = %u)",
                (int) type, seq_num, portid);

        return 0;
}

static int format_port_id_altlist(struct port_id_altlist *pos,
				  struct sk_buff * skb_out)
{
        struct nlattr * msg_ports;
	int i;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_ports =
              nla_nest_start(skb_out, PIA_ATTR_PORTIDS))) {
                nla_nest_cancel(skb_out, msg_ports);
                return -1;
        }

        for (i = 1; i <= pos->num_ports; i++) {
                if (nla_put_u32(skb_out, i, pos->ports[i-1]))
                        return -1;
        }

        nla_nest_end(skb_out, msg_ports);

	return 0;
}

static int format_pff_entry_altlists(struct list_head * entries,
                                         struct sk_buff * skb_out)
{
        struct nlattr * msg_alts;
	struct port_id_altlist * pos, * nxt;


        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_alts =
              nla_nest_start(skb_out, PFFELE_ATTR_PORT_ID_ALTLISTS))) {
                nla_nest_cancel(skb_out, msg_alts);
                return -1;
        }

	list_for_each_entry_safe(pos, nxt, entries, next) {
		if (format_port_id_altlist(pos,
                                           skb_out))
                        return format_fail("rnl_ipcm_pff_dump_resp_msg");

		list_del(&pos->next);
		rkfree(pos);
	}

        nla_nest_end(skb_out, msg_alts);

        return 0;
}

static int format_pff_entries_list(struct list_head * entries,
                                   struct sk_buff *   skb_out)
{
        struct nlattr * msg_entry;
        struct mod_pff_entry * pos, * nxt;
        int i = 0;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        list_for_each_entry_safe(pos, nxt, entries, next) {
                i++;
                if (!(msg_entry =
                      nla_nest_start(skb_out, i))) {
                        nla_nest_cancel(skb_out, msg_entry);
                        LOG_ERR(BUILD_STRERROR("pff entries list attribute"));
                        return format_fail("rnl_ipcm_pff_dump_resp_msg");
                }

                if (nla_put_u32(skb_out,
                                 PFFELE_ATTR_ADDRESS,
                                 pos->fwd_info)                        ||
                     nla_put_u32(skb_out, PFFELE_ATTR_QOSID, pos->qos_id) ||
			format_pff_entry_altlists(&pos->port_id_altlists,
						      skb_out))
                        return format_fail("rnl_ipcm_pff_dump_resp_msg");

                nla_nest_end(skb_out, msg_entry);
                list_del(&pos->next);
                rkfree(pos);
        }

        return 0;
}

static int rnl_format_ipcm_pff_dump_resp_msg(int                result,
                                             struct list_head * entries,
                                             struct sk_buff *   skb_out)
{
        struct nlattr * msg_entries;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, RPFD_ATTR_RESULT, result) < 0)
                return format_fail("rnl_ipcm_pff_dump_resp_msg");

        if (!(msg_entries =
              nla_nest_start(skb_out, RPFD_ATTR_ENTRIES))) {
                nla_nest_cancel(skb_out, msg_entries);
                LOG_ERR(BUILD_STRERROR("pff entries list attribute"));
                return format_fail("rnl_ipcm_pff_dump_resp_msg");
        }
        if (format_pff_entries_list(entries, skb_out))
                return format_fail("rnl_ipcm_pff_dump_resp_msg");
        nla_nest_end(skb_out, msg_entries);

        return 0;
}

static int format_ro_entries_list(struct list_head * entries,
                                   struct sk_buff *   skb_out)
{
        struct nlattr * msg_entry;
        struct rib_object_entry * pos, *nxt;
        int i = 0;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        list_for_each_entry_safe(pos, nxt, entries, next) {
                i++;
                if (!(msg_entry =
                      nla_nest_start(skb_out, i))) {
                        nla_nest_cancel(skb_out, msg_entry);
                        LOG_ERR(BUILD_STRERROR("ro entries list attribute"));
                        return format_fail("rnl_ipcm_query_rib_resp_msg");
                }

                if (nla_put_string(skb_out,
                		   RIBO_ATTR_OBJECT_CLASS,
                		   pos->class)             ||
                     nla_put_string(skb_out,
                    		    RIBO_ATTR_OBJECT_NAME,
                    		    pos->name)   	   ||
                     nla_put_string(skb_out,
                    		    RIBO_ATTR_OBJECT_DISPLAY_VALUE,
                    		    pos->display_value)    ||
                     nla_put_u64(skb_out,
                    		 RIBO_ATTR_OBJECT_INSTANCE,
                    		 pos->instance))
                        return format_fail("rnl_ipcm_query_rib_resp_msg");

                nla_nest_end(skb_out, msg_entry);
                list_del(&pos->next);
                rkfree(pos->class);
                rkfree(pos->name);
                rkfree(pos->display_value);
                rkfree(pos);
        }

        return 0;
}

static int rnl_format_ipcm_query_rib_resp_msg(int                result,
                                              struct list_head * entries,
                                              struct sk_buff *   skb_out)
{
        struct nlattr * msg_entries;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IDQRE_ATTR_RESULT, result) < 0)
                return format_fail("rnl_ipcm_pff_dump_resp_msg");

        if (!(msg_entries =
              nla_nest_start(skb_out, IDQRE_ATTR_RIB_OBJECTS))) {
                nla_nest_cancel(skb_out, msg_entries);
                LOG_ERR(BUILD_STRERROR("rib object list attribute"));
                return format_fail("rnl_ipcm_query_rib_resp_msg");
        }
        if (format_ro_entries_list(entries, skb_out))
                return format_fail("rnl_ipcm_query_rib_resp_msg");
        nla_nest_end(skb_out, msg_entries);

        return 0;
}

static int rnl_format_ipcp_set_policy_set_param_resp_msg(
                                                uint_t           result,
                                                struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                ISPSPR_ATTR_RESULT,
                                                "rnl_ipcp_set_policy_set"
                                                "_param_resp_msg",
                                                skb_out);
}

static int rnl_format_ipcp_select_policy_set_resp_msg(
                                                uint_t           result,
                                                struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                ISPSR_ATTR_RESULT,
                                                "rnl_ipcp_select_policy"
                                                "_set_resp_msg",
                                                skb_out);
}

static int rnl_format_ipcp_update_crypto_state_resp_msg(uint_t           result,
                                                	port_id_t 	 port_id,
                                                	struct sk_buff * skb_out)
{
	if (!skb_out) {
		LOG_ERR("Bogus input parameter(s), bailing out");
		return -1;
	}

	if (nla_put_u32(skb_out, IUCSRE_ATTR_RESULT, result) < 0)
		return format_fail("rnl_format_ipcp_update_crypto_state_resp_msg");

	if (nla_put_u32(skb_out, IUCSRE_ATTR_N_1_PORT, port_id) < 0)
		return format_fail("rnl_format_ipcp_update_crypto_state_resp_msg");

        return 0;
}

int rnl_assign_dif_response(ipc_process_id_t id,
                            uint_t           res,
                            rnl_sn_t         seq_num,
                            u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;

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

        genlmsg_end(out_msg, out_hdr);

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE,
                                   seq_num);

}
EXPORT_SYMBOL(rnl_assign_dif_response);

int rnl_update_dif_config_response(ipc_process_id_t id,
                                   uint_t           res,
                                   rnl_sn_t         seq_num,
                                   u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;

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

        genlmsg_end(out_msg, out_hdr);

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

        genlmsg_end(out_msg, out_hdr);

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
                nlmsg_free(msg);
                return -1;
        }

        genlmsg_end(msg, hdr);

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
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

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
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;

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
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

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
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;

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
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

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
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_ATOMIC);
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
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

#if 0
        result = genlmsg_unicast(&init_net, out_msg, nl_port_id);
#endif

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
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;

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
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

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
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

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
                            RINA_C_IPCP_CONN_DESTROY_RESULT);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_conn_destroy_result_msg(pid, res, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

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
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   dest_port,
                                   RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION,
                                   0);
}
EXPORT_SYMBOL(rnl_ipcm_sock_closed_notif_msg);

int rnl_ipcp_pff_dump_resp_msg(ipc_process_id_t   ipc_id,
                               int                result,
                               struct list_head * entries,
                               rnl_sn_t           seq_num,
                               u32                nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;

        /* FIXME: Maybe size should be obtained somehow */
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
                            RINA_C_RMT_DUMP_FT_REPLY);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_pff_dump_resp_msg(result, entries, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

        result = genlmsg_unicast(&init_net, out_msg, nl_port_id);
        if (result) {
                LOG_ERR("Could not send unicast msg: %d", result);
                return -1;
        }
        return 0;
}
EXPORT_SYMBOL(rnl_ipcp_pff_dump_resp_msg);

int rnl_ipcm_query_rib_resp_msg(ipc_process_id_t   ipc_id,
                                int                result,
                                struct list_head * entries,
                                rnl_sn_t           seq_num,
                                u32                nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;

        /* FIXME: Maybe size should be obtained somehow */
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
                            RINA_C_IPCM_QUERY_RIB_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_query_rib_resp_msg(result, entries, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

        result = genlmsg_unicast(&init_net, out_msg, nl_port_id);
        if (result) {
                LOG_ERR("Could not send unicast msg: %d", result);
                return -1;
        }
        return 0;
}
EXPORT_SYMBOL(rnl_ipcm_query_rib_resp_msg);

int rnl_set_policy_set_param_response(ipc_process_id_t id,
                                      uint_t           res,
                                      rnl_sn_t         seq_num,
                                      u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;

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
                            RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = id;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcp_set_policy_set_param_resp_msg(res, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE,
                                   seq_num);

}
EXPORT_SYMBOL(rnl_set_policy_set_param_response);

int rnl_select_policy_set_response(ipc_process_id_t id,
                                   uint_t           res,
                                   rnl_sn_t         seq_num,
                                   u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;

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
                            RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = id;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcp_select_policy_set_resp_msg(res, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE,
                                   seq_num);

}
EXPORT_SYMBOL(rnl_select_policy_set_response);

int rnl_update_crypto_state_response(ipc_process_id_t id,
                                     uint_t           res,
                                     rnl_sn_t         seq_num,
                                     port_id_t	    n_1_port,
                                     u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;

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
                            RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = id;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcp_update_crypto_state_resp_msg(res, n_1_port, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        genlmsg_end(out_msg, out_hdr);

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_update_crypto_state_response);
