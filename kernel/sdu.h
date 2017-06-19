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

#ifndef RINA_SDU_H
#define RINA_SDU_H

#include <linux/types.h>
#include <linux/list.h>

#include "common.h"

#define MAX_PCIS_LEN (40 * 5)
#define MAX_TAIL_LEN 20

struct efcp_config;
struct pdu;
struct sk_buff;

/*
 * Represents and SDU with the port-id the SDU is to be written to
 * or has been read from OR the destination address
 */
struct sdu_wpi {
        struct sdu * sdu;
        address_t    dst_addr;
        port_id_t    port_id;
};

struct sdu		*sdu_create(size_t data_len);
struct sdu		*sdu_create_ni(size_t data_len);
struct sdu		*sdu_from_buffer_ni(void *buffer);
struct sdu		*sdu_create_from_skb(struct sk_buff* skb);

int			sdu_destroy(struct sdu * sdu);
bool		is_sdu_ok(const struct sdu *sdu);
ssize_t		sdu_len(const struct sdu *sdu);
unsigned char	*sdu_buffer(const struct sdu *sdu);
void 		sdu_consume_data(struct sdu* sdu, size_t size);

/* FIXME: these two have to be removed */
struct sk_buff	*sdu_detach_skb(const struct sdu *sdu);
void		sdu_attach_skb(struct sdu *sdu, struct sk_buff *skb);

int		sdu_efcp_config_bind(struct sdu *sdu,
					     struct efcp_config *cfg);
struct sdu	*sdu_from_pdu(struct pdu *pdu);

/* For shim TCP/UDP */
int			sdu_shrink(struct sdu *sdu, size_t bytes);
/* For shim HV */
int			sdu_pop(struct sdu *sdu, size_t bytes);
int			sdu_push(struct sdu *sdu, size_t bytes);

/* For SDU_WPI */
struct sdu_wpi		*sdu_wpi_create(size_t data_len);
struct sdu_wpi		*sdu_wpi_create_ni(size_t data_len);
int			sdu_wpi_destroy(struct sdu_wpi *s);
bool		sdu_wpi_is_ok(const struct sdu_wpi *s);
int		sdu_wpi_detach(struct sdu_wpi *s);

#endif
