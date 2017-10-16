/*
 * Data Unit
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_DU_H
#define RINA_DU_H

#include <linux/skbuff.h>

#include "pci.h"

struct du {
	struct efcp_config *cfg;
	struct pci pci;
	void *sdup_head; /* opaque used by SDU protection policy (TTL)*/
	void *sdup_tail; /* opaque used by SDU protection policy (error check) */
	struct sk_buff *skb;
};

struct pci * du_pci(struct du * du);
struct du * du_create_ni(size_t data_len);
struct du * du_create(size_t data_len);
struct du *du_create_efcp_ni(pdu_type_t type, struct efcp_config *cfg);
struct du *du_create_efcp(pdu_type_t type, struct efcp_config *cfg);
int du_destroy(struct du * du);
bool is_du_ok(const struct du * du);
struct sk_buff * du_detach_skb(struct du * du);
void du_attach_skb(struct du *du, struct sk_buff *skb);
unsigned char * du_buffer(const struct du * du);
ssize_t du_len(const struct du  *du);
void du_consume_data(struct du* du, size_t size);
int du_encap(struct du * du, pdu_type_t type);
int du_decap(struct du * du);
struct du *du_dup(const struct du * du);
struct du *du_dup_ni(const struct du *du);
ssize_t du_data_len(const struct du * du);
struct du * du_create_from_skb(struct sk_buff* skb);
int du_tail_grow(struct du *du, size_t bytes);
int du_tail_shrink(struct du * du, size_t bytes);
int du_head_grow(struct du * du, size_t bytes);
int du_head_shrink(struct du * du, size_t bytes);
void * du_sdup_head(struct du *du);
int du_sdup_head_set(struct du *pdu, void *header);
struct du * du_from_buffer_ni(void *buffer);
int du_shrink(struct du * du, size_t bytes);

#endif
