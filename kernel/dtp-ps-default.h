/*
 * Default policy set for DTP
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef DTP_PS_DEFAULT_H
#define DTP_PS_DEFAULT_H

#include "dtp-ps.h"
#include "dtp.h"

struct ps_base * dtp_ps_default_create(struct rina_component *component);
void dtp_ps_default_destroy(struct ps_base *bps);
int default_transmission_control(struct dtp_ps * ps, struct du * du);

int default_closed_window(struct dtp_ps * ps, struct du * du);

int default_snd_flow_control_overrun(struct dtp_ps * ps,
                                 struct du *    du);

int default_initial_sequence_number(struct dtp_ps * ps);

int default_receiver_inactivity_timer(struct dtp_ps * ps);

int default_sender_inactivity_timer(struct dtp_ps * ps);

#endif /* DTP_PS_DEFAULT_H */
