/*
 * Debugging utils
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#ifndef RINAD_DEBUG_H
#define RINAD_DEBUG_H

#include <string>

void dump_backtrace(void);

#ifndef FLUSH_LOG
	//Force log flushing
	#define FLUSH_LOG(_lev_, _ss_)\
			do{\
				LOGF_##_lev_ ("%s", (_ss_).str().c_str());\
				ss.str(std::string());\
			}while (0)
#endif //FLUSH_LOG

#endif
