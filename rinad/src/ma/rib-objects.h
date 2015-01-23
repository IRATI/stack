/*
 * RIB objects of the Management Agent
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
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

#ifndef RIB_OBJECTS_H_
#define RIB_OBJECTS_H_

#include "rib-daemon.h"
#include <librina/concurrency.h>

namespace rinad{
namespace mad{

class SimplestRIBObj: public rina::BaseRIBObject{
public:
	SimplestRIBObj(MARIBDaemon *rib_daemon, const std::string &object_class, const std::string &object_name);
    const void* get_value() const;
};
/*
class RIBObjectIDGenerator: public rina::Lockable {
public:
	RIBObjectIDGenerator();
	unsigned int get_ID();
private:
	long id_;
};
*/
class SimpleRIBObj: public rina::BaseRIBObject{
public:
	SimpleRIBObj(MARIBDaemon *rib_daemon, const std::string &object_class, const std::string &object_name, unsigned int id);
    const void* get_value() const;
    unsigned int get_id() const;
private:
    unsigned int id_;
};

}
}


#endif /* RIB_OBJECTS_H_ */
