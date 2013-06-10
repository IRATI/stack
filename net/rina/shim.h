/*
 *  Shim IPC Process
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#ifndef RINA_SHIM_H
#define RINA_SHIM_H

int  shim_init(void);
void shim_exit(void);

ipc_process_id_t shim_create(struct ipc_config_t ** config);
int              shim_destroy(ipc_process_id_t ipc_process_id);

int shim_allocate_flow_request(struct name_t *      source,
                               struct name_t *      dest,
                               struct flow_spec_t * flow_spec,
                               port_id_t            port_id);
int  shim_allocate_flow_response(port_id_t           port_id,
                                 response_reason_t * response);
int  shim_deallocate_flow(port_id_t port_id);
int  shim_register_application(struct name_t * name);
int  shim_unregister_application(struct name_t * name);
int  shim_write_sdu(port_id_t port_id, const struct sdu_t * sdu);
int  shim_ipc_create(const struct name_t * name,
                     ipc_process_id_t      ipcp_id);
int  shim_ipc_configure(ipc_process_id_t      ipcp_id,
                        const struct ipc_process_shim_ethernet_conf_t *
                        configuration);

#endif
