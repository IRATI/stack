/*
 * FIDM (Flows-IDs Manager)
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

#include <linux/module.h>
#include <linux/bitmap.h>

#define RINA_PREFIX "fidm"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "fidm.h"

#define BITS_IN_BITMAP (2 ^ BITS_PER_BYTE * sizeof(flow_id_t))

struct fidm {
        spinlock_t     lock;
        DECLARE_BITMAP(bitmap, BITS_IN_BITMAP);
};

int fidm_init(void)
{
        struct fidm * instance;

        instance = rkmalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return -1;

        bitmap_zero(instance->bitmap, BITS_IN_BITMAP);
        spin_lock_init(&instance->lock);

        LOG_DBG("Instance initialized successfully (%zd bits)",
                BITS_IN_BITMAP);

        return instance;
}

int fidm_fini(struct fidm * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        rkfree(instance);

        return 0;
}

#define FLOW_ID_WRONG -1

int is_flow_id_ok(flow_id_t id)
{ return (id >= 0 && id < BITS_IN_BITMAP) ? 1 : 0; }
EXPORT_SYMBOL(is_flow_id_ok);

flow_id_t fidm_allocate(struct fidm * instance)
{
        flow_id_t id;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return FLOW_ID_WRONG;
        }
        
        spin_lock(&instance->lock);

        id = (flow_id_t) bitmap_find_next_zero_area(instance->bitmap,
                                                    BITS_IN_BITMAP,
                                                    0, 1, 0);
        if (id >= BITS_IN_BITMAP)
                id = -1;

        spin_unlock(&instance->lock);
        
        return id;
}
EXPORT_SYMBOL(fidm_allocate);

int fidm_release(struct fidm * instance,
                 flow_id_t     id)
{
        if (!is_flow_id_ok(id)) {
                LOG_ERR("Bad flow-id passed, bailing out");
                return -1;
        }
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        spin_lock(&instance->lock);

        bitmap_set(instance->bitmap, id, 1);

        spin_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(fidm_release);
