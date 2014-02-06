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

struct dt_sv {
        struct dtp_sv *  dtp_sv;
        struct dtcp_sv * dtcp_sv;

        spinlock_t       lock;
};

struct dt_sv * dtsv_create(void)
{
        struct dt_sv * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        spin_lock_init(&tmp->lock);

        return tmp;
}

struct dtp_sv * dtsv_dtp_take(struct dt_sv * sv)
{
        if (!sv) {
                LOG_ERR("Cannot take SV lock");
                return NULL;
        }

        spin_lock(&sv->lock);
        return sv->dtp_sv;
}

void dtsv_dtp_release(struct dt_sv * sv)
{
        if (!sv) {
                LOG_ERR("Cannot release SV lock");
                return NULL;
        }

        spin_unlock(&sv->lock);
}

struct dtcp_sv * dtsv_dtcp_take(struct dt_sv * sv)
{
        if (!sv) {
                LOG_ERR("Cannot take SV lock");
                return NULL;
        }

        spin_lock(&sv->lock);
        return sv->dtcp_sv;
}

void dtsv_dtcp_release(struct dt_sv * sv)
{
        if (!sv) {
                LOG_ERR("Cannot release SV lock");
                return NULL;
        }

        spin_unlock(&sv->lock);
}

int dtsv_destroy(struct dt_sv * sv)
{
        if (!sv)
                return -1;

        rkfree(sv);

        return 0;
}
