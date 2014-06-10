/*
 * Options parsing
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef RINAD_OPTIONS_H
#define RINAD_OPTIONS_H

#ifdef __cplusplus

#include <string>

namespace rinad {

// FIXME: Just started

class option {
 public:
        option(const std::string & lng,
               const std::string & shrt);

 private:
        std::string long_;
        std::string short_;
};

}

#endif

#endif
