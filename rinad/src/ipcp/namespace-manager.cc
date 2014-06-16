//
// Namespace Manager
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
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

#include <sstream>

#include "namespace-manager.h"

namespace rinad {

// CLASS DirectoryForwardingTableEntry	*/
DirectoryForwardingTableEntry::DirectoryForwardingTableEntry() {
	address_ = 0;
	timestamp_ = 0;
}

rina::ApplicationProcessNamingInformation DirectoryForwardingTableEntry::get_ap_naming_info() const {
	return ap_naming_info_;
}

void DirectoryForwardingTableEntry::set_ap_naming_info(
		const rina::ApplicationProcessNamingInformation &ap_naming_info) {
	ap_naming_info_ = ap_naming_info;
}

long DirectoryForwardingTableEntry::get_address() const {
	return address_;
}

void DirectoryForwardingTableEntry::set_address(long address) {
	address_ = address;
}

long DirectoryForwardingTableEntry::get_timestamp() const {
	return timestamp_;
}

void DirectoryForwardingTableEntry::set_timestamp(long timestamp) {
	timestamp_ = timestamp;
}

std::string DirectoryForwardingTableEntry::getKey() {
	return ap_naming_info_.getEncodedString();
}

bool DirectoryForwardingTableEntry::operator==(const DirectoryForwardingTableEntry &object) {
	if (object.get_ap_naming_info() != ap_naming_info_) {
		return false;
	}

	if (object.get_address() != address_) {
		return false;
	}
	return true;
}

std::string DirectoryForwardingTableEntry::toString() {
    std::stringstream ss;
    ss << this->get_ap_naming_info().toString() << std::endl;
	ss << "IPC Process address: " << address_ << std::endl;
	ss << "Timestamp: " << timestamp_ << std::endl;

	return ss.str();
}

//	CLASS WhatevercastName
const std::string WhatevercastName::WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME = RIBObjectNames::SEPARATOR + RIBObjectNames::DAF +
RIBObjectNames::SEPARATOR + RIBObjectNames::MANAGEMENT + RIBObjectNames::SEPARATOR + RIBObjectNames::NAMING +
RIBObjectNames::SEPARATOR + RIBObjectNames::WHATEVERCAST_NAMES;
const std::string WhatevercastName::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS = "whatname set";
const std::string WhatevercastName::WHATEVERCAST_NAME_RIB_OBJECT_CLASS = "whatname";
const std::string WhatevercastName::DIF_NAME_WHATEVERCAST_RULE = "any";

std::string WhatevercastName::get_name() const {
	return name_;
}

void WhatevercastName::set_name(std::string name) {
	name_ = name;
}

std::string WhatevercastName::get_rule() const {
	return rule_;
}

void WhatevercastName::set_rule(std::string rule) {
	rule_ = rule;
}

const std::list<std::string>& WhatevercastName::get_set_members() const {
	return set_members_;
}

void WhatevercastName::set_set_members(const std::list<std::string>& set_members) {
	set_members_ = set_members;
}

bool WhatevercastName::operator==(const WhatevercastName &other) {
	if (name_ == other.get_name()) {
		return true;
	}
	return false;
}

std::string WhatevercastName::toString() {
	std::string result = "Name: " + name_ + "\n";
	result = result + "Rule: " + rule_;
	return result;
}


}
