/*
 * Test interface for VMPI.
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#ifndef __VMPI_TEST_H__
#define __VMPI_TEST_H__

/* Enable guest-side test interface. */
#define VMPI_TEST

#ifdef VMPI_TEST
int vmpi_test_init(void *, bool deferred);
void vmpi_test_fini(bool deferred);
#endif  /* VMPI_TEST */

#endif  /* __VMPI_TEST_H__ */
