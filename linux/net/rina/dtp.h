/*
 * DTP (Data Transfer Protocol)
 *
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

#ifndef RINA_DTP_H
#define RINA_DTP_H

#include "common.h"

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

struct dtp {
        port_id_t                 id;

        struct dtp_state_vector * state_vector;
};

struct dtp * dtp_create(port_id_t id);
int          dtp_destroy(struct dtp * instance);

int          dtp_delimit_sdu(struct dtp * dtp,
                             struct sdu * sdu);
int          dtp_fragment_concatenate_sdu(struct dtp * dtp,
                                          struct sdu * sdu);
int          dtp_sequence_address_sdu(struct dtp * dtp,
                                      struct sdu * sdu);
int          dtp_crc_sdu(struct dtp * dtp,
                         struct sdu * sdu);
int          dtp_apply_policy_CsldWinQ(struct dtp * dtp,
                                       struct sdu * sdu);

#endif

