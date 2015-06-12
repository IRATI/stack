/*
 * Common policy set for PFT
 *
 *    Miquel Tarzan <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#include "pft-ps.h"

int common_next_hop(struct pft_ps * ps,
                    struct pci *    pci,
                    port_id_t **    ports,
                    size_t *        count);

int pft_ps_common_set_policy_set_param(struct ps_base * bps,
                                       const char *     name,
                                       const char *     value);
