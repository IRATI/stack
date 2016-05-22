/*
 * Service Data Unit
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
#include <linux/skbuff.h>

#define RINA_PREFIX "sdu"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "sdu.h"
#include "du.h"

#define to_du(sdu) ((struct du *)(sdu))

struct sdu {
	struct du sdu; /* do not move from 1st position */
};

struct sdu *sdu_create_gfp(size_t data_len, gfp_t flags)
{
	struct du *tmp;

	tmp = rkzalloc(sizeof(*tmp), flags);
	if (unlikely(!tmp))
		return NULL;

	tmp->skb = alloc_skb(MAX_PCIS_LEN + data_len + MAX_TAIL_LEN, flags);
	if (unlikely(!tmp->skb)) {
		rkfree(tmp);
		LOG_ERR("Could not allocate SDU...");
		return NULL;
	}
	tmp->pci.h = 0;
	tmp->cfg = NULL;
	skb_reserve(tmp->skb, MAX_PCIS_LEN);
	skb_put(tmp->skb, data_len);

	LOG_DBG("SDU allocated at %pk, with buffer %pk", tmp, tmp->skb);
	return (struct sdu*)tmp;
}

struct sdu *sdu_create(size_t data_len)
{ return sdu_create_gfp(data_len, GFP_KERNEL); }
EXPORT_SYMBOL(sdu_create);

struct sdu *sdu_create_ni(size_t data_len)
{ return sdu_create_gfp(data_len, GFP_ATOMIC); }
EXPORT_SYMBOL(sdu_create_ni);

struct sdu *sdu_from_buffer_ni(void *buffer)
{
	struct du *tmp;

	if (!unlikely(buffer))
		return NULL;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!unlikely(tmp))
		return NULL;

	/* FIXME: check if skb_get is needed */
	//tmp->skb = skb_get((struct sk_buff *)buffer);
	tmp->skb = (struct sk_buff *)buffer;
	tmp->pci.h = 0;
	tmp->cfg = NULL;

	LOG_DBG("SDU allocated at %pk, with buffer %pk", tmp, tmp->skb);
	return (struct sdu*)tmp;
}
EXPORT_SYMBOL(sdu_from_buffer_ni);

inline bool is_sdu_ok(const struct sdu *sdu)
{
	struct du *s;

	s = to_du(sdu);
	return (s && s->skb && !s->pci.h ? true : false);
}
EXPORT_SYMBOL(is_sdu_ok);

inline int sdu_efcp_config_bind(struct sdu *sdu, struct efcp_config *cfg)
{
	struct du *s;

	ASSERT(is_sdu_ok(sdu));
	ASSERT (cfg);

	s = to_du(sdu);
	s->cfg = cfg;
	return 0;
}
EXPORT_SYMBOL(sdu_efcp_config_bind);

inline ssize_t sdu_len(const struct sdu *sdu)
{
	struct du *s;

	ASSERT(is_sdu_ok(sdu));
	s = to_du(sdu);
	return s->skb->len;
}
EXPORT_SYMBOL(sdu_len);

inline unsigned char *sdu_buffer(const struct sdu *sdu)
{
	struct du *s;

	ASSERT(is_sdu_ok(sdu));
	s = to_du(sdu);
	return s->skb->data;
}
EXPORT_SYMBOL(sdu_buffer);

/* FIXME: this one should be removed to hide skb */
inline struct sk_buff *sdu_detach_skb(const struct sdu *sdu)
{
	struct du *s;
	struct sk_buff *skb;

	ASSERT(is_sdu_ok(sdu));
	s = to_du(sdu);
	skb = s->skb;
	s->skb = NULL;
	return skb;
}
EXPORT_SYMBOL(sdu_detach_skb);

/* FIXME: this one should be removed to hide skb */
inline void sdu_attach_skb(struct sdu *sdu, struct sk_buff *skb)
{
	ASSERT(sdu);
	ASSERT(skb);
	to_du(sdu)->skb = skb;
	return;
}
EXPORT_SYMBOL(sdu_attach_skb);

inline struct sdu *sdu_from_pdu(struct pdu *pdu)
{
	struct du *sdu;

	ASSERT(pdu);

	sdu = (struct du *)pdu;
	sdu->pci.h = 0;
	sdu->pci.len = 0;
	return (struct sdu *)sdu;
}
EXPORT_SYMBOL(sdu_from_pdu);

int sdu_destroy(struct sdu *sdu)
{
	struct du *s;

	ASSERT(sdu);

	s = to_du(sdu);
	if (s->skb)
		kfree_skb((s->skb));
	rkfree(s);
	return 0;
}
EXPORT_SYMBOL(sdu_destroy);

struct sdu_wpi *sdu_wpi_create_gfp(size_t data_len, gfp_t flags)
{
	struct sdu_wpi *tmp;

	tmp = rkzalloc(sizeof(*tmp), flags);
	if (unlikely(!tmp))
		return NULL;

	tmp->sdu = sdu_create_gfp(data_len, flags);
	if (unlikely(!is_sdu_ok(tmp->sdu))) {
		rkfree(tmp);
		return NULL;
	}
	return tmp;
}

struct sdu_wpi * sdu_wpi_create(size_t data_len)
{ return sdu_wpi_create_gfp(data_len, GFP_KERNEL); }
EXPORT_SYMBOL(sdu_wpi_create);

struct sdu_wpi * sdu_wpi_create_ni(size_t data_len)
{ return sdu_wpi_create_gfp(data_len, GFP_ATOMIC); }
EXPORT_SYMBOL(sdu_wpi_create_ni);

int sdu_wpi_destroy(struct sdu_wpi * s)
{
        ASSERT(s);

	if (s->sdu)
        	sdu_destroy(s->sdu);
        rkfree(s);

        return 0;
}
EXPORT_SYMBOL(sdu_wpi_destroy);

bool sdu_wpi_is_ok(const struct sdu_wpi * s)
{ return (s && is_sdu_ok(s->sdu)) ? true : false; }
EXPORT_SYMBOL(sdu_wpi_is_ok);

inline int sdu_wpi_detach(struct sdu_wpi *s)
{
	if (s->sdu)
		s->sdu = NULL;
	return 0;
}
EXPORT_SYMBOL(sdu_wpi_detach);
