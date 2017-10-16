/*
 * SDU Protection
 *
 *    Ondrej Lichtner <ilichtner@fit.vutbr.cz>
 *    Eduard Grasa    <eduard.grasa@i2cat.net>
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
#include "sdup-crypto-ps.h"
#include "sdup-errc-ps.h"
#include "sdup-ttl-ps.h"
#include "irati/kucommon.h"

static struct policy_set_list crypto_policy_sets = {
	.head = LIST_HEAD_INIT(crypto_policy_sets.head)
};

static struct policy_set_list errc_policy_sets = {
	.head = LIST_HEAD_INIT(errc_policy_sets.head)
};

static struct policy_set_list ttl_policy_sets = {
	.head = LIST_HEAD_INIT(ttl_policy_sets.head)
};

struct sdup_comp *sdup_comp_from_component(struct rina_component *component)
{ return container_of(component, struct sdup_comp, base); }
EXPORT_SYMBOL(sdup_comp_from_component);

int sdup_crypto_select_policy_set(struct sdup_comp * sdup_comp,
                                  const string_t *  path,
                                  const string_t *  name)
{
        struct ps_select_transaction trans;

        BUG_ON(!path);

        if (strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        base_select_policy_set_start(&sdup_comp->base,
        			     &trans,
        			     &crypto_policy_sets,
        			     name);

        if (trans.state == PS_SEL_TRANS_PENDING) {
                struct sdup_crypto_ps *ps;

                ps = container_of(trans.candidate_ps, struct sdup_crypto_ps, base);
                if (!ps->sdup_apply_crypto|| !ps->sdup_remove_crypto ||
                		!ps->sdup_update_crypto_state) {
                        LOG_ERR("SDUP encryption policy set is invalid, policies are "
                                "missing:\n"
                                "       sdup_apply_crypto=%p\n"
                                "       sdup_remove_crypto=%p\n"
                        	"       sdup_update_crypto_state=%p\n",
                                ps->sdup_apply_crypto,
                                ps->sdup_remove_crypto,
                                ps->sdup_update_crypto_state);
                        trans.state = PS_SEL_TRANS_ABORTED;
                }
        }

        base_select_policy_set_finish(&sdup_comp->base, &trans);

        return trans.state = PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(sdup_crypto_select_policy_set);

int sdup_errc_select_policy_set(struct sdup_comp * sdup_comp,
                                const string_t *  path,
                                const string_t *  name)
{
        struct ps_select_transaction trans;

        BUG_ON(!path);

        if (strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        base_select_policy_set_start(&sdup_comp->base,
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

        base_select_policy_set_finish(&sdup_comp->base, &trans);

        return trans.state = PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(sdup_errc_select_policy_set);

int sdup_ttl_select_policy_set(struct sdup_comp * sdup_comp,
                               const string_t *  path,
                               const string_t *  name)
{
        struct ps_select_transaction trans;

        BUG_ON(!path);

        if (strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        base_select_policy_set_start(&sdup_comp->base,
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

        base_select_policy_set_finish(&sdup_comp->base, &trans);

        return trans.state = PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(sdup_ttl_select_policy_set);

int sdup_port_select_policy_set(struct sdup_port * instance,
			        const string_t * path,
			        const string_t * name)
{
        size_t cmplen;
        size_t offset;

	ASSERT(path);

        ps_factory_parse_component_id(path, &cmplen, &offset);

        if (cmplen && strncmp(path, "crypto", cmplen) == 0) {
                /* The request addresses the crypto subcomponent. */
                return sdup_crypto_select_policy_set(instance->crypto,
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
EXPORT_SYMBOL(sdup_port_select_policy_set);

static struct sdup_port * find_port(struct sdup * sdup,
				    port_id_t port_id);

int sdup_select_policy_set(struct sdup * instance,
			   const string_t * path,
			   const string_t * name)
{
        struct sdup_port *sdup_port;
        int portid;
        size_t cmplen;
        size_t offset;
        char numbuf[8];
        int ret;

        if (!path) {
                LOG_ERR("NULL path");
                return -1;
        }

        ps_factory_parse_component_id(path, &cmplen, &offset);
        if (cmplen > sizeof(numbuf)-1) {
                LOG_ERR("Invalid port-id' %s'", path);
                return -1;
        }

        memcpy(numbuf, path, cmplen);
        numbuf[cmplen] = '\0';
        ret = kstrtoint(numbuf, 10, &portid);
        if (ret) {
                LOG_ERR("Invalid port-id '%s'", path);
                return -1;
        }

        sdup_port = find_port(instance, portid);
        if (!sdup_port) {
            LOG_ERR("No N-1 port with port-id %d", portid);
            return -1;
        }

        return sdup_port_select_policy_set(sdup_port, path + offset, name);
}
EXPORT_SYMBOL(sdup_select_policy_set);

static struct sdup_comp * sdup_comp_create(struct sdup_port * parent)
{
	struct sdup_comp * result;

	if (!parent)
		return NULL;

	result = rkmalloc(sizeof(*result), GFP_ATOMIC);
	if (!result)
		return NULL;

	rina_component_init(&result->base);
	result->parent = parent;

	return result;
}

static void sdup_comp_destroy(struct sdup_comp * sdup_comp)
{
	if (!sdup_comp)
		return;

	rina_component_fini(&sdup_comp->base);
	sdup_comp->parent = NULL;
	rkfree(sdup_comp);
}

static void sdup_port_destroy(struct sdup_port * instance)
{
	if (!instance)
		return;

	list_del(&(instance->list));

	sdup_comp_destroy(instance->crypto);
	sdup_comp_destroy(instance->errc);
	sdup_comp_destroy(instance->ttl);

	instance->conf = NULL;

	rkfree(instance);
};

static struct sdup_port * sdup_port_create(port_id_t port_id,
					   struct auth_sdup_profile * dup_conf,
					   struct dt_cons * dt_cons)
{
	struct sdup_port * tmp;
	const string_t * crypto_ps_name;
	const string_t * errc_ps_name;
	const string_t * ttl_ps_name;

	if (!dup_conf) {
		LOG_ERR("Bogus configuration entry passed");
		return NULL;
	}

	tmp = rkmalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		return NULL;
	}

	INIT_LIST_HEAD(&(tmp->list));

	tmp->port_id = port_id;
	tmp->conf = dup_conf;
	tmp->dt_cons = dt_cons;

	if (dup_conf->encrypt && policy_name(dup_conf->encrypt)) {
		crypto_ps_name = policy_name(dup_conf->encrypt);
		tmp->crypto = sdup_comp_create(tmp);
		if (!tmp->crypto) {
			sdup_port_destroy(tmp);
			return NULL;
		}

		if (sdup_crypto_select_policy_set(tmp->crypto, "", crypto_ps_name)) {
			LOG_ERR("Could not set policy set %s for SDUP Crypto",
				crypto_ps_name);
			sdup_port_destroy(tmp);
			return NULL;
		}
	} else
		tmp->crypto = NULL;

	if (dup_conf->crc && policy_name(dup_conf->crc)) {
		errc_ps_name = policy_name(dup_conf->crc);
		tmp->errc = sdup_comp_create(tmp);
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
	} else
		tmp->errc = NULL;

	if (dup_conf->ttl && policy_name(dup_conf->ttl)) {
		ttl_ps_name = policy_name(dup_conf->ttl);
		tmp->ttl = sdup_comp_create(tmp);
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
	} else
		tmp->ttl = NULL;

	return tmp;
}

static struct auth_sdup_profile * find_dup_config(struct secman_config * sdup_conf,
						 string_t * n_1_dif_name)
{
	struct auth_sdup_profile_entry * dup_pos;

	if (!sdup_conf)
		return NULL;

	list_for_each_entry(dup_pos, &sdup_conf->specific_profiles, next) {
		if (string_cmp(dup_pos->n1_dif_name,
			       n_1_dif_name) == 0) {
			LOG_DBG("SDU Protection config for N-1 DIF %s",
				n_1_dif_name);
			return dup_pos->entry;
		}
	}

	LOG_DBG("Returning default SDU Protection config for N-1 DIF %s",
		n_1_dif_name);
	return sdup_conf->default_profile;
}

int sdup_crypto_set_policy_set_param(struct sdup_comp * sdup_comp,
                                     const char * path,
                                     const char * name,
                                     const char * value)
{
        struct sdup_crypto_ps * ps;
        int ret = -1;

        if (!sdup_comp || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", sdup_comp, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this PFF instance. */
                rcu_read_lock();

                ps = container_of(rcu_dereference(sdup_comp->base.ps),
                                  struct sdup_crypto_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this SDUP Crypto component");
                } else {
                        LOG_ERR("Unknown SDUP Crypto parameter policy '%s'", name);
                }

                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&sdup_comp->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(sdup_crypto_set_policy_set_param);

int sdup_errc_set_policy_set_param(struct sdup_comp * sdup_comp,
                                   const char * path,
                                   const char * name,
                                   const char * value)
{
        struct sdup_errc_ps * ps;
        int ret = -1;

        if (!sdup_comp || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", sdup_comp, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this PFF instance. */
                rcu_read_lock();

                ps = container_of(rcu_dereference(sdup_comp->base.ps),
                                  struct sdup_errc_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this SDUP Error check component");
                } else {
                        LOG_ERR("Unknown SDUP Error check parameter policy '%s'", name);
                }

                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&sdup_comp->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(sdup_errc_set_policy_set_param);

int sdup_ttl_set_policy_set_param(struct sdup_comp * sdup_comp,
                                  const char * path,
                                  const char * name,
                                  const char * value)
{
        struct sdup_ttl_ps * ps;
        int ret = -1;

        if (!sdup_comp || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", sdup_comp, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this PFF instance. */
                rcu_read_lock();

                ps = container_of(rcu_dereference(sdup_comp->base.ps),
                                  struct sdup_ttl_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this SDUP TTL component");
                } else {
                        LOG_ERR("Unknown SDUP TTL parameter policy '%s'", name);
                }

                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&sdup_comp->base, path, name, value);
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

	if (!sdup_port || !path || !name || !value) {
		LOG_ERRF("NULL arguments %p %p %p %p", sdup_port, path, name, value);
		return -1;
	}

	ps_factory_parse_component_id(path, &cmplen, &offset);

	LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

	if (strcmp(path, "") == 0) {
		return -1;

	}

	if (cmplen && strncmp(path, "crypto", cmplen) == 0) {
		/* The request addresses the sdup-crypto subcomponent. */
		return sdup_crypto_set_policy_set_param(sdup_port->crypto,
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
		    struct secman_config * sm_config)
{
	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	if (!sm_config) {
		LOG_ERR("Bogus sdup_conf passed");
		return -1;
	}

	instance->sm_conf = sm_config;

	return 0;
}
EXPORT_SYMBOL(sdup_config_set);

int sdup_dt_cons_set(struct sdup *        instance,
		     struct dt_cons *     dt_cons)
{
	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	if (!dt_cons) {
		LOG_ERR("Bogus dt_cons passed");
		return -1;
	}

	instance->dt_cons = dt_cons;

	return 0;
}
EXPORT_SYMBOL(sdup_dt_cons_set);

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
	tmp->sm_conf = NULL;
	INIT_LIST_HEAD(&(tmp->instances));

	return tmp;
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

	if (instance->sm_conf)
		secman_config_free(instance->sm_conf);

	if (instance->dt_cons)
		rkfree(instance->dt_cons);

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
	struct auth_sdup_profile * dup_conf;

	dup_conf = find_dup_config(instance->sm_conf,
				   n1_dif_name->process_name);
	if (!dup_conf) {
		LOG_ERR("Problems finding config for N-1 DIF %s",
			n1_dif_name->process_name);
		return NULL;
	}

	tmp = sdup_port_create(port_id, dup_conf, instance->dt_cons);
	if (!tmp) {
		LOG_ERR("Problems creating SDUP port for port_id %d", port_id);
		return NULL;
	}

	list_add(&(tmp->list), &(instance->instances));

	LOG_DBG("Initialized SDUP configuration for port %d", port_id);

	return tmp;
}
EXPORT_SYMBOL(sdup_init_port_config);

int sdup_destroy_port_config(struct sdup_port * instance)
{
	port_id_t port_id;

	if (!instance) {
		LOG_ERR("Cannot destroy bogus instance");
		return -1;
	}

	port_id = instance->port_id;

	sdup_port_destroy(instance);
	LOG_DBG("Destroyed SDUP configuration for port %d",
		 port_id);

	return 0;
}
EXPORT_SYMBOL(sdup_destroy_port_config);

int sdup_protect_pdu(struct sdup_port * instance,
		     struct du * du)
{
	struct sdup_crypto_ps * crypto_ps = NULL;
	struct sdup_errc_ps * errc_ps = NULL;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	rcu_read_lock();
	if (instance->crypto) {
		crypto_ps = container_of(rcu_dereference(instance->crypto->base.ps),
				         struct sdup_crypto_ps,
				         base);

		if (crypto_ps->sdup_apply_crypto(crypto_ps, du)) {
			rcu_read_unlock();
			return -1;
		}
	}

	if (instance->errc) {
		errc_ps = container_of(rcu_dereference(instance->errc->base.ps),
				       struct sdup_errc_ps,
				       base);

		if (errc_ps->sdup_add_error_check_policy(errc_ps, du)) {
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
		       struct du * du)
{
	struct sdup_crypto_ps * crypto_ps = NULL;
	struct sdup_errc_ps * errc_ps = NULL;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	rcu_read_lock();

	if (instance->errc) {
		errc_ps = container_of(rcu_dereference(instance->errc->base.ps),
				       struct sdup_errc_ps,
				       base);

		if (errc_ps->sdup_check_error_check_policy(errc_ps, du)) {
			rcu_read_unlock();
			return -1;
		}
	}

	if (instance->crypto) {
		crypto_ps = container_of(rcu_dereference(instance->crypto->base.ps),
				         struct sdup_crypto_ps,
				         base);

		if (crypto_ps->sdup_remove_crypto(crypto_ps, du)) {
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
			    struct du * du)
{
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

		if (ttl_ps->sdup_set_lifetime_limit_policy(ttl_ps, du)) {
			rcu_read_unlock();
			return -1;
		}
	}

	rcu_read_unlock();

	return 0;
}
EXPORT_SYMBOL(sdup_set_lifetime_limit);

int sdup_get_lifetime_limit(struct sdup_port * instance,
			    struct du * du)
{
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

		if (ttl_ps->sdup_get_lifetime_limit_policy(ttl_ps, du)) {
			rcu_read_unlock();
			return -1;
		}
	}

	rcu_read_unlock();

	return 0;
}
EXPORT_SYMBOL(sdup_get_lifetime_limit);

int sdup_dec_check_lifetime_limit(struct sdup_port * instance,
				  struct du * du)
{
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

		if (ttl_ps->sdup_dec_check_lifetime_limit_policy(ttl_ps, du)) {
			rcu_read_unlock();
			return -1;
		}
	}

	rcu_read_unlock();
	return 0;
}
EXPORT_SYMBOL(sdup_dec_check_lifetime_limit);

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

	LOG_WARN("SDUP port config not found for port_id %d", port_id);
	return NULL;
}

int sdup_update_crypto_state(struct sdup * instance,
			     struct sdup_crypto_state * state,
			     port_id_t port_id)
{
	struct sdup_port * port;
	struct sdup_crypto_ps * crypto_ps;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	port = find_port(instance, port_id);
	if (!port)
		return -1;

	rcu_read_lock();
	if (port->crypto) {
		crypto_ps = container_of(rcu_dereference(port->crypto->base.ps),
				         struct sdup_crypto_ps,
				         base);

		if (crypto_ps->sdup_update_crypto_state(crypto_ps,
						        state)) {
			rcu_read_unlock();
			return -1;
		}
	}
	rcu_read_unlock();

	return 0;
}
EXPORT_SYMBOL(sdup_update_crypto_state);

int sdup_crypto_ps_publish(struct ps_factory * factory)
{
	if (factory == NULL) {
		LOG_ERR("%s: NULL factory", __func__);
		return -1;
	}

	return ps_publish(&crypto_policy_sets, factory);
}
EXPORT_SYMBOL(sdup_crypto_ps_publish);

int sdup_crypto_ps_unpublish(const char * name)
{ return ps_unpublish(&crypto_policy_sets, name); }
EXPORT_SYMBOL(sdup_crypto_ps_unpublish);

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
