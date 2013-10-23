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
#define RINA_PREFIX "arp826-tables"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826.h"
#include "arp826-utils.h"

struct table_entry {
        struct gpa *     pa; /* Protocol address */
        struct gha *     ha; /* Hardware address */

        struct list_head next;
};

static void tble_fini(struct table_entry * entry)
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

void tble_destroy(struct table_entry * entry)
{
        if (!entry) {
                LOG_ERR("Bogus table entry, cannot destroy");
                return;
        }

        tble_fini(entry);
        rkfree(entry);
}

/* Takes the ownership of the input GPA */
static int tble_init(struct table_entry * entry,
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

struct table_entry * tble_create(struct gpa * gpa,
                                 struct gha * gha)
{
        struct table_entry * entry;

        if (!gpa_is_ok(gpa)) {
                LOG_DBG("Bogus GPA, cannot create table entry");
                return NULL;
        }
        if (!gha_is_ok(gha)) {
                LOG_DBG("Bogus GHA, cannot create table entry");
                return NULL;
        }

        entry = rkmalloc(sizeof(*entry), GFP_KERNEL);
        if (!entry)
                return NULL;

        if (tble_init(entry, gpa_dup(gpa), gha_dup(gha))) {
                rkfree(entry);
                return NULL;
        }

        return entry;
}

#if 0
static bool tble_is_ok(const struct table_entry * entry)
{
        return (entry == NULL         ||
                !gpa_is_ok(entry->pa) ||
                !gha_is_ok(entry->ha)) ? false : true;
}

static bool tble_is_equal(struct table_entry * entry1,
                          struct table_entry * entry2)
{
        if (!tble_is_ok(entry1))
                return 0;
        if (!tble_is_ok(entry2))
                return 0;

        if (!gpa_is_equal(entry1->pa, entry2->pa))
                return 0;
        if (entry1->hal != entry2->hal)
                return 0;
        if (memcmp(entry1->ha, entry2->ha, entry1->hal)) return 0;

        return 1;
}
#endif

const struct gpa * tble_pa(const struct table_entry * entry)
{
        if (!entry) {
                LOG_ERR("Bogus input parameter, cannot get PA");
                return NULL;
        }
        return entry->pa;
}

const struct gha * tble_ha(const struct table_entry * entry)
{
        if (!entry) {
                LOG_ERR("Bogus input parameter, cannot get HA");
                return NULL;
        }
        return entry->ha;
}

struct table {
        size_t           hal;     /* Hardware address length */
        spinlock_t       lock;
        struct list_head entries;
};

static struct table * tbl_create(size_t ha_length)
{
        struct table * instance;

        LOG_DBG("Creating tbl instance for ha-len %zd", ha_length);

        if (ha_length == 0) {
                LOG_ERR("Bad HA length, cannot create table");
                return NULL;
        }

        instance = rkmalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;

        LOG_DBG("Got memory, fillin' it up");

        instance->hal = ha_length;
        INIT_LIST_HEAD(&instance->entries);
        spin_lock_init(&instance->lock);

        LOG_DBG("Table instance created successfully");

        return instance;
}

static void tbl_destroy(struct table * instance)
{
        struct table_entry * pos, * q;

        if (!instance) {
                LOG_ERR("Bogus input parameter, cannot destroy table");
                return;
        }

        spin_lock(&instance->lock);

        list_for_each_entry_safe(pos, q, &instance->entries, next) {
                ASSERT(pos);
                tble_destroy(pos);
        }

        spin_unlock(&instance->lock);

        rkfree(instance);
}

int tbl_update_by_gpa(struct table *     instance,
                      const struct gpa * pa,
                      struct gha *       ha)
{
        struct table_entry * pos;

        if (!instance) {
                LOG_ERR("Bogus instance, cannot update GPA");
                return -1;
        }
        if (!gpa_is_ok(pa)) {
                LOG_ERR("Bogus PA, cannot update GPA");
                return -1;
        }
        if (!gha_is_ok(ha)) {
                LOG_ERR("Bogus HA, cannot update GPA");
                return -1;
        }

        spin_lock(&instance->lock);

        list_for_each_entry(pos, &instance->entries, next) {
                if (gpa_is_equal(pos->pa, pa)) {
                        gha_destroy(pos->ha);
                        pos->ha = ha;
                        spin_unlock(&instance->lock);
                        return 0;
                }
        }

        spin_unlock(&instance->lock);

        return -1;
}

const struct table_entry * tbl_find(struct table *     instance,
                                    const struct gpa * pa,
                                    const struct gha * ha)
{
        struct table_entry * pos;

        if (!instance) {
                LOG_ERR("Bogus instance, cannot find entry");
                return NULL;
        }
        if (!gpa_is_ok(pa)) {
                LOG_ERR("Bogus PA, cannot find entry");
                return NULL;
        }
        if (!gha_is_ok(ha)) {
                LOG_ERR("Bogus HA, cannot find entry");
                return NULL;
        }

        spin_lock(&instance->lock);

        list_for_each_entry(pos, &instance->entries, next) {
                if (gpa_is_equal(pos->pa, pa) &&
                    gha_is_equal(pos->ha, ha)) {
                        spin_unlock(&instance->lock);
                        return pos;
                }
        }

        spin_unlock(&instance->lock);

        return NULL;
}

const struct table_entry * tbl_find_by_gha(struct table *     instance,
                                           const struct gha * address)
{
        struct table_entry * pos;

        if (!instance) {
                LOG_ERR("Bogus instance, cannot find by GHA");
                return NULL;
        }
        if (!gha_is_ok(address)) {
                LOG_ERR("Bogus address, cannot find by GHA");
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

const struct table_entry * tbl_find_by_gpa(struct table *     instance,
                                           const struct gpa * address)
{
        struct table_entry * pos;

        if (!instance) {
                LOG_ERR("Bogus instance, cannot find by GPA");
                return NULL;
        }
        if (!gpa_is_ok(address)) {
                LOG_ERR("Bogus address, cannot find by GPA");
                return NULL;
        }

        LOG_DBG("Looking for the following address in table");
        gpa_dump(address);

        LOG_DBG("Showing addresses in table");

        spin_lock(&instance->lock);
        list_for_each_entry(pos, &instance->entries, next) {
                gpa_dump(pos->pa);
                if (gpa_is_equal(pos->pa, address)) {
                        spin_unlock(&instance->lock);
                        return pos;
                }
        }
        spin_unlock(&instance->lock);

        LOG_DBG("Dump complete");

        return NULL;
}

int tbl_add(struct table * instance,
            struct gpa *   pa,
            struct gha *   ha)
{
        struct table_entry * entry;
        struct table_entry * pos;

        if (!instance) {
                LOG_ERR("Bogus instance, cannot add entry to table");
                return -1;
        }
        if (!gpa_is_ok(pa)) {
                LOG_ERR("Bogus PA, cannot add entry to table");
                return -1;
        }
        if (!gha_is_ok(ha)) {
                LOG_ERR("Bogus HA, cannot add entry to table");
                return -1;
        }

        entry = tble_create(pa, ha);
        if (!entry)
                return -1;

        spin_lock(&instance->lock);

        list_for_each_entry(pos, &instance->entries, next) {
                if (gha_is_equal(tble_ha(pos), ha) &&
                    gpa_is_equal(tble_pa(pos), pa)) {
                        LOG_WARN("We already have this entry ...");
                        spin_unlock(&instance->lock);
                        return 0;
                }

                /* FIXME: What about the other conditions ??? */
                if (gha_is_equal(tble_ha(pos), ha)) {
                        LOG_DBG("We already have the same GHA in the cache");

                        /* FIXME: What should we do here? */

                        /* Remember to: spin_unlock(&instance->lock); */
                }

                if (gpa_is_equal(tble_pa(pos), pa)) {
                        LOG_DBG("We already have the same GPA in the cache");

                        /* FIXME: What should we do here? */

                        /* Remember to: spin_unlock(&instance->lock); */
                }
        }

        list_add(&instance->entries, &entry->next);

        spin_unlock(&instance->lock);

        return 0;
}

void tbl_remove(struct table *             instance,
                const struct table_entry * entry)
{
        struct table_entry * pos, * q;

        if (!instance) {
                LOG_ERR("Bogus instance, cannot remove entry from table");
                return;
        }
        if (!entry) {
                LOG_ERR("Bogus entry, cannot remove entry from table");
                return;
        }

        spin_lock(&instance->lock);

        list_for_each_entry_safe(pos, q, &instance->entries, next) {
                if (pos == entry) {
                        struct table_entry * tmp = pos;
                        list_del(&pos->next);
                        tble_destroy(tmp);
                        spin_unlock(&instance->lock);
                        return;
                }
        }

        spin_unlock(&instance->lock);

        rkfree(instance);
}

static spinlock_t tables_lock;
struct tmap *     tables = NULL;

struct table * tbls_find(uint16_t ptype)
{
        struct tmap_entry * e;
        struct table *      tmp;

        spin_lock(&tables_lock);

        e = tmap_entry_find(tables, ptype);
        if (!e) {
                spin_unlock(&tables_lock);
                return NULL;
        }

        tmp = tmap_entry_value(e);

        spin_unlock(&tables_lock);

        return tmp;
}

int tbls_create(uint16_t ptype, size_t hwlen)
{
        struct table *      cl;
        struct tmap_entry * e;

        LOG_DBG("Creating table for ptype = 0x%04X, hwlen = %zd",
                ptype, hwlen);

        cl = tbls_find(ptype);
        if (cl) {
                LOG_WARN("Table for ptype 0x%04X already created",ptype);
                return 0;
        }

        cl = tbl_create(hwlen);
        if (!cl) {
                LOG_ERR("Cannot create table for ptype 0x%04X, hwlen %zd",
                        ptype, hwlen);
                return -1;
        }

        LOG_DBG("Creating new tmap-entry");
        e = tmap_entry_create(ptype, cl);
        if (!e) {
                LOG_ERR("Cannot create new entry, bailing out");
                return -1;
        }

        LOG_DBG("Now adding the new table to the tables map");

        spin_lock(&tables_lock);
        if (tmap_entry_insert(tables, ptype, e)) {
                spin_unlock(&tables_lock);

                LOG_ERR("Cannot insert new entry into table for ptype 0x%04X",
                        ptype);
                tmap_entry_destroy(e);
                tbl_destroy(cl);

                return -1;
        }
        spin_unlock(&tables_lock);

        LOG_DBG("Table for ptype 0x%04X created successfully", ptype);

        return 0;
}

int tbls_destroy(uint16_t ptype)
{
        struct tmap_entry * e;
        struct table *      cl;

        spin_lock(&tables_lock);

        e = tmap_entry_find(tables, ptype);
        if (!e) {
                LOG_ERR("Table for ptype 0x%04X is missing, cannot destroy",
                        ptype);
                spin_unlock(&tables_lock);
                return -1;
        }
        tmap_entry_remove(e);

        spin_unlock(&tables_lock); /* No need to hold the lock anymore */

        cl = tmap_entry_value(e);

        ASSERT(cl);

        tbl_destroy(cl);
        tmap_entry_destroy(e);

        LOG_DBG("Table for ptype 0x%04X destroyed successfully", ptype);

        return 0;
}

int tbls_init(void)
{
        LOG_DBG("Initializing");

        if (tables) {
                LOG_WARN("Tables already initialized, bailing out");
                return 0;
        }

        tables = tmap_create();
        if (!tables)
                return -1;

        spin_lock_init(&tables_lock);

        LOG_DBG("Initialized successfully");

        return 0;
}

void tbls_fini(void)
{
        if (!tables) {
                LOG_WARN("Already finalized, bailing out ...");
                return;
        }

        LOG_DBG("Finalizing");

        ASSERT(tmap_empty(tables));

        tmap_destroy(tables);

        tables = NULL;
}

int arp826_add(uint16_t           ptype,
               const struct gpa * pa,
               const struct gha * ha)
{
        struct table * cl;

        if (!gpa_is_ok(pa)) {
                LOG_ERR("Cannot add, bad PA");
                return -1;
        }
        if (!gha_is_ok(ha)) {
                LOG_ERR("Cannot add, bad HA");
                return -1;
        }

        cl = tbls_find(ptype);
        if (!cl)
                return -1;

        return tbl_add(cl, gpa_dup(pa), gha_dup(ha));
}
EXPORT_SYMBOL(arp826_add);

int arp826_remove(uint16_t           ptype,
                  const struct gpa * pa,
                  const struct gha * ha)
{
        struct table *             cl;
        const struct table_entry * ce;

        if (!gpa_is_ok(pa)) {
                LOG_ERR("Cannot remove, bad PA");
                return -1;
        }
        if (!gha_is_ok(ha)) {
                LOG_ERR("Cannot remove, bad HA");
                return -1;
        }

        cl = tbls_find(ptype);
        if (!cl)
                return -1;

        ce = tbl_find(cl, pa, ha);
        if (!ce)
                return -1;

        tbl_remove(cl, ce);

        return 0;
}
EXPORT_SYMBOL(arp826_remove);

const struct gpa * arp826_find_gpa(uint16_t           ptype,
                                   const struct gha * ha)
{
        struct table *             cl;
        const struct table_entry * ce;

        if (!gha_is_ok(ha)) {
                LOG_ERR("Cannot resolve, bad HA");
                return NULL;
        }

        cl = tbls_find(ptype);
        if (!cl)
                return NULL;

        ce = tbl_find_by_gha(cl, ha);
        if (!ce)
                return NULL;

        return tble_pa(ce);
}
EXPORT_SYMBOL(arp826_find_gpa);
