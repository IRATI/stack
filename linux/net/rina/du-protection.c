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

#define RINA_PREFIX "du-protection"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du-protection.h"

#ifdef CONFIG_RINA_DUP

#ifdef CONFIG_RINA_IPCPS_CRC

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

#endif

#ifdef CONFIG_RINA_IPCPS_TTL

bool dup_ttl_set(struct pdu_ser * pdu,
                 size_t           value)
{
        if (!pdu_ser_is_ok(pdu))
                return false;

        BUILD_BUG_ON(CONFIG_RINA_IPCPS_TTL_DEFAULT <= 0);

        LOG_MISSING;

        return true;
}
EXPORT_SYMBOL(dup_ttl_set);

ssize_t dup_ttl_decrement(struct pdu_ser * pdu)
{
        ssize_t val = -1;

        if (!pdu_ser_is_ok(pdu))
                return val;

        LOG_MISSING;

        return val;
}
EXPORT_SYMBOL(dup_ttl_decrement);

bool dup_ttl_is_expired(struct pdu_ser * pdu)
{
        if (!pdu_ser_is_ok(pdu))
                return false;

        LOG_MISSING;

        return true;
}
EXPORT_SYMBOL(dup_ttl_is_expired);

#endif

#endif
