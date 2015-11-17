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
#include "sdup-enc-ps.h"
#include "sdup-errc-ps.h"
#include "sdup-ttl-ps.h"

static struct policy_set_list enc_policy_sets = {
	.head = LIST_HEAD_INIT(enc_policy_sets.head)
};

static struct policy_set_list errc_policy_sets = {
	.head = LIST_HEAD_INIT(errc_policy_sets.head)
};

static struct policy_set_list ttl_policy_sets = {
	.head = LIST_HEAD_INIT(ttl_policy_sets.head)
};

struct sdup_enc *sdup_enc_from_component(struct rina_component *component)
{ return container_of(component, struct sdup_enc, base); }
EXPORT_SYMBOL(sdup_enc_from_component);

struct sdup_errc *sdup_errc_from_component(struct rina_component *component)
{ return container_of(component, struct sdup_errc, base); }
EXPORT_SYMBOL(sdup_errc_from_component);

struct sdup_ttl *sdup_ttl_from_component(struct rina_component *component)
{ return container_of(component, struct sdup_ttl, base); }
EXPORT_SYMBOL(sdup_ttl_from_component);

int sdup_enc_select_policy_set(struct sdup_enc * sdup_enc,
                               const string_t *  path,
                               const string_t *  name)
{
        struct ps_select_transaction trans;

        BUG_ON(!path);

        if (strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        base_select_policy_set_start(&sdup_enc->base,
        			     &trans,
        			     &enc_policy_sets,
        			     name);

        if (trans.state == PS_SEL_TRANS_PENDING) {
                struct sdup_enc_ps *ps;

                ps = container_of(trans.candidate_ps, struct sdup_enc_ps, base);
                if (!ps->sdup_add_hmac || !ps->sdup_compress ||
                		!ps->sdup_decompress || !ps->sdup_decrypt_policy ||
                		!ps->sdup_encrypt_policy ||  !ps->sdup_verify_hmac ||
                		!ps->sdup_enable_encryption) {
                        LOG_ERR("SDUP encryption policy set is invalid, policies are "
                                "missing:\n"
                                "       sdup_add_hmac=%p\n"
                                "       sdup_compress=%p\n"
                                "       sdup_decompress=%p\n"
                                "       sdup_decrypt_policy=%p\n"
                                "       sdup_encrypt_policy=%p\n"
                                "       sdup_verify_hmac=%p\n"
                        	"       sdup_enable_encryption=%p\n",
                                ps->sdup_add_hmac,
                                ps->sdup_compress,
                                ps->sdup_decompress,
                                ps->sdup_decrypt_policy,
                                ps->sdup_encrypt_policy,
                                ps->sdup_verify_hmac,
                                ps->sdup_enable_encryption);
                        trans.state = PS_SEL_TRANS_ABORTED;
                }
        }

        base_select_policy_set_finish(&sdup_enc->base, &trans);

        return trans.state = PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(sdup_enc_select_policy_set);

int sdup_errc_select_policy_set(struct sdup_errc * sdup_errc,
                                const string_t *  path,
                                const string_t *  name)
{
        struct ps_select_transaction trans;

        BUG_ON(!path);

        if (strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        base_select_policy_set_start(&sdup_errc->base,
        			     &trans,
        			     &errc_policy_sets,
        			     name);

        if (trans.state == PS_SEL_TRANS_PENDING) {
                struct sdup_errc_ps *ps;

                ps = container_of(trans.candidate_ps, struct sdup_errc_ps, base);
                if (!ps->sdup_add_error_check_policy ||
                		!ps->sdup_check_error_check_policy) {
                        LOG_ERR("SDUP error check policy set is invalid, policies are "
                                "missing:\n"
                                "       sdup_add_error_check_policy=%p\n"
                                "       sdup_check_error_check_policy=%p\n",
                                ps->sdup_add_error_check_policy,
                                ps->sdup_check_error_check_policy);
                        trans.state = PS_SEL_TRANS_ABORTED;
                }
        }

        base_select_policy_set_finish(&sdup_errc->base, &trans);

        return trans.state = PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(sdup_errc_select_policy_set);

int sdup_ttl_select_policy_set(struct sdup_ttl * sdup_ttl,
                               const string_t *  path,
                               const string_t *  name)
{
        struct ps_select_transaction trans;

        BUG_ON(!path);

        if (strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        base_select_policy_set_start(&sdup_ttl->base,
        			     &trans,
        			     &ttl_policy_sets,
        			     name);

        if (trans.state == PS_SEL_TRANS_PENDING) {
                struct sdup_ttl_ps *ps;

                ps = container_of(trans.candidate_ps, struct sdup_ttl_ps, base);
                if (!ps->sdup_dec_check_lifetime_limit_policy ||
                		!ps->sdup_get_lifetime_limit_policy ||
                		!ps->sdup_set_lifetime_limit_policy) {
                        LOG_ERR("SDUP TTL policy set is invalid, policies are "
                                "missing:\n"
                                "       sdup_dec_check_lifetime_limit_policy=%p\n"
                                "       sdup_get_lifetime_limit_policy=%p\n"
                                "       sdup_set_lifetime_limit_policy=%p\n",
                                ps->sdup_dec_check_lifetime_limit_policy,
                                ps->sdup_get_lifetime_limit_policy,
                                ps->sdup_set_lifetime_limit_policy);
                        trans.state = PS_SEL_TRANS_ABORTED;
                }
        }

        base_select_policy_set_finish(&sdup_ttl->base, &trans);

        return trans.state = PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(sdup_ttl_select_policy_set);

int sdup_select_policy_set(struct sdup_port * instance,
			   const string_t * path,
			   const string_t * name)
{
        size_t cmplen;
        size_t offset;

	ASSERT(path);

        ps_factory_parse_component_id(path, &cmplen, &offset);

        if (cmplen && strncmp(path, "enc", cmplen) == 0) {
                /* The request addresses the Encryption subcomponent. */
                return sdup_enc_select_policy_set(instance->enc,
                				  path + offset,
                				  name);
        }

        if (cmplen && strncmp(path, "errc", cmplen) == 0) {
                /* The request addresses the Error Check subcomponent. */
                return sdup_errc_select_policy_set(instance->errc,
                				   path + offset,
                				   name);
        }

        if (cmplen && strncmp(path, "ttl", cmplen) == 0) {
                /* The request addresses the TTL subcomponent. */
                return sdup_ttl_select_policy_set(instance->ttl,
                				  path + offset,
                				  name);
        }

        if (strcmp(path, "") != 0) {
                LOG_ERR("This component has no subcomponent named '%s'",
                	path);
                return -1;
        }

        return -1;
}
EXPORT_SYMBOL(sdup_select_policy_set);

static void sdup_port_destroy(struct sdup_port * instance)
{
	if (!instance)
		return;

	list_del(&(instance->list));

	if (instance->enc) {
		instance->enc->parent = NULL;
		rina_component_fini(&instance->enc->base);
		rkfree(instance->enc);
	}

	if (instance->errc) {
		instance->errc->parent = NULL;
		rina_component_fini(&instance->errc->base);
		rkfree(instance->errc);
	}

	if (instance->ttl) {
		instance->ttl->parent = NULL;
		rina_component_fini(&instance->ttl->base);
		rkfree(instance->ttl);
	}

	instance->conf = NULL;

	rkfree(instance);
};

struct sdup_port * sdup_port_create(port_id_t port_id,
				    struct dup_config_entry * dup_conf)
{
	struct sdup_port * tmp;
	struct sdup_enc * sdup_enc;
	struct sdup_errc * sdup_errc;
	struct sdup_ttl * sdup_ttl;
	const string_t * enc_ps_name;
	const string_t * errc_ps_name;
	const string_t * ttl_ps_name;

	if (!dup_conf) {
		LOG_ERR("Bogus configuration entry passed");
		return NULL;
	}

	tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp) {
		return NULL;
	}

	INIT_LIST_HEAD(&(tmp->list));

	tmp->port_id = port_id;
	tmp->conf = dup_conf;

	if (dup_conf->encryption_policy) {
		enc_ps_name = policy_name(dup_conf->encryption_policy);
		tmp->enc = rkmalloc(sizeof(*tmp->enc), GFP_KERNEL);
		if (!tmp->enc) {
			sdup_port_destroy(tmp);
			return NULL;
		}

		if (sdup_enc_select_policy_set(tmp->enc, "", enc_ps_name)) {
			LOG_ERR("Could not set policy set %s for SDUP Enc",
				enc_ps_name);
			sdup_port_destroy(tmp);
			return NULL;
		}

		tmp->enc->parent = tmp;
	} else
		tmp->enc = NULL;

	if (dup_conf->error_check_policy) {
		errc_ps_name = policy_name(dup_conf->error_check_policy);
		tmp->errc = rkmalloc(sizeof(*tmp->errc), GFP_KERNEL);
		if (!tmp->errc) {
			sdup_port_destroy(tmp);
			return NULL;
		}

		if (sdup_errc_select_policy_set(tmp->errc, "", errc_ps_name)) {
			LOG_ERR("Could not set policy set %s for SDUP Errc",
				errc_ps_name);
			sdup_port_destroy(tmp);
			return NULL;
		}

		tmp->errc->parent = tmp;
	} else
		tmp->errc = NULL;

	if (dup_conf->ttl_policy) {
		ttl_ps_name = policy_name(dup_conf->ttl_policy);
		tmp->ttl = rkmalloc(sizeof(*tmp->ttl), GFP_KERNEL);
		if (!tmp->ttl) {
			sdup_port_destroy(tmp);
			return NULL;
		}

		if (sdup_ttl_select_policy_set(tmp->ttl, "", ttl_ps_name)) {
			LOG_ERR("Could not set policy set %s for SDUP TTL",
				ttl_ps_name);
			sdup_port_destroy(tmp);
			return NULL;
		}

		tmp->ttl->parent = tmp;
	} else
		tmp->ttl = NULL;

	return tmp;
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

static struct sdup_port * find_port(struct sdup * sdup,
				    port_id_t port_id)
{
	struct sdup_port * pos;
	list_for_each_entry(pos, &(sdup->instances), list) {
		if (port_id == pos->port_id){
			LOG_DBG("SDUP port found for port_id %d", port_id);
			return pos;
		}
	}

	LOG_WARN("SDUP port not found for port-id %d", port_id);
	return NULL;
}

int sdup_enc_set_policy_set_param(struct sdup_enc * sdup_enc,
                                  const char * path,
                                  const char * name,
                                  const char * value)
{
        struct sdup_enc_ps * ps;
        int ret = -1;

        if (!sdup_enc || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", pff, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this PFF instance. */
                rcu_read_lock();

                ps = container_of(rcu_dereference(sdup_enc->base.ps),
                                  struct sdup_enc_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this SDUP Encryption component");
                } else {
                        LOG_ERR("Unknown SDUP Encryption parameter policy '%s'", name);
                }

                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&sdup_enc->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(sdup_enc_set_policy_set_param);

int sdup_errc_set_policy_set_param(struct sdup_errc * sdup_errc,
                                   const char * path,
                                   const char * name,
                                   const char * value)
{
        struct sdup_errc_ps * ps;
        int ret = -1;

        if (!sdup_errc || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", pff, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this PFF instance. */
                rcu_read_lock();

                ps = container_of(rcu_dereference(sdup_errc->base.ps),
                                  struct sdup_errc_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this SDUP Error check component");
                } else {
                        LOG_ERR("Unknown SDUP Error check parameter policy '%s'", name);
                }

                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&sdup_errc->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(sdup_errc_set_policy_set_param);

int sdup_ttl_set_policy_set_param(struct sdup_ttl * sdup_ttl,
                                  const char * path,
                                  const char * name,
                                  const char * value)
{
        struct sdup_enc_ps * ps;
        int ret = -1;

        if (!sdup_ttl || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", pff, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this PFF instance. */
                rcu_read_lock();

                ps = container_of(rcu_dereference(sdup_ttl->base.ps),
                                  struct sdup_ttl_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this SDUP TTL component");
                } else {
                        LOG_ERR("Unknown SDUP TTL parameter policy '%s'", name);
                }

                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&sdup_ttl->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(sdup_ttl_set_policy_set_param);

int sdup_set_policy_set_param(struct sdup_port * sdup_port,
                              const char * path,
                              const char * name,
                              const char * value)
{
	size_t cmplen;
	size_t offset;
	int ret = -1;

	if (!sdup_port || !path || !name || !value) {
		LOG_ERRF("NULL arguments %p %p %p %p", rmt, path, name, value);
		return -1;
	}

	ps_factory_parse_component_id(path, &cmplen, &offset);

	LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

	if (strcmp(path, "") == 0) {
		return -1;

	}

	if (cmplen && strncmp(path, "enc", cmplen) == 0) {
		/* The request addresses the sdup-enc subcomponent. */
		return sdup_enc_set_policy_set_param(sdup_port->enc,
						     path + offset,
						     name,
						     value);

	} else if (cmplen && strncmp(path, "errc", cmplen) == 0) {
		/* The request addresses the sdup-errc subcomponent. */
		return sdup_errc_set_policy_set_param(sdup_port->errc,
						      path + offset,
						      name,
						      value);
	} else if (cmplen && strncmp(path, "ttl", cmplen) == 0) {
		/* The request addresses the sdup-ttl subcomponent. */
		return sdup_ttl_set_policy_set_param(sdup_port->ttl,
						     path + offset,
						     name,
						     value);
	} else {
		return -1;
        }
}
EXPORT_SYMBOL(sdup_set_policy_set_param);

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
	INIT_LIST_HEAD(&(tmp->instances));

	LOG_INFO("SDUP Created!!!");

	return tmp;
fail:
	sdup_destroy(tmp);
	return NULL;
}
EXPORT_SYMBOL(sdup_create);

int sdup_destroy(struct sdup * instance)
{
	struct sdup_port * pos, * nxt;

	if (!instance) {
		LOG_ERR("Bogus instance passed, bailing out");
		return -1;
	}

	/* dealloc attributes */;
	list_for_each_entry_safe(pos, nxt,
				 &instance->instances,
				 list) {
		sdup_port_destroy(pos);
	}

	if (instance->sdup_conf)
		sdup_config_destroy(instance->sdup_conf);

	rkfree(instance);

	LOG_DBG("Instance %pK finalized successfully", instance);

	return 0;
}
EXPORT_SYMBOL(sdup_destroy);

struct sdup_port * sdup_init_port_config(struct sdup * instance,
			  	         const struct name * n1_dif_name,
			  	         port_id_t port_id)
{
	struct sdup_port *tmp;
	struct dup_config_entry * dup_conf;

	dup_conf = find_dup_config(instance->sdup_conf,
				   n1_dif_name->process_name);
	if (!dup_conf) {
		LOG_ERR("Problems finding config for N-1 DIF %s", n1_dif_name);
		return NULL;
	}

	tmp = sdup_port_create(port_id, dup_conf);
	if (!tmp) {
		LOG_ERR("Problems creating SDUP port for port_id %d", port_id);
		return NULL;
	}

	list_add(&(tmp->list), &(instance->instances));

	LOG_INFO("Initialized SDUP configuration for port %d", port_id);

	return tmp;
}
EXPORT_SYMBOL(sdup_init_port_config);

int sdup_destroy_port_config(struct sdup * instance,
			     port_id_t port_id)
{
	struct sdup_port * pos, * nxt;

	list_for_each_entry_safe(pos, nxt,
				 &instance->instances,
				 list) {
		if (pos->port_id == port_id){
			sdup_port_destroy(pos);
			LOG_INFO("Destroyed SDUP configuration for port %d",
				 port_id);
			return 0;
		}
	}

	LOG_INFO("No SDUP configuration found for port %d", port_id);

	return -1;
}
EXPORT_SYMBOL(sdup_destroy_port_config);

int sdup_protect_pdu(struct sdup_port * instance,
		     struct pdu_ser * pdu)
{
	struct sdup_port * port;
	struct sdup_enc_ps * enc_ps;
	struct sdup_errc_ps * errc_ps;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	rcu_read_lock();
	if (instance->enc) {
		enc_ps = container_of(rcu_dereference(instance->enc->base.ps),
				      struct sdup_enc_ps,
				      base);

		if (enc_ps->sdup_compress(enc_ps, pdu)) {
			rcu_read_unlock();
			return -1;
		}
	}

	if (instance->errc) {
		errc_ps = container_of(rcu_dereference(instance->errc->base.ps),
				       struct sdup_errc_ps,
				       base);

		if (errc_ps->sdup_add_error_check_policy(errc_ps, pdu)) {
			rcu_read_unlock();
			return -1;
		}
	}

	if (instance->enc) {
		if (enc_ps->sdup_add_hmac(enc_ps, pdu)) {
			rcu_read_unlock();
			return -1;
		}
	}

	rcu_read_unlock();

	LOG_DBG("Protected pdu.");

	return 0;
}
EXPORT_SYMBOL(sdup_protect_pdu);

int sdup_unprotect_pdu(struct sdup_port * instance,
			struct pdu_ser * pdu)
{
	struct sdup_port * port;
	struct sdup_enc_ps * enc_ps;
	struct sdup_errc_ps * errc_ps;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	rcu_read_lock();
	if (instance->enc) {
		enc_ps = container_of(rcu_dereference(instance->enc->base.ps),
				      struct sdup_enc_ps,
				      base);

		if (enc_ps->sdup_verify_hmac(enc_ps, pdu)) {
			rcu_read_unlock();
			return -1;
		}
	}

	if (instance->errc) {
		errc_ps = container_of(rcu_dereference(instance->errc->base.ps),
				       struct sdup_errc_ps,
				       base);

		if (errc_ps->sdup_check_error_check_policy(errc_ps, pdu)) {
			rcu_read_unlock();
			return -1;
		}
	}

	if (instance->enc) {
		if (enc_ps->sdup_decompress(enc_ps, pdu)) {
			rcu_read_unlock();
			return -1;
		}
	}

	rcu_read_unlock();

	LOG_DBG("Unprotected pdu.");

	return 0;
}
EXPORT_SYMBOL(sdup_unprotect_pdu);

int sdup_set_lifetime_limit(struct sdup_port * instance,
			    struct pdu_ser * pdu,
			    struct pci * pci)
{
	struct sdup_port * port;
	struct sdup_ttl_ps * ttl_ps;
	ssize_t ttl;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	rcu_read_lock();

	if (instance->ttl) {
		ttl_ps = container_of(rcu_dereference(instance->ttl->base.ps),
				      struct sdup_ttl_ps,
				      base);

		ttl = pci_ttl(pci);

		if (ttl_ps->sdup_set_lifetime_limit_policy(ttl_ps, pdu, ttl)) {
			rcu_read_unlock();
			return -1;
		}
	}

	rcu_read_unlock();

	return 0;
}
EXPORT_SYMBOL(sdup_set_lifetime_limit);

int sdup_get_lifetime_limit(struct sdup_port * instance,
			     struct pdu_ser * pdu,
			     size_t * ttl)
{
	struct sdup_port * port;
	struct sdup_ttl_ps * ttl_ps;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	rcu_read_lock();
	if (instance->ttl) {
		ttl_ps = container_of(rcu_dereference(instance->ttl->base.ps),
				      struct sdup_ttl_ps,
				      base);

		if (ttl_ps->sdup_get_lifetime_limit_policy(ttl_ps, pdu, ttl)) {
			rcu_read_unlock();
			return -1;
		}
	}

	rcu_read_unlock();

	return 0;
}
EXPORT_SYMBOL(sdup_get_lifetime_limit);

int sdup_dec_check_lifetime_limit(struct sdup_port * instance,
				  struct pdu * pdu)
{
	struct sdup_port * port;
	struct sdup_ttl_ps * ttl_ps;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	rcu_read_lock();
	if (instance->ttl) {
		ttl_ps = container_of(rcu_dereference(instance->ttl->base.ps),
				      struct sdup_ttl_ps,
				      base);

		if (ttl_ps->sdup_dec_check_lifetime_limit_policy(ttl_ps, pdu)) {
			rcu_read_unlock();
			return -1;
		}
	}

	rcu_read_unlock();
	return 0;
}
EXPORT_SYMBOL(sdup_dec_check_lifetime_limit);

int sdup_enable_encryption(struct sdup_port * instance,
			   bool enable_encryption,
			   bool enable_decryption,
			   struct buffer * encrypt_key)
{
	struct sdup_port * port;
	struct sdup_enc_ps * enc_ps;

	if (!instance || !encrypt_key) {
		LOG_ERR("Bogus parameters passed");
		return -1;
	}

	rcu_read_lock();
	if (instance->enc) {
		enc_ps = container_of(rcu_dereference(instance->enc->base.ps),
				      struct sdup_enc_ps,
				      base);

		if (enc_ps->sdup_enable_encryption(enc_ps,
						   enable_encryption,
						   enable_decryption,
						   encrypt_key)) {
			rcu_read_unlock();
			return -1;
		}
	}

	return 0;
}
EXPORT_SYMBOL(sdup_enable_encryption);

int sdup_enc_ps_publish(struct ps_factory * factory)
{
	if (factory == NULL) {
		LOG_ERR("%s: NULL factory", __func__);
		return -1;
	}

	return ps_publish(&enc_policy_sets, factory);
}
EXPORT_SYMBOL(sdup_enc_ps_publish);

int sdup_enc_ps_unpublish(const char * name)
{ return ps_unpublish(&enc_policy_sets, name); }
EXPORT_SYMBOL(sdup_enc_ps_unpublish);

int sdup_errc_ps_publish(struct ps_factory * factory)
{
	if (factory == NULL) {
		LOG_ERR("%s: NULL factory", __func__);
		return -1;
	}

	return ps_publish(&errc_policy_sets, factory);
}
EXPORT_SYMBOL(sdup_errc_ps_publish);

int sdup_errc_ps_unpublish(const char * name)
{ return ps_unpublish(&errc_policy_sets, name); }
EXPORT_SYMBOL(sdup_errc_ps_unpublish);

int sdup_ttl_ps_publish(struct ps_factory * factory)
{
	if (factory == NULL) {
		LOG_ERR("%s: NULL factory", __func__);
		return -1;
	}

	return ps_publish(&ttl_policy_sets, factory);
}
EXPORT_SYMBOL(sdup_ttl_ps_publish);

int sdup_ttl_ps_unpublish(const char * name)
{ return ps_unpublish(&ttl_policy_sets, name); }
EXPORT_SYMBOL(sdup_ttl_ps_unpublish);
