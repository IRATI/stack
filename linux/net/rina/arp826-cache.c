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
 *       have the time, a performance-wise implementation will obsolete this
 *       one completely.
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

#include "arp826-utils.h"

/*
 * Cache Entries - CE
 */

struct cache_entry {
        struct gpa *     pa;  /* Protocol address */

        unsigned char *  ha;  /* Hardware address */
        size_t           hal; /* Hardware address length (FIXME: REMOVE!) */

        struct list_head next;
};

static void ce_fini(struct cache_entry * entry)
{
        ASSERT(entry);

        /* Let's be merciful from now on */
        if (entry->pa) {
                gpa_destroy(entry->pa);
                entry->pa = NULL;
        }
        if (entry->ha) {
                rkfree(entry->ha);
                entry->ha = NULL;
        }
        entry->hal = 0;
}

void ce_destroy(struct cache_entry * entry)
{
        if (!entry) {
                LOG_ERR("Bogus CE, cannot destroy");
                return;
        }

        ce_fini(entry);
        rkfree(entry);
}

/* Takes the ownership of the input GPA */
static int ce_init(struct cache_entry * entry,
                   struct gpa *         protocol_address,
                   const uint8_t *      hardware_address,
                   size_t               hardware_address_length)
{
        ASSERT(entry);
        ASSERT(hardware_address);
        ASSERT(hardware_address_length > 0);

        /* It has been duplicated */
        if (!gpa_is_ok(protocol_address))
                return -1;

        entry->pa  = protocol_address;
        entry->hal = hardware_address_length;
        entry->ha  = rkmalloc(entry->hal, GFP_KERNEL);
        if (!entry->ha) {
                gpa_destroy(entry->pa);
                entry->pa  = NULL; /* Useless but merciful */
                entry->hal = 0;    /* Useless but merciful */
                return -1;
        }
        memcpy(entry->ha, hardware_address, entry->hal);
        
        INIT_LIST_HEAD(&entry->next);

        return 0;
}

struct cache_entry * ce_create(struct gpa *    gpa,
                               const uint8_t * hardware_address,
                               size_t          hardware_address_length)
{
        struct cache_entry * entry;

        if (!gpa_is_ok(gpa) || hardware_address || hardware_address_length) {
                LOG_DBG("Bogus input parameters, cannot create CE");
                return NULL;
        }

        entry = rkmalloc(sizeof(*entry), GFP_KERNEL);
        if (!entry)
                return NULL;
 
        if (ce_init(entry,
                    gpa_dup(gpa),
                    hardware_address, hardware_address_length)) {
                rkfree(entry);
                return NULL;
        }

        return entry;
}

static bool ce_is_ok(const struct cache_entry * entry)
{
        if (entry == NULL                           ||
            !gpa_is_ok(entry->pa)                   ||
            (entry->hal == 0 || entry->ha  == NULL))
                return 0;
        return 1;
}

static bool ce_is_equal(struct cache_entry * entry1,
                        struct cache_entry * entry2)
{
        if (!ce_is_ok(entry1))
                return 0;
        if (!ce_is_ok(entry2))
                return 0;

        if (!gpa_is_equal(entry1->pa, entry2->pa))
                return 0;
        if (entry1->hal != entry2->hal)
                return 0;
        if (memcmp(entry1->ha, entry2->ha, entry1->hal)) return 0;

        return 1;
}

const struct gpa * ce_pa(struct cache_entry * entry)
{
        if (!entry) {
                LOG_ERR("Bogus input parameter, cannot get CE-PA");
                return NULL;
        }
        return entry->pa;
}

const uint8_t * ce_ha(struct cache_entry * entry)
{
        if (!entry) {
                LOG_ERR("Bogus input parameter, cannot get CE-HA");
                return NULL;
        }
        return entry->ha;
}

/*
 * Cache Line - CL
 */

struct cache_line {
        size_t           hal;     /* Hardware address length */
        spinlock_t       lock;
        struct list_head entries;
};

struct cache_line * cl_create(size_t hw_address_length)
{
        struct cache_line * instance;

        if (hw_address_length == 0) {
                LOG_ERR("Bad CL size, cannot create");
                return NULL;
        }

        instance = rkmalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;

        instance->hal = hw_address_length;
        INIT_LIST_HEAD(&instance->entries);
        spin_lock_init(&instance->lock);

        return instance;
}

void cl_destroy(struct cache_line * instance)
{
        struct cache_entry * pos, * q;

        if (!instance) {
                LOG_ERR("Bogus input parameter, cannot destroy");
                return;
        }

        spin_lock(&instance->lock);

        list_for_each_entry_safe(pos, q, &instance->entries, next) {
                ASSERT(pos);
                ce_destroy(pos);
        }

        spin_unlock(&instance->lock);

        rkfree(instance);
}

size_t cl_hwa_length(struct cache_line * instance);

const struct cache_entry * cl_find_by_ha(struct cache_line * instance,
                                         const uint8_t *     hardware_address)
{
        struct cache_entry * pos;

        if (!instance || !hardware_address) {
                LOG_ERR("Bogus input parameters, cannot find-by HA");
                return NULL;
        }

        spin_lock(&instance->lock);

        list_for_each_entry(pos, &instance->entries, next) {
                if (!memcmp(pos->ha, hardware_address, instance->hal)) {
                        spin_unlock(&instance->lock);
                        return pos;
                }
        }

        spin_unlock(&instance->lock);

        return NULL;
}

const struct cache_entry * cl_find_by_gpa(struct cache_line * instance,
                                          const struct gpa *  protocol_address)
{
        struct cache_entry * pos;

        if (!instance || !gpa_is_ok(protocol_address)) {
                LOG_ERR("Bogus input parameters, cannot find-by by GPA");
                return NULL;
        }

        spin_lock(&instance->lock);

        list_for_each_entry(pos, &instance->entries, next) {
                if (gpa_is_equal(pos->pa, protocol_address)) {
                        spin_unlock(&instance->lock);
                        return pos;
                }
        }

        spin_unlock(&instance->lock);

        return NULL;
}

int cl_add(struct cache_line * instance,
           struct gpa *        protocol_address,
           const uint8_t *     hardware_address)
{
        struct cache_entry * entry;
        struct cache_entry * pos;

        if (!instance || !gpa_is_ok(protocol_address) || !hardware_address) {
                LOG_ERR("Bogus input parameters, cannot add CE to CL");
                return -1;
        }

        entry = ce_create(protocol_address, hardware_address, instance->hal);
        if (!entry)
                return -1;

        spin_lock(&instance->lock);

        list_for_each_entry(pos, &instance->entries, next) {
                if (ce_is_equal(pos, entry)) {
                        LOG_WARN("CE already present in this CL");
                        ce_destroy(entry);
                        return 0;
                }
        }

        list_add(&instance->entries, &entry->next);

        spin_unlock(&instance->lock);

        return 0;
}

void cl_remove(struct cache_line *        instance,
               const struct cache_entry * entry)
{
        struct cache_entry * pos, * q;

        if (!instance || !ce_is_ok(entry)) {
                LOG_ERR("Bogus input parameters, cannot remove CE");
                return;
        }

        spin_lock(&instance->lock);

        list_for_each_entry_safe(pos, q, &instance->entries, next) {
                if (pos == entry) {
                        struct cache_entry * tmp = pos;
                        list_del(&pos->next);
                        ce_destroy(tmp);
                        spin_unlock(&instance->lock);
                        return;
                }
        }

        spin_unlock(&instance->lock);

        rkfree(instance);
}
