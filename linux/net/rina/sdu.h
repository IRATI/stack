/*
 * Service Data Unit
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#ifndef RINA_SDU_H
#define RINA_SDU_H

#include <linux/types.h>
#include <linux/list.h>

#include "common.h"
#include "buffer.h"

/*
 * FIXME: This structure will be hidden soon. Do not access its field(s)
 *        directly, prefer the access functions below.
 */
struct sdu {
        struct buffer * buffer;
        struct list_head node;
};

/*
 * Represents and SDU with the port-id the SDU is to be written to
 * or has been read from OR the destination address
 */
struct sdu_wpi {
        struct sdu * sdu;
        address_t    dst_addr;
        port_id_t    port_id;
};

struct pdu;

/* NOTE: The following functions take the ownership of the buffer passed */
struct sdu *          sdu_create_buffer_with(struct buffer * buffer);
struct sdu *          sdu_create_buffer_with_ni(struct buffer * buffer);

/* FIXME: To be removed after ser/des */
struct sdu *          sdu_create_pdu_with(struct pdu * pdu);
struct sdu *          sdu_create_pdu_with_ni(struct pdu * pdu);

int                   sdu_destroy(struct sdu * s);
const struct buffer * sdu_buffer_ro(const struct sdu * s);
struct buffer *       sdu_buffer_rw(struct sdu * s);
struct sdu *          sdu_dup(const struct sdu * sdu);
struct sdu *          sdu_dup_ni(const struct sdu * sdu);
bool                  sdu_is_ok(const struct sdu * sdu);
int                   sdu_buffer_disown(struct sdu * sdu);

struct sdu_wpi *      sdu_wpi_create_with(struct buffer * buffer);
struct sdu_wpi *      sdu_wpi_create_with_ni(struct buffer * buffer);
int                   sdu_wpi_destroy(struct sdu_wpi * s);
bool                  sdu_wpi_is_ok(const struct sdu_wpi * s);

#endif
