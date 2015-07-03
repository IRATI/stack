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

struct dtp_config;

struct policy * dtp_initial_sequence_number(struct dtp_config * cfg);
int             dtp_initial_sequence_number_set(struct dtp_config * cfg,
struct policy * dtp_receiver_inactivity_timer(struct dtp_config * cfg);
int             dtp_receiver_inactivity_timer_set(struct dtp_config * cfg,
                                                  struct policy *     p);
struct policy * dtp_sender_inactivity_timer(struct dtp_config * cfg);

int             dtp_sender_inactivity_timer_set(struct dtp_config * cfg,
                                                struct policy *     p);
int             dtp_seq_num_ro_th(struct dtp_config * cfg);
int             dtp_seq_num_ro_th_set(struct dtp_config * cfg, int seq_num_ro_th);
timeout_t       dtp_initial_a_timer(struct dtp_config * cfg);
int             dtp_initial_a_timer_set(struct dtp_config * cfg,
                             timeout_t initial_a_timer);
bool            dtp_partial_del(struct dtp_config * cfg);
int             dtp_partial_del_set(struct dtp_config * cfg, bool partial_del);
bool            dtp_incomplete_del(struct dtp_config * cfg);
int             dtp_incomplete_del_set(struct dtp_config * cfg, bool incomplete_del);
seq_num_t       dtp_max_sdu_gap(struct dtp_config * cfg);
int             dtp_max_sdu_gap_set(struct dtp_config * cfg, seq_num_t max_sdu_gap);
