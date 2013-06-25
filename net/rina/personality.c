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
        LOG_DBG("Initializing personality layer");

        if (rina_personality) {
                LOG_ERR("The personality layer already initialized, "
                        "bailing out");
                return -1;
        }

        /* Useless */
        rina_personality = NULL;

        LOG_DBG("Personality layer initialized successfully");

        return 0;
}

void rina_personality_exit(void)
{
        LOG_DBG("Finalizing personality layer");

        if (rina_personality) {
                ASSERT(rina_personality->fini);
                rina_personality->fini(rina_personality->data);
        }

        rina_personality = NULL;

        LOG_DBG("Personality layer finalized successfully");
}

static int is_label_ok(const char * label)
{
        LOG_DBG("Checking label");

        if (!label) {
                LOG_DBG("Label is empty");
                return 0;
        }
        if (strlen(label) == 0) {
                LOG_DBG("Label has 0 length");
                return 0;
        }

        LOG_DBG("Label is ok");

        return 1;
}

static int is_ok(const struct personality_t * pers)
{
        if (!pers) {
                LOG_ERR("Personality is NULL");
                return 0;
        }

        if (!is_label_ok(pers->label)) {
                LOG_ERR("Personality %pK has bogus label", pers);
                return 0;
        }

        if ((pers->init && !pers->fini) ||
            (!pers->init && pers->fini)) {
                LOG_ERR("Personality '%s' has bogus "
                        "initializer/finalizer couple", pers->label);
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
                LOG_DBG("Personality '%s' is ok", pers->label);
                return 1;
        }

        LOG_ERR("Personality '%s' has bogus hooks", pers->label);

        return 0;
}

int rina_personality_register(struct personality_t * pers)
{
        LOG_DBG("Registering personality %pK", pers);

        if (!is_ok(pers)) {
                LOG_ERR("Cannot register personality %pK, it's bogus", pers);
                return -1;
        }
        ASSERT(pers);
        ASSERT(pers->label != NULL);

        if (rina_personality) {
                LOG_ERR("Personality '%s' is already present, "
                        "please remove it first before loading "
                        "personality '%s'",
                        rina_personality->label, pers->label);
                return -1;
        }

        if (pers->init) {
                LOG_DBG("Calling personality '%s' initializer", pers->label);
                if (!pers->init(pers->data)) {
                        LOG_ERR("Could not initialize personality '%s'",
                                pers->label);
                        return -1;
                }
        } else {
                LOG_DBG("Personality '%s' does not require initialization",
                        pers->label);
        }

        rina_personality = pers;

        LOG_INFO("Personality '%s' registered successfully", pers->label);

        return 0;
}
EXPORT_SYMBOL(rina_personality_register);

int rina_personality_unregister(struct personality_t * pers)
{
        LOG_DBG("Unregistering personality %pK", pers);

        if (!is_ok(pers)) {
                LOG_ERR("Cannot unregister personality %pK, it's bogus", pers);
                return -1;
        }

        ASSERT(pers);
        ASSERT(pers->label != NULL);

        if (rina_personality != pers) {
                LOG_ERR("Cannot unregister personality '%s', "
                        "it's different from the currently active one ('%s')",
                        pers->label, rina_personality->label);
                return -1;
        }

        ASSERT(rina_personality == pers);

        LOG_DBG("Un-registering personality '%s'", pers->label);

        if (pers->fini) {
                LOG_DBG("Calling personality '%s' finalizer",
                        pers->label);
                pers->fini(pers->data);
        } else {
                LOG_DBG("Personality '%s' does not require finalization",
                        pers->label);
        }

        rina_personality = 0;

        LOG_INFO("Personality '%s' unregistered successfully", pers->label);

        return 0;
}
EXPORT_SYMBOL(rina_personality_unregister);
