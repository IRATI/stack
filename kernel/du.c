/*
 * Protocol Data Unit
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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
#include <linux/version.h>

#define RINA_PREFIX "du"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"

/* If this is defined PCI is considered when growing/shrinking PDUs in SDUP */
#define PDU_HEAD_GROW_WITH_PCI
#define MAX_PCIS_LEN (40 * 5)
#define MAX_TAIL_LEN 20

int du_destroy(struct du * du)
{
	bool free_du = false;

	if (du->skb) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,13,0)
		if (likely(atomic_read(&du->skb->users) == 1))
#else
		if (likely(atomic_read(&du->skb->users.refs) == 1))
#endif
			free_du = true;
		kfree_skb(du->skb); /* this destroys pci too */
		if (likely(free_du))
			rkfree(du);
		return 0;
	}

	rkfree(du);
	return 0;
}
EXPORT_SYMBOL(du_destroy);

bool is_du_ok(const struct du * du)
{
	return (du && du->skb ? true : false);
}
EXPORT_SYMBOL(is_du_ok);

/* FIXME: this one should be removed to hide skb */
struct sk_buff * du_detach_skb(struct du * du)
{
	struct sk_buff *skb;

	skb = du->skb;
	du->skb = NULL;
	return skb;
}
EXPORT_SYMBOL(du_detach_skb);

struct du *du_create_gfp(size_t data_len, gfp_t flags)
{
	struct du *tmp;

	tmp = rkzalloc(sizeof(*tmp), flags);
	if (unlikely(!tmp))
		return NULL;

	tmp->skb = alloc_skb(MAX_PCIS_LEN + data_len + MAX_TAIL_LEN, flags);
	if (unlikely(!tmp->skb)) {
		rkfree(tmp);
		LOG_ERR("Could not allocate DU...");
		return NULL;
	}

	/* init PCI */
	tmp->pci.h = NULL;
	tmp->pci.len = 0;
	tmp->cfg = NULL;
	tmp->sdup_head = NULL;
	tmp->sdup_tail = NULL;
	skb_reserve(tmp->skb, MAX_PCIS_LEN);
	skb_put(tmp->skb, data_len);
	tmp->skb->ip_summed = CHECKSUM_UNNECESSARY;

	LOG_DBG("DU allocated at %pk, with buffer %pk", tmp, tmp->skb);

	return tmp;
}

struct du * du_create(size_t data_len)
{ return du_create_gfp(data_len, GFP_KERNEL); }
EXPORT_SYMBOL(du_create);

struct du *du_create_ni(size_t data_len)
{ return du_create_gfp(data_len, GFP_ATOMIC); }
EXPORT_SYMBOL(du_create_ni);

struct pci * du_pci(struct du * du)
{
	return &du->pci;
}
EXPORT_SYMBOL(du_pci);

/*XXX: This works well if buffer is linear */
unsigned char * du_buffer(const struct du * du)
{
	if (skb_is_nonlinear(du->skb)) {
		LOG_WARN("sk_buff contains fragments");
	}
	return du->skb->data;
}
EXPORT_SYMBOL(du_buffer);

ssize_t du_len(const struct du  *du)
{
	return du->skb->len;
}
EXPORT_SYMBOL(du_len);

void du_consume_data(struct du* du, size_t size)
{
	skb_pull(du->skb, size);
}
EXPORT_SYMBOL(du_consume_data);

int du_encap(struct du * du, pdu_type_t type)
{
	ssize_t pci_len;

	pci_len = pci_calculate_size(du->cfg, type);
	if (pci_len > 0) {
		du->pci.h = skb_push(du->skb, pci_len);
		du->pci.len = pci_len;
		return 0;
	}
	return -1;
}
EXPORT_SYMBOL(du_encap);

int du_decap(struct du * du)
{
	pdu_type_t type;
	ssize_t pci_len;

	du->pci.h = du->skb->data;
	type = pci_type(&du->pci);
	if (unlikely(!pdu_type_is_ok(type))) {
		LOG_ERR("Could not decap DU. Type is not ok");
		return -1;
	}

	pci_len = pci_calculate_size(du->cfg, type);
	if (pci_len <= 0) {
		LOG_ERR("Could not decap DU. PCI len is < 0");
		return -1;
	}

	/* Make up for tail padding introduced at lower layers. */
	if (du->skb->len > pci_length(&du->pci)) {
		du_tail_shrink(du, du->skb->len - pci_length(&du->pci));
	}

	skb_pull(du->skb, pci_len);
	du->pci.len = pci_len;

	return 0;
}
EXPORT_SYMBOL(du_decap);

static struct du *du_dup_gfp(gfp_t flags,
			     const struct du *du)
{
	struct du *tmp;

	tmp = rkmalloc(sizeof(*tmp), flags);
	if (!tmp)
		return NULL;

	tmp->skb = skb_clone(du->skb, flags);
	if (!tmp->skb) {
		rkfree(tmp);
		return NULL;
	}

	tmp->pci.h = du->pci.h;
	tmp->pci.len = du->pci.len;
	tmp->cfg = du->cfg;

	return tmp;
}

struct du *du_dup(const struct du * du)
{ return du_dup_gfp(GFP_KERNEL, du); }
EXPORT_SYMBOL(du_dup);

struct du *du_dup_ni(const struct du *du)
{ return du_dup_gfp(GFP_ATOMIC, du); }
EXPORT_SYMBOL(du_dup_ni);

ssize_t du_data_len(const struct du * du)
{
	if (du->pci.h != du->skb->data) /* up direction */
		return du->skb->len;
	return (du->skb->len - du->pci.len); /* down direction */
}
EXPORT_SYMBOL(du_data_len);

struct du * du_create_from_skb(struct sk_buff* skb)
{
	struct du *tmp;

	if (unlikely(!skb)) {
		LOG_ERR("Could not allocate SDU...");
		return NULL;
	}

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (unlikely(!tmp))
		return NULL;

	tmp->skb = skb;
	tmp->pci.h = NULL;
	tmp->pci.len = 0;
	tmp->cfg = NULL;
	tmp->sdup_head = NULL;
	tmp->sdup_tail = NULL;

	LOG_DBG("DU allocated at %pk, with skb %pk", tmp, tmp->skb);

	return tmp;
}
EXPORT_SYMBOL(du_create_from_skb);

int du_tail_grow(struct du *du, size_t bytes)
{
	if (unlikely(skb_tailroom(du->skb) < bytes)){
		LOG_DBG("Could not grow DU tail, no mem... (%d < %zd)",
			skb_tailroom(du->skb), bytes);
		if (pskb_expand_head(du->skb, 0, bytes, GFP_ATOMIC)) {
			LOG_ERR("Could not add tailroom to DU...");
			return -1;
		}

		/* Expand head has moved the pointers, update PCI */
		if (du->pci.h != NULL) {
			du->pci.h = du->skb->data;
		}
	}

	skb_put(du->skb, bytes);

	return 0;
}
EXPORT_SYMBOL(du_tail_grow);

int du_tail_shrink(struct du * du, size_t bytes)
{
	skb_trim(du->skb, du->skb->len - bytes);
	return 0;
}
EXPORT_SYMBOL(du_tail_shrink);

int du_head_grow(struct du * du, size_t bytes)
{
#ifdef PDU_HEAD_GROW_WITH_PCI
	int offset;

	if (du->pci.h != NULL)
		offset = du->skb->data - du->pci.h;
	else
		offset = 0;
#endif

	if (unlikely(skb_headroom(du->skb) < bytes)){
		LOG_DBG("Can not grow DU head, no mem... (%d < %zd)",
			 skb_headroom(du->skb), bytes);
		if (pskb_expand_head(du->skb, bytes, 0, GFP_ATOMIC)) {
			LOG_ERR("Could not add headroom to DU...");
			return -1;
		}

		/* Expand head has moved the pointers, update PCI */
		if (du->pci.h != NULL) {
			du->pci.h = du->skb->data;
		}
	}

#ifdef PDU_HEAD_GROW_WITH_PCI
	if (du->pci.h == NULL || offset <= 0) {
		/* not PCI in this PDU yet */
		skb_push(du->skb, bytes);
	} else {
		/* PCI is part of this PDU and must be considered */
		/* pci.h remains the same, skb->data is pushed bytes over */
		/* pci.h. Used when relaying */
		skb_push(du->skb, offset + bytes);
	}
#else
	skb_push(du->skb, bytes);
#endif

	return 0;
}
EXPORT_SYMBOL(du_head_grow);

int du_head_shrink(struct du * du, size_t bytes)
{
#ifdef PDU_HEAD_GROW_WITH_PCI
	void * pci_h;
#endif

#ifdef PDU_HEAD_GROW_WITH_PCI
	pci_h = skb_pull(du->skb, bytes);
	if (du->pci.h != NULL)
		du->pci.h = pci_h;
#else
	skb_pull(du->skb, bytes);
#endif

	return 0;
}
EXPORT_SYMBOL(du_head_shrink);

void * du_sdup_head(struct du *du)
{
	return du->sdup_head;
}
EXPORT_SYMBOL(du_sdup_head);

int du_sdup_head_set(struct du *du, void *header)
{
	du->sdup_head = header;
	return 0;
}
EXPORT_SYMBOL(du_sdup_head_set);

struct du * du_create_gfp_efcp(pdu_type_t type, struct efcp_config *cfg,
			       gfp_t flags)
{
	struct du *tmp;
	ssize_t pci_len;

	ASSERT(cfg);

	pci_len = pci_calculate_size(cfg, type);
	ASSERT(pci_len > 0);

	tmp = rkzalloc(sizeof(*tmp), flags);
	if (unlikely(!tmp))
		return NULL;

	tmp->skb = alloc_skb(MAX_PCIS_LEN + MAX_TAIL_LEN, flags);
	if (unlikely(!tmp->skb)) {
		rkfree(tmp);
		return NULL;
	}
	skb_reserve(tmp->skb, MAX_PCIS_LEN);
	tmp->skb->ip_summed = CHECKSUM_UNNECESSARY;
	tmp->pci.h = skb_push(tmp->skb, pci_len);
	tmp->pci.len = pci_len;
	tmp->sdup_head = NULL;
	tmp->sdup_tail = NULL;
	tmp->cfg = cfg;

	return tmp;
}

struct du *du_create_efcp(pdu_type_t type, struct efcp_config *cfg)
{ return du_create_gfp_efcp(type, cfg, GFP_KERNEL); }
EXPORT_SYMBOL(du_create_efcp);

struct du *du_create_efcp_ni(pdu_type_t type, struct efcp_config *cfg)
{ return du_create_gfp_efcp(type, cfg, GFP_ATOMIC); }
EXPORT_SYMBOL(du_create_efcp_ni);

void du_attach_skb(struct du *du, struct sk_buff *skb)
{
	du->skb = skb;
}
EXPORT_SYMBOL(du_attach_skb);

/* For shim TCP/UDP */
int du_shrink(struct du * du, size_t bytes)
{
	skb_trim(du->skb, du->skb->len - bytes);

	return 0;
}
EXPORT_SYMBOL(du_shrink);

struct du_list_item * du_list_item_create_gfp(struct du * du, gfp_t flags)
{
	struct du_list_item * item;

	item = rkzalloc(sizeof(*item), flags);
	if (unlikely(!item))
		return NULL;

	INIT_LIST_HEAD(&item->next);
	item->du = du;

	return item;
}

struct du_list_item * du_list_item_create(struct du * du)
{
	return du_list_item_create_gfp(du, GFP_KERNEL);
}
EXPORT_SYMBOL(du_list_item_create);

struct du_list_item * du_list_item_create_ni(struct du * du)
{
	return du_list_item_create_gfp(du, GFP_ATOMIC);
}
EXPORT_SYMBOL(du_list_item_create_ni);

int add_du_to_list_gfp(struct du_list * du_list, struct du * du, gfp_t flags)
{
	struct du_list_item * item;

	item = du_list_item_create_gfp(du, flags);
	if (!item)
		return -1;

	list_add_tail(&item->next, &du_list->dus);

	return 0;
}

int add_du_to_list(struct du_list * du_list, struct du * du)
{
	return add_du_to_list_gfp(du_list, du, GFP_KERNEL);
}
EXPORT_SYMBOL(add_du_to_list);

int add_du_to_list_ni(struct du_list * du_list, struct du * du)
{
	return add_du_to_list_gfp(du_list, du, GFP_ATOMIC);
}
EXPORT_SYMBOL(add_du_to_list_ni);

int du_list_item_destroy(struct du_list_item * item, bool destroy_du)
{
	if (!item)
		return -1;

	if (destroy_du)
		du_destroy(item->du);

	rkfree(item);

	return 0;
}
EXPORT_SYMBOL(du_list_item_destroy);

struct du_list * du_list_create_gfp(gfp_t flags)
{
	struct du_list * du_list = NULL;

	du_list = rkzalloc(sizeof(*du_list), flags);
	if (unlikely(!du_list))
		return NULL;

	INIT_LIST_HEAD(&du_list->dus);

	return du_list;
}

struct du_list * du_list_create()
{
	return du_list_create_gfp(GFP_KERNEL);
}
EXPORT_SYMBOL(du_list_create);

struct du_list * du_list_create_ni()
{
	return du_list_create_gfp(GFP_ATOMIC);
}
EXPORT_SYMBOL(du_list_create_ni);

int du_list_destroy(struct du_list * du_list, bool destroy_dus)
{
	if (du_list_clear(du_list, destroy_dus))
		return -1;

	rkfree(du_list);

	return  0;
}
EXPORT_SYMBOL(du_list_destroy);

int du_list_clear(struct du_list * du_list, bool destroy_dus)
{
	struct du_list_item * pos, * next;

	if (!du_list)
		return -1;

	list_for_each_entry_safe(pos, next, &du_list->dus, next) {
		du_list_item_destroy(pos, destroy_dus);
	}

	INIT_LIST_HEAD(&du_list->dus);

	return 0;
}
EXPORT_SYMBOL(du_list_clear);
