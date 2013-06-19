/*
 * RINA default personality
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

#include <linux/slab.h>
#include <linux/module.h>

#define RINA_PREFIX "personality-default"

#include "logs.h"
#include "utils.h"
#include "personality.h"
#include "shim.h"
#include "kipcm.h"
#include "efcp.h"
#include "rmt.h"

static struct personality_t * personality = NULL;

struct personality_data {
        void * kipcm;
        void * efcp;
        void * rmt;
};

static struct personality_t * personality_init(void)
{
        struct personality_t * p;
        
        p = kzalloc(sizeof(*p), GFP_KERNEL);
        if (!p) {
                LOG_ERR("Cannot allocate %zu bytes of memory",
                        sizeof(*p));
                return NULL;
        }
        p->data = kzalloc(sizeof(struct personality_data), GFP_KERNEL);
        if (!p->data) {
                LOG_ERR("Cannot allocate %zu bytes of memory",
                        sizeof(struct personality_data));
                kfree(personality);
                return NULL;
        }

        return p;
}

static void personality_fini(struct personality_t * pers)
{
        ASSERT(pers);
        ASSERT(pers->data);

        kfree(pers->data);
        kfree(pers);
}

static int __init mod_init(void)
{
        struct personality_data * pd;

        LOG_FBEGN;

        LOG_DBG("Rina personality initializing");

        personality = personality_init();
        if (!personality)
                return -ENOMEM;

        ASSERT(personality);
        ASSERT(personality->data);

        pd = personality->data;

        if (shim_init())
                return -1;

        pd->kipcm = kipcm_init();
        if (!pd->kipcm)
                goto CLEANUP_SHIM;

        pd->efcp = efcp_init();
        if (!pd->efcp)
                goto CLEANUP_KIPCM;

        pd->rmt = rmt_init();
        if (!pd->rmt)
                goto CLEANUP_EFCP;

        /* FIXME: To be filled properly */
        personality->init               = 0;
        personality->fini               = 0;
        personality->ipc_create         = 0;
        personality->ipc_configure      = 0;
        personality->ipc_destroy        = 0;
        personality->sdu_read           = 0;
        personality->sdu_write          = 0;
        personality->connection_create  = 0;
        personality->connection_destroy = 0;
        personality->connection_update  = 0;

        if (!rina_personality_register(personality)) {
                goto CLEANUP_RMT;
        }

        LOG_DBG("Rina personality loaded successfully");

        LOG_FEXIT;
        return 0;

 CLEANUP_RMT:
        rmt_fini(pd->rmt);
 CLEANUP_EFCP:
        efcp_fini(pd->efcp);
 CLEANUP_KIPCM:
        kipcm_fini(pd->kipcm);
 CLEANUP_SHIM:
        shim_exit();

        personality_fini(personality);
        personality = NULL;

        return -1;
}

static void __exit mod_exit(void)
{
        struct personality_data * pd;

        LOG_FBEGN;

        ASSERT(personality);

        if (rina_personality_unregister(personality)) {
                LOG_CRIT("Got problems while unregistering personality, "
                         "bailing out");

                LOG_FEXIT;
                return;
        }
        pd = personality->data;

        rmt_fini(pd->rmt);
        efcp_fini(pd->efcp);
        kipcm_fini(pd->kipcm);

        shim_exit();

        personality_fini(personality);
        personality = NULL;

        LOG_DBG("Rina personality unloaded successfully");

        LOG_FEXIT;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA default personality");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
