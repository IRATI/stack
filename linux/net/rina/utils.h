/*
 * Utilities
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

#ifndef RINA_UTILS_H
#define RINA_UTILS_H

#include <linux/kobject.h>
#define RINA_ATTR_RO(NAME)                              \
        static struct kobj_attribute NAME##_attr =      \
                __ATTR_RO(NAME)

#define RINA_ATTR_RW(NAME)                                      \
        static struct kobj_attribute NAME##_attr =              \
                __ATTR(NAME, 0644, NAME##show, NAME##store)

#include <linux/string.h>

#define bzero(DEST, LEN) do { (void) memset(DEST, 0, LEN); } while (0)

int    is_value_in_range(int value, int min_value, int max_value);

/* Memory */
#include <linux/slab.h>

void * rkmalloc(size_t size, gfp_t flags);
void * rkzalloc(size_t size, gfp_t flags);
void   rkfree(void * ptr);

/* Syscalls */
char * strdup_from_user(const char __user * src);

/* Workqueues */
#include <linux/workqueue.h>

struct workqueue_struct * rwq_create(const char * name);
int                       rwq_destroy(struct workqueue_struct * rwq);

/*
 * NOTE: The worker is the owner of the data passed (and must dispose it). It
 *       must return 0 if its work completed successfully.
 */
int                       rwq_post(struct workqueue_struct * rwq,
                                   int                       (* job)(void * o),
                                   void *                    data);

/* RMAP */
#define RMAP_BITS_DEFAULT          7 /* Change at your own risk */
#define __RMAP_HASH(TABLE, KEY)    hash_min(KEY, HASH_BITS(TABLE))
#define RMAP_TYPE(PREFIX)          struct PREFIX##_map
#define RMAP_ITERATOR_TYPE(PREFIX) struct PREFIX##_map_iterator

#define DECLARE_RMAP(PREFIX, BITS)              \
        RMAP_TYPE(PREFIX) {                     \
                DECLARE_HASHTABLE(table, BITS); \
        };

#define DEFINE_RMAP_ITERATOR(PREFIX, KEY_TYPE, VALUE_TYPE)      \
        struct PREFIX##_map_iterator {                          \
                KEY_TYPE          key;                          \
                VALUE_TYPE *      value;                        \
                struct hlist_node hlist;                        \
        };

#define DEFINE_RMAP_CREATE(PREFIX)                              \
        RMAP_TYPE(PREFIX) * PREFIX##_create(void)               \
        {                                                       \
                RMAP_TYPE(PREFIX) * tmp;                        \
                                                                \
                tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);       \
                if (!tmp)                                       \
                        return NULL;                            \
                                                                \
                hash_init(tmp->table);                          \
                                                                \
                return tmp;                                     \
        }

#define DEFINE_RMAP_EMPTY(PREFIX)                       \
        int PREFIX_##_empty(RMAP_TYPE(PREFIX) * map))   \
        {                                               \
                ASSERT(map);                            \
                                                        \
                return hash_empty(map->table);          \
        }

#define DEFINE_RMAP_DESTROY(PREFIX, VALUE_TYPE)                         \
        int PREFIX##_destroy(RMAP_TYPE(PREFIX) * map,                   \
                             void             (* dtor)(VALUE_TYPE * value)) \
        {                                                               \
                RMAP_ITERATOR_TYPE(PREFIX) *  iter;                     \
                struct hlist_node *           tmp;                      \
                int                           bucket;                   \
                                                                        \
                ASSERT(map);                                            \
                                                                        \
                hash_for_each_safe(map->table, bucket, tmp, iter, hlist) { \
                        hash_del(&iter->hlist);                         \
                        if (dtor) dtor(iter->value);                    \
                        rkfree(iter);                                   \
                }                                                       \
                                                                        \
                rkfree(map);                                            \
                                                                        \
                return 0;                                               \
        }

#define DEFINE_RMAP_FIND(PREFIX)                                        \
        static RMAP_ITERATOR_TYPE(PREFIX) * rmap_find(RMAP_TYPE(PREFIX) * map, \
                                                      KEY_TYPE            key) \
        {                                                               \
                RMAP_ITERATOR_TYPE(PREFIX) * iter;                      \
                struct hlist_head *          head;                      \
                                                                        \
                ASSERT(map);                                            \
                                                                        \
                head = &map->table[__RMAP_HASH(map->table, key)];       \
                hlist_for_each_entry(iter, head, hlist) {               \
                        if (iter->key == key)                           \
                                return iter;                            \
                }                                                       \
                                                                        \
                return NULL;                                            \
        }

#define DEFINE_RMAP_UPDATE(PREFIX)                      \
        int rmap_update(RMAP_TYPE(PREFIX) * map,        \
                        KEY_TYPE            key,        \
                        VALUE_TYPE          value)      \
        {                                               \
                RMAP_ITERATOR_TYPE(PREFIX) * cur;       \
                                                        \
                ASSERT(map);                            \
                                                        \
                cur = PREFIX##_find(map, key);          \
                if (!cur)                               \
                        return -1;                      \
                                                        \
                cur->value = value;                     \
                                                        \
                return 0;                               \
        }

#define DEFINE_RMAP_ADD(PREFIX)                                 \
        int PREFIX##_add(RMAP_TYPE(PREFIX) * map,               \
                         KEY_TYPE            key,               \
                         VALUE_TYPE *        value)             \
        {                                                       \
                RMAP_ITERATOR_TYPE(PREFIX) * tmp;               \
                                                                \
                ASSERT(map);                                    \
                                                                \
                tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);       \
                if (!tmp)                                       \
                        return -1;                              \
                                                                \
                tmp->key   = key;                               \
                tmp->value = value;                             \
                INIT_HLIST_NODE(&tmp->hlist);                   \
                                                                \
                hash_add(map->table, &tmp->hlist, key);         \
                                                                \
                return 0;                                       \
        }

#define RMAP_REMOVE(PREFIX)                             \
        int PREFIX##_remove(RMAP_TYPE(PREFIX) * map,    \
                            TYPE_KEY            key)    \
        {                                               \
                RMAP_ITERATOR_TYPE(PREFIX) * iter;      \
                                                        \
                ASSERT(map);                            \
                                                        \
                iter = PREFIX_##_find(map, key);        \
                if (!iter)                              \
                        return -1;                      \
                                                        \
                hash_del(&iter->hlist);                 \
                rkfree(iter);                           \
                                                        \
                return 0;                               \
        }

#define RMAP_ERASE(PREFIX)                                      \
        int PREFIX##_erase(RMAP_TYPE(PREFIX) *          map,    \
                           RMAP_ITERATOR_TYPE(PREFIX) * iter)   \
        {                                                       \
                ASSERT(map);                                    \
                                                                \
                hash_del(&iter->hlist);                         \
                rkfree(iter);                                   \
                                                                \
                return 0;                                       \
        }

#define RMAP_FIND(PREFIX)                                               \
        RMAP_ITERATOR_TYPE(PREFIX) * PREFIX##find(RMAP_TYPE(PREFIX) * map, \
                                                  TYPE_KEY            key) \
        {                                                               \
                RMAP_ITERATOR_TYPE(PREFIX) * iter;                      \
                                                                        \
                ASSERT(map);                                            \
                                                                        \
                iter = PREFIX##_find(map, key);                         \
                return iter;                                            \
        }

#endif
