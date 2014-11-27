/*
 * SDU Protection
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

#ifndef RINA_SDU_PROTECTION_H
#define RINA_SDU_PROTECTION_H

struct sdup_cksum;

struct sdup_cksum * sdup_cksum_create(const char * name,
                                      (size_t)(* cksum_sizeof)(void),
                                      (void)  (* cksum_calc)(void * buffer,
                                                             size_t size),
                                      (void)  (* cksum_update)(void * buffer,
                                                               size_t size));
void                sdup_cksum_destroy(sdup_cksum * d);

bool                sdup_cksum_is_present(struct sdup_cksum * d,
                                          struct sdu *        s);
bool                sdup_cksum_add(struct sdup_cksum * d,    struct sdu * s);
bool                sdup_cksum_remove(struct sdup_cksum * d, struct sdu * s);
bool                sdup_cksum_is_ok(struct sdup_cksum * d,  struct sdu * s);
void                sdup_cksum_update(struct sdup_cksum * d, struct sdu * s);

struct sdup_ttl;

#if 0
struct sdup_cksum * sdup_cksum_create(const char * name,
                                      (size_t)(* cksum_sizeof)(void),
                                      (void)  (* cksum_calc)(void * buffer,
                                                             size_t size),
                                      (void)  (* cksum_update)(void * buffer,
                                                               size_t size));
void              sdup_ttl_destroy(sdup_ttl * d);

bool              sdup_ttl_is_present(struct sdup_ttl * d,
                                      struct sdu *      s);
bool              sdup_ttl_add(struct sdup_ttl * d,    struct sdu * s);
bool              sdup_ttl_remove(struct sdup_ttl * d, struct sdu * s);
bool              sdup_ttl_inc(struct sdup_ttl * d,  struct sdu * s);
bool              sdup_ttl_dec(struct sdup_ttl * d,  struct sdu * s);
void              sdup_ttl_value(struct sdup_ttl * d, struct sdu * s);
#endif

struct sdup_compr;

sdup_compr *  sdup_compr_create();
void          sdup_compr_destroy(sdup_compr * d);

struct sdup_encdec;

sdup_encdec * sdup_encdec_create();
void          sdup_encdec_destroy(sdup_encdec * d);

#endif
