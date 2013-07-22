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

        /* Variables: Inbound */
        seq_num_t                  last_sequence_delivered;

        /* Variables: Outbound */
        seq_num_t                  next_sequence_to_send;
        seq_num_t                  right_window_edge;

        /* The companion DTCP state vector */
        struct dtcp_state_vector * dtcp_state_vector;
};

#if 0
struct dtp_policies {
        int (* UnknownPortPolicy)(struct dtp * dtp);
        int (* CsldWinQ)(struct dtp * dtp);
};
#endif

struct dtp {
        /* FIXME: Is port-id really needed here ??? */
        port_id_t                 id;

        struct dtp_state_vector * state_vector;
#if 0
        struct dtp_policies       policies;
#endif
};

struct dtp * dtp_create(port_id_t id);
int          dtp_destroy(struct dtp * instance);

int          dtp_state_vector_bind(struct dtp *               instance,
                                   struct dtcp_state_vector * state_vector);
int          dtp_state_vector_unbind(struct dtp * instance);

/* Used by the higher level component to send PDUs to RMT */
int          dtp_send(struct dtp *       dtp,
                      const struct sdu * sdu);

/* Used by the RMT to let DTP receive PDUs */
int          dtp_receive(struct dtp * dtp,
                         struct pdu * pdu);

#endif

