/*
 * Connection
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_CONNECTION_H
#define RINA_CONNECTION_H

#include <linux/uaccess.h>

#include "common.h"

struct connection;

struct connection *  connection_create(void);
struct connection *  connection_dup_from_user(const
                                                struct connection __user * c);
int                  connection_destroy(struct connection * conn);
port_id_t            connection_port_id(const struct connection * conn);
address_t            connection_src_addr(const struct connection * conn);
address_t            connection_dst_addr(const struct connection * conn);
cep_id_t             connection_src_cep_id(const struct connection * conn);
cep_id_t             connection_dst_cep_id(const struct connection * conn);
qos_id_t             connection_qos_id(const struct connection * conn);
int                  connection_port_id_set(struct connection * conn,
                                            port_id_t           port_id);
int                  connection_src_addr_set(struct connection * conn,
                                             address_t           addr);
int                  connection_dst_addr_set(struct connection * conn,
                                             address_t           addr);
int                  connection_src_cep_id_set(struct connection * conn,
                                               cep_id_t            cep_id);
int                  connection_dst_cep_id_set(struct connection * conn,
                                               cep_id_t            cep_id);
int                  connection_qos_id_set(struct connection * conn,
                                           qos_id_t            qos_id);

#endif
