/*
 * Data Unit
 *
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

#ifndef RINA_DU_H
#define RINA_DU_H

#include <linux/skbuff.h>

/* NOTE: This is just to be included by [pdu,sdu,pci].[ch] */

struct pci {
	unsigned char *h; /* do not move from 1st position */
	size_t len;
};

struct du {
	struct efcp_config *cfg;
	struct pci pci;
	void *sdup_head; /* opaque used by SDU protection policy (TTL)*/
	void *sdup_tail; /* opaque used by SDU protection policy (error check) */
	struct sk_buff *skb;
};

#endif
