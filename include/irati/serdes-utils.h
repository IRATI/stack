/*
 * Utilities to serialize and deserialize
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#ifndef IRATI_SERDES_H
#define IRATI_SERDES_H

#include "irati/kucommon.h"

#ifdef __cplusplus
extern "C" {
#endif

struct irati_msg_layout {
    unsigned int copylen;
    unsigned int names;
    unsigned int strings;
    unsigned int buffers;
};

void serialize_string(void **pptr, const char *s);
int deserialize_string(const void **pptr, char **s);

int rina_sername_valid(const char *str);
unsigned rina_name_serlen(const struct rina_name *name);
void serialize_rina_name(void **pptr, const struct name *name);
int deserialize_rina_name(const void **pptr, struct name *name);
void rina_name_free(struct name *name);
void rina_name_move(struct name *dst, struct name *src);
int rina_name_copy(struct name *dst, const struct name *src);
char *rina_name_to_string(const struct name *name);
int rina_name_from_string(const char *str, struct name *name);
int rina_name_cmp(const struct name *one, const struct name *two);
int rina_name_fill(struct name *name, const char *apn,
                   const char *api, const char *aen, const char *aei);
int rina_name_valid(const struct name *name);

unsigned int irati_msg_serlen(struct irati_msg_layout *numtables,
                              size_t num_entries,
                              const struct irati_msg_base *msg);
unsigned int irati_numtables_max_size(struct irati_msg_layout *numtables,
                                      unsigned int n);
unsigned int serialize_irati_msg(struct irati_msg_layout *numtables,
                                 size_t num_entries,
                                 void *serbuf,
                                 const struct irati_msg_base *msg);
int deserialize_irati_msg(struct irati_msg_layout *numtables, size_t num_entries,
                          const void *serbuf, unsigned int serbuf_len,
                          void *msgbuf, unsigned int msgbuf_len);
void irati_msg_free(struct irati_msg_layout *numtables, size_t num_entries,
                    struct irati_msg_base *msg);

#ifdef __KERNEL__
/* GFP variations of some of the functions above. */
int __rina_name_fill(struct name *name, const char *apn,
                      const char *api, const char *aen, const char *aei,
                      int maysleep);

char * __rina_name_to_string(const struct name *name, int maysleep);
int __rina_name_from_string(const char *str, struct name *name,
                            int maysleep);
#else  /* !__KERNEL__ */
#endif /* !__KERNEL__ */

#ifdef __cplusplus
}
#endif

#endif /* IRATI_SERDES_H */
