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

#define to_du(pdu) ((struct du *)(pdu))

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
	return (struct pdu*)tmp;
}

struct pdu *pdu_create(pdu_type_t type, struct efcp_config *cfg)
{ return pdu_create_gfp(type, cfg, GFP_KERNEL); }
EXPORT_SYMBOL(pdu_create);

struct pdu *pdu_create_ni(pdu_type_t type, struct efcp_config *cfg)
{ return pdu_create_gfp(type, cfg, GFP_ATOMIC); }
EXPORT_SYMBOL(pdu_create_ni);


inline bool pdu_is_ok(const struct pdu *pdu)
{
	struct du *p;

	ASSERT(pdu);
	p = to_du(pdu);
	return ((p && p->skb) ? true : false);
}
EXPORT_SYMBOL(pdu_is_ok);

inline struct pdu *pdu_encap_sdu(pdu_type_t type, struct sdu *sdu)
{
	struct du *pdu;
	ssize_t pci_len;

	ASSERT(is_sdu_ok(sdu));

	pdu = (struct du *)sdu;
	pci_len = pci_calculate_size(pdu->cfg, type);
	if (pci_len > 0) {
		pdu->pci.h = skb_push(pdu->skb, pci_len);
		pdu->pci.len = pci_len;
		return (struct pdu*)pdu;
	}
	return NULL;
}
EXPORT_SYMBOL(pdu_encap_sdu);

inline struct pdu *pdu_decap_sdu(struct sdu *sdu)
{
	struct du *pdu;
	pdu_type_t type;
	ssize_t pci_len;

	ASSERT(sdu);

	pdu = (struct du *)sdu;
	pdu->pci.h = pdu->skb->data;
	type = pci_type(&pdu->pci);
	if (unlikely(!pdu_type_is_ok(type))) {
		LOG_ERR("Could not decap SDU to PDU. Type is not ok");
		return NULL;
	}

	pci_len = pci_calculate_size(pdu->cfg, type);
	if (pci_len > 0) {
		skb_pull(pdu->skb, pci_len);
		pdu->pci.len = pci_len;
		return (struct pdu*)pdu;
	}
	LOG_ERR("Could not decap SDU to PDU. PCI len is < 0");
	return NULL;
}
EXPORT_SYMBOL(pdu_decap_sdu);

static struct pdu *pdu_dup_gfp(gfp_t flags,
			       const struct pdu *pdu)
{
	struct du *tmp, *p;

	ASSERT(pdu_is_ok(pdu));

	p = to_du(pdu);
	tmp = rkmalloc(sizeof(*tmp), flags);
	if (!tmp)
		return NULL;

	tmp->skb = skb_clone(p->skb, flags);
	if (!tmp->skb) {
		rkfree(tmp);
		return NULL;
	}

	tmp->pci.h = p->pci.h;
	tmp->pci.len = p->pci.len;
	tmp->cfg = p->cfg;
	return (struct pdu *)tmp;
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

	return (const struct pci *)&(pdu->pdu.pci);
}
EXPORT_SYMBOL(pdu_pci_get_ro);

inline struct pci *pdu_pci_get_rw(struct pdu *pdu)
{
	ASSERT(pdu_is_ok(pdu));

	return &(pdu->pdu.pci);
}
EXPORT_SYMBOL(pdu_pci_get_rw);

inline ssize_t pdu_data_len(const struct pdu *pdu)
{
	struct du *p;
	ASSERT(pdu_is_ok(pdu));

	p = to_du(pdu);
	if (p->pci.h != p->skb->data) /* up direction */
		return p->skb->len;
	return (p->skb->len - p->pci.len); /* down direction */
}
EXPORT_SYMBOL(pdu_data_len);

inline struct efcp_config *pdu_efcp_config(const struct pdu *pdu)
{
	ASSERT(pdu_is_ok(pdu));

	return pdu->pdu.cfg;
}
EXPORT_SYMBOL(pdu_efcp_config);

/*inline int pdu_pci_set(struct pdu *pdu, struct pci *pci)
{
	struct du *p;

	ASSERT(pdu);
	ASSERT(pci_is_ok(pci));

	p = to_du(pdu);
	p->pci.h = skb_put(p->skb, p->skb->data - pci->h);
	if (unlikely(pci->h != p->pci.h)) {
		LOG_ERR("Something went wrong setting a PCI");
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(pdu_pci_set);
*/

int pdu_destroy(struct pdu *pdu)
{
	struct du *p;

	ASSERT(pdu);

	p = to_du(pdu);
	if (p->skb)
		kfree_skb(p->skb); /* this destroys pci too */
	rkfree(p);
	return 0;
}
EXPORT_SYMBOL(pdu_destroy);
