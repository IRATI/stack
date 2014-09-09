/*
 * Echo Application
 *
 * Addy Bombeke <addy.bombeke@ugent.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <string>

class Application
{
public:
        Application(const std::string& dif_name_,
                    const std::string & app_name_,
                    const std::string & app_instance_);

        static const uint max_buffer_size;

protected:
        void applicationRegister();

        std::string dif_name;
        std::string app_name;
        std::string app_instance;

};
#endif
