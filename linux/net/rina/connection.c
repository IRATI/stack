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

#include <linux/export.h>
#include <linux/types.h>

#define RINA_PREFIX "connection"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "common.h"
#include "connection.h"
#include "dtp-conf-utils.h"
#include "dtcp-conf-utils.h"
#include "policies.h"

struct connection {
        port_id_t              port_id;

        address_t              source_address;
        address_t              destination_address;

        cep_id_t               source_cep_id;
        cep_id_t               destination_cep_id;

        qos_id_t               qos_id;
};

struct connection * connection_create(void)
{
        struct connection * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}
EXPORT_SYMBOL(connection_create);

struct connection *
connection_dup_from_user(const struct connection __user * conn)
{
        struct connection * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        if (copy_from_user(tmp, conn, sizeof(*tmp)))
                return NULL;

        return tmp;
}

int connection_destroy(struct connection * conn)
{
        if (!conn)
                return -1;
        rkfree(conn);
        return 0;
}
EXPORT_SYMBOL(connection_destroy);

port_id_t connection_port_id(const struct connection * conn)
{
        if (!conn)
                return port_id_bad();
        return conn->port_id;
}
EXPORT_SYMBOL(connection_port_id);

address_t connection_src_addr(const struct connection * conn)
{
        if (!conn)
                return address_bad();
        return conn->source_address;
}
EXPORT_SYMBOL(connection_src_addr);

address_t connection_dst_addr(const struct connection * conn)
{
        if (!conn)
                return address_bad();
        return conn->destination_address;
}
EXPORT_SYMBOL(connection_dst_addr);

cep_id_t connection_src_cep_id(const struct connection * conn)
{
        if (!conn)
                return cep_id_bad();
        return conn->source_cep_id;
}
EXPORT_SYMBOL(connection_src_cep_id);

cep_id_t connection_dst_cep_id(const struct connection * conn)
{
        if (!conn)
                return cep_id_bad();
        return conn->destination_cep_id;
}
EXPORT_SYMBOL(connection_dst_cep_id);

qos_id_t connection_qos_id(const struct connection * conn)
{
        if (!conn)
                return qos_id_bad();
        return conn->qos_id;
}
EXPORT_SYMBOL(connection_qos_id);

int connection_port_id_set(struct connection * conn,
                           port_id_t           port_id)
{
        if (!conn || !(is_port_id_ok(port_id))) return -1;
        conn->port_id = port_id;
        return 0;
}
EXPORT_SYMBOL(connection_port_id_set);

int connection_src_addr_set(struct connection * conn,
                            address_t           addr)
{
        if (!conn || !(is_address_ok(addr))) return -1;
        conn->source_address = addr;
        return 0;
}
EXPORT_SYMBOL(connection_src_addr_set);

int connection_dst_addr_set(struct connection * conn,
                            address_t           addr)
{
        if (!conn || !(is_address_ok(addr))) return -1;
        conn->destination_address = addr;
        return 0;
}
EXPORT_SYMBOL(connection_dst_addr_set);

int connection_src_cep_id_set(struct connection * conn,
                              cep_id_t            cep_id)
{
        if (!conn) return -1;
        conn->source_cep_id = cep_id;
        return 0;
}
EXPORT_SYMBOL(connection_src_cep_id_set);

int connection_dst_cep_id_set(struct connection * conn,
                              cep_id_t            cep_id)
{
        if (!conn) return -1;
        conn->destination_cep_id = cep_id;
        return 0;
}
EXPORT_SYMBOL(connection_dst_cep_id_set);

int connection_qos_id_set(struct connection * conn,
                          qos_id_t            qos_id)
{
        if (!conn || !(is_qos_id_ok(qos_id))) return -1;
        conn->qos_id = qos_id;
        return 0;
}
EXPORT_SYMBOL(connection_qos_id_set);
