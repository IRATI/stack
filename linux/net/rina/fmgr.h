/*
 * FMGR (Flows Manager)
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

#ifndef RINA_FMGR_H
#define RINA_FMGR_H

#include "common.h"
#include "du.h"

#if 0
int   kfa_init(void);
int   kfa_fini(void);

fid_t kfa_create(void);
int   kfa_bind(fid_t, pid_t);
fid_t kfa_undind(pid_t);
int   kfa_destroy(fid_t);
#endif

struct fmgr;

/*
 * Instance management related functions
 */
struct fmgr * fmgr_create(void);
int           fmgr_destroy(struct fmgr * instance);

/*
 * Each flow will have its own EFCP and RMT instance.
 *
 * EFCP will have its DTP and DTCP parts, loosely coupled by the DTSV:
 *
 *   The DTP part will perform fragmentation, reassembly, sequencing,
 *   concatenation and separation of SDUs.
 *
 *   The DTCP part will perform transmission, retransmission and flow control
 *
 */

/*
 * NOTE: is_port_id_ok() and is_flow_id_ok() must be used to detect error
 *       conditions. DO NOT ASSUME port-id or flow-id < 0 as an error condition
 *       AND ALWAYS use is_port_id_ok() and is_flow_id_ok() functions
 */

struct ipcp_flow;

/* Creates a flow (the flow is initially unbound from a port) */
struct ipcp_flow * fmgr_flow_create(struct fmgr * mgr);

/* Destroys a flow (either already bound to a port or not) */
int                fmgr_flow_destroy(struct fmgr *      mgr,
                                     struct ipcp_flow * flow);

/* (Re-)Binds the pointed flow to the port 'pid' */
int                fmgr_flow_bind(struct fmgr *      mgr,
                                  struct ipcp_flow * flow,
                                  port_id_t          pid);
/* Unbinds a flow from a port (if any) */
int                fmgr_flow_unbind(struct fmgr *      mgr,
                                    struct ipcp_flow * flow);

/* Returns the port the flow is bound to (if any) */
port_id_t          fmgr_flow2port(struct fmgr *      mgr,
                                  struct ipcp_flow * flow);

/* Returns the flow bound to the port 'pid' */
struct ipcp_flow * fmgr_port2flow(struct fmgr * mgr,
                                  port_id_t     pid);

/*
 * Once the flow is bound to a port, we can post SDUs ...
 */

/* ... directly, if we "know" the flow */
int                fmgr_flow_sdu_post(struct fmgr *      mgr,
                                      struct ipcp_flow * flow,
                                      struct sdu *       sdu);

/* ... indirectly if we don't know the flow (hence using its port-id) */
int                fmgr_flow_sdu_post_by_port(struct fmgr * mgr,
                                              port_id_t     pid,
                                              struct sdu *  sdu);

#endif
