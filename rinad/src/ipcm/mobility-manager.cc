/*
 * Mobility Manager (manages mobility locally in a system)
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#define RINA_PREFIX     "ipcm.mobility-manager"
#include <librina/logs.h>

#include "mobility-manager.h"

using namespace std;

namespace rinad {

//Class Mobility Manager
MobilityManager::MobilityManager(IPCMIPCProcessFactory * ipcp_factory)
{
	factory = ipcp_factory;
	LOG_INFO("Mobility Manager initialized");
}

void MobilityManager::media_reports_handler(rina::MediaReportEvent * event)
{
	LOG_DBG("Received new media report %s", event->media_report.toString().c_str());
}

} //namespace rinad
