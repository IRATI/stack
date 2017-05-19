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

#include <linux/types.h>
#include <linux/netdevice.h>

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-tables"

#include "logs.h"
#include "common.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826.h"
#include "arp826-utils.h"
#include "arp826-maps.h"
#include "arp826-tables.h"

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

int tble_destroy(struct table_entry * entry)
{
        if (!entry) {
                LOG_ERR("Bogus table entry, cannot destroy");
                return -1;
        }

        tble_fini(entry);
        rkfree(entry);

        return 0;
}
EXPORT_SYMBOL(tble_destroy);

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

static struct table_entry * tble_create_gfp(gfp_t        flags,
                                            struct gpa * gpa,
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

        LOG_DBG("Creating new table entry");

        entry = rkmalloc(sizeof(*entry), flags);
        if (!entry)
                return NULL;

        if (tble_init(entry,
                      gpa_dup_gfp(flags, gpa),
                      gha_dup_gfp(flags, gha))) {
                rkfree(entry);
                return NULL;
        }

        LOG_DBG("Table entry %pK created successfully", entry);

        return entry;
}

struct table_entry * tble_create(struct gpa * gpa,
                                 struct gha * gha)
{ return tble_create_gfp(GFP_KERNEL, gpa, gha); }
EXPORT_SYMBOL(tble_create);

struct table_entry * tble_create_ni(struct gpa * gpa,
                                    struct gha * gha)
{ return tble_create_gfp(GFP_ATOMIC, gpa, gha); }
EXPORT_SYMBOL(tble_create_ni);

bool tble_is_ok(const struct table_entry * entry)
{
        return (entry == NULL         ||
                !gpa_is_ok(entry->pa) ||
                !gha_is_ok(entry->ha)) ? false : true;
}

bool tble_is_equal(const struct table_entry * entry1,
                   const struct table_entry * entry2)
{
        if (!tble_is_ok(entry1))
                return false;
        if (!tble_is_ok(entry2))
                return false;

        if (!gpa_is_equal(entry1->pa, entry2->pa))
                return false;
        if (!gha_is_equal(entry1->ha, entry2->ha))
                return false;

        return true;
}

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

static struct table * tbl_create_gfp(gfp_t  flags,
                                     size_t ha_length)
{
        struct table * instance;

        LOG_DBG("Creating tbl instance for ha-len %zd", ha_length);

        if (ha_length == 0) {
                LOG_ERR("Bad HA length, cannot create table");
                return NULL;
        }

        instance = rkmalloc(sizeof(*instance), flags);
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
                      struct gha *       ha,
                      gfp_t              flags)
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
                        pos->ha = gha_dup_gfp(flags, ha);
                        spin_unlock(&instance->lock);
                        return 0;
                }
        }

        spin_unlock(&instance->lock);

        return -1;
}

struct table_entry * tbl_find(struct table *     instance,
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

struct table_entry * tbl_find_by_gha(struct table *     instance,
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

struct table_entry * tbl_find_by_gpa(struct table *     instance,
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

        LOG_DBG("Showing addresses in table in the meanwhile");

        spin_lock(&instance->lock);
        list_for_each_entry(pos, &instance->entries, next) {
                gpa_dump(pos->pa);
                if (gpa_is_equal(pos->pa, address)) {
                        LOG_DBG("That's the address I need");
                        spin_unlock(&instance->lock);
                        return pos;
                }
        }
        spin_unlock(&instance->lock);

        LOG_DBG("Got no matching address");

        return NULL;
}

int tbl_add(struct table *       instance,
            struct table_entry * entry)
{
        struct table_entry * pos;

        if (!instance) {
                LOG_ERR("Bogus instance, cannot add entry to table");
                return -1;
        }
        if (!entry) {
                LOG_ERR("Bogus entry, cannot add it to table");
                return -1;
        }

        LOG_DBG("Adding entry %pK to the table", entry);

        spin_lock(&instance->lock);

        list_for_each_entry(pos, &instance->entries, next) {
                if (tble_is_equal(pos, entry)) {
                        LOG_WARN("We already have an equal entry ...");
                        spin_unlock(&instance->lock);
                        return 0;
                }

                /* FIXME: What about the other conditions ??? */
                if (gha_is_equal(tble_ha(pos), tble_ha(entry))) {
                        /* FIXME: What should we do here? */

                        /* Remember to: spin_unlock(&instance->lock); */
                }

                if (gpa_is_equal(tble_pa(pos), tble_pa(entry))) {
                        LOG_WARN("We already have the same GPA in the cache");

                        /* FIXME: What should we do here? */

                        /* Remember to: spin_unlock(&instance->lock); */
                }
        }

        list_add(&entry->next, &instance->entries);

        spin_unlock(&instance->lock);

        LOG_DBG("Entry %pK added successfully to the table", entry);

        return 0;
}

int tbl_remove(struct table *             instance,
               const struct table_entry * entry)
{
        struct table_entry * pos, * q;

        if (!instance) {
                LOG_ERR("Bogus instance, cannot remove entry from table");
                return -1;
        }
        if (!entry) {
                LOG_ERR("Bogus entry, cannot remove entry from table");
                return -1;
        }

        spin_lock(&instance->lock);

        list_for_each_entry_safe(pos, q, &instance->entries, next) {
                if (pos == entry) {
                        list_del(&pos->next);
                        spin_unlock(&instance->lock);
                        return 0;
                }
        }

        spin_unlock(&instance->lock);

        return -1;
}

static DEFINE_SPINLOCK(tables_lock);
static struct tmap * tables = NULL;

struct table * tbls_find(struct net_device * device, uint16_t ptype)
{
        struct tmap_entry * e;

        if (!device)
                return NULL;

        spin_lock_bh(&tables_lock);

        e = tmap_entry_find(tables, device, ptype);
        if (!e) {
                spin_unlock_bh(&tables_lock);
                return NULL;
        }

        spin_unlock_bh(&tables_lock);

        return tmap_entry_value(e);
}

static struct table * tbls_create_gfp(gfp_t               flags,
                                      struct net_device * device,
                                      uint16_t            ptype,
                                      size_t              hwlen)
{
        struct table * cl;

        cl = tbls_find(device, ptype);
        if (cl) {
                LOG_ERR("Table for ptype 0x%04X already created", ptype);
                return NULL;
        }

        cl = tbl_create_gfp(flags, hwlen);
        if (!cl) {
                LOG_ERR("Cannot create table for ptype 0x%04X, hwlen %zd",
                        ptype, hwlen);
                return NULL;
        }

        LOG_DBG("Now adding the new table to the tables map");

        spin_lock_bh(&tables_lock);
        if (tmap_entry_add_ni(tables, device, ptype, cl)) {
                spin_unlock_bh(&tables_lock);

                tbl_destroy(cl);

                return NULL;
        }
        spin_unlock_bh(&tables_lock);

        LOG_DBG("Table for created successfully "
                "(device = %pK, ptype = 0x%04X, hwlen = %zd)",
                device, ptype, hwlen);

        return cl;
}

struct table * tbls_create_ni(struct net_device * device,
                              uint16_t            ptype,
                              size_t              hwlen)
{ return tbls_create_gfp(GFP_ATOMIC, device, ptype, hwlen); }

struct table * tbls_create(struct net_device * device,
                           uint16_t            ptype,
                           size_t              hwlen)
{ return tbls_create_gfp(GFP_KERNEL, device, ptype, hwlen); }

int tbls_destroy(struct net_device * device, uint16_t ptype)
{
        struct tmap_entry * e;
        struct table *      cl;

        spin_lock(&tables_lock);

        e = tmap_entry_find(tables, device, ptype);
        if (!e) {
                LOG_DBG("Table for ptype 0x%04X is missing, cannot destroy",
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
        if (tables) {
                LOG_WARN("Tables already initialized, bailing out");
                return 0;
        }

        tables = tmap_create();
        if (!tables)
                return -1;

        spin_lock_init(&tables_lock);

        LOG_INFO("ARP826 tables initialized successfully");

        return 0;
}

void tbls_fini(void)
{
        if (!tables) {
                LOG_WARN("Already finalized, bailing out ...");
                return;
        }

        ASSERT(tmap_is_empty(tables));

        tmap_destroy(tables);

        tables = NULL;

        LOG_INFO("ARP826 tables finalized successfully");
}

int arp826_add(struct net_device * device,
               uint16_t            ptype,
               const struct gpa *  pa,
               const struct gha *  ha)
{
        struct table *       cl;
        struct table_entry * e;
        struct gpa *         tmp_pa;
        struct gha *         tmp_ha;

        if (!gpa_is_ok(pa)) {
                LOG_ERR("Cannot add, bad PA");
                return -1;
        }
        if (!gha_is_ok(ha)) {
                LOG_ERR("Cannot add, bad HA");
                return -1;
        }

        LOG_DBG("Adding GPA/GHA couple to the 0x%04x ptype table", ptype);

        cl = tbls_find(device, ptype);
        if (!cl) {
                LOG_DBG("No table exists for this device yet, creating it");
                cl = tbls_create(device, ETH_P_RINA, 6);
                if (cl == NULL) {
                           LOG_ERR("Cannot create a new table");
                           return -1;
                }
        }

        tmp_pa = gpa_dup_gfp(GFP_ATOMIC, pa);
        if (!tmp_pa)
                return -1;

        tmp_ha = gha_dup_gfp(GFP_ATOMIC, ha);
        if (!tmp_ha) {
                gpa_destroy(tmp_pa);
                return -1;
        }

        LOG_DBG("Creating a new table entry for this ARP-add request");

        e = tble_create_gfp(GFP_ATOMIC, tmp_pa, tmp_ha);
        if (!e) {
                gpa_destroy(tmp_pa);
                gha_destroy(tmp_ha);
                return -1;
        }
        gpa_destroy(tmp_pa);
        gha_destroy(tmp_ha);

        LOG_DBG("Adding the GPA/GHA entry to the 0x%x04 table", ptype);

        if (tbl_add(cl, e)) {
                LOG_ERR("Cannot add to the 0x%x04 table, rolling back", ptype);
                tble_destroy(e);
                return -1;
        }

        LOG_DBG("GPA/GHA couple for ptype 0x%04x added successfully", ptype);
        return 0;
}
EXPORT_SYMBOL(arp826_add);

int arp826_remove(struct net_device * device,
                  uint16_t            ptype,
                  const struct gpa *  pa,
                  const struct gha *  ha)
{
        struct table *       cl;
        struct table_entry * ce;

        if (!gpa_is_ok(pa)) {
                LOG_ERR("Cannot remove, bad PA");
                return -1;
        }
        if (!gha_is_ok(ha)) {
                LOG_ERR("Cannot remove, bad HA");
                return -1;
        }

        cl = tbls_find(device, ptype);
        if (!cl)
                return -1;

        ce = tbl_find(cl, pa, ha);
        if (!ce)
                return -1;

        if (tbl_remove(cl, ce))
                return -1;

        tble_destroy(ce);

        return 0;
}
EXPORT_SYMBOL(arp826_remove);

const struct gpa * arp826_find_gpa(struct net_device * device,
                                   uint16_t            ptype,
                                   const struct gha *  ha)
{
        struct table *             cl;
        const struct table_entry * ce;

        if (!gha_is_ok(ha)) {
                LOG_ERR("Cannot resolve, bad HA");
                return NULL;
        }

        cl = tbls_find(device, ptype);
        if (!cl)
                return NULL;

        ce = tbl_find_by_gha(cl, ha);
        if (!ce)
                return NULL;

        return tble_pa(ce);
}
EXPORT_SYMBOL(arp826_find_gpa);
