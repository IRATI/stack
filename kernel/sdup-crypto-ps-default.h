/*
 * Default SDU Protection Cryptographic Policy Set (includes a combination of
 * compression, Message Authentication Codes and Encryption mechanisms)
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

#ifndef SDUP_CRYPTO_PS_DEFAULT_H
#define SDUP_CRYPTO_PS_DEFAULT_H

#include "sdup-crypto-ps.h"

//based on TLS - can optionally be made a policy parameter
#define MAX_COMP_INFLATION 1024

int default_sdup_apply_crypto(struct sdup_crypto_ps * ps,
			      struct du * pdu);

int default_sdup_remove_crypto(struct sdup_crypto_ps * ps,
			       struct du * pdu);

int default_sdup_update_crypto_state(struct sdup_crypto_ps * ps,
				     struct sdup_crypto_state * state);

struct ps_base * sdup_crypto_ps_default_create(struct rina_component * component);

void sdup_crypto_ps_default_destroy(struct ps_base * bps);

#endif
