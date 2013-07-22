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
//#include "kipcm.h"
#include "dtp.h"
#include "dtcp.h"

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

struct efcp {
        /* The connection endpoint id that identifies this instance */
        cep_id_t      id;

        /* The Data transfer protocol state machine instance */
        struct dtp *  dtp;

        /* The Data transfer control protocol state machine instance */
        struct dtcp * dtcp;

        /* Pointer to the flow data structure of the K-IPC Manager */
        struct flow * flow;
};

struct efcp * efcp_init(struct kobject * parent);
int           efcp_fini(struct efcp * instance);

int           efcp_create(struct efcp *             instance,
                          const struct connection * connection,
                          cep_id_t *                id);
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
