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

/* FIXME: Instances or State-Vectors ? */
struct dtp_sv;
struct dtcp_sv;
struct dt_sv;

struct dt_sv *   dtsv_create(void);
int              dtsv_destroy(struct dt_sv * sv);

int              dtsv_dtp_bind(struct dt_sv *  dt, struct dtp_sv *  dtp);
int              dtsv_dtcp_bind(struct dt_sv * dt, struct dtcp_sv * dtp);

struct dtp_sv *  dtsv_dtp_take(struct dt_sv * sv);
void             dtsv_dtp_release(struct dt_sv * sv);

struct dtcp_sv * dtsv_dtcp_take(struct dt_sv * sv);
void             dtsv_dtcp_release(struct dt_sv * sv);

#endif
