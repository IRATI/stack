/*
 * FMGR (Flows Manager)
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

#define RINA_PREFIX "fmgr"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "fidm.h"

struct fmgr {
        struct fidm * fidm;
};

struct fmgr * fmgr_create(void)
{
        struct fmgr * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->fidm = fidm_create();
        if (!tmp->fidm) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

int fmgr_destroy(struct fmgr * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        fidm_destroy(instance->fidm);
        rkfree(instance);

        return 0;
}


