/*
 * Common definition placeholder
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#include "rds/rstr.h"


/* definition of flow options */
#define FLOW_O_NONBLOCK 00004000 /* use same value as fcntl.h O_NONBLOCK */
#define FLOW_O_DEFAULT  00000000 /* default flow option */

/* definition of flow commands */
#define FLOW_F_GETFL    3        /* get flow->options, same value as F_GETFL */
#define FLOW_F_SETFL    4        /* set flow->options, same value as F_SETFL */

/* flow options, such as blocking I/O behavior */
typedef uint         flow_opts_t;

/* FIXME: Shouldn't we keep contrained to int32 ids ? */
typedef int           port_id_t;

/* ALWAYS use this function to check if the id looks good */
bool      is_port_id_ok(port_id_t id);

/* ALWAYS use this function to get a bad id */
port_id_t port_id_bad(void);

typedef int           cep_id_t;

/* ALWAYS use this function to check if the id looks good */
bool     is_cep_id_ok(cep_id_t id);

/* ALWAYS use this function to get a bad id */
cep_id_t cep_id_bad(void);

typedef uint16_t      ipc_process_id_t;
typedef unsigned int  ipc_process_address_t;

/* We should get rid of the following definitions */
typedef uint          uint_t;

/* FIXME: Is this address_t definition correct ??? */
typedef uint32_t      address_t;

bool      is_address_ok(address_t address);
address_t address_bad(void);

typedef uint          timeout_t;
typedef uint          seq_num_t;

typedef int qos_id_t;

/* ALWAYS use this function to check if the id looks good */
bool is_qos_id_ok(qos_id_t id);

/* ALWAYS use this function to get a bad id */
qos_id_t qos_id_bad(void);

struct uint_range {
        uint_t min;
        uint_t max;
};

struct name {
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

struct flow_spec {
        /* This structure defines the characteristics of a flow */

        /* Average bandwidth in bytes/s */
        uint_t average_bandwidth;
        /* Average bandwidth in SDUs/s */
        uint_t average_sdu_bandwidth;
        /* In milliseconds */
        uint_t peak_bandwidth_duration;
        /* In milliseconds */
        uint_t peak_sdu_bandwidth_duration;

        /* A value of 0 indicates 'do not care' */
        /* FIXME: This uint_t has to be transformed back to double */
        uint_t undetected_bit_error_rate;
        /* Indicates if partial delivery of SDUs is allowed or not */
        bool   partial_delivery;
        /* Indicates if SDUs have to be delivered in order */
        bool   ordered_delivery;
        /*
         * Indicates the maximum gap allowed among SDUs, a gap of N
         * SDUs is considered the same as all SDUs delivered.
         * A value of -1 indicates 'Any'
         */
        int    max_allowable_gap;
        /*
         * In milliseconds, indicates the maximum delay allowed in this
         * flow. A value of 0 indicates 'do not care'
         */
        uint_t delay;
        /*
         * In milliseconds, indicates the maximum jitter allowed
         * in this flow. A value of 0 indicates 'do not care'
         */
        uint_t jitter;
        /*
         * The maximum SDU size for the flow. May influence the choice
         * of the DIF where the flow will be created.
         */
        uint_t max_sdu_size;
};

/* FIXME: Move RNL related types to RNL header(s) */

typedef char          regex_t; /* FIXME: must be a string */

/* FIXME: needed for nl api */
enum rib_object_class_t {
        EMPTY,
};

struct rib_object_entry {
        string_t *        class;
        string_t *        name;
        long unsigned int instance;
        string_t *	  display_value;
        struct list_head  next;
};

struct port_id_altlist {
	port_id_t *		ports;
	size_t			num_ports;
	struct list_head	next;
};

struct mod_pff_entry {
        address_t        fwd_info; /* dest_addr, neighbor_addr, circuit-id */
        qos_id_t         qos_id;
	struct list_head port_id_altlists;
        struct list_head next;
};
#endif
