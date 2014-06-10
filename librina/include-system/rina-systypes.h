/*
 * RINA sys-types
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef RINA_SYS_TYPES_H
#define	RINA_SYS_TYPES_H

#ifdef __cplusplus

//#include <linux/types.h>

namespace rina {

typedef int	ipc_process_id_t;
typedef char string_t;
typedef unsigned int ipc_process_address_t;
typedef unsigned int port_id_t;

/* This structure represents raw data */
struct buffer_t {
	void * data;
	size_t size;
};

/* This structure represents a SDU is */
struct sdu_t {
	struct buffer_t * buffer;
};

/* This structure represents an SDU with a port-id */
struct sdu_wpi_t {
        struct sdu_t * sdu;
        port_id_t port_id;
};

struct name_t {
	/*
	 * The process_name identifies an application process within the
	 * application process namespace. This value is required, it
	 * cannot be NULL. This name has global scope (it is defined by
	 * the chain of IDD databases that are linked together), and is
	 * assigned by an authority that manages the namespace that
	 * particular application name belongs to.
         */
	string_t * process_name;

	/*
	 * The process_instance identifies a particular instance of the
	 * process. This value is optional, it may be NULL.
	 *
         */
	string_t * process_instance;

	/*
         * The entity_name identifies an application entity within the
	 * application process. This value is optional, it may be NULL.
         */
	string_t * entity_name;

	/*
         * The entity_name identifies a particular instance of an entity within
	 * the application process. This value is optional, it may be NULL.
         */
	string_t * entity_instance;
};

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

struct rmt_conf_t {
	/* To do, only a placeholder right now */
};

struct ipc_process_conf {
	/*
	 * Configuration of the kernel components of a normal IPC Process.
	 * Defines the struct for the kernel components of a fully RINA IPC
	 * Process.
 	 */

	/* The address of the IPC Process */
	ipc_process_address_t address;

	/* EFCP component configuration */
	struct efcp_conf_t *  efcp_config;

	/* RMT component configuration */
	struct rmt_conf_t *   rmt_config;
};
}

#endif

#endif
