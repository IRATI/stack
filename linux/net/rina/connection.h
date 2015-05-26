/*
 * Connection
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
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

#ifndef RINA_CONNECTION_H
#define RINA_CONNECTION_H

#include <linux/uaccess.h>

#include "common.h"
#include "qos.h"

struct dtcp_config;

/* FIXME: Move RNL structure to RNL placeholder files */
/* FIXME: More params to be added */
struct conn_policies {
        /* FIXME: Anyone using this variable? To be removed */
        bool                 dtcp_present;
        struct dtcp_config * dtcp_cfg;

        /* Policy sets configuration */
        struct policy * dtp_ps;
        struct policy * dtcp_ps;

        /* FIXME The following three "policies" are unused, useless
         * and duplicated - they already exist as function pointers
         * (dtp-ps.h). They have to be removed here, and therefore
         * from netlink (kernespace+userspace) and from librina. */
        struct policy *      initial_sequence_number;
        struct policy *      receiver_inactivity_timer;
        struct policy *      sender_inactivity_timer;
        /* Sequence number rollover threshold */
        int                  seq_num_ro_th;
        timeout_t            initial_a_timer;
        bool                 partial_delivery;
        bool                 incomplete_delivery;
        bool                 in_order_delivery;
        seq_num_t            max_sdu_gap;
};

/* NOTE: Do not use this struct directly, IT MUST BE HIDDEN */
/* FIXME: Add setters/getters to struct connection*/
struct connection {
        port_id_t              port_id;

        address_t              source_address;
        address_t              destination_address;

        cep_id_t               source_cep_id;
        cep_id_t               destination_cep_id;

        qos_id_t               qos_id;

        /* FIXME: Are we sure about the next fixme? */
        /* FIXME: Add the list of policies associated with this connection */
        struct conn_policies * policies_params;
};

struct conn_policies * conn_policies_create(void);
struct connection *    connection_create(void);
struct connection *    connection_dup_from_user(const
                                                struct connection __user * c);
int                    conn_policies_destroy(struct conn_policies * cp_params);
int                    connection_destroy(struct connection * conn);

#endif
