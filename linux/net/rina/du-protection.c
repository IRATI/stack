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

static bool data_len_from_pdu_ser(struct pdu_ser * pdu,
                                  unsigned char *  data,
                                  ssize_t *        len)
{
        struct buffer * buf;

        buf = pdu_ser_buffer(pdu);
        if (!buffer_is_ok(buf))
                return false;

        *len = buffer_length(buf);
        LOG_DBG("Length is %zd", *len);

        data = (unsigned char *) buffer_data_rw(buf);
        if (!data) {
                LOG_ERR("Cannot get data");
                return false;
        }

        return true;
}

static bool crc32_pdu_ser(struct pdu_ser * pdu,
                          u32 *            crc)
{
        ssize_t         len;
        unsigned char * data;

        ASSERT(pdu_ser_is_ok(pdu));

        data = 0;

        if (!data_len_from_pdu_ser(pdu, data, &len))
                return false;

        *crc = crc32_le(0, data + sizeof(*crc), len - sizeof(*crc));
        LOG_DBG("Calculated a crc of %d", *crc);

        return true;
}

bool dup_chksum_set(struct pdu_ser * pdu)
{
        u32             crc;
        unsigned char * data;
        ssize_t         len;

        if (!pdu_ser_is_ok(pdu))
                return false;

        if (!crc32_pdu_ser(pdu, &crc))
                return false;

        if (!data_len_from_pdu_ser(pdu, data, &len))
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

        if (!crc32_pdu_ser(pdu, &crc))
                return false;

        if (!data_len_from_pdu_ser(pdu, data, &len))
                return false;

        if (memcmp(&crc, data, sizeof(crc)))
                return false;

        return true;
}
EXPORT_SYMBOL(dup_chksum_is_ok);

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

size_t dup_ttl_decrement(struct pdu_ser * pdu)
{
        size_t val = 0;

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
