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

#define RINA_PREFIX "pdu"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "pdu.h"
#include "sdu.h"
#include "du.h"

#define to_du(x) ((struct du *)(x))
#define to_pdu(x) ((struct pdu *)(x))

struct pdu {
	struct du pdu; /* do not move from 1st position */
};

struct pdu *
pdu_create_gfp(pdu_type_t type, struct efcp_config *cfg, gfp_t flags)
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
	tmp->pci.h = skb_push(tmp->skb, pci_len);
	tmp->pci.len = pci_len;
	tmp->cfg = cfg;
	return to_pdu(tmp);
}

struct pdu *pdu_create(pdu_type_t type, struct efcp_config *cfg)
{ return pdu_create_gfp(type, cfg, GFP_KERNEL); }
EXPORT_SYMBOL(pdu_create);

struct pdu *pdu_create_ni(pdu_type_t type, struct efcp_config *cfg)
{ return pdu_create_gfp(type, cfg, GFP_ATOMIC); }
EXPORT_SYMBOL(pdu_create_ni);


inline bool pdu_is_ok(const struct pdu *pdu)
{
	struct du *du;

	ASSERT(pdu);
	du = to_du(pdu);
	return ((du && du->skb) ? true : false);
}
EXPORT_SYMBOL(pdu_is_ok);

inline struct pdu *pdu_encap_sdu(pdu_type_t type, struct sdu *sdu)
{
	struct du *du;
	ssize_t pci_len;

	ASSERT(is_sdu_ok(sdu));

	du = to_du(sdu);
	pci_len = pci_calculate_size(du->cfg, type);
	if (pci_len > 0) {
		du->pci.h = skb_push(du->skb, pci_len);
		du->pci.len = pci_len;
		return to_pdu(du);
	}
	return NULL;
}
EXPORT_SYMBOL(pdu_encap_sdu);

inline struct pdu *pdu_decap_sdu(struct sdu *sdu)
{
	struct du *du;
	pdu_type_t type;
	ssize_t pci_len;

	ASSERT(sdu);

	du = to_du(sdu);
	du->pci.h = du->skb->data;
	type = pci_type(&du->pci);
	if (unlikely(!pdu_type_is_ok(type))) {
		LOG_ERR("Could not decap SDU to PDU. Type is not ok");
		return NULL;
	}

	pci_len = pci_calculate_size(du->cfg, type);
	if (pci_len > 0) {
		skb_pull(du->skb, pci_len);
		du->pci.len = pci_len;
		return to_pdu(du);
	}
	LOG_ERR("Could not decap SDU to PDU. PCI len is < 0");
	return NULL;
}
EXPORT_SYMBOL(pdu_decap_sdu);

static struct pdu *pdu_dup_gfp(gfp_t flags,
			       const struct pdu *pdu)
{
	struct du *tmp, *du;

	ASSERT(pdu_is_ok(pdu));

	du = to_du(pdu);
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
	return to_pdu(tmp);
}

struct pdu *pdu_dup(const struct pdu *pdu)
{ return pdu_dup_gfp(GFP_KERNEL, pdu); }
EXPORT_SYMBOL(pdu_dup);

struct pdu *pdu_dup_ni(const struct pdu *pdu)
{ return pdu_dup_gfp(GFP_ATOMIC, pdu); }
EXPORT_SYMBOL(pdu_dup_ni);

inline const struct pci *pdu_pci_get_ro(const struct pdu *pdu)
{
	ASSERT(pdu_is_ok(pdu));

	return (const struct pci *)&(to_du(pdu)->pci);
}
EXPORT_SYMBOL(pdu_pci_get_ro);

inline struct pci *pdu_pci_get_rw(struct pdu *pdu)
{
	ASSERT(pdu_is_ok(pdu));

	return &(to_du(pdu)->pci);
}
EXPORT_SYMBOL(pdu_pci_get_rw);

inline ssize_t pdu_data_len(const struct pdu *pdu)
{
	struct du *du;
	ASSERT(pdu_is_ok(pdu));

	du = to_du(pdu);
	if (du->pci.h != du->skb->data) /* up direction */
		return du->skb->len;
	return (du->skb->len - du->pci.len); /* down direction */
}
EXPORT_SYMBOL(pdu_data_len);

inline struct efcp_config *pdu_efcp_config(const struct pdu *pdu)
{
	ASSERT(pdu_is_ok(pdu));

	return to_du(pdu)->cfg;
}
EXPORT_SYMBOL(pdu_efcp_config);

int pdu_destroy(struct pdu *pdu)
{
	struct du *du;

	ASSERT(pdu);

	du = to_du(pdu);
	if (du->skb)
		kfree_skb(du->skb); /* this destroys pci too */
	rkfree(du);
	return 0;
}
EXPORT_SYMBOL(pdu_destroy);
