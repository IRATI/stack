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

#include <linux/export.h>
#include <linux/types.h>

#define RINA_PREFIX "common"

#include "logs.h"
#include "common.h"

port_id_t port_id_bad(void)
{ return PORT_ID_WRONG; }
EXPORT_SYMBOL(port_id_bad);

bool is_port_id_ok(port_id_t id)
{ return id >= 0 ? true : false; }
EXPORT_SYMBOL(is_port_id_ok);

bool is_cep_id_ok(cep_id_t id)
{ return id >= 0 ? true : false; }
EXPORT_SYMBOL(is_cep_id_ok);

cep_id_t cep_id_bad(void)
{ return CEP_ID_WRONG; }
EXPORT_SYMBOL(cep_id_bad);

bool is_address_ok(address_t address)
{ return address != ADDRESS_WRONG ? true : false; }
EXPORT_SYMBOL(is_address_ok);

address_t address_bad(void)
{ return ADDRESS_WRONG; }
EXPORT_SYMBOL(address_bad);

qos_id_t qos_id_bad(void)
{ return QOS_ID_WRONG; }
EXPORT_SYMBOL(qos_id_bad);

bool is_qos_id_ok(qos_id_t id)
{ return id != QOS_ID_WRONG ? true : false; }
EXPORT_SYMBOL(is_qos_id_ok);
