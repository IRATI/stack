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

#include "common.h"
#include "qos.h"
#include "buffer.h"

/*
 * FIXME: This structure will be hidden soon. Do not access its field(s)
 *        directly, prefer the access functions below.
 */
struct sdu {
        struct buffer * buffer;
};

/* NOTE: The following function take the ownership of the buffer passed */
struct sdu *          sdu_create_with(struct buffer * buffer);
struct sdu *          sdu_create_with_ni(struct buffer * buffer);
int                   sdu_destroy(struct sdu * s);
const struct buffer * sdu_buffer(const struct sdu * s);
struct sdu *          sdu_dup(const struct sdu * sdu);
struct sdu *          sdu_dup_ni(const struct sdu * sdu);
bool                  sdu_is_ok(const struct sdu * sdu);
struct sdu *          sdu_protect(struct sdu * sdu);
struct sdu *          sdu_unprotect(struct sdu * sdu);

struct pci;

/* FIXME : Ugly as hell (c) Francesco. Miq */
struct sdu *          sdu_create_from(struct buffer * buffer,
                                      struct pci *    pci);

#endif
