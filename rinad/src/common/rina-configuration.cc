/*
 * RINA configuration tree
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

#include <librina/common.h>
#include "rina-configuration.h"

#include <string>
#include <list>
#include <map>


using namespace std;

namespace rinad {

/*
 * Return the configuration of the DIF named "difName" if it is known
 * @param difName
 * @result
 */
bool RINAConfiguration::getDIFConfiguration(const std::string& difName,
                                            DIFProperties& result) const
{
        if (!difConfigurations.size()){
                return false;
        }

        for (list<DIFProperties>::const_iterator it = difConfigurations.begin();
                                        it != difConfigurations.end(); it++) {
                if (it->difName == difName) {
                        result = *it;
                        return true;
                }
        }

        return false;
}

std::string LocalConfiguration::toString() const
{
        std::ostringstream ss;

        ss <<  "Local Configuration\n";
        ss << "   Installation path: " << installationPath << std::endl;
        ss << "   Library path: " << libraryPath << std::endl;
        ss << "   Console port: " << consolePort << std::endl;
        ss << "   CDAP timeout in ms: " << cdapTimeoutInMs << std::endl;
        ss << "   Enrollment timeout in ms: " << enrollmentTimeoutInMs << std::endl;
        ss << "   Flow allocator timeout in ms:  " << flowAllocatorTimeoutInMs << std::endl;
        ss << "   Watchdog period in ms: " << watchdogPeriodInMs << std::endl;
        ss << "   Declared dead interval in ms: " << declaredDeadIntervalInMs << std::endl;
        ss << "   Neighbors enroller period in ms: " << neighborsEnrollerPeriodInMs << std::endl;

        return ss.str();
}

std::string RINAConfiguration::toString() const
{
        return localConfiguration.toString();
}

}

