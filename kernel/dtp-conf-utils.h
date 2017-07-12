/*
 * DTP CONFIG UTILS (Data Transfer Protocol);
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option); any later version.
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

#ifndef RINA_DTP_CONF_UTILS_H
#define RINA_DTP_CONF_UTILS_H

#include "common.h"

struct dtp_config * dtp_config_create_ni(void);
int                 dtp_config_destroy(struct dtp_config * cfg);

bool            dtp_conf_dtcp_present(struct dtp_config * cfg);
int             dtp_conf_dtcp_present_set(struct dtp_config * cfg, bool present);
int             dtp_conf_seq_num_ro_th(struct dtp_config * cfg);
int             dtp_conf_seq_num_ro_th_set(struct dtp_config * cfg, int seq_num_ro_th);
timeout_t       dtp_conf_initial_a_timer(struct dtp_config * cfg);
int             dtp_conf_initial_a_timer_set(struct dtp_config * cfg,
                                            timeout_t initial_a_timer);
bool            dtp_conf_partial_del(struct dtp_config * cfg);
int             dtp_conf_partial_del_set(struct dtp_config * cfg, bool partial_del);
bool            dtp_conf_incomplete_del(struct dtp_config * cfg);
int             dtp_conf_incomplete_del_set(struct dtp_config * cfg, bool incomplete_del);
bool            dtp_conf_in_order_del(struct dtp_config * cfg);
int             dtp_conf_in_order_del_set(struct dtp_config * cfg, bool in_order_del);
seq_num_t       dtp_conf_max_sdu_gap(struct dtp_config * cfg);
int             dtp_conf_max_sdu_gap_set(struct dtp_config * cfg, seq_num_t max_sdu_gap);
struct policy * dtp_conf_ps_get(struct dtp_config * cfg);
int             dtp_conf_ps_set(struct dtp_config * cfg,
                           struct policy *     ps);

#endif
