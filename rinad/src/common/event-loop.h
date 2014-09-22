/*
 * Event loop over librina
 *
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

#ifndef __EVENT_LOOP_H__
#define __EVENT_LOOP_H__

#include <iostream>
#include <map>

#include <librina/common.h>


namespace rinad {

class EventLoopData {
 public:
        virtual ~EventLoopData() { }
};

class EventLoop {
 public:
        typedef void (*EventHandler)(rina::IPCEvent *event, EventLoopData *);

        void register_pre_function(EventHandler func);
        void register_post_function(EventHandler func);
        void register_event(rina::IPCEventType type, EventHandler handler);
        void run();

        EventLoop(EventLoopData *dm);
 private:
        std::map<rina::IPCEventType, EventHandler> handlers;
        EventLoopData *data_model;
        EventHandler pre_function;
        EventHandler post_function;
};

}
#endif   /* __EVENT_LOOP_H__ */
