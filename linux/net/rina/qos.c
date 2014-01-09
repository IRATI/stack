/*
 * QoS
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

#define RINA_PREFIX "qos"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "qos.h"

#define QOS_ID_WRONG 0

qos_id_t qos_id_bad(void)
{ return QOS_ID_WRONG; }
EXPORT_SYMBOL(qos_id_bad);

int is_qos_id_ok(qos_id_t id)
{ return id != QOS_ID_WRONG ? 1 : 0; }
EXQOS_SYMBOL(is_qos_id_ok);
