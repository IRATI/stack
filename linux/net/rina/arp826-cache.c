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
 * NOTE: This cache implementation is far far far away to be a performance-wise
 *       one. Its scope is to server the upper-layers, nothing else. If we will
 *       have the time, a performance-wise implementation will obsolete this one
 *       completely.
 */

#include <linux/kernel.h>

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

struct gpa {
        uint8_t * address;
        size_t    length;
};

struct gpa * gpa_create(const uint8_t * address, size_t length)
{
        struct gpa * tmp;

        if (!address || !length)
                return NULL;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->length  = length;
        tmp->address = rkmalloc(tmp->length, GFP_KERNEL);
        if (!tmp->address) {
                rkfree(tmp);
                return NULL;
        }
        memcpy(tmp->address, address, length);

        return tmp;
}

void gpa_destroy(struct gpa * gpa)
{
        ASSERT(gpa);
        ASSERT(gpa->address);

        rkfree(gpa->address);
        rkfree(gpa);
}

struct cache_entry {
        size_t           pal; /* FIXME: To be removed */

        unsigned char *  spa;
        unsigned char *  tpa;

        size_t           hal; /* FIXME: To be removed */

        unsigned char *  sha;
        unsigned char *  tha;

        struct list_head next;
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

        entry->pal = entry->hal = 0;
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

int ce_init(struct cache_entry *  entry,
            size_t                protocol_address_length,
            const unsigned char * protocol_address_source,
            const unsigned char * protocol_address_target,
            size_t                hardware_address_length,
            const unsigned char * hardware_address_source,
            const unsigned char * hardware_address_target)
{
        ASSERT(entry);
        ASSERT(protocol_address_length > 0);
        ASSERT(hardware_address_length > 0);
        ASSERT(protocol_address_source);
        ASSERT(protocol_address_target);
        ASSERT(hardware_address_source);
        ASSERT(hardware_address_target);

        entry->pal = protocol_address_length;
        entry->spa = rkmalloc(protocol_address_length, GFP_KERNEL);
        entry->tpa = rkmalloc(protocol_address_length, GFP_KERNEL);
        entry->hal = hardware_address_length;
        entry->sha = rkmalloc(hardware_address_length, GFP_KERNEL);
        entry->tha = rkmalloc(hardware_address_length, GFP_KERNEL);
        if (!ce_is_ok(entry)) {
                ce_fini(entry);
                return -1;
        }

        memcpy(entry->spa, protocol_address_source, protocol_address_length);
        memcpy(entry->tpa, protocol_address_target, protocol_address_length);
        memcpy(entry->sha, hardware_address_source, hardware_address_length);
        memcpy(entry->tha, hardware_address_target, hardware_address_length);

        INIT_LIST_HEAD(&entry->next);

        return 0;
}

static struct cache_entry *
ce_create(size_t                protocol_address_length,
          const unsigned char * protocol_address_source,
          const unsigned char * protocol_address_target,
          size_t                hardware_address_length,
          const unsigned char * hardware_address_source,
          const unsigned char * hardware_address_target)
{
        struct cache_entry * entry;

        ASSERT(protocol_address_length > 0);
        ASSERT(protocol_address_source);
        ASSERT(protocol_address_target);
        ASSERT(hardware_address_length > 0);
        ASSERT(hardware_address_source);
        ASSERT(hardware_address_target);

        entry = rkmalloc(sizeof(*entry), GFP_KERNEL);
        if (!entry)
                return NULL;
        if (ce_init(entry,
                    protocol_address_length,
                    protocol_address_source,
                    protocol_address_target,
                    hardware_address_length,
                    hardware_address_source,
                    hardware_address_target)) {
                rkfree(entry);
                return NULL;
        }

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

struct cache_line {
        spinlock_t       lock;
        struct list_head entries;
};

struct cache_line * cl_create(void)
{
        struct cache_line * instance;

        instance = rkmalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;

        INIT_LIST_HEAD(&instance->entries);
        spin_lock_init(&instance->lock);

        return instance;
}

void cl_destroy(struct cache_line * instance)
{
        struct cache_entry * pos, * q;

        ASSERT(instance);

        spin_lock(&instance->lock);

        list_for_each_entry_safe(pos, q, &instance->entries, next) {
                ce_destroy(pos);
        }

        spin_unlock(&instance->lock);
        
        rkfree(instance);
}

int cl_add(struct cache_line *   instance,
           size_t                protocol_address_length,
           const unsigned char * protocol_address_source,
           const unsigned char * protocol_address_target,
           size_t                hardware_address_length,
           const unsigned char * hardware_address_source,
           const unsigned char * hardware_address_target)
{
        struct cache_entry * entry;

        if (!instance)
                return -1;

        entry = ce_create(protocol_address_length,
                          protocol_address_source,
                          protocol_address_target,
                          hardware_address_length,
                          hardware_address_source,
                          hardware_address_target);
        if (!entry)
                return -1;

        ce_destroy(entry);

        LOG_MISSING;

        return -1;
}
