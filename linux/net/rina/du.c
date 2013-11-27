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
#include <linux/types.h>

#define RINA_PREFIX "du"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"

struct buffer {
        char * data;
        size_t size;
};

bool buffer_is_ok(const struct buffer * b)
{ return (b && b->data && b->size) ? true : false; }
EXPORT_SYMBOL(buffer_is_ok);

static int __buffer_destroy(struct buffer * b)
{
        ASSERT(b);

        if (b->data) rkfree(b->data);
        rkfree(b);

        return 0;
}

int buffer_destroy(struct buffer * b)
{
        if (!b)
                return -1;
        return __buffer_destroy(b);
}
EXPORT_SYMBOL(buffer_destroy);

static struct buffer * buffer_create_with_gfp(gfp_t  flags,
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

struct buffer * buffer_create_with(void * data,
                                   size_t size)
{ return buffer_create_with_gfp(GFP_KERNEL, data, size); }
EXPORT_SYMBOL(buffer_create_with);

struct buffer * buffer_create_with_ni(void * data,
                                      size_t size)
{ return buffer_create_with_gfp(GFP_ATOMIC, data, size); }
EXPORT_SYMBOL(buffer_create_with_ni);

static struct buffer * buffer_create_from_gfp(gfp_t        flags,
                                              const void * data,
                                              size_t       size)
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

        tmp->data = rkmalloc(size, flags);
        if (!tmp->data) {
                __buffer_destroy(tmp);
                return NULL;
        }
        if (!memcpy(tmp->data, data, size)) {
                __buffer_destroy(tmp);
                return NULL;
        }

        tmp->size = size;

        return tmp;
}

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

static struct buffer * buffer_dup_gfp(gfp_t                 flags,
                                      const struct buffer * b)
{
        struct buffer * tmp;
        void *          m;

        if (!buffer_is_ok(b))
                return NULL;

        m = rkmalloc(b->size, flags);
        if (!m)
                return NULL;

        if (!memcpy(m, b->data, b->size)) {
                rkfree(m);
                return NULL;
        }

        tmp = buffer_create_with_gfp(flags, m, b->size);
        if (!tmp) {
                rkfree(m);
                return NULL;
        }

        return tmp;
}

struct buffer * buffer_dup(const struct buffer * b)
{ return buffer_dup_gfp(GFP_KERNEL, b); }
EXPORT_SYMBOL(buffer_dup);

struct buffer * buffer_dup_ni(const struct buffer * b)
{ return buffer_dup_gfp(GFP_ATOMIC, b); }
EXPORT_SYMBOL(buffer_dup_ni);

ssize_t buffer_length(const struct buffer * b)
{
        if (!buffer_is_ok(b))
                return -1;

        return b->size;
}
EXPORT_SYMBOL(buffer_length);

const void * buffer_data_ro(const struct buffer * b)
{
        if (!buffer_is_ok(b))
                return NULL;

        return b->data;
}
EXPORT_SYMBOL(buffer_data_ro);

void * buffer_data_rw(struct buffer * b)
{
        if (!buffer_is_ok(b))
                return NULL;

        return b->data;
}
EXPORT_SYMBOL(buffer_data_rw);

struct pci {
        address_t  source;
        address_t  destination;

        pdu_type_t type;

        struct {
                cep_id_t source;
                cep_id_t destination;
        } ceps;

        qos_id_t   qos_id;
        seq_num_t  sequence_number;
};

static bool pci_is_ok(const struct pci * pci)
{ return pci && pdu_type_is_ok(pci->type) ? true : false; }

static struct pci * pci_create_from_gfp(gfp_t        flags,
                                        const void * data)
{
        struct pci * tmp;

        if (!data)
                return NULL;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        if (!memcpy(tmp, data, sizeof(*tmp))) {
                rkfree(tmp);
                return NULL;
        }

        ASSERT(pci_is_ok(tmp));

        return tmp;
}

struct pci * pci_create_from(const void * data)
{ return pci_create_from_gfp(GFP_KERNEL, data); }
EXPORT_SYMBOL(pci_create_from);

struct pci * pci_create_from_ni(const void * data)
{ return pci_create_from_gfp(GFP_ATOMIC, data); }
EXPORT_SYMBOL(pci_create_from_ni);

int pci_destroy(struct pci * pci)
{
        if (!pci)
                return -1;

        rkfree(pci);
        return 0;
}
EXPORT_SYMBOL(pci_destroy);

static struct pci * pci_dup_gfp(gfp_t              flags,
                                const struct pci * pci)
{
        struct pci * tmp;

        if (pci_is_ok(pci))
                return NULL;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        if (!memcpy(tmp, pci, sizeof(*tmp))) {
                rkfree(tmp);
                return NULL;
        }

        ASSERT(pci_is_ok(tmp));

        return tmp;
}

struct pci * pci_dup(const struct pci * pci)
{ return pci_dup_gfp(GFP_KERNEL, pci); }
EXPORT_SYMBOL(pci_dup);

struct pci * pci_dup_ni(const struct pci * pci)
{ return pci_dup_gfp(GFP_ATOMIC, pci); }
EXPORT_SYMBOL(pci_dup_ni);

pdu_type_t pci_type(const struct pci * pci)
{
        ASSERT(pci); /* FIXME: Should not be an ASSERT ... */

        return pci->type;
}
EXPORT_SYMBOL(pci_type);

ssize_t pci_length(const struct pci * pci)
{
        if (!pci_is_ok(pci))
                return -1;

        return sizeof(*pci);
}
EXPORT_SYMBOL(pci_length);

address_t pci_source(const struct pci * pci)
{
        ASSERT(pci); /* FIXME: Should not be an ASSERT ... */

        return pci->source;
}
EXPORT_SYMBOL(pci_source);

address_t pci_destination(const struct pci * pci)
{
        ASSERT(pci); /* FIXME: Should not be an ASSERT ... */

        return pci->destination;
}
EXPORT_SYMBOL(pci_destination);

cep_id_t pci_cep_destination(const struct pci * pci)
{
        if (!pci)
                return cep_id_bad();

        return pci->ceps.destination;
}
EXPORT_SYMBOL(pci_cep_destination);

cep_id_t pci_cep_source(const struct pci * pci)
{
        if (!pci)
                return cep_id_bad();

        return pci->ceps.source;
}
EXPORT_SYMBOL(pci_cep_source);

struct pdu {
        struct pci *    pci;
        struct buffer * buffer;
};

bool pdu_is_ok(const struct pdu * p)
{ return (p && p->pci && p->buffer) ? true : false; }
EXPORT_SYMBOL(pdu_is_ok);

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

static struct pdu * pdu_create_with_gfp(gfp_t        flags,
                                        struct sdu * sdu)
{
        struct pdu *          tmp_pdu;
        const struct buffer * tmp_buff;
        const uint8_t *       ptr;

        if (!sdu_is_ok(sdu))
                return NULL;

        tmp_buff = sdu_buffer(sdu);
        ASSERT(tmp_buff);

        if (buffer_length(tmp_buff) < sizeof(struct pci))
                return NULL;
        
        tmp_pdu = pdu_create_gfp(flags);
        if (!tmp_pdu)
                return NULL;

        ptr = (const uint8_t *) buffer_data_ro(tmp_buff);
        ASSERT(!ptr);

        tmp_pdu->pci    =
                pci_create_from_gfp(flags, ptr);
        tmp_pdu->buffer =
                buffer_create_from_gfp(flags,
                                       ptr + sizeof(struct pci),
                                       (buffer_length(sdu->buffer) -
                                        sizeof(struct pci)));

        ASSERT(pdu_is_ok(tmp_pdu));

        return tmp_pdu;
}

struct pdu * pdu_create_with(struct sdu * sdu)
{ return pdu_create_with_gfp(GFP_KERNEL, sdu); }
EXPORT_SYMBOL(pdu_create_with);

struct pdu * pdu_create_with_ni(struct sdu * sdu)
{ return pdu_create_with_gfp(GFP_ATOMIC, sdu); }
EXPORT_SYMBOL(pdu_create_with_ni);

struct buffer * pdu_buffer(struct pdu * pdu)
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

pdu_type_t pdu_type(const struct pdu * pdu)
{
        ASSERT(pdu_is_ok(pdu));

        return pci_type(pdu->pci);
}
EXPORT_SYMBOL(pdu_type);

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

static struct sdu * sdu_create_with_gfp(gfp_t           flags,
                                        struct buffer * buffer)
{
        struct sdu * tmp;

        if (!buffer_is_ok(buffer))
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->buffer = buffer;

        return tmp;
}

struct sdu * sdu_create_with(struct buffer * buffer)
{ return sdu_create_with_gfp(GFP_KERNEL, buffer); }
EXPORT_SYMBOL(sdu_create_with);

struct sdu * sdu_create_with_ni(struct buffer * buffer)
{ return sdu_create_with_gfp(GFP_ATOMIC, buffer); }
EXPORT_SYMBOL(sdu_create_with_ni);

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

        if (!memcpy(tmp->buffer->data, sdu->buffer->data, sdu->buffer->size)) {
                sdu_destroy(tmp);
                return NULL;
        }
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
