/*
 *  C wrapper for librina
 *
 *    Sander Vrijders   <sander.vrijders@intec.ugent.be>
 *    Dimitri Staessens <dimitri.staessens@intec.ugent.be>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 */

#ifndef LIBRINA_C_H
#define LIBRINA_C_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <unistd.h>

#define FLOW_F_SETFL    00000002
#define FLOW_O_NONBLOCK 00004000

// TODO: Extend QoS spec
struct qos_spec {
        char * qos_name;
        char * dif_name;

        uint32_t delay;
        uint32_t jitter;
};

int     ap_init(char * ap_name);
void    ap_fini(void);

/* Returns file descriptor */
int     ap_reg(char ** difs, size_t difs_size);
int     ap_unreg(char ** difs, size_t difs_size);

/* Returns file descriptor (> 0) and client name(s) */
int     flow_accept(int fd, char ** ap_name, char ** ae_name);
int     flow_alloc_resp(int fd, int result);

/* Returns file descriptor */
int     flow_alloc(char * dst_name, char * src_ae_name, struct qos_spec * qos);

/* If flow is accepted returns a value > 0 */
int     flow_alloc_res(int fd);
int     flow_dealloc(int fd);

/* Wraps around fnctl */
int     flow_cntl(int fd, int cmd, int oflags);
ssize_t flow_write(int fd, void * buf, size_t count);
ssize_t flow_read(int fd, void * buf, size_t count);

#ifdef __cplusplus
}
#endif

#endif //LIBRINA_C_H
