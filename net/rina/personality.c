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

#define RINA_PREFIX "personality"

#include "logs.h"
#include "utils.h"
#include "personality.h"

struct personality_t * personality = NULL;

static int is_ok(const struct personality_t * pers)
{
        ASSERT(pers);

        if (pers->init               &&
            pers->exit               &&
            pers->ipc_create         &&
            pers->ipc_configure      &&
            pers->ipc_destroy        &&
            pers->sdu_read           &&
            pers->sdu_write          &&
            pers->connection_create  &&
            pers->connection_destroy &&
            pers->connection_update)
                return 1;

        return 0;
}

int rina_personality_init(void)
{
        personality = NULL;

        LOG_DBG("Personality initialized");

        return 0;
}

void rina_personality_exit(void)
{
        if (personality) {
                ASSERT(personality->exit);
                personality->exit(personality->data);
        }
        personality = NULL;

        LOG_DBG("Personality finalized successfully");
}

int rina_personality_register(struct personality_t * pers)
{
        LOG_DBG("Registering personality %pK", pers);

        if (!pers || !is_ok(pers)) {
                LOG_ERR("Cannot register personality, it's bogus");
                return -1;
        }
        if (personality) {
                LOG_ERR("Another personality is already present, "
                        "please remove it first");
                return -1;
        }

        ASSERT(personality       != NULL);
        ASSERT(personality->init != NULL);

        if (!pers->init(pers->data)) {
                LOG_ERR("Could not initialize personality");
                return -1;
        }

        personality = pers;

        LOG_DBG("Personality registered successfully");

        return 0;
}

int rina_personality_unregister(struct personality_t * pers)
{
        LOG_DBG("Un-registering personality %pK", pers);

        if (!pers) {
                LOG_ERR("Cannot unregister personality, it's bogus");
                return -1;
        }

        if (personality != pers) {
                LOG_ERR("Cannot remove a different personality");
                return -1;
        }

        ASSERT(personality       != NULL);
        ASSERT(personality->exit != NULL);

        personality->exit(pers->data);
        personality = 0;

        LOG_DBG("Personality un-registered successfully");

        return 0;
}
