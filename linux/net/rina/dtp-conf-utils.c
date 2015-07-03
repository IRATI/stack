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
#include "utils.h"
#include "debug.h"
#include "dtcp-utils.h"
#include "policies.h"

struct dtp_config {
        /* FIXME The following three "policies" are unused, useless
         * and duplicated - they already exist as function pointers
         * (dtp-ps.h). They have to be removed here, and therefore
         * from netlink (kernespace+userspace) and from librina. */
        struct policy *      initial_sequence_number;
        struct policy *      receiver_inactivity_timer;
        struct policy *      sender_inactivity_timer;
        /* Sequence number rollover threshold */
        int                  seq_num_ro_th;
        timeout_t            initial_a_timer;
        bool                 partial_delivery;
        bool                 incomplete_delivery;
        bool                 in_order_delivery;
        seq_num_t            max_sdu_gap;
};

struct policy * dtp_initial_sequence_number(struct dtp_config * cfg)
{
        if (!cfg) return NULL;
        return cfg->initial_sequence_number;
}
EXPORT_SYMBOL(dtp_initial_sequence_number);

int dtp_initial_sequence_number_set(struct dtp_config * cfg,
                                    struct policy *     p)
{
        if (!cfg || p) return -1;
        cfg->initial_sequence_number = p;
        return 0;
}
EXPORT_SYMBOL(dtp_initial_sequence_number_set);

struct policy * dtp_receiver_inactivity_timer(struct dtp_config * cfg)
{
        if (!cfg) return NULL;
        return cfg->receiver_inactivity_timer;
}
EXPORT_SYMBOL(dtp_receiver_inactivity_timer);

int dtp_receiver_inactivity_timer_set(struct dtp_config * cfg,
                                      struct policy *     p)
{
        if (!cfg || p) return -1;
        cfg->receiver_inactivity_timer = p;
        return 0;
}
EXPORT_SYMBOL(dtp_receiver_inactivity_timer_set);

struct policy * dtp_sender_inactivity_timer(struct dtp_config * cfg)
{
        if (!cfg) return NULL;
        return cfg->sender_inactivity_timer;
}
EXPORT_SYMBOL(dtp_receiver_inactivity_timer);

int dtp_sender_inactivity_timer_set(struct dtp_config * cfg,
                                struct policy *     p)
{
        if (!cfg || p) return -1;
        cfg->sender_inactivity_timer = p;
        return 0;
}
EXPORT_SYMBOL(dtp_receiver_inactivity_timer_set);


int dtp_seq_num_ro_th(struct dtp_config * cfg)
{
        if (!cfg) return -1;
        return cfg->seq_num_ro_th;
}
EXPORT_SYMBOL(dtp_seq_num_ro_th);

int dtp_seq_num_ro_th_set(struct dtp_config * cfg, int seq_num_ro_th)
{
        if (!cfg) return -1;
        cfg->seq_num_ro_th = seq_num_ro_th;
        return 0;
}
EXPORT_SYMBOL(dtp_seq_num_ro_th_set);

timeout_t dtp_initial_a_timer(struct dtp_config * cfg)
{
        if (!cfg) return -1;
        return cfg->initial_a_timer;
}
EXPORT_SYMBOL(dtp_initial_a_timer);

int dtp_initial_a_timer_set(struct dtp_config * cfg,
                             timeout_t initial_a_timer)
{
        if (!cfg) return -1;
        cfg->initial_a_timer = initial_a_timer;
        return 0;
}
EXPORT_SYMBOL(dtp_initial_a_timer_set);

bool dtp_partial_del(struct dtp_config * cfg)
{
        if (!cfg) return -1;
        return cfg->partial_delivery;
}
EXPORT_SYMBOL(dtp_partial_del);

int dtp_partial_del_set(struct dtp_config * cfg, bool partial_del)
{
        if (!cfg) return -1;
        cfg->partial_delivery = partial_del;
        return 0;
}
EXPORT_SYMBOL(dtp_partial_del_set);

bool dtp_incomplete_del(struct dtp_config * cfg)
{
        if (!cfg) return -1;
        return cfg->incomplete_delivery;
}
EXPORT_SYMBOL(dtp_incomplete_del);

int dtp_incomplete_del_set(struct dtp_config * cfg, bool incomplete_del)
{
        if (!cfg) return -1;
        cfg->incomplete_delivery = incomplete_del;
        return 0;
}
EXPORT_SYMBOL(dtp_incomplete_del_set);

seq_num_t dtp_max_sdu_gap(struct dtp_config * cfg)
{
        if (!cfg) return -1;
        return cfg->max_sdu_gap;
}
EXPORT_SYMBOL(dtp_max_sdu_gap);

int dtp_max_sdu_gap_set(struct dtp_config * cfg, seq_num_t max_sdu_gap)
{
        if (!cfg) return -1;
        cfg->max_sdu_gap = max_sdu_gap;
        return 0;
}
EXPORT_SYMBOL(dtp_max_sdu_gap_set);
