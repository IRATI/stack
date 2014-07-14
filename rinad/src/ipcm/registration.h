/*
 * IPC Manager - Registration and unregistration
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#ifndef __IPCM_APPLICATION_REGISTRATION_H__
#define __IPCM_APPLICATION_REGISTRATION_H__

#include <librina/common.h>
#include "event-loop.h"


namespace rinad {

void application_registration_request_event_handler(rina::IPCEvent *e,
                                                    EventLoopData *opaque);
void ipcm_register_app_response_event_handler(rina::IPCEvent *e,
                                              EventLoopData *opaque);
void application_unregistration_request_event_handler(rina::IPCEvent *e,
                                                 EventLoopData *opaque);
void ipcm_unregister_app_response_event_handler(rina::IPCEvent *e,
                                                EventLoopData *opaque);
void register_application_response_event_handler(rina::IPCEvent *event,
                                                 EventLoopData *opaque);
void unregister_application_response_event_handler(rina::IPCEvent *event,
                                                   EventLoopData *opaque);
void application_registration_canceled_event_handler(rina::IPCEvent *event,
                                                     EventLoopData *opaque);

}
#endif  /* __IPCM_APPLICATION_REGISTRATION_H__ */
