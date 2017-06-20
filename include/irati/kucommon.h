/*
 * Common definitions for the IRATI implementation,
 * shared by kernel and user space components
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef IRATI_KUCOMMON_H
#define IRATI_KUCOMMON_H

/*
 * When compiling from userspace include <stdint.h>,
 * when compiling from kernelspace include <linux/types.h>
 */
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdio.h>
#include <stdint.h>
#include <asm/ioctl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IRATI_CTRLDEV_NAME	"/dev/irati-ctrl"
#define IRATI_IODEV_NAME	"/dev/irati"

#define PORT_ID_WRONG -1
#define CEP_ID_WRONG -1
#define ADDRESS_WRONG -1
#define QOS_ID_WRONG -1

typedef int32_t  port_id_t;
typedef int32_t  cep_id_t;
typedef int16_t  qos_id_t;
typedef uint32_t address_t;
typedef uint32_t seq_num_t;

typedef uint16_t      ipc_process_id_t;
typedef unsigned int  ipc_process_address_t;

/* We should get rid of the following definitions */
typedef uint          uint_t;
typedef uint          timeout_t;

struct uint_range {
        uint_t min;
        uint_t max;
};

typedef char string_t;

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
} __attribute__((packed));

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

#define IRATI_SUCC  0
#define IRATI_ERR   1

typedef uint32_t irati_msg_port_t;
typedef uint32_t irati_msg_seqn_t;
typedef uint16_t irati_msg_t;

/* All the possible messages begin like this. */
struct irati_msg_base {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;
} __attribute__((packed));

/* A simple response message layout that can be shared by many
 * different types. */
struct irati_msg_base_resp {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	uint8_t result;
} __attribute__((packed));

/* Some useful macros for casting. */
#define IRATI_MB(m) (struct irati_msg_base *)(m)
#define IRATI_MBR(m) (struct irati_msg_base_resp *)(m)

#ifdef __cplusplus
}
#endif

#endif /* IRATI_KUCOMMON_H */
