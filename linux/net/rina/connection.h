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

/* FIXME: Move RNL structure to RNL placeholder files */
/* FIXME: More params to be added */
struct conn_p_params {
        /* FIXME: Anyone using this variable? To be removed */
        bool    dtcp_present;
        /* FIXME: Same for this one, you can AND window and rate */
        bool    flow_ctrl;
        bool    rtx_ctrl;
        /* FIXME: Rename to window_based and rate_based? */
        bool    window_based_fctrl;
        bool    rate_based_fctrl;
};

/* NOTE: Do not use this struct directly, IT MUST BE HIDDEN */
/* FIXME: Add setters/getters to struct connection*/
struct connection {
        port_id_t            port_id;

        address_t            source_address;
        address_t            destination_address;

        cep_id_t             source_cep_id;
        cep_id_t             destination_cep_id;

        qos_id_t             qos_id;

        /* FIXME: Are we sure about the next fixme? */
        /* FIXME: Add the list of policies associated with this connection */
        struct conn_p_params policies_params;
};

struct connection * connection_create(void);
struct connection * connection_dup_from_user(const
                                             struct connection __user * conn);
int                 connection_destroy(struct connection * conn);

#endif
