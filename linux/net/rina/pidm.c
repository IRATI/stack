/*
 * PIDM (Port-IDs Manager)
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

#include <linux/bitmap.h>

#define RINA_PREFIX "pidm"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "pidm.h"
#include "common.h"

#define BITS_IN_BITMAP ((2 << BITS_PER_BYTE) * sizeof(port_id_t))

struct pidm {
        DECLARE_BITMAP(bitmap, BITS_IN_BITMAP);
};

struct pidm * pidm_create(void)
{
        struct pidm * instance;

        instance = rkmalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;

        bitmap_zero(instance->bitmap, BITS_IN_BITMAP);

        LOG_DBG("Instance initialized successfully (%zd bits)",
                BITS_IN_BITMAP);

        return instance;
}

int pidm_destroy(struct pidm * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        rkfree(instance);

        return 0;
}

port_id_t pidm_allocate(struct pidm * instance)
{
        port_id_t id;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return port_id_bad();
        }

        id = (port_id_t) bitmap_find_next_zero_area(instance->bitmap,
                                                    BITS_IN_BITMAP,
                                                    0, 1, 0);
        LOG_DBG("The pidm bitmap find returned id %d (bad = %d)",
                id, port_id_bad());

        if (!is_port_id_ok(id)) {
                LOG_WARN("Got an out-of-range flow-id (%d) from "
                         "the bitmap allocator, the bitmap is full ...", id);
                return port_id_bad();
        }

        bitmap_set(instance->bitmap, id, 1);

        LOG_DBG("Bitmap allocation completed successfully (id = %d)", id);

        return (id + 1);
}

int pidm_release(struct pidm * instance,
                 port_id_t     id)
{
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bad flow-id passed, bailing out");
                return -1;
        }
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        bitmap_clear(instance->bitmap, (id - 1), 1);

        LOG_DBG("Bitmap release completed successfully (port_id: %d)", id);

        return 0;
}
