/*
 *  POSIX-like RINA API, wrapper for IRATI librina
 *
 *    Vincenzo Maffione   <v.maffione@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <string>
#include <stdlib.h>
#include <librina/librina.h>
#include <rina-api/api.h>

using namespace rina;
using namespace std;

extern "C"
{

static int initialized = 0;

static int
librina_init(void)
{
        if (initialized) {
                return 0;
        }
        initialized = 1;
        try {
                rina::initialize("INFO", "/dev/null");
                return 0;
        } catch (Exception) {
                return -1;
        }
}

int
rina_open(void)
{
        librina_init();
        return -1;
}

int
rina_register(int fd, const char *dif_name, const char *local_appl)
{
        return -1;
}

int
rina_unregister(int fd, const char *dif_name, const char *local_appl)
{
        return -1;
}

int
rina_flow_accept(int fd, const char **remote_appl)
{
        return -1;
}

int
rina_flow_alloc(const char *dif_name, const char *local_appl,
              const char *remote_appl, const struct rina_flow_spec *flowspec)
{
        return -1;
}

void
rina_flow_spec_default(struct rina_flow_spec *spec)
{
}

}
