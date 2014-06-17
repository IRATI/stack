/*
 * Namespace Manager
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef IPCP_NAMESPACE_MANAGER_HH
#define IPCP_NAMESPACE_MANAGER_HH

#ifdef __cplusplus

#include "components.h"

namespace rinad {

/// Defines a whatevercast name (or a name of a set of names).
/// In traditional architectures, sets that returned all members were called multicast; while
/// sets that returned one member were called anycast.  It is not clear what sets that returned
/// something in between were called.  With the more general definition here, these
/// distinctions are unnecessary.
class WhatevercastName {
public:
	static const std::string WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME;
	static const std::string WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS;
	static const std::string WHATEVERCAST_NAME_RIB_OBJECT_CLASS;
	static const std::string DIF_NAME_WHATEVERCAST_RULE;

	std::string get_name() const;
	void set_name(std::string name);
	std::string get_rule() const;
	void set_rule(std::string rule);
	const std::list<std::string>& get_set_members() const;
	void set_set_members(const std::list<std::string>& set_members);
	bool operator==(const WhatevercastName &other);
	std::string toString();

private:
	/// The name
	std::string name_;

	/// The members of the set
	std::list<std::string> set_members_;

	/// The rule to select one or more members from the set
	std::string rule_;
};

}

#endif

#endif
