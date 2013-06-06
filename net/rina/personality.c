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
#include "personality.h"

const struct personality_t * personality = 0;

static int is_ok(const struct personality_t * pers)
{
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
        personality = 0;
        return 0;
}

void rina_personality_exit(void)
{
        if (personality)
                personality->exit();
        personality = 0;
}

int rina_personality_add(const struct personality_t * pers)
{
        if (!pers)
                BUG();

        if (!is_ok(pers))
                return -1;

        if (!personality) {
                int retval;

                personality = pers;
                retval      = personality->init();
                if (!retval) {
                        LOG_ERR("Could not initialize personality");
                        personality = 0;
                        return retval;
                }

                return retval;
        }

        LOG_ERR("Personality already present, remove it first");
        return -1;
}

int rina_personality_remove(const struct personality_t * pers)
{
        if (personality == pers) {
                personality->exit();
                personality = 0;
                return 0;
        }

        LOG_ERR("Cannot remove a different personality");
        return -1;
}
