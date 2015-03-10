//
// Common librina classes for IPC Process and IPC Manager daemons
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <sstream>

#define RINA_PREFIX "ipc-daemons"

#include "librina/logs.h"
#include "librina/ipc-daemons.h"

namespace rina {

/* CLASS NEIGHBOR */
Neighbor::Neighbor() {
	address_ = false;
	average_rtt_in_ms_ = 0;
	last_heard_from_time_in_ms_ = 0;
	enrolled_ = false;
	underlying_port_id_ = 0;
	number_of_enrollment_attempts_ = 0;
}

bool Neighbor::operator==(const Neighbor &other) const{
	return name_ == other.get_name();
}

bool Neighbor::operator!=(const Neighbor &other) const{
	return !(*this == other);
}

const ApplicationProcessNamingInformation&
Neighbor::get_name() const {
	return name_;
}

void Neighbor::set_name(
		const ApplicationProcessNamingInformation& name) {
	name_ = name;
}

const ApplicationProcessNamingInformation&
Neighbor::get_supporting_dif_name() const {
	return supporting_dif_name_;
}

void Neighbor::set_supporting_dif_name(
		const ApplicationProcessNamingInformation& supporting_dif_name) {
	supporting_dif_name_ = supporting_dif_name;
}

const std::list<ApplicationProcessNamingInformation>&
Neighbor::get_supporting_difs() {
	return supporting_difs_;
}

void Neighbor::set_supporting_difs(
		const std::list<ApplicationProcessNamingInformation>& supporting_difs) {
	supporting_difs_ = supporting_difs;
}

void Neighbor::add_supporting_dif(
		const ApplicationProcessNamingInformation& supporting_dif) {
	supporting_difs_.push_back(supporting_dif);
}

unsigned int Neighbor::get_address() const {
	return address_;
}

void Neighbor::set_address(unsigned int address) {
	address_ = address;
}

unsigned int Neighbor::get_average_rtt_in_ms() const {
	return average_rtt_in_ms_;
}

void Neighbor::set_average_rtt_in_ms(unsigned int average_rtt_in_ms) {
	average_rtt_in_ms_ = average_rtt_in_ms;
}

bool Neighbor::is_enrolled() const {
	return enrolled_;
}

void Neighbor::set_enrolled(bool enrolled){
	enrolled_ = enrolled;
}

int Neighbor::get_last_heard_from_time_in_ms() const {
	return last_heard_from_time_in_ms_;
}

void Neighbor::set_last_heard_from_time_in_ms(int last_heard_from_time_in_ms) {
	last_heard_from_time_in_ms_ = last_heard_from_time_in_ms;
}

int Neighbor::get_underlying_port_id() const {
	return underlying_port_id_;
}

void Neighbor::set_underlying_port_id(int underlying_port_id) {
	underlying_port_id_ = underlying_port_id;
}

unsigned int Neighbor::get_number_of_enrollment_attempts() const {
	return number_of_enrollment_attempts_;
}

void Neighbor::set_number_of_enrollment_attempts(
		unsigned int number_of_enrollment_attempts) {
	number_of_enrollment_attempts_ = number_of_enrollment_attempts;
}

const std::string Neighbor::toString(){
	std::stringstream ss;

	ss<<"Address: "<<address_;
	ss<<"; Average RTT(ms): "<<average_rtt_in_ms_;
	ss<<"; Is enrolled: "<<enrolled_<<std::endl;
	ss<<"Name: "<<name_.toString()<<std::endl;
	ss<<"Supporting DIF in common: "<<supporting_dif_name_.processName;
	ss<<"; N-1 port-id: "<<underlying_port_id_<<std::endl;
	ss<<"List of supporting DIFs: ";
	for (std::list<ApplicationProcessNamingInformation>::iterator it = supporting_difs_.begin();
			it != supporting_difs_.end(); it++)
		ss<< it->processName << "; ";
	ss<<std::endl;
	ss<<"Last heard from time (ms): "<<last_heard_from_time_in_ms_;
	ss<<"; Number of enrollment attempts: "<<number_of_enrollment_attempts_;

	return ss.str();
}

/* CLASS ASSIGN TO DIF RESPONSE EVENT */
AssignToDIFResponseEvent::AssignToDIFResponseEvent(
                int result, unsigned int sequenceNumber):
                        BaseResponseEvent(result,
                                        ASSIGN_TO_DIF_RESPONSE_EVENT,
                                        sequenceNumber) {
}

/* CLASS SET POLICY SET PARAM RESPONSE EVENT */
SetPolicySetParamResponseEvent::SetPolicySetParamResponseEvent(
                int result, unsigned int sequenceNumber) :
			IPCEvent(IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE,
                                         sequenceNumber)
{ this->result = result; }

/* CLASS SELECT POLICY SET RESPONSE EVENT */
SelectPolicySetResponseEvent::SelectPolicySetResponseEvent(
                int result, unsigned int sequenceNumber) :
			IPCEvent(IPC_PROCESS_SELECT_POLICY_SET_RESPONSE,
                                         sequenceNumber)
{ this->result = result; }

/* CLASS PLUGIN LOAD RESPONSE EVENT */
PluginLoadResponseEvent::PluginLoadResponseEvent(
                int result, unsigned int sequenceNumber) :
			IPCEvent(IPC_PROCESS_PLUGIN_LOAD_RESPONSE,
                                         sequenceNumber)
{ this->result = result; }

}

