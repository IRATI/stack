/*
 * DTP (Data Transfer Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#define RINA_PREFIX "dtp"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtp.h"
#include "dt.h"
#include "dt-utils.h"

enum flow_state {
        FLOW_NULL = 1,
        FLOW_ACTIVE,
        FLOW_PORT_ID_TRANSITION
};

/* This is the DT-SV part maintained by DTP */
struct dtp_sv {
        /* Configuration values */
        struct connection * connection; /* FIXME: Are we really sure ??? */
        uint_t              max_flow_sdu_size;
        uint_t              max_flow_pdu_size;
        uint_t              seq_number_rollover_threshold;
        bool                window_based;
        bool                window_closed;
        uint_t              max_cwq_len;
        int                 rexmsn_ctrl;
        enum flow_state     state;

        struct {
                seq_num_t   left_window_edge;
        } inbound;

        struct {
                seq_num_t   next_sequence_to_send;
                seq_num_t   right_window_edge;
        } outbound;
};

/* FIXME: Has to be rearranged */
struct dtp_policies {
        int (* transmission_control)(struct dtp * instance,
                                     struct sdu * sdu);
        int (* flow_control_overrun)(struct dtp * instance,
                                     struct sdu * sdu);
        int (* unknown_flow)(struct dtp * instance,
                             struct pdu * pdu);
        int (* initial_sequence_number)(struct dtp * instance);
        int (* receiver_inactivity_timer)(struct dtp * instance);
        int (* sender_inactivity_timer)(struct dtp * instance);
};

/* FIXME: Should probably be kept in a different component */
#define TIME_MPL 100 /* FIXME: Completely bogus value, must be in ms */
#define TIME_R   200 /* FIXME: Completely bogus value, must be in ms */
#define TIME_A   300 /* FIXME: Completely bogus value, must be in ms */
struct dtp {
        struct dt *           parent;
        /*
         * NOTE: The DTP State Vector is discarded only after and explicit
         *       release by the AP or by the system (if the AP crashes).
         */
        struct dtp_sv *       sv; /* The state-vector */

        struct dtp_policies * policies;
        struct rmt *          rmt;
        struct kfa *          kfa;

        struct {
                struct rtimer * sender_inactivity;
                struct rtimer * receiver_inactivity;
                struct rtimer * a;
        } timers;

};

static struct dtp_sv default_sv = {
        .connection                      = NULL,
        .max_flow_sdu_size               = 0,
        .max_flow_pdu_size               = 0,
        .seq_number_rollover_threshold   = 0,
        .window_based                    = false,
        .window_closed                   = false,
        .rexmsn_ctrl                     = false,
        .state                           = FLOW_NULL,
        .max_cwq_len                     = 0,
        .inbound.left_window_edge        = 0,
        .outbound.next_sequence_to_send  = 0,
        .outbound.right_window_edge      = 0
};

static struct dtp_policies default_policies = {
        .transmission_control      = NULL,
        .flow_control_overrun      = NULL,
        .unknown_flow              = NULL,
        .initial_sequence_number   = NULL,
        .receiver_inactivity_timer = NULL,
        .sender_inactivity_timer   = NULL,
};

static void tf_sender_inactivity(void * data)
{ /* Runs the SenderInactivityTimerPolicy */ }

static void tf_receiver_inactivity(void * data)
{ /* Runs the ReceiverInactivityTimerPolicy */ }

static void tf_a(void * data)
{ }

struct dtp * dtp_create(struct dt *         dt,
                        struct rmt *        rmt,
                        struct kfa *        kfa,
                        struct connection * connection)
{
        struct dtp * tmp;

        if (!dt) {
                LOG_ERR("No DT passed, bailing out");
                return NULL;
        }

        if (!rmt) {
                LOG_ERR("No RMT passed, bailing out");
                return NULL;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp) {
                LOG_ERR("Cannot create DTP instance");
                return NULL;
        }

        tmp->parent = dt;

        tmp->sv = rkmalloc(sizeof(*tmp->sv), GFP_KERNEL);
        if (!tmp->sv) {
                LOG_ERR("Cannot create DTP state-vector");
                rkfree(tmp);
                return NULL;
        }
        *tmp->sv            = default_sv;
        /* FIXME: fixups to the state-vector should be placed here */

        tmp->sv->connection   = connection;

        tmp->policies       = &default_policies;
        /* FIXME: fixups to the policies should be placed here */

        tmp->rmt            = rmt;
        tmp->kfa            = kfa;

        tmp->timers.sender_inactivity   = rtimer_create(tf_sender_inactivity,
                                                        tmp);
        tmp->timers.receiver_inactivity = rtimer_create(tf_receiver_inactivity,
                                                        tmp);
        tmp->timers.a                   = rtimer_create(tf_a, tmp);
        if (!tmp->timers.sender_inactivity   ||
            !tmp->timers.receiver_inactivity ||
            !tmp->timers.a) {
                dtp_destroy(tmp);
                return NULL;
        }

        LOG_DBG("Instance %pK created successfully", tmp);

        return tmp;
}

int dtp_destroy(struct dtp * instance)
{
        if (!instance) {
                LOG_ERR("Bad instance passed, bailing out");
                return -1;
        }

        if (instance->timers.sender_inactivity)
                rtimer_destroy(instance->timers.sender_inactivity);
        if (instance->timers.receiver_inactivity)
                rtimer_destroy(instance->timers.receiver_inactivity);
        if (instance->timers.a)
                rtimer_destroy(instance->timers.a);

        if (instance->sv)
                rkfree(instance->sv);
        rkfree(instance);

        LOG_DBG("Instance %pK destroyed successfully", instance);

        return 0;
}

#if 0
/* FIXME: Seems unneeded? */
static int apply_policy_CsldWinQ(struct dtp * dtp,
                                 struct sdu * sdu)
{
        ASSERT(dtp);
        ASSERT(sdu);

        LOG_MISSING;

        return 0;
}

static int apply_policy_RexmsnQ(struct dtp * dtp,
                                struct sdu * sdu)
{
        ASSERT(dtp);
        ASSERT(sdu);

        LOG_MISSING;

        return 0;
}
#endif

int dtp_write(struct dtp * instance,
              struct sdu * sdu)
{
        struct pdu *          pdu;
        struct pci *          pci;
        struct dtp_sv *       sv;
        struct dt *           dt;
        struct cwq *          cwq;
        struct rtxq *         rtxq;
        int                   ret;
        struct pdu *          cpdu;
        struct dtp_policies * policies;

        if (!sdu_is_ok(sdu))
                return -1;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                sdu_destroy(sdu);
                return -1;
        }

        /* Stop SenderInactivityTimer */
        if (rtimer_stop(instance->timers.sender_inactivity)) {
                LOG_ERR("Failed to stop timer");
                sdu_destroy(sdu);
                return -1;
        }

        sv = instance->sv;
        ASSERT(sv); /* State Vector must not be NULL */

        dt = instance->parent;
        ASSERT(dt);

        policies = instance->policies;
        ASSERT(policies);

        pci = pci_create();
        if (!pci) {
                sdu_destroy(sdu);
                return -1;
        }

        if (pci_format(pci,
                       sv->connection->source_cep_id,
                       sv->connection->destination_cep_id,
                       sv->connection->source_address,
                       sv->connection->destination_address,
                       sv->outbound.next_sequence_to_send,
                       sv->connection->qos_id,
                       PDU_TYPE_DT)) {
                pci_destroy(pci);
                sdu_destroy(sdu);
                return -1;
        }

        pdu = pdu_create();
        if (!pdu) {
                pci_destroy(pci);
                sdu_destroy(sdu);
                return -1;
        }

        if (pdu_buffer_set(pdu, sdu_buffer_rw(sdu))) {
                pci_destroy(pci);
                sdu_destroy(sdu);
                return -1;
        }

        if (pdu_pci_set(pdu, pci)) {
                sdu_buffer_disown(sdu);
                pdu_destroy(pdu);
                sdu_destroy(sdu);
                pci_destroy(pci);
                return -1;
        }

        sdu_buffer_disown(sdu);
        sdu_destroy(sdu);

        /*
         * Incrementing here means the PDU cannot
         * be just thrown away from this point onwards
         */
        /* Probably needs to be revised */
        sv->outbound.next_sequence_to_send++;

        /* Step 1: Sequencing */
        /* Step 2: Protection */
        /* Step 2: Delimiting (fragmentation/reassembly) */


        if (sv->window_based) {
                if (!sv->window_closed &&
                    pci_sequence_number_get(pci) <
                    /* Should probably be kept in DTCP SV*/
                    sv->outbound.right_window_edge) {
                        /*
                         * Might close window
                         */
                        policies->transmission_control(instance, sdu);
                } else {
                        cwq = dt_cwq(dt);
                        if (!cwq) {
                                LOG_ERR("Failed to get cwq");
                                pdu_destroy(pdu);
                                return -1;
                        }

                        if (cwq_size(cwq) < sv->max_cwq_len-1) {
                                if (cwq_push(cwq, pdu)) {
                                        LOG_ERR("Failed to push to cwq");
                                        pdu_destroy(pdu);
                                        return -1;
                                }
                                return 0;
                        } else {
                                policies->flow_control_overrun(instance, sdu);
                                return 0;
                        }
                }
        }

        if (sv->rexmsn_ctrl) {
                /* FIXME: Add timer for PDU */
                rtxq = dt_rtxq(dt);
                if (!rtxq) {
                        LOG_ERR("Failed to get rtxq");
                        pdu_destroy(pdu);
                        return -1;
                }

                cpdu = pdu_dup(pdu);
                if (!cpdu) {
                        LOG_ERR("Failed to copy PDU");
                        pdu_destroy(pdu);
                        return -1;
                }

                if (rtxq_push(rtxq, cpdu)) {
                        LOG_ERR("Couldn't push to rtxq");
                        pdu_destroy(pdu);
                        return -1;
                }
        }


        /* Post SDU to RMT */
        ret = rmt_send(instance->rmt,
                       pci_destination(pci),
                       pci_qos_id(pci),
                       pdu);

        if (rtimer_start(instance->timers.sender_inactivity, 
                         2*(TIME_MPL+TIME_R+TIME_A))) {
                LOG_ERR("Failed to start timer");
                return -1;
        }
        
        return ret;
}

int dtp_mgmt_write(struct rmt * rmt,
                   address_t    src_address,
                   port_id_t    port_id,
                   struct sdu * sdu)
{
        struct pci * pci;
        struct pdu * pdu;
        address_t    dst_address;

        /*
         * NOTE:
         *   DTP should build the PCI header src and dst cep_ids = 0
         *   ask FT for the dst address the N-1 port is connected to
         *   pass to the rmt
         */

        if (!sdu) {
                LOG_ERR("No data passed, bailing out");
                return -1;
        }

        dst_address = 0; /* FIXME: get from PFT */

        /*
         * FIXME:
         *   We should avoid to create a PCI only to have its fields to use
         */
        pci = pci_create();
        if (!pci)
                return -1;

        if (pci_format(pci,
                       0,
                       0,
                       src_address,
                       dst_address,
                       0,
                       0,
                       PDU_TYPE_MGMT)) {
                pci_destroy(pci);
                return -1;
        }

        pdu = pdu_create();
        if (!pdu) {
                pci_destroy(pci);
                return -1;
        }

        if (pdu_buffer_set(pdu, sdu_buffer_rw(sdu))) {
                pci_destroy(pci);
                pdu_destroy(pdu);
                return -1;
        }

        if (pdu_pci_set(pdu, pci)) {
                pci_destroy(pci);
                return -1;
        }

        /* Give the data to RMT now ! */

        /* FIXME: What about sequencing (and all the other procedures) ? */
        return rmt_send(rmt,
                        pci_destination(pci),
                        pci_cep_destination(pci),
                        pdu);

};
EXPORT_SYMBOL(dtp_mgmt_write);

int dtp_receive(struct dtp * instance,
                struct pdu * pdu)
{
        struct sdu *          sdu;
        struct buffer *       buffer;
        struct dtp_policies * policies;
        struct pci *          pci;
        struct dtp_sv *       sv;

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("Bogus data, bailing out");
                return -1;
        }

        if (!instance                  ||
            !instance->kfa             ||
            !instance->sv              ||
            !instance->sv->connection) {
                LOG_ERR("Bogus instance passed, bailing out");
                pdu_destroy(pdu);
                return -1;
        }

        policies = instance->policies;
        ASSERT(policies);

        sv = instance->sv;
        ASSERT(sv); /* State Vector must not be NULL */

        if (sv->state == FLOW_NULL) {
                policies->unknown_flow(instance, pdu);
                return 0;
        }

        
        if (!pdu_pci_present(pdu)) {
                LOG_DBG("Couldn't find PCI in PDU");
                pdu_destroy(pdu);
                return -1;
        }
        pci = pdu_pci_get_rw(pdu);

        /* Stop ReceiverInactivityTimer */
        if (rtimer_stop(instance->timers.receiver_inactivity)) {
                LOG_ERR("Failed to stop timer");
                pdu_destroy(pdu);
                return -1;
        }


        if (!(pci_flags_get(pci) ^ PDU_FLAGS_DATA_RUN)) {
                
        }



        buffer = pdu_buffer_get_rw(pdu);
        sdu    = sdu_create_buffer_with(buffer);
        if (!sdu) {
                pdu_destroy(pdu);
                return -1;
        }

        ASSERT(instance->sv);

        if (kfa_sdu_post(instance->kfa,
                         instance->sv->connection->port_id,
                         sdu)) {
                LOG_ERR("Could not post SDU to KFA");
                pdu_destroy(pdu);
                return -1;
        }

        pdu_buffer_disown(pdu);
        pdu_destroy(pdu);

        return 0;
}

int dtp_rcv_flow_ctl(struct dtp * instance)
{
        LOG_MISSING;
        return 0;
}
EXPORT_SYMBOL(dtp_rcv_flow_ctl);
