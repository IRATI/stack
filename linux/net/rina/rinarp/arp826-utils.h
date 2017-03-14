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

#ifndef ARP826_UTILS_H
#define ARP826_UTILS_H

#include <linux/types.h>

/*
 * Generic Protocol Address - GPA
 */

struct gpa;

struct gpa *    gpa_create(const uint8_t * address,
                           size_t          length);
struct gpa *    gpa_create_ni(const uint8_t * address,
                              size_t          length);
struct gpa *    gpa_create_gfp(gfp_t           flags,
                               const uint8_t * address,
                               size_t          length);
void            gpa_destroy(struct gpa * gpa);
bool            gpa_is_ok(const struct gpa * gpa);
struct gpa *    gpa_dup(const struct gpa * gpa);
struct gpa *    gpa_dup_ni(const struct gpa * gpa);
struct gpa *    gpa_dup_gfp(gfp_t              flags,
                            const struct gpa * gpa);
bool            gpa_is_equal(const struct gpa * a,
                             const struct gpa * b);
const uint8_t * gpa_address_value(const struct gpa * gpa);
string_t * 	gpa_address_to_string_gfp(gfp_t              flags,
				          const struct gpa * gpa);
size_t          gpa_address_length(const struct gpa * gpa);

/* Grows a GPA adding the filler symbols up to length (if needed) */
int             gpa_address_grow(struct gpa * gpa,
                                 size_t       length,
                                 uint8_t      filler);
int             gpa_address_grow_ni(struct gpa * gpa,
                                    size_t       length,
                                    uint8_t      filler);
int             gpa_address_grow_gfp(gfp_t        flags,
                                     struct gpa * gpa,
                                     size_t       length,
                                     uint8_t      filler);

/* Shrinks a GPA removing all the filler symbols (if any) */
int             gpa_address_shrink(struct gpa * gpa,
                                   uint8_t      filler);
int             gpa_address_shrink_ni(struct gpa * gpa,
                                      uint8_t      filler);
int             gpa_address_shrink_gfp(gfp_t        flags,
                                       struct gpa * gpa,
                                       uint8_t      filler);

void            gpa_dump(const struct gpa * gpa);

typedef enum {
        MAC_ADDR_802_3
} gha_type_t;

/*
 * Generic Hardware Address - GHA
 */

struct gha;

struct gha *        gha_create(gha_type_t      type,
                               const uint8_t * address);
struct gha *        gha_create_ni(gha_type_t      type,
                                  const uint8_t * address);
struct gha *        gha_create_gfp(gfp_t           flags,
                                   gha_type_t      type,
                                   const uint8_t * address);
struct gha *        gha_create_broadcast(gha_type_t type);
struct gha *        gha_create_broadcast_ni(gha_type_t type);
struct gha *        gha_create_broadcast_gfp(gfp_t      flags,
                                             gha_type_t type);
struct gha *        gha_create_unknown(gha_type_t type);
struct gha *        gha_create_unknown_ni(gha_type_t type);
struct gha *        gha_create_unknown_gfp(gfp_t      flags,
                                           gha_type_t type);
int                 gha_destroy(struct gha * gha);
bool                gha_is_ok(const struct gha * gha);
struct gha *        gha_dup(const struct gha * gha);
struct gha *        gha_dup_ni(const struct gha * gha);
struct gha *        gha_dup_gfp(gfp_t              flags,
                                const struct gha * gha);
const uint8_t *     gha_address(const struct gha * gha);
size_t              gha_address_length(const struct gha * gha);
gha_type_t          gha_type(const struct gha * gha);
bool                gha_is_equal(const struct gha * a,
                                 const struct gha * b);
void                gha_dump(const struct gha * gha);


/*
 * Miscellaneous
 */

#include <linux/netdevice.h>

struct arp_header {
        __be16     htype; /* Hardware type */
        __be16     ptype; /* Protocol type */
        __u8       hlen;  /* Hardware address length */
        __u8       plen;  /* Protocol address length */
        __be16     oper;  /* Operation */

#if 0
        __u8[hlen] sha; /* Sender hardware address */
        __u8[plen] spa; /* Sender protocol address */
        __u8[hlen] tha; /* Target hardware address */
        __u8[plen] tpa; /* Target protocol address */
#endif
};

enum arp826_optypes {
        ARP_REQUEST = 1,
        ARP_REPLY   = 2,
};

enum arp826_htypes {
        HW_TYPE_ETHER = 1,
        HW_TYPE_MAX,
};

#endif
