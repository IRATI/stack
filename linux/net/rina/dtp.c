/*
 * DTP (Data Transfer Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#define RINA_PREFIX "dtp"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtp.h"

struct dtp_sv {
        /* Configuration values */
        struct connection * connection; /* FIXME: Are we really sure ??? */
        uint_t              max_flow_sdu;
        uint_t              max_flow_pdu_size;
        uint_t              initial_sequence_number;
        uint_t              seq_number_rollover_threshold;

        struct {
                seq_num_t   last_sequence_delivered;
        } inbound;

        struct {
                seq_num_t   next_sequence_to_send;
                seq_num_t   right_window_edge;
        } outbound;
};

struct dtp_policies {
        int (* xxx_fixme_add_policies_here)(struct dtp * instance);
};

struct dtp {
        /* NOTE: The DTP State Vector cannot be discarded */
        struct dtp_sv *       state_vector;
        struct dtp_policies * policies;
        struct dtcp *         peer;
        struct rmt *          rmt;
        struct kfa *          kfa;
};

static struct dtp_sv default_sv = {
        .connection                      = NULL,
        .max_flow_sdu                    = 0,
        .max_flow_pdu_size               = 0,
        .initial_sequence_number         = 0,
        .seq_number_rollover_threshold   = 0,
        .inbound.last_sequence_delivered = 0,
        .outbound.next_sequence_to_send  = 0,
        .outbound.right_window_edge      = 0
};

static struct dtp_policies default_policies = {
        .xxx_fixme_add_policies_here = NULL
};

struct dtp * dtp_create(struct rmt *        rmt,
                        struct kfa *        kfa,
                        struct connection * connection)
{
        struct dtp * tmp;

        if (!rmt) {
                LOG_ERR("No rmt, bailing out");
                return NULL;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp) {
                LOG_ERR("Cannot create DTP instance");
                return NULL;
        }

        tmp->state_vector = rkmalloc(sizeof(*tmp->state_vector), GFP_KERNEL);
        if (!tmp->state_vector) {
                LOG_ERR("Cannot create DTP state-vector");

                rkfree(tmp);
                return NULL;
        }

        *tmp->state_vector            = default_sv;

        /* FIXME: fixups to the state-vector should be placed here */
        tmp->state_vector->connection = connection;

        tmp->policies                 = &default_policies;
        /* FIXME: fixups to the policies should be placed here */

        tmp->peer                     = NULL;
        tmp->rmt                      = rmt;
        tmp->kfa                      = kfa;

        LOG_DBG("Instance %pK created successfully", tmp);

        return tmp;
}

int dtp_destroy(struct dtp * instance)
{
        if (!instance) {
                LOG_ERR("Bad instance passed, bailing out");
                return -1;
        }

        /* NOTE: The DTP State Vector cannot be discarded */
        ASSERT(instance->state_vector);

        rkfree(instance->state_vector);
        rkfree(instance);

        /*
         * FIXME: RMT is destroyed by EFCP Container, should be fine
         *        but better check it ...
         */

        LOG_DBG("Instance %pK destroyed successfully", instance);

        return 0;
}

int dtp_bind(struct dtp *  instance,
             struct dtcp * peer)
{
        if (!instance) {
                LOG_ERR("Bad instance passed, bailing out");
                return -1;
        }
        if (!peer) {
                LOG_ERR("Bad peer passed, bailing out");
                return -1;
        }

        if (instance->peer) {
                if (instance->peer != peer) {
                        LOG_ERR("This instance is already bound to "
                                "a different peer, unbind it first");
                        return -1;
                }

                LOG_DBG("This instance is already bound to the same peer ...");
                return 0;
        }

        instance->peer = peer;

        return 0;
}

int dtp_unbind(struct dtp * instance)
{
        if (!instance) {
                LOG_ERR("Bad instance passed, bailing out");
                return -1;
        }

        instance->peer = NULL;

        return 0;

}

#if 0
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
        struct pdu *    pdu;
        struct pci *    pci;
        struct dtp_sv * sv;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (!sdu_is_ok(sdu))
                return -1;

        sv = instance->state_vector;
        ASSERT(sv); /* State Vector must not be NULL */

        pci = pci_create();
        if (!pci)
                return -1;

        if (pci_format(pci,
                       sv->connection->source_cep_id,
                       sv->connection->destination_cep_id,
                       sv->connection->source_address,
                       sv->connection->destination_address,
                       sv->outbound.next_sequence_to_send,
                       sv->connection->qos_id,
                       PDU_TYPE_DT)) {
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
                return -1;
        }

        if (pdu_pci_set(pdu, pci)) {
                pci_destroy(pci);
                return -1;
        }

        /* Give the data to RMT now ! */
        return rmt_send(instance->rmt,
                        pci_destination(pci),
                        pci_qos_id(pci),
                        pdu);
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
        return rmt_send(rmt,
                        pci_destination(pci),
                        pci_cep_destination(pci),
                        pdu);

};
EXPORT_SYMBOL(dtp_mgmt_write);

int dtp_receive(struct dtp * instance,
                struct pdu * pdu)
{
        struct sdu *    sdu;
        struct buffer * buffer;

        if (!instance                            ||
            !instance->kfa                       ||
            !instance->state_vector              ||
            !instance->state_vector->connection) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("Bogus data, bailing out");
                return -1;
        }

        buffer = pdu_buffer_get_rw(pdu);
        sdu    = sdu_create_buffer_with(buffer);
        if (!sdu) {
                pdu_destroy(pdu);
                return -1;
        }

        if (kfa_sdu_post(instance->kfa,
                         instance->state_vector->connection->port_id,
                         sdu)) {
                LOG_ERR("Could not post SDU to KFA");
                pdu_destroy(pdu);
                return -1;
        }

        /*
         * FIXME: PDU is now useless, it must be destroyed, but its
         * buffer is within the passed sdu, so pdu_destroy can't be used.
         */

        return 0;
}
