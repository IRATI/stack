/*
 *  POSIX-like RINA API
 *
 *    Vincenzo Maffione   <v.maffione@nextworks.it>
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

#ifndef __RINA_API_H__
#define __RINA_API_H__

#ifdef __cplusplus
extern "C" {
#endif

struct rina_flow_spec {
    uint64_t max_sdu_gap;       /* in SDUs */
    uint64_t avg_bandwidth;     /* in bits per second */
    uint32_t max_delay;         /* in microseconds */
    uint32_t max_jitter;        /* in microseconds */
    uint8_t in_order_delivery;  /* boolean */
};

int rina_open(void);

int rina_register(int fd, const char *dif_name, const char *local_appl);

int rina_unregister(int fd, const char *dif_name, const char *local_appl);

int rina_flow_accept(int fd, char **remote_appl);

int rina_flow_alloc(const char *dif_name, const char *local_appl,
              const char *remote_appl, const struct rina_flow_spec *flowspec);

void rina_flow_spec_default(struct rina_flow_spec *spec);

#ifdef __cplusplus
}
#endif

#endif  /* __RINA_API_H__ */

