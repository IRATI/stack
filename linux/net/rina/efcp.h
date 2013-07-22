/*
 * EFCP (Error and Flow Control Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

#include <linux/kobject.h>

#include "common.h"
#include "kipcm.h"

struct efcp_conf {

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

struct dtp_state_vector {
        /* Configuration values */
        struct connection *        connection;
        uint_t                     max_flow_sdu;
        uint_t                     max_flow_pdu_size;
        uint_t                     initial_sequence_number;
        uint_t                     seq_number_rollover_threshold;
        struct dtcp_state_vector * dtcp_state_vector;

        /* Variables: Inbound */
        seq_num_t                  last_sequence_delivered;

        /* Variables: Outbound */
        seq_num_t                  next_sequence_to_send;
        seq_num_t                  right_window_edge;
};

struct dtp_instance {
        port_id_t                  id;
        struct dtcp_state_vector * dtcp_state_vector;
};

struct dtcp_state_vector {
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

struct dtcp_instance {
        /*
         * This instance of dtcp_state_vector should be the one
         * contained into the dt_instance_vector of the
         * associated dtp_instance_t
         */
        struct dtcp_state_vector * dtcp_state_vector;

        /* FIXME: Queues must be added
         * QUEUE(flow_control_queue, pdu);
         * QUEUE(closed_window_queue, pdu);
         * struct workqueue_struct * rx_control_queue;
         */
};

struct efcp {
        /* The connection endpoint id that identifies this instance */
        cep_id_t               id;

        /* The Data transfer protocol state machine instance */
        struct dtp_instance *  dtp_instance;

        /* The Data transfer control protocol state machine instance */
        struct dtcp_instance * dtcp_instance;

        /* Pointer to the flow data structure of the K-IPC Manager */
        struct flow *          flow;
};

struct efcp * efcp_init(struct kobject * parent);
int           efcp_fini(struct efcp * opaque);

cep_id_t      efcp_create(struct efcp *             instance,
                          const struct connection * connection);
int           efcp_destroy(struct efcp *   instance,
                           cep_id_t id);
int           efcp_update(struct efcp * instance,
                          cep_id_t      from,
                          cep_id_t      to);

int           efcp_write(struct efcp *      instance,
                         port_id_t          id,
                         const struct sdu * sdu);
int           efcp_receive_pdu(struct efcp * instance,
                               struct pdu *  pdu);

#endif
