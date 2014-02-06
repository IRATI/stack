/*
 * DT (Data Transfer)
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

#define RINA_PREFIX "dt"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dt.h"

struct dt {
        struct dtp *  dtp;
        struct dtcp * dtcp;

        spinlock_t    lock;
};

struct dt * dt_create(void)
{
        struct dt * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        spin_lock_init(&tmp->lock);

        return tmp;
}

int dt_destroy(struct dt * sv)
{
        if (!sv)
                return -1;

        rkfree(sv);

        return 0;
}

int dt_dtp_bind(struct dt *  dt, struct dtp * dtp)
{
        if (!dt)
                return -1;

        spin_lock(&dt->lock);
        dt->dtp = dtp;
        spin_unlock(&dt->lock);

        return 0;
}

int dt_dtcp_bind(struct dt * dt, struct dtcp * dtcp)
{
        if (!dt)
                return -1;

        spin_lock(&dt->lock);
        dt->dtcp = dtcp;
        spin_unlock(&dt->lock);

        return 0;
}

struct dtp * dt_dtp(struct dt * dt)
{
        struct dtp * tmp;

        if (!dt)
                return NULL;

        spin_lock(&dt->lock);
        tmp = dt->dtp;
        spin_unlock(&dt->lock);

        return tmp;
}

struct dtcp * dt_dtcp(struct dt * dt)
{
        struct dtcp * tmp;

        if (!dt)
                return NULL;

        spin_lock(&dt->lock);
        tmp = dt->dtcp;
        spin_unlock(&dt->lock);

        return tmp;
}
