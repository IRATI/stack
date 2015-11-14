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

#include <linux/export.h>
#include <linux/types.h>
#include <linux/hashtable.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/string.h>
/*FIXME: to be re removed after removing tasklets */
#include <linux/interrupt.h>

#define RINA_PREFIX "sdup"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "policies.h"
#include "ipcp-instances.h"
#include "ipcp-utils.h"
#include "sdup.h"
#include "sdup-ps.h"

static struct policy_set_list policy_sets = {
	.head = LIST_HEAD_INIT(policy_sets.head)
};

struct sdup {
	struct rina_component     base;
	struct sdup_config *      sdup_conf;

	struct list_head          port_confs;
};

struct sdup_port_conf * port_conf_create(port_id_t port_id,
					 struct dup_config_entry * dup_conf)
{
	struct sdup_port_conf * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp){
		LOG_ERR("Failed to alloc SDUP Port Configuration.");
		return NULL;
	}

	tmp->port_id = port_id;

	INIT_LIST_HEAD(&(tmp->list));

	tmp->dup_conf = dup_conf;

	tmp->initial_ttl_value = 0;
	tmp->blkcipher = NULL;
	tmp->enable_encryption = false;
	tmp->enable_decryption = false;
	tmp->compress_alg = NULL;

	/* init the rest of attributes */
	if (dup_conf != NULL){
		if (dup_conf->encryption_cipher != NULL){
			tmp->blkcipher =
				crypto_alloc_blkcipher(dup_conf->encryption_cipher,
						       0,
						       0);
			if (IS_ERR(tmp->blkcipher)) {
				LOG_ERR("could not allocate blkcipher handle for %s\n",
					dup_conf->encryption_cipher);
				return NULL;
			}
		}

		tmp->initial_ttl_value = dup_conf->initial_ttl_value;
		tmp->enable_encryption = false;
		tmp->enable_decryption = false;
		tmp->compress_alg = dup_conf->compress_alg;
	}
	return tmp;
}

static int * port_conf_destroy(struct sdup_port_conf * instance)
{
	list_del(&(instance->list));

	/* dealloc the rest of attributes */
	if (instance->blkcipher != NULL){
		crypto_free_blkcipher(instance->blkcipher);
	}

	rkfree(instance);
	return 0;
}

static int extract_policy_parameters(struct dup_config_entry * entry)
{
	struct policy * policy;
	struct policy_parm * parameter;
	const string_t * aux;

	if (!entry) {
		LOG_ERR("Bogus entry passed");
		return -1;
	}

	policy = entry->ttl_policy;
	if (policy) {
		parameter = policy_param_find(policy, "initialValue");
		if (!parameter) {
			LOG_ERR("Could not find 'initialValue' in TTL policy");
			return -1;
		}

		if (kstrtouint(policy_param_value(parameter),
			       10,
			       &entry->initial_ttl_value)) {
			LOG_ERR("Failed to convert TTL string to int");
			return -1;
		}

		LOG_DBG("Initial TTL value is %u", entry->initial_ttl_value);
	}

	policy = entry->encryption_policy;
	if (policy) {
		parameter = policy_param_find(policy, "encryptAlg");
		if (!parameter) {
			LOG_ERR("Could not find 'encryptAlg' in Encr. policy");
			return -1;
		}

		aux = policy_param_value(parameter);
		if (string_cmp(aux, "AES128") == 0 ||
		    string_cmp(aux, "AES256") == 0) {
			if (string_dup("ecb(aes)",
				       &entry->encryption_cipher)) {
				LOG_ERR("Problems copying 'encryptAlg' value");
				return -1;
			}
			LOG_DBG("Encryption cipher is %s",
				entry->encryption_cipher);
		} else {
			LOG_DBG("Unsupported encryption cipher %s", aux);
		}

		parameter = policy_param_find(policy, "macAlg");
		if (!parameter) {
			LOG_ERR("Could not find 'macAlg' in Encrypt. policy");
			return -1;
		}

		aux = policy_param_value(parameter);
		if (string_cmp(aux, "SHA1") == 0) {
			if (string_dup("sha1", &entry->message_digest)) {
				LOG_ERR("Problems copying 'digest' value");
				return -1;
			}
			LOG_DBG("Message digest is %s", entry->message_digest);
		} else if (string_cmp(aux, "MD5") == 0) {
			if (string_dup("md5", &entry->message_digest)) {
				LOG_ERR("Problems copying 'digest' value)");
				return -1;
			}
			LOG_DBG("Message digest is %s", entry->message_digest);
		} else {
			LOG_DBG("Unsupported message digest %s", aux);
		}
	}

	return 0;
}

static struct dup_config_entry * find_dup_config(struct sdup_config * sdup_conf,
						 string_t * n_1_dif_name)
{
	struct dup_config * dup_pos;

	if (!sdup_conf)
		return NULL;

	list_for_each_entry(dup_pos, &sdup_conf->specific_dup_confs, next) {
		if (string_cmp(dup_pos->entry->n_1_dif_name,
			       n_1_dif_name) == 0) {
			LOG_DBG("SDU Protection config for N-1 DIF %s",
				n_1_dif_name);
			return dup_pos->entry;
		}
	}

	LOG_DBG("Returning default SDU Protection config for N-1 DIF %s",
		n_1_dif_name);
	return sdup_conf->default_dup_conf;
}

static struct sdup_port_conf * find_port_conf(struct sdup * sdup,
					      port_id_t port_id)
{
	struct sdup_port_conf * pos;
	list_for_each_entry(pos, &(sdup->port_confs), list) {
		if (port_id == pos->port_id){
			LOG_DBG("SDUP port config found");
			return pos;
		}
	}

	LOG_DBG("SDUP port config not found");
	return NULL;
}

int sdup_select_policy_set(struct sdup * instance,
			   const string_t * path,
			   const string_t * name)
{
	struct ps_select_transaction trans;

	ASSERT(path);

	if (strcmp(path, "") != 0) {
		LOG_ERR("This component has no subcomponent named '%s'", path);
		return -1;
	}

	/* The request addresses this policy-set. */
	base_select_policy_set_start(&instance->base,
				     &trans,
				     &policy_sets,
				     name);

	if (trans.state == PS_SEL_TRANS_PENDING) {
		struct sdup_ps * ps;

		ps = container_of(trans.candidate_ps, struct sdup_ps, base);
		if (!ps){
			LOG_ERR("SDUP policy set is invalid");
			trans.state = PS_SEL_TRANS_ABORTED;
		}
	}

	base_select_policy_set_finish(&instance->base, &trans);

	return trans.state == PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(sdup_select_policy_set);

int sdup_config_set(struct sdup *        instance,
		    struct sdup_config * sdup_config)
{
	struct dup_config * dup_pos;
	LOG_INFO("SDUP Config received.");

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	if (!sdup_config) {
		LOG_ERR("Bogus sdup_conf passed");
		return -1;
	}

	/*verify policy params, TODO without extract policy parameters */
	if (extract_policy_parameters(sdup_config->default_dup_conf)) {
		LOG_DBG("Setting SDU protection policies to NULL");
		sdup_config_destroy(sdup_config);
		return -1;
	}
	list_for_each_entry(dup_pos, &sdup_config->specific_dup_confs, next) {
		if (extract_policy_parameters(dup_pos->entry)) {
			LOG_DBG("Setting sdu protection policies to NULL");
			sdup_config_destroy(sdup_config);
			return -1;
		}
	}

	instance->sdup_conf = sdup_config;

	return 0;
}
EXPORT_SYMBOL(sdup_config_set);

struct sdup * sdup_create(struct ipcp_instance * parent)
{
	struct sdup * tmp;

	if (!parent) {
		LOG_ERR("Bogus input parameters");
		return NULL;
	}

	tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp)
		return NULL;

	/*init attributes*/
	tmp->sdup_conf = NULL;
	INIT_LIST_HEAD(&(tmp->port_confs));

	rina_component_init(&tmp->base);
	if (sdup_select_policy_set(tmp, "", RINA_PS_DEFAULT_NAME)) {
		goto fail;
	}
	LOG_INFO("SDUP Created!!!");

	return tmp;
fail:
	sdup_destroy(tmp);
	return NULL;
}
EXPORT_SYMBOL(sdup_create);

int sdup_destroy(struct sdup * instance)
{
	struct sdup_port_conf * pos, * nxt;

	if (!instance) {
		LOG_ERR("Bogus instance passed, bailing out");
		return -1;
	}

	/* dealloc attributes */;
	list_for_each_entry_safe(pos, nxt,
				 &instance->port_confs,
				 list) {
		port_conf_destroy(pos);
	}

	if (instance->sdup_conf)
		sdup_config_destroy(instance->sdup_conf);

	rina_component_fini(&instance->base);

	rkfree(instance);

	LOG_DBG("Instance %pK finalized successfully", instance);

	return 0;
}
EXPORT_SYMBOL(sdup_destroy);

int sdup_init_port_config(struct sdup * instance,
			  const struct name * n1_dif_name,
			  port_id_t port_id)
{
	struct sdup_port_conf *tmp;
	struct dup_config_entry * dup_conf;

	dup_conf = find_dup_config(instance->sdup_conf,
				   n1_dif_name->process_name);
	tmp = port_conf_create(port_id, dup_conf);

	list_add(&(tmp->list), &(instance->port_confs));

	LOG_INFO("Initialized SDUP configuration for port");
	return 0;
}
EXPORT_SYMBOL(sdup_init_port_config);

int sdup_destroy_port_config(struct sdup * instance,
			     port_id_t port_id)
{
	struct sdup_port_conf * pos, * nxt;

	list_for_each_entry_safe(pos, nxt,
				 &instance->port_confs,
				 list) {
		if (pos->port_id == port_id){
			port_conf_destroy(pos);
			LOG_INFO("Destroyed SDUP configuration for port");
			return 0;
		}
	}
	LOG_INFO("No SDUP configuration found for port");
	return -1;
}
EXPORT_SYMBOL(sdup_destroy_port_config);

bool sdup_protect_pdu(struct sdup * instance,
		      struct pdu_ser * pdu,
		      port_id_t port_id)
{
	struct sdup_port_conf * port_conf;
	struct sdup_ps * ps;

	port_conf = find_port_conf(instance, port_id);

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
			  struct sdup_ps,
			  base);

	if (ps->sdup_compress(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}

	if (ps->sdup_add_padding_policy(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}

	if (ps->sdup_encrypt_policy(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}

	if (ps->sdup_add_error_check_policy(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}

	if (ps->sdup_add_hmac(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}
	rcu_read_unlock();

	LOG_DBG("Protected pdu.");
	return 0;
}
EXPORT_SYMBOL(sdup_protect_pdu);

bool sdup_unprotect_pdu(struct sdup * instance,
			struct pdu_ser * pdu,
			port_id_t port_id)
{
	struct sdup_port_conf * port_conf;
	struct sdup_ps * ps;

	port_conf = find_port_conf(instance, port_id);

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
			  struct sdup_ps,
			  base);

	if (ps->sdup_verify_hmac(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}

	if (ps->sdup_check_error_check_policy(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}

	if (ps->sdup_decrypt_policy(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}

	if (ps->sdup_remove_padding_policy(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}

	if (ps->sdup_decompress(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}

	rcu_read_unlock();

	LOG_DBG("Unprotected pdu.");
	return 0;
}
EXPORT_SYMBOL(sdup_unprotect_pdu);

bool sdup_set_lifetime_limit(struct sdup * instance,
			     struct pdu_ser * pdu,
			     port_id_t port_id,
			     struct pci * pci)
{
	struct sdup_port_conf * port_conf;
	struct sdup_ps * ps;
	size_t ttl;

	port_conf = find_port_conf(instance, port_id);

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
			  struct sdup_ps,
			  base);

	ttl = pci_ttl(pci);

	if (ps->sdup_set_lifetime_limit_policy(ps, pdu, port_conf, ttl)){
		rcu_read_unlock();
		return -1;
	}

	rcu_read_unlock();

	return 0;
}
EXPORT_SYMBOL(sdup_set_lifetime_limit);

bool sdup_get_lifetime_limit(struct sdup * instance,
			     struct pdu_ser * pdu,
			     port_id_t port_id,
			     size_t * ttl)
{
	struct sdup_port_conf * port_conf;
	struct sdup_ps * ps;

	port_conf = find_port_conf(instance, port_id);

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
			  struct sdup_ps,
			  base);

	if (ps->sdup_get_lifetime_limit_policy(ps, pdu, port_conf, ttl)){
		rcu_read_unlock();
		return -1;
	}

	rcu_read_unlock();
	return 0;
}
EXPORT_SYMBOL(sdup_get_lifetime_limit);

bool sdup_dec_check_lifetime_limit(struct sdup * instance,
				   struct pdu * pdu,
				   port_id_t port_id)
{
	struct sdup_port_conf * port_conf;
	struct sdup_ps * ps;

	port_conf = find_port_conf(instance, port_id);

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
			  struct sdup_ps,
			  base);

	if (ps->sdup_dec_check_lifetime_limit_policy(ps, pdu, port_conf)){
		rcu_read_unlock();
		return -1;
	}

	rcu_read_unlock();
	return 0;
}
EXPORT_SYMBOL(sdup_dec_check_lifetime_limit);

int sdup_enable_encryption(struct sdup *    instance,
			   bool 	    	  enable_encryption,
			   bool      	  enable_decryption,
			   struct buffer * encrypt_key,
			   port_id_t 	  port_id)
{
	struct sdup_port_conf * port_conf;

	if (!instance) {
		LOG_ERR("Bogus SDUP instance passed");
		return -1;
	}

	if (!encrypt_key) {
		LOG_ERR("Bogus encryption key passed");
		return -1;
	}

	port_conf = find_port_conf(instance, port_id);
	if (!port_conf) {
		LOG_ERR("Could not find SDUP Port Configuration for N-1 port %d", port_id);
		return -1;
	}

	if (!port_conf->dup_conf) {
		LOG_ERR("SDU Protection for N-1 port %d is NULL", port_id);
		return -1;
	}

	if (!port_conf->dup_conf->encryption_policy) {
		LOG_ERR("Encryption policy for N-1 port %d is NULL", port_id);
		return -1;
	}

	if (!port_conf->blkcipher) {
		LOG_ERR("Block cipher is not set for N-1 port %d", port_id);
		return -1;
	}

	if (!port_conf->enable_decryption &&
	    !port_conf->enable_encryption) {
		if (crypto_blkcipher_setkey(port_conf->blkcipher,
					    buffer_data_ro(encrypt_key),
					    buffer_length(encrypt_key))) {
			LOG_ERR("Could not set encryption key for N-1 port %d",
				port_id);
			return -1;
		}
	}

	if (!port_conf->enable_decryption){
		port_conf->enable_decryption = enable_decryption;
	}
	if (!port_conf->enable_encryption){
		port_conf->enable_encryption = enable_encryption;
	}

	LOG_DBG("Encryption enabled state: %d", port_conf->enable_encryption);
	LOG_DBG("Decryption enabled state: %d", port_conf->enable_decryption);

	return 0;
}
EXPORT_SYMBOL(sdup_enable_encryption);

struct sdup *
sdup_from_component(struct rina_component * component)
{
	return container_of(component, struct sdup, base);
}
EXPORT_SYMBOL(sdup_from_component);

int sdup_ps_publish(struct ps_factory * factory)
{
	if (factory == NULL) {
		LOG_ERR("%s: NULL factory", __func__);
		return -1;
	}

	return ps_publish(&policy_sets, factory);
}
EXPORT_SYMBOL(sdup_ps_publish);

int sdup_ps_unpublish(const char * name)
{ return ps_unpublish(&policy_sets, name); }
EXPORT_SYMBOL(sdup_ps_unpublish);
