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

#include "common.h"
#include "ipcp-factories.h"
#include "ipcp-instances.h"
#include "ps-factory.h"
#include "pci.h"

/** An SDU Protection module sub-component */
struct sdup_comp {
	/* The SDUP module policy-set instance */
	struct rina_component base;

	/* The parent SDU protection instance */
	struct sdup_port * parent;
};

/** SDU protection instance for an N-1 port */
struct sdup_port {
	/* The id of the N-1 port this instance is protecting */
	port_id_t port_id;

	/* Cryptographic component */
	struct sdup_comp * crypto;

	/* Error check component */
	struct sdup_comp * errc;

	/* TTL component */
	struct sdup_comp * ttl;

	/* Configuration of this instance */
	struct auth_sdup_profile * conf;

	/* Data transfer constants - needed to check max pdu size on RX */
	struct dt_cons * dt_cons;

	/* Link it to the main IPCP SDU Protection component */
	struct list_head list;
};

/** The SDU Protection component of an IPC Process */
struct sdup {
	/* The SDU Protection module configuration */
	struct secman_config * sm_conf;

	/* Data transfer constants - needed to check max pdu size on RX */
	struct dt_cons * dt_cons;

	/* The list of SDU Protection instances, one per N-1 port */
	struct list_head instances;
};

struct sdup_comp * sdup_comp_from_component(struct rina_component *component);

int sdup_enc_select_policy_set(struct sdup_comp * sdup_comp,
                               const char * path,
                               const char * name);
int sdup_crypto_select_policy_set(struct sdup_comp * sdup_comp,
                                  const char * path,
                                  const char * name);
int sdup_ttl_select_policy_set(struct sdup_comp * sdup_comp,
                               const char * path,
                               const char * name);
int sdup_port_select_policy_set(struct sdup_port * instance,
                                const string_t * path,
                                const string_t * name);
int sdup_select_policy_set(struct sdup * instance,
			   const string_t * path,
			   const string_t * name);

int sdup_enc_set_policy_set_param(struct sdup_comp * sdup_comp,
                                  const char * path,
                                  const char * name,
                                  const char * value);
int sdup_crypto_set_policy_set_param(struct sdup_comp * sdup_comp,
                                     const char * path,
                                     const char * name,
                                     const char * value);
int sdup_ttl_set_policy_set_param(struct sdup_comp * sdup_comp,
                                  const char * path,
                                  const char * name,
                                  const char * value);
int sdup_set_policy_set_param(struct sdup_port * sdup_port,
                              const char * path,
                              const char * name,
                              const char * value);

int sdup_config_set(struct sdup *        instance,
		    struct secman_config * sm_config);

int sdup_dt_cons_set(struct sdup *        instance,
		     struct dt_cons *     dt_cons);

struct sdup * sdup_create(struct ipcp_instance *  parent);

int           sdup_destroy(struct sdup * instance);

struct sdup_port * sdup_init_port_config(struct sdup * instance,
			  	  	 const struct name * n1_dif_name,
			  	  	 port_id_t port_id);

int sdup_destroy_port_config(struct sdup_port * instance);

int sdup_protect_pdu(struct sdup_port * instance,
		     struct du * du);

int sdup_unprotect_pdu(struct sdup_port * instance,
		       struct du * du);

int sdup_set_lifetime_limit(struct sdup_port * instance,
			    struct du * du);

int sdup_get_lifetime_limit(struct sdup_port * instance,
			    struct du * du);

int sdup_dec_check_lifetime_limit(struct sdup_port * instance,
				  struct du * du);

int sdup_update_crypto_state(struct sdup * instance,
			     struct sdup_crypto_state * state,
			     port_id_t port_id);

#endif
