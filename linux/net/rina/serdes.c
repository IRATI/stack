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
/* FIXME: To be also defined in dt_cons when rate based fc is added */
#define RATE_LEN 0
#define TIME_LEN 0
/* FIXME: To be added in dt_cons ASAP */
#define CTRL_SEQ_NR 4

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

struct pdu_ser {
        struct buffer * buf;
};

static bool serdes_pdu_is_ok(const struct pdu_ser * s)
{ return (s && buffer_is_ok(s->buf)) ? true : false; }

static int base_pci_size(const struct dt_cons * dt_cons)
{ 
        return VERSION_SIZE + 
                2 * dt_cons->address_length +
                dt_cons->qos_id_length +
                2 * dt_cons->cep_id_length + 
                PDU_TYPE_SIZE +
                FLAGS_SIZE +
                dt_cons->length_length;
}

static int fc_pci_size(const struct dt_cons * dt_cons) 
{
        return 2 * dt_cons->seq_num_length +
                2 * RATE_LEN +
                TIME_LEN;
}
                
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

        ASSERT(data);
        ASSERT(dt_cons);
        ASSERT(pci_is_ok(pci));
        ASSERT(pdu_len);

        /* 
         * Putting 1 as version number for now 
         * If another version is needed, this will have to change
         * Always 8 bit long
         */
        offset = 0;
        memcpy(data + offset, 
               &version, 
               VERSION_SIZE);
        offset += VERSION_SIZE;

        addr = pci_destination(pci);
        memcpy(data + offset, 
               &addr, 
               dt_cons->address_length);
        offset += dt_cons->address_length;

        addr = pci_source(pci);
        memcpy(data + offset, 
               &addr, 
               dt_cons->address_length);
        offset += dt_cons->address_length;

        qos = pci_qos_id(pci);
        memcpy(data + offset, 
               &qos, 
               dt_cons->qos_id_length);
        offset += dt_cons->qos_id_length;

        cep = pci_cep_source(pci);
        memcpy(data + offset, 
               &cep, 
               dt_cons->cep_id_length);
        offset += dt_cons->cep_id_length;

        cep = pci_cep_destination(pci);
        memcpy(data + offset, 
               &cep, 
               dt_cons->cep_id_length);
        offset += dt_cons->cep_id_length;

        type = pci_type(pci);
        memcpy(data + offset, 
               &type,
               PDU_TYPE_SIZE);
        offset += PDU_TYPE_SIZE;

        flags = pci_flags_get(pci);
        memcpy(data + offset, 
               &flags,
               FLAGS_SIZE);
        offset += FLAGS_SIZE;

        memcpy(data + offset, 
               &pdu_len,
               dt_cons->length_length);
        offset += dt_cons->length_length;

        return 0;
}

static int construct_ack_pci(char *                 data, 
                             const struct dt_cons * dt_cons,
                             const struct pci *     pci, 
                             int                    offset) 
{
        seq_num_t ack_seq;

        ASSERT(data);
        ASSERT(dt_cons);
        ASSERT(pci);
        ASSERT(offset);

        ack_seq = pci_control_ack_seq_num(pci);
        memcpy(data + offset, 
               &ack_seq,
               dt_cons->seq_num_length);

        return 0;
}

static int construct_fc_pci(char *                 data, 
                            const struct dt_cons * dt_cons,
                            const struct pci *     pci, 
                            int                    offset) 
{
        seq_num_t seq;

        ASSERT(data);
        ASSERT(dt_cons);
        ASSERT(pci);
        ASSERT(offset);

        /* 
         * Not filling in rate-based fields for now 
         * since they are not defined as a type either 
         * Add them when needed
         */
        seq = pci_control_new_rt_wind_edge(pci);
        memcpy(data + offset, 
               &seq, 
               dt_cons->seq_num_length);
        offset += dt_cons->seq_num_length;
        
        seq = pci_control_left_wind_edge(pci);
        memcpy(data + offset, 
               &seq, 
               dt_cons->seq_num_length);
        offset += dt_cons->seq_num_length;
        
        seq = pci_control_rt_wind_edge(pci);
        memcpy(data + offset, 
               &seq, 
               dt_cons->seq_num_length);
        
        return 0;
}

static int deconstruct_base_pci(struct pci *           new_pci,
                                const struct dt_cons * dt_cons,
                                int *                  offset,
                                const uint8_t *        ptr,
                                ssize_t *              pdu_len) 
{
        int         vers;
        address_t   addr;
        cep_id_t    cep;
        qos_id_t    qos;
        pdu_type_t  pdu_type;
        pdu_flags_t pdu_flags;
        
        ASSERT(new_pci);
        ASSERT(dt_cons);
        ASSERT(ptr);

        *offset = 0;
        memcpy(&vers, 
               ptr + *offset, 
               VERSION_SIZE);
        *offset += VERSION_SIZE;

        if (vers != version) {
                LOG_ERR("Received an unknown version of the EFCP PDU");
                return -1;
        }

        memcpy(&addr, 
               ptr + *offset, 
               dt_cons->address_length);
        *offset += dt_cons->address_length;
        if (pci_destination_set(new_pci, addr)) {
                return -1;
        }

        memcpy(&addr, 
               ptr + *offset, 
               dt_cons->address_length);
        *offset += dt_cons->address_length;
        if (pci_source_set(new_pci, addr)) {
                return -1;
        }

        memcpy(&qos, 
               ptr + *offset, 
               dt_cons->qos_id_length);
        *offset += dt_cons->qos_id_length;
        if (pci_qos_id_set(new_pci, qos)) {
                return -1;
        }

        memcpy(&cep, 
               ptr + *offset, 
               dt_cons->cep_id_length);
        *offset += dt_cons->cep_id_length;
        if (pci_cep_source_set(new_pci, cep)) {
                return -1;
        }

        memcpy(&cep, 
               ptr + *offset, 
               dt_cons->cep_id_length);
        *offset += dt_cons->cep_id_length;
        if (pci_cep_destination_set(new_pci, cep)) {
                return -1;
        }
        
        memcpy(&pdu_type, 
               ptr + *offset, 
               PDU_TYPE_SIZE);
        *offset += PDU_TYPE_SIZE;
        if (pci_type_set(new_pci, pdu_type)) {
                return -1;
        }

        memcpy(&pdu_flags, 
               ptr + *offset, 
               FLAGS_SIZE);
        *offset += FLAGS_SIZE;
        if (pci_flags_set(new_pci, pdu_flags)) {
                return -1;
        }

        memcpy(pdu_len, 
               ptr + *offset, 
               dt_cons->length_length);
        *offset += dt_cons->length_length;

        return 0;
}


static int deconstruct_fc_pci(struct pci *           new_pci,
                              const struct dt_cons * dt_cons,
                              int *                  offset,
                              const uint8_t *        ptr) 
{
        seq_num_t seq;
        
        ASSERT(new_pci);
        ASSERT(dt_cons);
        ASSERT(offset);
        ASSERT(ptr);

        memcpy(&seq, 
               ptr + *offset, 
               dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_new_rt_wind_edge_set(new_pci, seq)) {
                return -1;
        }
                
        /* 
         * Note that the same applies here as before
         * Rate based has to be added
         */

        memcpy(&seq, 
               ptr + *offset, 
               dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_left_wind_edge_set(new_pci, seq)) {
                return -1;
        }
                
        memcpy(&seq, 
               ptr + *offset, 
               dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_rt_wind_edge_set(new_pci, seq)) {
                return -1;
        }
        
        return 0;
}


static int deconstruct_ack_pci(struct pci *           new_pci,
                               const struct dt_cons * dt_cons,
                               int *                  offset,
                               const uint8_t *        ptr) 
{
        seq_num_t seq;
        
        ASSERT(new_pci);
        ASSERT(dt_cons);
        ASSERT(offset);
        ASSERT(ptr);

        memcpy(&seq, 
               ptr + *offset, 
               dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_ack_seq_num_set(new_pci, seq)) {
                return -1;
        }
             
        return 0;
}

/* Note: Already passing dt_cons here for future work */
static int deconstruct_ctrl_seq(struct pci *           new_pci,
                                const struct dt_cons * dt_cons,
                                int *                  offset,
                                const uint8_t *        ptr) 
{
        seq_num_t seq;
        
        ASSERT(new_pci);
        ASSERT(dt_cons);
        ASSERT(offset);
        ASSERT(ptr);

        memcpy(&seq, 
               ptr + *offset, 
               CTRL_SEQ_NR);
        *offset += CTRL_SEQ_NR;
        if (pci_sequence_number_set(new_pci, seq)) {
                return -1;
        }

        return 0;
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
        seq_num_t             seq;
        
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

        buffer_size = buffer_length(buffer);
        if (buffer_size <= 0) {
                return NULL;
        }
        LOG_DBG("Buffer size is %zd", buffer_size);

        pci = pdu_pci_get_ro(pdu);
        if (!pci) {
                return NULL;
        }

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
         * Set size in first switch/case 
         */
        switch (pdu_type) {
        case PDU_TYPE_MGMT:
        case PDU_TYPE_DT:
                LOG_DBG("OMGWTFBBQ, it is a DT PDU");
                             
                size = pci_size + dt_cons->seq_num_length + buffer_size;
                if (size <= 0) {
                        pdu_destroy(pdu);
                        return NULL;
                }
                LOG_DBG("Total size is %zd", size);

                break;
        case PDU_TYPE_FC:
                LOG_DBG("OMGWTFBBQ, it is a FC PDU");

                size = pci_size + CTRL_SEQ_NR + fc_pci_size(dt_cons);
                if (size <= 0) {
                        pdu_destroy(pdu);
                        return NULL;
                }
                LOG_DBG("Total size is %zd", size);

                break;
        case PDU_TYPE_ACK:
                LOG_DBG("OMGWTFBBQ, it is an ACK PDU");

                size = pci_size + CTRL_SEQ_NR + dt_cons->seq_num_length;
                if (size <= 0) {
                        pdu_destroy(pdu);
                        return NULL;
                }
                LOG_DBG("Total size is %zd", size);
               
                break;
        case PDU_TYPE_ACK_AND_FC:
                LOG_DBG("OMGWTFBBQ, it is an ACK and FC PDU");

                size = pci_size + 
                        CTRL_SEQ_NR + 
                        fc_pci_size(dt_cons) + 
                        dt_cons->seq_num_length;
                if (size <= 0) {
                        pdu_destroy(pdu);
                        return NULL;
                }
                LOG_DBG("Total size is %zd", size);

                break;
        default:
                LOG_ERR("Unknown PDU type %02X", pdu_type);
                return NULL;
        }

        data = rkmalloc(size, flags);
        if (!data) {
                pdu_destroy(pdu);
                return NULL;
        }

        /* Needed for all PDUs */
        if (construct_base_pci(data, dt_cons, pci, size)) {
                LOG_ERR("Failed to construct base PCI");
                rkfree(data);
                pdu_destroy(pdu);
                return NULL;
        }

        /* Do actual serializing in the second switch case */
        switch (pdu_type) {
        case PDU_TYPE_MGMT:
        case PDU_TYPE_DT:
                seq = pci_sequence_number_get(pci);
                memcpy(data + pci_size, 
                       &seq,
                       dt_cons->seq_num_length);

                memcpy(data + pci_size + 
                       dt_cons->seq_num_length, 
                       buffer_data, 
                       buffer_size);

                break;
        case PDU_TYPE_FC:
                seq = pci_sequence_number_get(pci);
                memcpy(data + pci_size,
                       &seq,
                       CTRL_SEQ_NR);

                if (construct_fc_pci(data, 
                                     dt_cons, 
                                     pci, 
                                     pci_size + CTRL_SEQ_NR)) {
                        LOG_ERR("Failed to construct FC PCI");
                        rkfree(data);
                        pdu_destroy(pdu);
                }
               
                break;
        case PDU_TYPE_ACK:
                seq = pci_sequence_number_get(pci);
                memcpy(data + pci_size,
                       &seq,
                       CTRL_SEQ_NR);

                if (construct_ack_pci(data, 
                                      dt_cons, 
                                      pci, 
                                      pci_size + CTRL_SEQ_NR)) {
                        LOG_ERR("Failed to construct ACK PCI");
                        rkfree(data);
                        pdu_destroy(pdu);
                }
               
                break;
        case PDU_TYPE_ACK_AND_FC:
                seq = pci_sequence_number_get(pci);
                memcpy(data + pci_size,
                       &seq,
                       CTRL_SEQ_NR);

                if (construct_ack_pci(data, 
                                      dt_cons, 
                                      pci, 
                                      pci_size + CTRL_SEQ_NR)) {
                        LOG_ERR("Failed to construct ACK PCI");
                        rkfree(data);
                        pdu_destroy(pdu);
                }

                if (construct_fc_pci(data, 
                                     dt_cons, 
                                     pci, 
                                     pci_size + 
                                     CTRL_SEQ_NR + 
                                     dt_cons->seq_num_length)) {
                        LOG_ERR("Failed to construct FC PCI");
                        rkfree(data);
                        pdu_destroy(pdu);
                }

                break;
        default:
                LOG_ERR("Unknown PDU type %02X", pdu_type);
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
        struct pdu *          new_pdu;
        const struct buffer * tmp_buff;
        struct buffer *       new_buff;
        struct pci *          new_pci;
        const uint8_t *       ptr;
        int                   offset;
        ssize_t               pdu_len;
        seq_num_t             seq;

        if (!serdes_pdu_is_ok(pdu))
                return NULL;

        tmp_buff = serdes_pdu_buffer(pdu);
        ASSERT(tmp_buff);

        if (buffer_length(tmp_buff) < base_pci_size(dt_cons))
                return NULL;

        ptr = (const uint8_t *) buffer_data_ro(tmp_buff);
        ASSERT(ptr);

        new_pdu = pdu_create_gfp(flags);
        if (!new_pdu)
                return NULL;

        new_pci = pci_create_gfp(flags);
        if (!new_pci) {
                pdu_destroy(new_pdu);
                return NULL;
        }
       
        if (deconstruct_base_pci(new_pci, dt_cons, &offset, ptr, &pdu_len)) {
                pdu_destroy(new_pdu);
                return NULL;
        }
      
        LOG_DBG("PCI Type: %02X", pci_type(new_pci));
        LOG_DBG("PCI Source: %d", pci_source(new_pci));
        LOG_DBG("PCI Destination: %d", pci_destination(new_pci));
        LOG_DBG("PCI Source CEP: %d", pci_cep_source(new_pci));
        LOG_DBG("PCI Destination CEP: %d", pci_cep_destination(new_pci));
        LOG_DBG("PCI QoS ID: %d", pci_qos_id(new_pci));
        LOG_DBG("PCI Flags: %d", pci_flags_get(new_pci));

        switch (pci_type(new_pci)) {
        case PDU_TYPE_MGMT:
        case PDU_TYPE_DT:
                LOG_DBG("OMGWTFBBQ, it is a MGMT/DT PDU");
                /* Create buffer with rest of PDU if it is a DT or MGMT PDU*/
                memcpy(&seq, 
                       ptr + offset, 
                       dt_cons->seq_num_length);
                offset += dt_cons->seq_num_length;
                if (pci_sequence_number_set(new_pci, seq)) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                new_buff = buffer_create_from_gfp(flags,
                                                  ptr + offset,
                                                  pdu_len - offset);
                if (!new_buff) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                if (!buffer_is_ok(new_buff)) {
                        LOG_ERR("Buffer is NOT okay");
                }

                break;
        case PDU_TYPE_FC:
                if (deconstruct_ctrl_seq(new_pci, dt_cons, &offset, ptr)) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                if (deconstruct_fc_pci(new_pci, dt_cons, &offset, ptr)) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                /* 
                 * FIXME: It would be better if a PDU consisted of 
                 * Base PCI + Data | Control PDU stuff
                 * e.g.
                 * struct pdu {
                 *    struct pci base_pci;
                 *    union {
                 *       struct buffer * data;
                 *       struct pci_ctrl * ctrl;
                 *     }
                 * }
                 */
                new_buff = buffer_create_gfp(flags, 1);
                if (!new_buff) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                break;
        case PDU_TYPE_ACK:
                if (deconstruct_ctrl_seq(new_pci, dt_cons, &offset, ptr)) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }
                 
                if (deconstruct_ack_pci(new_pci, dt_cons, &offset, ptr)) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                new_buff = buffer_create_gfp(flags, 1);
                if (!new_buff) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }
                break;
        case PDU_TYPE_ACK_AND_FC:
                LOG_DBG("OMGWTFBBQ, it is an ACK/FC PDU");
                if (deconstruct_ctrl_seq(new_pci, dt_cons, &offset, ptr)) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                if (deconstruct_ack_pci(new_pci, dt_cons, &offset, ptr)) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }
      
                if (deconstruct_fc_pci(new_pci, dt_cons, &offset, ptr)) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                new_buff = buffer_create_gfp(flags, 1);
                if (!new_buff) {
                        pdu_destroy(new_pdu);
                        return NULL;
                }
                break;
        default:
                LOG_ERR("Unknown PDU type %02X", pci_type(new_pci));
                pdu_destroy(new_pdu);
                return NULL;
        }

        if (pdu_pci_set(new_pdu, new_pci)) {
                LOG_ERR("Failed to set PCI in PDU");
                pdu_destroy(new_pdu);
                return NULL;
        }
       
        if (pdu_buffer_set(new_pdu, new_buff)) {
                LOG_ERR("Failed to set buffer in PDU");
                pdu_destroy(new_pdu);
                return NULL;
        }

        if (!pdu_is_ok(new_pdu)) {
                LOG_ERR("PDU is not okay");
        }

        ASSERT(pdu_is_ok(new_pdu));

        serdes_pdu_destroy(pdu);

        return new_pdu;        
}


struct pdu * serdes_pdu_deser(const struct dt_cons * dt_cons,
                              struct pdu_ser *       pdu)
{ return serdes_pdu_deser_gfp(GFP_KERNEL, dt_cons, pdu); }
EXPORT_SYMBOL(serdes_pdu_deser);
