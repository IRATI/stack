/*
 * Common definitions for the IRATI implementation,
 * shared by kernel and user space components
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

#ifndef IRATI_KUCOMMON_H
#define IRATI_KUCOMMON_H

/*
 * When compiling from userspace include <stdint.h>,
 * when compiling from kernelspace include <linux/types.h>
 */
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdio.h>
#include <stdint.h>
#include <asm/ioctl.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IRATI_CTRLDEV_NAME "/dev/irati-ctrl"
#define IRATI_IODEV_NAME   "/dev/irati"
#define IPCM_CTRLDEV_PORT  1

#define PORT_ID_WRONG -1
#define CEP_ID_WRONG -1
#define ADDRESS_WRONG -1
#define QOS_ID_WRONG -1

typedef int32_t  port_id_t;
typedef int32_t  cep_id_t;
typedef int16_t  qos_id_t;
typedef uint32_t address_t;
typedef uint32_t seq_num_t;

typedef uint16_t      ipc_process_id_t;
typedef unsigned int  ipc_process_address_t;

/* We should get rid of the following definitions */
typedef unsigned int  uint_t;
typedef unsigned int  timeout_t;

struct uint_range {
        uint_t min;
        uint_t max;
};

typedef char string_t;

struct buffer {
        unsigned char * data;
        uint32_t size;
};

#ifdef __KERNEL__
#else

/* Buffer functions */
int buffer_destroy(struct buffer * b);
struct buffer * buffer_create(void);

/* List API copied from kernel */
struct list_head {
	struct list_head *next, *prev;
	void * container;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *newl,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = newl;
	newl->next = next;
	newl->prev = prev;
	prev->next = newl;
}

static inline void list_add_tail(struct list_head *newl, struct list_head *head)
{
	__list_add(newl, head->prev, head);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = 0;
	entry->prev = 0;
}

#define container_of(ptr, type, member) ({\
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type, member) );})

#define list_for_each_entry(pos, head, member) \
        for (pos = container_of((head)->next, typeof(*pos),member); \
	     &pos->member != (head); \
	     pos = container_of(pos->member.next,typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member) \
	for (pos = container_of((head)->next, typeof(*pos),member), \
	        n = container_of(pos->member.next,typeof(*pos), member); \
	     &pos->member != (head); \
	     pos = n, n = container_of(n->member.next,typeof(*n), member))

#endif

struct name {
        /*
         * The process_name identifies an application process within the
         * application process namespace. This value is required, it
         * cannot be NULL. This name has global scope (it is defined by
         * the chain of IDD databases that are linked together), and is
         * assigned by an authority that manages the namespace that
         * particular application name belongs to.
         */
        string_t * process_name;

        /*
         * The process_instance identifies a particular instance of the
         * process. This value is optional, it may be NULL.
         *
         */
        string_t * process_instance;

        /*
         * The entity_name identifies an application entity within the
         * application process. This value is optional, it may be NULL.
         */
        string_t * entity_name;

        /*
         * The entity_name identifies a particular instance of an entity within
         * the application process. This value is optional, it may be NULL.
         */
        string_t * entity_instance;
} __attribute__((packed));

struct flow_spec {
        /* This structure defines the characteristics of a flow */

        /* Average bandwidth in bytes/s */
        uint32_t average_bandwidth;

        /* Average bandwidth in SDUs/s */
        uint32_t average_sdu_bandwidth;

        /*
         * In milliseconds, indicates the maximum delay allowed in this
         * flow. A value of 0 indicates 'do not care'
         */
        uint32_t delay;
        /*
         * In milliseconds, indicates the maximum jitter allowed
         * in this flow. A value of 0 indicates 'do not care'
         */
        uint32_t jitter;

        /*
         * Indicates the maximum packet loss (loss/10000) allowed in this
         * flow. A value of loss >=10000 indicates 'do not care'
         */
        uint16_t loss;

        /*
         * Indicates the maximum gap allowed among SDUs, a gap of N
         * SDUs is considered the same as all SDUs delivered.
         * A value of -1 indicates 'Any'
         */
        int32_t max_allowable_gap;

        /*
         * The maximum SDU size for the flow. May influence the choice
         * of the DIF where the flow will be created.
         */
        uint32_t max_sdu_size;

        /* Indicates if SDUs have to be delivered in order */
        bool   ordered_delivery;

        /* Indicates if partial delivery of SDUs is allowed or not */
        bool   partial_delivery;

        /* In milliseconds */
        uint32_t peak_bandwidth_duration;

        /* In milliseconds */
        uint32_t peak_sdu_bandwidth_duration;

        /* A value of 0 indicates 'do not care' */
        /* FIXME: This uint_t has to be transformed back to double */
        uint32_t undetected_bit_error_rate;

        /* Preserve message boundaries */
        bool msg_boundaries;
};

struct policy_parm {
        string_t *       name;
        string_t *       value;
        struct list_head next;
};

struct policy {
        string_t *       name;
        string_t *       version;
        struct list_head params;
};

struct dtp_config {
        bool                 dtcp_present;
        /* Sequence number rollover threshold */
        int                  seq_num_ro_th;
        timeout_t            initial_a_timer;
        bool                 partial_delivery;
        bool                 incomplete_delivery;
        bool                 in_order_delivery;
        seq_num_t            max_sdu_gap;

        struct policy *      dtp_ps;
};

struct window_fctrl_config {
        uint32_t        max_closed_winq_length; /* in cwq */
        uint32_t        initial_credit; /* to initialize sv */
        struct policy * rcvr_flow_control;
        struct policy * tx_control;
};

struct rate_fctrl_config {
        /* FIXME: to initialize sv? does not seem so*/
        uint32_t        sending_rate;
        uint32_t        time_period;
        struct policy * no_rate_slow_down;
        struct policy * no_override_default_peak;
        struct policy * rate_reduction;
};

struct dtcp_fctrl_config {
        bool                         window_based_fctrl;
        struct window_fctrl_config * wfctrl_cfg;
        bool                         rate_based_fctrl;
        struct rate_fctrl_config *   rfctrl_cfg;
        uint32_t                     sent_bytes_th;
        uint32_t                     sent_bytes_percent_th;
        uint32_t                     sent_buffers_th;
        uint32_t                     rcvd_bytes_th;
        uint32_t                     rcvd_bytes_percent_th;
        uint32_t                     rcvd_buffers_th;
        struct policy *              closed_window;
        struct policy *              reconcile_flow_conflict;
        struct policy *              receiving_flow_control;
        struct policy * 	     flow_control_overrun;
};

struct dtcp_rxctrl_config {
        uint32_t        max_time_retry;
        uint32_t        data_retransmit_max;
        uint32_t        initial_tr;
        struct policy * retransmission_timer_expiry;
        struct policy * sender_ack;
        struct policy * receiving_ack_list;
        struct policy * rcvr_ack;
        struct policy * sending_ack;
        struct policy * rcvr_control_ack;
};

/* This is the DTCP configurations from connection policies */
struct dtcp_config {
        bool                        flow_ctrl;
        struct dtcp_fctrl_config *  fctrl_cfg;
        bool                        rtx_ctrl;
        struct dtcp_rxctrl_config * rxctrl_cfg;
        struct policy *             lost_control_pdu;
        struct policy *             dtcp_ps;
        struct policy *             rtt_estimator;
};

enum ipcp_config_type {
        IPCP_CONFIG_UINT   = 1,
        IPCP_CONFIG_STRING,
};

struct ipcp_config_value {
        enum ipcp_config_type type;
        void *                data;
};

struct dt_cons {
        /* The length of the address field in the DTP PCI, in bytes */
        uint16_t address_length;

        /* The length of the CEP-id field in the DTP PCI, in bytes */
        uint16_t cep_id_length;

        /* The length of the length field in the DTP PCI, in bytes */
        uint16_t length_length;

        /* The length of the Port-id field in the DTP PCI, in bytes */
        uint16_t port_id_length;

        /* The length of QoS-id field in the DTP PCI, in bytes */
        uint16_t qos_id_length;

        /* The length of the sequence number field in the DTP PCI, in bytes */
        uint16_t seq_num_length;

        /* The length of the sequence number field in the DTCP PCI, in bytes */
        uint16_t ctrl_seq_num_length;

        /* The maximum length allowed for a PDU in this DIF, in bytes */
        uint32_t max_pdu_size;

        /* The maximum size allowed for a SDU in this DIF, in bytes */
        uint32_t max_sdu_size;

        /*
         * The maximum PDU lifetime in this DIF, in milliseconds. This is MPL
         * in delta-T
         */
        uint32_t max_pdu_life;

        /* Rate for rate based mechanism. */
        uint16_t rate_length;

        /* Time frame for rate based mechanism. */
        uint16_t frame_length;

        /*
         * True if the PDUs in this DIF have CRC, TTL, and/or encryption.
         * Since headers are encrypted, not just user data, if any flow uses
         * encryption, all flows within the same DIF must do so and the same
         * encryption algorithm must be used for every PDU; we cannot identify
         * which flow owns a particular PDU until it has been decrypted.
         */
        bool dif_integrity;

        uint32_t seq_rollover_thres;
        bool dif_concat;
        bool dif_frag;
        uint32_t max_time_to_keep_ret_;
        uint32_t max_time_to_ack_;
};

struct dt_cons * dt_cons_dup(const struct dt_cons * dt_cons);

/* Configuration of a QoS cube with associated DTP/DTCP policies */
struct qos_cube {
	string_t * name;
	uint16_t id;
	uint32_t avg_bw;
	uint32_t avg_sdu_bw;
	uint32_t peak_bw_duration;
	uint32_t peak_sdu_bw_duration;
	bool partial_delivery;
	bool ordered_delivery;
	int32_t max_allowed_gap;
	uint32_t delay;
	uint32_t jitter;
	uint16_t loss;
	struct dtp_config * dtpc;
	struct dtcp_config * dtcpc;
};

struct qos_cube_entry {
	struct list_head next;
	struct qos_cube * entry;
};

/* Represents the configuration of the EFCP */
struct efcp_config {
        /* The data transfer constants */
        struct dt_cons * dt_cons;

	ssize_t *pci_offset_table;

        /* FIXME: Left here for phase 2 */
        struct policy * unknown_flow;

        /* List of qos_cubes supported by the EFCP config */
        struct list_head qos_cubes;
};

/* Represents the configuration of the Flow Allocator */
struct fa_config {
	struct policy * ps;
	struct policy * allocate_notify;
	struct policy * allocate_retry;
	struct policy * new_flow_req;
	struct policy * seq_roll_over;
	uint32_t max_create_flow_retries;
};

/* Represents the configuration of the Resource Allocator */
struct resall_config {
	struct policy * pff_gen;
};

/* Represents the configuration of the Enrollment Task */
struct et_config {
	struct policy * ps;
};

/* A static IPC Process address*/
struct static_ipcp_addr {
	string_t * ap_name;
	string_t * ap_instance;
	uint32_t address;
};

struct static_ipcp_addr_entry {
	struct list_head next;
	struct static_ipcp_addr * entry;
};

/* An address prefix configuration*/
struct address_pref_config {
	string_t * org;
	uint32_t prefix;
};

struct address_pref_config_entry {
	struct list_head next;
	struct address_pref_config * entry;
};

/* List of known IPCP addresses and prefixes */
struct addressing_config {
	struct list_head static_ipcp_addrs;
	struct list_head address_prefixes;
};

/* Represents the configuration of the Namespace Manager */
struct nsm_config {
	struct policy * ps;
	struct addressing_config * addr_conf;
};

struct auth_sdup_profile {
	struct policy * auth;
	struct policy * encrypt;
	struct policy * crc;
	struct policy * ttl;
};

struct auth_sdup_profile_entry {
	struct list_head next;
	string_t * n1_dif_name;
	struct auth_sdup_profile * entry;
};

/* Represents the configuration of the Security Manager */
struct secman_config {
	struct policy * ps;
	struct auth_sdup_profile * default_profile;
	struct list_head specific_profiles;
};

/* Represents the configuration of the routing Task */
struct routing_config {
	struct policy * ps;
};

/* Represents the configuration of the PFF */
struct pff_config {
	/* The PS name for the PDU Forwarding Function */
	struct policy * policy_set;
};

/* Represents the configuration of the RMT */
struct rmt_config {
	/* The PS name for the RMT */
	struct policy * policy_set;

	/* The configuration of the PDU Forwarding Function subcomponent */
	struct pff_config * pff_conf;
};

struct ipcp_config_entry {
        string_t * name;
        string_t * value;
};

struct ipcp_config {
        struct list_head           next;
        struct ipcp_config_entry * entry;
};

/* Represents a DIF configuration (policies, parameters, etc) */
struct dif_config {
        /* List of configuration entries */
        struct list_head    ipcp_config_entries;

        /* the config of the efcp */
        struct efcp_config * efcp_config;

        /* the config of the rmt */
        struct rmt_config * rmt_config;

        /* The address of the IPC Process*/
        address_t           address;

        struct fa_config * fa_config;
        struct et_config * et_config;
        struct nsm_config * nsm_config;
        struct routing_config * routing_config;
        struct resall_config * resall_config;
        struct secman_config * secman_config;
};

/* Represents the information about a DIF (name, type, configuration) */
struct dif_info {
        /* The DIF type. Can be 'NORMAL' or one of the shims */
        string_t *          type;

        /* The DIF Distributed Application Name (DAN) */
        struct name *       dif_name;

        /* The DIF configuration (policies, parameters, etc) */
        struct dif_config * configuration;
};

struct name_entry {
	struct list_head next;
	struct name * entry;
};

/* Represents a neighbor of an IPCP */
struct ipcp_neighbor {
	struct name * ipcp_name;
	struct name * sup_dif_name;
	struct list_head supporting_difs;
	uint32_t address;
	uint32_t old_address;
	bool enrolled;
	uint32_t average_rtt_in_ms;
	int32_t under_port_id;
	int32_t intern_port_id;
	int32_t last_heard_time_ms;
	uint32_t num_enroll_attempts;
};

struct ipcp_neighbor_entry {
	struct list_head next;
	struct ipcp_neighbor * entry;
};

struct ipcp_neigh_list {
	struct list_head ipcp_neighbors;
};

struct rib_object_data {
        struct list_head next;
        string_t *	 clazz;
        string_t *	 name;
        string_t *	 disp_value;
        uint64_t	 instance;
};

struct query_rib_resp {
	struct list_head rib_object_data_entries;
};

/* Represents the (static) properties of a DIF */
struct dif_properties_entry {
	struct list_head next;
	struct name * dif_name;
	uint16_t max_sdu_size;
};

struct get_dif_prop_resp {
	struct list_head dif_propery_entries;
};

/** The state of a particular instance of an SDU crypto protection policy */
struct sdup_crypto_state {
	/** Enable or disable encryption crypto operations on write */
	bool enable_crypto_tx;

	/** Enable or disable encryption crypto operations on read */
	bool enable_crypto_rx;

	/** Message Authentication algorithm name */
        string_t * mac_alg;

	/** Message Authentication Key to be used for write */
	struct buffer * mac_key_tx;

	/** Message Authentication key to be used for read */
	struct buffer * mac_key_rx;

	/** Encryption algorithm name */
        string_t * enc_alg;

	/** Encryption key to be used for write */
	struct buffer * encrypt_key_tx;

	/** Encryption key to be used for read */
	struct buffer * encrypt_key_rx;

	/** Initialization vector to be used for write */
	struct buffer * iv_tx;

	/** Initialization vector to be used for read*/
	struct buffer * iv_rx;

        string_t * compress_alg;

        port_id_t port_id;
};

struct port_id_altlist {
	port_id_t *		ports;
	uint16_t		num_ports;
	struct list_head	next;
};

struct mod_pff_entry {
        address_t        fwd_info; /* dest_addr, neighbor_addr, circuit-id */
        qos_id_t         qos_id;
        uint32_t	 cost;
	struct list_head port_id_altlists;
        struct list_head next;
};

struct pff_entry_list {
	struct list_head pff_entries;
};

struct bs_info_entry {
	struct list_head next;
	int32_t signal_strength;
	string_t * ipcp_addr;
};

struct media_dif_info {
	string_t * dif_name;
	string_t * sec_policies;
	struct list_head available_bs_ipcps;
};

struct media_info_entry {
	struct list_head next;
	struct media_dif_info * entry;
	string_t * dif_name;
};

struct media_report {
	ipc_process_id_t ipcp_id;
	string_t * dif_name;
	string_t * bs_ipcp_addr;
	struct list_head available_difs;
};

struct name * rina_name_create(void);
void rina_name_free(struct name *name);
void flow_spec_free(struct flow_spec * fspec);
struct flow_spec * rina_fspec_create(void);
void policy_parm_free(struct policy_parm * prm);
struct policy_parm * policy_parm_create(void);
void policy_free(struct policy * policy);
struct policy * policy_create(void);
void dtp_config_free(struct dtp_config * dtp_config);
struct dtp_config * dtp_config_create(void);
void window_fctrl_config_free(struct window_fctrl_config * wfc);
struct window_fctrl_config * window_fctrl_config_create(void);
void rate_fctrl_config_free(struct rate_fctrl_config * rfc);
struct rate_fctrl_config * rate_fctrl_config_create(void);
void dtcp_fctrl_config_free(struct dtcp_fctrl_config * dfc);
struct dtcp_fctrl_config * dtcp_fctrl_config_create(void);
void dtcp_rxctrl_config_free(struct dtcp_rxctrl_config * rxfc);
struct dtcp_rxctrl_config * dtcp_rxctrl_config_create(void);
void dtcp_config_free(struct dtcp_config * dtcp_config);
struct dtcp_config * dtcp_config_create(void);
void pff_config_free(struct pff_config * pff);
struct pff_config * pff_config_create(void);
void rmt_config_free(struct rmt_config * rmt);
struct rmt_config * rmt_config_create(void);
void dt_cons_free(struct dt_cons * dtc);
struct dt_cons * dt_cons_create(void);
void qos_cube_free(struct qos_cube * qos);
struct qos_cube * qos_cube_create(void);
void efcp_config_free(struct efcp_config * efc);
struct efcp_config * efcp_config_create(void);
void fa_config_free(struct fa_config * fac);
struct fa_config * fa_config_create(void);
void resall_config_free(struct resall_config * resc);
struct resall_config * resall_config_create(void);
void et_config_free(struct et_config * etc);
struct et_config * et_config_create(void);
void static_ipcp_addr_free(struct static_ipcp_addr * addr);
struct static_ipcp_addr * static_ipcp_addr_create(void);
void address_pref_config_free(struct address_pref_config * apc);
struct address_pref_config * address_pref_config_create(void);
void addressing_config_free(struct addressing_config * ac);
struct addressing_config * addressing_config_create(void);
void nsm_config_free(struct nsm_config * nsmc);
struct nsm_config * nsm_config_create(void);
void auth_sdup_profile_free(struct auth_sdup_profile * asp);
struct auth_sdup_profile * auth_sdup_profile_create(void);
void secman_config_free(struct secman_config * sc);
struct secman_config * secman_config_create(void);
void routing_config_free(struct routing_config * rc);
struct routing_config * routing_config_create(void);
void ipcp_config_entry_free(struct ipcp_config_entry * ice);
struct ipcp_config_entry * ipcp_config_entry_create(void);
void dif_config_free(struct dif_config * dif_config);
struct dif_config * dif_config_create(void);
void rib_object_data_free(struct rib_object_data * rod);
struct rib_object_data * rib_object_data_create(void);
void query_rib_resp_free(struct query_rib_resp * qrr);
struct query_rib_resp * query_rib_resp_create(void);
void port_id_altlist_free(struct port_id_altlist * pia);
struct port_id_altlist * port_id_altlist_create(void);
void mod_pff_entry_free(struct mod_pff_entry * pffe);
struct mod_pff_entry * mod_pff_entry_create(void);
void pff_entry_list_free(struct pff_entry_list * pel);
struct pff_entry_list * pff_entry_list_create(void);
void sdup_crypto_state_free(struct sdup_crypto_state * scs);
struct sdup_crypto_state * sdup_crypto_state_create(void);
void dif_properties_entry_free(struct dif_properties_entry * dpe);
struct dif_properties_entry * dif_properties_entry_create(void);
void get_dif_prop_resp_free(struct get_dif_prop_resp * gdp);
struct get_dif_prop_resp * get_dif_prop_resp_create(void);
void ipcp_neighbor_free(struct ipcp_neighbor * nei);
struct ipcp_neighbor * ipcp_neighbor_create(void);
void ipcp_neigh_list_free(struct ipcp_neigh_list * nei);
struct ipcp_neigh_list * ipcp_neigh_list_create(void);
void bs_info_entry_free(struct bs_info_entry * bie);
struct bs_info_entry * bs_info_entry_create(void);
void media_dif_info_free(struct media_dif_info * mdi);
struct media_dif_info * media_dif_info_create(void);
void media_report_free(struct media_report * mre);
struct media_report * media_report_create(void);

#define IRATI_SUCC  0
#define IRATI_ERR   1

typedef uint32_t irati_msg_port_t;
typedef uint32_t irati_msg_seqn_t;
typedef uint16_t irati_msg_t;

/* All the possible messages begin like this. */
struct irati_msg_base {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;
} __attribute__((packed));

/* A simple response message layout that can be shared by many
 * different types. */
struct irati_msg_base_resp {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	int8_t result;
} __attribute__((packed));

/* Some useful macros for casting. */
#define IRATI_MB(m) (struct irati_msg_base *)(m)
#define IRATI_MBR(m) (struct irati_msg_base_resp *)(m)

/* Data structure passed along with ioctl */
struct irati_iodev_ctldata {
        uint32_t port_id;
};

/* Data structure passed along with ioctl */
struct irati_ctrldev_ctldata {
	irati_msg_port_t port_id;
};

#define IRATI_FLOW_BIND _IOW(0xAF, 0x00, struct irati_iodev_ctldata)
#define IRATI_CTRL_FLOW_BIND _IOW(0xAF, 0x01, struct irati_ctrldev_ctldata)
#define IRATI_IOCTL_MSS_GET _IOR(0xAF, 0x02, struct irati_iodev_ctldata)

#ifdef __cplusplus
}
#endif

#endif /* IRATI_KUCOMMON_H */
