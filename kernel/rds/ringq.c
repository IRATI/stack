/*
 * RINA RING QUEUES
 *
 *    Miquel Tarzan <miquel.tarzan@i2cat.net>
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

#include <linux/export.h>
#include <linux/list.h>
#include <linux/types.h>

#define RINA_PREFIX "ringq"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "ringq.h"

struct ringq_entry {
	struct list_head list;
	void * data;
};

/* The crappiest implementation we could have ... */
struct ringq {
	struct list_head     head;
	struct ringq_entry * cur;
	size_t		     length;
	size_t		     occupation;
	spinlock_t           lock;
	int (* comp)(void *x, void *y);
	void (* add)(void *x, void *y);
};

static void entry_destroy(struct ringq_entry * entry, void (* dtor)(void * e))
{
	if (dtor && entry->data)
		dtor(entry->data);

	list_del(&entry->list);
	rkfree(entry);

	return;
}

static void q_destroy(struct ringq * q, void (* dtor)(void * e))
{
	struct ringq_entry * pos, * nxt;

	spin_lock_bh(&q->lock);
	list_for_each_entry_safe(pos, nxt, &q->head, list) {
		entry_destroy(pos, dtor);
	}
	spin_unlock_bh(&q->lock);
	rkfree(q);

	return;
}

static struct ringq * ringq_create_gfp(gfp_t flags, unsigned int length, size_t size)
{
        struct ringq * q;
        int i;

        q = rkzalloc(sizeof(*q), flags);
        if (!q)
                return NULL;

        INIT_LIST_HEAD(&q->head);
        spin_lock_init(&q->lock);

        if (!size) {
                for(i=0; i<length; i++) {
                	struct ringq_entry * entry = rkzalloc(sizeof(*entry), flags);

                	if (!entry) {
                		q_destroy(q, NULL);
                		return NULL;
                	}
                	INIT_LIST_HEAD(&entry->list);
                	list_add(&entry->list, &q->head);
                }
                q->length     = length;
                q->occupation = 0;
        	q->cur	      = list_first_entry(&q->head, struct ringq_entry, list);
        	if (!q->cur) {
        		LOG_ERR("Could not initialize RINGQ");
        		q_destroy(q, NULL);
        		return NULL;
        	}
        	q->comp = NULL;
        	q->add  = NULL;

                LOG_DBG("RINGQ %pK:%pK:%pK:%pK created successfully", q, &q->head, q->cur, &q->cur->list);

                return q;
        }
        if (size > 0) {
                for(i=0; i<length; i++) {
                	struct ringq_entry * entry = rkzalloc(sizeof(*entry), flags);

                	if (!entry) {
                		q_destroy(q, NULL);
                		return NULL;
                	}
                	entry->data = rkzalloc(size, flags);
                	if (!entry->data) {
                		q_destroy(q, NULL);
                		return NULL;
                	}
                	INIT_LIST_HEAD(&entry->list);
                	list_add(&entry->list, &q->head);
                }
                q->length     = length;
                q->occupation = 0;
        	q->cur	      = list_first_entry(&q->head, struct ringq_entry, list);
        	if (!q->cur) {
        		LOG_ERR("Could not initialize RINGQ");
        		q_destroy(q, NULL);
        		return NULL;
        	}
        	q->comp = NULL;

                LOG_DBG("RINGQ %pK:%pK:%pK:%pK created successfully", q, &q->head, q->cur, &q->cur->list);

                return q;
        }
        q_destroy(q, NULL);

        return NULL;
}

struct ringq * ringq_create(unsigned int length)
{ return ringq_create_gfp(GFP_KERNEL, length, 0); }
EXPORT_SYMBOL(ringq_create);

struct ringq * ringq_create_ni(unsigned int length)
{ return ringq_create_gfp(GFP_ATOMIC, length, 0); }
EXPORT_SYMBOL(ringq_create_ni);

int ringq_destroy(struct ringq * q,
                  void        (* dtor)(void * e))
{
        if (!q) {
                LOG_ERR("Bogus input parameters, can't destroy NULL");
                return -1;
        }
        if (!dtor) {
                LOG_ERR("Bogus input parameters, no destructor provided");
                return -1;
        }

        q_destroy(q, dtor);

        LOG_DBG("RING Queue %pK destroyed successfully", q);

        return 0;
}
EXPORT_SYMBOL(ringq_destroy);

static struct ringq_entry * nxt_entry(struct ringq_entry * entry)
{
	struct ringq_entry * pos;

	pos = list_next_entry(entry, list);

	return pos;
}

static void entry_add(struct ringq * q, void * e)
{
	q->cur->data = e;
        if (q->occupation < (q->length - 1)) {
        	q->cur = nxt_entry(q->cur);
        }
	q->occupation++;

}

int ringq_push(struct ringq * q, void * e)
{

        if (!q) {
                LOG_ERR("Can't push into a NULL ringq ...");
                return -1;
        }

        spin_lock_bh(&q->lock);
        if (q->occupation >= q->length) {
        	spin_unlock_bh(&q->lock);
        	LOG_WARN("Attempted to push into a full ringq");
        	return -1;
        }
        q->cur->data = e;
        if (q->occupation < (q->length - 1)) {
                q->cur = nxt_entry(q->cur);
        }
        q->occupation++;
	spin_unlock_bh(&q->lock);

        return 0;
}
EXPORT_SYMBOL(ringq_push);

void * ringq_pop(struct ringq * q)
{
	struct ringq_entry * entry;
	void * 		     aux;

        if (!q) {
                LOG_ERR("Can't pop from a NULL ring queue ...");
                return NULL;
        }

        spin_lock_bh(&q->lock);
        if (q->occupation == 0) {
        	spin_unlock_bh(&q->lock);
        	return NULL;
        }
        entry = list_first_entry(&q->head, struct ringq_entry, list);
        if (!entry) {
        	spin_unlock_bh(&q->lock);
        	return NULL;
        }

        list_move_tail(&entry->list, &q->head);
        aux = entry->data;
	entry->data = NULL;
	if (q->occupation == q->length) {
		q->cur = nxt_entry(q->cur);
	}
	q->occupation--;
	spin_unlock_bh(&q->lock);

        return aux;
}
EXPORT_SYMBOL(ringq_pop);

bool ringq_is_empty(struct ringq * q)
{
	bool aux;

	spin_lock_bh(&q->lock);
	aux = q->occupation ? false : true;
	spin_unlock_bh(&q->lock);

        return aux;
}
EXPORT_SYMBOL(ringq_is_empty);

ssize_t ringq_length(struct ringq * q)
{
        if (!q) {
                LOG_ERR("Can't get size of a NULL ring queue");
                return -1;
        }

        return q->length;
}
EXPORT_SYMBOL(ringq_length);

ssize_t ringq_occupation(struct ringq * q)
{
	ssize_t aux;

        if (!q) {
                LOG_ERR("Can't get size of a NULL ring queue");
                return -1;
        }

	spin_lock_bh(&q->lock);
	aux = q->occupation;
	spin_unlock_bh(&q->lock);

        return aux;
}
EXPORT_SYMBOL(ringq_occupation);

struct ringq * ringq_order_create(unsigned int length, int (* comp)(void*, void*))
{
	struct ringq * tmp;

	tmp = ringq_create(length);
	if (!tmp)
		return NULL;

	tmp->comp = comp;

	return tmp;
}
EXPORT_SYMBOL(ringq_order_create);

int ringq_order_push(struct ringq * q, void * e)
{
	struct ringq_entry * last;
	struct ringq_entry * pos;

	spin_lock_bh(&q->lock);
	if (!q->occupation) {
		entry_add(q, e);
		spin_unlock_bh(&q->lock);

		return 0;
	}

        if (q->occupation >= q->length) {
        	spin_unlock_bh(&q->lock);
        	LOG_WARN("Attempted push into a full RINGQ");

        	return -1;
        }
        last = list_prev_entry(q->cur, list);
        if (q->comp(e, last->data)) {
        	entry_add(q, e);
		spin_unlock_bh(&q->lock);

		return 0;
        }

        list_for_each_entry(pos, &q->head, list) {
	        if (q->comp(pos->data, e)) {
	        	struct ringq_entry * aux = q->cur;

	        	entry_add(q, e);
	        	list_move_tail(&aux->list, &pos->list);
			spin_unlock_bh(&q->lock);

			return 0;
	        }
        }
	spin_unlock_bh(&q->lock);

	return -1;
}
EXPORT_SYMBOL(ringq_order_push);

struct ringq * ringq_order_entry_create(unsigned int length,
					unsigned int size,
					int (* comp)(void*, void*),
					void (* add)(void*, void*))
{
	struct ringq * tmp;

	tmp = ringq_create_gfp(GFP_ATOMIC, length, size);
	if (!tmp)
		return NULL;

	tmp->comp = comp;
	tmp->add  = add;

	return tmp;
}
EXPORT_SYMBOL(ringq_order_entry_create);

static void entry_cpy(struct ringq * q, void * entry)
{
	q->add(q->cur->data, entry);
        if (q->occupation < (q->length - 1)) {
        	q->cur = nxt_entry(q->cur);
        }
	q->occupation++;
}

int ringq_push_entry(struct ringq * q, void * entry)
{
	struct ringq_entry * last;
	struct ringq_entry * pos;

	spin_lock_bh(&q->lock);
	if (!q->occupation) {
		entry_cpy(q, entry);
		spin_unlock_bh(&q->lock);

		return 0;
	}

        if (q->occupation >= q->length) {
        	spin_unlock_bh(&q->lock);
        	LOG_WARN("Attempted push into a full RINGQ");

        	return -1;
        }
        last = list_prev_entry(q->cur, list);
        if (q->comp(entry, last->data)) {
        	entry_cpy(q, entry);
		spin_unlock_bh(&q->lock);

		return 0;
        }

        list_for_each_entry(pos, &q->head, list) {
	        if (q->comp(pos->data, entry)) {
	        	struct ringq_entry * aux = q->cur;

	        	entry_cpy(q, entry);
	        	list_move_tail(&aux->list, &pos->list);
			spin_unlock_bh(&q->lock);

			return 0;
	        }
        }
	spin_unlock_bh(&q->lock);

	return -1;
}
EXPORT_SYMBOL(ringq_push_entry);

void * ringq_pop_entry(struct ringq * q)
{
	struct ringq_entry * entry;
	void * 		     aux;

        if (!q) {
                LOG_ERR("Can't pop from a NULL ring queue ...");
                return NULL;
        }

        spin_lock_bh(&q->lock);
        if (q->occupation == 0) {
        	spin_unlock_bh(&q->lock);
        	return NULL;
        }
        entry = list_first_entry(&q->head, struct ringq_entry, list);
        if (!entry) {
        	spin_unlock_bh(&q->lock);
        	return NULL;
        }

        list_move_tail(&entry->list, &q->head);
        aux = entry->data;
	if (q->occupation == q->length) {
		q->cur = nxt_entry(q->cur);
	}
	q->occupation--;
	spin_unlock_bh(&q->lock);

        return aux;
}
EXPORT_SYMBOL(ringq_pop_entry);

#ifdef CONFIG_RINA_RINGQ_REGRESSION_TESTS
static void buffer_dealloc(void * buffer)
{
	if (!buffer)
		return;

	rkfree(buffer);
}

static int comp(void * x, void * y)
{
	return (strlen(x) > strlen(y)) ? 1 : 0;
}

struct entry_test {
	char * data;
	int size;
	unsigned long tnow;
};

static void data_entry_cpy(void * x, void * y)
{
	struct entry_test * xentry, * yentry;

	xentry = (struct entry_test *) x;
	yentry = (struct entry_test *) y;
	*xentry = *yentry;
	return;
}

static int comp_2(void * x, void * y)
{
	struct entry_test * xentry, * yentry;

	yentry = (struct entry_test *) y;
	xentry = (struct entry_test *) x;

	return ((xentry->size) > (yentry->size)) ? 1 : 0;
}

static bool regression_test_ringq_order_with_entry(void)
{
	struct ringq * q;
	char * buffer_1 = "Test 1\0";
	char * buffer_2 = "Test 2+\0";
	char * buffer_3 = "Test 3++\0";
	char * buffer_4 = "Test 4+++\0";
	struct entry_test entry;
	struct entry_test * buff;
	unsigned int length = 4;
	int i;

	q = ringq_order_entry_create(length, sizeof(entry), comp_2, data_entry_cpy);
	if (!q) {
		LOG_ERR("Failed creation of RINGQ order with SIZE");
		return false;
	}
	LOG_INFO("Successfully created RINGQ order with size");

	entry.data = buffer_3;
	entry.size = 8;
	entry.tnow = jiffies;
	i = ringq_push_entry(q, &entry);
	LOG_INFO("Buffer 3 tnow: %llu, result: %d", entry.tnow, i);

	entry.data = buffer_1;
	entry.size = 6;
	entry.tnow = jiffies + 1000;
	i = ringq_push_entry(q, &entry);
	LOG_INFO("Buffer 1 tnow: %llu, result: %d", entry.tnow, i);

	entry.data = buffer_4;
	entry.size = 9;
	entry.tnow = jiffies;
	i = ringq_push_entry(q, &entry);
	LOG_INFO("Buffer 4 tnow: %llu, result: %d", entry.tnow, i);

	entry.data = buffer_2;
	entry.size = 7;
	entry.tnow = jiffies + 1000;
	i = ringq_push_entry(q, &entry);
	LOG_INFO("Buffer 2 tnow: %llu, result: %d", entry.tnow, i);

	LOG_INFO("Successfully pushed entries into RINGQ order size");

	for (i = 0; i < length; i++) {
		buff = ringq_pop_entry(q);
		if (!buff) {
			LOG_INFO("Failed pop from RINGQ order size");
			ringq_destroy(q, buffer_dealloc);
			return false;
		}
		entry = *buff;
		LOG_INFO("%s", entry.data);
		LOG_INFO("%lu", entry.tnow);
	}
	LOG_INFO("Success RINGQ order size");
	ringq_destroy(q, buffer_dealloc);

	return true;
}

bool regression_tests_ringq(void)
{
	struct ringq * q;
	char * buffer;
	char * rbuff = NULL;
	char * rbuff_2 = NULL;
	int ret;
	unsigned int length = 5;
	int i;
	ssize_t s;
	ssize_t r;
	char * buffer_1 = "Test 1\0";
	char * buffer_2 = "Test 2+\0";
	char * buffer_3 = "Test 3++\0";
	char * buffer_4 = "Test 4+++\0";

	q = ringq_create_ni(length);
	if (!q) {
		LOG_ERR("Failed creation of RINGQ");
		return false;
	}

	ret = ringq_destroy(q, buffer_dealloc);
	if (ret) {
		LOG_ERR("Failed destruction of RINGQ");
		return false;
	}
	q = NULL;
	q = ringq_create_ni(length);
	if (!q)
		return false;

	buffer = rkmalloc(10, GFP_KERNEL);
	if (!buffer) {
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ creation and destruction");

	r = ringq_length(q);
	if (r != length) {
		LOG_ERR("Failed length of RINGQ: %d", r);
		rkfree(buffer);
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ length");

	s = ringq_occupation(q);
	if (s != 0) {
		LOG_ERR("Failed length of RINGQ: %d", s);
		rkfree(buffer);
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ occupation 0");

	ret = ringq_push(q, buffer);
	if (ret) {
		LOG_ERR("Failed push into RINGQ");
		rkfree(buffer);
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ single push");

	s = ringq_occupation(q);
	if (s != 1) {
		LOG_ERR("Failed length of RINGQ: %d", s);
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ occupation 1");

	rbuff = ringq_pop(q);
	if (!rbuff) {
		LOG_ERR("Failed pop from RINGQ");
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ single pop");

	s = ringq_occupation(q);
	if (s != 0) {
		LOG_ERR("Failed length of RINGQ: %d", s);
		rkfree(rbuff);
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ occupation 1->0");

	for (i = 0; i<length; i++) {
		char * buff = rkmalloc(10, GFP_KERNEL);
		ret = ringq_push(q, buff);
		if (ret) {
			LOG_ERR("Failed to push sequentially into RINGQ");
			rkfree(buff);
			ringq_destroy(q, buffer_dealloc);
			return false;
		}
		s = ringq_occupation(q);
		if (s != (i+1)) {
			LOG_ERR("Failed length of RINGQ: %d", s);
			rkfree(buffer);
			ringq_destroy(q, buffer_dealloc);
			return false;
		}
		LOG_INFO("SUCCESS RINGQ occupation %d", i+1);
	}
	LOG_INFO("SUCCESS RINGQ sequential push");

	ret = ringq_push(q, rbuff);
	if (ret != -1) {
		LOG_ERR("Failed by pushing sequentially into RINGQ: %d", ret);
		rkfree(rbuff);
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ forbidden push");

	s = ringq_occupation(q);
	if (s != length) {
		LOG_ERR("Failed length of RINGQ: %d", s);
		rkfree(rbuff);
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ occupation MAX");

	rbuff_2 = ringq_pop(q);
	if (!rbuff_2) {
		LOG_ERR("Failed to pop data from full queue");
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ pop from a full queue");

	s = ringq_occupation(q);
	if (s != (length-1)) {
		LOG_ERR("Failed length of RINGQ: %d", s);
		rkfree(rbuff_2);
		rkfree(rbuff);
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ occupation MAX-1");

	ret = ringq_push(q, rbuff_2);
	if (ret) {
		LOG_ERR("Failed pushing into an almost full RINGQ");
		rkfree(rbuff_2);
		rkfree(rbuff);
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ push into an almost full queue");

	s = ringq_occupation(q);
	if (s != length) {
		LOG_ERR("Failed length of RINGQ: %d", s);
		rkfree(rbuff);
		ringq_destroy(q, buffer_dealloc);
		return false;
	}
	LOG_INFO("SUCCESS RINGQ occupation %d", s);

	rkfree(rbuff);
	ringq_destroy(q, buffer_dealloc);

	length = 4;
	q = ringq_order_create(length, comp);
	if (!q) {
		LOG_ERR("Failed creation of RINGQ");
		return false;
	}
	ringq_order_push(q, buffer_3);
	ringq_order_push(q, buffer_1);
	ringq_order_push(q, buffer_2);
	ringq_order_push(q, buffer_4);
	for (i = 0; i < length; i++) {
		char * aux = ringq_pop(q);
		LOG_INFO("%s", aux);
	}
	ringq_destroy(q, buffer_dealloc);

	if (!regression_test_ringq_order_with_entry()) {
		LOG_INFO("Failed RINGQ order size");
		return false;
	}

	LOG_INFO("RINGQ regression tests passed");

	return true;
}
#endif
