//
// Manager
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Bernat Gastón <bernat.gaston@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <string>
#include <librina/application.h>
#include <librina/cdap_v2.h>

static const unsigned int max_sdu_size_in_bytes = 10000;

class Application
{
 public:
        Application(const std::string& dif_name_, const std::string & app_name_,
                    const std::string & app_instance_);

        static const uint max_buffer_size;

 protected:
        void applicationRegister();

        std::string dif_name;
        std::string app_name;
        std::string app_instance;

};

class ConnectionCallback : public rina::cdap::CDAPCallbackInterface
{
 public:
        ConnectionCallback(rina::cdap::CDAPProviderInterface **prov);
        void open_connection(const rina::cdap_rib::con_handle_t &con,
                             const rina::cdap_rib::flags_t &flags,
                             int message_id);
        void remote_create_result(const rina::cdap_rib::con_handle_t &con,
                                  const rina::cdap_rib::res_info_t &res);
 private:
        rina::cdap::CDAPProviderInterface **prov_;
};

class Manager : public Application
{
 public:
        Manager(const std::string& dif_name, const std::string& apn,
                const std::string& api, bool quiet);
        void run();
        ~Manager();
 protected:
        void startWorker(rina::Flow *flow);
        void createIPCP(rina::Flow *flow);

        //void cacep();
        //void sendAssignToDIF();
        //void sendRegisterAtDIF();
        //void sendQueryRIB();
        //void release();
        //void destroyFlow();

 private:
        std::string dif_name_;
        bool client_app_reg_;
        rina::cdap_rib::con_handle_t con_;
        static const std::string mad_name;
        static const std::string mad_instance;
};
#endif//MANAGER_HPP
