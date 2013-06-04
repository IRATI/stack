/*
 * RMT (Relaying and Multiplexing Task)
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

#define RINA_PREFIX "rmt"

#include "logs.h"
#include "rmt.h"
#include "pdufwdt.h"

int rmt_init(void)
{
        LOG_FBEGN;

        if (pdufwdt_init()) {
                LOG_FEXIT;

                return 1;
        }

        LOG_FEXIT;

        return 0;
}

void rmt_exit(void)
{
        LOG_FBEGN;

        pdufwdt_exit();

        LOG_FEXIT;
}
