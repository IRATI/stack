/*
 * RINA personality
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

#define RINA_PREFIX "personality-default"

#include "logs.h"
#include "utils.h"
#include "personality.h"
#include "shim.h"
#include "kipcm.h"
#include "efcp.h"

struct personality_t * personality = NULL;

static int __init mod_init(void)
{

        LOG_FBEGN;

        LOG_DBG("Rina personality initializing");

        personality = kmalloc(sizeof(*personality), GFP_KERNEL);
        if (!personality) {
                LOG_ERR("Cannot allocate %zu bytes of memory",
                        sizeof(*personality));
                return -1;
        }
        ASSERT(personality);

        if (shim_init())
                return -1;

        if (kipcm_init()) {
                shim_exit();
                return -1;
        }

        if (efcp_init()) {
                kipcm_exit();
                shim_exit();
                return -1;
        }

        if (rina_personality_register(personality)) {
                efcp_exit();
                kipcm_exit();
                shim_exit();
                kfree(personality);
                personality = NULL;
                return -1;
        }

        LOG_DBG("Rina personality loaded successfully");

        LOG_FEXIT;
        return 0;
}

static void __exit mod_exit(void)
{
        LOG_FBEGN;

        ASSERT(personality);

        (void) rina_personality_unregister(personality);
        kfree(personality);
        personality = NULL;

        efcp_exit();
        kipcm_exit();
        shim_exit();

        LOG_DBG("Rina personality unloaded successfully");

        LOG_FEXIT;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA default personality");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
