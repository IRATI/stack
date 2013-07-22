/*
 * (S|P) Data Unit
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

#ifndef RINA_DU_H
#define RINA_DU_H

#include "common.h"

/* This structure represents raw data */
struct buffer {
	char * data;
	size_t size;
};

struct pci {
	address_t  source;
	address_t  destination;
	pdu_type_t type;
	cep_id_t   source_cep_id;
	cep_id_t   dest_cep_id;
	qos_id_t   qos_id;
	seq_num_t  sequence_number;
};

struct pdu {
	struct pci *    pci;
	struct buffer * buffer;
};

struct sdu {
        struct buffer * buffer;
};

#endif
