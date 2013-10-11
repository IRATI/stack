/*
 * ARP 826 (wonnabe) core
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

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-utils"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

/*
 * NOTE: In 'static' functions there should be no input parameters checks while
 *       in non-static ones there should be plenty
 */

/*
 * General Protocol Address - GPA
 */

struct gpa {
        uint8_t * address;
        size_t    length;
};

/*
 * NOTE: Even if GPA is a base type we avoid ASSERTions here, preferring a
 *       merciful approach. Look at the logs guys ...
 */

struct gpa * gpa_create(const uint8_t * address,
                        size_t          length)
{
        struct gpa * tmp;

        if (!address || length == 0) {
                LOG_ERR("Bad input parameters, cannot create GPA");
                return NULL;
        }

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->length  = length;
        tmp->address = rkmalloc(tmp->length, GFP_KERNEL);
        if (!tmp->address) {
                rkfree(tmp);
                return NULL;
        }
        memcpy(tmp->address, address, tmp->length);

        return tmp;
}

bool gpa_is_ok(const struct gpa * gpa)
{ return (!gpa || gpa->address == NULL || gpa->length == 0) ? 0 : 1; }

void gpa_destroy(struct gpa * gpa)
{
        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bogus GPA, cannot destroy");
                return;
        }

        rkfree(gpa->address);
        rkfree(gpa);
}

struct gpa * gpa_dup(const struct gpa * gpa)
{
        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bogus input parameter, cannot duplicate GPA");
                return NULL;
        }

        return gpa_create(gpa->address, gpa->length);
}

const char * gpa_address_value(const struct gpa * gpa)
{
        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bad input parameter, "
                        "cannot get a meaningful address from GPA");
                return NULL;
        }

        return gpa->address;
}

size_t gpa_address_length(const struct gpa * gpa)
{
        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bad input parameter, "
                        "cannot get a meaningful size from GPA");
                return 0;
        }

        return gpa->length;
}

int gpa_address_shrink(struct gpa * gpa, size_t length, uint8_t filler)
{
        uint8_t * new_address;
        uint8_t * position;
        uint8_t * tmp;
        size_t    count;

        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bad input parameter, cannot shrink the GPA");
                return -1;
        }

        if (length == 0 || length > gpa->length) {
                LOG_ERR("Can't shrink the GPA, bad length");
                return -1;
        }

        /* No needs to shrink */
        if (gpa->length == length)
                return 1;

        ASSERT(length < gpa->length);

        position = strnchr(gpa->address, filler, gpa->length);
        if (!position) {
                LOG_ERR("No filler in the GPA, cannot shrink");
                return -1;
        }

        count = position - gpa->address;
        ASSERT(count);

        new_address = rkmalloc(length, GFP_KERNEL);
        if (!new_address)
                return -1;

        memcpy(new_address, gpa->address, gpa->length - count);
        for (tmp = new_address + count; count != 0; tmp++, count--)
                *tmp = filler;

        rkfree(gpa->address);
        gpa->address = new_address;
        gpa->length  = length;

        return 0;
}

int gpa_address_grow(struct gpa * gpa, size_t length, uint8_t filler)
{
        uint8_t * new_address;

        if (!gpa_is_ok(gpa)) {
                LOG_ERR("Bad input parameter, cannot grow the GPA");
                return -1;
        }

        if (length == 0 || length < gpa->length) {
                LOG_ERR("Can't grow the GPA, bad length");
                return -1;
        }

        /* No needs to grow */
        if (gpa->length == length)
                return 1;

        ASSERT(length > gpa->length);

        new_address = rkmalloc(length, GFP_KERNEL);
        if (!new_address)
                return -1;

        memcpy(new_address, gpa->address, gpa->length);
        memset(new_address + gpa->length, filler, length - gpa->length);
        rkfree(gpa->address);
        gpa->address = new_address;
        gpa->length  = length;

        return 1;
}

bool gpa_is_equal(const struct gpa * a, const struct gpa * b)
{
        if (!gpa_is_ok(a) || !gpa_is_ok(b)) {
                LOG_ERR("Bad input parameters, cannot compare GPAs");
                return 0;
        }

        ASSERT(a && a->length != 0 && a->address != NULL);
        ASSERT(b && b->length != 0 && b->address != NULL);

        if (a->length != b->length)
                return 0;

        ASSERT(a->length == b->length);

        if (memcmp(a->address, b->address, a->length))
                return 0;

        return 1;
}
