/*
 * PDU-FWD-T (PDU Forwarding Table)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#include <linux/list.h>
#include <linux/slab.h>

#define RINA_PREFIX "pft"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "pft.h"

/*
 * FIXME: This representation is crappy and MUST be changed
 */
struct pft_port_entry {
        port_id_t        port_id;

        struct list_head next;
};

static struct pft_port_entry * pft_pe_create_gfp(gfp_t     flags,
                                                 port_id_t port_id)
{
        struct pft_port_entry * tmp;

        ASSERT(is_port_id_ok(port_id));
        ASSERT(flags);
              
        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->port_id = port_id;
        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

#if 0
static struct pft_port_entry * pft_pe_create_ni(port_id_t port_id)
{ return pft_pe_create_gfp(GFP_ATOMIC, port_id); }
#endif

static struct pft_port_entry * pft_pe_create(port_id_t port_id)
{ return pft_pe_create_gfp(GFP_KERNEL, port_id); }

static bool pft_pe_is_ok(struct pft_port_entry * pe)
{ return pe ? true : false;  }

static int pft_pe_destroy(struct pft_port_entry * pe)
{
        ASSERT(pft_pe_is_ok(pe));
               
        list_del(&pe->next);
        rkfree(pe);

        return 0;
}

static port_id_t pft_pe_port(struct pft_port_entry * pe)
{
        ASSERT(pft_pe_is_ok(pe));

        return pe->port_id;
}

/*
 * FIXME: This representation is crappy and MUST be changed
 */
struct pft_entry {
        address_t destination;
        qos_id_t  qos_id;   
        struct    list_head ports;
        struct    list_head next;
};

static struct pft_entry * pft_e_create_gfp(gfp_t     flags,
                                           address_t destination,
                                           qos_id_t  qos_id)
{
        struct pft_entry * tmp;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->destination = destination;
        tmp->qos_id      = qos_id;
        INIT_LIST_HEAD(&tmp->ports);
        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

#if 0
static struct pft_entry * pft_e_create_ni(address_t destination,
                                          qos_id_t  qos_id)
{ return pft_e_create_gfp(GFP_ATOMIC, destination, qos_id); }
#endif

static struct pft_entry * pft_e_create(address_t destination,
                                       qos_id_t  qos_id)
{ return pft_e_create_gfp(GFP_KERNEL, destination, qos_id); }

static bool pft_e_is_ok(struct pft_entry * entry)
{ return entry ? true : false; }

static int pft_e_destroy(struct pft_entry * entry)
{
        struct pft_port_entry * pos, * nxt;
        int                     ret;

        ASSERT(pft_e_is_ok(entry));

        list_for_each_entry_safe(pos, nxt, &entry->ports, next) {
                ret = pft_pe_destroy(pos);
                if (!ret) {
                        LOG_WARN("Could not destroy PDU-FWD-T entry %pK", pos);
                        return ret;
                }
        }

        list_del(&entry->next);
        rkfree(entry);

        return 0;

}

static struct pft_port_entry * pft_e_port_find(struct pft_entry * entry,
                                               port_id_t          id)
{
        struct pft_port_entry * pos;

        ASSERT(pft_e_is_ok(entry));

        list_for_each_entry(pos, &entry->ports, next) {
                if (pos->port_id == id)
                        return pos;
        }

        return NULL;
}

static int pft_e_port_add(struct pft_entry * entry,
                          port_id_t          id)
{
        struct pft_port_entry * pe;

        ASSERT(pft_e_is_ok(entry));

        pe = pft_e_port_find(entry, id);
        if (pe)
                return 0;

        pe = pft_pe_create(id);
        if (!pe)
                return -1;

        list_add(&pe->next, &entry->ports);

        return 0;
}


static int pft_e_port_remove(struct pft_entry * entry,
                             port_id_t          id)
{
        struct pft_port_entry * pos, * nxt;
        int                     ret;

        ASSERT(pft_e_is_ok(entry));
        ASSERT(is_port_id_ok(id));

        /* Remove the port-id here */
        list_for_each_entry_safe(pos, nxt, &entry->ports, next) {
                if (pft_pe_port(pos) == id) {
                        ret = pft_pe_destroy(pos);
                        if (!ret) {
                                LOG_WARN("Could not destroy PDU-FWD-T"
                                         "entry %pK", pos);
                                return ret;
                        }
                }
        }

        /* If the list of port-ids is empty, remove the entry */
        if (list_empty(&entry->ports)) {
                if(pft_e_destroy(entry)) {
                        LOG_ERR("Failed to cleanup entry");
                        return -1;
                }
        }

        return 0;
}

static int pft_e_ports(struct pft_entry * entry,
                       port_id_t **       port_ids,
                       size_t *           size)
{
        struct pft_port_entry * pos, * nxt;
        size_t                  ports_size;
        int                     i;

        ASSERT(pft_e_is_ok(entry));
        ASSERT(*size);

        ports_size = 0;
        list_for_each_entry_safe(pos, nxt, &entry->ports, next) {
                ++ports_size;
        }
        
        if (*size != ports_size) {
                if (*size > 0)
                        rkfree(*port_ids);
                *port_ids = 
                        rkzalloc(ports_size * sizeof(**port_ids), GFP_KERNEL);
                if (!*port_ids) {
                        LOG_ERR("Could not allocate memory "
                                "to return resulting ports");
                        *size = 0;
                        return -1;
                }
                *size = ports_size;
        }

        /* Get the first port, and so on, fill in the port_ids */
        i = 0;
        list_for_each_entry_safe(pos, nxt, &entry->ports, next) {
                *port_ids[i] = pft_pe_port(pos);
                ++i;
        }

        return 0;
}

/*
 * FIXME: This representation is crappy and MUST be changed
 */
struct pft {
        struct list_head entries;
};

static struct pft * pft_create_gfp(gfp_t flags)
{
        struct pft * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->entries);

        return tmp;
}

struct pft * pft_create_ni(void)
{ return pft_create_gfp(GFP_ATOMIC); }

struct pft * pft_create(void)
{ return pft_create_gfp(GFP_KERNEL); }

static bool pft_is_ok(struct pft * instance)
{ return instance ? true : false; }

bool pft_is_empty(struct pft * instance)
{
        if (!pft_is_ok(instance))
                return false;
        return list_empty(&instance->entries);
}

static int __pft_flush(struct pft * instance)
{
        struct pft_entry * pos, * nxt;
        int                ret;

        ASSERT(pft_is_ok(instance));

        list_for_each_entry_safe(pos, nxt, &instance->entries, next) {
                list_del(&pos->next);
                ret = pft_e_destroy(pos);
                if (!ret) {
                        LOG_WARN("Could not destroy PDU-FWD-T entry %pK", pos);
                        return ret;
                }
        }

        return 0;
}

int pft_flush(struct pft * instance)
{
        if (!pft_is_ok(instance))
                return -1;

        return __pft_flush(instance);
}

int pft_destroy(struct pft * instance)
{
        int ret;

        if (!pft_is_ok(instance))
                return -1;

        ret = __pft_flush(instance);
        if (ret)
                return ret;

        rkfree(instance);

        return 0;
}

static struct pft_entry * pft_find(struct pft * instance,
                                   address_t    destination,
                                   qos_id_t     qos_id)
{
        struct pft_entry * pos;

        ASSERT(pft_is_ok(instance));

        list_for_each_entry(pos, &instance->entries, next) {
                if ((pos->destination == destination) &&
                    (pos->qos_id      == qos_id))
                        return pos;
        }

        return NULL;
}

int pft_add(struct pft * instance,
            address_t    destination,
            qos_id_t     qos_id,
            port_id_t    port_id)
{
        struct pft_entry * tmp;

        if (!pft_is_ok(instance))
                return -1;

        tmp = pft_find(instance, destination, qos_id);
        /* Create a new entry? */
        if (!tmp) {
                tmp = pft_e_create(destination, qos_id);
                if (!tmp)
                        return -1;
                list_add(&tmp->next, &instance->entries);
        }

        if (pft_e_port_add(tmp, port_id)) {
                pft_e_destroy(tmp);
                return -1;
        }

        return 0;
}

int pft_remove(struct pft * instance,
               address_t    destination,
               qos_id_t     qos_id,
               port_id_t    port_id)
{
        struct pft_entry *      tmp;

        if (!pft_is_ok(instance))
                return -1;

        tmp = pft_find(instance, destination, qos_id);
        if (!tmp)
                return -1;

        if (pft_e_port_remove(tmp, port_id)) {
                LOG_ERR("Failed to remove port");
                return -1;
        }

        return 0;
}

int pft_nhop(struct pft * instance,
             address_t    destination,
             qos_id_t     qos_id,
             port_id_t ** port_ids,
             size_t *     size)
{
        struct pft_entry * tmp;


        if (!pft_is_ok(instance))
                return -1;

        tmp = pft_find(instance, destination, qos_id);
        if (!tmp) {
                LOG_ERR("Could not find any PFT entry");
                return -1;
        }

        if (pft_e_ports(tmp, port_ids, size)) {
                LOG_ERR("Failed to get ports");
                return -1;
        }

        return 0;
}


#ifdef CONFIG_RINA_PFT_REGRESSION_TESTS
static bool regression_test_pft_nhop(void)
{
        struct pft *       tmp;
        port_id_t *        port_ids;
        size_t             nr;
 
        tmp = pft_create();
        if (!tmp) {
                LOG_DBG("Failed to create pft instance");
                return false;
        }

        if (pft_add(tmp, 30, 2, 2)) {
                LOG_DBG("Failed to add entry");
                return false;
        }
        
        if (pft_add(tmp, 30, 2, 99)) {
                LOG_DBG("Failed to add entry");
                return false;
        }

        nr = 0;
        if (pft_nhop(tmp, 30, 2, &port_ids, &nr)) {
                LOG_DBG("Failed to get port-ids");
                return false;
        }

        if (nr != 2) {
                LOG_DBG("Wrong number of port-ids returned");
                return false;
        }

        if (port_ids[0] != 2) {
                LOG_DBG("Wrong port-id returned");
                return false;
        }
        
        if (port_ids[1] != 99) {
                LOG_DBG("Wrong port-id returned");
                return false;
        }

        if (pft_flush(tmp)) {
                LOG_DBG("Failed to flush table");
                return false;
        }

        /* Port-id table is now 2 in size */
        if (pft_add(tmp, 30, 2, 2)) {
                LOG_DBG("Failed to add entry");
                return false;
        }
        
        if (pft_add(tmp, 30, 2, 99)) {
                LOG_DBG("Failed to add entry");
                return false;
        }

        if (pft_nhop(tmp, 30, 2, &port_ids, &nr)) {
                LOG_DBG("Failed to get port-ids");
                return false;
        }

        if (nr != 2) {
                LOG_DBG("Wrong number of port-ids returned");
                return false;
        }

        if (pft_flush(tmp)) {
                LOG_DBG("Failed to flush table");
                return false;
        }

        /* Trying with 1 port-id */

        if (pft_add(tmp, 30, 2, 2)) {
                LOG_DBG("Failed to add entry");
                return false;
        }        
        
        if (pft_nhop(tmp, 30, 2, &port_ids, &nr)) {
                LOG_DBG("Failed to get port-ids");
                return false;
        }

        if (nr != 1) {
                LOG_DBG("Wrong number of port-ids returned");
                return false;
        }


        if (pft_flush(tmp)) {
                LOG_DBG("Failed to flush table");
                return false;
        }

        /* Trying with 3 port-ids */

        if (pft_add(tmp, 30, 2, 2)) {
                LOG_DBG("Failed to add entry");
                return false;
        }
        
        if (pft_add(tmp, 30, 2, 99)) {
                LOG_DBG("Failed to add entry");
                return false;
        }

        if (pft_add(tmp, 30, 2, 9)) {
                LOG_DBG("Failed to add entry");
                return false;
        }


        if (pft_nhop(tmp, 30, 2, &port_ids, &nr)) {
                LOG_DBG("Failed to get port-ids");
                return false;
        }

        if (nr != 3) {
                LOG_DBG("Wrong number of port-ids returned");
                return false;
        }

        if (pft_destroy(tmp)) {
                LOG_DBG("Failed to destroy instance");
                return false;
        }
        
        return true;
}

static bool regression_test_add_remove_pft_entries(void)
{
        struct pft * tmp;
        struct pft_entry * e;
 
        tmp = pft_create();
        if (!tmp) {
                LOG_DBG("Failed to create pft instance");
                return false;
        }
        
        if (pft_add(tmp, 16, 1, 1)) {
                LOG_DBG("Failed to add entry");
                return false;
        }
        
        e = pft_find(tmp, 16,1);
        if (!e) {
                LOG_DBG("Failed to retrieve stored entry");
                return false;
        }


        if (pft_remove(tmp, 16, 1, 1)) {
                LOG_DBG("Failed to remove entry");
                return false;
        }

        if (!pft_remove(tmp, 16, 1, 1)) {
                LOG_DBG("Entry should have already been removed");
                return false;
        }

        if (!pft_remove(tmp, 35, 4, 6)) {
                LOG_DBG("No such entry was added");
                return false;
        }

        if (pft_add(tmp, 30, 2, 2)) {
                LOG_DBG("Failed to add entry");
                return false;
        }
        
        if (pft_add(tmp, 35, 5, 99)) {
                LOG_DBG("Failed to add entry");
                return false;
        }

        if (pft_flush(tmp)) {
                LOG_DBG("Failed to flush table");
                return false;
        }

        if (pft_destroy(tmp)) {
                LOG_DBG("Failed to destroy instance");
                return false;
        }

        return true;
}

static bool regression_test_create_pft_instance(void) 
{
        struct pft * tmp;

        tmp = pft_create();
        if (!tmp) {
                LOG_DBG("Failed to create pft instance");
                return false;
        }
        
        if (pft_destroy(tmp)) {
                LOG_DBG("Failed to destroy instance");
                return false;
        }

        return true;
}

static bool regression_tests(void)
{
        if (!regression_test_create_pft_instance()) {
                LOG_ERR("Creating of a pft instance test failed, "
                        "bailing out");
                return false;
        }

        if (!regression_test_add_remove_pft_entries()) {
                LOG_ERR("Adding/removing pft entries test failed, "
                        "bailing out");
                return false;
        }

        if (!regression_test_pft_nhop()) {
                LOG_ERR("Pft_nhop operation is crap, "
                        "bailing out");
                return false;
        }

        return true;
}
#endif
