/*
 *  EFCP (Error and Flow Control Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_EFCP_H
#define RINA_EFCP_H

#include "common.h"

struct efcp_conf_t {

	/* Length of the address fields of the PCI */
	int address_length;

	/* Length of the port_id fields of the PCI */
	int port_id_length;

	/* Length of the cep_id fields of the PCI */
	int cep_id_length;

	/* Length of the qos_id field of the PCI */
	int qos_id_length;

	/* Length of the length field of the PCI */
	int length_length;

	/* Length of the sequence number fields of the PCI */
	int seq_number_length;
};

struct efcp_t {
	/* 
	 * FIXME hash_table must be added
	 * HASH_TABLE(efcp_components_per_ipc_process, ipc_process_id_t, 
	 * efcp_ipc_t *);
	 */
	
};

struct efcp_ipc_t {
	/* EFCP component configuration */
	struct efcp_conf_t *efcp_config;

	/* FIXME: This hash holds the EFCP instances of a single IPC Process 
	 * HASH_TABLE(efcp_instances, cep_id_t, efcp_instance_t *);
	 */
};

struct efcp_instance_t {
	/* The connection endpoint id that identifies this instance */
	cep_id_t                cep_id;
	
	/* The Data transfer protocol state machine instance */
	struct dtp_instance_t  *dtp_instance;
	
	/* The Data transfer control protocol state machine instance */
	struct dtcp_instance_t *dtcp_instance;
	
	/* Pointer to the flow data structure of the K-IPC Manager */
	flow_t          *flow;
};

struct dtp_instance_t {
	port_id_t                   port_id;
	struct dtcp_state_vector_t *dtcp_state_vector;
};

struct dtp_state_vector_t {

	/* Configuration values */
	struct connection_t        *connection;
	uint_t                      max_flow_sdu;
	uint_t                      max_flow_pdu_size;
	uint_t                      initial_sequence_number;
	uint_t                      seq_number_rollover_threshold;
	struct dtcp_state_vector_t *dtcp_state_vector;
	
	/* Variables: Inbound */
	seq_num_t                   last_sequence_delivered;
	
	/* Variables: Outbound */
	seq_num_t                   next_sequence_to_send;
	seq_num_t                   right_window_edge;
};

struct dtcp_instance_t {
	/*
	 * This instance of dtcp_state_vector should be the one
	 * contained into the dt_instance_vector of the
	 * associated dtp_instance_t
	 */
	struct dtcp_state_vector_t *dtcp_state_vector;

	/* FIXME : Queues must be added
	 * QUEUE(flow_control_queue, pdu_t);
	 * QUEUE(closed_window_queue, pdu_t);
	 * struct workqueue_struct * rx_control_queue;
	 */
};

struct dtcp_state_vector_t {
	/* Control state */
	uint_t max_pdu_size;
	
	/* TimeOuts */
	
	/*
	 * Time interval sender waits for a positive ack before
	 * retransmitting
	 */
	timeout_t trd;
	
	/*
	 * When flow control is rate based this timeout may be
	 * used to pace number of PDUs sent in TimeUnit
	 */
	uint_t    pdus_per_time_unit;
	
	/* DTCP sequencing */
	
	/*
	 * Outbound: NextSndCtlSeq contains the Sequence Number to
	 * be assigned to a control PDU
	 */
	seq_num_t next_snd_ctl_seq;
	
	/*
	 * Inbound: LastRcvCtlSeq - Sequence number of the next
	 * expected // Transfer(? seems an error in the spec’s
	 * doc should be Control) PDU received on this connection
	 */
	seq_num_t last_rcv_ctl_seq;
	
	/*
	 * DTCP retransmission: There’s no retransmission queue,
	 * when a lost PDU is detected a new one is generated
	 */
	
	/* Outbound */
	seq_num_t last_snd_data_ack;
	
	/*
	 * Seq number of the lowest seq number expected to be
	 * Acked. Seq number of the first PDU on the
	 * RetransmissionQ.
	 */
	seq_num_t send_left_wind_edge;
	
	/*
	 * Maximum number of retransmissions of PDUs without a
	 * positive ack before declaring an error
	 */
	uint_t    data_retransmit_max;
	
	/* Inbound */
	seq_num_t last_rcv_data_ack;
	seq_num_t rcv_left_wind_edge;
	
	/* Time (ms) over which the rate is computed */
	uint_t    time_unit;
	
	/* Flow Control State */
	
	/* Outbound */
	uint_t    sndr_credit;
	
	/* snd_rt_wind_edge = LastSendDataAck + PDU(credit) */
	seq_num_t snd_rt_wind_edge;
	
	/* PDUs per TimeUnit */
	uint_t    sndr_rate;
	
	/* PDUs already sent in this time unit */
	uint_t    pdus_sent_in_time_unit;
	
	/* Inbound */
	
	/*
	 * PDUs receiver believes sender may send before extending
	 * credit or stopping the flow on the connection
	 */
	uint_t    rcvr_credit;
	
	/* Value of credit in this flow */
	uint_t    rcvr_rt_wind_edge;
	
	/*
	 * Current rate receiver has told sender it may send PDUs
	 * at.
	 */
	uint_t    rcvr_rate;
	
	/*
	 * PDUs received in this time unit. When it equals
	 * rcvr_rate, receiver is allowed to discard any PDUs
	 * received until a new time unit begins
	 */
	uint_t    pdus_rcvd_in_time_unit;
};

int      efcp_init(void);
void     efcp_exit(void);
int      efcp_write(port_id_t           port_id,
		    const struct sdu_t *sdu);
int      efcp_receive_pdu(struct pdu_t pdu);
cep_id_t efcp_create(struct connection_t *connection);
int      efcp_destroy(cep_id_t cep_id);
int      efcp_update(cep_id_t cep_id,
		     cep_id_t dest_cep_id);

#endif
