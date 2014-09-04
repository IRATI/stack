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

#include "Application.hpp"

#include <iostream>
#include <librina/librina.h>

using namespace std;
using namespace rina;

Application::Application(const string& app_name_,
                         const string& app_instance_) :
        app_name(app_name_),
        app_instance(app_instance_)
{ }

void Application::applicationRegister()
{
        ApplicationRegistrationInformation
                ari(ApplicationRegistrationType::APPLICATION_REGISTRATION_ANY_DIF);

        ari.appName = ApplicationProcessNamingInformation(app_name,
                                                          app_instance);

        try {
                ipcManager->requestApplicationRegistration(ari);
        } catch(ApplicationRegistrationException e) {
                cerr << e.what() << endl;
        }
}

const uint Application::max_buffer_size = 1024;
