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

        /* Variables: Inbound */
        seq_num_t           last_sequence_delivered;

        /* Variables: Outbound */
        seq_num_t           next_sequence_to_send;
        seq_num_t           right_window_edge;
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
        .connection                    = NULL,
        .max_flow_sdu                  = 0,
        .max_flow_pdu_size             = 0,
        .initial_sequence_number       = 0,
        .seq_number_rollover_threshold = 0,
        .last_sequence_delivered       = 0,
        .next_sequence_to_send         = 0,
        .right_window_edge             = 0
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
                LOG_ERR("Bogus rmt, bailing out");
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

        tmp->policies      = &default_policies;
        /* FIXME: fixups to the policies should be placed here */

        tmp->peer          = NULL;
        tmp->rmt           = rmt;
        tmp->kfa           = kfa;

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

/* Closed Window Queue policy */
int apply_policy_CsldWinQ(struct dtp * dtp,
                          struct sdu * sdu)
{
        ASSERT(dtp);
        ASSERT(sdu);

        LOG_MISSING;

        return 0;
}

int apply_policy_RexmsnQ(struct dtp * dtp,
                         struct sdu * sdu)
{
        ASSERT(dtp);
        ASSERT(sdu);

        LOG_MISSING;

        return 0;
}

int dtp_write(struct dtp * instance,
              struct sdu * sdu)
{
        struct pdu * pdu;
        struct pci * pci;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!sdu) {
                LOG_ERR("No data passed, bailing out");
                return -1;
        }

        pdu = rkzalloc(sizeof(*pdu), GFP_KERNEL);
        if (!pdu) {
                LOG_ERR("Could not allocate memory for PDU");
                return -1;
        }
        pci = rkzalloc(sizeof(*pci), GFP_KERNEL);
        if (!pci) {
                LOG_ERR("Could not allocate memory for PCI");
                return -1;
        }

        /* FIXME : This is ugly as hell (c), must be removed asap */
        pdu->pci = pci;
        pci->ceps.dest_id   = instance->state_vector->
                connection->destination_cep_id;
        pci->ceps.source_id = instance->state_vector->
                connection->source_cep_id;
        pci->destination    = instance->state_vector->
                connection->destination_address;
        pci->source = instance->state_vector->connection->source_address;
        pci->sequence_number = instance->state_vector->next_sequence_to_send;
        instance->state_vector->next_sequence_to_send++;
        pci->type = PDU_TYPE_DT;
        pci->qos_id = instance->state_vector->connection->qos_id;
        pdu->buffer = sdu->buffer;
        /* Give the data to RMT now ! */
        return rmt_send(instance->rmt, pci->destination, pci->ceps.dest_id, pdu);
}

int dtp_receive(struct dtp * instance,
                struct pdu * pdu)
{
        LOG_MISSING;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        return 0;
}
