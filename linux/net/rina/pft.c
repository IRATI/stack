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

        if (is_port_id_ok(port_id))
                return NULL;

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
        if (!pft_pe_is_ok(pe))
                return -1;

        list_del(&pe->next);
        rkfree(pe);

        return 0;
}

static port_id_t pft_pe_port(struct pft_port_entry * pe)
{
        if (!pft_pe_is_ok(pe))
                return port_id_bad();

        return pe->port_id;
}

/*
 * FIXME: This representation is crappy and MUST be changed
 */
struct pft_entry {
        address_t destination;
        qos_id_t  qos_id;
        size_t    ports_size;     
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
        tmp->ports_size  = 0;
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

        if (!pft_e_is_ok(entry))
                return -1;
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

        if (!pft_e_is_ok(entry))
                return -1;

        pe = pft_e_port_find(entry, id);
        if (pe)
                return 0;

        pe = pft_pe_create(id);
        if (!pe)
                return -1;

        list_add(&pe->next, &entry->ports);
        entry->ports_size++;

        return 0;
}


static int pft_e_port_remove(struct pft_entry * entry,
                             port_id_t          id)
{
        struct pft_port_entry * pos, * nxt;
        int                     ret;

        /* Remove the port-id here */
        list_for_each_entry_safe(pos, nxt, &entry->ports, next) {
                if (pft_pe_port(pos) == id) {
                        ret = pft_pe_destroy(pos);
                        if (!ret) {
                                LOG_WARN("Could not destroy PDU-FWD-T"
                                         "entry %pK", pos);
                                return ret;
                        }
                        entry->ports_size--;
                }
        }

        /* If the list of port-ids is empty, remove the entry */
        if (entry->ports_size == 0) {
                if(pft_e_destroy(entry)) {
                        LOG_ERR("Failed to cleanup entry");
                        return -1;
                }
        }

        return 0;
}


/*
 * FIXME: This representation is crappy and MUST be changed
 */
struct pft {
        struct list_head entries;
};

struct pft * pft_create_gfp(gfp_t flags)
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
        struct pft_entry *      e;
        struct pft_port_entry * pos, * nxt;
        int i;

        if (!pft_is_ok(instance))
                return -1;

        e = pft_find(instance, destination, qos_id);
        if (!e) {
                LOG_ERR("Could not find any PFT entry");
                return -1;
        }
        
        if (*size < e->ports_size) {
                rkfree(*port_ids);
                *port_ids = 
                        rkzalloc(e->ports_size * sizeof(**port_ids), GFP_KERNEL);
                if (!*port_ids) {
                        LOG_ERR("Could not allocate memory "
                                "to return resulting ports");
                        return -1;
                }
        }
        *size = e->ports_size;

        /* Get the first port, and so on, fill in the port_ids */
        i = 0;
        list_for_each_entry_safe(pos, nxt, &e->ports, next) {
                *port_ids[i] = pft_pe_port(pos);
                ++i;
        }

        return 0;
}
