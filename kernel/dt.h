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

#include "efcp-str.h"

/*
 * FIXME: The DT instance would represent the DTP/DTCP couple. It has to land
 *        on EFCP. DTP, DTCP instances have to be removed from there
 */
struct dt *   dt_create(void);
int           dt_destroy(struct dt * dt);

int           dt_sv_init(struct dt * instance,
                         uint_t      mfps,
                         uint_t      mfss,
                         u_int32_t   mpl,
                         timeout_t   a,
                         timeout_t   r,
                         timeout_t   tr);

#endif
