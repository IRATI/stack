/*
 * RINA Bitmaps
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

#include <linux/export.h>

#define RINA_PREFIX "rbmp"

#include "logs.h"
#include "debug.h"

struct rbmp {
        int empty;
};

struct rbmp * rbmp_create(int offset)
{ return NULL; }
EXPORT_SYMBOL(rbmp_create);

int rbmp_destroy(struct rbmp * b)
{ return -1; }
EXPORT_SYMBOL(rbmp_destroy);

int rbmp_allocate(struct rbmp * b)
{ return -1; }
EXPORT_SYMBOL(rbmp_allocate);

int rbmp_release(struct rbmp * b,
                 int           id)
{ return -1; }
EXPORT_SYMBOL(rbmp_release);
