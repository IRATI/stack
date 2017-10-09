/*
 * Default policy set for SDUP Error check
 *
 *    Ondrej Lichtner <ilichtner@fit.vutbr.cz>
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

#ifndef SDUP_ERRC_PS_DEFAULT_H
#define SDUP_ERRC_PS_DEFAULT_H

#include "sdup-errc-ps.h"

int default_sdup_add_error_check_policy(struct sdup_errc_ps * ps,
					struct du * du);

int default_sdup_check_error_check_policy(struct sdup_errc_ps * ps,
					  struct du * du);

struct ps_base * sdup_errc_ps_default_create(struct rina_component * component);

void sdup_errc_ps_default_destroy(struct ps_base * bps);

#endif
