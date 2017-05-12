/*
 * ARP 826 (wonnabe) related utilities
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

#include <linux/types.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/if_ether.h>

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-utils"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826-utils.h"

/*
 * NOTE: In 'static' functions there should be no input parameters checks while
 *       in non-static ones there should be plenty
 */

struct gpa {
        /*
         * FIXME: To be revised in order to use struct buffer (and related
         *        methods
         */
        uint8_t * address;
        size_t    length;
};

/*
 * NOTE: Even if GPA is a base type we avoid ASSERTions here, preferring a
 *       merciful approach. Look at the logs guys ...
 */

struct gpa * gpa_create_gfp(gfp_t           flags,
                            const uint8_t * address,
                            size_t          length)
{
        struct gpa * tmp;

        if (!address || length == 0) {
                LOG_ERR("Bad input parameters, cannot create GPA");
                return NULL;
        }

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->length  = length;
        tmp->address = rkmalloc(tmp->length, flags);
        if (!tmp->address) {
                rkfree(tmp);
                return NULL;
        }
        memcpy(tmp->address, address, tmp->length);

        return tmp;
}
EXPORT_SYMBOL(gpa_create_gfp);

struct gpa * gpa_create(const uint8_t * address,
                        size_t          length)
{ return gpa_create_gfp(GFP_KERNEL, address, length); }
EXPORT_SYMBOL(gpa_create);

struct gpa * gpa_create_ni(const uint8_t * address,
                           size_t          length)
{ return gpa_create_gfp(GFP_ATOMIC, address, length); }
EXPORT_SYMBOL(gpa_create_ni);

bool gpa_is_ok(const struct gpa * gpa)
{ return (!gpa || gpa->address == NULL || gpa->length == 0) ? false : true; }
EXPORT_SYMBOL(gpa_is_ok);

void gpa_destroy(struct gpa * gpa)
{
        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bogus GPA, cannot destroy");
                return;
        }

        rkfree(gpa->address);
        rkfree(gpa);
}
EXPORT_SYMBOL(gpa_destroy);

struct gpa * gpa_dup_gfp(gfp_t              flags,
                         const struct gpa * gpa)
{
        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bogus input parameter, cannot duplicate GPA");
                return NULL;
        }

        return gpa_create_gfp(flags, gpa->address, gpa->length);
}
EXPORT_SYMBOL(gpa_dup_gfp);

struct gpa * gpa_dup(const struct gpa * gpa)
{ return gpa_dup_gfp(GFP_KERNEL, gpa); }
EXPORT_SYMBOL(gpa_dup);

struct gpa * gpa_dup_ni(const struct gpa * gpa)
{ return gpa_dup_gfp(GFP_ATOMIC, gpa); }
EXPORT_SYMBOL(gpa_dup_ni);

const uint8_t * gpa_address_value(const struct gpa * gpa)
{
        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bad input parameter, "
                        "cannot get a meaningful address from GPA");
                return NULL;
        }

        return gpa->address;
}
EXPORT_SYMBOL(gpa_address_value);

string_t * gpa_address_to_string_gfp(gfp_t              flags,
				     const struct gpa * gpa)
{
	string_t * tmp;
	string_t * p;

        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bad input parameter, "
                        "cannot get a meaningful address from GPA");
                return NULL;
        }

        tmp = rkmalloc(gpa->length + 1, flags);
        if (!tmp)
        	return NULL;

        memcpy(tmp, gpa->address, gpa->length);
        p = tmp + gpa->length;
        *(p) = '\0';

        return tmp;
}
EXPORT_SYMBOL(gpa_address_to_string_gfp);

size_t gpa_address_length(const struct gpa * gpa)
{
        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bad input parameter, "
                        "cannot get a meaningful size from GPA");
                return 0;
        }

        return gpa->length;
}
EXPORT_SYMBOL(gpa_address_length);

/* FIXME: Crappy ... we should avoid rkmalloc and rkfree as much as possible */
void gpa_dump(const struct gpa * gpa)
{
        string_t * tmp;

        if (!gpa) {
                LOG_DBG("GPA %pK: <null>", gpa);
                return;
        }
        if (gpa->length == 0) {
                LOG_DBG("GPA %pK: <empty>", gpa);
                return;
        }

        tmp = gpa_address_to_string_gfp(GFP_ATOMIC, gpa);
        if (!tmp)
        	return;

        LOG_DBG("GPA %pK (%zd): 0x%s", gpa, gpa->length, tmp);

        rkfree(tmp);
}
EXPORT_SYMBOL(gpa_dump);

int gpa_address_shrink_gfp(gfp_t        flags,
                           struct gpa * gpa,
                           uint8_t      filler)
{
        uint8_t * new_address;
        uint8_t * position;
        size_t    length;

        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bad input parameter, cannot shrink the GPA");
                return -1;
        }

        gpa_dump(gpa);

        LOG_DBG("Looking for filler 0x%02X in GPA (length = %zd)",
                filler, gpa->length);

        position = memscan(gpa->address, filler, gpa->length);
        if (position >= gpa->address + gpa->length) {
                LOG_DBG("GPA doesn't need to be shrinked ...");
                return 0;
        }

        length = position - gpa->address;

        LOG_DBG("Shrinking GPA to %zd", length);

        new_address = rkmalloc(length, flags);
        if (!new_address)
                return -1;

        memcpy(new_address, gpa->address, length);

        rkfree(gpa->address);
        gpa->address = new_address;
        gpa->length  = length;

        gpa_dump(gpa);

        return 0;
}
EXPORT_SYMBOL(gpa_address_shrink_gfp);

int gpa_address_shrink(struct gpa * gpa, uint8_t filler)
{ return gpa_address_shrink_gfp(GFP_KERNEL, gpa, filler); }
EXPORT_SYMBOL(gpa_address_shrink);

int gpa_address_shrink_ni(struct gpa * gpa, uint8_t filler)
{ return gpa_address_shrink_gfp(GFP_ATOMIC, gpa, filler); }
EXPORT_SYMBOL(gpa_address_shrink_ni);

int gpa_address_grow_gfp(gfp_t        flags,
                         struct gpa * gpa,
                         size_t       length,
                         uint8_t      filler)
{
        uint8_t * new_address;

        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bad input parameter, cannot grow the GPA");
                return -1;
        }

        gpa_dump(gpa);

        if (length == 0 || length < gpa->length) {
                LOG_ERR("Can't grow the GPA, bad length");
                return -1;
        }

        if (gpa->length == length) {
                LOG_DBG("No needs to grow the GPA");
                return 0;
        }

        ASSERT(length > gpa->length);

        LOG_DBG("Growing GPA to %zd with filler 0x%02X", length, filler);
        new_address = rkmalloc(length, flags);
        if (!new_address)
                return -1;

        memcpy(new_address, gpa->address, gpa->length);
        memset(new_address + gpa->length, filler, length - gpa->length);
        rkfree(gpa->address);
        gpa->address = new_address;
        gpa->length  = length;

        LOG_DBG("GPA is now %zd characters long", gpa->length);

        gpa_dump(gpa);

        return 0;
}
EXPORT_SYMBOL(gpa_address_grow_gfp);

int gpa_address_grow(struct gpa * gpa, size_t length, uint8_t filler)
{ return gpa_address_grow_gfp(GFP_KERNEL, gpa, length, filler); }
EXPORT_SYMBOL(gpa_address_grow);

int gpa_address_grow_ni(struct gpa * gpa, size_t length, uint8_t filler)
{ return gpa_address_grow_gfp(GFP_ATOMIC, gpa, length, filler); }
EXPORT_SYMBOL(gpa_address_grow_ni);

bool gpa_is_equal(const struct gpa * a, const struct gpa * b)
{
        if (!gpa_is_ok(a)) {
                LOG_ERR("Bad input parameter (LHS), cannot compare GPAs");
                return false;
        }
        if (!gpa_is_ok(b)) {
                LOG_ERR("Bad input parameter (RHS), cannot compare GPAs");
                return false;
        }

        ASSERT(a && a->length != 0 && a->address != NULL);
        ASSERT(b && b->length != 0 && b->address != NULL);

        if (a->length != b->length)
                return false;

        ASSERT(a->length == b->length);

        if (memcmp(a->address, b->address, a->length))
                return false;

        return true;
}
EXPORT_SYMBOL(gpa_is_equal);

struct gha {
        gha_type_t type;
        union {
                uint8_t mac_802_3[6];
        } data;
};

void gha_dump(const struct gha * gha)
{
        if (!gha) {
                LOG_DBG("GHA %pK: <null>", gha);
                return;
        }

        if (gha->type == MAC_ADDR_802_3) {
                LOG_DBG("GHA %pK: %02X:%02X:%02X:%02X:%02X:%02X",
                        gha,
                        gha->data.mac_802_3[5],
                        gha->data.mac_802_3[4],
                        gha->data.mac_802_3[3],
                        gha->data.mac_802_3[2],
                        gha->data.mac_802_3[1],
                        gha->data.mac_802_3[0]);
        } else {
                LOG_DBG("GHA %pK: <unknown format>", gha);
        }
}
EXPORT_SYMBOL(gha_dump);

bool gha_is_ok(const struct gha * gha)
{
        if (!gha)
                return false;
        if (gha->type != MAC_ADDR_802_3)
                return false;
        return true;

        /* return (!gha || gha->type != MAC_ADDR_802_3) ? false : true; */
}
EXPORT_SYMBOL(gha_is_ok);

struct gha * gha_create_gfp(gfp_t           flags,
                            gha_type_t      type,
                            const uint8_t * address)
{
        struct gha * gha;

        if (type != MAC_ADDR_802_3 || !address) {
                LOG_ERR("Wrong input parameters, cannot create GHA");
                return NULL;
        }

        gha = rkzalloc(sizeof(*gha), flags);
        if (!gha)
                return NULL;

        gha->type = type;
        switch (type) {
        case MAC_ADDR_802_3:
                memcpy(gha->data.mac_802_3,
                       address,
                       sizeof(gha->data.mac_802_3));
                break;
        default:
                BUG();
                break; /* Only to stop the compiler from barfing */
        }

        return gha;
}
EXPORT_SYMBOL(gha_create_gfp);

struct gha * gha_create(gha_type_t      type,
                        const uint8_t * address)
{ return gha_create_gfp(GFP_KERNEL, type, address); }
EXPORT_SYMBOL(gha_create);

struct gha * gha_create_ni(gha_type_t      type,
                           const uint8_t * address)
{ return gha_create_gfp(GFP_ATOMIC, type, address); }
EXPORT_SYMBOL(gha_create_ni);

struct gha * gha_create_broadcast_gfp(gfp_t      flags,
                                      gha_type_t type)
{
        const uint8_t addr[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

        if (type == MAC_ADDR_802_3)
                return gha_create_gfp(flags, MAC_ADDR_802_3, addr);

        return NULL;
}
EXPORT_SYMBOL(gha_create_broadcast_gfp);

struct gha * gha_create_broadcast(gha_type_t type)
{ return gha_create_broadcast_gfp(GFP_KERNEL, type); }
EXPORT_SYMBOL(gha_create_broadcast);

struct gha * gha_create_broadcast_ni(gha_type_t type)
{ return gha_create_broadcast_gfp(GFP_ATOMIC, type); }
EXPORT_SYMBOL(gha_create_broadcast_ni);

struct gha * gha_create_unknown_gfp(gfp_t      flags,
                                    gha_type_t type)
{
        const uint8_t addr[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        if (type == MAC_ADDR_802_3)
                return gha_create_gfp(flags, MAC_ADDR_802_3, addr);

        return NULL;
}
EXPORT_SYMBOL(gha_create_unknown_gfp);

struct gha * gha_create_unknown(gha_type_t type)
{ return gha_create_broadcast_gfp(GFP_KERNEL, type); }
EXPORT_SYMBOL(gha_create_unknown);

struct gha * gha_create_unknown_ni(gha_type_t type)
{ return gha_create_broadcast_gfp(GFP_ATOMIC, type); }
EXPORT_SYMBOL(gha_create_unknown_ni);

int gha_destroy(struct gha * gha)
{
        if (!gha_is_ok(gha)) {
                LOG_ERR("Bogus GHA, cannot destroy");
                return -1;
        }

        rkfree(gha);

        return 0;
}
EXPORT_SYMBOL(gha_destroy);

struct gha * gha_dup_gfp(gfp_t              flags,
                         const struct gha * gha)
{
        struct gha * tmp;

        if (!gha_is_ok(gha)) {
                LOG_ERR("Bogus GHA, cannot duplicate");
                return NULL;
        }

        tmp = rkmalloc(sizeof(*gha), flags);
        if (!tmp)
                return NULL;

        *tmp = *gha;

        return tmp;
}
EXPORT_SYMBOL(gha_dup_gfp);

struct gha * gha_dup(const struct gha * gha)
{ return gha_dup_gfp(GFP_KERNEL, gha); }
EXPORT_SYMBOL(gha_dup);

struct gha * gha_dup_ni(const struct gha * gha)
{ return gha_dup_gfp(GFP_ATOMIC, gha); }
EXPORT_SYMBOL(gha_dup_ni);

size_t gha_address_length(const struct gha * gha)
{
        size_t tmp;

        if (!gha_is_ok(gha))
                return 0;

        switch (gha->type) {
        case MAC_ADDR_802_3: tmp = sizeof(gha->data.mac_802_3); break;
        default:             BUG();                             break;
        }

        return tmp;

}
EXPORT_SYMBOL(gha_address_length);

const uint8_t * gha_address(const struct gha * gha)
{
        const uint8_t * tmp;

        if (!gha_is_ok(gha)) {
                LOG_ERR("Bogus GHA passed, cannot get address");
                return NULL;
        }

        switch (gha->type) {
        case MAC_ADDR_802_3: tmp = gha->data.mac_802_3; break;
        default:             BUG();                     break; /* shut up */
        }

        return tmp;
}
EXPORT_SYMBOL(gha_address);

gha_type_t gha_type(const struct gha * gha)
{
        ASSERT(gha_is_ok(gha));

        return gha->type;
}
EXPORT_SYMBOL(gha_type);

bool gha_is_equal(const struct gha * a,
                  const struct gha * b)
{
        bool v;

        v = false;
        if (!gha_is_ok(a) || !gha_is_ok(b))
                return v;

        if (a->type != b->type)
                return v;

        ASSERT(a->type == b->type);

        switch (a->type) {
        case MAC_ADDR_802_3:
                v = (memcmp(a->data.mac_802_3,
                            b->data.mac_802_3,
                            sizeof(a->data.mac_802_3)) == 0) ? true : false;
                break;
        default:
                BUG(); /* As usual, shut up compiler! */
                break;
        }

        return v;
}
EXPORT_SYMBOL(gha_is_equal);

#if 0
struct net_device * gha_to_device(const struct gha * ha)
{
        struct net_device *     dev;
        struct netdev_hw_addr * hwa;

        if (!gha_is_ok(ha)) {
                LOG_ERR("Wrong input, cannot get device from GHA");
                return NULL;
        }

        LOG_DBG("Looking for a device with this ha");
        gha_dump(ha);
        LOG_DBG("Showing all device+addresses");

        read_lock(&dev_base_lock);

        dev = first_net_device(&init_net);
        while (dev) {
                LOG_DBG("Next device");
                LOG_DBG("addr_len: %d", dev->addr_len);
                if (dev->addr_len == gha_address_length(ha)) {
                        for_each_dev_addr(dev, hwa) {
                                if (dev->addr_len == 6) {
                                        LOG_DBG("HA: "
                                                "%02X:%02X:"
                                                "%02X:%02X:"
                                                "%02X:%02X",
                                                hwa->addr[5],
                                                hwa->addr[4],
                                                hwa->addr[3],
                                                hwa->addr[2],
                                                hwa->addr[1],
                                                hwa->addr[0]);
                                }
                                if (!memcmp(hwa->addr,
                                            gha_address(ha),
                                            gha_address_length(ha))) {
                                        read_unlock(&dev_base_lock);
                                        return dev;
                                }
                        }
                }
                dev = next_net_device(dev);
        }

        read_unlock(&dev_base_lock);

        return NULL;
}
EXPORT_SYMBOL(gha_to_device);
#endif
