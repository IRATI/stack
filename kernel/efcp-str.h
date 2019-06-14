/*
 * EFCP Data Structures (EFCP, Data Transfer, Data Transfer Control)
 *
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#ifndef RINA_EFCP_STR_H
#define RINA_EFCP_STR_H

#include <linux/list.h>

#include "common.h"
#include "delim.h"
#include "kfa.h"
#include "rmt.h"
#include "ps-factory.h"
#include "rds/robjects.h"

/*
 * IMAPs
 */

#define IMAP_HASH_BITS 7

struct efcp_imap {
        DECLARE_HASHTABLE(table, IMAP_HASH_BITS);
};

struct efcp_imap_entry {
        cep_id_t          key;
        struct efcp *     value;
        struct hlist_node hlist;
};

struct connection {
        port_id_t              port_id;
        address_t              source_address;
        address_t              destination_address;
        cep_id_t               source_cep_id;
        cep_id_t               destination_cep_id;
        qos_id_t               qos_id;
};

/*
 * EFCP
 */

enum efcp_state {
        EFCP_ALLOCATED = 1,
        EFCP_DEALLOCATED
};

struct efcp_container {
	struct rset *        rset;
        struct efcp_imap *   instances;
        struct cidm *        cidm;
        struct efcp_config * config;
        struct rmt *         rmt;
        struct kfa *         kfa;
        spinlock_t           lock;
	wait_queue_head_t    del_wq;
};

struct efcp {
        struct connection *     connection;
        struct ipcp_instance *  user_ipcp;
        struct dtp *            dtp;
        struct delim *		delim;
        struct efcp_container * container;
        enum efcp_state         state;
        atomic_t                pending_ops;
	struct robject          robj;
};

struct rtxq_entry {
        unsigned long    time_stamp;
        struct du *      du;
        int              retries;
        struct list_head next;
};

struct cwq {
        struct rqueue * q;
        spinlock_t      lock;
};

struct rtxqueue {
	int len;
	int drop_pdus;
        struct list_head head;
};

struct rtxq {
        spinlock_t                lock;
        struct dtp *              parent;
        struct rmt *              rmt;
        struct rtxqueue *         queue;
};

struct rtt_entry {
	unsigned long time_stamp;
	seq_num_t sn;
        struct list_head next;
};

struct rttq {
	spinlock_t lock;
	struct dtp * parent;
        struct list_head head;
};

/* This is the DT-SV part maintained by DTP */
struct dtp_sv {
        uint_t       max_flow_pdu_size;
        uint_t       max_flow_sdu_size;
        timeout_t    MPL;
        timeout_t    R;
        timeout_t    A;
        timeout_t    tr;
        seq_num_t    rcv_left_window_edge;
        bool         window_closed;
        bool         drf_flag;

        uint_t     seq_number_rollover_threshold;
	/* FIXME: we need to control rollovers...*/
	struct {
		unsigned int drop_pdus;
		unsigned int err_pdus;
		unsigned int tx_pdus;
		unsigned int tx_bytes;
		unsigned int rx_pdus;
		unsigned int rx_bytes;
	} stats;
        seq_num_t  max_seq_nr_rcv;
        seq_num_t  seq_nr_to_send;
        seq_num_t  max_seq_nr_sent;

        bool       window_based;
        bool       rexmsn_ctrl;
        bool       rate_based;
        bool       drf_required;
        bool       rate_fulfiled;
};

struct dtp {
        struct dtcp *       dtcp;
        struct efcp *       efcp;

        struct cwq *        cwq;
        struct rtxq *       rtxq;
        struct rttq * 	    rttq;

        /*
         * NOTE: The DTP State Vector is discarded only after and explicit
         *       release by the AP or by the system (if the AP crashes).
         */
        struct dtp_sv * 	sv; /* The state-vector */
        spinlock_t          sv_lock; /* The state vector lock (DTP & DTCP) */

        struct rina_component     base;
        struct dtp_config *       cfg;
        struct rmt *              rmt;
        struct squeue *           seqq;
        struct ringq *            to_post;
        struct ringq *            to_send;
        struct {
                struct timer_list sender_inactivity;
                struct timer_list receiver_inactivity;
                struct timer_list a;
                struct timer_list rate_window;
                struct timer_list rtx;
                struct timer_list rendezvous;
        } timers;
        struct robject	robj;

        spinlock_t		lock;
};

/* This is the DT-SV part maintained by DTCP */
struct dtcp_sv {
        /* TimeOuts */
        /*
         * When flow control is rate based this timeout may be
         * used to pace number of PDUs sent in TimeUnit
         */
        uint_t       pdus_per_time_unit;

        /* Sequencing */

        /*
         * Outbound: NextSndCtlSeq contains the Sequence Number to
         * be assigned to a control PDU
         */
        seq_num_t    next_snd_ctl_seq;

        /*
         * Inbound: LastRcvCtlSeq - Sequence number of the next
         * expected // Transfer(? seems an error in the spec’s
         * doc should be Control) PDU received on this connection
         */
        seq_num_t    last_rcv_ctl_seq;

        /*
         * Retransmission: There’s no retransmission queue,
         * when a lost PDU is detected a new one is generated
         */

        /* Outbound */
        seq_num_t    last_snd_data_ack;

        /*
         * Seq number of the lowest seq number expected to be
         * Acked. Seq number of the first PDU on the
         * RetransmissionQ. My LWE thus.
         */
        seq_num_t    snd_lft_win;

        /*
         * Maximum number of retransmissions of PDUs without a
         * positive ack before declaring an error
         */
        uint_t       data_retransmit_max;

        /* Inbound */
        seq_num_t    last_rcv_data_ack;

        /* Time (ms) over which the rate is computed */
        uint_t       time_unit;

        /* Flow Control State */

        /* Outbound */
        uint_t       sndr_credit;

        /* snd_rt_wind_edge = LastSendDataAck + PDU(credit) */
        seq_num_t    snd_rt_wind_edge;

        /* PDUs per TimeUnit */
        uint_t       sndr_rate;

        /* PDUs already sent in this time unit */
        uint_t       pdus_sent_in_time_unit;

        /* Inbound */

        /*
         * PDUs receiver believes sender may send before extending
         * credit or stopping the flow on the connection
         */
        uint_t       rcvr_credit;

        /* Value of credit in this flow */
        seq_num_t    rcvr_rt_wind_edge;

        /*
         * Current rate receiver has told sender it may send PDUs
         * at.
         */
        uint_t       rcvr_rate;

        /*
         * PDUs received in this time unit. When it equals
         * rcvr_rate, receiver is allowed to discard any PDUs
         * received until a new time unit begins
         */
        uint_t       pdus_rcvd_in_time_unit;

        /* Rate based both in and out-bound */

		/* Last time-instant when the credit check has been done.
		 * This is used by rate-based flow control mechanism.
		 */
		struct timespec last_time;

        /*
         * Control of duplicated control PDUs
         * */
        uint_t       acks;
        uint_t       flow_ctl;

        /* RTT estimation */
        uint_t       rtt;
        uint_t       srtt;
        uint_t       rttvar;

        /* Rendezvous */

        /* This Boolean indicates whether there is a zero-length window and a
         * Rendezvous PDU has been sent.
         */
        bool         rendezvous_sndr;

        /* This Boolean indicates whether a Rendezvous PDU was received. The
         * next DT-PDU is expected to have a DRF bit set to true.
         */
        bool         rendezvous_rcvr;
};

struct dtcp {
        struct dtp *            parent;

        /*
         * NOTE: The DTCP State Vector can be discarded during long periods of
         *       no traffic
         */
        struct dtcp_sv *       sv; /* The state-vector */
        struct rina_component  base;
        struct dtcp_config *   cfg;
        struct rmt *           rmt;
        struct timer_list 	   rendezvous_rcv;

        atomic_t               cpdus_in_transit;
        struct robject         robj;
};



#endif

