/*
 * DTCP (Data Transfer Protocol) kRPI
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

#ifndef RINA_DTCP_PS_H
#define RINA_DTCP_PS_H

#include <linux/types.h>

#include "dtcp.h"
#include "rds/rfifo.h"
#include "ps-factory.h"

struct dtcp_flowctrl_window_params {
        unsigned int max_closed_winq_length; /* in cwq */
        unsigned int initial_credit; /* to initialize sv */
};

struct dtcp_flowctrl_rate_params {
        unsigned int sending_rate;
        unsigned int time_period;
};

struct dtcp_flowctrl_params {
        bool window_based;
        struct dtcp_flowctrl_window_params window;
        bool rate_based;
        struct dtcp_flowctrl_rate_params rate;
        unsigned int sent_bytes_th;
        unsigned int sent_bytes_percent_th;
        unsigned int sent_buffers_th;
        unsigned int rcvd_bytes_th;
        unsigned int rcvd_bytes_percent_th;
        unsigned int rcvd_buffers_th;
};

struct dtcp_rtx_params {
        unsigned int max_time_retry;
        unsigned int data_retransmit_max;
        unsigned int initial_tr;
};

struct dtcp_ps {
        struct ps_base base;

        /* Behavioural policies. */
        int (* flow_init)(struct dtcp_ps * instance);
        int (* lost_control_pdu)(struct dtcp_ps * instance);
        int (* rtt_estimator)(struct dtcp_ps * instance, seq_num_t sn);
        int (* retransmission_timer_expiry)(struct dtcp_ps * instance);
        int (* received_retransmission)(struct dtcp_ps * instance);
        int (* rcvr_ack)(struct dtcp_ps * instance, const struct pci * pci);
        int (* sender_ack)(struct dtcp_ps * instance, seq_num_t seq);
        int (* sending_ack)(struct dtcp_ps * instance, seq_num_t seq);
        int (* receiving_ack_list)(struct dtcp_ps * instance);
        int (* initial_rate)(struct dtcp_ps * instance);
        int (* receiving_flow_control)(struct dtcp_ps * instance,
                                       const struct pci * pci);
        int (* update_credit)(struct dtcp_ps * instance);
        int (* rcvr_flow_control)(struct dtcp_ps * instance,
                                  const struct pci * pci);
        int (* rate_reduction)(struct dtcp_ps * instance,
        			const struct pci * pci);
        int (* rcvr_control_ack)(struct dtcp_ps * instance);
        int (* no_rate_slow_down)(struct dtcp_ps * instance);
        int (* no_override_default_peak)(struct dtcp_ps * instance);
        int (* rcvr_rendezvous)(struct dtcp_ps * instance,
        		        const struct pci * pci);

        /* Parametric policies. */
        bool flow_ctrl;
        struct dtcp_flowctrl_params flowctrl;
        bool rtx_ctrl;
        struct dtcp_rtx_params rtx;

        /* Reference used to access the DTCP data model. */
        struct dtcp * dm;

        /* Data private to the policy-set implementation. */
        void *       priv;
};

/*
 * The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary.
 */
int dtcp_ps_publish(struct ps_factory * factory);
int dtcp_ps_unpublish(const char * name);

#endif
