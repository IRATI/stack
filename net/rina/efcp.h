/*
 *  EFCP (Error and Flow Control Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef RINA_EFCP_H
#define RINA_EFCP_H

struct efcp_conf_t {

	/* Length of the address fields of the PCI */
	int address_length;

	/* Length of the port_id fields of the PCI */
	int port_id_length;

	/* Length of the cep_id fields of the PCI */
	int cep_id_length;

	/* Length of the qos_id field of the PCI */
	int qos_id_length;

	/* Length of the length field of the PCI */
	int length_length;

	/* Length of the sequence number fields of the PCI */
	int seq_number_length;
};

struct efcp_t {
	/* 
	 * FIXME hash_table must be added
	 * HASH_TABLE(efcp_components_per_ipc_process, ipc_process_id_t, 
	 * efcp_ipc_t *);
	 */
	
};

struct efcp_ipc_t {
	/* EFCP component configuration */
	efcp_conf_t efcp_config;

	/* FIXME: This hash holds the EFCP instances of a single IPC Process 
	 * HASH_TABLE(efcp_instances, cep_id_t, efcp_instance_t *);
	 */
};

int  efcp_init(void);
void efcp_exit(void);

#endif
