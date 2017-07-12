/*
 * DTP CONFIG UTILS (Data Transfer Protocol)
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#define RINA_PREFIX "dtp-conf-utils"

#include "logs.h"
#include "dtp-conf-utils.h"
#include "utils.h"
#include "debug.h"
#include "policies.h"

int dtp_config_destroy(struct dtp_config * cfg)
{
        if (!cfg)
                return -1;
        if (cfg->dtp_ps)
                policy_destroy(cfg->dtp_ps);

        rkfree(cfg);

        return 0;
}
EXPORT_SYMBOL(dtp_config_destroy);

static struct dtp_config * dtp_config_create_gfp(gfp_t flags)
{
        struct dtp_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->dtp_ps = policy_create();
        if (!tmp->dtp_ps)
                goto clean;

        return tmp;

clean:
        dtp_config_destroy(tmp);
        return NULL;
}

struct dtp_config * dtp_config_create_ni(void)
{ return dtp_config_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(dtp_config_create_ni);

bool dtp_conf_dtcp_present(struct dtp_config * cfg)
{
        if (!cfg) return false;
        return cfg->dtcp_present;
}
EXPORT_SYMBOL(dtp_conf_dtcp_present);

int dtp_conf_dtcp_present_set(struct dtp_config * cfg, bool present)
{
        if (!cfg) return -1;
        return cfg->dtcp_present = present;
}
EXPORT_SYMBOL(dtp_conf_dtcp_present_set);

int dtp_conf_seq_num_ro_th(struct dtp_config * cfg)
{
        if (!cfg) return -1;
        return cfg->seq_num_ro_th;
}
EXPORT_SYMBOL(dtp_conf_seq_num_ro_th);

int dtp_conf_seq_num_ro_th_set(struct dtp_config * cfg, int seq_num_ro_th)
{
        if (!cfg) return -1;
        cfg->seq_num_ro_th = seq_num_ro_th;
        return 0;
}
EXPORT_SYMBOL(dtp_conf_seq_num_ro_th_set);

timeout_t dtp_conf_initial_a_timer(struct dtp_config * cfg)
{
        if (!cfg) return -1;
        return cfg->initial_a_timer;
}
EXPORT_SYMBOL(dtp_conf_initial_a_timer);

int dtp_conf_initial_a_timer_set(struct dtp_config * cfg,
                             timeout_t initial_a_timer)
{
        if (!cfg) return -1;
        cfg->initial_a_timer = initial_a_timer;
        return 0;
}
EXPORT_SYMBOL(dtp_conf_initial_a_timer_set);

bool dtp_conf_partial_del(struct dtp_config * cfg)
{
        if (!cfg) return false;
        return cfg->partial_delivery;
}
EXPORT_SYMBOL(dtp_conf_partial_del);

int dtp_conf_partial_del_set(struct dtp_config * cfg, bool partial_del)
{
        if (!cfg) return -1;
        cfg->partial_delivery = partial_del;
        return 0;
}
EXPORT_SYMBOL(dtp_conf_partial_del_set);

bool dtp_conf_incomplete_del(struct dtp_config * cfg)
{
        if (!cfg) return false;
        return cfg->incomplete_delivery;
}
EXPORT_SYMBOL(dtp_conf_incomplete_del);

int dtp_conf_incomplete_del_set(struct dtp_config * cfg, bool incomplete_del)
{
        if (!cfg) return -1;
        cfg->incomplete_delivery = incomplete_del;
        return 0;
}
EXPORT_SYMBOL(dtp_conf_incomplete_del_set);

bool dtp_conf_in_order_del(struct dtp_config * cfg)
{
        if (!cfg) return false;
        return cfg->in_order_delivery;
}
EXPORT_SYMBOL(dtp_conf_in_order_del);

int dtp_conf_in_order_del_set(struct dtp_config * cfg, bool in_order_del)
{
        if (!cfg) return -1;
        cfg->in_order_delivery = in_order_del;
        return 0;
}
EXPORT_SYMBOL(dtp_conf_in_order_del_set);

seq_num_t dtp_conf_max_sdu_gap(struct dtp_config * cfg)
{
        if (!cfg) return -1;
        return cfg->max_sdu_gap;
}
EXPORT_SYMBOL(dtp_conf_max_sdu_gap);

int dtp_conf_max_sdu_gap_set(struct dtp_config * cfg, seq_num_t max_sdu_gap)
{
        if (!cfg) return -1;
        cfg->max_sdu_gap = max_sdu_gap;
        return 0;
}
EXPORT_SYMBOL(dtp_conf_max_sdu_gap_set);

struct policy * dtp_conf_ps_get(struct dtp_config * cfg)
{
        if (!cfg) return NULL;
        return cfg->dtp_ps;
}
EXPORT_SYMBOL(dtp_conf_ps_get);

int dtp_conf_ps_set(struct dtp_config * cfg,
               struct policy *     ps)
{
        if (!cfg || ps) return -1;
        cfg->dtp_ps = ps;
        return 0;
}
EXPORT_SYMBOL(dtp_conf_ps_set);
