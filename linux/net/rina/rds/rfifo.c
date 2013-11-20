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

#define RINA_PREFIX "rfifo"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "rqueue.h"

/* The crappiest implementation we could have at the moment ... */
struct rfifo {
        struct rqueue * q;
};

struct rfifo * rfifo_create(void)
{
        struct rfifo * f;

        f = rkzalloc(sizeof(*f), GFP_KERNEL);
        if (!f)
                return NULL;

        f->q = rqueue_create();
        if (!f->q) {
                rkfree(f);
                return NULL;
        }

        LOG_DBG("Fifo %pK created successfully", f);

        return f;
}
EXPORT_SYMBOL(rfifo_create);

int rfifo_destroy(struct rfifo * f,
                  void         (* dtor)(void * e))
{
        if (!f || !dtor) {
                LOG_ERR("Bogus input parameters, can't destroy fifo %pK", f);
                return -1;
        }

        if (rqueue_destroy(f->q, dtor))
                return -1;
        rkfree(f);

        LOG_DBG("Fifo %pK destroyed successfully", f);

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

void * rfifo_pop(struct rfifo * f)
{
        if (!f) {
                LOG_ERR("Can't pop from a NULL fifo ...");
                return NULL;
        }

        return rqueue_head_pop(f->q);
}
EXPORT_SYMBOL(rfifo_pop);

bool rfifo_is_empty(struct rfifo * f)
{
        if (!f) {
                LOG_ERR("Can't chek the emptiness of a NULL fifo");
                return false;
        }

        return rqueue_is_empty(f->q);
}
EXPORT_SYMBOL(rfifo_is_empty);
