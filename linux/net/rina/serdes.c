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
#include <linux/crypto.h>

#define RINA_PREFIX "serdes"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "serdes.h"
#include "pdu-ser.h"
#include "du.h"
#include "ipcp-instances.h"
#include "buffer.h"
#include "pci.h"
#include "du-protection.h"

/* FIXME: This is wrong, use a version value and use sifeof later */
#define VERSION_SIZE  1

/* FIXME: Remove this useless variable, we have to have a define for that */
/* Cannot be a literal for memcpy */
const uint8_t version = 1;

#define PDU_TYPE_SIZE 1
#define FLAGS_SIZE    1
/* FIXME: To be also defined in dt_cons when rate based fc is added */
#define RATE_LEN      0
#define TIME_LEN      0
/* FIXME: To be added in dt_cons ASAP */
#define CTRL_SEQ_NR   4

/* FIXME: These externs have to disappear from here */
struct pdu *     pdu_create_gfp(gfp_t flags);

struct pci *     pci_create_gfp(gfp_t flags);

struct buffer *  buffer_create_with_gfp(gfp_t  flags,
                                        void * data,
                                        size_t size);

struct buffer *  buffer_create_from_gfp(gfp_t        flags,
                                        const void * data,
                                        size_t       size);

struct buffer *  buffer_create_gfp(gfp_t  flags,
                                   size_t size);

struct pdu_ser * pdu_ser_create_buffer_with_gfp(gfp_t           flags,
                                                struct buffer * buffer);

struct serdes {
        struct dt_cons * dt_cons;
};

static struct serdes * serdes_create_gfp(gfp_t            flags,
                                         struct dt_cons * dt_cons)
{
        struct serdes * tmp;

        if (!dt_cons)
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->dt_cons = dt_cons;

        return tmp;
}

struct serdes * serdes_create(struct dt_cons * dt_cons)
{ return serdes_create_gfp(GFP_KERNEL, dt_cons); }
EXPORT_SYMBOL(serdes_create);

const struct dt_cons * serdes_dt_cons(const struct serdes * instance)
{ return instance ? instance->dt_cons : NULL; }
EXPORT_SYMBOL(serdes_dt_cons);

int serdes_destroy(struct serdes * instance)
{
        if (!instance)
                return -1;

        if (instance->dt_cons)
                rkfree(instance->dt_cons);

        rkfree(instance);

        return 0;
}
EXPORT_SYMBOL(serdes_destroy);

static int base_pci_size(const struct dt_cons * dt_cons)
{
        return VERSION_SIZE                 +
                2 * dt_cons->address_length +
                dt_cons->qos_id_length      +
                2 * dt_cons->cep_id_length  +
                PDU_TYPE_SIZE               +
                FLAGS_SIZE                  +
                dt_cons->length_length;
}

static int fc_pci_size(const struct dt_cons * dt_cons)
{
        return 3 * dt_cons->seq_num_length +
                2 * RATE_LEN               +
                TIME_LEN;
}

static int serialize_base_pci(const struct serdes * instance,
                              char *                data,
                              const struct pci *    pci,
                              size_t                pdu_len)
{
        int              offset;
        address_t        addr;
        cep_id_t         cep;
        qos_id_t         qos;
        pdu_type_t       type;
        pdu_flags_t      flags;
        struct dt_cons * dt_cons;

        ASSERT(instance);
        ASSERT(data);
        ASSERT(pci_is_ok(pci));
        ASSERT(pdu_len);

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        /*
         * Putting 1 as version number for now
         * If another version is needed, this will have to change
         * Always 8 bit long
         */
        offset = 0;
        memcpy(data + offset, &version, VERSION_SIZE);
        offset += VERSION_SIZE;
        LOG_DBG("Serialized version %d, with size %d", version, VERSION_SIZE);

        addr = pci_destination(pci);
        memcpy(data + offset, &addr, dt_cons->address_length);
        offset += dt_cons->address_length;

        addr = pci_source(pci);
        memcpy(data + offset, &addr, dt_cons->address_length);
        offset += dt_cons->address_length;

        qos = pci_qos_id(pci);
        memcpy(data + offset, &qos, dt_cons->qos_id_length);
        offset += dt_cons->qos_id_length;

        cep = pci_cep_source(pci);
        memcpy(data + offset, &cep, dt_cons->cep_id_length);
        offset += dt_cons->cep_id_length;

        cep = pci_cep_destination(pci);
        memcpy(data + offset, &cep, dt_cons->cep_id_length);
        offset += dt_cons->cep_id_length;

        type = pci_type(pci);
        memcpy(data + offset, &type, PDU_TYPE_SIZE);
        offset += PDU_TYPE_SIZE;

        flags = pci_flags_get(pci);
        memcpy(data + offset, &flags, FLAGS_SIZE);
        offset += FLAGS_SIZE;

        memcpy(data + offset, &pdu_len, dt_cons->length_length);
        offset += dt_cons->length_length;

        return 0;
}

static int serialize_ack_pci(const struct serdes * instance,
                             char *                data,
                             const struct pci *    pci,
                             int                   offset)
{
        seq_num_t        ack_seq;
        struct dt_cons * dt_cons;

        ASSERT(instance);
        ASSERT(data);
        ASSERT(pci);
        ASSERT(offset);

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        ack_seq = pci_control_ack_seq_num(pci);
        memcpy(data + offset, &ack_seq, dt_cons->seq_num_length);

        return 0;
}

/* Passing dt_cons already, for future work */
static int serialize_ctrl_seq(const struct serdes * instance,
                              char *                data,
                              const struct pci *    pci,
                              int                   offset)
{
        seq_num_t        seq;
        struct dt_cons * dt_cons;

        ASSERT(instance);
        ASSERT(data);
        ASSERT(pci);
        ASSERT(offset);

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        seq = pci_sequence_number_get(pci);
        memcpy(data + offset, &seq, CTRL_SEQ_NR);

        return 0;
}

static int serialize_cc_pci(const struct serdes * instance,
                            char *                data,
                            const struct pci *    pci,
                            int                   offset)
{
        seq_num_t        seq;
        struct dt_cons * dt_cons;

        ASSERT(instance);
        ASSERT(data);
        ASSERT(pci);
        ASSERT(offset);

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        seq = pci_control_last_seq_num_rcvd(pci);
        memcpy(data + offset, &seq, CTRL_SEQ_NR);

        seq = pci_control_new_left_wind_edge(pci);
        memcpy(data + offset, &seq, dt_cons->seq_num_length);
        offset += dt_cons->seq_num_length;

        seq = pci_control_new_rt_wind_edge(pci);
        memcpy(data + offset, &seq, dt_cons->seq_num_length);
        offset += dt_cons->seq_num_length;

        seq = pci_control_my_left_wind_edge(pci);
        memcpy(data + offset, &seq, dt_cons->seq_num_length);
        offset += dt_cons->seq_num_length;

        seq = pci_control_my_rt_wind_edge(pci);
        memcpy(data + offset, &seq, dt_cons->seq_num_length);
        offset += dt_cons->seq_num_length;

        /* Add MyRcvRate here in the future */

        return 0;
}

static int serialize_fc_pci(const struct serdes * instance,
                            char *                data,
                            const struct pci *    pci,
                            int                   offset)
{
        seq_num_t        seq;
        struct dt_cons * dt_cons;

        ASSERT(instance);
        ASSERT(data);
        ASSERT(pci);
        ASSERT(offset);

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        /*
         * Not filling in rate-based fields for now
         * since they are not defined as a type either
         * Add them when needed
         */
        seq = pci_control_new_rt_wind_edge(pci);
        memcpy(data + offset, &seq, dt_cons->seq_num_length);
        offset += dt_cons->seq_num_length;

        seq = pci_control_my_left_wind_edge(pci);
        memcpy(data + offset, &seq, dt_cons->seq_num_length);
        offset += dt_cons->seq_num_length;

        seq = pci_control_my_rt_wind_edge(pci);
        memcpy(data + offset, &seq, dt_cons->seq_num_length);

        return 0;
}

static int deserialize_base_pci(const struct serdes * instance,
                                struct pci *          new_pci,
                                int *                 offset,
                                const uint8_t *       ptr,
                                ssize_t *             pdu_len)
{
        int              vers;
        address_t        addr;
        cep_id_t         cep;
        qos_id_t         qos;
        pdu_type_t       pdu_type;
        pdu_flags_t      pdu_flags;
        struct dt_cons * dt_cons;

        ASSERT(instance);
        ASSERT(new_pci);
        ASSERT(ptr);

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        vers      = 0;
        addr      = 0;
        cep       = 0;
        qos       = 0;
        pdu_type  = 0x00;
        pdu_flags = 0x0;

        *offset = 0;
        memcpy(&vers, ptr + *offset, VERSION_SIZE);
        *offset += VERSION_SIZE;

        if (vers != version) {
                LOG_ERR("Received an unknown version of the EFCP PDU (%d)",
                        vers);
                return -1;
        }

        memcpy(&addr, ptr + *offset, dt_cons->address_length);
        *offset += dt_cons->address_length;
        if (pci_destination_set(new_pci, addr))
                return -1;

        memcpy(&addr, ptr + *offset, dt_cons->address_length);
        *offset += dt_cons->address_length;
        if (pci_source_set(new_pci, addr))
                return -1;

        memcpy(&qos, ptr + *offset, dt_cons->qos_id_length);
        *offset += dt_cons->qos_id_length;
        if (pci_qos_id_set(new_pci, qos))
                return -1;

        memcpy(&cep, ptr + *offset, dt_cons->cep_id_length);
        *offset += dt_cons->cep_id_length;
        if (pci_cep_source_set(new_pci, cep))
                return -1;

        memcpy(&cep, ptr + *offset, dt_cons->cep_id_length);
        *offset += dt_cons->cep_id_length;
        if (pci_cep_destination_set(new_pci, cep))
                return -1;

        memcpy(&pdu_type, ptr + *offset, PDU_TYPE_SIZE);
        *offset += PDU_TYPE_SIZE;
        if (pci_type_set(new_pci, pdu_type))
                return -1;

        memcpy(&pdu_flags, ptr + *offset, FLAGS_SIZE);
        *offset += FLAGS_SIZE;
        if (pci_flags_set(new_pci, pdu_flags))
                return -1;

        memcpy(pdu_len, ptr + *offset, dt_cons->length_length);
        *offset += dt_cons->length_length;

        return 0;
}

static int deserialize_fc_pci(const struct serdes * instance,
                              struct pci *          new_pci,
                              int *                 offset,
                              const uint8_t *       ptr)
{
        seq_num_t        seq;
        struct dt_cons * dt_cons;

        ASSERT(instance);
        ASSERT(new_pci);
        ASSERT(offset);
        ASSERT(ptr);

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        seq = 0;

        memcpy(&seq, ptr + *offset, dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_new_rt_wind_edge_set(new_pci, seq))
                return -1;

        /*
         * Note that the same applies here as before
         * Rate based has to be added
         */

        memcpy(&seq, ptr + *offset, dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_my_left_wind_edge_set(new_pci, seq))
                return -1;

        memcpy(&seq, ptr + *offset, dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_my_rt_wind_edge_set(new_pci, seq))
                return -1;

        return 0;
}

static int deserialize_ack_pci(const struct serdes * instance,
                               struct pci *          new_pci,
                               int *                 offset,
                               const uint8_t *       ptr)
{
        seq_num_t        seq;
        struct dt_cons * dt_cons;

        ASSERT(instance);
        ASSERT(new_pci);
        ASSERT(offset);
        ASSERT(ptr);

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        seq = 0;

        memcpy(&seq, ptr + *offset, dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_ack_seq_num_set(new_pci, seq))
                return -1;

        return 0;
}

static int deserialize_cc_pci(const struct serdes * instance,
                              struct pci *          new_pci,
                              int *                 offset,
                              const uint8_t *       ptr)
{
        seq_num_t        seq;
        struct dt_cons * dt_cons;

        ASSERT(instance);
        ASSERT(new_pci);
        ASSERT(offset);
        ASSERT(ptr);

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        seq = 0;

        memcpy(&seq, ptr + *offset, CTRL_SEQ_NR);
        *offset += CTRL_SEQ_NR;
        if (pci_control_last_seq_num_rcvd_set(new_pci, seq))
                return -1;

        memcpy(&seq, ptr + *offset, dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_new_left_wind_edge_set(new_pci, seq))
                return -1;

        memcpy(&seq, ptr + *offset, dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_new_rt_wind_edge_set(new_pci, seq))
                return -1;

        memcpy(&seq, ptr + *offset, dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_my_left_wind_edge_set(new_pci, seq))
                return -1;

        memcpy(&seq, ptr + *offset, dt_cons->seq_num_length);
        *offset += dt_cons->seq_num_length;
        if (pci_control_my_rt_wind_edge_set(new_pci, seq))
                return -1;

        /*
         * Note that the same applies here as before
         * MyRcvRate to be added here in the future
         */

        return 0;
}

/* Note: Already passing dt_cons here for future work */
static int deserialize_ctrl_seq(const struct serdes * instance,
                                struct pci *          new_pci,
                                int *                 offset,
                                const uint8_t *       ptr)
{
        seq_num_t        seq;
        struct dt_cons * dt_cons;

        ASSERT(instance);
        ASSERT(new_pci);
        ASSERT(offset);
        ASSERT(ptr);

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        seq = 0;

        memcpy(&seq, ptr + *offset, CTRL_SEQ_NR);
        *offset += CTRL_SEQ_NR;
        if (pci_sequence_number_set(new_pci, seq))
                return -1;

        return 0;
}

static struct pdu_ser * pdu_serialize_gfp(gfp_t                       flags,
                                          const const struct serdes * instance,
                                          struct pdu *                pdu,
                                          struct dup_config_entry   * dup_conf,
                                          struct crypto_blkcipher   * blkcipher)
{
        struct pdu_ser *      tmp;
        struct dt_cons *      dt_cons;
        const void *          buffer_data;
        const struct buffer * buffer;
        const struct pci *    pci;
        size_t                size;
        ssize_t               buffer_size;
        ssize_t               blk_size;
        ssize_t               encrypted_size;
        ssize_t               pci_size;
        char *                data;
        pdu_type_t            pdu_type;
        seq_num_t             seq;
        struct buffer *       buf;
        int i;

        if (!pdu_is_ok(pdu))
                return NULL;

        if (!instance)
                return NULL;

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        buffer = pdu_buffer_get_ro(pdu);
        if (!buffer)
                return NULL;

        buffer_data = buffer_data_ro(buffer);
        if (!buffer_data)
                return NULL;

        buffer_size = buffer_length(buffer);
        if (buffer_size <= 0)
                return NULL;

        pci = pdu_pci_get_ro(pdu);
        if (!pci)
                return NULL;

        pdu_type = pci_type(pci);
        if (!pdu_type_is_ok(pdu_type)) {
                LOG_ERR("Wrong PDU type");
                return NULL;
        }

        /* Base PCI size, fields present in all PDUs */
        pci_size = base_pci_size(dt_cons);

        /*
         * These are available in the stack at this point in time
         * Extend if necessary (e.g.) NACKs, SACKs, ...
         * Set size in first switch/case
         */
        switch (pdu_type) {
        case PDU_TYPE_MGMT:
        case PDU_TYPE_DT:
                size = pci_size + dt_cons->seq_num_length + buffer_size;

                if (size <= 0)
                        return NULL;

                break;
        case PDU_TYPE_FC:
                size = pci_size + CTRL_SEQ_NR + fc_pci_size(dt_cons);
                if (size <= 0)
                        return NULL;

                break;
        case PDU_TYPE_ACK:
                size = pci_size + CTRL_SEQ_NR + dt_cons->seq_num_length;
                if (size <= 0)
                        return NULL;

                break;
        case PDU_TYPE_ACK_AND_FC:
                size = pci_size +
                        CTRL_SEQ_NR +
                        fc_pci_size(dt_cons) +
                        dt_cons->seq_num_length;
                if (size <= 0)
                        return NULL;

                break;
        case PDU_TYPE_CACK:
                size = pci_size +
                        2 * CTRL_SEQ_NR +
                        4 * dt_cons->seq_num_length +
                        RATE_LEN;
                if (size <= 0)
                        return NULL;

                break;
        default:
                LOG_ERR("Unknown PDU type %02X", pdu_type);
                return NULL;
        }

        data = rkmalloc(size, flags);
        if (!data)
                return NULL;

        /* Needed for all PDUs */
        if (serialize_base_pci(instance, data, pci, size)) {
                LOG_ERR("Failed to serialize base PCI");
                rkfree(data);
                return NULL;
        }

        /* Do actual serializing in the second switch case */
        switch (pdu_type) {
        case PDU_TYPE_MGMT:
        case PDU_TYPE_DT:
                seq = pci_sequence_number_get(pci);
                memcpy(data + pci_size, &seq, dt_cons->seq_num_length);

                memcpy(data + pci_size + dt_cons->seq_num_length,
                       buffer_data, buffer_size);

                break;
        case PDU_TYPE_FC:
                if (serialize_ctrl_seq(instance, data, pci, pci_size) ||
                    serialize_fc_pci(instance, data, pci,
                                     pci_size + CTRL_SEQ_NR)) {
                        rkfree(data);
                        return NULL;
                }

                break;
        case PDU_TYPE_ACK:
                if (serialize_ctrl_seq(instance, data, pci, pci_size) ||
                    serialize_ack_pci(instance, data, pci,
                                      pci_size + CTRL_SEQ_NR)) {
                        rkfree(data);
                        return NULL;
                }

                break;
        case PDU_TYPE_ACK_AND_FC:
                if (serialize_ctrl_seq(instance, data, pci, pci_size) ||
                    serialize_ack_pci(instance, data, pci,
                                      pci_size + CTRL_SEQ_NR) ||
                    serialize_fc_pci(instance, data, pci,
                                     pci_size +
                                     CTRL_SEQ_NR +
                                     dt_cons->seq_num_length)) {
                        rkfree(data);
                        return NULL;
                }

                break;
        case PDU_TYPE_CACK:
                if (serialize_ctrl_seq(instance, data, pci, pci_size) ||
                    serialize_cc_pci(instance, data, pci,
                                     pci_size + CTRL_SEQ_NR)) {
                        rkfree(data);
                        return NULL;
                }

                break;
        default:
                LOG_ERR("Unknown PDU type %02X", pdu_type);
                return NULL;
        }

        buf = buffer_create_with_gfp(flags, data, size);
        if (!buf) {
                rkfree(data);
                return NULL;
        }

        tmp = pdu_ser_create_buffer_with_gfp(flags, buf);
        if (!tmp) {
                rkfree(buf);
                return NULL;
        }

        /* FIXME: this should be moved to specific policy code */
        if (dup_conf != NULL && dup_conf->ttl_policy != NULL){
            if (pdu_ser_head_grow_gfp(flags, tmp, sizeof(u8))) {
                    LOG_ERR("Failed to grow ser PDU");
                    pdu_ser_destroy(tmp);
                    return NULL;
            }

            if (!dup_ttl_set(tmp, pci_ttl(pci))) {
                    LOG_ERR("Could not set TTL");
                    pdu_ser_destroy(tmp);
                    return NULL;
            }

            if (dup_ttl_is_expired(tmp)) {
                    LOG_DBG("TTL is expired, dropping PDU");
                    pdu_ser_destroy(tmp);
                    return NULL;
            }
        }

        /* FIXME: this should be moved to specific policy code */
        if (blkcipher != NULL && dup_conf != NULL
        		&& dup_conf->enable_encryption){
                buf = pdu_ser_buffer(tmp);
                blk_size = crypto_blkcipher_blocksize(blkcipher);
                buffer_size = buffer_length(buf);
                encrypted_size = (buffer_size/blk_size + 1) * blk_size;

                if (pdu_ser_tail_grow_gfp(tmp, encrypted_size - buffer_size)){
                    LOG_ERR("Failed to grow ser PDU");
                    pdu_ser_destroy(tmp);
                    return NULL;
                }

                /* PADDING */
                data = buffer_data_rw(buf);
                for (i=encrypted_size-1; i>buffer_size; i--){
                    data[i] = encrypted_size - buffer_size;
                }

                /* Encrypt */
                if (!dup_encrypt_data(tmp, blkcipher)) {
                	LOG_ERR("Failed to encrypt PDU");
                	pdu_ser_destroy(tmp);
                	return NULL;
                }
        }

        /* FIXME: this should be moved to specific policy code */
        if (dup_conf != NULL && dup_conf->error_check_policy != NULL){
            /* Assuming CRC32 */
            if (pdu_ser_head_grow_gfp(flags, tmp, sizeof(u32))) {
                    LOG_ERR("Failed to grow ser PDU");
                    pdu_ser_destroy(tmp);
                    return NULL;
            }

            if (!dup_chksum_set(tmp)) {
                    LOG_ERR("Failed to add CRC");
                    pdu_ser_destroy(tmp);
                    return NULL;
            }

            ASSERT(dup_chksum_is_ok(tmp));
        }

        return tmp;
}

struct pdu_ser * pdu_serialize(const struct serdes * instance,
                               struct pdu *          pdu)
{ return pdu_serialize_gfp(GFP_KERNEL, instance, pdu, NULL, NULL); }
EXPORT_SYMBOL(pdu_serialize);

struct pdu_ser * pdu_serialize_ni(const struct serdes * instance,
                                  struct pdu *          pdu,
                                  struct dup_config_entry * dup_conf,
                                  struct crypto_blkcipher * blkcipher)
{ return pdu_serialize_gfp(GFP_ATOMIC, instance, pdu, dup_conf, blkcipher); }
EXPORT_SYMBOL(pdu_serialize_ni);

static struct pdu * pdu_deserialize_gfp(gfp_t                 flags,
                                        const struct serdes * instance,
                                        struct pdu_ser *      pdu,
                                        struct dup_config_entry * dup_conf,
                                        struct crypto_blkcipher * blkcipher)
{
        struct pdu *          new_pdu;
        struct dt_cons *      dt_cons;
        struct buffer *       tmp_buff;
        struct buffer *       new_buff;
        struct pci *          new_pci;
        const uint8_t *       ptr;
        int                   offset;
        ssize_t               pdu_len;
        seq_num_t             seq;
        ssize_t               ttl;
        uint8_t               pad_len;
        const char *          data;

        if (!instance)
                return NULL;

        if (!pdu_ser_is_ok(pdu))
                return NULL;

        /* FIXME: this should be moved to specific policy code */
        if (dup_conf != NULL && dup_conf->error_check_policy != NULL){
            if (!dup_chksum_is_ok(pdu)) {
                    LOG_ERR("Bad CRC, PDU has been corrupted");
                    return NULL;
            }

            /* Assuming CRC32 */
            if (pdu_ser_head_shrink_gfp(flags, pdu, sizeof(u32))) {
                    LOG_ERR("Failed to shrink ser PDU");
                    return NULL;
            }
        }

        /* FIXME: this should be moved to specific policy code */
        if (blkcipher != NULL && dup_conf != NULL &&
        		dup_conf->enable_decryption){
        	if (!dup_decrypt_data(pdu, blkcipher)) {
        		LOG_ERR("Failed to decrypt PDU");
        		return NULL;
        	}

                tmp_buff = pdu_ser_buffer(pdu);
                data = buffer_data_ro(tmp_buff);
                pad_len = data[buffer_length(tmp_buff)-1];

                //remove padding
                if (pdu_ser_tail_shrink_gfp(pdu, pad_len)){
                        LOG_ERR("Failed to shrink ser PDU");
                        return NULL;
                }
        }

        dt_cons = instance->dt_cons;
        ASSERT(dt_cons);

        tmp_buff = pdu_ser_buffer(pdu);
        ASSERT(tmp_buff);

        if (buffer_length(tmp_buff) < base_pci_size(dt_cons))
                return NULL;

        new_pdu = pdu_create_gfp(flags);
        if (!new_pdu) {
                LOG_ERR("Failed to create new pdu");
                return NULL;
        }

        new_pci = pci_create_gfp(flags);
        if (!new_pci) {
                LOG_ERR("Failed to create new pci");
                pdu_destroy(new_pdu);
                return NULL;
        }

        ttl = 0;

        /* FIXME: this should be moved to specific policy code */
        if (dup_conf != NULL && dup_conf->ttl_policy != NULL){
                ttl = dup_ttl_decrement(pdu);
                if (ttl < 0) {
                        LOG_ERR("Could not decrement TTL");
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                if (pci_ttl_set(new_pci, ttl)) {
                        LOG_ERR("Could not set TTL");
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                if (pdu_ser_head_shrink_gfp(flags, pdu, sizeof(u8))) {
                        LOG_ERR("Failed to shrink ser PDU");
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }
        }

        ptr = (const uint8_t *) buffer_data_ro(tmp_buff);
        ASSERT(ptr);

        pdu_len = 0;
        if (deserialize_base_pci(instance, new_pci, &offset, ptr, &pdu_len)) {
                LOG_ERR("Could not deser base PCI");
                pci_destroy(new_pci);
                pdu_destroy(new_pdu);
                return NULL;
        }

        switch (pci_type(new_pci)) {
        case PDU_TYPE_MGMT:
        case PDU_TYPE_DT:
                /* Create buffer with rest of PDU if it is a DT or MGMT PDU*/
                memcpy(&seq,
                       ptr + offset,
                       dt_cons->seq_num_length);
                offset += dt_cons->seq_num_length;
                if (pci_sequence_number_set(new_pci, seq)) {
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                new_buff = buffer_create_from_gfp(flags,
                                                  ptr + offset,
                                                  pdu_len - offset);

                if (!new_buff) {
                        LOG_ERR("Could not create new_buff");
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                break;
        case PDU_TYPE_FC:
                if (deserialize_ctrl_seq(instance, new_pci, &offset, ptr) ||
                    deserialize_fc_pci(instance, new_pci, &offset, ptr)) {
                        pci_destroy(new_pci);
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
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                break;
        case PDU_TYPE_ACK:
                if (deserialize_ctrl_seq(instance, new_pci, &offset, ptr) ||
                    deserialize_ack_pci(instance, new_pci, &offset, ptr)) {
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                new_buff = buffer_create_gfp(flags, 1);
                if (!new_buff) {
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                break;
        case PDU_TYPE_ACK_AND_FC:
                if (deserialize_ctrl_seq(instance, new_pci, &offset, ptr) ||
                    deserialize_ack_pci(instance, new_pci, &offset, ptr) ||
                    deserialize_fc_pci(instance, new_pci, &offset, ptr)) {
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                new_buff = buffer_create_gfp(flags, 1);
                if (!new_buff) {
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                break;
        case PDU_TYPE_CACK:
                if (deserialize_ctrl_seq(instance, new_pci, &offset, ptr) ||
                    deserialize_cc_pci(instance, new_pci, &offset, ptr)) {
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }

                new_buff = buffer_create_gfp(flags, 1);
                if (!new_buff) {
                        pci_destroy(new_pci);
                        pdu_destroy(new_pdu);
                        return NULL;
                }
                break;
        default:
                LOG_ERR("Unknown PDU type %02X", pci_type(new_pci));
                pci_destroy(new_pci);
                pdu_destroy(new_pdu);
                return NULL;
        }

        if (pdu_pci_set(new_pdu, new_pci)) {
                LOG_ERR("Failed to set PCI in PDU");
                buffer_destroy(new_buff);
                pci_destroy(new_pci);
                pdu_destroy(new_pdu);
                return NULL;
        }

        if (pdu_buffer_set(new_pdu, new_buff)) {
                LOG_ERR("Failed to set buffer in PDU");
                pdu_destroy(new_pdu);
                return NULL;
        }

        ASSERT(pdu_is_ok(new_pdu));

        pdu_ser_destroy(pdu);

        return new_pdu;
}

struct pdu * pdu_deserialize(const struct serdes * instance,
                             struct pdu_ser *      pdu)
{ return pdu_deserialize_gfp(GFP_KERNEL, instance, pdu, NULL, NULL); }
EXPORT_SYMBOL(pdu_deserialize);

struct pdu * pdu_deserialize_ni(const struct serdes * instance,
                                struct pdu_ser *      pdu,
                                struct dup_config_entry * dup_conf,
                                struct crypto_blkcipher * blkcipher)
{ return pdu_deserialize_gfp(GFP_ATOMIC, instance, pdu, dup_conf, blkcipher); }
EXPORT_SYMBOL(pdu_deserialize_ni);
