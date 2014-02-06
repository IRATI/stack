/*
 * DT (Data Transfer)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef RINA_DT_H
#define RINA_DT_H

#include <linux/kernel.h>

#include "common.h"

struct dtp;
struct dtcp;
struct dt;

struct dt *   dt_create(void);
int           dt_destroy(struct dt * sv);

int           dt_dtp_bind(struct dt *  dt, struct dtp *  dtp);
int           dt_dtcp_bind(struct dt * dt, struct dtcp * dtp);

struct dtp *  dt_dtp_take(struct dt * sv);
void          dt_dtp_release(struct dt * sv);

struct dtcp * dt_dtcp_take(struct dt * sv);
void          dt_dtcp_release(struct dt * sv);

#endif
