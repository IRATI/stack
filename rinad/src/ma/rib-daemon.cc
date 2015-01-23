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

#include "rib-daemon.h"
#include "librina/rib.h"

namespace rinad{
namespace mad{

//TODO: change these const
const unsigned int COMPUTING_SYSTEM_ID = 1;

//CLASS MARIBDaemon
MARIBDaemon::MARIBDaemon (const rina::RIBSchema *schema): rina::RIBDaemon(schema){

}

void MARIBDaemon::sendMessageSpecific(bool useAddress, const rina::CDAPMessage & cdapMessage, int sessionId,
                unsigned int address, rina::ICDAPResponseMessageHandler * cdapMessageHandler) {
}

}
}
