/*
 * Data Unit Protection
 *
 *    Dimitri Staessens     <dimitri.staessens@intec.ugent.be>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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
#include <linux/types.h>
#include <linux/crc32.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/scatterlist.h>

#define RINA_PREFIX "du-protection"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du-protection.h"

static bool pdu_ser_data_and_length(struct pdu_ser * pdu,
                                    unsigned char ** data,
                                    ssize_t *        len)
{
        struct buffer * buf;

        buf = pdu_ser_buffer(pdu);
        if (!buffer_is_ok(buf))
                return false;

        ASSERT(data);
        ASSERT(len);

        *data = (unsigned char *) buffer_data_rw(buf);
        if (!*data) {
                LOG_ERR("Cannot get data from serialised PDU");
                return false;
        }

        *len = buffer_length(buf);

        return true;
}


static bool pdu_ser_crc32(struct pdu_ser * pdu,
                          u32 *            crc)
{
        ssize_t         len;
        unsigned char * data;

        ASSERT(pdu_ser_is_ok(pdu));
        ASSERT(crc);

        data = 0;
        len  = 0;

        if (!pdu_ser_data_and_length(pdu, &data, &len))
                return false;

        *crc = crc32_le(0, data + sizeof(*crc), len - sizeof(*crc));
        LOG_DBG("CRC32(%p, %zd) = %X", data, len, *crc);

        return true;
}

bool dup_chksum_set(struct pdu_ser * pdu)
{
        u32             crc;
        unsigned char * data;
        ssize_t         len;

        crc  = 0;
        data = 0;
        len  = 0;

        if (!pdu_ser_is_ok(pdu))
                return false;

        if (!pdu_ser_crc32(pdu, &crc))
                return false;

        if (!pdu_ser_data_and_length(pdu, &data, &len))
                return false;

        memcpy(data, &crc, sizeof(crc));

        return true;
}
EXPORT_SYMBOL(dup_chksum_set);

bool dup_chksum_is_ok(struct pdu_ser * pdu)
{
        u32             crc;
        unsigned char * data;
        ssize_t         len;

        if (!pdu_ser_is_ok(pdu))
                return false;

        data = 0;
        crc  = 0;
        len  = 0;

        if (!pdu_ser_crc32(pdu, &crc))
                return false;

        if (!pdu_ser_data_and_length(pdu, &data, &len))
                return false;

        if (memcmp(&crc, data, sizeof(crc)))
                return false;

        LOG_DBG("CRC is ok");

        return true;
}
EXPORT_SYMBOL(dup_chksum_is_ok);

bool dup_ttl_set(struct pdu_ser * pdu,
                 size_t           value)
{
        unsigned char * data;
        size_t          len;
        u8              ttl;

        if (!pdu_ser_is_ok(pdu))
                return false;

        /*BUILD_BUG_ON(CONFIG_RINA_IPCPS_TTL_DEFAULT <= 0);*/

        if (!pdu_ser_is_ok(pdu))
                return false;

        if (!pdu_ser_data_and_length(pdu, &data, &len))
                return false;

        ttl = (u8) value;

        LOG_DBG("Passed value was %zu", value);
        LOG_DBG("TTL to be put in PCI is %d", ttl);

        memcpy(data, &ttl, sizeof(ttl));

        return true;
}
EXPORT_SYMBOL(dup_ttl_set);

ssize_t dup_ttl_decrement(struct pdu_ser * pdu)
{
        ssize_t         val = -1;
        unsigned char * data;
        ssize_t         len;
        u8              ttl;

        if (!pdu_ser_is_ok(pdu))
                return val;

        if (!pdu_ser_data_and_length(pdu, &data, &len))
                return val;

        ttl = 0;
        memcpy(&ttl, data, sizeof(ttl));
        val = --ttl;

        LOG_DBG("TTL value from PCI is %d", ttl);
        LOG_DBG("Returned TTL is %zu", val);

        return val;
}
EXPORT_SYMBOL(dup_ttl_decrement);

bool dup_ttl_is_expired(struct pdu_ser * pdu)
{
        u8              ttl;
        unsigned char * data;
        ssize_t         len;

        ttl = 0;

        if (!pdu_ser_is_ok(pdu))
                return false;

        if (!pdu_ser_data_and_length(pdu, &data, &len))
                return false;

        memcpy(&ttl, data, sizeof(ttl));
        LOG_DBG("TTL is now %d", ttl);

        if (ttl == 0)
                return true;

        return false;
}
EXPORT_SYMBOL(dup_ttl_is_expired);

bool dup_encrypt_data(struct pdu_ser * 	        pdu,
                      struct crypto_blkcipher * blkcipher)
{
    struct blkcipher_desc desc;
    struct scatterlist    sg_src;
    struct scatterlist    sg_dst;
    struct buffer *       buf;
    ssize_t 		  buffer_size;
    const void *	  data;

    if (blkcipher == NULL){
        LOG_ERR("invalid blk cipher structure!");
        return false;
    }

    if (!pdu_ser_is_ok(pdu))
	    return false;

    desc.flags = 0;
    desc.tfm = blkcipher;
    buf = pdu_ser_buffer(pdu);
    buffer_size = buffer_length(buf);
    data = buffer_data_ro(buf);

    sg_init_one(&sg_src, data, buffer_size);
    sg_init_one(&sg_dst, data, buffer_size);

    if (crypto_blkcipher_encrypt(&desc, &sg_dst, &sg_src, buffer_size)) {
	    return false;
    }

    return true;
}
EXPORT_SYMBOL(dup_encrypt_data);

bool dup_decrypt_data(struct pdu_ser * 	        pdu,
                      struct crypto_blkcipher * blkcipher)
{
    struct blkcipher_desc desc;
    struct scatterlist    sg_src;
    struct scatterlist    sg_dst;
    struct buffer *       buf;
    ssize_t 		  buffer_size;
    const void *	  data;

    if (blkcipher == NULL){
        LOG_ERR("invalid blk cipher structure!");
        return false;
    }

    if (!pdu_ser_is_ok(pdu))
	    return false;

    desc.flags = 0;
    desc.tfm = blkcipher;
    buf = pdu_ser_buffer(pdu);
    buffer_size = buffer_length(buf);
    data = buffer_data_ro(buf);

    sg_init_one(&sg_src, data, buffer_size);
    sg_init_one(&sg_dst, data, buffer_size);

    if (crypto_blkcipher_decrypt(&desc, &sg_dst, &sg_src, buffer_size)) {
	    return false;
    }

    return true;
}
EXPORT_SYMBOL(dup_decrypt_data);
