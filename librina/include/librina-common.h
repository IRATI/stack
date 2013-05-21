/*
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

/*
 * This library provides definitions common to all the other RINA
 * related libraries presented in this document. Common
 * functionalities shared among framework components (i.e. applications,
 * daemons and libraries) might be made available from this library
 * as well.
 */

#ifndef LIBRINA_COMMON_H
#define LIBRINA_COMMON_H

#include <stdint.h>
#include <stddef.h>

/* A generic UTF-8 encoded string */
typedef unsigned char string_t;

/* The local identifier of a flow, unique within a system */
typedef uint16_t port_id_t;

/* The value should be interpreted as false if the value is 0 or true
 * otherwise. This typedef should be interpreted as ISO C99 bool/_Bool
 * and could be replaced by the inclusion of stdbool.h where/when
 * possible)
 */
typedef int bool_t;

/* This structure represents raw data */
typedef struct {
	char * data;
	size_t size;
} buffer_t;

/* An SDU */
typedef struct {
	buffer_t * buffer;
} sdu_t;

/*
 * This structure represents a range of integer values
 */
typedef struct {
	/* Minimum value */
	uint32_t min_value;

	/* Maximum value */
	uint32_t max_value;
} uint_range_t;

/* The address of an IPC Process */
typedef uint32_t   address_t;

/* The id of a QoS cube */
typedef uint32_t   qos_id_t;

/* A connection-endpoint id */
typedef uint32_t   cep_id_t;

/* A sequence number */
typedef uint32_t   seq_num_t;

/* A unique identifier for a local IPC Process */
typedef int        ipc_process_id_t;

/* A NULL-terminated string describing why an operation was unsuccessful */
typedef string_t   response_reason_t;

#endif
