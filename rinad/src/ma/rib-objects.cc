//
// Management Agent RIB Daemon
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "rib-objects.h"

namespace rinad{
namespace mad{

//TODO: change these const
const unsigned int COMPUTING_SYSTEM_ID = 1;

//CLASS SimplestRIBObj
SimplestRIBObj::SimplestRIBObj(MARIBDaemon *rib_daemon,	const std::string& object_class, const std::string& object_name):
		rina::BaseRIBObject(rib_daemon, object_class, rina::objectInstanceGenerator->getObjectInstance(), object_name)
{}
const void* SimplestRIBObj::get_value() const
{
	return 0;
}
/*
// CLASS RIBObjectIDGenerator
RIBObjectIDGenerator::RIBObjectIDGenerator()
{
	id_ = 0;
}

unsigned int RIBObjectIDGenerator::get_ID()
{
    unsigned int result = 0;

    lock();
    id_++;
    result = id_;
    unlock();

    return result;
}
*/
//CLASS SimpleRIBObj
SimpleRIBObj::SimpleRIBObj(MARIBDaemon *rib_daemon,	const std::string& object_class, const std::string& object_name, unsigned int id):
		rina::BaseRIBObject(rib_daemon, object_class, rina::objectInstanceGenerator->getObjectInstance(), object_name)
{
	id_ = id;
}
const void* SimpleRIBObj::get_value() const
{
	return 0;
}

}
}
