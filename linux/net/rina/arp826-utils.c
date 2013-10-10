/*
 * An RFC 826 ARP implementation
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
