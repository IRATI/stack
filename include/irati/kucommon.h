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
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IRATI_CTRLDEV_NAME	"/dev/irati-ctrl"
#define IRATI_IODEV_NAME	"/dev/irati"

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
typedef uint          uint_t;
typedef uint          timeout_t;

struct uint_range {
        uint_t min;
        uint_t max;
};

typedef char string_t;

struct buffer {
        char * data;
        size_t size;
};

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
         * Indicates the maximum gap allowed among SDUs, a gap of N
         * SDUs is considered the same as all SDUs delivered.
         * A value of -1 indicates 'Any'
         */
        int32_t    max_allowable_gap;

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

struct ipcp_config_entry {
        string_t * name;
        string_t * value;
};

struct ipcp_config {
        struct list_head           next;
        struct ipcp_config_entry * entry;
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
        bool      dif_integrity;
};

struct dt_cons * dt_cons_dup(const struct dt_cons * dt_cons);

/* Represents the configuration of the EFCP */
struct efcp_config {
        /* The data transfer constants */
        struct dt_cons * dt_cons;

	ssize_t *pci_offset_table;

        /* FIXME: Left here for phase 2 */
        struct policy * unknown_flow;
};

struct dup_config_entry {
	// The N-1 dif_name this configuration applies to
	string_t * 	n_1_dif_name;

	// If NULL TTL is disabled,
	// otherwise contains the TTL policy data
	struct policy * ttl_policy;

	// if NULL error_check is disabled,
	// otherwise contains the error check policy
	// data
	struct policy * error_check_policy;

	//Cryptographic-related fields
	struct policy * crypto_policy;
};

struct dup_config {
	struct list_head          next;
	struct dup_config_entry * entry;
};

/* Represents the configuration of the SDUProtection module */
struct sdup_config {
	struct dup_config_entry * default_dup_conf;
	struct list_head	  specific_dup_confs;
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

        /* List of Data Unit Protection configuration entries */
        struct sdup_config * sdup_config;
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

struct rib_object_data {
        struct list_head next;
        string_t *	 class;
        string_t *	 name;
        string_t *	 disp_value;
        uint64_t	 instance;
};

struct query_rib_resp {
	struct list_head rib_object_data_entries;
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
};

struct port_id_altlist {
	port_id_t *		ports;
	uint16_t		num_ports;
	struct list_head	next;
};

struct mod_pff_entry {
        address_t        fwd_info; /* dest_addr, neighbor_addr, circuit-id */
        qos_id_t         qos_id;
	struct list_head port_id_altlists;
        struct list_head next;
};

struct pff_entry_list {
	struct list_head pff_entries;
};

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

	uint8_t result;
} __attribute__((packed));

/* Some useful macros for casting. */
#define IRATI_MB(m) (struct irati_msg_base *)(m)
#define IRATI_MBR(m) (struct irati_msg_base_resp *)(m)

#ifdef __cplusplus
}
#endif

#endif /* IRATI_KUCOMMON_H */
