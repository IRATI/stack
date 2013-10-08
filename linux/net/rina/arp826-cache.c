/*
 * ARP 826 cache
 *
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

/*
 * FIXME: The following lines provide basic framework and utilities. These
 *        dependencies will be removed ASAP to let this module live its own
 *        life
 */
#define RINA_PREFIX "arp826-cache"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

static struct list_head cache;

int arp826_cache_init(void)
{
        INIT_LIST_HEAD(&cache);

        return 0;
}

void arp826_cache_fini(void)
{
        LOG_MISSING;
}

int arp826_cache_add(size_t                protocol_address_length,
                     const unsigned char * source_protocol_address,
                     const unsigned char * target_protocol_address,
                     size_t                hardware_address_length,
                     const unsigned char * source_hardware_address,
                     const unsigned char * target_hardware_address)
{
        unsigned char * spa;
        unsigned char * tpa;
        unsigned char * sha;
        unsigned char * tha;

        LOG_MISSING;

        spa = rkmalloc(protocol_address_length, GFP_KERNEL);
        tpa = rkmalloc(protocol_address_length, GFP_KERNEL);
        sha = rkmalloc(hardware_address_length, GFP_KERNEL);
        tha = rkmalloc(hardware_address_length, GFP_KERNEL);

        if (!spa || !tpa || !sha || !tha) {
                if (spa) rkfree(spa);
                if (tpa) rkfree(tpa);
                if (sha) rkfree(sha);
                if (tha) rkfree(tha);
        }

        memcpy(spa, source_protocol_address, protocol_address_length);
        memcpy(tpa, target_protocol_address, protocol_address_length);
        memcpy(sha, source_hardware_address, hardware_address_length);
        memcpy(tha, target_hardware_address, hardware_address_length);

        LOG_MISSING;

        return -1;
}
