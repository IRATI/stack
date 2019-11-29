/*
 * KIPCM (Kernel IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_KIPCM_H
#define RINA_KIPCM_H

#include "common.h"
#include "ipcp-instances.h"
#include "ipcp-factories.h"
#include "du.h"
#include "ctrldev.h"
#include "kfa.h"
#include "rds/robjects.h"

struct kipcm;
extern struct kipcm * default_kipcm;
/*
 * The following functions represent the KIPCM northbound interface
 */

int kipcm_init(struct robject * parent);
int kipcm_fini(struct kipcm * kipcm);

/*
 * NOTE: factory_name must be the string published by the choosen IPC
 *       factory (module). if type is NULL a default factory will be assumed
 */
int            kipcm_ipc_create(struct kipcm *      kipcm,
                                const struct name * name,
                                ipc_process_id_t    id,
				uint_t		    us_nl_port,
                                const char *        factory_name);
int            kipcm_ipc_destroy(struct kipcm *   kipcm,
                                 ipc_process_id_t id);

/* If successful: takes the ownership of the DU */
int            kipcm_du_write(struct kipcm * kipcm,
                              port_id_t      id,
			      const char __user * buffer,
			      struct iov_iter * iov,
			      ssize_t size,
                              bool blocking);

/* If the flow is deallocated it returns 0 (EOF), otherwise
 * it may report an error with a negative value or return
 * the number of bytes read (positive value)
 */
int            kipcm_du_read(struct kipcm * kipcm,
                             port_id_t      id,
                             struct du **   du,
			     size_t         size,
                             bool           blocking);

/* If successful: takes the ownership of the SDU */
int            kipcm_mgmt_du_write(struct kipcm *   kipcm,
                                   ipc_process_id_t id,
	                           port_id_t      port_id,
	                           struct du *   du);
port_id_t      kipcm_flow_create(struct kipcm *   kipcm,
				 ipc_process_id_t ipc_id,
				 bool		  msg_boundaries,
				 struct name *    process_name);

int            kipcm_flow_destroy(struct kipcm *   kipcm,
				  ipc_process_id_t ipc_id,
				  port_id_t        port_id);
/*
 * The following functions represent the KIPCM southbound interface
 */

/*
 * FIXME: This is a core "accessor", to be removed ASAP. It's currently here
 *        in the meanwhile we find the best way to settle the component in its
 *        final position.
 *
 * NOTE: The KFA lifetime is "contained" into the KIPCM one; if the KIPCM is
 *       alive and running, its KFA will also be. For the time being, IPCPs
 *       (shims as well the normal-ipc) are allowed to store the kfa instance
 *       returned by the KIPCM (through kipcm_kfa()). DO NOT TRUST THIS
 *       BEHAVIOR BURYING IT INTO THE CODE, WE WILL TRY TO GET RID OF IT
 *       ASAP
 */
struct kfa *           kipcm_kfa(struct kipcm * kipcm);
struct ipcp_instance * kipcm_find_ipcp(struct kipcm *   kipcm,
                                       ipc_process_id_t ipc_id);

struct ipcp_factory *
kipcm_ipcp_factory_register(struct kipcm *             kipcm,
                            const char *               name,
                            struct ipcp_factory_data * data,
                            struct ipcp_factory_ops *  ops);
int            kipcm_ipcp_factory_unregister(struct kipcm *        kipcm,
                                             struct ipcp_factory * factory);
/* On the destination */
int            kipcm_flow_arrived(struct kipcm *     kipcm,
                                  ipc_process_id_t   ipc_id,
                                  port_id_t          port_id,
                                  struct name *      dif_name,
                                  struct name *      source,
                                  struct name *      dest,
                                  struct flow_spec * fspec);

/* On both source and destination */
int            kipcm_notify_flow_alloc_req_result(struct kipcm *   kipcm,
                                                  ipc_process_id_t id,
                                                  port_id_t        pid,
                                                  uint_t           res);

int            kipcm_notify_flow_dealloc(ipc_process_id_t ipc_id,
                                         int8_t           code,
                                         port_id_t        port_id,
					 irati_msg_port_t irati_port);

struct ipcp_instance * kipcm_find_ipcp_by_name(struct kipcm * kipcm,
                                               struct name *  name);

struct rset * kipcm_rset(struct kipcm * kipcm);

#endif
