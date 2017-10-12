/*
 * DTP (Data Transfer Protocol) kRPI
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#ifndef RINA_DTP_PS_H
#define RINA_DTP_PS_H

#include <linux/types.h>

#include "dtp.h"
#include "du.h"
#include "rds/rfifo.h"
#include "ps-factory.h"

struct dtp_ps {
        struct ps_base base;

        /* Behavioural policies. */
        int (* transmission_control)(struct dtp_ps * ps,
                                     struct du * du);
        int (* closed_window)(struct dtp_ps * instance,
                              struct du * du);
        int (* rcv_flow_control_overrun)(struct dtp_ps * ps,
                                         struct du * du);
        int (* snd_flow_control_overrun)(struct dtp_ps * ps,
                                         struct du * du);
        int (* initial_sequence_number)(struct dtp_ps * ps);
        int (* receiver_inactivity_timer)(struct dtp_ps * ps);
        int (* sender_inactivity_timer)(struct dtp_ps * ps);
        bool (* reconcile_flow_conflict)(struct dtp_ps * ps);

        /* Parametric policies. */
        bool            dtcp_present;
        int             seq_num_ro_th; /* Sequence number rollover threshold */
        timeout_t       initial_a_timer;
        bool            partial_delivery;
        bool            incomplete_delivery;
        bool            in_order_delivery;
        seq_num_t       max_sdu_gap;

        /* Reference used to access the DTP data model. */
        struct dtp * dm;

        /* Data private to the policy-set implementation. */
        void *       priv;
};

/*
 * The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary.
 */
int dtp_ps_publish(struct ps_factory * factory);
int dtp_ps_unpublish(const char * name);

#endif
