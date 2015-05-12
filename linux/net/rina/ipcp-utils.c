/*
 * IPC Processes related utilities
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#define RINA_PREFIX "ipcp-utils"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "common.h"
#include "ipcp-utils.h"
#include "policies.h"

/* FIXME: These externs have to disappear from here */
extern int string_dup_gfp(gfp_t            flags,
                          const string_t * src,
                          string_t **      dst);

static struct name * name_create_gfp(gfp_t flags)
{ return rkzalloc(sizeof(struct name), flags); }

struct name * name_create(void)
{ return name_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(name_create);

struct name * name_create_ni(void)
{ return name_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(name_create_ni);

/* FIXME: This thing is bogus and has to be fixed properly */
#ifdef CONFIG_RINA_ASSERTIONS
static bool name_is_initialized(struct name * dst)
{
        ASSERT(dst);

        if (!dst->process_name     &&
            !dst->process_instance &&
            !dst->entity_name      &&
            !dst->entity_instance)
                return true;

        return false;
}
#endif

struct name * name_init_with(struct name * dst,
                             string_t *    process_name,
                             string_t *    process_instance,
                             string_t *    entity_name,
                             string_t *    entity_instance)
{
        if (!dst)
                return NULL;

        /* Clean up the destination, leftovers might be there ... */
        name_fini(dst);

        ASSERT(name_is_initialized(dst));

        dst->process_name     = process_name;
        dst->process_instance = process_instance;
        dst->entity_name      = entity_name;
        dst->entity_instance  = entity_instance;

        return dst;
}
EXPORT_SYMBOL(name_init_with);

static struct name * name_init_from_gfp(gfp_t            flags,
                                        struct name *    dst,
                                        const string_t * process_name,
                                        const string_t * process_instance,
                                        const string_t * entity_name,
                                        const string_t * entity_instance)
{
        if (!dst)
                return NULL;

        /* Clean up the destination, leftovers might be there ... */
        name_fini(dst);

        ASSERT(name_is_initialized(dst));

        /* Boolean shortcuits ... */
        if (string_dup_gfp(flags, process_name,     &dst->process_name)     ||
            string_dup_gfp(flags, process_instance, &dst->process_instance) ||
            string_dup_gfp(flags, entity_name,      &dst->entity_name)      ||
            string_dup_gfp(flags, entity_instance,  &dst->entity_instance)) {
                name_fini(dst);
                return NULL;
        }

        return dst;
}

struct name * name_init_from(struct name *    dst,
                             const string_t * process_name,
                             const string_t * process_instance,
                             const string_t * entity_name,
                             const string_t * entity_instance)
{
        return name_init_from_gfp(GFP_KERNEL,
                                  dst,
                                  process_name,
                                  process_instance,
                                  entity_name,
                                  entity_instance);
}
EXPORT_SYMBOL(name_init_from);

struct name * name_init_from_ni(struct name *    dst,
                                const string_t * process_name,
                                const string_t * process_instance,
                                const string_t * entity_name,
                                const string_t * entity_instance)
{
        return name_init_from_gfp(GFP_ATOMIC,
                                  dst,
                                  process_name,
                                  process_instance,
                                  entity_name,
                                  entity_instance);
}
EXPORT_SYMBOL(name_init_from_ni);

void name_fini(struct name * n)
{
        ASSERT(n);

        if (n->process_name) {
                rkfree(n->process_name);
                n->process_name = NULL;
        }
        if (n->process_instance) {
                rkfree(n->process_instance);
                n->process_instance = NULL;
        }
        if (n->entity_name) {
                rkfree(n->entity_name);
                n->entity_name = NULL;
        }
        if (n->entity_instance) {
                rkfree(n->entity_instance);
                n->entity_instance = NULL;
        }

        LOG_DBG("Name at %pK finalized successfully", n);
}
EXPORT_SYMBOL(name_fini);

void name_destroy(struct name * ptr)
{
        ASSERT(ptr);

        name_fini(ptr);

        ASSERT(name_is_initialized(ptr));

        rkfree(ptr);

        LOG_DBG("Name at %pK destroyed successfully", ptr);
}
EXPORT_SYMBOL(name_destroy);

int name_cpy(const struct name * src, struct name * dst)
{
        if (!src || !dst)
                return -1;

        LOG_DBG("Copying name %pK into %pK", src, dst);

        name_fini(dst);

        ASSERT(name_is_initialized(dst));

        /* We rely on short-boolean evaluation ... :-) */
        if (string_dup(src->process_name,     &dst->process_name)     ||
            string_dup(src->process_instance, &dst->process_instance) ||
            string_dup(src->entity_name,      &dst->entity_name)      ||
            string_dup(src->entity_instance,  &dst->entity_instance)) {
                name_fini(dst);
                return -1;
        }

        LOG_DBG("Name %pK copied successfully into %pK", src, dst);

        return 0;
}
EXPORT_SYMBOL(name_cpy);

struct name * name_dup(const struct name * src)
{
        struct name * tmp;

        if (!src)
                return NULL;

        tmp = name_create();
        if (!tmp)
                return NULL;
        if (name_cpy(src, tmp)) {
                name_destroy(tmp);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(name_dup);

#define NAME_CMP_FIELD(X, Y, FIELD)                                     \
        ((X->FIELD && Y->FIELD) ? string_cmp(X->FIELD, Y->FIELD) :      \
         ((!X->FIELD && !Y->FIELD) ? 0 : -1))

/* NOTE: RINA reference model says only process_name is mandatory */
bool name_is_ok(const struct name * n)
{ return (n && n->process_name && strlen(n->process_name)); }
EXPORT_SYMBOL(name_is_ok);

bool name_cmp(uint8_t             flags,
              const struct name * a,
              const struct name * b)
{
        if (a == b)
                return true;
        if (!a || !b)
                return false;

        ASSERT(a != b);
        ASSERT(a != NULL);
        ASSERT(b != NULL);

        if (flags & NAME_CMP_ALL)
                LOG_DBG("No flags, name comparison will be meaningless ...");

        if (flags & NAME_CMP_APN)
                if (NAME_CMP_FIELD(a, b, process_name))
                        return false;

        if (flags & NAME_CMP_API)
                if (NAME_CMP_FIELD(a, b, process_instance))
                        return false;

        if (flags & NAME_CMP_AEN)
                if (NAME_CMP_FIELD(a, b, entity_name))
                        return false;

        if (flags & NAME_CMP_AEI)
                if (NAME_CMP_FIELD(a, b, entity_instance))
                        return false;

        return true;
}
EXPORT_SYMBOL(name_cmp);

bool name_is_equal(const struct name * a,
                   const struct name * b)
{ return name_cmp(NAME_CMP_ALL, a, b); }
EXPORT_SYMBOL(name_is_equal);

#define DELIMITER "/"

static char * name_tostring_gfp(gfp_t               flags,
                                const struct name * n)
{
        char *       tmp;
        size_t       size;
        const char * none     = "<NONE>";
        size_t       none_len = strlen(none);

        if (!n)
                return NULL;

        size  = 0;

        size += (n->process_name                 ?
                 string_len(n->process_name)     : none_len);
        size += strlen(DELIMITER);

        size += (n->process_instance             ?
                 string_len(n->process_instance) : none_len);
        size += strlen(DELIMITER);

        size += (n->entity_name                  ?
                 string_len(n->entity_name)      : none_len);
        size += strlen(DELIMITER);

        size += (n->entity_instance              ?
                 string_len(n->entity_instance)  : none_len);
        size += strlen(DELIMITER);

        tmp = rkmalloc(size, flags);
        if (!tmp)
                return NULL;

        if (snprintf(tmp, size,
                     "%s%s%s%s%s%s%s",
                     (n->process_name     ? n->process_name     : none),
                     DELIMITER,
                     (n->process_instance ? n->process_instance : none),
                     DELIMITER,
                     (n->entity_name      ? n->entity_name      : none),
                     DELIMITER,
                     (n->entity_instance  ? n->entity_instance  : none)) !=
            size - 1) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

char * name_tostring(const struct name * n)
{ return name_tostring_gfp(GFP_KERNEL, n); }
EXPORT_SYMBOL(name_tostring);

char * name_tostring_ni(const struct name * n)
{ return name_tostring_gfp(GFP_ATOMIC, n); }
EXPORT_SYMBOL(name_tostring_ni);

static struct name * string_toname_gfp(gfp_t            flags,
                                       const string_t * input)
{
        struct name * name;

        string_t *    tmp1   = NULL;
        string_t *    tmp_pn = NULL;
        string_t *    tmp_pi = NULL;
        string_t *    tmp_en = NULL;
        string_t *    tmp_ei = NULL;

        if (input) {
                string_t * tmp2;

                string_dup_gfp(flags, input, &tmp1);
                if (!tmp1) {
                        return NULL;
                }
                tmp2 = tmp1;

                tmp_pn = strsep(&tmp2, DELIMITER);
                tmp_pi = strsep(&tmp2, DELIMITER);
                tmp_en = strsep(&tmp2, DELIMITER);
                tmp_ei = strsep(&tmp2, DELIMITER);
        }

        name = name_create_gfp(flags);
        if (!name) {
                if (tmp1) rkfree(tmp1);
                return NULL;
        }

        if (!name_init_from_gfp(flags, name, tmp_pn, tmp_pi, tmp_en, tmp_ei)) {
                name_destroy(name);
                if (tmp1) rkfree(tmp1);
                return NULL;
        }

        if (tmp1) rkfree(tmp1);

        return name;
}

struct name * string_toname(const string_t * input)
{ return string_toname_gfp(GFP_KERNEL, input); }
EXPORT_SYMBOL(string_toname);

struct name * string_toname_ni(const string_t * input)
{ return string_toname_gfp(GFP_ATOMIC, input); }
EXPORT_SYMBOL(string_toname_ni);

struct ipcp_config * ipcp_config_create(void)
{
        struct ipcp_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->entry = NULL;

        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

int ipcp_config_destroy(struct ipcp_config * cfg)
{
        if (!cfg)
                return -1;

        if (!cfg->entry)
                return -1;

        if (cfg->entry->name) rkfree(cfg->entry->name);

        if (cfg->entry->value) rkfree(cfg->entry->value);

        rkfree(cfg->entry);

        rkfree(cfg);

        return 0;
}

struct flow_spec * flow_spec_dup(const struct flow_spec * fspec)
{
        struct flow_spec * tmp;

        if (!fspec)
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        *tmp = *fspec;

        return tmp;
}
EXPORT_SYMBOL(flow_spec_dup);

struct efcp_config * efcp_config_create(void)
{
        struct efcp_config * tmp;
        struct dt_cons *     tmp_dt;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp_dt = rkzalloc(sizeof(*tmp_dt), GFP_KERNEL);
        if (!tmp_dt) {
                rkfree(tmp);
                return NULL;
        }
        tmp->dt_cons = tmp_dt;

        tmp->unknown_flow = policy_create();
        if (!tmp->unknown_flow) {
                rkfree(tmp_dt);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(efcp_config_create);

int efcp_config_destroy(struct efcp_config * efcp_config)
{
        if (efcp_config->dt_cons)
                rkfree(efcp_config->dt_cons);

        if (efcp_config->unknown_flow)
                policy_destroy(efcp_config->unknown_flow);

        rkfree(efcp_config);

        return 0;
}
EXPORT_SYMBOL(efcp_config_destroy);

struct rmt_config * rmt_config_create(void)
{
        struct rmt_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->pdu_forwarding = policy_create();
        if (!tmp->pdu_forwarding) {
                rkfree(tmp);
                return NULL;
        }

        tmp->q_monitor = policy_create();
        if (!tmp->q_monitor) {
        	rkfree(tmp->pdu_forwarding);
                rkfree(tmp);
                return NULL;
        }

        tmp->max_q = policy_create();
        if (!tmp->max_q) {
        	rkfree(tmp->q_monitor);
        	rkfree(tmp->pdu_forwarding);
                rkfree(tmp);
                return NULL;
        }

        tmp->scheduling = policy_create();
        if (!tmp->scheduling) {
        	rkfree(tmp->max_q);
        	rkfree(tmp->q_monitor);
        	rkfree(tmp->pdu_forwarding);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(rmt_config_create);

int rmt_config_destroy(struct rmt_config * rmt_config)
{
        if (rmt_config->pdu_forwarding)
                rkfree(rmt_config->pdu_forwarding);

        if (rmt_config->q_monitor)
                policy_destroy(rmt_config->q_monitor);

        if (rmt_config->max_q)
                rkfree(rmt_config->max_q);

        if (rmt_config->scheduling)
                policy_destroy(rmt_config->scheduling);

        rkfree(rmt_config);

        return 0;
}
EXPORT_SYMBOL(rmt_config_destroy);

struct dif_config * dif_config_create(void)
{
        struct dif_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->efcp_config = NULL;
        tmp->rmt_config = NULL;
        INIT_LIST_HEAD(&(tmp->ipcp_config_entries));

        return tmp;
}
EXPORT_SYMBOL(dif_config_create);

int dif_config_destroy(struct dif_config * dif_config)
{
        struct ipcp_config * pos, * nxt;

        if (!dif_config)
                return -1;

        list_for_each_entry_safe(pos, nxt,
                                 &dif_config->ipcp_config_entries,
                                 next) {
                list_del(&pos->next);
                ipcp_config_destroy(pos);
        }

        rkfree(dif_config);

        return 0;
}
EXPORT_SYMBOL(dif_config_destroy);

struct dif_info * dif_info_create(void)
{
        struct dif_info * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;
        tmp->dif_name = name_create();
        if (!tmp->dif_name) {
                rkfree(tmp);
                return NULL;
        }

        tmp->configuration = dif_config_create();
        if (!tmp->configuration) {
                name_destroy(tmp->dif_name);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(dif_info_create);

int dif_info_destroy(struct dif_info * dif_info)
{
        LOG_DBG("Destroying DIF-info");

        if (dif_info) {
                if (dif_info->dif_name) {
                        name_destroy(dif_info->dif_name);
                }

                if (dif_info->configuration) {
                        if (dif_config_destroy(dif_info->configuration))
                                return -1;
                }

                rkfree(dif_info->type);
                rkfree(dif_info);
        }

        return 0;
}
EXPORT_SYMBOL(dif_info_destroy);
