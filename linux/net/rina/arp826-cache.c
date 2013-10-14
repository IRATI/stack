/*
 * ARP 826 (wonnabe) cache
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

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-cache"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826.h"
#include "arp826-utils.h"

/*
 * Cache Entries - CE
 */

struct cache_entry {
        struct gpa *     pa; /* Protocol address */
        struct gha *     ha; /* Hardware address */

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
                gha_destroy(entry->ha);
                entry->ha = NULL;
        }
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
                   struct gpa *         pa,
                   struct gha *         ha)
{
        ASSERT(entry);
        ASSERT(pa);
        ASSERT(ha);

        /* It has been duplicated therefore, no assertions here */
        if (!gpa_is_ok(pa) || !gha_is_ok(ha))
                return -1;

        entry->pa = pa;
        entry->ha = ha;
        
        INIT_LIST_HEAD(&entry->next);

        return 0;
}

struct cache_entry * ce_create(struct gpa * gpa,
                               struct gha * gha)
{
        struct cache_entry * entry;

        if (!gpa_is_ok(gpa) || !gha_is_ok(gha)) {
                LOG_DBG("Bogus input parameters, cannot create CE");
                return NULL;
        }

        entry = rkmalloc(sizeof(*entry), GFP_KERNEL);
        if (!entry)
                return NULL;
 
        if (ce_init(entry, gpa_dup(gpa), gha_dup(gha))) {
                rkfree(entry);
                return NULL;
        }

        return entry;
}

static bool ce_is_ok(const struct cache_entry * entry)
{
        return (entry == NULL         ||
                !gpa_is_ok(entry->pa) ||
                !gha_is_ok(entry->ha)) ? 0 : 1;
}

#if 0
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
#endif

const struct gpa * ce_pa(struct cache_entry * entry)
{
        if (!entry) {
                LOG_ERR("Bogus input parameter, cannot get CE-PA");
                return NULL;
        }
        return entry->pa;
}

const struct gha * ce_ha(struct cache_entry * entry)
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

struct cache_line * cl_create(size_t ha_length)
{
        struct cache_line * instance;

        if (ha_length == 0) {
                LOG_ERR("Bad CL HA size, cannot create");
                return NULL;
        }

        instance = rkmalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;

        instance->hal = ha_length;
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

const struct cache_entry * cl_find_by_gha(struct cache_line * instance,
                                         const struct gha *   address)
{
        struct cache_entry * pos;

        if (!instance || !address) {
                LOG_ERR("Bogus input parameters, cannot find-by HA");
                return NULL;
        }

        spin_lock(&instance->lock);

        list_for_each_entry(pos, &instance->entries, next) {
                if (gha_is_equal(pos->ha, address)) {
                        spin_unlock(&instance->lock);
                        return pos;
                }
        }

        spin_unlock(&instance->lock);

        return NULL;
}

const struct cache_entry * cl_find_by_gpa(struct cache_line * instance,
                                          const struct gpa *  address)
{
        struct cache_entry * pos;

        if (!instance || !gpa_is_ok(address)) {
                LOG_ERR("Bogus input parameters, cannot find-by by GPA");
                return NULL;
        }

        spin_lock(&instance->lock);

        list_for_each_entry(pos, &instance->entries, next) {
                if (gpa_is_equal(pos->pa, address)) {
                        spin_unlock(&instance->lock);
                        return pos;
                }
        }

        spin_unlock(&instance->lock);

        return NULL;
}

int cl_add(struct cache_line * instance,
           struct gpa *        pa,
           struct gha *        ha)
{
        struct cache_entry * entry;
        struct cache_entry * pos;

        if (!instance || !gpa_is_ok(pa) || !gha_is_ok(ha)) {
                LOG_ERR("Bogus input parameters, cannot add CE to CL");
                return -1;
        }

        entry = ce_create(pa, ha);
        if (!entry)
                return -1;

        spin_lock(&instance->lock);

        list_for_each_entry(pos, &instance->entries, next) {
                if (gha_is_equal(ce_ha(pos), ha) &&
                    gpa_is_equal(ce_pa(pos), pa)) {
                        LOG_WARN("We already have this entry ...");
                        spin_unlock(&instance->lock);
                        return 0;
                }

                /* FIXME: What about the other conditions ??? */
                if (gha_is_equal(ce_ha(pos), ha)) {
                        LOG_DBG("We already have the same GHA in the cache");

                        /* FIXME: What should we do here? */

                        /* Remember to: spin_unlock(&instance->lock); */
                }

                if (gpa_is_equal(ce_pa(pos), pa)) {
                        LOG_DBG("We already have the same GPA in the cache");

                        /* FIXME: What should we do here? */

                        /* Remember to: spin_unlock(&instance->lock); */
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

int arp826_add(uint16_t           ptype,
               const struct gpa * pa,
               const struct gha * ha,
               arp826_timeout_t   timeout)
{
        if (!gpa_is_ok(pa) || !gha_is_ok(ha)) {
                LOG_ERR("Cannot remove, bad input parameters");
                return -1;
        }
        
        LOG_MISSING;
        
        return 0;
}
 EXPORT_SYMBOL(arp826_add);

int arp826_remove(uint16_t           ptype,
                  const struct gpa * pa,
                  const struct gha * ha)
{
        if (!gpa_is_ok(pa) || !gha_is_ok(ha)) {
                LOG_ERR("Cannot remove, bad input parameters");
                return -1;
        }

        LOG_MISSING;

        return 0;
}
EXPORT_SYMBOL(arp826_remove);
