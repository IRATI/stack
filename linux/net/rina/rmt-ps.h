/*
 * RMT (Relaying and Multiplexing Task) kRPI
 *
 *    Vincenzo Maffione<v.maffione@nextworks.it>
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

#ifndef RINA_KRPI_RMT_H
#define RINA_KRPI_RMT_H

#include <linux/list.h>
#include "rmt.h"

#define PARAMETER_DESC_MAX_LEN  32
#define POLICY_SET_NAME_MAX_LEN 64

struct parameter_desc {
        char type;
        char name[PARAMETER_DESC_MAX_LEN];
};

struct base_ps_factory {
        /* A name for this policy-set. */
        char name[POLICY_SET_NAME_MAX_LEN];

        /* Policy-set-specific parameters. */
        struct parameter_desc   *parameters;
        unsigned int num_parameters;

        struct list_head        node;
};

struct rmt_ps {
        /* Behavioural policies. */
        void (*max_q_policy)(struct rmt_ps *);

        /* Parametric policies. */
        int max_q;

        /* Reference used to access the RMT data model. */
        struct rmt *dm;

        /* Data private to the policy-set implementation. */
        void *priv;
};

struct rmt_ps_factory {
        /* Parent struct. */
        struct base_ps_factory base;

        /* Factory callbacks. */
        struct rmt_ps (*create)(struct rmt *);
        void (*destroy)(struct rmt_ps *);
};

/* The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary. */
int publish_rmt_ps(struct rmt_ps_factory *factory);

int unpublish_rmt_ps(const char *name);

#endif
