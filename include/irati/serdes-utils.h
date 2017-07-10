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
struct buffer * default_buffer_create(void);

int rina_sername_valid(const char *str);
unsigned rina_name_serlen(const struct name *name);
void serialize_rina_name(void **pptr, const struct name *name);
int deserialize_rina_name(const void **pptr, struct name ** name);
void rina_name_free(struct name *name);
void rina_name_move(struct name *dst, struct name *src);
int rina_name_copy(struct name *dst, const struct name *src);
char *rina_name_to_string(const struct name *name);
int rina_name_from_string(const char *str, struct name *name);
int rina_name_cmp(const struct name *one, const struct name *two);
int rina_name_fill(struct name *name, const char *apn,
                   const char *api, const char *aen, const char *aei);
int rina_name_valid(const struct name *name);
struct name * rina_name_create(void);
struct name * rina_default_name_create(void);

int flow_spec_serlen(const struct flow_spec * fspec);
void serialize_flow_spec(void **pptr, const struct flow_spec *fspec);
int deserialize_flow_spec(const void **pptr, struct flow_spec ** fspec);
void flow_spec_free(struct flow_spec * fspec);
struct flow_spec * rina_fspec_create(void);
struct flow_spec * rina_default_fspec_create(void);

int policy_parm_serlen(const struct policy_parm * prm);
void serialize_policy_parm(void **pptr, const struct policy_parm *prm);
int deserialize_policy_parm(const void **pptr, struct policy_parm *prm);
void policy_parm_free(struct policy_parm * prm);
struct policy_parm * policy_parm_create(void);
struct policy_parm * policy_parm_default_create(void);

int policy_serlen(const struct policy * policy);
void serialize_policy(void **pptr, const struct policy *prm);
int deserialize_policy(const void **pptr, struct policy *prm);
void policy_free(struct policy * policy);
struct policy * policy_create(void);
struct policy * policy_default_create(void);

int dtp_config_serlen(const struct dtp_config * dtp_config);
void serialize_dtp_config(void **pptr, const struct dtp_config *dtp_config);
int deserialize_dtp_config(const void **pptr, struct dtp_config ** dtp_config);
void dtp_config_free(struct dtp_config * dtp_config);
struct dtp_config * dtp_config_create(void);
struct dtp_config * dtp_config_default_create(void);

int window_fctrl_config_serlen(const struct window_fctrl_config * wfc);
void serialize_window_fctrl_config(void **pptr, const struct window_fctrl_config *wfc);
int deserialize_window_fctrl_config(const void **pptr, struct window_fctrl_config *wfc);
void window_fctrl_config_free(struct window_fctrl_config * wfc);
struct window_fctrl_config * window_fctrl_config_create(void);
struct window_fctrl_config * window_fctrl_config_default_create(void);

int rate_fctrl_config_serlen(const struct rate_fctrl_config * rfc);
void serialize_rate_fctrl_config(void **pptr, const struct rate_fctrl_config *rfc);
int deserialize_rate_fctrl_config(const void **pptr, struct rate_fctrl_config *rfc);
void rate_fctrl_config_free(struct rate_fctrl_config * rfc);
struct rate_fctrl_config * rate_fctrl_config_create(void);
struct rate_fctrl_config * rate_fctrl_config_default_create(void);

int dtcp_fctrl_config_serlen(const struct dtcp_fctrl_config * dfc);
void serialize_dtcp_fctrl_config(void **pptr, const struct dtcp_fctrl_config *dfc);
int deserialize_dtcp_fctrl_config(const void **pptr, struct dtcp_fctrl_config *dfc);
void dtcp_fctrl_config_free(struct dtcp_fctrl_config * dfc);
struct dtcp_fctrl_config * dtcp_fctrl_config_create(void);
struct dtcp_fctrl_config * dtcp_fctrl_config_default_create(void);

int dtcp_rxctrl_config_serlen(const struct dtcp_rxctrl_config * rxfc);
void serialize_dtcp_rxctrl_config(void **pptr, const struct dtcp_rxctrl_config *rxfc);
int deserialize_dtcp_rxctrl_config(const void **pptr, struct dtcp_rxctrl_config *rxfc);
void dtcp_rxctrl_config_free(struct dtcp_rxctrl_config * rxfc);
struct dtcp_rxctrl_config * dtcp_rxctrl_config_create(void);
struct dtcp_rxctrl_config * dtcp_rxctrl_config_default_create(void);

int dtcp_config_serlen(const struct dtcp_config * dtcp_config);
void serialize_dtcp_config(void **pptr, const struct dtcp_config *dtcp_config);
int deserialize_dtcp_config(const void **pptr, struct dtcp_config **dtcp_config);
void dtcp_config_free(struct dtcp_config * dtcp_config);
struct dtcp_config * dtcp_config_create(void);
struct dtcp_config * dtcp_config_default_create(void);

int pff_config_serlen(const struct pff_config * pff);
void serialize_pff_config(void **pptr, const struct pff_config *pff);
int deserialize_pff_config(const void **pptr, struct pff_config *pff);
void pff_config_free(struct pff_config * pff);
struct pff_config * pff_config_create(void);
struct pff_config * pff_config_default_create(void);

int rmt_config_serlen(const struct rmt_config * rmt);
void serialize_rmt_config(void **pptr, const struct rmt_config *rmt);
int deserialize_rmt_config(const void **pptr, struct rmt_config *rmt);
void rmt_config_free(struct rmt_config * rmt);
struct rmt_config * rmt_config_create(void);
struct rmt_config * rmt_config_default_create(void);

int dup_config_entry_serlen(const struct dup_config_entry * dce);
void serialize_dup_config_entry(void **pptr, const struct dup_config_entry *dce);
int deserialize_dup_config_entry(const void **pptr, struct dup_config_entry *dce);
void dup_config_entry_free(struct dup_config_entry * dce);
struct dup_config_entry * dup_config_entry_create(void);
struct dup_config_entry * dup_config_entry_default_create(void);

int sdup_config_serlen(const struct sdup_config * sdc);
void serialize_sdup_config(void **pptr, const struct sdup_config *sdc);
int deserialize_sdup_config(const void **pptr, struct sdup_config *sdc);
void sdup_config_free(struct sdup_config * sdc);
struct sdup_config * sdup_config_create(void);
struct sdup_config * sdup_config_default_create(void);

int dt_cons_serlen(const struct dt_cons * dtc);
void serialize_dt_cons(void **pptr, const struct dt_cons *dtc);
int deserialize_dt_cons(const void **pptr, struct dt_cons *dtc);
void dt_cons_free(struct dt_cons * dtc);
struct dt_cons * dt_cons_create(void);
struct dt_cons * dt_cons_default_create(void);

int qos_cube_serlen(const struct qos_cube * qos);
void serialize_qos_cube(void **pptr, const struct qos_cube *qos);
int deserialize_qos_cube(const void **pptr, struct qos_cube *qos);
void qos_cube_free(struct qos_cube * qos);
struct qos_cube * qos_cube_create(void);
struct qos_cube * qos_cube_default_create(void);

int efcp_config_serlen(const struct efcp_config * efc);
void serialize_efcp_config(void **pptr, const struct efcp_config *efc);
int deserialize_efcp_config(const void **pptr, struct efcp_config *efc);
void efcp_config_free(struct efcp_config * efc);
struct efcp_config * efcp_config_create(void);
struct efcp_config * efcp_config_default_create(void);

int fa_config_serlen(const struct fa_config * fac);
void serialize_fa_config(void **pptr, const struct fa_config *fac);
int deserialize_fa_config(const void **pptr, struct fa_config *fac);
void fa_config_free(struct fa_config * fac);
struct fa_config * fa_config_create(void);
struct fa_config * fa_config_default_create(void);

int resall_config_serlen(const struct resall_config * resc);
void serialize_resall_config(void **pptr, const struct resall_config *resc);
int deserialize_resall_config(const void **pptr, struct resall_config *resc);
void resall_config_free(struct resall_config * resc);
struct resall_config * resall_config_create(void);
struct resall_config * resall_config_default_create(void);

int et_config_serlen(const struct et_config * etc);
void serialize_et_config(void **pptr, const struct et_config *etc);
int deserialize_et_config(const void **pptr, struct et_config *etc);
void et_config_free(struct et_config * etc);
struct et_config * et_config_create(void);
struct et_config * et_config_default_create(void);

int static_ipcp_addr_serlen(const struct static_ipcp_addr * addr);
void serialize_static_ipcp_addr(void **pptr, const struct static_ipcp_addr *addr);
int deserialize_static_ipcp_addr(const void **pptr, struct static_ipcp_addr *addr);
void static_ipcp_addr_free(struct static_ipcp_addr * addr);
struct static_ipcp_addr * static_ipcp_addr_create(void);
struct static_ipcp_addr * static_ipcp_addr_default_create(void);

int address_pref_config_serlen(const struct address_pref_config * apc);
void serialize_address_pref_config(void **pptr, const struct address_pref_config *apc);
int deserialize_address_pref_config(const void **pptr, struct address_pref_config *apc);
void address_pref_config_free(struct address_pref_config * apc);
struct address_pref_config * address_pref_config_create(void);
struct address_pref_config * address_pref_config_default_create(void);

int addressing_config_serlen(const struct addressing_config * ac);
void serialize_addressing_config(void **pptr, const struct addressing_config *ac);
int deserialize_addressing_config(const void **pptr, struct addressing_config *ac);
void addressing_config_free(struct addressing_config * ac);
struct addressing_config * addressing_config_create(void);
struct addressing_config * addressing_config_default_create(void);

int nsm_config_serlen(const struct nsm_config * nsmc);
void serialize_nsm_config(void **pptr, const struct nsm_config *nsmc);
int deserialize_nsm_config(const void **pptr, struct nsm_config *nsmc);
void nsm_config_free(struct nsm_config * nsmc);
struct nsm_config * nsm_config_create(void);
struct nsm_config * nsm_config_default_create(void);

int auth_sdup_profile_serlen(const struct auth_sdup_profile * asp);
void serialize_auth_sdup_profile(void **pptr, const struct auth_sdup_profile *asp);
int deserialize_auth_sdup_profile(const void **pptr, struct auth_sdup_profile *asp);
void auth_sdup_profile_free(struct auth_sdup_profile * asp);
struct auth_sdup_profile * auth_sdup_profile_create(void);
struct auth_sdup_profile * auth_sdup_profile_default_create(void);

int secman_config_serlen(const struct secman_config * sc);
void serialize_secman_config(void **pptr, const struct secman_config *sc);
int deserialize_secman_config(const void **pptr, struct secman_config *sc);
void secman_config_free(struct secman_config * sc);
struct secman_config * secman_config_create(void);
struct secman_config * secman_config_default_create(void);

int routing_config_serlen(const struct routing_config * rc);
void serialize_routing_config(void **pptr, const struct routing_config *rc);
int deserialize_routing_config(const void **pptr, struct routing_config *rc);
void routing_config_free(struct routing_config * rc);
struct routing_config * routing_config_create(void);
struct routing_config * routing_config_default_create(void);

int ipcp_config_entry_serlen(const struct ipcp_config_entry * ice);
void serialize_ipcp_config_entry(void **pptr, const struct ipcp_config_entry *ice);
int deserialize_ipcp_config_entry(const void **pptr, struct ipcp_config_entry *ice);
void ipcp_config_entry_free(struct ipcp_config_entry * ice);
struct ipcp_config_entry * ipcp_config_entry_create(void);
struct ipcp_config_entry * ipcp_config_entry_default_create(void);

int dif_config_serlen(const struct dif_config * dif_config);
void serialize_dif_config(void **pptr, const struct dif_config *dif_config);
int deserialize_dif_config(const void **pptr, struct dif_config ** dif_config);
void dif_config_free(struct dif_config * dif_config);
struct dif_config * dif_config_create(void);
struct dif_config * dif_config_default_create(void);

int rib_object_data_serlen(const struct rib_object_data * rod);
void serialize_rib_object_data(void **pptr, const struct rib_object_data *rod);
int deserialize_rib_object_data(const void **pptr, struct rib_object_data *rod);
void rib_object_data_free(struct rib_object_data * rod);
struct rib_object_data * rib_object_data_create(void);
struct rib_object_data * rib_object_data_default_create(void);

int query_rib_resp_serlen(const struct query_rib_resp * qrr);
void serialize_query_rib_resp(void **pptr, const struct query_rib_resp *qrr);
int deserialize_query_rib_resp(const void **pptr, struct query_rib_resp **qrr);
void query_rib_resp_free(struct query_rib_resp * qrr);
struct query_rib_resp * query_rib_resp_create(void);
struct query_rib_resp * query_rib_resp_default_create(void);

int port_id_altlist_serlen(const struct port_id_altlist * pia);
void serialize_port_id_altlist(void **pptr, const struct port_id_altlist *pia);
int deserialize_port_id_altlist(const void **pptr, struct port_id_altlist *pia);
void port_id_altlist_free(struct port_id_altlist * pia);
struct port_id_altlist * port_id_altlist_create(void);
struct port_id_altlist * port_id_altlist_default_create(void);

int mod_pff_entry_serlen(const struct mod_pff_entry * pffe);
void serialize_mod_pff_entry(void **pptr, const struct mod_pff_entry *pffe);
int deserialize_mod_pff_entry(const void **pptr, struct mod_pff_entry *pffe);
void mod_pff_entry_free(struct mod_pff_entry * pffe);
struct mod_pff_entry * mod_pff_entry_create(void);
struct mod_pff_entry * mod_pff_entry_default_create(void);

int pff_entry_list_serlen(const struct pff_entry_list * pel);
void serialize_pff_entry_list(void **pptr, const struct pff_entry_list *pel);
int deserialize_pff_entry_list(const void **pptr, struct pff_entry_list **pel);
void pff_entry_list_free(struct pff_entry_list * pel);
struct pff_entry_list * pff_entry_list_create(void);
struct pff_entry_list * pff_entry_list_default_create(void);

int sdup_crypto_state_serlen(const struct sdup_crypto_state * scs);
void serialize_sdup_crypto_state(void **pptr, const struct sdup_crypto_state *scs);
int deserialize_sdup_crypto_state(const void **pptr, struct sdup_crypto_state **scs);
void sdup_crypto_state_free(struct sdup_crypto_state * scs);
struct sdup_crypto_state * sdup_crypto_state_create(void);
struct sdup_crypto_state * sdup_crypto_statet_default_create(void);

int dif_properties_entry_serlen(const struct dif_properties_entry * dpe);
void serialize_dif_properties_entry(void **pptr, const struct dif_properties_entry *dpe);
int deserialize_dif_properties_entry(const void **pptr, struct dif_properties_entry *dpe);
void dif_properties_entry_free(struct dif_properties_entry * dpe);
struct dif_properties_entry * dif_properties_entry_create(void);
struct dif_properties_entry * dif_properties_entry_default_create(void);

int get_dif_prop_resp_serlen(const struct get_dif_prop_resp * gdp);
void serialize_get_dif_prop_resp(void **pptr, const struct get_dif_prop_resp *gdp);
int deserialize_get_dif_prop_resp(const void **pptr, struct get_dif_prop_resp **gdp);
void get_dif_prop_resp_free(struct get_dif_prop_resp * gdp);
struct get_dif_prop_resp * get_dif_prop_resp_create(void);
struct get_dif_prop_resp * get_dif_prop_resp_default_create(void);

int ipcp_neighbor_serlen(const struct ipcp_neighbor * nei);
void serialize_ipcp_neighbor(void **pptr, const struct ipcp_neighbor *nei);
int deserialize_ipcp_neighbor(const void **pptr, struct ipcp_neighbor *nei);
void ipcp_neighbor_free(struct ipcp_neighbor * nei);
struct ipcp_neighbor * ipcp_neighbor_create(void);
struct ipcp_neighbor * ipcp_neighbor_default_create(void);

int ipcp_neigh_list_serlen(const struct ipcp_neigh_list * nei);
void serialize_ipcp_neigh_list(void **pptr, const struct ipcp_neigh_list *nei);
int deserialize_ipcp_neigh_list(const void **pptr, struct ipcp_neigh_list **nei);
void ipcp_neigh_list_free(struct ipcp_neigh_list * nei);
struct ipcp_neigh_list * ipcp_neigh_list_create(void);
struct ipcp_neigh_list * ipcp_neigh_list_default_create(void);

int bs_info_entry_serlen(const struct bs_info_entry * bie);
void serialize_bs_info_entry(void **pptr, const struct bs_info_entry *bie);
int deserialize_bs_info_entry(const void **pptr, struct bs_info_entry *bie);
void bs_info_entry_free(struct bs_info_entry * bie);
struct bs_info_entry * bs_info_entry_create(void);
struct bs_info_entry * bs_info_entry_default_create(void);

int media_dif_info_serlen(const struct media_dif_info * mdi);
void serialize_media_dif_info(void **pptr, const struct media_dif_info *mdi);
int deserialize_media_dif_info(const void **pptr, struct media_dif_info *mdi);
void media_dif_info_free(struct media_dif_info * mdi);
struct media_dif_info * media_dif_info_create(void);
struct media_dif_info * media_dif_info_default_create(void);

int media_report_serlen(const struct media_report * mre);
void serialize_media_report(void **pptr, const struct media_report *mre);
int deserialize_media_report(const void **pptr, struct media_report **mre);
void media_report_free(struct media_report * mre);
struct media_report * media_report_create(void);
struct media_report * media_report_default_create(void);

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
