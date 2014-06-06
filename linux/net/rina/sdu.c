/*
 * Service Data Unit
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

#define RINA_PREFIX "sdu"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"

/* FIXME: These externs have to disappear from here */
struct buffer * buffer_create_with_gfp(gfp_t  flags,
                                       void * data,
                                       size_t size);
struct buffer * buffer_create_from_gfp(gfp_t        flags,
                                       const void * data,
                                       size_t       size);
struct buffer * buffer_dup_gfp(gfp_t                 flags,
                               const struct buffer * b);

static struct sdu * sdu_create_buffer_with_gfp(gfp_t           flags,
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

struct sdu * sdu_create_buffer_with(struct buffer * buffer)
{ return sdu_create_buffer_with_gfp(GFP_KERNEL, buffer); }
EXPORT_SYMBOL(sdu_create_buffer_with);

struct sdu * sdu_create_buffer_with_ni(struct buffer * buffer)
{ return sdu_create_buffer_with_gfp(GFP_ATOMIC, buffer); }
EXPORT_SYMBOL(sdu_create_buffer_with_ni);

static struct sdu * sdu_create_pdu_with_gfp(gfp_t        flags,
                                            struct pdu * pdu)
{
        struct sdu *          sdu;
        const struct buffer * buffer;
        const struct pci *    pci;
        const void *          buffer_data;
        size_t                size;
        ssize_t               buffer_size;
        ssize_t               pci_size;
        struct buffer *       tmp_buff;
        char *                data;

        if (!pdu)
                return NULL;

        buffer = pdu_buffer_get_ro(pdu);
        if (!buffer) {
                pdu_destroy(pdu);
                return NULL;
        }

        buffer_data = buffer_data_ro(buffer);
        if (!buffer_data) {
                pdu_destroy(pdu);
                return NULL;
        }

        pci = pdu_pci_get_ro(pdu);
        if (!pci) {
                pdu_destroy(pdu);
                return NULL;
        }

        buffer_size = buffer_length(buffer);
        if (buffer_size <= 0) {
                pdu_destroy(pdu);
                return NULL;
        }

        pci_size = pci_length(pci);
        if (pci_size <= 0) {
                pdu_destroy(pdu);
                return NULL;
        }

        size = pci_size + buffer_size;
        data = rkmalloc(size, flags);
        if (!data) {
                pdu_destroy(pdu);
                return NULL;
        }

        /* FIXME: Useless check */
        if (!memcpy(data, pci, pci_size)) {
                rkfree(data);
                pdu_destroy(pdu);
                return NULL;
        }

        /* FIXME: Useless check */
        if (!memcpy(data + pci_size, buffer_data, buffer_size)) {
                rkfree(data);
                pdu_destroy(pdu);
                return NULL;
        }

        tmp_buff = buffer_create_with_gfp(flags, data, size);
        if (!tmp_buff) {
                rkfree(data);
                pdu_destroy(pdu);
                return NULL;
        }

        sdu = sdu_create_buffer_with(tmp_buff);
        if (!sdu) {
                pdu_destroy(pdu);
                buffer_destroy(tmp_buff);
                return NULL;
        }

        pdu_destroy(pdu);

        return sdu;
}

struct sdu * sdu_create_pdu_with(struct pdu * pdu)
{ return sdu_create_pdu_with_gfp(GFP_KERNEL, pdu); }
EXPORT_SYMBOL(sdu_create_pdu_with);

struct sdu * sdu_create_pdu_with_ni(struct pdu * pdu)
{ return sdu_create_pdu_with_gfp(GFP_ATOMIC, pdu); }
EXPORT_SYMBOL(sdu_create_pdu_with_ni);

int sdu_destroy(struct sdu * s)
{
        if (!s) return -1;

        if (s->buffer)
                buffer_destroy(s->buffer);

        rkfree(s);

        return 0;
}
EXPORT_SYMBOL(sdu_destroy);

const struct buffer * sdu_buffer_ro(const struct sdu * s)
{
        if (!sdu_is_ok(s))
                return NULL;

        return s->buffer;
}
EXPORT_SYMBOL(sdu_buffer_ro);

struct buffer * sdu_buffer_rw(struct sdu * s)
{
        if (!sdu_is_ok(s))
                return NULL;

        return s->buffer;
}
EXPORT_SYMBOL(sdu_buffer_rw);

static struct sdu * sdu_dup_gfp(gfp_t              flags,
                                const struct sdu * sdu)
{
        struct sdu * tmp;

        if (!sdu_is_ok(sdu))
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->buffer = buffer_dup_gfp(flags, sdu->buffer);

        return tmp;
}

struct sdu * sdu_dup(const struct sdu * sdu)
{ return sdu_dup_gfp(GFP_KERNEL, sdu); }
EXPORT_SYMBOL(sdu_dup);

struct sdu * sdu_dup_ni(const struct sdu * sdu)
{ return sdu_dup_gfp(GFP_ATOMIC, sdu); }
EXPORT_SYMBOL(sdu_dup_ni);

bool sdu_is_ok(const struct sdu * s)
{ return (s && buffer_is_ok(s->buffer)) ? true : false; }
EXPORT_SYMBOL(sdu_is_ok);

int sdu_buffer_disown(struct sdu * sdu)
{
        if (!sdu)
                return -1;

        sdu->buffer = NULL;
        return 0;
}
EXPORT_SYMBOL(sdu_buffer_disown);

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

static struct sdu_wpi * sdu_wpi_create_with_gfp(gfp_t           flags,
                                                struct buffer * buffer)
{
        struct sdu_wpi * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->sdu = sdu_create_buffer_with_gfp(flags, buffer);
        if (!tmp->sdu) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

struct sdu_wpi * sdu_wpi_create_with(struct buffer * buffer)
{ return sdu_wpi_create_with_gfp(GFP_KERNEL, buffer); }
EXPORT_SYMBOL(sdu_wpi_create_with);

struct sdu_wpi * sdu_wpi_create_with_ni(struct buffer * buffer)
{ return sdu_wpi_create_with_gfp(GFP_ATOMIC, buffer); }
EXPORT_SYMBOL(sdu_wpi_create_with_ni);

int sdu_wpi_destroy(struct sdu_wpi * s)
{
        if (!s) return -1;

        sdu_destroy(s->sdu);
        rkfree(s);

        return 0;
}
EXPORT_SYMBOL(sdu_wpi_destroy);

bool sdu_wpi_is_ok(const struct sdu_wpi * s)
{ return (s && sdu_is_ok(s->sdu)) ? true : false; }
EXPORT_SYMBOL(sdu_wpi_is_ok);
