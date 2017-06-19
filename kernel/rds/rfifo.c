/*
 * RINA FIFOs
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

#include <linux/export.h>
#include <linux/list.h>
#include <linux/types.h>

#define RINA_PREFIX "rfifo"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "rqueue.h"
#include "rfifo.h"

/* The crappiest implementation we could have at the moment ... */
struct rfifo {
        struct rqueue * q;
};

/* FIXME: This extern has to disappear from here */
struct rqueue * rqueue_create_gfp(gfp_t flags);

static struct rfifo * rfifo_create_gfp(gfp_t flags)
{
        struct rfifo * f;

        f = rkzalloc(sizeof(*f), flags);
        if (!f)
                return NULL;

        f->q = rqueue_create_gfp(flags);
        if (!f->q) {
                rkfree(f);
                return NULL;
        }

        LOG_DBG("FIFO %pK created successfully", f);

        return f;
}

struct rfifo * rfifo_create(void)
{ return rfifo_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(rfifo_create);

struct rfifo * rfifo_create_ni(void)
{ return rfifo_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(rfifo_create_ni);

int rfifo_destroy(struct rfifo * f,
                  void        (* dtor)(void * e))
{
        if (!f) {
                LOG_ERR("Bogus input parameters, can't destroy NULL");
                return -1;
        }
        if (!dtor) {
                LOG_ERR("Bogus input parameters, no destructor provided");
                return -1;
        }

        if (rqueue_destroy(f->q, dtor))
                return -1;
        rkfree(f);

        LOG_DBG("FIFO %pK destroyed successfully", f);

        return 0;
}
EXPORT_SYMBOL(rfifo_destroy);

int rfifo_push(struct rfifo * f, void * e)
{
        if (!f) {
                LOG_ERR("Can't push into a NULL fifo ...");
                return -1;
        }

        return rqueue_tail_push(f->q, e);
}
EXPORT_SYMBOL(rfifo_push);

int rfifo_push_ni(struct rfifo * f, void * e)
{
        if (!f) {
                LOG_ERR("Can't push into a NULL fifo ...");
                return -1;
        }

        return rqueue_tail_push_ni(f->q, e);
}
EXPORT_SYMBOL(rfifo_push_ni);

int rfifo_head_push_ni(struct rfifo * f, void * e)
{
        if (!f) {
                LOG_ERR("Can't push into a NULL fifo ...");
                return -1;
        }

        return rqueue_head_push_ni(f->q, e);
}
EXPORT_SYMBOL(rfifo_head_push_ni);

void * rfifo_pop(struct rfifo * f)
{
        if (!f) {
                LOG_ERR("Can't pop from a NULL fifo ...");
                return NULL;
        }

        return rqueue_head_pop(f->q);
}
EXPORT_SYMBOL(rfifo_pop);

void * rfifo_peek(struct rfifo * f)
{
        if (!f) {
                LOG_ERR("Can't peek from a NULL fifo ...");
                return NULL;
        }

        return rqueue_head_peek(f->q);
}
EXPORT_SYMBOL(rfifo_peek);

bool rfifo_is_empty(struct rfifo * f)
{
        if (!f) {
                LOG_ERR("Can't check the emptiness of a NULL fifo");
                return false;
        }

        return rqueue_is_empty(f->q);
}
EXPORT_SYMBOL(rfifo_is_empty);

ssize_t rfifo_length(struct rfifo * f)
{
        if (!f) {
                LOG_ERR("Can't get size of a NULL fifo");
                return -1;
        }

        return rqueue_length(f->q);
}
EXPORT_SYMBOL(rfifo_length);

#ifdef CONFIG_RINA_RFIFO_REGRESSION_TESTS
bool regression_tests_rfifo(void)
{ return true; }
#endif
