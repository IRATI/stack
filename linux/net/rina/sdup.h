/*
 * SDU Protection
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

#ifndef RINA_SDUP_H
#define RINA_SDUP_H

#include <linux/crypto.h>

#include "common.h"
#include "ipcp-factories.h"
#include "ipcp-instances.h"
#include "ps-factory.h"
#include "pdu-ser.h"

struct sdup;

struct sdup_port_conf {
	port_id_t port_id;
	struct dup_config_entry * dup_conf;

	u_int32_t  	initial_ttl_value;

	//Encryption-related fields
	bool 		enable_encryption;
	bool		enable_decryption;
	string_t * 	encryption_cipher;
	string_t * 	message_digest;
	string_t * 	compress_alg;

	struct crypto_blkcipher * blkcipher;

	struct list_head list;
};

int sdup_select_policy_set(struct sdup * instance,
			   const string_t * path,
			   const string_t * name);

int sdup_set_policy_set_param(struct sdup * sdup,
                              const char * path,
                              const char * name,
                              const char * value);


int sdup_config_set(struct sdup *        instance,
		    struct sdup_config * sdup_config);

struct sdup * sdup_create(struct ipcp_instance *  parent);

int           sdup_destroy(struct sdup * instance);

int sdup_init_port_config(struct sdup * instance,
			  const struct name * n1_dif_name,
			  port_id_t port_id);

int sdup_destroy_port_config(struct sdup * instance,
			     port_id_t port_id);

bool sdup_protect_pdu(struct sdup * instance,
		      struct pdu_ser * pdu,
		      port_id_t port_id);

bool sdup_unprotect_pdu(struct sdup * instance,
			struct pdu_ser * pdu,
			port_id_t port_id);

bool sdup_set_lifetime_limit(struct sdup * instance,
			     struct pdu_ser * pdu,
			     port_id_t port_id,
			     struct pci * pci);

bool sdup_get_lifetime_limit(struct sdup * instance,
			     struct pdu_ser * pdu,
			     port_id_t port_id,
			     size_t * ttl);

bool sdup_dec_check_lifetime_limit(struct sdup * instance,
				   struct pdu * pdu,
				   port_id_t port_id);

int sdup_enable_encryption(struct sdup *     instance,
			   bool 	    enable_encryption,
			   bool    	    enable_decryption,
			   struct buffer *  encrypt_key,
			   port_id_t 	    port_id);

struct sdup * sdup_from_component(struct rina_component * component);

#endif
