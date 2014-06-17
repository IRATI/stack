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

#include <linux/export.h>
#include <linux/types.h>

#define RINA_PREFIX "serdes"

#include "serdes.h"
#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"
#include "ipcp-instances.h"
#include "buffer.h"
#include "pci.h"

#define VERSION_SIZE 1
/* Cannot be a literal for memcpy */
const uint8_t version = 1;
#define PDU_TYPE_SIZE 1
#define FLAGS_SIZE 1

/* FIXME: These externs have to disappear from here */
struct buffer * buffer_create_with_gfp(gfp_t  flags,
                                       void * data,
                                       size_t size);

struct buffer * buffer_create_from_gfp(gfp_t        flags,
                                       const void * data,
                                       size_t       size);

struct buffer * buffer_create_gfp(gfp_t  flags,
                                  size_t size);

struct pdu * pdu_create_gfp(gfp_t flags);

struct pci * pci_create_gfp(gfp_t flags);

                
static int construct_base_pci(char *                 data, 
                              const struct dt_cons * dt_cons,
                              const struct pci *     pci, 
                              size_t                 pdu_len) 
{         
        int         offset;
        address_t   addr;
        cep_id_t    cep;
        qos_id_t    qos;
        pdu_type_t  type;
        pdu_flags_t flags;
        seq_num_t   seq;

        ASSERT(data);
        ASSERT(dt_cons);
        ASSERT(pci_is_ok(pci));

        /* 
         * Putting 1 as version number for now 
         * If another version is needed, this will have to change
         * Always 8 bit long
         */
        offset = 0;
        LOG_DBG("Offset is now %d", offset);
        memcpy(data + offset, 
               &version, 
               VERSION_SIZE);
        offset += VERSION_SIZE;
        LOG_DBG("Offset is now %d", offset);

        addr = pci_destination(pci);
        memcpy(data + offset, 
               &addr, 
               dt_cons->address_length);
        offset += dt_cons->address_length;
        LOG_DBG("Offset is now %d", offset);

        addr = pci_source(pci);
        memcpy(data + offset, 
               &addr, 
               dt_cons->address_length);
        offset += dt_cons->address_length;
        LOG_DBG("Offset is now %d", offset);

        qos = pci_qos_id(pci);
        memcpy(data + offset, 
               &qos, 
               dt_cons->qos_id_length);
        offset += dt_cons->qos_id_length;
        LOG_DBG("Offset is now %d", offset);

        cep = pci_cep_source(pci);
        memcpy(data + offset, 
               &cep, 
               dt_cons->cep_id_length);
        offset += dt_cons->cep_id_length;
        LOG_DBG("Offset is now %d", offset);

        cep = pci_cep_destination(pci);
        memcpy(data + offset, 
               &cep, 
               dt_cons->cep_id_length);
        offset += dt_cons->cep_id_length;
        LOG_DBG("Offset is now %d", offset);

        type = pci_type(pci);
        memcpy(data + offset, 
               &type,
               PDU_TYPE_SIZE);
        offset += PDU_TYPE_SIZE;
        LOG_DBG("Offset is now %d", offset);

        flags = pci_flags_get(pci);
        memcpy(data + offset, 
               &flags,
               FLAGS_SIZE);
        offset += FLAGS_SIZE;
        LOG_DBG("Offset is now %d", offset);

        LOG_DBG("PDU Len is %zd", pdu_len);
        memcpy(data + offset, 
               &pdu_len,
               dt_cons->length_length);
        offset += dt_cons->length_length;
        LOG_DBG("Offset is now %d", offset);

        seq = pci_sequence_number_get(pci);
        memcpy(data + offset, 
               &seq,
               dt_cons->seq_num_length);

        return 0;
}


struct pdu_ser {
        struct buffer * buf;
};

static bool serdes_pdu_is_ok(const struct pdu_ser * s)
{ return (s && buffer_is_ok(s->buf)) ? true : false; }

static int base_pci_size(const struct dt_cons * dt_cons)
{ 
        LOG_DBG("Version: %d", VERSION_SIZE);
        LOG_DBG("Address Length: %d", dt_cons->address_length);
        LOG_DBG("QoS ID Length: %d", dt_cons->qos_id_length);
        LOG_DBG("CEP ID Length: %d", dt_cons->cep_id_length);
        LOG_DBG("PDU Type Size: %d", PDU_TYPE_SIZE);
        LOG_DBG("Flags Size: %d", FLAGS_SIZE);
        LOG_DBG("Length Length: %d", dt_cons->length_length);
        LOG_DBG("Sequence Number Length: %d", dt_cons->seq_num_length);

        return VERSION_SIZE + 
                2 * dt_cons->address_length +
                dt_cons->qos_id_length +
                2 * dt_cons->cep_id_length + 
                PDU_TYPE_SIZE +
                FLAGS_SIZE +
                dt_cons->length_length +
                dt_cons->seq_num_length;
}

static struct pdu_ser * serdes_pdu_ser_gfp(gfp_t                  flags,
                                           const struct dt_cons * dt_cons,
                                           struct pdu *           pdu)
{
        struct pdu_ser *      tmp;
        const void *          buffer_data;
        const struct buffer * buffer;
        const struct pci *    pci;
        size_t                size;
        ssize_t               buffer_size;
        ssize_t               pci_size;
        char *                data;
        pdu_type_t            pdu_type;
        
        if (!pdu_is_ok(pdu))
                return NULL;

        if (!dt_cons)
                return NULL;

        buffer = pdu_buffer_get_ro(pdu);
        if (!buffer) {
                return NULL;
        }

        buffer_data = buffer_data_ro(buffer);
        if (!buffer_data) {
                return NULL;
        }

        pci = pdu_pci_get_ro(pdu);
        if (!pci) {
                return NULL;
        }

        /* Serialize the PCI here */
        pdu_type = pci_type(pci);
        if (!pdu_type_is_ok(pdu_type)) {
                LOG_ERR("Wrong PDU type");
                return NULL;
        }
        LOG_DBG("PDU Type: %02X", pdu_type);

        /* Base PCI size, fields present in all PDUs */
        pci_size = base_pci_size(dt_cons);
        LOG_DBG("PCI size is %zd", pci_size);

        /* 
         * These are available in the stack at this point in time 
         * Extend if necessary (e.g.) NACKs, SACKs, ...
         */
        switch (pdu_type) {
        case PDU_TYPE_MGMT:
        case PDU_TYPE_DT:
                LOG_DBG("OMGWTFBBQ, it is a DT PDU");
                buffer_size = buffer_length(buffer);
                if (buffer_size <= 0) {
                        return NULL;
                }

                LOG_DBG("Buffer size is %zd", buffer_size);
                
                size = pci_size + buffer_size;
                if (pci_size <= 0) {
                        pdu_destroy(pdu);
                        return NULL;
                }
                LOG_DBG("Total size is %zd", size);
                
                data = rkmalloc(size, flags);
                if (!data) {
                        pdu_destroy(pdu);
                        return NULL;
                }

                if (construct_base_pci(data, dt_cons, pci, size)) {
                        LOG_ERR("Failed to construct base PCI");
                        rkfree(data);
                        pdu_destroy(pdu);
                        return NULL;
                }

                memcpy(data + pci_size, buffer_data, buffer_size);

                break;
        case PDU_TYPE_FC:
                LOG_DBG("OMGWTFBBQ, it is a FC PDU");
                /* Serialize other fields here (not needed for unreliable) */
                data = rkmalloc(pci_size, flags);
                if (!data) {
                        pdu_destroy(pdu);
                        return NULL;
                }
                size = pci_size;
                break;
        case PDU_TYPE_ACK:
                LOG_DBG("OMGWTFBBQ, it is an ACK  PDU");
                /* Serialize other fields here */
                data = rkmalloc(pci_size, flags);
                if (!data) {
                        pdu_destroy(pdu);
                        return NULL;
                }
                size = pci_size;
                break;
        case PDU_TYPE_ACK_AND_FC:
                LOG_DBG("OMGWTFBBQ, it is a ACK and FC PDU");
                /* Serialize other fields here */
                data = rkmalloc(pci_size, flags);
                if (!data) {
                        pdu_destroy(pdu);
                        return NULL;
                }
                size = pci_size;
                break;
        default:
                LOG_ERR("Unknown PDU type %d", pdu_type);
                return NULL;
        }

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp) {
                rkfree(data);
                return NULL;
        }

        LOG_DBG("Creating the buffer with size %zd", size);
        tmp->buf = buffer_create_with_gfp(flags, data, size);
        if (!tmp->buf) {
                rkfree(data);
                rkfree(tmp);
                return NULL;
        } 

        pdu_destroy(pdu);

        return tmp;
}

struct pdu_ser * serdes_pdu_ser(const struct dt_cons * dt_cons, 
                                struct pdu *           pdu)
{ return serdes_pdu_ser_gfp(GFP_KERNEL, dt_cons, pdu); }
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



struct pdu * serdes_pdu_deser_gfp(gfp_t                  flags,
                                  const struct dt_cons * dt_cons,
                                  struct pdu_ser *       pdu) 
{
        struct pdu *          tmp_pdu;
        const struct buffer * tmp_buff;
        struct buffer *       new_buff;
        struct pci *          new_pci;
        const uint8_t *       ptr;
        int                   offset;
        int                   vers;
        address_t             addr;
        cep_id_t              cep;
        ssize_t               pdu_len;
        qos_id_t              qos;
        pdu_type_t            pdu_type;
        pdu_flags_t           pdu_flags;
        seq_num_t             seq;

        if (!serdes_pdu_is_ok(pdu))
                return NULL;

        tmp_buff = serdes_pdu_buffer(pdu);
        ASSERT(tmp_buff);

        if (buffer_length(tmp_buff) < base_pci_size(dt_cons))
                return NULL;

        ptr = (const uint8_t *) buffer_data_ro(tmp_buff);
        ASSERT(ptr);


        tmp_pdu = pdu_create_gfp(flags);
        if (!tmp_pdu)
                return NULL;

        new_pci = pci_create_gfp(flags);
        if (!new_pci) {
                pdu_destroy(tmp_pdu);
                return NULL;
        }
        pdu_pci_set(tmp_pdu, new_pci);

        /* Now to parse all fields */
        offset = 0;
        memcpy(&vers, 
               ptr + offset, 
               VERSION_SIZE);
        offset += VERSION_SIZE;

        if (vers != version) {
                LOG_ERR("Received an unknown version of the EFCP PDU");
                pdu_destroy(tmp_pdu);
                return NULL;
        }

        memcpy(&addr, 
               ptr + offset, 
               dt_cons->address_length);
        offset += dt_cons->address_length;
        if (pci_destination_set(new_pci, addr)) {
                pdu_destroy(tmp_pdu);
                return NULL;
        }

        memcpy(&addr, 
               ptr + offset, 
               dt_cons->address_length);
        offset += dt_cons->address_length;
        if (pci_source_set(new_pci, addr)) {
                pdu_destroy(tmp_pdu);
                return NULL;
        }

        memcpy(&qos, 
               ptr + offset, 
               dt_cons->qos_id_length);
        offset += dt_cons->qos_id_length;
        if (pci_qos_id_set(new_pci, qos)) {
                pdu_destroy(tmp_pdu);
                return NULL;
        }

        memcpy(&cep, 
               ptr + offset, 
               dt_cons->cep_id_length);
        offset += dt_cons->cep_id_length;
        if (pci_cep_source_set(new_pci, cep)) {
                pdu_destroy(tmp_pdu);
                return NULL;
        }

        memcpy(&cep, 
               ptr + offset, 
               dt_cons->cep_id_length);
        offset += dt_cons->cep_id_length;
        if (pci_cep_destination_set(new_pci, cep)) {
                pdu_destroy(tmp_pdu);
                return NULL;
        }
        
        memcpy(&pdu_type, 
               ptr + offset, 
               PDU_TYPE_SIZE);
        offset += PDU_TYPE_SIZE;
        if (pci_type_set(new_pci, pdu_type)) {
                pdu_destroy(tmp_pdu);
                return NULL;
        }

        memcpy(&pdu_flags, 
               ptr + offset, 
               FLAGS_SIZE);
        offset += FLAGS_SIZE;
        if (pci_flags_set(new_pci, pdu_flags)) {
                pdu_destroy(tmp_pdu);
                return NULL;
        }

        memcpy(&pdu_len, 
               ptr + offset, 
               dt_cons->length_length);
        offset += dt_cons->length_length;

        memcpy(&seq, 
               ptr + offset, 
               dt_cons->seq_num_length);
        offset += dt_cons->seq_num_length;
        if (pci_sequence_number_set(new_pci, seq)) {
                pdu_destroy(tmp_pdu);
                return NULL;
        }

        switch (pdu_type) {
        case PDU_TYPE_MGMT:
        case PDU_TYPE_DT:
       
                /* Create buffer with rest of PDU if it is a DT or MGMT PDU*/
                new_buff = buffer_create_from_gfp(flags,
                                                  ptr + offset,
                                                  pdu_len);
                if (!new_buff) {
                        pdu_destroy(tmp_pdu);
                        return NULL;
                }

        case PDU_TYPE_FC:
        case PDU_TYPE_ACK:
        case PDU_TYPE_ACK_AND_FC:
                /* Buffer size as small as possible */
                new_buff = buffer_create_gfp(flags, 1);
                if (!new_buff) {
                        pdu_destroy(tmp_pdu);
                        return NULL;
                }
        default:
                LOG_ERR("Unknown PDU type %d", pdu_type);
                pdu_destroy(tmp_pdu);
                return NULL;
        }

        pdu_buffer_set(tmp_pdu, new_buff);

        ASSERT(pdu_is_ok(tmp_pdu));

        serdes_pdu_destroy(pdu);

        return tmp_pdu;        
}


struct pdu * serdes_pdu_deser(const struct dt_cons * dt_cons,
                              struct pdu_ser *       pdu)
{ return serdes_pdu_deser_gfp(GFP_KERNEL, dt_cons, pdu); }
EXPORT_SYMBOL(serdes_pdu_deser);
