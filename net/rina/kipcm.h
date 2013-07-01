/*
 * K-IPCM (Kernel-IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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
#include "shim.h"
#include "efcp.h"
#include "rmt.h"

typedef enum {
        DIF_TYPE_NORMAL,
        DIF_TYPE_SHIM
} dif_type_t;

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

struct normal_ipc_process_t {
	/*
         * Contains all the data structures of a normal IPC Process 
         */

	/* The ID of the IPC Process */
	ipc_process_id_t          ipcp_id; 
	
	/* Contains the configuration of the kernel components */
	struct ipc_process_conf * configuration;

	/* The EFCP data structure associated to the IPC Process */
	struct efcp_ipc_t *       efcp;
	
	/* The RMT instance associated to the IPC Process */
	struct rmt_instance_t *   rmt;
};

struct ipc_process_t {
	dif_type_t type;

	union {
		struct normal_ipc_process_t * normal_ipcp;
#if 0
		struct shim_instance *        shim_instance;
#endif
	} data;
};

struct flow {
	/* The port-id identifying the flow */
	port_id_t              port_id;

	/*
         * The components of the IPC Process that will handle the
         * write calls to this flow
         */
	struct ipc_process_t * ipc_process;

	/*
         * True if this flow is serving a user-space application, false
         * if it is being used by an RMT
         */
	bool_t                 application_owned;

	/*
         * In case this flow is being used by an RMT, this is a pointer
         * to the RMT instance.
         */
	struct rmt_instance_t  rmt_instance;

	//FIXME: Define QUEUE
	//QUEUE(segmentation_queue, pdu_t *);
	//QUEUE(reassembly_queue,	pdu_t *);
	//QUEUE(sdu_ready, sdu_t *);
	struct kfifo *         sdu_ready;
};

struct kipcm {
        struct shims * shims;
};

struct kipcm * kipcm_init(struct kobject * parent);
int            kipcm_fini(struct kipcm * kipcm);

struct shim *  kipcm_shim_register(struct kipcm *    kipcm,
                                   const char *      name,
                                   void *            data,
                                   struct shim_ops * ops);
int            kipcm_shim_unregister(struct kipcm * kipcm,
                                     struct shim *  shim);

int            kipcm_ipc_create(struct kipcm *        kipcm,
                                const struct name_t * name,
                                ipc_process_id_t      id,
                                dif_type_t            type);
int            kipcm_ipc_configure(struct kipcm *                  kipcm,
                                   ipc_process_id_t                id,
                                   const struct ipc_process_conf * config);
int            kipcm_ipc_destroy(struct kipcm *   kipcm,
                                 ipc_process_id_t id);

int            kipcm_flow_add(struct kipcm *      kipcm,
                              port_id_t           id,
                              const struct flow * flow);
int            kipcm_flow_remove(struct kipcm * kipcm,
                                 port_id_t      id);

int            kipcm_sdu_write(struct kipcm *       kipcm,
                               port_id_t            id,
                               const struct sdu_t * sdu);
int            kipcm_sdu_read(struct kipcm * kipcm,
                              port_id_t      id,
                              struct sdu_t * sdu);

#endif
