/*
 * PDU Serialization/Deserialization 
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#define RINA_PREFIX "serdes"

#include "serdes.h"
#include "logs.h"
#include "utils.h"
#include "debug.h"

struct pdu_ser {
        struct buffer * buf;
};

static bool serdes_pdu_is_ok(const struct pdu_ser * s)
{ return (s && buffer_is_ok(s->buf)) ? true : false; }

static struct pdu_ser * serdes_pdu_ser_gfp(gfp_t flags,
                                           struct pdu * pdu)
{
        struct pdu_ser *      tmp;
        const void *          buffer_data;
        const struct buffer * buffer;
        const struct pci *    pci;
        ssize_t               buffer_size;
        ssize_t               pci_size;
        
        if (!pdu_is_ok(pdu))
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

        /* TODO: Serialize the PCI here */

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

        memcpy(data, pci, pci_size);
        memcpy(data + pci_size, buffer_data, buffer_size);

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp) {
                rkfree(data);
                return NULL;
        }

        tmp->buf = buffer_create_with_gfp(flags, data, size);
        if (!tmp->buf) {
                rkfree(data);
                rkfree(tmp);
                return NULL;
        } 

        pdu_destroy(pdu);

        return tmp;
}

struct pdu_ser * serdes_pdu_ser(struct pdu * pdu)
{ return serdes_pdu_ser_gfp(GFP_KERNEL, pdu); }
EXPORT_SYMBOL(serdes_pdu_ser);

struct buffer * serdes_pdu_buffer(struct pdu_ser * pdu)
{
        if (!serdes_pdu_is_ok(pdu))
                return NULL;
        
        return pdu->buf;
}
EXPORT_SYMBOL(serdes_pdu_buffer);

int serdes_pdu_destroy(struct pdu_ser * pdu) 
{
        if (!pdu) return -1;

        if (pdu->buf)
                buffer_destroy(pdu->buf);

        rkfree(pdu);

        return 0;
}
EXPORT_SYMBOL(serdes_pdu_destroy);



struct pdu * serdes_pdu_deser(struct pdu_ser * pdu) 
{
        LOG_MISSING;
        
        return NULL;
}
EXPORT_SYMBOL(serdes_pdu_deser);
