/*
 * Common parts of the common policy set for SDUP.
 *
 *    Ondrej Lichtner <ilichtner@fit.vutbr.cz>
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

#ifndef RINA_SDUP_PS_COMMON_H
#define RINA_SDUP_PS_COMMON_H

#include "sdup.h"
#include "pdu.h"
#include "ps-factory.h"

int common_sdup_add_padding_policy(struct sdup_ps * ps,
				   struct pdu_ser * pdu,
				   struct sdup_port_conf * port_conf);
int common_sdup_remove_padding_policy(struct sdup_ps * ps,
				      struct pdu_ser * pdu,
				      struct sdup_port_conf * port_conf);

int common_sdup_encrypt_policy(struct sdup_ps * ps,
			       struct pdu_ser * pdu,
			       struct sdup_port_conf * port_conf);
int common_sdup_decrypt_policy(struct sdup_ps * ps,
			       struct pdu_ser * pdu,
			       struct sdup_port_conf * port_conf);

int common_sdup_set_lifetime_limit_policy(struct sdup_ps * ps,
					  struct pdu_ser * pdu,
					  struct sdup_port_conf * port_conf,
					  size_t ttl);
int common_sdup_get_lifetime_limit_policy(struct sdup_ps * ps,
					  struct pdu_ser * pdu,
					  struct sdup_port_conf * port_conf,
					  size_t * ttl);
int common_sdup_dec_check_lifetime_limit_policy(struct sdup_ps * ps,
						struct pdu * pdu,
						struct sdup_port_conf * port_conf);

int common_sdup_add_error_check_policy(struct sdup_ps * ps,
				       struct pdu_ser * pdu,
				       struct sdup_port_conf * port_conf);
int common_sdup_check_error_check_policy(struct sdup_ps * ps,
					 struct pdu_ser * pdu,
					 struct sdup_port_conf * port_conf);

int common_sdup_add_hmac(struct sdup_ps * ps,
			 struct pdu_ser * pdu,
			 struct sdup_port_conf * port_conf);
int common_sdup_verify_hmac(struct sdup_ps * ps,
			    struct pdu_ser * pdu,
			    struct sdup_port_conf * port_conf);

int common_sdup_compress(struct sdup_ps *,
			 struct pdu_ser *,
			 struct sdup_port_conf *);
int common_sdup_decompress(struct sdup_ps *,
			   struct pdu_ser *,
			   struct sdup_port_conf *);
#endif
