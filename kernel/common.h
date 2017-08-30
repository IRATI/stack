/*
 * Common definition placeholder
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef RINA_COMMON_H
#define RINA_COMMON_H

#define ETH_P_RINA      0xD1F0          /* Recursive Internet Architecture [ NOT AN OFFICIALLY REGISTERED ID ] */

#include <linux/types.h>
#include <linux/poll.h>

#include "irati/kucommon.h"

/* ALWAYS use this function to check if the id looks good */
bool      is_port_id_ok(port_id_t id);

/* ALWAYS use this function to get a bad id */
port_id_t port_id_bad(void);

/* ALWAYS use this function to check if the id looks good */
bool     is_cep_id_ok(cep_id_t id);

/* ALWAYS use this function to get a bad id */
cep_id_t cep_id_bad(void);

bool      is_address_ok(address_t address);
address_t address_bad(void);

/* ALWAYS use this function to check if the id looks good */
bool is_qos_id_ok(qos_id_t id);

/* ALWAYS use this function to get a bad id */
qos_id_t qos_id_bad(void);

/* FIXME: Move RNL related types to RNL header(s) */

typedef char          regex_t; /* FIXME: must be a string */

/* FIXME: needed for nl api */
enum rib_object_class_t {
        EMPTY,
};

#endif
