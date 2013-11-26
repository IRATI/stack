/*
 * K-IPCM (Kernel-IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#define RINA_PREFIX "du"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"

static struct pdu * pdu_create_gfp(gfp_t flags)
{
        struct pdu * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->pci    = NULL;
        tmp->buffer = NULL;

        return tmp;
}

struct pdu * pdu_create(void)
{ return pdu_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(pdu_create);

struct pdu * pdu_create_ni(void)
{ return pdu_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(pdu_create_ni);

bool pdu_is_ok(const struct pdu * p)
{ return (p && p->pci && p->buffer) ? true : false; }
EXPORT_SYMBOL(pdu_is_ok);

const struct buffer * pdu_buffer(const struct pdu * pdu)
{
        if (!pdu_is_ok(pdu))
                return NULL;

        return pdu->buffer;
}
EXPORT_SYMBOL(pdu_buffer);

const struct pci * pdu_pci(const struct pdu * pdu)
{
        if (!pdu_is_ok(pdu))
                return NULL;

        return pdu->pci;
}
EXPORT_SYMBOL(pdu_pci);

int pdu_destroy(struct pdu * p)
{
        if (p)
                return -1;

        if (p->pci)    rkfree(p->pci);
        if (p->buffer) buffer_destroy(p->buffer);

        rkfree(p);

        return 0;
}
EXPORT_SYMBOL(pdu_destroy);

bool buffer_is_ok(const struct buffer * b)
{ return (b && b->data && b->size) ? true : false; }
EXPORT_SYMBOL(buffer_is_ok);

static struct buffer * buffer_create_from_gfp(gfp_t  flags,
                                              void * data,
                                              size_t size)
{
        struct buffer * tmp;

        if (!data) {
                LOG_ERR("Cannot create buffer, data is NULL");
                return NULL;
        }
        if (!size) {
                LOG_ERR("Cannot create buffer, data is 0 size");
                return NULL;
        }

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->data = data;
        tmp->size = size;

        return tmp;
}

struct buffer * buffer_create_from(void * data,
                                   size_t size)
{ return buffer_create_from_gfp(GFP_KERNEL, data, size); }
EXPORT_SYMBOL(buffer_create_from);

struct buffer * buffer_create_from_ni(void * data,
                                      size_t size)
{ return buffer_create_from_gfp(GFP_ATOMIC, data, size); }
EXPORT_SYMBOL(buffer_create_from_ni);

static struct buffer * buffer_create_gfp(gfp_t  flags,
                                         size_t size)
{
        struct buffer * tmp;

        if (!size) {
                LOG_ERR("Cannot create buffer, data is 0 size");
                return NULL;
        }

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->data = rkmalloc(size, flags);
        if (!tmp->data) {
                rkfree(tmp);
                return NULL;
        }

        tmp->size = size;

        return tmp;
}

struct buffer * buffer_create(size_t size)
{ return buffer_create_gfp(GFP_KERNEL, size); }
EXPORT_SYMBOL(buffer_create);

struct buffer * buffer_create_ni(size_t size)
{ return buffer_create_gfp(GFP_ATOMIC, size); }
EXPORT_SYMBOL(buffer_create_ni);

static struct buffer * buffer_dup_gfp(gfp_t           flags,
                                      struct buffer * b)
{
        struct buffer * tmp;
        void *          m;

        if (!buffer_is_ok(b)) {
                LOG_ERR("Cannot duplicate buffer, bad input parameter");
                return NULL;
        }

        m = rkmalloc(b->size, flags);
        if (!m)
                return NULL;

        if (!memcpy(m, b->data, b->size)) {
                LOG_ERR("Cannot copy memory block, cannot duplicate buffer");
                rkfree(m);
                return NULL;
        }

        tmp = buffer_create_from_gfp(flags, m, b->size);
        if (!tmp) {
                rkfree(m);
                return NULL;
        }

        return tmp;
}

struct buffer * buffer_dup(struct buffer * b)
{ return buffer_dup_gfp(GFP_KERNEL, b); }
EXPORT_SYMBOL(buffer_dup);

struct buffer * buffer_dup_ni(struct buffer * b)
{ return buffer_dup_gfp(GFP_ATOMIC, b); }
EXPORT_SYMBOL(buffer_dup_ni);

int buffer_destroy(struct buffer * b)
{
        if (!b)
                return -1;

        /* NOTE: Be merciful and destroy even a non-ok buffer ... */
        if (b->data) rkfree(b->data);

        rkfree(b);

        return 0;
}
EXPORT_SYMBOL(buffer_destroy);

ssize_t buffer_length(const struct buffer * b)
{
        if (!buffer_is_ok(b))
                return -1;

        return b->size;
}
EXPORT_SYMBOL(buffer_length);

void * buffer_data(struct buffer * b)
{
        if (!buffer_is_ok(b))
                return NULL;

        return b->data;
}
EXPORT_SYMBOL(buffer_data);

static struct sdu * sdu_create_from_buffer_gfp(gfp_t           flags,
                                               struct buffer * buffer)
{
        struct sdu * tmp;

        if (!buffer_is_ok(buffer))
                return NULL;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->buffer = buffer;

        return tmp;
}

struct sdu * sdu_create_from_buffer(struct buffer * buffer)
{ return sdu_create_from_buffer_gfp(GFP_KERNEL, buffer); }
EXPORT_SYMBOL(sdu_create_from_buffer);

struct sdu * sdu_create_from_buffer_ni(struct buffer * buffer)
{ return sdu_create_from_buffer_gfp(GFP_ATOMIC, buffer); }
EXPORT_SYMBOL(sdu_create_from_buffer_ni);

int sdu_destroy(struct sdu * s)
{
        /* FIXME: Should we assert here ? */
        if (!s)
                return -1;

        if (s->buffer) {
                if (s->buffer->data) rkfree(s->buffer->data);
                rkfree(s->buffer);
        }

        rkfree(s);
        return 0;
}
EXPORT_SYMBOL(sdu_destroy);

const struct buffer * sdu_buffer(const struct sdu * s)
{
        if (!sdu_is_ok(s))
                return NULL;

        return s->buffer;
}
EXPORT_SYMBOL(sdu_buffer);

static struct sdu * sdu_dup_gfp(gfp_t              flags,
                                const struct sdu * sdu)
{
        struct sdu * tmp;

        if (!sdu_is_ok(sdu))
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->buffer = rkzalloc(sizeof(struct buffer), flags);
        if (!tmp->buffer) {
                rkfree(tmp);
                return NULL;
        }

        tmp->buffer->data = (char *) rkzalloc(sdu->buffer->size, flags);
        if (!tmp->buffer->data) {
                rkfree(tmp->buffer);
                rkfree(tmp);
                return NULL;
        }

        memcpy(tmp->buffer->data, sdu->buffer->data, sdu->buffer->size);
        tmp->buffer->size = sdu->buffer->size;

        return tmp;
}

struct sdu * sdu_dup(const struct sdu * sdu)
{ return sdu_dup_gfp(GFP_KERNEL, sdu); }
EXPORT_SYMBOL(sdu_dup);

struct sdu * sdu_dup_ni(const struct sdu * sdu)
{ return sdu_dup_gfp(GFP_ATOMIC, sdu); }
EXPORT_SYMBOL(sdu_dup_ni);

bool sdu_is_ok(const struct sdu * s)
{
        if (!s)
                return false;

        if (!s->buffer)
                return false;

        /* Should we accept an empty sdu ? */
        if (!s->buffer->data)
                return false;
        if (!s->buffer->size)
                return false;

        /* FIXME: More checks expected here ... */

        return true;
}
EXPORT_SYMBOL(sdu_is_ok);

struct sdu * sdu_protect(struct sdu * s)
{
        LOG_MISSING;

        return NULL;
}
EXPORT_SYMBOL(sdu_protect);

struct sdu * sdu_unprotect(struct sdu * s)
{
        LOG_MISSING;

        return NULL;
}
EXPORT_SYMBOL(sdu_unprotect);
