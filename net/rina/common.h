/*
 *  Common definition placeholder
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net> 
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
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

#ifndef RINA_COMMON_H
#define RINA_COMMON_H


#include <linux/types.h>

typedef unsigned int	ipc_process_address_t;
typedef uint16_t	port_id_t;
/*typedef int		ipc_process_id_t;*/
typedef unsigned char	utf8_t;
typedef unsigned char	string_t;
typedef uint		uint_t;
typedef uint		response_reason_t;
typedef uint            cep_id_t;
typedef uint16_t        address_t;
typedef uint            seq_num_t;
/* FIXME : The PDU type should be defined correctly in the near future */
typedef uint            pdu_type_t;
/* FIXME : The qos_id_t should be defined correctly in the near future */
typedef uint            qos_id_t;

/*
 * The value should be interpreted as false if the value is 0 or true
 * otherwise.
 * This typedef should be interpreted as ISO C99 bool/_Bool and could be
 * replaced by the inclusion of stdbool.h where/when possible.
 *
 */
typedef int bool_t;

/* This structure represents raw data */
struct buffer_t {
	char * data;
	size_t size;
};

/* This structure represents a SDU is */
struct sdu_t {
        struct buffer_t * buffer;
};

struct uint_range_t {
	/* Minimum value */
	uint_t min_value;

	/* Maximum value */
	uint_t max_value;
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

struct flow_spec_t {
	/* This structure defines the characteristics of a flow */

	/* Average bandwidth in bytes/s */
	struct uint_range_t *average_bandwidth;
	/* Average bandwidth in SDUs/s */
	struct uint_range_t *average_sdu_bandwidth;
	/* In milliseconds */
	struct uint_range_t *peak_bandwidth_duration;
	/* In milliseconds */
	struct uint_range_t *peak_sdu_bandwidth_duration;

	/* A value of 0 indicates 'do not care' */
	double               undetected_bit_error_rate;
	/* Indicates if partial delivery of SDUs is allowed or not */
	bool_t               partial_delivery;
	/* Indicates if SDUs have to be delivered in order */
	bool_t               ordered_delivery;
	/*
	 * Indicates the maximum gap allowed among SDUs, a gap of N
	 * SDUs is considered the same as all SDUs delivered.
	 * A value of -1 indicates 'Any'
	 */
	int                  max_allowable_gap;
	/*
	 * In milliseconds, indicates the maximum delay allowed in this
	 * flow. A value of 0 indicates 'do not care'
	 */
	uint_t               delay;
	/*
	 * In milliseconds, indicates the maximum jitter allowed
	 * in this flow. A value of 0 indicates 'do not care'
	 */
	uint_t               jitter;
	/*
	 * The maximum SDU size for the flow. May influence the choice
	 * of the DIF where the flow will be created.
	 */
	uint_t              max_sdu_size;
};

struct pci_t {
	address_t source;
	address_t destination;
	pdu_type_t type;
	cep_id_t source_cep_id;
	cep_id_t dest_cep_id;
	qos_id_t qos_id;
	seq_num_t sequence_number;
};

struct pdu_t {
	struct pci_t    *pci;
	struct buffer_t *buffer;
};

struct connection_t {
	/* This structure defines an EFCP connection */

	/* The port_id this connection is bound to */
	port_id_t port_id;
	
	/*
	 * The address of the IPC Process that is the source of this
	 * connection
	 */
	address_t source_address;
	
	/*
	 * The address of the IPC Process that is the destination of
	 * this connection
	 */
	address_t destination_address;
	
	/* The source connection endpoint Id */
	cep_id_t source_cep_id;
	
	/* The destination connection endpoint id */
	cep_id_t dest_cep_id;
	
	/* The QoS id */
	qos_id_t qos_id;
	
	/* FIXME : policy type remains undefined */
	/* The list of policies associated with this connection */
	/* policy_t ** policies; */
};

#endif
