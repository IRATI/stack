/*
 * ARP 826 cache
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

/*
 * FIXME: The following lines provide basic framework and utilities. These
 *        dependencies will be removed ASAP to let this module live its own
 *        life
 */
#define RINA_PREFIX "arp826-cache"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

static struct list_head cache;

int arp826_cache_init(void)
{
        INIT_LIST_HEAD(&cache);

        return 0;
}

void arp826_cache_fini(void)
{
        LOG_MISSING;
}

struct cache_entry {
        size_t          pal; /* FIXME: To be removed */

        unsigned char * spa;
        unsigned char * tpa;

        size_t          hal; /* FIXME: To be removed */

        unsigned char * sha;
        unsigned char * tha;
};

static int ce_is_ok(struct cache_entry * entry)
{
        if (entry      == NULL ||
            entry->pal == 0    ||
            entry->hal == 0    ||
            entry->spa == NULL ||
            entry->tpa == NULL ||
            entry->sha == NULL ||
            entry->tha == NULL)
                return 0;
        return 1;
}

static void ce_fini(struct cache_entry * entry)
{
        ASSERT(entry);

        if (entry->spa) rkfree(entry->spa);
        if (entry->tpa) rkfree(entry->tpa);
        if (entry->sha) rkfree(entry->sha);
        if (entry->tha) rkfree(entry->tha);
}

static void ce_destroy(struct cache_entry * entry)
{
        ASSERT(entry);

        ce_fini(entry);
        rkfree(entry);
}

static struct cache_entry *
ce_create(size_t                protocol_address_length,
          const unsigned char * source_protocol_address,
          const unsigned char * target_protocol_address,
          size_t                hardware_address_length,
          const unsigned char * source_hardware_address,
          const unsigned char * target_hardware_address)
{
        struct cache_entry * entry;

        ASSERT(protocol_address_length > 0);
        ASSERT(hardware_address_length > 0);
        ASSERT(source_protocol_address);
        ASSERT(target_protocol_address);
        ASSERT(source_hardware_address);
        ASSERT(target_hardware_address);
        
        entry = rkmalloc(sizeof(*entry), GFP_KERNEL);
        if (!entry)
                return NULL;

        entry->pal = protocol_address_length;
        entry->spa = rkmalloc(protocol_address_length, GFP_KERNEL);
        entry->tpa = rkmalloc(protocol_address_length, GFP_KERNEL);
        entry->hal = hardware_address_length;
        entry->sha = rkmalloc(hardware_address_length, GFP_KERNEL);
        entry->tha = rkmalloc(hardware_address_length, GFP_KERNEL);
        if (!ce_is_ok(entry)) {
                ce_destroy(entry);
                return NULL;
        }

        memcpy(entry->spa, source_protocol_address, protocol_address_length);
        memcpy(entry->tpa, target_protocol_address, protocol_address_length);
        memcpy(entry->sha, source_hardware_address, hardware_address_length);
        memcpy(entry->tha, target_hardware_address, hardware_address_length);

        return entry;
}

#if 0
static bool ce_is_equal(struct cache_entry * entry1,
                        struct cache_entry * entry2)
{
        if (!ce_is_ok(entry1))
                return 0;
        if (!ce_is_ok(entry2))
                return 0;

        if (entry1->pal != entry2->pal)
                return 0;
        if (entry1->hal != entry2->hal)
                return 0;

        if (memcmp(entry1->spa, entry2->spa, entry1->pal)) return 0;
        if (memcmp(entry1->tpa, entry2->tpa, entry1->pal)) return 0;
        if (memcmp(entry1->sha, entry2->sha, entry1->hal)) return 0;
        if (memcmp(entry1->tha, entry2->tha, entry1->hal)) return 0;

        return 1;
}
#endif

int arp826_cache_add(size_t                protocol_address_length,
                     const unsigned char * source_protocol_address,
                     const unsigned char * target_protocol_address,
                     size_t                hardware_address_length,
                     const unsigned char * source_hardware_address,
                     const unsigned char * target_hardware_address)
{
        struct cache_entry * entry = ce_create(protocol_address_length,
                                               source_protocol_address,
                                               target_protocol_address,
                                               hardware_address_length,
                                               source_hardware_address,
                                               target_hardware_address);
        if (!entry)
                return -1;

        ce_destroy(entry);
                                               
        LOG_MISSING;

        return -1;
}
