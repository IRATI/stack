/*
 * CIDM (CEP-IDs Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#define RINA_PREFIX "cidm"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "cidm.h"
#include "common.h"

#define MAX_CEP_ID (((2 << BITS_PER_BYTE) * sizeof(cep_id_t)) - 1)

struct cidm {
	struct list_head allocated_cep_ids;
	cep_id_t last_allocated;
};

struct alloc_cepid {
        struct list_head       list;
        cep_id_t cep_id;
};

struct cidm * cidm_create(void)
{
	struct cidm * instance;

	instance = rkmalloc(sizeof(*instance), GFP_KERNEL);
	if (!instance)
		return NULL;

	INIT_LIST_HEAD(&(instance->allocated_cep_ids));
	instance->last_allocated = 0;

	LOG_INFO("Instance initialized successfully (%zd cep-ids)",
			MAX_CEP_ID);

	return instance;
}

int cidm_destroy(struct cidm * instance)
{
	struct alloc_cepid * pos, * next;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        list_for_each_entry_safe(pos, next, &instance->allocated_cep_ids, list) {
                rkfree(pos);
        }

        rkfree(instance);

        return 0;
}

int cidm_allocated(struct cidm * instance, cep_id_t cep_id)
{
	struct alloc_cepid * pos, * next;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        list_for_each_entry_safe(pos, next, &instance->allocated_cep_ids, list) {
                if (pos->cep_id == cep_id) {
                	return 1;
                }
        }

        return 0;
}

cep_id_t cidm_allocate(struct cidm * instance)
{
	struct alloc_cepid * new_cep_id;
        cep_id_t cep_id;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return port_id_bad();
        }

        if (instance->last_allocated == MAX_CEP_ID) {
        	cep_id = 1;
        } else {
        	cep_id = instance->last_allocated + 1;
        }

        while (cidm_allocated(instance, cep_id)) {
        	if (cep_id == MAX_CEP_ID) {
        		cep_id = 1;
        	} else {
        		cep_id ++;
        	}
        }

        new_cep_id = rkmalloc(sizeof(*new_cep_id), GFP_ATOMIC);
        if (!new_cep_id)
        	return cep_id_bad();

        INIT_LIST_HEAD(&(new_cep_id->list));
        new_cep_id->cep_id = cep_id;
        list_add(&(new_cep_id->list), &(instance->allocated_cep_ids));

        instance->last_allocated = cep_id;

        LOG_DBG("Cep-id allocation completed successfully (id = %d)", cep_id);

        return cep_id;
}

int cidm_release(struct cidm * instance,
                 cep_id_t      id)
{
	struct alloc_cepid * pos, * next;

       if (!is_port_id_ok(id)) {
               LOG_ERR("Bad cep-id passed, bailing out");
               return -1;
       }
       if (!instance) {
               LOG_ERR("Bogus instance passed, bailing out");
               return -1;
       }

       list_for_each_entry_safe(pos, next, &instance->allocated_cep_ids, list) {
               if (pos->cep_id == id) {
               	list_del(&pos->list);
               	rkfree(pos);
               	LOG_DBG("Cep-id release completed successfully (cep_id: %d)", id);

               	return 0;
               }
       }

       LOG_ERR("Didn't find cep-id %d, returning error", id);

       return 0;
}
