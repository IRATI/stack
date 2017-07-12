/*
 * Utilities to serialize and deserialize
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#ifndef IRATI_SERDES_H
#define IRATI_SERDES_H

#include "irati/kucommon.h"

#ifdef __cplusplus
extern "C" {
#endif

struct irati_msg_layout {
    unsigned int copylen;
    unsigned int names;
    unsigned int strings;
    unsigned int flow_specs;
    unsigned int dif_configs;
    unsigned int dtp_configs;
    unsigned int dtcp_configs;
    unsigned int query_rib_resps;
    unsigned int pff_entry_lists;
    unsigned int sdup_crypto_states;
    unsigned int dif_properties;
    unsigned int ipcp_neigh_lists;
    unsigned int media_reports;
    unsigned int buffers;
};

void serialize_string(void **pptr, const char *s);
int deserialize_string(const void **pptr, char **s);

void serialize_buffer(void **pptr, const struct buffer *b);
int deserialize_buffer(const void **pptr, struct buffer **b);

int rina_sername_valid(const char *str);
unsigned rina_name_serlen(const struct name *name);
void serialize_rina_name(void **pptr, const struct name *name);
int deserialize_rina_name(const void **pptr, struct name ** name);
void rina_name_move(struct name *dst, struct name *src);
int rina_name_copy(struct name *dst, const struct name *src);
char *rina_name_to_string(const struct name *name);
int rina_name_from_string(const char *str, struct name *name);
int rina_name_cmp(const struct name *one, const struct name *two);
int rina_name_fill(struct name *name, const char *apn,
                   const char *api, const char *aen, const char *aei);
int rina_name_valid(const struct name *name);

int flow_spec_serlen(const struct flow_spec * fspec);
void serialize_flow_spec(void **pptr, const struct flow_spec *fspec);
int deserialize_flow_spec(const void **pptr, struct flow_spec ** fspec);

int policy_parm_serlen(const struct policy_parm * prm);
void serialize_policy_parm(void **pptr, const struct policy_parm *prm);
int deserialize_policy_parm(const void **pptr, struct policy_parm *prm);

int policy_serlen(const struct policy * policy);
void serialize_policy(void **pptr, const struct policy *prm);
int deserialize_policy(const void **pptr, struct policy *prm);

int dtp_config_serlen(const struct dtp_config * dtp_config);
void serialize_dtp_config(void **pptr, const struct dtp_config *dtp_config);
int deserialize_dtp_config(const void **pptr, struct dtp_config ** dtp_config);

int window_fctrl_config_serlen(const struct window_fctrl_config * wfc);
void serialize_window_fctrl_config(void **pptr, const struct window_fctrl_config *wfc);
int deserialize_window_fctrl_config(const void **pptr, struct window_fctrl_config *wfc);

int rate_fctrl_config_serlen(const struct rate_fctrl_config * rfc);
void serialize_rate_fctrl_config(void **pptr, const struct rate_fctrl_config *rfc);
int deserialize_rate_fctrl_config(const void **pptr, struct rate_fctrl_config *rfc);

int dtcp_fctrl_config_serlen(const struct dtcp_fctrl_config * dfc);
void serialize_dtcp_fctrl_config(void **pptr, const struct dtcp_fctrl_config *dfc);
int deserialize_dtcp_fctrl_config(const void **pptr, struct dtcp_fctrl_config *dfc);

int dtcp_rxctrl_config_serlen(const struct dtcp_rxctrl_config * rxfc);
void serialize_dtcp_rxctrl_config(void **pptr, const struct dtcp_rxctrl_config *rxfc);
int deserialize_dtcp_rxctrl_config(const void **pptr, struct dtcp_rxctrl_config *rxfc);

int dtcp_config_serlen(const struct dtcp_config * dtcp_config);
void serialize_dtcp_config(void **pptr, const struct dtcp_config *dtcp_config);
int deserialize_dtcp_config(const void **pptr, struct dtcp_config **dtcp_config);

int pff_config_serlen(const struct pff_config * pff);
void serialize_pff_config(void **pptr, const struct pff_config *pff);
int deserialize_pff_config(const void **pptr, struct pff_config *pff);

int rmt_config_serlen(const struct rmt_config * rmt);
void serialize_rmt_config(void **pptr, const struct rmt_config *rmt);
int deserialize_rmt_config(const void **pptr, struct rmt_config *rmt);

int dt_cons_serlen(const struct dt_cons * dtc);
void serialize_dt_cons(void **pptr, const struct dt_cons *dtc);
int deserialize_dt_cons(const void **pptr, struct dt_cons *dtc);

int qos_cube_serlen(const struct qos_cube * qos);
void serialize_qos_cube(void **pptr, const struct qos_cube *qos);
int deserialize_qos_cube(const void **pptr, struct qos_cube *qos);

int efcp_config_serlen(const struct efcp_config * efc);
void serialize_efcp_config(void **pptr, const struct efcp_config *efc);
int deserialize_efcp_config(const void **pptr, struct efcp_config *efc);

int fa_config_serlen(const struct fa_config * fac);
void serialize_fa_config(void **pptr, const struct fa_config *fac);
int deserialize_fa_config(const void **pptr, struct fa_config *fac);

int resall_config_serlen(const struct resall_config * resc);
void serialize_resall_config(void **pptr, const struct resall_config *resc);
int deserialize_resall_config(const void **pptr, struct resall_config *resc);

int et_config_serlen(const struct et_config * etc);
void serialize_et_config(void **pptr, const struct et_config *etc);
int deserialize_et_config(const void **pptr, struct et_config *etc);

int static_ipcp_addr_serlen(const struct static_ipcp_addr * addr);
void serialize_static_ipcp_addr(void **pptr, const struct static_ipcp_addr *addr);
int deserialize_static_ipcp_addr(const void **pptr, struct static_ipcp_addr *addr);

int address_pref_config_serlen(const struct address_pref_config * apc);
void serialize_address_pref_config(void **pptr, const struct address_pref_config *apc);
int deserialize_address_pref_config(const void **pptr, struct address_pref_config *apc);

int addressing_config_serlen(const struct addressing_config * ac);
void serialize_addressing_config(void **pptr, const struct addressing_config *ac);
int deserialize_addressing_config(const void **pptr, struct addressing_config *ac);

int nsm_config_serlen(const struct nsm_config * nsmc);
void serialize_nsm_config(void **pptr, const struct nsm_config *nsmc);
int deserialize_nsm_config(const void **pptr, struct nsm_config *nsmc);

int auth_sdup_profile_serlen(const struct auth_sdup_profile * asp);
void serialize_auth_sdup_profile(void **pptr, const struct auth_sdup_profile *asp);
int deserialize_auth_sdup_profile(const void **pptr, struct auth_sdup_profile *asp);

int secman_config_serlen(const struct secman_config * sc);
void serialize_secman_config(void **pptr, const struct secman_config *sc);
int deserialize_secman_config(const void **pptr, struct secman_config *sc);

int routing_config_serlen(const struct routing_config * rc);
void serialize_routing_config(void **pptr, const struct routing_config *rc);
int deserialize_routing_config(const void **pptr, struct routing_config *rc);

int ipcp_config_entry_serlen(const struct ipcp_config_entry * ice);
void serialize_ipcp_config_entry(void **pptr, const struct ipcp_config_entry *ice);
int deserialize_ipcp_config_entry(const void **pptr, struct ipcp_config_entry *ice);

int dif_config_serlen(const struct dif_config * dif_config);
void serialize_dif_config(void **pptr, const struct dif_config *dif_config);
int deserialize_dif_config(const void **pptr, struct dif_config ** dif_config);

int rib_object_data_serlen(const struct rib_object_data * rod);
void serialize_rib_object_data(void **pptr, const struct rib_object_data *rod);
int deserialize_rib_object_data(const void **pptr, struct rib_object_data *rod);

int query_rib_resp_serlen(const struct query_rib_resp * qrr);
void serialize_query_rib_resp(void **pptr, const struct query_rib_resp *qrr);
int deserialize_query_rib_resp(const void **pptr, struct query_rib_resp **qrr);

int port_id_altlist_serlen(const struct port_id_altlist * pia);
void serialize_port_id_altlist(void **pptr, const struct port_id_altlist *pia);
int deserialize_port_id_altlist(const void **pptr, struct port_id_altlist *pia);

int mod_pff_entry_serlen(const struct mod_pff_entry * pffe);
void serialize_mod_pff_entry(void **pptr, const struct mod_pff_entry *pffe);
int deserialize_mod_pff_entry(const void **pptr, struct mod_pff_entry *pffe);

int pff_entry_list_serlen(const struct pff_entry_list * pel);
void serialize_pff_entry_list(void **pptr, const struct pff_entry_list *pel);
int deserialize_pff_entry_list(const void **pptr, struct pff_entry_list **pel);

int sdup_crypto_state_serlen(const struct sdup_crypto_state * scs);
void serialize_sdup_crypto_state(void **pptr, const struct sdup_crypto_state *scs);
int deserialize_sdup_crypto_state(const void **pptr, struct sdup_crypto_state **scs);

int dif_properties_entry_serlen(const struct dif_properties_entry * dpe);
void serialize_dif_properties_entry(void **pptr, const struct dif_properties_entry *dpe);
int deserialize_dif_properties_entry(const void **pptr, struct dif_properties_entry *dpe);

int get_dif_prop_resp_serlen(const struct get_dif_prop_resp * gdp);
void serialize_get_dif_prop_resp(void **pptr, const struct get_dif_prop_resp *gdp);
int deserialize_get_dif_prop_resp(const void **pptr, struct get_dif_prop_resp **gdp);

int ipcp_neighbor_serlen(const struct ipcp_neighbor * nei);
void serialize_ipcp_neighbor(void **pptr, const struct ipcp_neighbor *nei);
int deserialize_ipcp_neighbor(const void **pptr, struct ipcp_neighbor *nei);

int ipcp_neigh_list_serlen(const struct ipcp_neigh_list * nei);
void serialize_ipcp_neigh_list(void **pptr, const struct ipcp_neigh_list *nei);
int deserialize_ipcp_neigh_list(const void **pptr, struct ipcp_neigh_list **nei);

int bs_info_entry_serlen(const struct bs_info_entry * bie);
void serialize_bs_info_entry(void **pptr, const struct bs_info_entry *bie);
int deserialize_bs_info_entry(const void **pptr, struct bs_info_entry *bie);

int media_dif_info_serlen(const struct media_dif_info * mdi);
void serialize_media_dif_info(void **pptr, const struct media_dif_info *mdi);
int deserialize_media_dif_info(const void **pptr, struct media_dif_info *mdi);

int media_report_serlen(const struct media_report * mre);
void serialize_media_report(void **pptr, const struct media_report *mre);
int deserialize_media_report(const void **pptr, struct media_report **mre);

unsigned int irati_msg_serlen(struct irati_msg_layout *numtables,
                              size_t num_entries,
                              const struct irati_msg_base *msg);
unsigned int irati_numtables_max_size(struct irati_msg_layout *numtables,
                                      unsigned int n);
int serialize_irati_msg(struct irati_msg_layout *numtables,
                        size_t num_entries,
                        void *serbuf,
                        const struct irati_msg_base *msg);
void * deserialize_irati_msg(struct irati_msg_layout *numtables,
			     size_t num_entries,
                             const void *serbuf,
			     unsigned int serbuf_len);
void irati_msg_free(struct irati_msg_layout *numtables,
		    size_t num_entries,
                    struct irati_msg_base *msg);

#ifdef __KERNEL__
/* GFP variations of some of the functions above. */
int __rina_name_fill(struct name *name, const char *apn,
                      const char *api, const char *aen, const char *aei,
                      int maysleep);

char * __rina_name_to_string(const struct name *name, int maysleep);
int __rina_name_from_string(const char *str, struct name *name,
                            int maysleep);
#else  /* !__KERNEL__ */
#endif /* !__KERNEL__ */

#ifdef __cplusplus
}
#endif

#endif /* IRATI_SERDES_H */
