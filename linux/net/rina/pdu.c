/*
 * Protocol Data Unit
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

#define RINA_PREFIX "pdu"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "pdu.h"
#include "pci.h"

/* FIXME: These externs have to disappear from here */
struct buffer * buffer_create_with_gfp(gfp_t  flags,
                                       void * data,
                                       size_t size);
struct buffer * buffer_create_from_gfp(gfp_t        flags,
                                       const void * data,
                                       size_t       size);
struct buffer * buffer_dup_gfp(gfp_t                 flags,
                               const struct buffer * b);

struct pci *    pci_dup_gfp(gfp_t              flags,
                            const struct pci * pci);

struct pci *    pci_create_from_gfp(gfp_t        flags,
                                    const void * data);

struct pdu {
        struct pci *    pci;
        struct buffer * buffer;
};

bool pdu_is_ok(const struct pdu * p)
{ return (p && pci_is_ok(p->pci) && buffer_is_ok(p->buffer)) ? true : false; }
EXPORT_SYMBOL(pdu_is_ok);

struct pdu * pdu_create_gfp(gfp_t flags)
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

static struct pdu * pdu_create_from_gfp(gfp_t              flags,
                                        const struct sdu * sdu)
{
        struct pdu *          tmp_pdu;
        const struct buffer * tmp_buff;
        const uint8_t *       ptr;
        size_t                pci_size;

        /* FIXME: This implementation is pure crap, please fix it soon */

        if (!sdu_is_ok(sdu))
                return NULL;

        tmp_buff = sdu_buffer_ro(sdu);
        ASSERT(tmp_buff);

        if (buffer_length(tmp_buff) < pci_length_min())
                return NULL;

        /* FIXME: We should compute the real PCI length */
        pci_size = pci_length_min();

        tmp_pdu = pdu_create_gfp(flags);
        if (!tmp_pdu)
                return NULL;

        ptr = (const uint8_t *) buffer_data_ro(tmp_buff);
        ASSERT(ptr);

        tmp_pdu->pci    = pci_create_from_gfp(flags, ptr);
        tmp_pdu->buffer = buffer_create_from_gfp(flags,
                                                 ptr + pci_size,
                                                 (buffer_length(sdu->buffer) -
                                                  pci_size));

        ASSERT(pdu_is_ok(tmp_pdu));

        return tmp_pdu;
}

struct pdu * pdu_create_from(const struct sdu * sdu)
{ return pdu_create_from_gfp(GFP_KERNEL, sdu); }
EXPORT_SYMBOL(pdu_create_from);

struct pdu * pdu_create_from_ni(const struct sdu * sdu)
{ return pdu_create_from_gfp(GFP_ATOMIC, sdu); }
EXPORT_SYMBOL(pdu_create_from_ni);

/* FIXME: This code must be completely re-written */
static struct pdu * pdu_create_with_gfp(gfp_t        flags,
                                        struct sdu * sdu)
{
        struct pdu * tmp;

        /* Just to prevent complaints from sdu_destroy */
        if (!sdu_is_ok(sdu))
                return NULL;

        /*
         * FIXME: We use pdu_create_from_gfp to mimic the intended behavior
         *        of pdu_create_with_gfp. This implementation has to be fixed.
         */

        tmp = pdu_create_from_gfp(flags, sdu);

        /*
         * NOTE: We took ownership of the SDU and "theoretically" we built a
         *       PDU from the SDU. Due to this crap implementation, we're
         *       destroying the input SDU now ...
         */
        sdu_destroy(sdu);

        return tmp;
}

struct pdu * pdu_create_with(struct sdu * sdu)
{ return pdu_create_with_gfp(GFP_KERNEL, sdu); }
EXPORT_SYMBOL(pdu_create_with);

struct pdu * pdu_create_with_ni(struct sdu * sdu)
{ return pdu_create_with_gfp(GFP_ATOMIC, sdu); }
EXPORT_SYMBOL(pdu_create_with_ni);

static struct pdu * pdu_dup_gfp(gfp_t              flags,
                                const struct pdu * pdu)
{
        struct pdu * tmp;

        if (!pdu_is_ok(pdu))
                return NULL;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->pci    = pci_dup_gfp(flags, pdu->pci);
        tmp->buffer = buffer_dup_gfp(flags, pdu->buffer);

        if (!pdu_is_ok(tmp)) {
                pdu_destroy(tmp);
                return NULL;
        }

        return tmp;
}

struct pdu * pdu_dup(const struct pdu * pdu)
{ return pdu_dup_gfp(GFP_KERNEL, pdu); }
EXPORT_SYMBOL(pdu_dup);

struct pdu * pdu_dup_ni(const struct pdu * pdu)
{ return pdu_dup_gfp(GFP_ATOMIC, pdu); }
EXPORT_SYMBOL(pdu_dup_ni);

const struct buffer * pdu_buffer_get_ro(const struct pdu * pdu)
{
        if (!pdu_is_ok(pdu))
                return NULL;

        return pdu->buffer;
}
EXPORT_SYMBOL(pdu_buffer_get_ro);

struct buffer * pdu_buffer_get_rw(struct pdu * pdu)
{
        if (!pdu_is_ok(pdu))
                return NULL;

        return pdu->buffer;
}
EXPORT_SYMBOL(pdu_buffer_get_rw);

int pdu_buffer_disown(struct pdu * pdu)
{
        if (!pdu)
                return -1;

        pdu->buffer = NULL;
        return 0;
}
EXPORT_SYMBOL(pdu_buffer_disown);

int pdu_buffer_set(struct pdu * pdu, struct buffer * buffer)
{
        if (!pdu)
                return -1;

        if (pdu->buffer) {
                buffer_destroy(pdu->buffer);
        }
        pdu->buffer = buffer;

        return 0;
}
EXPORT_SYMBOL(pdu_buffer_set);

const struct pci * pdu_pci_get_ro(const struct pdu * pdu)
{
        if (!pdu_is_ok(pdu))
                return NULL;

        return pdu->pci;
}
EXPORT_SYMBOL(pdu_pci_get_ro);

bool pdu_pci_present(const struct pdu * pdu)
{
        if (!pdu_is_ok(pdu))
                return false;

        return pdu->pci ? true : false;
}
EXPORT_SYMBOL(pdu_pci_present);

struct pci * pdu_pci_get_rw(struct pdu * pdu)
{
        if (!pdu_is_ok(pdu))
                return NULL;

        return pdu->pci;
}
EXPORT_SYMBOL(pdu_pci_get_rw);

int pdu_pci_set(struct pdu * pdu, struct pci * pci)
{
        if (!pdu)
                return -1;

        if (!pci_is_ok(pci))
                return -1;

        pdu->pci = pci;

        return 0;
}
EXPORT_SYMBOL(pdu_pci_set);

int pdu_destroy(struct pdu * p)
{
        if (!p)
                return -1;

        if (p->pci)    pci_destroy(p->pci);
        if (p->buffer) buffer_destroy(p->buffer);

        rkfree(p);

        return 0;
}
EXPORT_SYMBOL(pdu_destroy);
