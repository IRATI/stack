/*
 * Common utilities
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

#include <linux/module.h>

#define RINA_PREFIX "common"

#include "logs.h"
#include "common.h"

#define PORT_ID_WRONG -1

port_id_t port_id_bad(void)
{ return PORT_ID_WRONG; }
EXPORT_SYMBOL(port_id_bad);

int is_port_id_ok(port_id_t id)
{ return id >= 0 ? 1 : 0; }
EXPORT_SYMBOL(is_port_id_ok);

#define CEP_ID_WRONG -1

int is_cep_id_ok(cep_id_t id)
{ return 1; /* FIXME: Bummer, add it */ }
EXPORT_SYMBOL(is_cep_id_ok);

cep_id_t cep_id_bad(void)
{ return CEP_ID_WRONG; }
EXPORT_SYMBOL(cep_id_bad);
