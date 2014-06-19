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
