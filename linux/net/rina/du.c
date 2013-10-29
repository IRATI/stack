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

#define RINA_PREFIX "du"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"

struct pdu * pdu_create(void)
{
        struct pdu * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->pci    = NULL;
        tmp->buffer = NULL;

        return tmp;
}

int pdu_destroy(struct pdu * p)
{
        ASSERT(p);

        if (p->pci)
                rkfree(p->pci);
        if (p->buffer)
                rkfree(p->buffer);
        rkfree(p);

        return 0;
}

struct sdu * sdu_create_from_gfp(gfp_t  flags,
                                 void * data,
                                 size_t size)
{
        struct sdu * tmp;

        LOG_DBG("Trying to create an SDU of size %zd from data in the buffer",
                size);

        if (!data)
                return NULL;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->buffer = rkmalloc(sizeof(struct buffer), flags);
        if (!tmp->buffer) {
                rkfree(tmp);
                return NULL;
        }

        tmp->buffer->data = data;
        tmp->buffer->size = size;

        return tmp;
}
EXPORT_SYMBOL(sdu_create_from_gfp);

struct sdu * sdu_create_from(void * data, size_t size)
{ return sdu_create_from_gfp(GFP_KERNEL, data, size); }
EXPORT_SYMBOL(sdu_create_from);

int sdu_destroy(struct sdu * s)
{
        /* FIXME: Should we assert here ? */
        if (!s)
                return -1;

        if (s->buffer) {
                if (s->buffer->data)
                        rkfree(s->buffer->data);
                rkfree(s->buffer);
        }

        rkfree(s);
        return 0;
}
EXPORT_SYMBOL(sdu_destroy);

struct sdu * sdu_dup_gfp(gfp_t        flags,
                         struct sdu * sdu)
{
        struct sdu * tmp;

        if (!is_sdu_ok(sdu))
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
EXPORT_SYMBOL(sdu_dup_gfp);

struct sdu * sdu_dup(struct sdu * sdu)
{ return sdu_dup_gfp(GFP_KERNEL, sdu); }
EXPORT_SYMBOL(sdu_dup);

bool is_sdu_ok(const struct sdu * s)
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
EXPORT_SYMBOL(is_sdu_ok);

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
