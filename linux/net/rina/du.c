/*
 * K-IPCM (Kernel-IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#define RINA_PREFIX "du"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"

struct pdu * pdu_create(void)
{
        struct pdu * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->pci    = NULL;
        tmp->buffer = NULL;

        return tmp;
}

int pdu_destroy(struct pdu * p)
{
        ASSERT(p);

        if (p->pci)
                rkfree(p->pci);
        if (p->buffer)
                rkfree(p->buffer);
        rkfree(p);

        return 0;
}

int sdu_is_ok(struct sdu * sdu)
{
        ASSERT(sdu);

        if (!sdu->buffer)
                return 0;

        /* FIXME: More checks expected here ... */

        return 1;
}

