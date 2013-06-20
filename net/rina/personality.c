/*
 * Personality
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

#include <linux/export.h>

#define RINA_PREFIX "personality"

#include "logs.h"
#include "utils.h"
#include "personality.h"
#include "shim.h"

/* FIXME: This layer allows 1 personality only, that's enough for the moment */
struct personality_t * rina_personality = NULL;

int rina_personality_init(void)
{
        LOG_DBG("Initializing personality");

        if (rina_personality) {
                LOG_ERR("Personality already initialized, remove it first");
                return -1;
        }

        /* Useless */
        rina_personality = NULL;

        LOG_DBG("Personality initialized successfully");

        return 0;
}

void rina_personality_exit(void)
{
        LOG_DBG("Finalizing personality");

        if (rina_personality) {
                ASSERT(rina_personality->fini);
                rina_personality->fini(rina_personality->data);
        }

        rina_personality = NULL;

        LOG_DBG("Personality finalized successfully");
}

static int is_ok(const struct personality_t * pers)
{
        ASSERT(pers);

        if ((pers->init && !pers->fini) ||
            (!pers->init && pers->fini)) {
                LOG_DBG("Personality %pK has bogus "
                        "initializer/finalizer couple", pers);
                return 0;
        }

        if (pers->ipc_create         &&
            pers->ipc_configure      &&
            pers->ipc_destroy        &&
            pers->sdu_read           &&
            pers->sdu_write          &&
            pers->connection_create  &&
            pers->connection_destroy &&
            pers->connection_update) {
                LOG_DBG("Personality %pK has bogus hooks", pers);
                return 0;
        }

        return 1;
}

int rina_personality_register(struct personality_t * pers)
{
        LOG_DBG("Registering personality %pK", pers);

        if (!pers || !is_ok(pers)) {
                LOG_ERR("Cannot register personality, it's bogus");
                return -1;
        }
        if (rina_personality) {
                LOG_ERR("Another personality is already present, "
                        "please remove it first");
                return -1;
        }

        ASSERT(pers != NULL);

        if (pers->init) {
                LOG_DBG("Calling personality %pK initialized", pers);
                if (!pers->init(pers->data)) {
                        LOG_ERR("Could not initialize personality");
                        return -1;
                }
        } else {
                LOG_DBG("This personality does not require initialization");
        }

        rina_personality = pers;

        LOG_DBG("Personality %pK registered successfully", pers);

        return 0;
}
EXPORT_SYMBOL(rina_personality_register);

int rina_personality_unregister(struct personality_t * pers)
{
        if (!pers) {
                LOG_ERR("Cannot unregister personality, it's bogus");
                return -1;
        }

        if (rina_personality != pers) {
                LOG_ERR("Cannot unregister a different personality");
                return -1;
        }

        LOG_DBG("Un-registering personality %pK", pers);

        ASSERT(pers != NULL);
        ASSERT(rina_personality == pers);

        if (pers->fini) {
                LOG_DBG("Calling personality %pK initialized", pers);
                pers->fini(pers->data);
        } else {
                LOG_DBG("This personality does not require finalization");
        }

        rina_personality = 0;

        LOG_DBG("Personality %pK unregistered successfully", pers);

        return 0;
}
EXPORT_SYMBOL(rina_personality_unregister);
