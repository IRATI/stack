/*
 * DTCP (Data Transfer Protocol) kRPI
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#ifndef RINA_DTCP_PS_H
#define RINA_DTCP_PS_H

#include <linux/types.h>

#include "dtcp.h"
#include "pdu.h"
#include "rds/rfifo.h"
#include "ps-factory.h"

struct dtcp_ps {
        struct ps_base base;

        /* Behavioural policies. */

        /* Parametric policies. */
        // TODO

        /* Reference used to access the DTCP data model. */
        struct dtcp * dm;

        /* Data private to the policy-set implementation. */
        void *       priv;
};

/*
 * The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary.
 */
int dtcp_ps_publish(struct ps_factory * factory);
int dtcp_ps_unpublish(const char * name);

#endif
