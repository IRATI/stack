/*
 * KIO (Kernel I/O)
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

#ifndef RINA_KIO_H
#define RINA_KIO_H

#include "common.h"
#include "efcp.h"
#include "rmt.h"

struct kio;

struct kio *            kio_create(void);
int                     kio_destroy(struct kio * instance);

struct efcp_container * kio_egress_get(struct kio * instance,
                                       port_id_t    id);
int                     kio_egress_set(struct kio *            instance,
                                       port_id_t               id,
                                       struct efcp_container * container);
struct rmt *            kio_ingress_get(struct kio * instance,
                                        port_id_t     id);
int                     kio_ingress_set(struct kio * instance,
                                        port_id_t    id,
                                        struct rmt * rmt);

#endif
